/*
 * wifi.c
 *
 *  Created on: 2 июн. 2019 г.
 *      Author: Administrator
 */

#include <string.h>
#include "esp_err.h"
#include "esp_wifi.h"
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include "freertos/event_groups.h"
#include "esp_log.h"

#include "lwip/err.h"
#include "lwip/sys.h"

#include "nvs_flash.h"
#include "nvs.h"

#include "wifi.h"
#include "params.h"

#include "http_srv.h"
#include "cayenne.h"

//#include "ota_client.h"

static const char *TAG = "WIFI";
#undef __ESP_FILE__
#define __ESP_FILE__			NULL 		//Нет имен ошибок

#define STORAGE_WIFI_PARAM 		"wifi_prm"	//wifi parameters storage
#define AP_MAX_STA_CONN			4			//maximum count clients on AP

wifi_sta_config_t wifi_sta_param;

/* FreeRTOS event group to signal when we are connected*/
static EventGroupHandle_t wifi_event_group;

const int WIFI_PROCESS_BIT = BIT0,			//Wifi запущен
		WIFI_PROCESS_AP_BIT = BIT1,			//Режим AP включен, дейстивтельно если WIFI_PROCESS_BIT = 1
		CLIENT_CONNECTED = BIT3,			//Есть подключенные клиенты
		WIFI_GOT_IP_BIT = BIT4;				//Есть подключение к сети

esp_err_t read_wifi_param(const paramName_t paramName, char *value, size_t maxLen) {

	return read_nvs_param(STORAGE_WIFI_PARAM, paramName, value, maxLen);
}

esp_err_t read_wifi_params(void) {

	ESP_LOGI(TAG, "wifi param read");
	esp_err_t ret = read_nvs_param(STORAGE_WIFI_PARAM, STA_PARAM_SSID_NAME, (char*) wifi_sta_param.ssid, 32);
	if (ret == ESP_OK) {
		ret = read_nvs_param(STORAGE_WIFI_PARAM, STA_PARAM_PASWRD_NAME, (char*) wifi_sta_param.password, 32);
	}
	return ret;
}

esp_err_t write_wifi_param(const paramName_t paramName, const char *value, size_t maxLen) {
	ESP_LOGI(TAG, "wifi param %s %s save", paramName, value);
	if (paramName && value) {
		if (strlen(value) <= maxLen) {
			if (!strcmp(paramName, STA_PARAM_SSID_NAME)) {
				strcpy((char*) wifi_sta_param.ssid, value);
			} else if (!strcmp(paramName, STA_PARAM_PASWRD_NAME)) {
				strcpy((char*) wifi_sta_param.password, value);
			}
			ESP_LOGI(TAG, "save OK");
			return ESP_OK;
		}
	}
	ESP_LOGE(TAG, "wifi param %s %s save error", paramName, value);
	return ESP_ERR_INVALID_ARG;
}

esp_err_t save_wifi_params(void) {

	nvs_handle my_handle;
	esp_err_t ret = ESP_ERR_INVALID_SIZE;

	ESP_LOGI(TAG, "wifi param save to nvs start");
	if (wifi_sta_param.ssid[0] != '\0') {
		if (nvs_open(STORAGE_WIFI_PARAM, NVS_READWRITE, &my_handle) == ESP_OK) {
			ESP_LOGI(TAG, "Write open ok");
			ESP_ERROR_CHECK(nvs_set_str(my_handle, STA_PARAM_SSID_NAME, (char *) wifi_sta_param.ssid));
			ESP_LOGI(TAG, "ssid = %s save", (char* ) wifi_sta_param.ssid);
			ESP_ERROR_CHECK(nvs_set_str(my_handle, STA_PARAM_PASWRD_NAME, (char *) wifi_sta_param.password));
			ESP_LOGI(TAG, "pass = %s save", (char* ) wifi_sta_param.password);
			ESP_ERROR_CHECK(nvs_commit(my_handle));
			ESP_LOGI(TAG, "commit Ok");

			nvs_close(my_handle);
			ret = ESP_OK;
		}
	}
	return ret;
}

static void ip_event_handler(void *arg, esp_event_base_t event_base, int32_t event_id, void *event_data) {

	ip_event_got_ip_t *event = (ip_event_got_ip_t*) event_data;

	//  httpd_handle_t *server = (httpd_handle_t *) ctx;

	switch (event_id) {
	case IP_EVENT_STA_GOT_IP:
		ESP_LOGI(TAG, "got ip:" IPSTR, IP2STR(&event->ip_info.ip));
		Cayenne_app_start();
		start_webserver();
		xEventGroupSetBits(wifi_event_group, WIFI_GOT_IP_BIT);
		break;
	default:
		break;
	}
}

static void wifi_event_handler(void *arg, esp_event_base_t event_base, int32_t event_id, void *event_data) {

	/*wifi_event_ap_staconnected_t *event =
	 (wifi_event_ap_staconnected_t*) event_data;*/

	static uint8_t ap_sta_connect_count = 0;		//count clients from mode AP

	switch (event_id) {
	case WIFI_EVENT_STA_START:
		xEventGroupSetBits(wifi_event_group, WIFI_PROCESS_BIT);
		esp_wifi_connect();
		break;
	case WIFI_EVENT_STA_DISCONNECTED:
		esp_wifi_connect();
		break;
	case WIFI_EVENT_AP_STACONNECTED:
		ap_sta_connect_count++;
		ESP_LOGI(TAG, "number client %d", ap_sta_connect_count);
		xEventGroupSetBits(wifi_event_group, CLIENT_CONNECTED);
		break;
	case WIFI_EVENT_AP_STADISCONNECTED:
		ap_sta_connect_count--;
		if (!ap_sta_connect_count) {
			xEventGroupClearBits(wifi_event_group, CLIENT_CONNECTED);
		}
		break;
	case WIFI_EVENT_AP_START:
		xEventGroupSetBits(wifi_event_group, WIFI_PROCESS_BIT | WIFI_PROCESS_AP_BIT);
		ap_sta_connect_count = 0;
		xEventGroupClearBits(wifi_event_group, CLIENT_CONNECTED);
		start_webserver();
		break;
	case WIFI_EVENT_AP_STOP:
		xEventGroupClearBits(wifi_event_group, WIFI_PROCESS_BIT | WIFI_PROCESS_AP_BIT);
		xEventGroupClearBits(wifi_event_group, CLIENT_CONNECTED);
		if (!(xEventGroupGetBitsFromISR(wifi_event_group) & WIFI_GOT_IP_BIT)) {
			stop_webserver();
		}
		break;
	default:
		break;
	}
}

