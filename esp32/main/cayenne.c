/*
 * Send and receive cloud cayenne.mydevices.com
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <sdkconfig.h>
#include <time.h>
#include "esp_log.h"
#include "nvs.h"

#include "utils.h"
#include "cayenne.h"
#include "params.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"

static const char *TAG = "CAYEN";

#define STORAGE_CAY_PARAM 		"cayen_prm"	//cayenne parameters storage

typedef struct {
	char host[CAYENN_MAX_LEN];
	char port[CAYENN_MAX_LEN];
	char user[CAYENN_MAX_LEN];
	char pass[CAYENN_MAX_LEN];
	char client_id[CAYENN_MAX_LEN];
	char deviceName[CAYENN_MAX_LEN];
} cayenne_t;

cayenne_t cayenn_cfg;
esp_mqtt_client_handle_t mqtt_client;	//cayenne client

#define PARAMS_COUNT 6
typedef struct {
	paramName_t param;
	char *value;
} param_t;

param_t params[PARAMS_COUNT] = {	// @formatter:off
		{ PARAM_MQTT_HOST, (char*) cayenn_cfg.host },
		{ PARAM_MQTT_PORT, (char*) cayenn_cfg.port },
		{ PARAM_MQTT_USER, (char*) cayenn_cfg.user },
		{ PARAM_MQTT_PASS,(char*) cayenn_cfg.pass },
		{ PARAM_MQTT_CLIENT_ID, (char*) cayenn_cfg.client_id },
		{ PARAM_MQTT_MODEL_NAME, (char*) cayenn_cfg.deviceName }
		// @formatter:on
		};

cay_reciv_cb_t reciveTopic, pubSuccess;	//callback function for recive data from broker and receive answer from published

#define CONFIRM_NO_MES			-1
#define WAIT_MES_MAX_LEN		3				//1 - counter, 2 - bat, 3 - raw sensor
#define CONFIRM_COUNTER_MES		0
#define CONFIRM_BAT_MES			1
#define CONFIRM_RAW_SENSOR_MES	2

//use critical section only!
static portMUX_TYPE confirmMesLock = portMUX_INITIALIZER_UNLOCKED;

typedef struct {
	cay_send_cb_t func; 				//callback function for send data to broker.
	int id_mes_send;					//id message send to broker, for qos>0 only!
	int id_mes_confirm;					//id confirm message
} sendControl_t;

sendControl_t messageConfirm[WAIT_MES_MAX_LEN];

char*
CayenneTopic(const char *type, const char *channal) {
	char *msg = NULL;

	if ((cayenn_cfg.user) && (cayenn_cfg.client_id)) {
		uint8_t lenTopic = strlen(MQTT_CAYENNE_VER) + strlen(cayenn_cfg.user) + strlen(MQTT_CAYENNE_DELEMITER) + strlen(cayenn_cfg.client_id) + strlen(type);
		if (channal)
			lenTopic += strlen((const char*) channal);
		msg = (char*) calloc(lenTopic + 1, sizeof(char));
		if (msg) {
			strcpy(msg, MQTT_CAYENNE_VER);
			strcpy(msg + strlen(msg), cayenn_cfg.user);
			strcpy(msg + strlen(msg), MQTT_CAYENNE_DELEMITER);
			strcpy(msg + strlen(msg), cayenn_cfg.client_id);
			strcpy(msg + strlen(msg), type);
			if (channal)
				strcpy(msg + strlen(msg), (const char*) channal);
		}
	}
	return msg;
}

//registry function for send to broker
//send_counter_cb - for send counter
//send_bat_volt_cb - for send battery voltage
//send_cnt_raw_cb - for send raw data sensor
//answer_cb - the function is called after receiving a response to all transfers.
esp_err_t Cayenne_send_reg(cay_send_cb_t send_counter_cb, cay_send_cb_t send_bat_volt_cb, cay_send_cb_t send_cnt_raw_cb, cay_reciv_cb_t answer_cb) {
	portENTER_CRITICAL(&confirmMesLock);
	messageConfirm[CONFIRM_COUNTER_MES].func = send_counter_cb;
	messageConfirm[CONFIRM_BAT_MES].func = send_bat_volt_cb;
	messageConfirm[CONFIRM_RAW_SENSOR_MES].func = send_cnt_raw_cb;
	portEXIT_CRITICAL(&confirmMesLock);
	pubSuccess = answer_cb;
	return ESP_OK;
}

esp_err_t read_cay_param(const paramName_t paramName, char *value, size_t maxLen) {

	return read_nvs_param(STORAGE_CAY_PARAM, paramName, value, maxLen);
}

esp_err_t read_cay_params(void) {

	ESP_LOGI(TAG, "cay param read");
	uint8_t i = 0;
	esp_err_t ret;

	for (; i < 6; i++) {
		ret = read_nvs_param(STORAGE_CAY_PARAM, params[i].param, params[i].value, CAYENN_MAX_LEN);
		if (ret != ESP_OK) {
			break;
		}
	}
	return ret;
}

esp_err_t write_cay_param(const paramName_t paramName, const char *value, size_t maxLen) {
	ESP_LOGI(TAG, "cay param save");

	uint8_t i;
	if (paramName && value) {
		if (strlen(value) <= maxLen) {
			for (i = 0; i < PARAMS_COUNT; i++) {
				if (!strcmp(paramName, params[i].param)) {
					strcpy(params[i].value, value);
					return ESP_OK;
				}
			}
		}
	}
	return ESP_ERR_INVALID_ARG;
}

esp_err_t save_cay_params(void) {

	nvs_handle my_handle;
	esp_err_t ret = ESP_ERR_INVALID_SIZE;

	if (nvs_open(STORAGE_CAY_PARAM, NVS_READWRITE, &my_handle) == ESP_OK) {
		uint8_t i = 0;
		for (; i < PARAMS_COUNT; i++) {
			ret = nvs_set_str(my_handle, params[i].param, params[i].value);
			if (ret != ESP_OK) {
				break;
			}
		}
		if (ret == ESP_OK)
			ESP_ERROR_CHECK(nvs_commit(my_handle));
		nvs_close(my_handle);
		ret = ESP_OK;
	}
	return ret;
}

int CayenneChangeInteger(const uint8_t chanal, const char *sensorType, const uint32_t value, const int qos) {

	int ret = -1;
	if (mqtt_client) {
#define CHANAL_NUM_LEN_MAX 10
		char *chanalString = (char*) calloc(CHANAL_NUM_LEN_MAX, sizeof(char));
		sprintf(chanalString, "%d", chanal);
		char *topic = CayenneTopic(MQTT_CAYENNE_TYPE_DATA, chanalString);
#define VALUE_LEN 20 //Типа 20 цифр данные
		char *result = calloc(strlen(sensorType) + VALUE_LEN, sizeof(char));
		sprintf(result, sensorType, value);
		ESP_LOGI(TAG, "pub topic = %s result = %s", topic, result);
		ret = esp_mqtt_client_publish(mqtt_client, topic, result, strlen(result), qos, MQTT_RETAIN_OFF);
		free(result);
		free(topic);
		free(chanalString);
	}
	return ret;
}

esp_err_t CayenneSubscribe(const uint8_t chanal) {
	esp_err_t ret = ESP_ERR_INVALID_STATE;
	if (mqtt_client) {
#define CHANAL_NUM_LEN_MAX 10
		char *chanalString = (char*) calloc(CHANAL_NUM_LEN_MAX, sizeof(char));
		sprintf(chanalString, "%d", chanal);

		char *topic = CayenneTopic(MQTT_CAYENNE_TYPE_CMD, chanalString);
		ESP_LOGI(TAG, "subs topic = %s", topic);
		ret = esp_mqtt_client_subscribe(mqtt_client, topic, MQTT_RETAIN_OFF);
		free(topic);
		free(chanalString);
	}
	return ret;
}

esp_err_t CayenneResponse(const char *nameResponse, esp_err_t response, const char *msg_error) {
	esp_err_t ret = ESP_ERR_INVALID_STATE;
	if (mqtt_client) {
		char *topic = CayenneTopic(MQTT_CAYENNE_TYPE_RESPONSE, NULL);

		int resultLen = strlen(MQTT_CAYENNE_RESPONSE_ERR) //max length string
		+ strlen(nameResponse);
		if ((response != ESP_OK) && (msg_error)) {
			resultLen += strlen(msg_error);
		}
		char *result = calloc(resultLen + (sizeof(char) * 3), sizeof(char)); //3 = end string + symbol "=" for error message + reserved
		if (response != ESP_OK) {
			strcpy(result, MQTT_CAYENNE_RESPONSE_ERR);
			strcpy(result, nameResponse);
			if (msg_error) {
				strcpy(result, "=");
				strcpy(result, msg_error);
			}
		} else {
			strcpy(result, MQTT_CAYENNE_RESPONSE_OK);
			strcpy(result, nameResponse);
		}
		ESP_LOGI(TAG, "resp topic = %s result = %s", topic, result);
		if (esp_mqtt_client_publish(mqtt_client, topic, result, strlen(result), MQTT_QOS_TYPE_AT_MOST_ONCE, MQTT_RETAIN_OFF) >= 0)
			ret = ESP_OK;
		free(topic);
		free(result);
	}
	return ret;
}

/*
 * Работает, но на стороне сервера обнвоялет состояение только после обновления страницы
 */
