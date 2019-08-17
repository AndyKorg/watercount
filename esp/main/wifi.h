/*
 * wifi.h
 *
 *  Created on: 2 ���. 2019 �.
 *      Author: Administrator
 */

#ifndef MAIN_WIFI_H_
#define MAIN_WIFI_H_

#include <string.h>
#include <stddef.h>
#include "esp_wifi_types.h"

#define AP_SSID_WATERCOUNT	"watercounter"
#define AP_PASS_WATERCOUNT	""			//���� �����, �� ��� ������

extern wifi_sta_config_t wifi_sta_param;

void wifi_init_param(void);				//��������� ��������� wifi
void wifi_init(wifi_mode_t mode);
uint8_t wifi_isOn();					//wifi �������
uint8_t wifi_AP_isOn();					//AP �������, ������������� ������ ���� wifi_isOn() != 0
uint8_t wifi_ap_count_client();				//���������� �������� ������������ � ap


#endif /* MAIN_WIFI_H_ */
