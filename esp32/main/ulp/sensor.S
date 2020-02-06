/*
   Gets the specified number of transitions in a global variable.
 ULP wakes up with a given period.
 Measures voltage on the sensor and on the battery.
 The result is put into the corresponding global variables.
 After each waking up, the number of periods decreases by 1.
 If the number of periods becomes equal to zero, the main processor wakes up.
*/

/* ULP assembly files are passed through C preprocessor first, so include directives
   and C macros may be used in these files 
 */
#include "soc/rtc_cntl_reg.h"
#include "soc/soc_ulp.h"
#include "soc/rtc_io_reg.h"

	/* ADC1 channel sensor, see ulp_sensor.c */
	.set adc_channel_sensor, 6
	/* sensor power enable, see ulp_sensor.c */
	.set sensor_power_enable, 10
	/* ADC1 batarey voltage, see ulp_sensor.c */
	.set adc_channel_batarey, 3

	/* Configure the number of ADC samples to average on each measurement.
	   For convenience, make it a power of 2. */
	.set adc_oversampling_factor_log, 2
	.set adc_oversampling_factor, (1 << adc_oversampling_factor_log)
	/* state sensor */
	.set sensor_normal, 0
	.set sensor_short_circuit, 1
	.set sensor_break_line, 2
	.set sensor_alarm_level, 3
	.set sensor_hight_level, 1
	.set sensor_low_level, 0

	/* Define variables, which go into .bss section (zero-initialized data) */
	.bss
/* sensor */
	/* Low ADC sensor reading threshold when it is turned off.*/
	.global low_thr_sensor
low_thr_sensor:
	.long 0
	/* minimum low ADC sensor reading threshold when it is shot circuit.*/
	.global low_min_thr_sensor
low_min_thr_sensor:
	.long 0
	/* Hight ADC sensor reading threshold when it is turned on.*/
	.global high_thr_sensor
high_thr_sensor:
	.long 0
	/* Hight maximum ADC sensor reading threshold when it is turned on.*/
	.global high_max_thr_sensor
high_max_thr_sensor:
	.long 0

	/* state sensor. see sensor_state_t */
	.global sensor_state
sensor_state:
	.long 0
	/* previous sensor value. */
	.global previous_sensor_value
previous_sensor_value:
	.long 0
	/* sensor_counter */
	.global sensor_counter
sensor_counter:
	.long 0
	/* sensor value last measurement */
	.global last_result_sensor
last_result_sensor:
	.long 0

/* batarey */
	/* batarey voltage */
	.global batarey_voltage
batarey_voltage:
	.long 0

/* second's timer */
	/* tics per second */
	.global ticks_per_second
ticks_per_second:
	.long 0
	/*counter tics */
	.global tics_count
tics_count:
	.long 0
	/* second counter 32 bit - two 16 bit counter*/
	.global secondLo
secondLo:
	.long 0
	.global secondHi
secondHi:
	.long 0

/* sleep timer */
	/* counter tics sleep period */
	.global sleep_countLo_tics
sleep_countLo_tics:
	.long 0
	.global sleep_countHi_tics
sleep_countHi_tics:
	.long 0
	/* mode for cpu, not used for ulp */
	.global sleep_mode
sleep_mode:
	.long 0

	/* Code goes into .text section */
	.text
	.global entry
entry:
	/* enable sensor */
	WRITE_RTC_REG(RTC_GPIO_OUT_REG, RTC_GPIO_OUT_DATA_S + sensor_power_enable, 1, 1)

	/* battarey measure */
	move r0, 0
	/* initialize the loop counter */
	stage_rst
measure_bat:
	/* measure and add value to accumulator */
	adc r1, 0, adc_channel_batarey + 1
	add r0, r0, r1
	/* increment loop counter and check exit condition */
	stage_inc 1
	jumps measure_bat, adc_oversampling_factor, lt

	/* divide accumulator by adc_oversampling_factor.
	   Since it is chosen as a power of two, use right shift */
	rsh r0, r0, adc_oversampling_factor_log
	/* averaged value is now in r0; store it into batarey_voltage */
	move r3, batarey_voltage
	st r0, r3, 0

	/* do measurements using ADC sensor */
	/* r0 will be used as accumulator */
	move r0, 0
	/* initialize the loop counter */
	stage_rst
