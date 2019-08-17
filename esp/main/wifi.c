/*
 * wifi.c
 *
 *  Created on: 2 июн. 2019 г.
 *      Author: Administrator
 */

#include <string.h>
#include "wifi.h"
#include "esp_wifi.h"
#include "rom/ets_sys.h"
#include "freertos/event_groups.h"
#include "sdkconfig.h"
#include "esp_log.h"
#include "esp_event_loop.h"
#include "http_srv.h"
#include "nvs_params.h"
#include "command.h"

#undef __ESP_FILE__
#define __ESP_FILE__			NULL 		//Нет имен ошибок

#define AP_MAX_STA_CONN			4			//Максимальное количество клиентов подключаемых к AP

/* FreeRTOS event group to signal when we are connected*/
static EventGroupHandle_t wifi_event_group;

const int WIFI_PROCESS_BIT = BIT0,			//Wifi запущен
	  WIFI_PROCESS_AP_BIT = BIT1,			//Режим AP включен, дейстивтельно если WIFI_PROCESS_BIT = 1
	  TCP_INIT = BIT2,						//TCP адаптер инициализирован
	  CLIENT_CONNECTED = BIT3;				//Есть подклченные клиенты

static const char *TAG = "WIFI";

wifi_sta_config_t wifi_sta_param;

static esp_err_t event_handler(void *ctx, system_event_t *event)
{
  httpd_handle_t *server = (httpd_handle_t *) ctx;
  ESP_LOGI(TAG, "event %d", event->event_id);
  static uint8_t ap_sta_connect_count = 0;		//счетчик подключенных клиентов в режиме AP

  switch(event->event_id) {
    case SYSTEM_EVENT_STA_START:
    	xEventGroupSetBits(wifi_event_group, WIFI_PROCESS_BIT);
        esp_wifi_connect();
        break;
    case SYSTEM_EVENT_STA_STOP:
        esp_wifi_connect();
        xEventGroupClearBits(wifi_event_group, WIFI_PROCESS_BIT);
        break;
    case SYSTEM_EVENT_STA_GOT_IP:
        ESP_LOGI(TAG, "got ip:%s",
                 ip4addr_ntoa(&event->event_info.got_ip.ip_info.ip));
        Cayenne_app_start();
        if (*server == NULL) {
            *server = start_webserver();
        }
        break;
    case SYSTEM_EVENT_STA_DISCONNECTED:
        esp_wifi_connect();
        if (*server) {
            stop_webserver(*server);
            *server = NULL;
        }
        break;
    case SYSTEM_EVENT_AP_STACONNECTED:
        ap_sta_connect_count++;
		xEventGroupSetBits(wifi_event_group, CLIENT_CONNECTED);
        break;
    case SYSTEM_EVENT_AP_STAIPASSIGNED:
		if (*server == NULL) {
			*server = start_webserver();
		}
		break;
    case SYSTEM_EVENT_AP_STADISCONNECTED:
        if (*server) {
            stop_webserver(*server);
            *server = NULL;
        }
        ap_sta_connect_count--;
        if (!ap_sta_connect_count){
    		xEventGroupClearBits(wifi_event_group, CLIENT_CONNECTED);
        }
        break;
    case SYSTEM_EVENT_AP_START:
		xEventGroupSetBits(wifi_event_group, WIFI_PROCESS_BIT | WIFI_PROCESS_AP_BIT);
		ap_sta_connect_count = 0;
		xEventGroupClearBits(wifi_event_group, CLIENT_CONNECTED);
		break;
    case SYSTEM_EVENT_AP_STOP:
		xEventGroupClearBits(wifi_event_group, WIFI_PROCESS_BIT | WIFI_PROCESS_AP_BIT);
    	xEventGroupClearBits(wifi_event_group, CLIENT_CONNECTED);
		break;
    default:
        break;
    }
    return ESP_OK;
}

uint8_t wifi_isOn(){
  return (xEventGroupGetBitsFromISR(wifi_event_group) & WIFI_PROCESS_BIT)?1:0;
}

uint8_t wifi_AP_isOn(){
  return (xEventGroupGetBitsFromISR(wifi_event_group) & WIFI_PROCESS_AP_BIT)?1:0;
}

uint8_t wifi_ap_count_client(){
	return (xEventGroupGetBitsFromISR(wifi_event_group) & CLIENT_CONNECTED)?1:0;
}

void wifi_init(wifi_mode_t mode)
{
  static wifi_mode_t currentMode = WIFI_MODE_NULL;

  if ((wifi_sta_param.ssid[0] == 0) && ((mode == WIFI_MODE_STA) || (mode == WIFI_MODE_APSTA))){//Нет параметров ST
    if (mode == WIFI_MODE_APSTA){	//попробуем запустить только AP
      mode = WIFI_MODE_AP;
    }
    else{
      return;//нечего запускать
    }
  }

 /* if ((xEventGroupGetBits(wifi_event_group) & WIFI_PROCESS_BIT) && (mode == currentMode)){//Уже работает
    C1HIP_READY();
    return;
  }*/

  tcpip_adapter_init();
  xEventGroupSetBits(wifi_event_group, TCP_INIT);

  ESP_ERROR_CHECK(esp_wifi_set_mode(mode) );
  wifi_config_t wifi_config = {.ap = {.ssid = AP_SSID_WATERCOUNT}};//Именно так инициализируется имя сети для AP, если просто скопировать имя в массив, то не работает, инициализация происходит, но к сети никто подключится не моежт
  if ((mode == WIFI_MODE_AP) || (mode == WIFI_MODE_APSTA)){
    wifi_config.ap.ssid_len = strlen(AP_SSID_WATERCOUNT);
    strcpy((char*)wifi_config.ap.password, AP_PASS_WATERCOUNT);
    wifi_config.ap.max_connection = AP_MAX_STA_CONN;
    wifi_config.ap.authmode = WIFI_AUTH_WPA2_PSK;
    if (strlen(AP_PASS_WATERCOUNT) == 0) {
	  wifi_config.ap.authmode = WIFI_AUTH_OPEN;
    }
    ESP_ERROR_CHECK(esp_wifi_set_config(ESP_IF_WIFI_AP, &wifi_config));
    ESP_LOGI(TAG, "ap net=%s pas=%s maxch %d", wifi_config.ap.ssid, wifi_config.ap.password, wifi_config.ap.max_connection);
  }
  if ((mode == WIFI_MODE_STA) || (mode == WIFI_MODE_APSTA)){
    strcpy((char*)wifi_config.sta.ssid, (char*)wifi_sta_param.ssid);
    strcpy((char*)wifi_config.sta.password, (char*)wifi_sta_param.password);
    ESP_ERROR_CHECK(esp_wifi_set_config(ESP_IF_WIFI_STA, &wifi_config) );
  }

  ESP_ERROR_CHECK(esp_wifi_start() );
  currentMode = mode;
}

void wifi_init_param(void){

 wifi_event_group = xEventGroupCreate();

 wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
 ESP_ERROR_CHECK(esp_wifi_init(&cfg));

 ESP_ERROR_CHECK(esp_event_loop_init(event_handler, &http_server) );
 xEventGroupClearBits(wifi_event_group, WIFI_PROCESS_AP_BIT | WIFI_PROCESS_BIT);
 xEventGroupClearBits(wifi_event_group, CLIENT_CONNECTED);

 read_wifi_param(&wifi_sta_param);
}
