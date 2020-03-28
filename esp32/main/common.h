/*
 * common types
 */

#ifndef MAIN_COMMON_H_
#define MAIN_COMMON_H_

typedef enum {
	SENSOR_ALARM_NO = 0,			//sensor is Ok
	SENSOR_ALARM_SHOT_CIR = 1,		//short circuit
	SENSOR_ALARM_BREAK = 2,			//Sensor break
	SENSOR_ALARM_LEVEL = 3			//Failed to determine sensor status due to incorrect levels
} sensor_state_t;

typedef struct {
	sensor_state_t status;
	time_t timeCheck;
} sensor_status_t;


#endif /* MAIN_COMMON_H_ */
