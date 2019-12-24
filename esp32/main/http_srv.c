/*
 * http_srv.c
 * Simple http server.
 * Returns the contents of a file from the file system by URL.
 * SPIFFS does not support directories!
 */

#include <stdint.h>
#include <string.h>
#include <esp_err.h>
#include <sdkconfig.h>
#include <esp_log.h>
#include <sys/param.h>
#include <esp_http_server.h>
#include <dirent.h>
#include <sys/stat.h>
#include <sys/types.h>

#include "http_srv.h"
#include "wifi.h"
#include "utils.h"
#include "esp_spiffs.h"
#include "params.h"

static const char *TAG = "HTTP";

//#define FS_TYPE			SPIFFS //FAT_FS
//#define FS_TYPE			FAT_FS

#define DIR_SPLASH			"/"
#define ROOT_FILE_NAME		"index.html"

#if (FS_TYPE == SPIFFS)
#define BASE_PATH  			"/spiffs"
#endif

httpd_handle_t http_server = NULL;

httpd_uri_t *uriGetList;			//List url from server
uint8_t numberUri;					//number of Uri in uriGetList

char* putsValue(char *toStr, char *varName, size_t *lenVal) {
	uint8_t lenValue = paramRead(varName, NULL);
	if (lenValue) {
		paramRead(varName, toStr);
		toStr += lenValue;
		*lenVal += lenValue;		//add length value in common length page
	}
	return toStr;
}

//Only not big file
char* read_file(const char *fileName, size_t *size_ptr, esp_err_t *err_ptr) {

	char *fileName_full = calloc(strlen(BASE_PATH) + strlen(DIR_SPLASH) + strlen(fileName) + 1, sizeof(char));
	*err_ptr = ESP_ERR_NOT_FOUND;
	char *ret = NULL;

	if (fileName_full) {
		strcat(fileName_full, BASE_PATH);
		strcat(fileName_full, DIR_SPLASH);
		strcat(fileName_full, fileName);
		FILE *f = fopen(fileName_full, "r");
		if (f) {
			struct stat statistics;
			if (stat(fileName_full, &statistics) != -1) {
				if (statistics.st_size) {
					ret = (char*) calloc(statistics.st_size + 1, sizeof(char));
					*err_ptr = ESP_ERR_NO_MEM;
					if (ret) {
						size_t read = fread((void*) ret, 1, statistics.st_size, f);
						if (read != statistics.st_size) {
							free(ret);
						} else {
							*err_ptr = ESP_OK;
							*size_ptr = statistics.st_size;
						}
					}
				}
			}
			fclose(f);
		}
	}
	return ret;
}

// @formatter:off
enum fileType {
	ftBinary = 0,
	ftText = 1
};
// @formatter:on
char* getTypeFile(const char *fileName, enum fileType *isTextFile) {
	*isTextFile = ftBinary;

	if (fileName) {
		char *dotPos = strrchr(fileName, '.');

		if (dotPos) {
			if (				// @formatter:off
			!strcmp(dotPos, ".html") ||
			!strcmp(dotPos, ".htm") ||
			!strcmp(dotPos, ".js") ||
			!strcmp(dotPos, ".txt")) {
			// @formatter:on
				*isTextFile = ftText;
				return HTTPD_TYPE_TEXT;
			}
			if (!strcmp(dotPos, ".ico")) {
				return "image/vnd.microsoft.icon";
			}
		}
	}
	return HTTPD_TYPE_OCTET;
}

/*
 * Parse file page, find variable name and full value
 */
char* parse_page(char *fileName, size_t *fileSize, esp_err_t *err) {

#define	LEN_VAR_ADD  		500	//Резерв для переменных из файла TODO: переделать на точный подсчет
#define LEN_VAR_MAX			16	//Максимальная длина имени переменной, вместе с оканчивающемся нулем
#define SYMBOL_DELEMITER	'~'	//Разделитель для имени переменной

	char *str = NULL, *ret = NULL, *fromPos, *currPos, *srcPos;
	char varName[LEN_VAR_MAX];
	size_t lenVal = 0, lenNew = 0;

	str = read_file(fileName, fileSize, err);
	if (*err == ESP_OK) {
		ret = (char*) calloc(*fileSize + LEN_VAR_ADD, 1);
		lenNew = *fileSize;
		if (ret) {
			fromPos = ret;
			srcPos = str;
			currPos = cmpcpystr(srcPos, SYMBOL_DELEMITER, SYMBOL_DELEMITER, varName, LEN_VAR_MAX, *fileSize);
			while (currPos) {
				strncpy(fromPos, srcPos, (currPos - strlen(varName) - 1) - srcPos); 	//Кусок файла до переменной
				fromPos += (currPos - strlen(varName) - 1) - srcPos;					//позицию на это место
				fromPos = putsValue(fromPos, varName, &lenVal);							//Дописываем значение переменной
				lenNew -= strlen(varName) + 2;											//Минус длина найденной переменной в размере файла
				srcPos = ++currPos;														//Терминатор пропускаем
				currPos = cmpcpystr(currPos, SYMBOL_DELEMITER, SYMBOL_DELEMITER, varName, LEN_VAR_MAX, *fileSize); //
			}
			*fileSize = lenNew + lenVal;									//Новый размер = старый минус имена переменных + подставленные значения переменных
			strcat(fromPos, srcPos);
		}
	}
	if (ret == NULL) {
		if (str) {
			free(str);
			str = NULL;
			*err = ESP_ERR_NOT_FOUND;
		}
	}
	return ret;
}

