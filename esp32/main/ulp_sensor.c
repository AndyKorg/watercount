/*
 * initialize ulp. sensors periferia - adc, and e.t.
 */
#include <time.h>
#include "esp_log.h"
#include "esp_sleep.h"
#include "esp32/ulp.h"
#include "driver/rtc_io.h"
#include "esp32/rom/rtc.h"
#include "driver/gpio.h"

#include "HAL_GPIO.h"

#include "ulp_main.h"
#include "ulp_sensor.h"

#include "driver/adc.h"
#include "esp_adc_cal.h"

//threshold sensors
#define BAT_LOW							990 //3v low threshold
#define ADC_COEFF_BAT_mV				1700 //correction factor of the voltage divider

/* period rtc */
#define SLEEP1MS	1000				//1 ms for internal 150kHz RC oscillator
#define SLEEP100MS	(SLEEP1MS*100)		//0.1 second
#define SLEEP1S		(SLEEP1MS*1000)		//1 second
#define SLEEP10S	(SLEEP1S*10)		//10 second

//rtc timer period
#define SleepPeriod	SLEEP100MS			//one tick sleep
#define TicPerSec	10					//Ticks sleep per second

//set sleep period in seconds
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

#define SensorCheckTime()				((ulp_sensor_check_timeLo & UINT16_MAX) | ((ulp_sensor_check_timeHi <<16) & (UINT16_MAX << 16)))

extern const uint8_t ulp_main_bin_start[] asm("_binary_ulp_main_bin_start");
extern const uint8_t ulp_main_bin_end[] asm("_binary_ulp_main_bin_end");

static const char *TAG = "ULP";

sensor_status_t sensor_state(void){
	sensor_status_t ret;
	ret.status = ulp_sensor_state & UINT16_MAX;
	ret.timeCheck = SensorCheckTime();
	return ret;
}

//raw last result sensor
uint16_t sensor_raw(void) {
	return ((uint16_t) (ulp_last_result_sensor & UINT16_MAX));
}

uint32_t sensor_count(uint32_t *newValue) {
	if (newValue) {
		CounterSet(*newValue);
	}
	return CounterGet();
}

uint32_t bat_voltage(void) {
	static esp_adc_cal_characteristics_t *adc_chars;

	adc_chars = calloc(1, sizeof(esp_adc_cal_characteristics_t));
	esp_adc_cal_value_t type = esp_adc_cal_characterize(ADC_UNIT_1, SENSOR_BAT_ATTEN, ADC_WIDTH_SENSOR, 1100, adc_chars);
	ESP_LOGI(TAG, "type vref %d", type);
	return (esp_adc_cal_raw_to_voltage((ulp_batarey_voltage & UINT16_MAX), adc_chars)+ADC_COEFF_BAT_mV);
}

bool battery_low(void) {
	return (ulp_batarey_voltage & UINT16_MAX) <= BAT_LOW;
}

void RTC_IRAM_ATTR wake_stub(void) {
	if ((ulp_batarey_voltage & UINT16_MAX) > BAT_LOW) {
		// On revision 0 of ESP32, this function must be called:
		esp_default_wake_deep_sleep();
		// Return from the wake stub function to continue
		// booting the firmware.
		return;
	}
	// Set the pointer of the wake stub function.
	REG_WRITE(RTC_ENTRY_ADDR_REG, (uint32_t )&wake_stub);
	// Go to sleep.
	CLEAR_PERI_REG_MASK(RTC_CNTL_STATE0_REG, RTC_CNTL_SLEEP_EN);
	SET_PERI_REG_MASK(RTC_CNTL_STATE0_REG, RTC_CNTL_SLEEP_EN);
	// A few CPU cycles may be necessary for the sleep to start...
	while (true) {
		;
	}
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
				ulp_high_max_thr_sensor = 1700;
				ulp_high_thr_sensor = 1300;
				ulp_low_min_thr_sensor = 300;
				ulp_low_thr_sensor = 700;
				ulp_previous_sensor_value = 0;

				ulp_ticks_per_second = TicPerSec;
				ulp_tics_count = 0;

				ulp_batarey_voltage = BAT_LOW+1;//from start application battery is nolrmal
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
	ESP_LOGI(TAG, "sensor prev_level %d current %d state %d", ulp_previous_sensor_value & UINT16_MAX, ulp_last_result_sensor & UINT16_MAX,
			ulp_sensor_state & UINT16_MAX);
	ESP_LOGI(TAG, "sensor counter %d", sensor_count(NULL));
	ESP_LOGI(TAG, "bat %d", ulp_batarey_voltage & UINT16_MAX);

	sensor_power_pin_enable(); // repair setting power sensor pin
}

esp_err_t start_ulp_program(void) {
	return ulp_run(&ulp_entry - RTC_SLOW_MEM);
}

void setSecond(time_t value){
	SecondSet(value);
}

time_t getSecond(){
	return SecondGet();
}
