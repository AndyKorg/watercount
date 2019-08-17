/*
 * cayenne.c
 *
 *  Created on: 10 мая 2019 г.
 *      Author: Administrator
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include "sdkconfig.h"
#include "esp_log.h"
#include "nvs_params.h"
#include "cayenne.h"

#include "params.h"
#include "command.h"

static const char *TAG_MQTT = "MQTT-MY:";

cayenne_t cayenn_cfg;
esp_mqtt_client_handle_t mqtt_client;	//Клиент для каена

char*
CayenneTopic (cayenne_t* cfg, const char* type, const char* channal) {
  char* msg = NULL;

  if ((cfg->user) && (cfg->client_id)){
    uint8_t temp = strlen (MQTT_CAYENNE_VER) + strlen(cfg->user)
	+ strlen (MQTT_CAYENNE_DELEMITER) + strlen (cfg->client_id)
	+ strlen (type);
    if (channal)
      temp += strlen ((const char*)channal);
    msg = (char*) calloc (temp + 1, sizeof(char));
    if (msg) {
      strcpy (msg, MQTT_CAYENNE_VER);
      strcpy (msg + strlen (msg), cfg->user);
      strcpy (msg + strlen (msg), MQTT_CAYENNE_DELEMITER);
      strcpy (msg + strlen (msg), cfg->client_id);
      strcpy (msg + strlen (msg), type);
      if (channal)
	strcpy (msg + strlen (msg), (const char*) channal);
    }
  }
  return msg;
}

esp_err_t CayenneChangeInteger(cayenne_t* cfg, const uint8_t chanal, const char* nameSensor, const uint32_t value){

  esp_err_t ret = ESP_ERR_INVALID_STATE;
  if (mqtt_client){
    #define CHANAL_NUM_LEN_MAX 10
    char* chanalString = (char*) calloc(CHANAL_NUM_LEN_MAX, sizeof(char));
    sprintf(chanalString, "%d", chanal);

    char* topic = CayenneTopic(cfg, MQTT_CAYENNE_TYPE_DATA, chanalString);
    #define VALUE_LEN 20 //Типа 20 цифр данные
    char* result = calloc(strlen(nameSensor)+VALUE_LEN, sizeof(char));
    sprintf(result, CAYENNE_DIGITAL_SENSOR, value);//TODO: нужна динамическая строка форматирования
    if (esp_mqtt_client_publish(mqtt_client, topic, result, strlen(result), MQTT_QOS_TYPE_AT_MOST_ONCE, MQTT_RETAIN_OFF) >= 0)
      ret = ESP_OK;
    free(result);
    free(topic);
    free(chanalString);
  }
  return ret;
}

esp_err_t Cayenne_event_handler (esp_mqtt_event_handle_t event) {
  esp_mqtt_client_handle_t client = event->client;
  char* topic = NULL;
  char * topicCmd = NULL;
  // your_context_t *context = event->context;
  switch (event->event_id)
  {
    case MQTT_EVENT_CONNECTED:
      ESP_LOGE(TAG_MQTT, "MQTT_EVENT_CONNECTED");
      topic = CayenneTopic (&cayenn_cfg, MQTT_CAYENNE_TYPE_SYS_MODEL, NULL);
      int msg_id = esp_mqtt_client_publish (client, topic, cayenn_cfg.deviceName, strlen(cayenn_cfg.deviceName), MQTT_QOS_TYPE_AT_MOST_ONCE, MQTT_RETAIN_OFF);
      if (msg_id >= 0){
    	  cayenn_cfg.LastState = CayenneChangeInteger(&cayenn_cfg, PARAM_CHANAL_CAYEN, PARAM_NAME_SENSOR, watercount.count);
      }
      ESP_LOGE(TAG_MQTT, "connect sent publish successful, msg_id = %x cayen state = %x", msg_id, cayenn_cfg.LastState);
      CHIP_READY();
      break;
    case MQTT_EVENT_DISCONNECTED:
      ESP_LOGI(TAG_MQTT, "MQTT_EVENT_DISCONNECTED");
      CHIP_READY();
      break;
    case MQTT_EVENT_SUBSCRIBED:
      ESP_LOGI(TAG_MQTT, "MQTT_EVENT_SUBSCRIBED, msg_id=%d", event->msg_id);
      break;
    case MQTT_EVENT_UNSUBSCRIBED:
      ESP_LOGI(TAG_MQTT, "MQTT_EVENT_UNSUBSCRIBED, msg_id=%d", event->msg_id);
      break;
    case MQTT_EVENT_PUBLISHED:
      ESP_LOGE(TAG_MQTT, "MQTT_EVENT_PUBLISHED, msg_id=%d", event->msg_id);
      break;
    case MQTT_EVENT_DATA:
      ESP_LOGI(TAG_MQTT, "MQTT_EVENT_DATA");
      printf ("TOPIC=%.*s\r\n", event->topic_len, event->topic);
      printf ("DATA=%.*s\r\n", event->data_len, event->data);
      break;
    case MQTT_EVENT_ERROR:
      ESP_LOGI(TAG_MQTT, "MQTT_EVENT_ERROR");
      break;
    default:
      ESP_LOGI(TAG_MQTT, "Other event id:%d", event->event_id);
      break;
  }
  if (topic != NULL){
    free (topic);
  }
  if (topicCmd != NULL){
    free (topicCmd);
  }

  return ESP_OK;
}

void Cayenne_app_start (void) {
  if (read_cay_param(&cayenn_cfg) == ESP_OK){

    char* tmp = CayenneTopic (&cayenn_cfg, MQTT_CAYENNE_TYPE_SYS_MODEL, NULL);

    char* hostProtokol = calloc(CAYENN_MAX_LEN+strlen(MQTT_PROTOKOL)+1, sizeof(char));
    strcpy(hostProtokol, MQTT_PROTOKOL);
    strcat(hostProtokol, cayenn_cfg.host);
    ESP_LOGE(TAG_MQTT, "h %s", hostProtokol);

    esp_mqtt_client_config_t mqtt_cfg =
      { .uri = hostProtokol, .port = cayenn_cfg.port,
	  .username = cayenn_cfg.user, .password = cayenn_cfg.pass, .client_id = cayenn_cfg.client_id,
	  .lwt_qos = 0, //Разобраться
	  .lwt_topic = cayenn_cfg.deviceName,
	  .lwt_msg = tmp,
	  .lwt_msg_len = strlen ((const char*)tmp),
	  .event_handle = Cayenne_event_handler,
	  .transport = MQTT_TRANSPORT_OVER_TCP
      // .user_context = (void *)your_context
      };


    mqtt_client = esp_mqtt_client_init (&mqtt_cfg);
    cayenn_cfg.LastState = CAYENN_PROCESS;
    esp_mqtt_client_start (mqtt_client);
    free (tmp);
    free (hostProtokol);
  }
}

esp_err_t Cayenne_app_stop (void){ //Оторвать все подключения, ESP_OK - закрытие соединения начато
	return esp_mqtt_client_stop(mqtt_client);
}

esp_err_t Cayenne_Init (void) {
  cayenn_cfg.LastState = read_cay_param(&cayenn_cfg);
  if (cayenn_cfg.LastState == ESP_OK){
	  cayenn_cfg.LastState = CAYENN_PROCESS;
  }
  return cayenn_cfg.LastState;
}
