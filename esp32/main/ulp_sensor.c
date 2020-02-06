/*
 * initialize ulp. sensors periferia - adc, and e.t.
 */
#include <time.h>
#include "esp_log.h"
#include "esp_sleep.h"
#include "esp32/ulp.h"
#include "driver/rtc_io.h"
#include "driver/gpio.h"
#include "ulp_main.h"
#include "ulp_sensor.h"

#include "driver/adc.h"
#include "esp_adc_cal.h"

//adc sensor, see adc.s
#define SENSOR_NAMUR_CHANAL 			ADC1_CHANNEL_6
#define SENSOR_NAMUR_ATTEN				ADC_ATTEN_11db
#define SENSOR_BAT_CHANAL 				ADC1_CHANNEL_3
#define SENSOR_BAT_ATTEN				ADC_ATTEN_11db
#define ADC_WIDTH_SENSOR				ADC_WIDTH_11Bit

//power enable sensor, see adc.s
#define SENSOR_POWER_EN					GPIO_NUM_4

/* Set ULP wake up period */
#define SLEEP20MS	20000				//20 ms
#define SLEEP100MS	100000				//0.1 second
#define SLEEP10S	10000000			//10 second
//rtc timer period
#define SleepPeriod	SLEEP100MS
#define TicPerSec	(1000000/SleepPeriod)
#define SleepPeriodSecSet(sec)			do{\
											ulp_sleep_countHi_tics = (sec * TicPerSec) & UINT16_MAX;\
											ulp_sleep_countLo_tics = ((sec * TicPerSec) >>16) & UINT16_MAX;\
										} while(0)


#define SecondSet(sec)					do{\
											ulp_secondLo = sec & UINT16_MAX;\
											ulp_secondHi = (sec>>16) & UINT16_MAX;\
										} while(0)
#define SecondGet()						((ulp_secondLo & UINT16_MAX) | ((ulp_secondHi <<16) & (UINT16_MAX << 16)))

extern const uint8_t ulp_main_bin_start[] asm("_binary_ulp_main_bin_start");
extern const uint8_t ulp_main_bin_end[] asm("_binary_ulp_main_bin_end");

static const char *TAG = "ULP";

void sensor_power_mode(void) {
//sensor power enabled
	rtc_gpio_init(SENSOR_POWER_EN);
	rtc_gpio_set_direction(SENSOR_POWER_EN, RTC_GPIO_MODE_OUTPUT_ONLY);
	rtc_gpio_pulldown_en(SENSOR_POWER_EN);
	rtc_gpio_pullup_dis(SENSOR_POWER_EN);
	//rtc_gpio_hold_en(SENSOR_POWER_EN);
}

esp_err_t init_ulp_program(void) {
	if (ulp_load_binary(0, ulp_main_bin_start, (ulp_main_bin_end - ulp_main_bin_start) / sizeof(uint32_t)) == ESP_OK) {
		ESP_LOGI(TAG, "ADC start");
		if ((adc1_config_channel_atten(SENSOR_NAMUR_CHANAL, SENSOR_NAMUR_ATTEN) == ESP_OK)
				&& (adc1_config_channel_atten(SENSOR_BAT_CHANAL, SENSOR_BAT_ATTEN) == ESP_OK)) {
			if (adc1_config_width(ADC_WIDTH_SENSOR) == ESP_OK) {
				adc1_ulp_enable();
				ESP_LOGI(TAG, "ADC config OK");
				/* Set low and high thresholds*/
				//TODO: get parameters adc sensor
				ulp_high_max_thr_sensor = 2000;
				ulp_high_thr_sensor = 1900;
				ulp_low_min_thr_sensor = 1500;
				ulp_low_thr_sensor = 1600;
				ulp_previous_sensor_value = 0;

				ulp_ticks_per_second = TicPerSec;
				ulp_tics_count = 0;
				SecondSet(0);

				SleepPeriodSecSet(0);//startup setting 0
				set_sleepMode(0);

				sensor_power_mode();
				ulp_set_wakeup_period(0, SleepPeriod);

				/* Disconnect GPIO12 and GPIO15 to remove current drain through
				 * pullup/pulldown resistors.
				 * GPIO12 may be pulled high to select flash voltage.
				 */
				rtc_gpio_isolate(GPIO_NUM_12);
//				rtc_gpio_isolate(GPIO_NUM_15);
				esp_deep_sleep_disable_rom_logging(); // suppress boot messages
				return ESP_OK;
			}
		}
	}
	return ESP_ERR_NOT_FOUND;
}

void set_ulp_SleepPeriod(uint32_t second){
	SleepPeriodSecSet(second);

	ESP_LOGI(TAG, "sleep period %x%x", ulp_sleep_countHi_tics, ulp_sleep_countLo_tics);
	ESP_LOGI(TAG, "seconds %d tics_count %d", SecondGet(), ulp_tics_count & UINT16_MAX);
	ESP_LOGI(TAG, "sensor level %d current %d state %d", ulp_previous_sensor_value & UINT16_MAX, ulp_last_result_sensor & UINT16_MAX,
			ulp_sensor_state & UINT16_MAX);
	ESP_LOGI(TAG, "sensor counter %d", ulp_sensor_counter & UINT16_MAX);
	ESP_LOGI(TAG, "bat %d", ulp_batarey_voltage & UINT16_MAX);

	sensor_power_mode(); // repair setting power sensor pin
}

void start_ulp_program(void) {
	/* Start the program */
	esp_err_t err = ulp_run(&ulp_entry - RTC_SLOW_MEM);
	ESP_ERROR_CHECK(err);
}

uint32_t get_sleepMode(void){
	return ulp_sleep_mode;
}

void set_sleepMode(uint32_t mode){
	ulp_sleep_mode = mode;
}