esp_err_t CayenneUpdateActuator(const uint8_t chanal, const uint32_t value) {

	esp_err_t ret = ESP_ERR_INVALID_STATE;
	if (mqtt_client) {
#define CHANAL_NUM_LEN_MAX 10
		char *chanalString = (char*) calloc(CHANAL_NUM_LEN_MAX, sizeof(char));
		sprintf(chanalString, "%d", chanal);
		char *topic = CayenneTopic(MQTT_CAYENNE_TYPE_DATA, chanalString);
#define VALUE_LEN 20 //Типа 20 цифр данные
		char *result = calloc(strlen(CAYENNE_DIGITAL_SENSOR) + VALUE_LEN, sizeof(char));
		sprintf(result, "%d", value); //TODO: нужна динамическая строка форматирования
		ESP_LOGI(TAG, "upd topic = %s result = %s", topic, result);
		if (esp_mqtt_client_publish(mqtt_client, topic, result, strlen(result), MQTT_QOS_TYPE_AT_MOST_ONCE, MQTT_RETAIN_OFF) >= 0)
			ret = ESP_OK;
		free(result);
		free(topic);
		free(chanalString);
	}
	return ret;
}

esp_err_t Cayenne_reciv_reg(uint8_t chanal, cay_reciv_cb_t func) {	//Регистрация реакции на событие в канале
	reciveTopic = func;
	return ESP_OK;
}

