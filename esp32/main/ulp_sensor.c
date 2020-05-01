/*
 * initialize ulp. sensors periferia - adc, and e.t.
 */
#include <time.h>
#include <string.h>
#include "esp_log.h"
#include "esp_sleep.h"
#include "esp32/ulp.h"
#include "driver/rtc_io.h"
#include "esp32/rom/rtc.h"
#include "driver/gpio.h"
#include "nvs.h"

#include "HAL_GPIO.h"

#include "ulp_main.h"
#include "ulp_sensor.h"
#include "params.h"

#include "driver/adc.h"
#include "esp_adc_cal.h"

/* period rtc */
#define SLEEP1MS	1000				//1 ms for internal 150kHz RC oscillator
#define SLEEP100MS	(SLEEP1MS*100)		//0.1 second
#define SLEEP1S		(SLEEP1MS*1000)		//1 second
#define SLEEP10S	(SLEEP1S*10)		//10 second

//rtc timer period
#define SLEEP_PERIOD	SLEEP100MS		//one tick sleep
#define TicPerSec	10					//Ticks sleep per second

//parameters for html page
#define	SENS_THR_LEN_MAX				6
#define STORAGE_SENS_THR				"sens_thr_namur"
#define SENS_THR_HIGH_MAX_PARAM			"sens_thr_h_max"
#define SENS_THR_HIGH_PARAM				"sens_thr_h"
#define SENS_THR_LOW_PARAM				"sens_thr_l"
#define SENS_THR_LOW_MIN_PARAM			"sens_thr_l_min"

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

typedef struct {
	uint16_t high_max;
	uint16_t high;
	uint16_t low_min;
	uint16_t low;
} sensor_threshold_t;

