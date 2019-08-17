/*
 * sensor.c
 *
 * Created: 21.07.2019 11:01:22
 *  Author: Administrator
 */ 
#include "adc_diver.h"
#include "display.h"

volatile uint8_t countMeasure = 0;		//������� ��������� � ������, ���� �� 0, �� ��������� ADC �� ����� ��� ������
uint16_t preSensorValue = 0;			//���������� �������� �������
uint8_t alarmSensor	= 0;				//��������� �������
volatile uint32_t countWater = 0;		//���������� ��� ������� ����
time_t	dtCheckPower = NULL_DATE;		//����� ��������� �������� �������
volatile uint16_t batVoltage;			//���������� �� ������� � �������� ADC


ISR(ADC_vect){
	static uint32_t tmpSum = 0;
	static uint8_t countS = 0;
	uint16_t adc = ADCW;			//����������� ���� ���������
	
	if (countMeasure < (MEASURE_COUNT-1)){
		tmpSum += adc;
	}

	if (countMeasure){				//���� ��� ��� ��������
		ADCSRA |= (1<<ADSC);
		countMeasure--;
	}
	else{							//���� ��������� ������ ����������
		if (IS_POWER_MEASURE){
			batVoltage = tmpSum/(MEASURE_COUNT-1);
			dtCheckPower = time(NULL);
		}
		else{
			tmpSum = tmpSum/(MEASURE_COUNT-1);
			if (tmpSum < SENSOR_MIN){  //�.�. �������
				alarmSensor = SENSOR_ALARM_SHOT_CIR;
			}
			else if (tmpSum > SENSOR_MAX){	//����� �������
				alarmSensor = SENSOR_ALARM_BREAK;
			}
			else {
				if (preSensorValue >= tmpSum){	//��� �� �� ������� � ��������������� � �������� ����������
					if ((preSensorValue - tmpSum) >= SENSOR_LEVEL_DIFF){ //�� �������� (off) � ������� (on)
						countWater++;
					}
				}
				alarmSensor = SENSOR_ALARM_NO;
			}
			preSensorValue = tmpSum;
		}
		//���������� � ���������� ���������
		tmpSum = 0;
		if (countS--){
			SENSOR_MEASURE();
		}
		else{
			POWER_MEASURE();
			countS = POWER_PERIOD_MEASURE;
		}
	}
}

void adc_sensor_init(void){

	PORTA |= (1<<NAMUR_PIN_STATE);//��������� ������� namur � 1
	if NAMUR_IS_OFF {
		//��� ����������� �������� ���� �� ����������
	}
	else{
		//SENSOR_ANALOG_CHANAL();
		POWER_MEASURE();
		ADCSRA = (1<<ADEN)		//Enable
				| (0<<ADSC)		//no start
				| (0<<ADATE)	//autotrigger off
				| (0<<ADIF)		//flag intrrupt reset
				| (1<<ADIE)		//Interrupt go
				| (0<<ADPS2) | (0<<ADPS1) | (0<<ADPS0) //prescaller 2
				;
		ADCSRB = (0<<ACME)		//Comparator off
				| (0<<ADTS2)	//Free mode
				| (0<<ADTS1)
				| (0<<ADTS0)
				;
	}

	countMeasure = 0;
}