int SendData(cay_send_cb_t funcGetdata) {
	uint8_t chanal;
	char *sensorType = NULL;
	uint32_t value;
	int id_msg = -1;
	if (funcGetdata) {
		ESP_LOGI(TAG, "cb send start");
		if (funcGetdata(&chanal, &sensorType, &value) == ESP_OK) {
			ESP_LOGI(TAG, "sens type %s", sensorType);
			if (sensorType && strlen(sensorType) > 0) {
				ESP_LOGI(TAG, "pub start");
				id_msg = CayenneChangeInteger(chanal, sensorType, value, MQTT_QOS_TYPE_AT_LEAST_ONCE);	//necessarily MQTT_QOS_TYPE_AT_LEAST_ONCE!
				free(sensorType);
			}
		}
	}
	return id_msg;
}

void vSendDataTask(void *Param) {
	int id_msg, i;
	while (1) {
		for(i=0;i<WAIT_MES_MAX_LEN;i++){
			if (messageConfirm[i].func){
				id_msg = SendData(messageConfirm[i].func);
				portENTER_CRITICAL(&confirmMesLock);
				messageConfirm[i].id_mes_send = id_msg;
				portEXIT_CRITICAL(&confirmMesLock);
			}
		}
		vTaskSuspend(NULL);
	}
}

