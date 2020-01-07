#include "nvs_flash.h"
#include "esp_sleep.h"
#include "esp_log.h"
#include "ulp_sensor.h"

#include "wifi.h"
#include "driver/gpio.h"

#include "display/epd1in54.h"
#include "display.h"

void app_main(void) {
	esp_sleep_wakeup_cause_t cause = esp_sleep_get_wakeup_cause();
	if (cause == ESP_SLEEP_WAKEUP_ULP) {
		ESP_LOGI("ULP", "ULP wakeup");
		displayPowerOff();
		if (esp_sleep_enable_ulp_wakeup() == ESP_OK) {
			set_ulp_SleepPeriod(5);
			esp_deep_sleep_start();
		}
	} else {
		ESP_LOGI("ULP", "Not ULP wakeup");
		//Initialize NVS
		esp_err_t ret = nvs_flash_init();
		if (ret == ESP_ERR_NVS_NO_FREE_PAGES) {
			ESP_ERROR_CHECK(nvs_flash_erase());
			ret = nvs_flash_init();
		}
		ESP_ERROR_CHECK(ret);

		gpio_config_t io_conf;
		io_conf.pin_bit_mask = GPIO_SEL_36;
		io_conf.intr_type = GPIO_PIN_INTR_DISABLE;
		io_conf.mode = GPIO_MODE_INPUT;
		io_conf.pull_down_en = GPIO_PULLDOWN_DISABLE;
		io_conf.pull_up_en = GPIO_PULLUP_DISABLE;
		ESP_ERROR_CHECK(gpio_config(&io_conf));

		init_ulp_program();

		epdInit(lut_full_update);

		start_ulp_program(); //start ulp for sensor control

		wifi_init_param();

		if (gpio_get_level(GPIO_NUM_36)) {
			ESP_LOGI("GPIO", "36 is high");
			return;
		}
//		  esp_bluedroid_disable(), esp_bt_controller_disable(), esp_wifi_stop();
//		wifi_init(WIFI_MODE_STA);
		displayInit();
		displayShow();
		displayPowerOff();

		if (esp_sleep_enable_ulp_wakeup() == ESP_OK) {
			ESP_LOGI("GPIO", "sleep start");
			set_ulp_SleepPeriod(5);
			esp_deep_sleep_start();
		}

	}
}

