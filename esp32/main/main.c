#include "stddef.h"
#include "nvs_flash.h"
#include "esp_sleep.h"
#include "esp_log.h"

#include "params.h"
#include "version.h"

#include "HAL_GPIO.h"
#include "ulp_sensor.h"

#include "wifi.h"
#include "esp_wifi.h"
#include "driver/gpio.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"

#include "display/epd1in54.h"//TODO: delete it, epdInit move to display!
#include "display.h"

#include "cayenne.h"

#include "sntp_client.h"

#define SETTING_TIMEOUT_S	60			//timeout sleep
#define APP_CORE_ID			(portNUM_PROCESSORS-1)

#define SLEEP_PERIOD_S		3600		//sleep period cpu,
#define	DISPALY_PERIOD_S	1			//refresh display period
#define WIFI_SEND_PERIOD_S	1			//send data period wifi

#define WIFI_AP_ATTEMPT_MAX	5			//The maximum number of attempts to start the AP.

#define	WATERCOUNTER_CHANL	1			//Number channel from cloud for counter water
#define	BAT_CHANL			2			//for battery
#define	RAW_CHANL			3			//for raw data sensor

//parameters for html page
#define VERSION_PARAM		"version"
#define	WATERCOUNTER_PARAM	"watercount"

typedef enum {
	alarmSemafor, alarmSleepTask, alarmUlpStart, alarmDispalyShow
} alarm_system_t;

SemaphoreHandle_t xEndWiFi, 	//Wifi operation end
		xEndDisp,				//display show end
		xDispShow,				//can be displayed.
		xTimeReady; 			//time received

static const char *TAG = "main";

RTC_SLOW_ATTR uint32_t attemptAP_RTC; 		//count attempt mode AP
RTC_SLOW_ATTR uint32_t wake_up_display_RTC;	//count wake up for display
RTC_SLOW_ATTR uint32_t wake_up_wifi_st_RTC;	//count wake up for ST
RTC_SLOW_ATTR time_t dtSend;				//time send to cloud
RTC_SLOW_ATTR bool autoSwitchST_RTC;		//there was an automatic switch to st mode. Reset only if the jumper is switched to st or power off

//wait and sleep
#define SEMAPHORE_TIMEOUT	((SETTING_TIMEOUT_S*1000)/portTICK_RATE_MS)

void vSleepTask(void *vParameters) {
	uint32_t sleepPeriod;

	while (1) {
		ESP_LOGI(TAG, "sleep task start");
		xSemaphoreTake(xEndWiFi, SEMAPHORE_TIMEOUT);
		xSemaphoreTake(xEndDisp, SEMAPHORE_TIMEOUT);
		xSemaphoreTake(xTimeReady, SEMAPHORE_TIMEOUT);
		if (wifi_ap_count_client()) {	//client AP connect
			continue;
		}
		if (wifi_AP_isOn()) {			//timeout AP mode, switch to sleep mode
			wake_up_display_RTC = DISPALY_PERIOD_S-1; 	//showing now
			wake_up_wifi_st_RTC = WIFI_SEND_PERIOD_S-1;	//send mow
			autoSwitchST_RTC = true;
			sleepPeriod = 1;
		}
		else {
			sleepPeriod = SLEEP_PERIOD_S;
		}
		displayPowerOff();
		if (esp_sleep_enable_ulp_wakeup() == ESP_OK) {
			ESP_LOGI(TAG, "sleep start\r");
//		esp_bluedroid_disable();
			//esp_bt_controller_disable(),
			//esp_wifi_stop();
			set_ulp_SleepPeriod(sleepPeriod);
			// Set the wake stub function
			esp_set_deep_sleep_wake_stub(&wake_stub);
			esp_deep_sleep_start();
		}
	}
}

//deep sleep system and power off all
void alarmOff(alarm_system_t alarm) {
	ESP_LOGI(TAG, "alarm sleep %d", (uint8_t )alarm);
	epdInit(lut_full_update);		//start spi
	//display alarm
	//display sleep
	//display power off
	//esp_bluedroid_disable(), esp_bt_controller_disable(), esp_wifi_stop();
	esp_sleep_disable_wakeup_source(ESP_SLEEP_WAKEUP_ALL);
	esp_deep_sleep_start(); //system is off
}