/*
 * parse variable http-query
 */
esp_err_t parse_query(httpd_req_t *req) {
#define VALUE_MAX_LEN 	128 	//maximum length variable value
#define VALUE_COUNT_MAX	6		//maximum number variables
#define BUF_LEN			VALUE_MAX_LEN*VALUE_COUNT_MAX*2	//size bufer variable

	char *buf, *pair_ptr, *value_ptr, *pair_buf, *val;
	buf = calloc(BUF_LEN, 1);
	if (buf == NULL) {
		return ESP_ERR_NO_MEM;
	}
	int ret, remaining = req->content_len;

	paramName_t paramName;
	handler_save saveFunc = NULL, saveFuncPrev = NULL;

	esp_err_t resultSave = ESP_OK;

	while (remaining > 0) {
		/* Read the data for the request */
		if ((ret = httpd_req_recv(req, buf, MIN(remaining, BUF_LEN))) <= 0) {
			ESP_LOGI(TAG, "ret = %d", ret);
			if (ret == HTTPD_SOCK_ERR_TIMEOUT) {
				/* Retry receiving if timeout occurred */
				continue;
			}
			free(buf);
			return ESP_FAIL;
		}
		saveFunc = NULL;
		saveFuncPrev = NULL;
		pair_buf = strtok_r(buf, "&", &pair_ptr);
		while (pair_buf) {
			paramName = strtok_r(pair_buf, "=", &value_ptr);
			if (paramName) {
				val = strtok_r(NULL, "=", &value_ptr);
				ESP_LOGI(TAG, "param query = %s val = %s", paramName, val);
				if (paramWrite(paramName, val, &saveFunc) == ESP_OK){
					if (saveFuncPrev == NULL){
						saveFuncPrev = saveFunc;
					}
					else if (saveFunc != saveFuncPrev){
						(*saveFuncPrev)();
						saveFuncPrev = saveFunc;
					}
				}
			}
			pair_buf = strtok_r(NULL, "&", &pair_ptr);
		};
		if (saveFunc){
			(*saveFunc)();
		}
		remaining -= ret;
	}
	free(buf);
	return resultSave;
}

/*
 * Process uri
 * Processes the parameters if any.
 * returns the resource if it finds.
 */
esp_err_t get_page_handler(httpd_req_t *req) {

	char *resp = NULL;
	size_t fileSize;
	esp_err_t err;
	char *fileName;
	enum fileType typeFile;

	if (req->user_ctx) {
		ESP_LOGI(TAG, "con_len = %d", req->content_len);
		if (req->content_len > 0) {	//query process
			if (parse_query(req) != ESP_OK) {
				httpd_resp_send(req, HTTPD_500, sizeof(HTTPD_500));
				return ESP_OK;
			}
		}
		//find resource in file system
		size_t fnLen = strlen((char*) req->user_ctx);
		if (fnLen == 0) {
			ESP_LOGI(TAG, "user_ctx is empty");
			httpd_resp_send(req, HTTPD_500, sizeof(HTTPD_500));
			return ESP_OK;
		}
		ESP_LOGI(TAG, "req uri = %s", req->uri);
		ESP_LOGI(TAG, "user_ctx = %s", (char* ) req->user_ctx);
		fileName = (char*) req->user_ctx;
		ESP_LOGI(TAG, "typeHdr = %s", getTypeFile(fileName, &typeFile));
		httpd_resp_set_type(req, getTypeFile(fileName, &typeFile));

		ESP_LOGI(TAG, "uri file = %s", fileName);
		if (typeFile == ftText) {
			resp = parse_page(fileName, &fileSize, &err);
			ESP_LOGI(TAG, "size = %d content %s", fileSize, resp);
		} else {
			resp = read_file(fileName, &fileSize, &err);
			ESP_LOGI(TAG, "size = %d binary file", fileSize);
		}

		if (err == ESP_OK) {
			httpd_resp_send(req, resp, fileSize);
			if (httpd_req_get_hdr_value_len(req, "Host") == 0) {	//TODO: Del
				ESP_LOGI(TAG, "Request headers lost");
			}
		} else {
			httpd_resp_send(req, HTTPD_404, sizeof(HTTPD_404));
		}
	}
	if (resp != NULL) {
		free(resp);
	}
	return ESP_OK;
}

