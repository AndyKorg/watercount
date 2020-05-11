/*
 * common types
 */

#ifndef MAIN_COMMON_H_
#define MAIN_COMMON_H_

#include "HAL_GPIO.h"

typedef enum {
	SENSOR_ALARM_NO = SENSOR_STATE_NORMAL,					//sensor is Ok
	SENSOR_ALARM_SHOT_CIR = SENSOR_STATE_SHOT_CIRCUIT,		//short circuit
	SENSOR_ALARM_BREAK = SENSOR_STATE_BREAK_LINE,			//Sensor break
	SENSOR_ALARM_LEVEL = SENSOR_STATE_ALARM_LEVEL			//Failed to determine sensor status due to incorrect levels
} sensor_state_t;

typedef struct {
	sensor_state_t status;
	time_t timeCheck;
} sensor_status_t;

#endif /* MAIN_COMMON_H_ */
