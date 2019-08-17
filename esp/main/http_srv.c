/*
 * http_srv.c
 *
 *  Created on: 9 июн. 2019 г.
 *      Author: Administrator
 */

#include <string.h>
#include "esp_err.h"
#include "sdkconfig.h"
#include "esp_log.h"
#include "sys/param.h"
#include "http_srv.h"
#include "wifi.h"
#include "esp_http_server.h"
#include "utils.h"
#include "my_fs.h"
#include "params.h"
#include "nvs_params.h"

static const char *TAG="HTTP:";

httpd_handle_t http_server = NULL;

file_system_t* 	my_fs;
httpd_uri_t*	uriGetList;

/*
 * Заполняет заначения переменных найденых на странице
 */
char* parse_page(char* fileName, size_t *fileSize, esp_err_t *err){

#define	LEN_VAR_ADD  		500	//Резерв для переменных из файла TODO: переделать на точный подсчет
#define LEN_VAR_MAX		16	//Максимальная длина имени переменной, вместе с оканчивающемся нулем
#define SYMBOL_DELEMITER	'~'	//Разделитель для имени переменной

  char *str = NULL, *ret = NULL, *fromPos, *currPos, *srcPos;
  char varName[LEN_VAR_MAX];
  size_t lenVal = 0, lenNew = 0;

  str = read_file(my_fs, fileName, fileSize, err);
  if (*err == ESP_OK){
    ret = (char *) calloc(*fileSize+LEN_VAR_ADD, 1);
    lenNew = *fileSize;
    if (ret){
      fromPos = ret;
      srcPos = str;
      currPos = cmpcpystr(srcPos, SYMBOL_DELEMITER, SYMBOL_DELEMITER, varName, LEN_VAR_MAX, *fileSize);
      while (currPos){
	strncpy(fromPos, srcPos, (currPos-strlen(varName)-1)-srcPos); 	//Кусок файла до переменной
	fromPos += (currPos-strlen(varName)-1)-srcPos;			//позицию на это место
	fromPos = putsValue(fromPos, varName, &lenVal);			//Дописываем значение переменной
	if (fromPos == NULL){
	  fromPos += (currPos-strlen(varName)-1)-srcPos;		//Обратно вернуть указатель
	}
	lenNew -= strlen(varName)+2;					//Минус длина найденной переменной в размере файла
	srcPos = ++currPos;						//Терминатор пропускаем
	currPos = cmpcpystr(currPos, SYMBOL_DELEMITER, SYMBOL_DELEMITER, varName, LEN_VAR_MAX, *fileSize); //
      }
      *fileSize = lenNew + lenVal;					//Новый размер = старый минус имена переменных + подставленные значения переменных
      strcat(fromPos, srcPos);
    }
  }
  if (ret == NULL){
    if (str){
      free(str);
      str = NULL;
      *err = ESP_ERR_NOT_FOUND;
    }
  }
  return ret;
}

enum fileType{
  ftBinary = 0,
  ftText = 1
};

char * getTypeFile(const char *fileName, enum fileType* isTextFile){
  char* dotPos = strrchr(fileName, '.');

  *isTextFile = ftBinary;

  if (dotPos){
    if (!strcmp(dotPos, ".html")
	||
	!strcmp(dotPos, ".htm")
	||
	!strcmp(dotPos, ".js")
	||
	!strcmp(dotPos, ".txt")
	){
      *isTextFile = ftText;
      return HTTPD_TYPE_TEXT;
    }
    if (!strcmp(dotPos, ".ico")){
      return "image/vnd.microsoft.icon";
    }
  }
  return HTTPD_TYPE_OCTET;
}

/*
 * Разбор и применение переменных запроса
 */
esp_err_t parse_query(httpd_req_t *req)
{
  #define VALUE_MAX_LEN 	128 	//максимальная длина значения переменной
  #define VALUE_COUNT_MAX	6	//максимальное количество переменых
  #define BUF_LEN		VALUE_MAX_LEN*VALUE_COUNT_MAX*2	//размер с запасом

  char* buf, value[VALUE_MAX_LEN];
  buf = calloc(BUF_LEN, 1);
  if (buf == NULL){
    return ESP_ERR_NO_MEM;
  }
  int ret, remaining = req->content_len;

  seechRecord_t sr;
  char* varName;
  esp_err_t resultSave = ESP_OK;

  uint8_t wifi_save = 0, cayenn_save = 0;

  while (remaining > 0) {
    /* Read the data for the request */
    if ((ret = httpd_req_recv(req, buf,
		    MIN(remaining, BUF_LEN))) <= 0) {
	ESP_LOGI(TAG, "ret = %d", ret);
	if (ret == HTTPD_SOCK_ERR_TIMEOUT) {
	    /* Retry receiving if timeout occurred */
	    continue;
	}
	free(buf);
	return ESP_FAIL;
    }
    ESP_LOGI(TAG, "buf query = %s", buf);
    if (getFirstVarName(&sr) == ESP_OK){
      do{
	varName = getNextVarName(&sr);
	ESP_LOGI(TAG, "check = %s", varName);
	if (varName == NULL)
	  break;
	if (httpd_query_key_value(buf, varName, value, VALUE_MAX_LEN) == ESP_OK){
	  ESP_LOGI(TAG, "varName = %s", varName);
	  if (sr.paramType == PARAM_TYPE_WIFI){
	    if(sr.pos == PARAM_SSID_NAME_NUM)
	      strcpy((char*) wifi_sta_param.ssid, value);
	    else if (sr.pos == PARAM_PASWRD_NUM)
	      strcpy((char*) wifi_sta_param.password, value);
	    wifi_save = 1;
	  }
	  else if ((sr.paramType == PARAM_TYPE_CAEN) && (strlen(value) <= CAYENN_MAX_LEN)){
	    if (sr.pos == PARAM_MQTT_HOST_NUM)
	      strcpy(cayenn_cfg.host, value);
	    else if (sr.pos == PARAM_MQTT_PORT_NUM)
	      cayenn_cfg.port = atoi(value);
	    else if (sr.pos == PARAM_MQTT_USER_NUM)
	      strcpy(cayenn_cfg.user, value);
	    else if (sr.pos == PARAM_MQTT_PASS_NUM)
	      strcpy(cayenn_cfg.pass, value);
	    else if (sr.pos == PARAM_MQTT_CLIENT_ID_NUM)
	      strcpy(cayenn_cfg.client_id, value);
	    else if (sr.pos == PARAM_MQTT_MODEL_NAME_NUM)
	      strcpy(cayenn_cfg.deviceName, value);
	    cayenn_save = 1;
	  }
	  else if(sr.paramType == PARAM_TYPE_NONE){
	    if (sr.pos == PARAM_WATERCOUNT_NUM){
	      watercount.count = atoi(value);
	      watercount.state = NO_READ_SPI;
	    }
	  }
	}
      } while(1);
    }
    remaining -= ret;
  }
  if (wifi_save)
    save_wifi_param(&wifi_sta_param);
  if (cayenn_save)
    save_cay_param(&cayenn_cfg);

  free(buf);
  return resultSave;
}

