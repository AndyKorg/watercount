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

/* period rtc */
#define SLEEP1MS	1000				//1 ms for internal 150kHz RC oscillator
#define SLEEP100MS	(SLEEP1MS*100)		//0.1 second
#define SLEEP1S		(SLEEP1MS*1000)		//1 second
#define SLEEP10S	(SLEEP1S*10)		//10 second

//rtc timer period
#define SleepPeriod	SLEEP100MS			//one tick sleep
#define TicPerSec	(10*SleepPeriod)

#define SleepPeriodSecSet(sec)			do{\
											ulp_sleep_countHi_tics = (sec * TicPerSec) & UINT16_MAX;\
											ulp_sleep_countLo_tics = ((sec * TicPerSec) >>16) & UINT16_MAX;\
										} while(0)

#define SecondSet(sec)					do{\
											ulp_secondLo = sec & UINT16_MAX;\
											ulp_secondHi = (sec>>16) & UINT16_MAX;\
										} while(0)
#define SecondGet()						((ulp_secondLo & UINT16_MAX) | ((ulp_secondHi <<16) & (UINT16_MAX << 16)))

#define CounterGet()					((ulp_sensor_countLo & UINT16_MAX) | ((ulp_sensor_countHi <<16) & (UINT16_MAX << 16)))
#define CounterSet(val)					do{\
											ulp_sensor_countLo = val & UINT16_MAX;\
											ulp_sensor_countHi = (val>>16) & UINT16_MAX;\
										} while(0)

extern const uint8_t ulp_main_bin_start[] asm("_binary_ulp_main_bin_start");
extern const uint8_t ulp_main_bin_end[] asm("_binary_ulp_main_bin_end");

static const char *TAG = "ULP";

uint32_t sensor_count(uint32_t *newValue){
	if (newValue){
		CounterSet(*newValue);
	}
	return CounterGet();
}

void sensor_power_pin_enable(void) {
	//sensor power enabled for controlling from ulp
	rtc_gpio_init(SENSOR_POWER_EN);
	rtc_gpio_set_direction(SENSOR_POWER_EN, RTC_GPIO_MODE_OUTPUT_ONLY);
	rtc_gpio_pulldown_dis(SENSOR_POWER_EN);
	rtc_gpio_pullup_en(SENSOR_POWER_EN);
}

esp_err_t init_ulp_program(void) {
	esp_err_t ret = ESP_ERR_NOT_FOUND;
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

				sensor_power_pin_enable();
				ret = ulp_set_wakeup_period(0, SleepPeriod);

				esp_deep_sleep_disable_rom_logging(); // suppress boot messages
			}
		}
	}
	return ret;
}

void set_ulp_SleepPeriod(uint32_t second) {
	SleepPeriodSecSet(second);

	ESP_LOGI(TAG, "sleep period %x%x", ulp_sleep_countHi_tics, ulp_sleep_countLo_tics);
	ESP_LOGI(TAG, "seconds %d tics_count %d", SecondGet(), ulp_tics_count & UINT16_MAX);
	ESP_LOGI(TAG, "sensor level %d current %d state %d", ulp_previous_sensor_value & UINT16_MAX, ulp_last_result_sensor & UINT16_MAX,
			ulp_sensor_state & UINT16_MAX);
	ESP_LOGI(TAG, "sensor counter %d", sensor_count(NULL));
	ESP_LOGI(TAG, "bat %d", ulp_batarey_voltage & UINT16_MAX);

	sensor_power_pin_enable(); // repair setting power sensor pin
}

esp_err_t start_ulp_program(void) {
	return ulp_run(&ulp_entry - RTC_SLOW_MEM);
}

