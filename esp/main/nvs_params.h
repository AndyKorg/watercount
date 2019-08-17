/*
 * ��������� ����������� �� flash
 */

#ifndef MAIN_NVS_PARAMS_H_
#define MAIN_NVS_PARAMS_H_

#include "esp_err.h"
#include "esp_wifi_types.h"
#include "cayenne.h"

#define STA_PARAM_SSID_NAME		"ssid"		//��� ����, ���� ������ ������, �� ���������� ���
#define STA_PARAM_PASWRD_NAME		"pswrd"		//������ ����

//��������� ��� ������ � mydevices.com (cayenne)
#define PARAM_MQTT_HOST 		"cay_host"	//��� ������� cayeen ���� "mqtt://mqtt.mydevices.com"
#define PARAM_MQTT_PORT 		"cay_port"	//���� �������, �� ��������� 1883
#define PARAM_MQTT_USER 		"cay_user"	//��� ������������
#define PARAM_MQTT_PASS 		"cay_pas"	//������
#define PARAM_MQTT_CLIENT_ID 		"cay_clnid"	//������ �������
#define PARAM_MQTT_MODEL_NAME 		"cay_model"	//��� ����������

esp_err_t read_wifi_param(wifi_sta_config_t *wifi_sta_param);
esp_err_t save_wifi_param(wifi_sta_config_t *wifi_sta_param);

esp_err_t read_cay_param(cayenne_t* value);
esp_err_t save_cay_param(cayenne_t* value);

#endif /* MAIN_NVS_PARAMS_H_ */
