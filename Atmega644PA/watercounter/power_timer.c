/*
 * power_timer.c
 *
 * Created: 21.07.2019 11:44:34
 *  Author: Administrator
 */ 
#include "adc_diver.h"
#include "power_timer.h"
#include "wifi.h"

//Асинхронный таймер 16 раз в секунду
ISR(TIMER2_OVF_vect){
	static uint8_t tic = 16, esp_reboot = WIFI_NOT_NEED_ON;

	TCNT2 = 0;//Взвести TCN2UB для контроля готовности таймера
	while(ASSR & (1<<TCN2UB));// Ждем окончания предыдущей операции - сброса TCN2UB

	tic--;
	
	if (!(NAMUR_IS_OFF)) {
		PWR_ADC(PWR_ON);
		if ((!(ADCSRA & (1<<ADSC))) && (countMeasure == 0)){			//Предыдущее измерение закончено
			countMeasure = MEASURE_COUNT;
			ADCSRA |= (1<<ADSC);
		}
	}
	
	if (esp_reboot == WIFI_NEED_ON){									//Требуется перезагрузка чипа wifi
		esp_reboot = 0;
		WIFI_POWER_ON();
		command_reset();												//Отправить начальную команду снова после выставления готовности от esp
	}
	if (!tic){															//Прошла одна секунда
		system_tick();
		tic = 16;
		esp_reboot = command_timeout_check_and_off();
#ifdef DEBUG
		time_t t = time(NULL);
		if (t & 0x3){
			DispleyNeedRefresh();										//Требуется обновить дисплей
		}
		/*if (esp_reboot != WIFI_NEED_ON){
			if ((t & 7) == 0){
				if (WIFI_POWER_IS_OFF()){
					esp_reboot = WIFI_NEED_ON;
				}
			}
		}*/
#endif		
	}
}

void power_init(void){
	
	adc_sensor_init();		//Что бы при срабатывании таймера измеритель был готов
	
	//Отсчет времени
	ASSR = (1<<AS2);
	TCNT2 = 0;
	//TCCR2B = (1<<CS02) | (0<<CS01) | (1<<CS00);//clk/128 prescaller
	//TCCR2B = (1<<CS02) | (0<<CS01) | (0<<CS00);//clk/64 prescaller
	TCCR2B = (0<<CS02) | (1<<CS01) | (0<<CS00);//clk/8 prescaller
	//TCCR2B = (0<<CS02) | (0<<CS01) | (1<<CS00);//no prescaller
	TIMSK2 = (1<<TOIE2);	//interrupt overflow

	struct tm start_time;	//Начальное время
	start_time.tm_hour = 0;
	start_time.tm_min = 0;
	start_time.tm_sec = 0;
	start_time.tm_mday = 1;
	start_time.tm_mon = 1;
	start_time.tm_year = (2019-1900);
	
	set_system_time(mktime(&start_time));
}