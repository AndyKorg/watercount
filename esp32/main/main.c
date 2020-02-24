#include "nvs_flash.h"
#include "esp_sleep.h"
#include "esp_log.h"
#include "ulp_sensor.h"

#include "wifi.h"
#include "esp_wifi.h"
#include "driver/gpio.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"

#include "display/epd1in54.h"//TODO: delete it, epdInit move to display!
#include "display.h"

#define STARTUP_MODE_PIN	GPIO_NUM_36	//start up mode: AP start or ST start
#define SETTING_TIMEOUT_S	60
#define APP_CORE_ID			(portNUM_PROCESSORS-1)

#define SLEEP_PERIOD_S		10			//sleep period cpu,
#define	DISPALY_PERIOD_S	10			//refresh display period
#define WIFI_SEND_PERIOD_S	10			//send data period wifi

#define WIFI_AP_ATTEMPT_MAX	5			//The maximum number of attempts to start the AP.

typedef enum {
	alarmSemafor, alarmSleepTask,
} alarm_system_t;

SemaphoreHandle_t sleepEnable; //ready to sleep

static const char *TAG = "main";

//wait and sleep
void vSleepTask(void *vParameters) {

	while (1) {
		xSemaphoreTake(sleepEnable, ((SETTING_TIMEOUT_S*1000)/portTICK_RATE_MS));
		if (wifi_ap_count_client()) {//client AP connect
			ESP_LOGI(TAG, "client AP is connected!");
			continue;
		}
		displayPowerOff();
		if (esp_sleep_enable_ulp_wakeup() == ESP_OK) {
			ESP_LOGI(TAG, "sleep start");
			displayPowerOff();
//		esp_bluedroid_disable();
			//esp_bt_controller_disable(),
			//esp_wifi_stop();
			set_ulp_SleepPeriod(SLEEP_PERIOD_S);
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

void app_main(void) {

	esp_sleep_wakeup_cause_t cause = esp_sleep_get_wakeup_cause();

	bool ulpRun = false;

	gpio_config_t io_conf;

	//Initialize NVS
	esp_err_t ret = nvs_flash_init();
	if (ret == ESP_ERR_NVS_NO_FREE_PAGES) {
		ESP_ERROR_CHECK(nvs_flash_erase());
		ret = nvs_flash_init();
	}
	ESP_ERROR_CHECK(ret);

	if (cause != ESP_SLEEP_WAKEUP_ULP) { //first start, ulp setting need
		set_attemptAP(WIFI_AP_ATTEMPT_MAX);
		init_ulp_program();
		ulpRun = (start_ulp_program() == ESP_OK); //start ulp for sensor control
		ESP_LOGI(TAG, "first start!");
	}

	sleepEnable = xSemaphoreCreateBinary();
	if (sleepEnable == NULL) {
		alarmOff(alarmSemafor);
		return;
	}

	if (xTaskCreatePinnedToCore(vSleepTask, "vSleepTask", 2048, NULL, 5, NULL, APP_CORE_ID) != pdPASS) {
		alarmOff(alarmSleepTask);
		return;
	}

	gpio_set_direction(STARTUP_MODE_PIN, GPIO_MODE_INPUT);
	if (gpio_get_level(STARTUP_MODE_PIN)) { //setting mode - wifi AP on and wait SETTING_TIMEOUT_S
		ESP_LOGI(TAG, "ap attempt = %d", get_attemptAP());
		uint32_t attemptAP = get_attemptAP();
		if (attemptAP) { // check count attempt AP
			//TODO: Show attempt to display
			ESP_LOGI(TAG, "AP mode startup!");
			set_attemptAP(attemptAP - 1);
			wifi_init_param();
			wifi_init(WIFI_MODE_AP);
			epdInit(lut_full_update);			//start spi
			ESP_LOGI(TAG, "AP start!");
			return;
		}
	}
	uint32_t countWakeUp = get_wakeUpCount();
	//start ST mode if parameter is set, or show problem and not start mode ST
	ESP_LOGI(TAG, "continue start!");

// од ниже оставил как пример
	/*
	 esp_sleep_wakeup_cause_t cause = esp_sleep_get_wakeup_cause();
	 if (cause == ESP_SLEEP_WAKEUP_ULP) {
	 ESP_LOGI("ULP", "ULP wakeup");
	 uint32_t mode = get_sleepMode();
	 if (mode++ == 3){
	 io_conf.pin_bit_mask = GPIO_SEL_35;
	 io_conf.intr_type = GPIO_PIN_INTR_DISABLE;
	 io_conf.mode = GPIO_MODE_INPUT;
	 io_conf.pull_down_en = GPIO_PULLDOWN_DISABLE;
	 io_conf.pull_up_en = GPIO_PULLUP_DISABLE;
	 ESP_ERROR_CHECK(gpio_config(&io_conf));
	 if (gpio_get_level(GPIO_NUM_35)){
	 epdInit(lut_full_update);//start spi
	 displayInit();
	 }
	 displayShow();//вылетает тут
	 mode = 0;
	 ESP_LOGI("ULP", "display show");
	 }
	 set_sleepMode(mode);
	 if (esp_sleep_enable_ulp_wakeup() == ESP_OK) {
	 displayPowerOff();
	 set_ulp_SleepPeriod(2);
	 esp_deep_sleep_start();
	 }
	 } else {
	 ESP_LOGI("ULP", "NOT ULP WAKEUP ----------------------");
	 //Initialize NVS
	 esp_err_t ret = nvs_flash_init();
	 if (ret == ESP_ERR_NVS_NO_FREE_PAGES) {
	 ESP_ERROR_CHECK(nvs_flash_erase());
	 ret = nvs_flash_init();
	 }
	 ESP_ERROR_CHECK(ret);

	 init_ulp_program();


	 epdInit(lut_full_update);

	 start_ulp_program(); //start ulp for sensor control

	 wifi_init_param();

	 if (gpio_get_level(STARTUP_MODE_PIN)) {
	 ESP_LOGI("GPIO", "AP mode startup!");
	 wifi_init(WIFI_MODE_AP);
	 }
	 else{
	 ESP_LOGI("GPIO", "ST mode startup!");
	 }
	 //		  esp_bluedroid_disable(), esp_bt_controller_disable(), esp_wifi_stop();
	 //		wifi_init(WIFI_MODE_STA);
	 displayInit();
	 displayShow();

	 //		epdSleep();
	 if (esp_sleep_enable_ulp_wakeup() == ESP_OK) {
	 ESP_LOGI("GPIO", "sleep start");
	 displayPowerOff();
	 set_ulp_SleepPeriod(5);
	 esp_deep_sleep_start();
	 }

	 }
	 */
}