uint8_t countFileFromDir(char *dir_name) {
	uint8_t ret = 0;
	struct dirent *entry;
	char *dir_full = calloc(strlen(BASE_PATH) + strlen(DIR_SPLASH) + strlen(dir_name) + 1, sizeof(char));

	strcat(dir_full, BASE_PATH);
	strcat(dir_full, dir_name);
	DIR *dir_ptr = opendir(dir_full);
	ESP_LOGI(TAG, "Open dir %s", dir_full);
	if (dir_ptr) {
		while ((entry = readdir(dir_ptr)) != NULL) {
			if (entry->d_type == DT_DIR) {
				uint16_t nextLen = strlen(BASE_PATH) + strlen(DIR_SPLASH) + strlen(dir_name) + strlen(entry->d_name) + strlen(DIR_SPLASH) + 1;
				if (nextLen <= FILENAME_MAX) {
					char *nextDir = (char*) calloc(nextLen, sizeof(char));
					strcat(nextDir, dir_name);
					strcat(nextDir, DIR_SPLASH);
					strcat(nextDir, entry->d_name);
					ret += countFileFromDir(nextDir);
					free(nextDir);
				}
			} else {
				ret++;
			}
		}
		closedir(dir_ptr);
	}
	free(dir_full);
	return ret;
}

bool urlRegister(httpd_handle_t server, char *url, char *fileName, httpd_uri_t *urlList) {
	char *uri = calloc(strlen(DIR_SPLASH) + strlen(url) + 2, 1);
	if (uri) {
		urlList++;
		strcat(uri, DIR_SPLASH);
		strcat(uri, url);
		//GET method
		urlList->method = HTTP_GET;
		urlList->user_ctx = (void*) fileName;
		urlList->handler = get_page_handler;
		urlList->uri = uri;
		httpd_register_uri_handler(server, urlList);
		//POST method
		urlList->method = HTTP_POST;
		httpd_register_uri_handler(server, urlList);
		ESP_LOGI(TAG, "Registering URI %s file name %s", urlList->uri, (char* )urlList->user_ctx);
		return true;
	}
	return false;
}

void urlsListReg(httpd_handle_t server, char *dir_name, httpd_uri_t *urlList) {

	struct dirent *entry;
	char *dir_full = calloc(strlen(BASE_PATH) + strlen(DIR_SPLASH) + strlen(dir_name) + 1, sizeof(char));

	strcat(dir_full, BASE_PATH);
	strcat(dir_full, dir_name);
	DIR *dir_ptr = opendir(dir_full);
	if (dir_ptr) {
		while ((entry = readdir(dir_ptr)) != NULL) {
			if (entry->d_type == DT_DIR) {
				uint16_t nextLen = strlen(dir_name) + strlen(entry->d_name) + strlen(DIR_SPLASH) + 1;
				if (nextLen > FILENAME_MAX) {
					break;
				}
				char *nextDir = (char*) calloc(nextLen, sizeof(char));
				strcat(nextDir, dir_name);
				strcat(nextDir, DIR_SPLASH);
				strcat(nextDir, entry->d_name);
				urlsListReg(server, nextDir, urlList);
			} else {
				if (!urlRegister(server, entry->d_name, entry->d_name, urlList)) {
					break;
				}
			}
		}
		closedir(dir_ptr);
	}
	free(dir_full);
}

void freeUrlsList(void) {
	uint8_t i;

	if (uriGetList) {
		if (numberUri) {
			for (i = 0; i <= numberUri; i++) {		//Именно от 0 до count, т.к. регестрируется пустой uri "/"
				free((char*) uriGetList->uri);
				uriGetList++;
			}
			free(uriGetList);
			uriGetList = NULL;
			numberUri = 0;
		}
	}
}

bool start_webserver(void) {
	httpd_config_t config = HTTPD_DEFAULT_CONFIG();
	config.max_uri_handlers = 1024;
	config.max_resp_headers = 1024;

	ESP_LOGI(TAG, "Starting server on port: '%d'", config.server_port);
	if (!http_server) {
		if (httpd_start(&http_server, &config) != ESP_OK) {
			return false;
		}
	}
	ESP_LOGI(TAG, "httpd_start Ok");
	if (!esp_spiffs_mounted(NULL)) {
		// @formatter:off
		esp_vfs_spiffs_conf_t conf = {
			.base_path = BASE_PATH,
			.partition_label = NULL,
			.max_files = 5,
			.format_if_mount_failed = false};
		// @formatter:on
		if (esp_vfs_spiffs_register(&conf) != ESP_OK) {
			return false;
		}
	}
	ESP_LOGI(TAG, "spiffs Ok");
	// Set URI handlers
	freeUrlsList(); //Old clear
	ESP_LOGI(TAG, "old free Ok");
	numberUri = countFileFromDir(DIR_SPLASH);
	ESP_LOGI(TAG, "num Uri = %d", numberUri);
	if (numberUri) {
		ESP_LOGI(TAG, "Registering URI handlers");
		uriGetList = (httpd_uri_t*) calloc(numberUri + 1, sizeof(httpd_uri_t));
		if (uriGetList) {
			ESP_LOGI(TAG, "Registering URI start");
			if (urlRegister(http_server, "", ROOT_FILE_NAME, uriGetList)) {
				urlsListReg(http_server, DIR_SPLASH, uriGetList);
			}
			return true;
		}
	}
	return true;
}

void stop_webserver(void) {
	if (http_server) {
		httpd_stop(http_server);
		freeUrlsList();
	}
}