sensor_status_t sensor_state(void) {
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
	esp_adc_cal_characteristics_t *adc_chars = calloc(1, sizeof(esp_adc_cal_characteristics_t));
	esp_adc_cal_characterize(ADC_UNIT_1, SENSOR_BAT_ATTEN, ADC_WIDTH_SENSOR, DEFAULT_VREF, adc_chars);
	uint32_t ret = esp_adc_cal_raw_to_voltage((ulp_batarey_voltage & UINT16_MAX), adc_chars);
	free(adc_chars);
	//ESP_LOGI(TAG, "raw=%d mV=%d adc=%d", (ulp_batarey_voltage & UINT16_MAX), ret, adc1_get_raw(SENSOR_BAT_CHANAL)); adc1_get_raw conflict with ulp adc!
	return (uint32_t) (((uint64_t) ret * ADC_COEFF_BAT) / (uint64_t) 1000000);
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

//sensor threshold for namur in html page
sensor_threshold_t sensor_threshold(sensor_threshold_t *newValue) {
	if (newValue) {
		ulp_high_max_thr_sensor = (uint32_t) newValue->high_max;
		ulp_high_thr_sensor = (uint32_t) newValue->high;
		ulp_low_min_thr_sensor = (uint32_t) newValue->low_min;
		ulp_low_thr_sensor = (uint32_t) newValue->low;
	}
	sensor_threshold_t ret;
	ret.high_max = (uint16_t) (ulp_high_max_thr_sensor & UINT16_MAX);
	ret.high = (uint16_t) (ulp_high_thr_sensor & UINT16_MAX);
	ret.low_min = (uint16_t) (ulp_low_min_thr_sensor & UINT16_MAX);
	ret.low = (uint16_t) (ulp_low_thr_sensor & UINT16_MAX);
	return ret;
}

sensor_threshold_t sensor_threshold_default(void) {
	sensor_threshold_t ret;
	ret.high_max = SENSOR_THR_HIGH_MAX;
	ret.high = SENSOR_THR_HIGH;
	ret.low_min = SENSOR_THR_LOW_MIN;
	ret.low = SENSOR_THR_LOW;
	return ret;
}

esp_err_t read_thrSensor_param(const paramName_t paramName, char *value, size_t maxLen) {
	if (!strcmp(paramName, SENS_THR_HIGH_MAX_PARAM)) {
		sprintf(value, "%hu", ulp_high_max_thr_sensor & UINT16_MAX);
	} else if (!strcmp(paramName, SENS_THR_HIGH_PARAM)) {
		sprintf(value, "%hu", ulp_high_thr_sensor & UINT16_MAX);
	} else if (!strcmp(paramName, SENS_THR_LOW_PARAM)) {
		sprintf(value, "%hu", ulp_low_thr_sensor & UINT16_MAX);
	} else if (!strcmp(paramName, SENS_THR_LOW_MIN_PARAM)) {
		sprintf(value, "%hu", ulp_low_min_thr_sensor & UINT16_MAX);
	}
	return ESP_OK;
}

esp_err_t read_thrSensor_params(void) {
	nvs_handle my_handle;
	esp_err_t ret = ESP_ERR_NVS_NOT_FOUND;
	sensor_threshold_t threshold;

	if (strlen(STORAGE_SENS_THR) > PARAM_NAME_LEN) {
		return ESP_ERR_NVS_KEY_TOO_LONG;
	}
	if (nvs_open(STORAGE_SENS_THR, NVS_READONLY, &my_handle) == ESP_OK) {
		ESP_LOGI(TAG, "nvs open Ok");
		ret = nvs_get_u16(my_handle, SENS_THR_HIGH_MAX_PARAM, &threshold.high_max);
		if (ret == ESP_OK) {
			ret = nvs_get_u16(my_handle, SENS_THR_HIGH_PARAM, &threshold.high);
			if (ret == ESP_OK) {
				ret = nvs_get_u16(my_handle, SENS_THR_LOW_PARAM, &threshold.low);
				if (ret == ESP_OK) {
					ret = nvs_get_u16(my_handle, SENS_THR_LOW_MIN_PARAM, &threshold.low_min);
					if (ret == ESP_OK) {
						sensor_threshold(&threshold);
						ESP_LOGI(TAG, "threshold read ok");
					}
				}
			}
		}
	}
	nvs_close(my_handle);
	if (ret != ESP_OK) {
		threshold = sensor_threshold_default();
		sensor_threshold(&threshold);
		ESP_LOGI(TAG, "threshold default");
	}
	return ret;
}

esp_err_t write_thrSensor_param(const paramName_t paramName, const char *value, size_t maxLen) {
	sensor_threshold_t threshold = sensor_threshold(NULL);
	uint16_t *tmp = NULL;
	if (paramName && value) {
		if (strlen(value) <= maxLen) {
			ESP_LOGE(TAG, "thr name %s val %s", paramName, value);
			if (!strcmp(paramName, SENS_THR_HIGH_MAX_PARAM)) {
				tmp = &threshold.high_max;
			} else if (!strcmp(paramName, SENS_THR_HIGH_PARAM)) {
				tmp = &threshold.high;
			} else if (!strcmp(paramName, SENS_THR_LOW_PARAM)) {
				tmp = &threshold.low;
			} else if (!strcmp(paramName, SENS_THR_LOW_MIN_PARAM)) {
				tmp = &threshold.low_min;
			}
			if (tmp) {
				if (sscanf(value, "%hu", tmp) == 1) {
					sensor_threshold(&threshold);
				}
			}
			return ESP_OK;
		}
	}
	ESP_LOGE(TAG, "counter save error");
	return ESP_ERR_INVALID_ARG;
}

esp_err_t save_thrSensor_params(void) {

	nvs_handle my_handle;
	esp_err_t ret = ESP_ERR_INVALID_SIZE;
	sensor_threshold_t threshold = sensor_threshold(NULL);

	ESP_LOGI(TAG, "threshold save to nvs start");
	if (nvs_open(STORAGE_SENS_THR, NVS_READWRITE, &my_handle) == ESP_OK) {
		ESP_ERROR_CHECK(nvs_set_u16(my_handle, SENS_THR_HIGH_MAX_PARAM, threshold.high_max));
		ESP_ERROR_CHECK(nvs_set_u16(my_handle, SENS_THR_HIGH_PARAM, threshold.high));
		ESP_ERROR_CHECK(nvs_set_u16(my_handle, SENS_THR_LOW_PARAM, threshold.low));
		ESP_ERROR_CHECK(nvs_set_u16(my_handle, SENS_THR_LOW_MIN_PARAM, threshold.low_min));
		ESP_ERROR_CHECK(nvs_commit(my_handle));
		ESP_LOGI(TAG, "commit Ok");

		nvs_close(my_handle);
		ret = ESP_OK;
	}
	return ret;
}

esp_err_t init_ulp_program(void) {
	esp_err_t ret = ESP_ERR_NOT_FOUND;
	if (ulp_load_binary(0, ulp_main_bin_start, (ulp_main_bin_end - ulp_main_bin_start) / sizeof(uint32_t)) == ESP_OK) {
		ESP_LOGI(TAG, "ADC start");
		if ((adc1_config_channel_atten(SENSOR_NAMUR_CHANAL, SENSOR_NAMUR_ATTEN) == ESP_OK)
				&& (adc1_config_channel_atten(SENSOR_BAT_CHANAL, SENSOR_BAT_ATTEN) == ESP_OK)
				&& (adc1_config_channel_atten(SENSOR_PWR_CHANAL, SENSOR_NAMUR_ATTEN) == ESP_OK)
				) {
			if (adc1_config_width(ADC_WIDTH_SENSOR) == ESP_OK) {
				ESP_LOGI(TAG, "ADC config OK");
				/* Set low and high thresholds*/
				uint64_t power = ((uint64_t) adc1_get_raw(SENSOR_PWR_CHANAL)) * 100 * COEFF_POWER;
				//uint64_t bat = ((uint64_t) adc1_get_raw(SENSOR_BAT_CHANAL)) * 100 * COEFF_POWER;
				ESP_LOGI(TAG, "pwr = %d bat = %d", adc1_get_raw(SENSOR_PWR_CHANAL), adc1_get_raw(SENSOR_BAT_CHANAL));
				adc1_ulp_enable(); //WARNING! Only adc1_get_raw use! Else blocked read adc ulp!
				sensor_threshold_t threshold;
				threshold.high = (uint16_t)(power/COEFF_SENSOR_HI/COEFF_DEVIDER)-SENSOR_OFFSET;
				threshold.low = (uint16_t)(power/COEFF_SENSOR_LO/COEFF_DEVIDER)+SENSOR_OFFSET;
				threshold.high_max = threshold.high+(ADC_MAX_VALUE-threshold.high)/2;
				threshold.low_min = threshold.low-(threshold.low/2);
				sensor_threshold(&threshold);
				save_thrSensor_params();
				paramReg(SENS_THR_HIGH_MAX_PARAM, SENS_THR_LEN_MAX, read_thrSensor_param, write_thrSensor_param, save_thrSensor_params);
				paramReg(SENS_THR_HIGH_PARAM, SENS_THR_LEN_MAX, read_thrSensor_param, write_thrSensor_param, save_thrSensor_params);
				paramReg(SENS_THR_LOW_PARAM, SENS_THR_LEN_MAX, read_thrSensor_param, write_thrSensor_param, save_thrSensor_params);
				paramReg(SENS_THR_LOW_MIN_PARAM, SENS_THR_LEN_MAX, read_thrSensor_param, write_thrSensor_param, save_thrSensor_params);
				//read_thrSensor_params();

				ulp_previous_sensor_value = 0;

				ulp_ticks_per_second = TicPerSec;
				ulp_tics_count = 0;

				ulp_batarey_voltage = BAT_LOW + 1;		//from start application battery is nolrmal
				SecondSet(0);

				sensor_power_pin_enable();
				ret = ulp_set_wakeup_period(0, SLEEP_PERIOD);

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

void setSecond(time_t value) {
	SecondSet(value);
}

time_t getSecond() {
	return SecondGet();
}
