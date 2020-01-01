//for ulp file include
#include "esp_sleep.h"
#include "driver/adc.h"
#include "esp32/ulp.h"
#include "driver/rtc_io.h"
#include "ulp_main.h"

#include "nvs_flash.h"

#include "wifi.h"

#include "esp_log.h"
#include "driver/gpio.h"

//for spi debug
#include "display/epd1in54.h"

extern const uint8_t ulp_main_bin_start[] asm("_binary_ulp_main_bin_start");
extern const uint8_t ulp_main_bin_end[] asm("_binary_ulp_main_bin_end");

/* This function is called once after power-on reset, to load ULP program into
 * RTC memory and configure the ADC.
 */
static void init_ulp_program(void);

/* This function is called every time before going into deep sleep.
 * It starts the ULP program and resets measurement counter.
 */
static void start_ulp_program(void);


void app_main(void) {
	esp_sleep_wakeup_cause_t cause = esp_sleep_get_wakeup_cause();
	if (cause == ESP_SLEEP_WAKEUP_ULP) {
		ESP_LOGI("ULP", "Deep sleep wakeup\n");
		ESP_LOGI("ULP", "ULP did %d measurements since last reset\n", ulp_sample_counter & UINT16_MAX);
		ESP_LOGI("ULP", "Thresholds:  low=%d  high=%d\n", ulp_low_thr, ulp_high_thr);
		ulp_last_result &= UINT16_MAX;
		ESP_LOGI("ULP", "Value=%d was %s threshold\n", ulp_last_result, ulp_last_result < ulp_low_thr ? "below" : "above");
		ESP_LOGI("ULP", "Entering deep sleep\n\n");
		start_ulp_program();
		ESP_ERROR_CHECK(esp_sleep_enable_ulp_wakeup());
		esp_deep_sleep_start();
	} else {
		ESP_LOGI("ULP", "Not ULP wakeup");
		init_ulp_program();
		//Initialize NVS
		esp_err_t ret = nvs_flash_init();
		if (ret == ESP_ERR_NVS_NO_FREE_PAGES) {
			ESP_ERROR_CHECK(nvs_flash_erase());
			ret = nvs_flash_init();
		}
		ESP_ERROR_CHECK(ret);

		//start_ulp_program();

		gpio_config_t io_conf;
		io_conf.pin_bit_mask = GPIO_SEL_36;
		io_conf.intr_type = GPIO_PIN_INTR_DISABLE;
		io_conf.mode = GPIO_MODE_INPUT;
		io_conf.pull_down_en = GPIO_PULLDOWN_DISABLE;
		io_conf.pull_up_en = GPIO_PULLUP_DISABLE;
		ESP_ERROR_CHECK(gpio_config(&io_conf));
		if (gpio_get_level(GPIO_NUM_36)) {
//			wifi_init_param();
//			wifi_init(WIFI_MODE_AP);
			ESP_LOGI("GPIO", "36 is high");
			return;
		}
//		  esp_bluedroid_disable(), esp_bt_controller_disable(), esp_wifi_stop();
//		wifi_init_param();
//		wifi_init(WIFI_MODE_STA);

//		ESP_ERROR_CHECK(esp_sleep_enable_ulp_wakeup());
//		esp_deep_sleep_start();

		ESP_LOGI("GPIO", "36 is low");
		epdInit(lut_full_update);

		epdClearPatternMemory(0xff);// bit set = white, bit reset = black
		epdDisplayPattern();
		epdClearPatternMemory(0xff);// bit set = white, bit reset = black
		epdDisplayPattern();
	}
}

static void init_ulp_program(void) {
	esp_err_t err = ulp_load_binary(0, ulp_main_bin_start, (ulp_main_bin_end - ulp_main_bin_start) / sizeof(uint32_t));
	ESP_ERROR_CHECK(err);

	/* Configure ADC channel */
	/* Note: when changing channel here, also change 'adc_channel' constant
	 in adc.S */
	adc1_config_channel_atten(ADC1_CHANNEL_6, ADC_ATTEN_DB_11);
	adc1_config_width(ADC_WIDTH_BIT_12);
	adc1_ulp_enable();

	/* Set low and high thresholds, approx. 1.35V - 1.75V*/
	ulp_low_thr = 1500;
	ulp_high_thr = 2000;

	/* Set ULP wake up period to 20ms */
#define SLEEB20MS	20000
#define SLEEB10S	10000000

	ulp_set_wakeup_period(0, SLEEB10S);

	/* Disconnect GPIO12 and GPIO15 to remove current drain through
	 * pullup/pulldown resistors.
	 * GPIO12 may be pulled high to select flash voltage.
	 */
	rtc_gpio_isolate(GPIO_NUM_12);
	rtc_gpio_isolate(GPIO_NUM_15);
	esp_deep_sleep_disable_rom_logging(); // suppress boot messages
}

static void start_ulp_program(void) {
	/* Reset sample counter */
	ulp_sample_counter = 0;

	/* Start the program */
	esp_err_t err = ulp_run(&ulp_entry - RTC_SLOW_MEM);
	ESP_ERROR_CHECK(err);
}
