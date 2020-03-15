/*
 * ulp coprocessor init for sensors
 */

#ifndef MAIN_ULP_SENSOR_H_
#define MAIN_ULP_SENSOR_H_
#include <time.h>
#include <stdint.h>

typedef enum {
	SENSOR_ALARM_NO = 0,			//sensor is Ok
	SENSOR_ALARM_SHOT_CIR = 1,		//short circuit
	SENSOR_ALARM_BREAK = 2,			//Sensor break
	SENSOR_ALARM_LEVEL = 3			//Failed to determine sensor status due to incorrect levels
} sensor_state_t;

/* This function is called once after power-on reset, to load ULP program into
 * RTC memory and configure the ADC and e.t.
 */
esp_err_t init_ulp_program(void);
esp_err_t start_ulp_program(void);
void set_ulp_SleepPeriod(uint32_t second);
uint32_t sensor_count(uint32_t *newValue);//if newValue is null then no set new value counter, only return current value
uint16_t sensor_raw(void);	//raw last result sensor
bool battery_low(void);		//battery is low
uint32_t bat_voltage(void);
void RTC_IRAM_ATTR wake_stub(void); // Function which runs after exit from deep sleep
sensor_state_t sensor_state(void);

#endif /* MAIN_ULP_SENSOR_H_ */