//send raw data sensor to cloud
esp_err_t sendRawSensor(uint8_t *chanal, char **sensorType, uint32_t *value) {
	ESP_LOGI(TAG, "send start raw");
	*sensorType = calloc(strlen(CAYENNE_ANALOG_SENSOR) + 12, sizeof(char)); //12 digit for uint32_t
	if (sensorType) {
		ESP_LOGI(TAG, "send raw");
		*chanal = RAW_CHANL;
		*value = (uint32_t) sensor_raw();
		memcpy(*sensorType, CAYENNE_ANALOG_SENSOR, strlen(CAYENNE_ANALOG_SENSOR));
		return ESP_OK;
	}
	ESP_LOGI(TAG, "send no memory");
	return ESP_ERR_NO_MEM;
}

//send battery to cloud
esp_err_t sendBat(uint8_t *chanal, char **sensorType, uint32_t *value) {
	ESP_LOGI(TAG, "send start bat");
	*sensorType = calloc(strlen(CAYENNE_VOLTAGE_MV) + 12, sizeof(char)); //12 digit for uint32_t
	if (sensorType) {
		ESP_LOGI(TAG, "send bat");
		*chanal = BAT_CHANL;
		*value = bat_voltage();
		memcpy(*sensorType, CAYENNE_VOLTAGE_MV, strlen(CAYENNE_VOLTAGE_MV));
		return ESP_OK;
	}
	ESP_LOGI(TAG, "send no memory");
	return ESP_ERR_NO_MEM;
}

//send counter to cloud
esp_err_t sendCounter(uint8_t *chanal, char **sensorType, uint32_t *value) {
	ESP_LOGI(TAG, "send start counter");
	*sensorType = calloc(strlen(CAYENNE_COUNTER) + 12, sizeof(char)); //12 digit for uint32_t
	if (sensorType) {
		ESP_LOGI(TAG, "send count");
		*chanal = WATERCOUNTER_CHANL;
		*value = sensor_count(NULL);
		memcpy(*sensorType, CAYENNE_COUNTER, strlen(CAYENNE_COUNTER));
		return ESP_OK;
	}
	ESP_LOGI(TAG, "send no memory");
	return ESP_ERR_NO_MEM;
}

//published is ended, sleep enable
esp_err_t pubEnd(int data) {
	dtSend = getSecond();
	xSemaphoreGive(xEndWiFi);
	return ESP_OK;
}

//show data on display
void vDisplayShow(void *Param) {
	while (1) {
		xSemaphoreTake(xDispShow, (SEMAPHORE_TIMEOUT/4));
		ESP_LOGI(TAG, "display show start");
		epdInit(lut_full_update);		//start spi, full update
		displayInit(cdClear);
		displayShow(sensor_count(NULL), sensor_state(), wifi_paramIsSet(), dtSend, bat_voltage(), wifi_AP_isOn(), AP_SSID);
		displayPowerOff();
		ESP_LOGI(TAG, "Display show end");
		xSemaphoreGive(xEndDisp);		//start sleep
	}
}

//sntp time callback
void time_sync_notification_cb(struct timeval *tv) {
	struct tm *t = localtime(&(tv->tv_sec));
	ESP_LOGI(TAG, "date time = %02d %02d %04d %02d %02d", t->tm_mday, t->tm_mon, t->tm_year + 1900, t->tm_hour, t->tm_min);
	setSecond(tv->tv_sec);
	if (esp_sleep_get_wakeup_cause() != ESP_SLEEP_WAKEUP_ULP) { //first start
		xSemaphoreGive(xDispShow); //Show display
	}
	xSemaphoreGive(xTimeReady);
}

//version on html page
esp_err_t read_version_param(const paramName_t paramName, char *value, size_t maxLen) {
	sprintf(value, "%d.%d.%d.%d", VERSION_APPLICATION.part[VERSION_MAJOR], VERSION_APPLICATION.part[VERSION_MINOR], VERSION_APPLICATION.part[VERSION_PATCH],
			VERSION_APPLICATION.part[VERSION_BUILD]);
	return ESP_OK;
}

//counter on html page - read and write
esp_err_t read_counter_param(const paramName_t paramName, char *value, size_t maxLen) {
	sprintf(value, "%d", sensor_count(NULL));
	return ESP_OK;
}

esp_err_t write_counter_param(const paramName_t paramName, const char *value, size_t maxLen) {
	if (paramName && value) {
		if (strlen(value) <= maxLen) {
			uint32_t newCount;
			if (sscanf(value, "%d", &newCount) == 1) {
				ESP_LOGI(TAG, "counter param %d save", sensor_count(&newCount));
			}
			return ESP_OK;
		}
	}
	ESP_LOGE(TAG, "counter save error");
	return ESP_ERR_INVALID_ARG;
}

