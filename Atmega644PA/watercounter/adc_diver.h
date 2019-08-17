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

#define MEASURE_COUNT				6				//���������� ��������� � ������ (��� ������� 8��� ������� ���������� 700 ��), �� ������ 2! ��. sensorValue
#define POWER_PERIOD_MEASURE		16				//������ ���������� ���������� ������� � ���������� ��������� �������

// ������ ������������
#define NAMUR_PORT_STATE			PINA
#define NAMUR_PIN_STATE				PINA6			//��� ������ - namur ��� ������� �����������
#define SENSOR_DIGITAL				PINA7			//���� ������� ��� ������ �������� �����������
#define SENSOR_ANALOG				ADC7D			//�������� ���� �������, ����������� ���� ������������ namur
#define SENSOR_ANALOG_ON()			do {DIDR0 &= ~(1<<SENSOR_ANALOG);} while (0)
#define SENSOR_ANALOG_OFF()			do {DIDR0 |= (1<<SENSOR_ANALOG);} while (0)
#define SENSOR_MEASURE()			do {ADMUX = (1<<REFS1) | (1<<REFS0) | (0<<ADLAR) | (0<<MUX4) | (0<<MUX3) |(1<<MUX2) | (1<<MUX1) | (1<<MUX0); ADC_NOT_USE_OFF(); SENSOR_ANALOG_ON();} while (0) //REF=2,56V, ����� 7, ������������ ������
#define SENSOR_MIN					100				//����������� �������� ������� ������ ���������� �������
#define SENSOR_MAX					600				//������������ �������� �������� ������ ���������� �������
#define SENSOR_LEVEL_DIFF			200				//����������� �������� ������� ���������� �������
#define SENSOR_ALARM_NO				0				//��� ��������� � ��������
#define SENSOR_ALARM_SHOT_CIR		1				//�������� ���������
#define SENSOR_ALARM_BREAK			2				//����� �������
#define SENSOR_ALARM_LEVEL			3				//�� ������� ���������� ��������� ������� ��-�� ������������ �������

#define NAMUR_IS_OFF				(0)//(NAMUR_PORT_STATE & (1<<NAMUR_PIN_STATE))

extern volatile uint32_t countWater;				//���������� ��� ������� ����
extern volatile uint8_t countMeasure ;				//������� ��������� � ������, ���� �� 0, �� ��������� ADC �� ����� ��� ������
extern uint8_t alarmSensor;							//��������� �������
extern uint16_t preSensorValue;						//���������� �������� �������

//�������� ������� - �������� ������ ������� ����������
#define REF_1V1_ACCURATE			1078			//������ �������� �������� ���������� ��������� 1.1V ���������� �� 1000 (1,078*1000)
#define POWER_LOW					0x1b8			//2,49V ��� REF = 1,072V
#define SELF_SOURCE					((1<<MUX4) | (1<<MUX3) |(1<<MUX2) | (1<<MUX1) | (0<<MUX0))
#define POWER_MEASURE()				do {ADMUX = (0<<REFS1) | (1<<REFS0) | (0<<ADLAR) | SELF_SOURCE; ADC_NOT_USE_OFF(); SENSOR_ANALOG_OFF();} while (0) //REF=VCC, ����� AREF
#define IS_POWER_MEASURE			((ADMUX & ADC_MUX_MASK) == SELF_SOURCE)
extern time_t	dtCheckPower;						//����� ��������� �������� �������
extern volatile uint16_t batVoltage;				//���������� �� ������� � �������� ADC
#define POWER_IS_LOW()				(batVoltage >= POWER_LOW)	//������� ���������


void adc_sensor_init(void);

#endif /* SENSOR_H_ */