esp_err_t Cayenne_event_handler(esp_mqtt_event_handle_t event) {
	esp_mqtt_client_handle_t client = event->client;
	char *topic = NULL;
	char *topicCmd = NULL;
	// your_context_t *context = event->context;
	switch (event->event_id) {
	case MQTT_EVENT_CONNECTED:
		ESP_LOGI(TAG, "MQTT_EVENT_CONNECTED");
		topic = CayenneTopic(MQTT_CAYENNE_TYPE_SYS_MODEL, NULL);
		ESP_LOGI(TAG, "topic = %s", topic);
		ESP_LOGI(TAG, "dev = %s", cayenn_cfg.deviceName);
		if (esp_mqtt_client_publish(client, topic, cayenn_cfg.deviceName, strlen(cayenn_cfg.deviceName), MQTT_QOS_TYPE_AT_MOST_ONCE, MQTT_RETAIN_OFF) >= 0) {
			ESP_LOGI(TAG, "connect sent publish successful");
			xTaskCreate(vSendDataTask, "vSendDataTask", 2048, NULL, 5, NULL);
		}
		//CayenneSubscribe(&cayenn_cfg, PARAM_CHANAL_LED_UPDATE);
		break;
	case MQTT_EVENT_DISCONNECTED:
		ESP_LOGI(TAG, "MQTT_EVENT_DISCONNECTED");
		break;
	case MQTT_EVENT_SUBSCRIBED:
		ESP_LOGI(TAG, "MQTT_EVENT_SUBSCRIBED, msg_id=%d", event->msg_id);
		break;
	case MQTT_EVENT_UNSUBSCRIBED:
		ESP_LOGI(TAG, "MQTT_EVENT_UNSUBSCRIBED, msg_id=%d", event->msg_id);
		break;
	case MQTT_EVENT_PUBLISHED:	//Only if qos > MQTT_QOS_TYPE_AT_MOST_ONCE
		ESP_LOGI(TAG, "MQTT_EVENT_PUBLISHED, msg_id=%d", event->msg_id);
		int i = 0;
		for (; i < WAIT_MES_MAX_LEN; i++) {
			if (messageConfirm[i].id_mes_send == event->msg_id) {
				messageConfirm[i].id_mes_confirm = event->msg_id;
			}
		}
		for (i = 0; i < WAIT_MES_MAX_LEN; i++) {
			if ((messageConfirm[i].id_mes_confirm != messageConfirm[i].id_mes_send) || (messageConfirm[i].id_mes_send == CONFIRM_NO_MES)) {
				break;
			}
		}
		for (i = 0; i < WAIT_MES_MAX_LEN; i++) {
			messageConfirm[i].id_mes_send = CONFIRM_NO_MES;
		}
		if (pubSuccess) {
			int id_msg = event->msg_id;
			ESP_LOGI(TAG, "cb pub start");
			pubSuccess(id_msg);
		}
		break;
	case MQTT_EVENT_DATA:
		ESP_LOGI(TAG, "MQTT_EVENT_DATA");
		ESP_LOGI(TAG, "TOPIC=%.*s\r\n", event->topic_len, event->topic);
		ESP_LOGI(TAG, "DATA=%.*s\r\n", event->data_len, event->data);
		char *comma = strchr(event->data, ',');
		if (comma) {
			*comma = '\0';
			ESP_LOGI(TAG, "payload=%s", event->data);
			CayenneResponse(event->data, ESP_OK, NULL);
			if (reciveTopic) {
				int reciv = atoi(++comma);
				ESP_LOGI(TAG, "led topic=%d", reciv);
				reciveTopic(reciv);
			}
		}
		break;
	case MQTT_EVENT_ERROR:
		ESP_LOGI(TAG, "MQTT_EVENT_ERROR");
		break;
	default:
		ESP_LOGI(TAG, "Other event id:%d", event->event_id);
		break;
	}
	if (topic != NULL) {
		free(topic);
	}
	if (topicCmd != NULL) {
		free(topicCmd);
	}

	return ESP_OK;
}

void Cayenne_app_start(void) {
	if (read_cay_params() == ESP_OK) {

		char *hostProtokol = calloc(CAYENN_MAX_LEN + strlen(MQTT_PROTOKOL) + 1, sizeof(char));
		strcpy(hostProtokol, MQTT_PROTOKOL);
		strcat(hostProtokol, cayenn_cfg.host);
		ESP_LOGI(TAG, "h %s", hostProtokol);

		char *tmp = CayenneTopic(MQTT_CAYENNE_TYPE_SYS_MODEL, NULL);
		uint16_t port = atoi(cayenn_cfg.port);

		esp_mqtt_client_config_t mqtt_cfg = { .uri = hostProtokol, .port = port, .username = cayenn_cfg.user, .password = cayenn_cfg.pass, .client_id =
				cayenn_cfg.client_id,
				.lwt_qos = 0, //Подтверждение доставки
				.lwt_topic = cayenn_cfg.deviceName, .lwt_msg = tmp, .lwt_msg_len = strlen((const char*) tmp), .event_handle = Cayenne_event_handler,
				.transport = MQTT_TRANSPORT_OVER_TCP
		// .user_context = (void *)your_context
				};

		mqtt_client = esp_mqtt_client_init(&mqtt_cfg);
		if (mqtt_client) {
			ESP_LOGI(TAG, "start client");
			portENTER_CRITICAL(&confirmMesLock);
			int i;
			for (i = 0; i < WAIT_MES_MAX_LEN; i++) {
				messageConfirm[i].id_mes_send = CONFIRM_NO_MES;
				messageConfirm[i].id_mes_confirm = CONFIRM_NO_MES;
			}
			portEXIT_CRITICAL(&confirmMesLock);
			esp_mqtt_client_start(mqtt_client);
		}
		free(tmp);
		free(hostProtokol);
	}
}

esp_err_t Cayenne_app_stop(void) { //Close all connect, return ESP_OK - closed process start
	esp_err_t ret = ESP_OK;
	if (mqtt_client) {
		ret = esp_mqtt_client_stop(mqtt_client); //TODO: отписку сделать
	}
	return ret;
}

esp_err_t Cayenne_Init(void) {
	esp_err_t ret = ESP_ERR_NOT_SUPPORTED;
	uint8_t i = 0;
	for (; i < PARAMS_COUNT; i++) {
		ret = paramReg(params[i].param, CAYENN_MAX_LEN, read_cay_param, write_cay_param, save_cay_params);
		if (ret != ESP_OK) {
			break;
		}
	}
	for (; i < WAIT_MES_MAX_LEN; i++) {
		messageConfirm[i].func = NULL;
	}
	pubSuccess = NULL;
	return read_cay_params();
}
