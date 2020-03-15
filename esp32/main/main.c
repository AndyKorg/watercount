#include "stddef.h"
#include "nvs_flash.h"
#include "esp_sleep.h"
#include "esp_log.h"

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

#define SETTING_TIMEOUT_S	60
#define APP_CORE_ID			(portNUM_PROCESSORS-1)

#define SLEEP_PERIOD_S		10			//sleep period cpu,
#define	DISPALY_PERIOD_S	1			//refresh display period
#define WIFI_SEND_PERIOD_S	100			//send data period wifi

#define WIFI_AP_ATTEMPT_MAX	5			//The maximum number of attempts to start the AP.

#define	WATERCOUNTER_CHANL	1			//Number channel from cloud for counter water

typedef enum {
	alarmSemafor, alarmSleepTask, alarmUlpStart, alarmDispalyShow
} alarm_system_t;

SemaphoreHandle_t xEndWiFi, xEndDisp, xDispShow; //ready to sleep
xTaskHandle displayTask;

static const char *TAG = "main";

RTC_SLOW_ATTR uint32_t attemptAP_RTC; 		//count attempt mode AP
RTC_SLOW_ATTR uint32_t wake_up_display_RTC;	//count wake up for display
RTC_SLOW_ATTR uint32_t wake_up_wifi_st_RTC;	//count wake up for ST

//wait and sleep
void vSleepTask(void *vParameters) {

	while (1) {
		ESP_LOGI(TAG, "sleep task start");
		xSemaphoreTake(xEndWiFi, ((SETTING_TIMEOUT_S*1000)/portTICK_RATE_MS));
		xSemaphoreTake(xEndDisp, ((SETTING_TIMEOUT_S*1000)/portTICK_RATE_MS));
		if (wifi_ap_count_client()) {	//client AP connect
			continue;
		}
		displayPowerOff();
		if (esp_sleep_enable_ulp_wakeup() == ESP_OK) {
			ESP_LOGI(TAG, "sleep start");
//		esp_bluedroid_disable();
			//esp_bt_controller_disable(),
			//esp_wifi_stop();
			set_ulp_SleepPeriod(SLEEP_PERIOD_S);
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

//send counter to cloud
esp_err_t sendCounter(uint8_t *chanal, char **sensorType, uint32_t *value) {
	ESP_LOGI(TAG, "send start");
	*sensorType = calloc(strlen(CAYENNE_COUNTER) + 12, sizeof(char)); //12 digit for uint32_t
	if (sensorType) {
		ESP_LOGI(TAG, "send value full");
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
	xSemaphoreGive(xEndWiFi);
	return ESP_OK;
}

void vDisplayShow(void *Param) {
	while (1) {
		if (xSemaphoreTake(xDispShow, portMAX_DELAY) == pdPASS) {
			ESP_LOGI(TAG, "display show start");
			epdInit(lut_full_update);		//start spi
			displayInit(cdClear);
			displayShow();
			displayPowerOff();
			ESP_LOGI(TAG, "Display show end");
			xSemaphoreGive(xEndDisp);//start sleep
		}
	}
}

void app_main(void) {

	esp_sleep_wakeup_cause_t cause = esp_sleep_get_wakeup_cause();

	//Initialize NVS
	esp_err_t ret = nvs_flash_init();
	if (ret == ESP_ERR_NVS_NO_FREE_PAGES) {
		ESP_ERROR_CHECK(nvs_flash_erase());
		ret = nvs_flash_init();
	}
	ESP_ERROR_CHECK(ret);

	if (cause != ESP_SLEEP_WAKEUP_ULP) { 			//first start, ulp setting need
		attemptAP_RTC = WIFI_ANT_MODE_MAX;
		wake_up_display_RTC = DISPALY_PERIOD_S; 	//show status
		wake_up_wifi_st_RTC = 0; //DEBUG = OFF !WIFI_SEND_PERIOD_S;	//send status
		init_ulp_program();
		if (start_ulp_program() != ESP_OK) { 		//start ulp for sensor control
			alarmOff(alarmUlpStart);
			return;
		}
	}

	xEndWiFi = xSemaphoreCreateBinary();
	xEndDisp = xSemaphoreCreateBinary();
	xDispShow = xSemaphoreCreateBinary();
	if ((xEndWiFi == NULL) || (xEndDisp == NULL) || (xDispShow == NULL)) {
		alarmOff(alarmSemafor);
		return;
	}

	if (xTaskCreatePinnedToCore(vSleepTask, "vSleepTask", 2048, NULL, 5, NULL, APP_CORE_ID) != pdPASS) {
		alarmOff(alarmSleepTask);
		return;
	}

	if (xTaskCreatePinnedToCore(vDisplayShow, "vDisplayShow", 2048, NULL, 5, &displayTask, APP_CORE_ID) != pdPASS) {
		alarmOff(alarmDispalyShow);
		return;
	}

	wifi_init_param();
	gpio_set_direction(STARTUP_MODE_PIN, GPIO_MODE_INPUT);
	if (gpio_get_level(STARTUP_MODE_PIN) && (!battery_low())) { //setting mode - wifi AP on and wait SETTING_TIMEOUT_S
		ESP_LOGI(TAG, "ap attempt = %d", attemptAP_RTC);
		if (attemptAP_RTC) { // check count attempt AP
			ESP_LOGI(TAG, "AP mode startup!");
			attemptAP_RTC--;
			wifi_init(WIFI_MODE_AP);
			ESP_LOGI(TAG, "AP start!");
			xSemaphoreGive(xDispShow);
			return;
		}
	}
	//start ST mode if parameter is set, or show problem and not start mode ST
	if (cause == ESP_SLEEP_WAKEUP_ULP) {
		wake_up_wifi_st_RTC++;
		wake_up_display_RTC++;
	}
	ESP_LOGI(TAG, "wake_up: wifi %d disp %d", wake_up_wifi_st_RTC, wake_up_display_RTC);
	if ((wake_up_display_RTC == DISPALY_PERIOD_S) && (!battery_low())) {
		wake_up_display_RTC = 0;
		xSemaphoreGive(xDispShow);//Show display
	} else {
		ESP_LOGI(TAG, "disp sleep continue");
		xSemaphoreGive(xEndDisp);
	}
	if ((wake_up_wifi_st_RTC == WIFI_SEND_PERIOD_S) && (!battery_low())) {
		ESP_LOGI(TAG, "Wifi send");
		Cayenne_send_reg(sendCounter, pubEnd);
		wifi_init(WIFI_MODE_STA);
		wake_up_wifi_st_RTC = 0;
	} else {
		ESP_LOGI(TAG, "WIFI sleep continue");
		xSemaphoreGive(xEndWiFi);
	}
}

