/*
 * ulp coprocessor init for sensors
 */

#ifndef MAIN_ULP_SENSOR_H_
#define MAIN_ULP_SENSOR_H_
#include <time.h>
#include <stdint.h>
#include <stdbool.h>
#include "esp_attr.h"
#include "common.h"

typedef struct {
	uint16_t high_max;
	uint16_t high;
	uint16_t low_min;
	uint16_t low;
} sensor_threshold_t;

/* This function is called once after power-on reset, to load ULP program into
 * RTC memory and configure the ADC and e.t.
 */
esp_err_t init_ulp_program(void);
esp_err_t start_ulp_program(void);

void set_ulp_SleepPeriod(uint32_t second);

uint32_t sensor_count(uint32_t *newValue);	//if newValue is null then no set new value counter, only return current value
uint16_t sensor_raw(void);	//raw last result sensor
sensor_status_t sensor_state(void);
sensor_threshold_t sensor_threshold(sensor_threshold_t *newValue);


bool battery_low(void);		//battery is low
uint32_t bat_voltage(void);

void RTC_IRAM_ATTR wake_stub(void); // Function which runs after exit from deep sleep

void setSecond(time_t value);
time_t getSecond();

#endif /* MAIN_ULP_SENSOR_H_ */