bool wifi_isOn() {
	return (xEventGroupGetBitsFromISR(wifi_event_group) & WIFI_PROCESS_BIT);
}

bool wifi_AP_isOn() {
	return (xEventGroupGetBitsFromISR(wifi_event_group) & WIFI_PROCESS_AP_BIT);
}

bool wifi_ap_count_client() {
	return (xEventGroupGetBitsFromISR(wifi_event_group) & CLIENT_CONNECTED);
}

void wifi_init(wifi_mode_t mode) {
	static uint8_t netIfInit = 0;

	if ((wifi_sta_param.ssid[0] == 0) && ((mode == WIFI_MODE_STA) || (mode == WIFI_MODE_APSTA))) {		//parameters ST is empty
		if (mode == WIFI_MODE_APSTA) {	//Only AP
			mode = WIFI_MODE_AP;
		} else {
			return;						//no mode
		}
	}

	if (!netIfInit) {
		esp_netif_init();
		netIfInit = 1;
	}
	ESP_ERROR_CHECK(esp_event_loop_create_default());

	ESP_ERROR_CHECK(esp_event_handler_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &wifi_event_handler, NULL));
	ESP_ERROR_CHECK(esp_event_handler_register(IP_EVENT, ESP_EVENT_ANY_ID, &ip_event_handler, NULL));

	wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT()
	;
	ESP_ERROR_CHECK(esp_wifi_init(&cfg));

	if (mode == WIFI_MODE_APSTA) {
		ESP_LOGI(TAG, "wifi MIX mode start");
	} else if (mode == WIFI_MODE_STA) {
		ESP_LOGI(TAG, "wifi ST mode start");
	} else if (mode == WIFI_MODE_AP) {
		ESP_LOGI(TAG, "wifi AP mode start");
	}

	ESP_ERROR_CHECK(esp_wifi_set_mode(mode));

	wifi_config_t wifi_config = { .ap = { .ssid = AP_SSID } };//This is how the network name for the AP is initialized, if you just copy the name into an array, it doesn’t work, initialization happens, but no one can connect to the network
	if ((mode == WIFI_MODE_AP) || (mode == WIFI_MODE_APSTA)) {
		esp_netif_create_default_wifi_ap();
		wifi_config.ap.ssid_len = strlen(AP_SSID);
		strcpy((char*) wifi_config.ap.password, AP_PASS);
		wifi_config.ap.max_connection = AP_MAX_STA_CONN;
		wifi_config.ap.authmode = WIFI_AUTH_WPA2_PSK;
		if (strlen(AP_PASS) == 0) {
			wifi_config.ap.authmode = WIFI_AUTH_OPEN;
		}
		ESP_ERROR_CHECK(esp_wifi_set_config(ESP_IF_WIFI_AP, &wifi_config));
		ESP_LOGI(TAG, "ap net=%s pas=%s maxch %d", wifi_config.ap.ssid, wifi_config.ap.password, wifi_config.ap.max_connection);
	}
	if ((mode == WIFI_MODE_STA) || (mode == WIFI_MODE_APSTA)) {
		esp_netif_create_default_wifi_sta();
		strcpy((char*) wifi_config.sta.ssid, (char*) wifi_sta_param.ssid);
		strcpy((char*) wifi_config.sta.password, (char*) wifi_sta_param.password);
		ESP_LOGI(TAG, "st net=%s pas=%s", wifi_config.sta.ssid, wifi_config.sta.password);
		ESP_ERROR_CHECK(esp_wifi_set_config(ESP_IF_WIFI_STA, &wifi_config));
	}

	ESP_ERROR_CHECK(esp_wifi_start());
}

void task_ota_check(void *pvParameters) {

	const TickType_t oneMinute = (60 * 1000) / portTICK_PERIOD_MS; //one minute
	uint32_t count_period = 0;

	while (1) {
		if (count_period == 0) {
			if (xEventGroupGetBitsFromISR(wifi_event_group) & WIFI_GOT_IP_BIT) {
				ESP_LOGI(TAG, "ota check");
//				ota_check();
				count_period = OTA_CHECK_PERIOD_MIN + 1;
			}
		} else {
			count_period--;
		}
		vTaskDelay(oneMinute);
	}
}

void wifi_init_param(void) {

	wifi_event_group = xEventGroupCreate();
	xEventGroupClearBits(wifi_event_group, WIFI_PROCESS_AP_BIT | WIFI_PROCESS_BIT | WIFI_GOT_IP_BIT | CLIENT_CONNECTED);
	if (paramReg(STA_PARAM_SSID_NAME, 32, read_wifi_param, write_wifi_param, save_wifi_params) == ESP_OK) {
		paramReg(STA_PARAM_PASWRD_NAME, 64, read_wifi_param, write_wifi_param, save_wifi_params);
	}
	read_wifi_params();
	Cayenne_Init();
}
