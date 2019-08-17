/*
 * Определенные переменные и их сохранение и чтение
 */
#include <stdlib.h>
#include "sdkconfig.h"
#include "params.h"
#include "esp_log.h"
#include "nvs_params.h"
#include "wifi.h"
#include "cayenne.h"
#include "esp_err.h"

#undef __ESP_FILE__
#define __ESP_FILE__	NULL

static const char *TAG = "PRMS";

watercount_t	watercount;			//Собственно сами показания счетчика

esp_err_t getFirstVarName(seechRecord_t* sr){
  if (sr){
    sr->pos = -1;
    sr->paramType = PARAM_TYPE_NONE;
    return ESP_OK;		//Первая переменная
  }
  return ESP_ERR_NOT_FOUND;
}

char* getNextVarName(seechRecord_t* sr){
    char* ret = NULL;
    if (sr){
      (sr->pos)++;
      switch(sr->pos){
	case PARAM_SSID_NAME_NUM:
	  ret = STA_PARAM_SSID_NAME;
	  sr->paramType = PARAM_TYPE_WIFI;
	  break;
	case PARAM_PASWRD_NUM:
	  ret = STA_PARAM_PASWRD_NAME;
	  sr->paramType = PARAM_TYPE_WIFI;
	  break;
	case PARAM_MQTT_HOST_NUM:
	  ret = PARAM_MQTT_HOST;
	  sr->paramType = PARAM_TYPE_CAEN;
	  break;
	case PARAM_MQTT_PORT_NUM:
	  ret = PARAM_MQTT_PORT;
	  sr->paramType = PARAM_TYPE_CAEN;
	  break;
	case PARAM_MQTT_USER_NUM:
	  ret = PARAM_MQTT_USER;
	  sr->paramType = PARAM_TYPE_CAEN;
	  break;
	case PARAM_MQTT_PASS_NUM:
	  ret = PARAM_MQTT_PASS;
	  sr->paramType = PARAM_TYPE_CAEN;
	  break;
	case PARAM_MQTT_CLIENT_ID_NUM:
	  ret = PARAM_MQTT_CLIENT_ID;
	  sr->paramType = PARAM_TYPE_CAEN;
	  break;
	case PARAM_MQTT_MODEL_NAME_NUM:
	  ret = PARAM_MQTT_MODEL_NAME;
	  sr->paramType = PARAM_TYPE_CAEN;
	  break;
	case PARAM_WATERCOUNT_NUM:
	  ret = PARAM_WATERCOUNT;
	  sr->paramType = PARAM_TYPE_NONE;
	  break;
	default:
	  break;
      }
    }
  return ret;
}

/*
 * Возварщает значение переменной.
 * NULL если не найдена переменная
 */
char* putsValue(char* toStr, char* varName, size_t *lenVal){
  char* value = NULL;
  uint32_t needFree = 0;

  ESP_LOGI(TAG, "varName = %s", varName);
  if ( !strcmp(varName, STA_PARAM_SSID_NAME)){
    value = (char*) wifi_sta_param.ssid;
  }
  else if ( !strcmp(varName, STA_PARAM_PASWRD_NAME)){
    value = (char*) wifi_sta_param.password;
  }
  else if ( !strcmp(varName, PARAM_MQTT_HOST)){
    value = cayenn_cfg.host;
  }
  else if ( !strcmp(varName, PARAM_MQTT_PORT)){
    value = calloc(20, sizeof(char));
    if (value){
      value = itoa(cayenn_cfg.port, value, 10);
      needFree = 1;
    }
  }
  else if ( !strcmp(varName, PARAM_MQTT_USER)){
    value = cayenn_cfg.user;
  }
  else if ( !strcmp(varName, PARAM_MQTT_PASS)){
    value = cayenn_cfg.pass;
  }
  else if ( !strcmp(varName, PARAM_MQTT_CLIENT_ID)){
    value = cayenn_cfg.client_id;
  }
  else if ( !strcmp(varName, PARAM_MQTT_MODEL_NAME)){
    value = cayenn_cfg.deviceName;
  }
  else if ( !strcmp(varName, PARAM_WATERCOUNT)){
    value = calloc(20, sizeof(char));
    if (value){
      value = itoa(watercount.count, value, 10);
      needFree = 1;
    }
  }
  if (value){
    strncpy(toStr, value, strlen(value));
    ESP_LOGI(TAG, "val = %s", value);
    toStr += strlen((char*) value);
    *lenVal += strlen((char*) value);		//общая длина добавленых строк
    if (needFree){
      free(value);
    }
  }
  return toStr;
}

