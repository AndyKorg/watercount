/*
 * sensor.h
 *
 * Created: 21.07.2019 11:01:09
 *  Author: Administrator
 */ 
#ifndef SENSOR_H_
#define SENSOR_H_

#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <avr/io.h>
#include <avr/interrupt.h>

#define ADC_MUX_MASK				((1<<MUX4) | (1<<MUX3) |(1<<MUX2) | (1<<MUX1) | (1<<MUX0))
#define ADC_NOT_USE_OFF()			do {DIDR0 = (1<<ADC0D) | (1<<ADC1D) | (1<<ADC2D) | (1<<ADC3D) | (1<<ADC4D) | (1<<ADC5D)| (1<<ADC6D) | (0<<ADC7D); DIDR1 = (1<<AIN1D) | (1<<AIN0D); } while (0)

#define MEASURE_COUNT				6				//Количество измерений в пакете (при частоте 8МГц частота измерерний 700 Гц), не меньше 2! см. sensorValue
#define POWER_PERIOD_MEASURE		16				//Период измерерния напряжения батареи в количестве измерений сенсора

// датчик водосчетчика
#define NAMUR_PORT_STATE			PINA
#define NAMUR_PIN_STATE				PINA6			//Чем мерить - namur или простой выключатель
#define SENSOR_DIGITAL				PINA7			//Нога датчика для режима простого выключателя
#define SENSOR_ANALOG				ADC7D			//Цифровая нога датчика, выключается если используется namur
#define SENSOR_ANALOG_ON()			do {DIDR0 &= ~(1<<SENSOR_ANALOG);} while (0)
#define SENSOR_ANALOG_OFF()			do {DIDR0 |= (1<<SENSOR_ANALOG);} while (0)
#define SENSOR_MEASURE()			do {ADMUX = (1<<REFS1) | (1<<REFS0) | (0<<ADLAR) | (0<<MUX4) | (0<<MUX3) |(1<<MUX2) | (1<<MUX1) | (1<<MUX0); ADC_NOT_USE_OFF(); SENSOR_ANALOG_ON();} while (0) //REF=2,56V, канал 7, выравнивание вправо
#define SENSOR_MIN					100				//Минимальное значение нижнего порога исправного сенсора
#define SENSOR_MAX					600				//Максимальное значение верхнего порога исправного сенсора
#define SENSOR_LEVEL_DIFF			200				//Минимальная разность порогов исправного сенсора
#define SENSOR_ALARM_NO				0				//Все нормально с датчиком
#define SENSOR_ALARM_SHOT_CIR		1				//Короткое замыкание
#define SENSOR_ALARM_BREAK			2				//Обрыв датчика
#define SENSOR_ALARM_LEVEL			3				//не удалось определить состояние датчика из-за неправильных уровней

#define NAMUR_IS_OFF				(0)//(NAMUR_PORT_STATE & (1<<NAMUR_PIN_STATE))

extern volatile uint32_t countWater;				//Собственно сам счетчик воды
extern volatile uint8_t countMeasure ;				//Счетчик измерений в пакете, если не 0, то выключать ADC на время сна нельзя
extern uint8_t alarmSensor;							//Состояние сенсора
extern uint16_t preSensorValue;						//Предыдущее значение сенсора

//Контроль питания - значение меньше большее напряжение
#define REF_1V1_ACCURATE			1078			//Точное значение опорного напряжения источника 1.1V умноженное на 1000 (1,078*1000)
#define POWER_LOW					0x1b8			//2,49V при REF = 1,072V
#define SELF_SOURCE					((1<<MUX4) | (1<<MUX3) |(1<<MUX2) | (1<<MUX1) | (0<<MUX0))
#define POWER_MEASURE()				do {ADMUX = (0<<REFS1) | (1<<REFS0) | (0<<ADLAR) | SELF_SOURCE; ADC_NOT_USE_OFF(); SENSOR_ANALOG_OFF();} while (0) //REF=VCC, канал AREF
#define IS_POWER_MEASURE			((ADMUX & ADC_MUX_MASK) == SELF_SOURCE)
extern time_t	dtCheckPower;						//Время последней проверки батареи
extern volatile uint16_t batVoltage;				//Напряжение на батарее в еденицах ADC
#define POWER_IS_LOW()				(batVoltage >= POWER_LOW)	//Батарея разряжена


void adc_sensor_init(void);

#endif /* SENSOR_H_ */