/*
 * Обработка запроса старницы.
 * Обрабатывает параметры если есть.
 * Отдает страницу если найдет.
 */
esp_err_t get_page_handler(httpd_req_t *req)
{

    char* resp = NULL;
    size_t fileSize;
    esp_err_t err;
    char* fileName;
    enum fileType typeFile;

    if (req->user_ctx){
      ESP_LOGI(TAG, "con_len = %d", req->content_len);
      if (req->content_len > 0){	//обработать запрос если есть
		if (parse_query(req) != ESP_OK){
		  httpd_resp_send(req, HTTPD_500, sizeof(HTTPD_500));
		  return ESP_OK;
		}
      }
      //Поиск страницы в файловой системе
      fileName = (char *)req->user_ctx;
      char* typeHdr = getTypeFile(fileName, &typeFile);
      httpd_resp_set_type(req, typeHdr);
      free(typeHdr);

      ESP_LOGI(TAG, "uri file = %s", fileName);
      if (typeFile == ftText){
		resp = parse_page(fileName, &fileSize, &err);
		ESP_LOGI(TAG, "size = %d content %s", fileSize, resp);
      }
      else{
		  resp = read_file(my_fs, fileName, &fileSize, &err);
		  ESP_LOGI(TAG, "size = %d binary file", fileSize);
      }

      if (err == ESP_OK){
		httpd_resp_send(req, resp, fileSize);
		if (httpd_req_get_hdr_value_len(req, "Host") == 0) {//TODO: Удалить
			ESP_LOGI(TAG, "Request headers lost");
		}
      }
      else{
    	  httpd_resp_send(req, HTTPD_404, sizeof(HTTPD_404));
      }
    }
    if (resp != NULL){
      free(resp);
    }
    return ESP_OK;
}

httpd_handle_t start_webserver(void)
{
    httpd_handle_t server = NULL;
    httpd_config_t config = HTTPD_DEFAULT_CONFIG();
    config.max_uri_handlers = 1024;
    config.max_resp_headers = 1024;

    uint8_t i;
    httpd_uri_t *current_uriGet;
    char *uri;

    my_fs = myfsInit();
    if (my_fs){
      // Start the httpd server
      ESP_LOGI(TAG, "Starting server on port: '%d'", config.server_port);
      if (httpd_start(&server, &config) == ESP_OK) {
		// Set URI handlers
		if (my_fs->count){
		  ESP_LOGI(TAG, "Registering URI handlers");
		  uriGetList = calloc(my_fs->count+1, sizeof(httpd_uri_t));
		  if (uriGetList){
			current_uriGet = uriGetList;
			current_uriGet->uri = "/";
			current_uriGet->method = HTTP_GET;
			current_uriGet->user_ctx = (void*)"index.html";
			current_uriGet->handler = get_page_handler;
			ESP_LOGI(TAG, "Registering URI start");
			httpd_register_uri_handler(server, current_uriGet);
			for(i=0; i < my_fs->count; i++){
			  current_uriGet++;
			  uri = calloc(strlen(my_fs->files[i].FileName)+2, 1);
			  if (uri){
				*uri = '/';
				strcat(uri, my_fs->files[i].FileName);
				//Регистрация метода get для страницы
				current_uriGet->method = HTTP_GET;
				current_uriGet->user_ctx = (void*) uri+1;
				current_uriGet->handler = get_page_handler;
				current_uriGet->uri = uri;
				httpd_register_uri_handler(server, current_uriGet);
				//Регистрация метода post для страницы
				current_uriGet->method = HTTP_POST;
				httpd_register_uri_handler(server, current_uriGet);
				ESP_LOGI(TAG, "Registering URI %s", current_uriGet->uri);
			  }
			}
		  }
		}
		return server;
      }
    }

    ESP_LOGI(TAG, "Error starting server!");
    return NULL;
}

void stop_webserver(httpd_handle_t server)
{
    uint8_t i;

    httpd_stop(server);
    if (my_fs){
      if (uriGetList){
        for(i=0;i<=my_fs->count;i++){	//Именно от 0 до count, т.к. регестрируется пустой uri "/"
          free((char*) uriGetList->uri);
          uriGetList++;
        }
        free(uriGetList);
        uriGetList = NULL;
      }
      free(my_fs);
      my_fs = NULL;
    }
}