measure_sensor:
	/* measure and add value to accumulator */
	adc r1, 0, adc_channel_sensor + 1
	add r0, r0, r1
	/* increment loop counter and check exit condition */
	stage_inc 1
	jumps measure_sensor, adc_oversampling_factor, lt

	/* disable sensor */
	WRITE_RTC_REG(RTC_GPIO_OUT_REG, RTC_GPIO_OUT_DATA_S + sensor_power_enable, 1, 0)

	/* divide accumulator by adc_oversampling_factor.
	   Since it is chosen as a power of two, use right shift */
	rsh r0, r0, adc_oversampling_factor_log
	/* averaged value is now in r0; store it into last_result */
	move r3, last_result_sensor
	st r0, r3, 0

	/* compare with low_thr_sensor; short circuit state if value < low_min_thr_sensor */
	move r2, sensor_break_line
	move r3, low_min_thr_sensor
	ld r3, r3, 0
	sub r3, r0, r3
	jump sensor_state_set, ov

	/* compare with high_thr_sensor; break state if value > high_max_thr_sensor */
	move r2, sensor_break_line
	move r3, high_max_thr_sensor
	ld r3, r3, 0
	sub r3, r3, r0
	jump sensor_state_set, ov

	move r1, previous_sensor_value
	/* compare with low_thr_sensor; state is 0 if value < low_thr */
	move r3, low_thr_sensor
	ld r3, r3, 0
	sub r3, r0, r3
	jump sensor_low_lvl, ov

	/* compare with high_thr_sensor; state is 1 if value > high_thr */
	move r3, high_thr_sensor
	ld r3, r3, 0
	sub r3, r3, r0
	move r0, sensor_hight_level
	jump normal_state, ov
	/* undefine voltage sensor */
	move r2, sensor_alarm_level
	jump sensor_state_set

	/* compare current level and prev_level */
sensor_low_lvl:
	ld r0, r1, 0
	sub r0, r0, 0
	move r0, sensor_low_level
	jump normal_state, eq
	/* add sensor_counter, no overflow check */
	move r3, sensor_counter
	ld r0, r3, 0
	add r0, r0, 1
	st r0, r3, 0

	/* normal state sensor set */
normal_state:
	st r0, r1, 0	/* r1 - is previous_sensor_value */
	move r2, sensor_normal
sensor_state_set:
	move r3, sensor_state
	st r2, r3, 0

	/*second counter */
	/* tic counter added and check per second*/
	move r3, tics_count
	ld r0, r3, 0
	jumpr check_sleep_period, 0, gt
	/* second add */
	move r2, secondLo
	ld r0, r2, 0
	add r0, r0, 1
	st r0, r2, 0
	jumpr second_add_end, 0, gt
	move r2, secondHi
	ld r0, r2, 0
	add r0, r0, 1
	st r0, r2, 0
second_add_end:

	/* one second, start new one second*/
	move r2, ticks_per_second
	ld r0, r2, 0
	add r0, r0, 1 /* add 1 for next iteration */

check_sleep_period:
	/* save count tics */
	sub r0, r0, 1
	st r0, r3, 0 /* r3 is tic counter */

	/* decrement sleep tics period */
	move r3, sleep_countLo_tics
	ld r0, r3, 0
	jumpr sleepLoZero, 0, eq
	sub r0, r0, 1
	st r0, r3, 0
	jump exit
sleepLoZero:
	move r3, sleep_countHi_tics
	ld r0, r3, 0
	jumpr wake_up, 0, eq /* sleep end, load new period */
	sub r0, r0, 1
	st r0, r3, 0

	.global exit
exit:
	halt

	.global wake_up
wake_up:
	/* Check if the system can be woken up */
	READ_RTC_FIELD(RTC_CNTL_LOW_POWER_ST_REG, RTC_CNTL_RDY_FOR_WAKEUP)
	and r0, r0, 1
	jump exit, eq

	/* Wake up the SoC, end program */
	wake
	//RTC timer continue work!
	/*WRITE_RTC_FIELD(RTC_CNTL_STATE0_REG, RTC_CNTL_ULP_CP_SLP_TIMER_EN, 0) RTC timer off*/
	halt