void app_main(void) {

	//Initialize NVS
	esp_err_t ret = nvs_flash_init();
	if (ret == ESP_ERR_NVS_NO_FREE_PAGES) {
		ESP_ERROR_CHECK(nvs_flash_erase());
		ret = nvs_flash_init();
	}
	ESP_ERROR_CHECK(ret);

	if (esp_sleep_get_wakeup_cause() != ESP_SLEEP_WAKEUP_ULP) { 			//first start, ulp setting need
		attemptAP_RTC = WIFI_ANT_MODE_MAX;
		wake_up_display_RTC = 0; 					//showing status, if only sntp time recived
		wake_up_wifi_st_RTC = WIFI_SEND_PERIOD_S;	//send status
		autoSwitchST_RTC = false;						//swith auto off
		init_ulp_program();
		if (start_ulp_program() != ESP_OK) { 		//start ulp for sensor control
			alarmOff(alarmUlpStart);
			return;
		}
	}

	xEndWiFi = xSemaphoreCreateBinary();
	xEndDisp = xSemaphoreCreateBinary();
	xDispShow = xSemaphoreCreateBinary();
	xTimeReady = xSemaphoreCreateBinary();
	if ((xEndWiFi == NULL) || (xEndDisp == NULL) || (xDispShow == NULL) || (xTimeReady == NULL)) {
		alarmOff(alarmSemafor);
		return;
	}

	if (xTaskCreatePinnedToCore(vSleepTask, "vSleepTask", 2048, NULL, 5, NULL, APP_CORE_ID) != pdPASS) {
		alarmOff(alarmSleepTask);
		return;
	}

	if (xTaskCreatePinnedToCore(vDisplayShow, "vDisplayShow", 2048, NULL, 5, NULL, APP_CORE_ID) != pdPASS) {
		alarmOff(alarmDispalyShow);
		return;
	}

	wifi_init_param();
	gpio_set_direction(STARTUP_MODE_PIN, GPIO_MODE_INPUT);
	if (gpio_get_level(STARTUP_MODE_PIN) == 0) {
		autoSwitchST_RTC = false;
	}
	if (gpio_get_level(STARTUP_MODE_PIN) && (!battery_low()) && (!autoSwitchST_RTC)) { //setting mode - wifi AP on and wait SETTING_TIMEOUT_S
		ESP_LOGI(TAG, "ap attempt = %d", attemptAP_RTC);
		if (attemptAP_RTC) { // check count attempt AP
			ESP_LOGI(TAG, "AP mode startup!");
			attemptAP_RTC--;
			paramReg(VERSION_PARAM, (VERSION_PART_COUNT * 3) + 1, read_version_param, NULL, NULL);
			paramReg(WATERCOUNTER_PARAM, 12, read_counter_param, write_counter_param, NULL);
			wifi_init(WIFI_MODE_AP);
			ESP_LOGI(TAG, "AP start!");
			xSemaphoreGive(xDispShow);
			return;
		}
	}
	//start ST mode if parameter is set, or show problem and not start mode ST
	if (esp_sleep_get_wakeup_cause() == ESP_SLEEP_WAKEUP_ULP) {
		wake_up_wifi_st_RTC++;
		wake_up_display_RTC++;
	} else
		ESP_LOGI(TAG, "wake_up: wifi %d disp %d", wake_up_wifi_st_RTC, wake_up_display_RTC);

	if ((wake_up_wifi_st_RTC == WIFI_SEND_PERIOD_S) && (!battery_low())) {
		sntp_init_app(time_sync_notification_cb);
		Cayenne_send_reg(sendCounter, sendBat, sendRawSensor, pubEnd);
		wifi_init(WIFI_MODE_STA);
		wake_up_wifi_st_RTC = 0;
	} else {
		ESP_LOGI(TAG, "WIFI sleep continue");
		xSemaphoreGive(xEndWiFi);
		xSemaphoreGive(xTimeReady);
	}

	if ((wake_up_display_RTC == DISPALY_PERIOD_S) && (!battery_low())) {
		wake_up_display_RTC = 0;
		xSemaphoreGive(xDispShow); //Show display
	} else {
		if (esp_sleep_get_wakeup_cause() == ESP_SLEEP_WAKEUP_ULP) { //only for not first start
			ESP_LOGI(TAG, "disp sleep continue");
			xSemaphoreGive(xEndDisp);
		}
	}
}

