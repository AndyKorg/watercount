/*
 * wifi control
 *
 */

#ifndef MAIN_WIFI_H_
#define MAIN_WIFI_H_

#include <string.h>
#include <stddef.h>
#include "esp_wifi_types.h"

//Soft AP mode parameters
#define AP_SSID						"watercounter"
#define AP_PASS						""					//if empty, then no password
//ST mode parameters
#define STA_PARAM_SSID_NAME			"ssid"				//Name parameter for name network connect
#define STA_PARAM_PASWRD_NAME		"pswrd"

#define OTA_CHECK_PERIOD_MIN	(24*60)					//Period checked update application, minute

void wifi_init_param(void);								//Read WiFi parameters, WiFi transmitter no start!
//esp_err_t save_wifi_params(void);
void wifi_init(wifi_mode_t mode);	//start WiFI transmitter

bool wifi_paramIsSet(void);							//WiFI ST parameters is empty
bool wifi_isOn(void);									//WiFI transmitter is on
bool wifi_AP_isOn(void);								//AP mode is on, if wifi_isOn() is true only
bool wifi_ap_count_client(void);						//number of connected clients in AP mode

#endif /* MAIN_WIFI_H_ */
