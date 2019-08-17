/*
 * params.c
 *
 *  Created on: 12 июн. 2019 г.
 *      Author: Administrator
 */
#include <stdlib.h>
#include "sdkconfig.h"
#include "nvs_params.h"

#include "esp_err.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include "nvs.h"

#define STORAGE_PARAM			"strge"		//имя хранилища для параметров wifi

#undef __ESP_FILE__
#define __ESP_FILE__			NULL 		//Нет имен ошибок

static const char *TAG = "NVS";

esp_err_t read_wifi_param(wifi_sta_config_t *wifi_sta_param){

  nvs_handle my_handle;
  esp_err_t ret = ESP_ERR_NVS_NOT_FOUND;

  if (nvs_open(STORAGE_PARAM, NVS_READONLY, &my_handle) == ESP_OK) {
    ESP_LOGI(TAG, "nvs open Ok");
    wifi_sta_param->ssid[0] = '\0';
    wifi_sta_param->password[0] = '\0';
    size_t size = 0;
    ret = nvs_get_str(my_handle, STA_PARAM_SSID_NAME, NULL, &size);
    if (ret == ESP_OK){
      ESP_LOGI(TAG, "size ssid = %d", size);
      if (size <= 32){//ограничено в объявлении wifi_sta_param
		nvs_get_str(my_handle, STA_PARAM_SSID_NAME, (char*) wifi_sta_param->ssid, &size);
		ESP_LOGI(TAG, "ssid = %s", (char*) wifi_sta_param->ssid);
		size = 0;
		if (nvs_get_str(my_handle, STA_PARAM_PASWRD_NAME, NULL, &size) == ESP_OK){
			if (size<=64){
				nvs_get_str(my_handle, STA_PARAM_PASWRD_NAME, (char*) wifi_sta_param->password, &size);
				ESP_LOGI(TAG, "pass = %s", (char*) wifi_sta_param->password);
				ret = ESP_OK;
			}
		}
      }
    }
  }
  nvs_close(my_handle);
  return ret;
}

esp_err_t save_wifi_param(wifi_sta_config_t *wifi_sta_param){

  nvs_handle my_handle;
  esp_err_t ret = ESP_ERR_INVALID_SIZE;

  if (wifi_sta_param->ssid[0] != '\0'){
    if (nvs_open(STORAGE_PARAM, NVS_READWRITE, &my_handle) == ESP_OK) {
      ESP_LOGI(TAG, "Write open ok");
      ESP_ERROR_CHECK(nvs_set_str(my_handle, STA_PARAM_SSID_NAME, (char *) wifi_sta_param->ssid));
      ESP_LOGI(TAG, "ssid = %s save", (char*) wifi_sta_param->ssid);
      ESP_ERROR_CHECK(nvs_set_str(my_handle, STA_PARAM_PASWRD_NAME, (char *) wifi_sta_param->password));
      ESP_LOGI(TAG, "pass = %s save", (char*) wifi_sta_param->password);
      ESP_ERROR_CHECK(nvs_commit(my_handle));
      ESP_LOGI(TAG, "commit Ok");

      nvs_close(my_handle);
      ret = ESP_OK;
    }
  }
  return ret;
}

esp_err_t read_cay_param(cayenne_t* value){

  esp_err_t ret = ESP_ERR_NOT_FOUND;
  if (value){
    cayenne_t buf;
    size_t size = CAYENN_MAX_LEN;//Потому что nvs функции требуют указатель для размера
    nvs_handle my_handle;

    ret = nvs_open(STORAGE_PARAM, NVS_READONLY, &my_handle);
    if (ret != ESP_OK) return ret;
    ret = nvs_get_str(my_handle, PARAM_MQTT_HOST, buf.host, &size);
    if (ret != ESP_OK) return ret;
    size = CAYENN_MAX_LEN;
    ret = nvs_get_str(my_handle, PARAM_MQTT_PASS, buf.pass, &size);
    if (ret != ESP_OK) return ret;
    size = CAYENN_MAX_LEN;
    ret = nvs_get_str(my_handle, PARAM_MQTT_USER, buf.user, &size);
    if (ret != ESP_OK) return ret;
    size = CAYENN_MAX_LEN;
    ret = nvs_get_str(my_handle, PARAM_MQTT_CLIENT_ID, buf.client_id, &size);
    if (ret != ESP_OK) return ret;
    size = CAYENN_MAX_LEN;
    ret = nvs_get_str(my_handle, PARAM_MQTT_MODEL_NAME, buf.deviceName, &size);
    if (ret != ESP_OK) return ret;
    ret = nvs_get_u16(my_handle, PARAM_MQTT_PORT, &(buf.port));
    if (ret != ESP_OK) return ret;
    if (!buf.port){
      buf.port = MQTT_PORT;
    }
    memcpy(value, &buf, sizeof(cayenne_t));
    ret = ESP_OK;
    nvs_close(my_handle);
    if (ret != ESP_OK) return ret;
  }
  return ret;
}

esp_err_t save_cay_param(cayenne_t* value){

  if (value){
    nvs_handle my_handle;
    if (nvs_open(STORAGE_PARAM, NVS_READWRITE, &my_handle) == ESP_OK) {
      ESP_ERROR_CHECK(nvs_set_str(my_handle, PARAM_MQTT_HOST, value->host));
      ESP_ERROR_CHECK(nvs_set_u16(my_handle, PARAM_MQTT_PORT, value->port));
      ESP_ERROR_CHECK(nvs_set_str(my_handle, PARAM_MQTT_USER, value->user));
      ESP_ERROR_CHECK(nvs_set_str(my_handle, PARAM_MQTT_PASS, value->pass));
      ESP_ERROR_CHECK(nvs_set_str(my_handle, PARAM_MQTT_CLIENT_ID, value->client_id));
      ESP_ERROR_CHECK(nvs_set_str(my_handle, PARAM_MQTT_MODEL_NAME, value->deviceName));
      ESP_ERROR_CHECK(nvs_commit(my_handle));

      nvs_close(my_handle);
      return ESP_OK;
    }
  }
  return ESP_ERR_INVALID_SIZE;
}
