/*
 * power_timer.h
 *
 * Created: 21.07.2019 11:43:35
 *  Author: Administrator
 */ 


#ifndef POWER_TIMER_H_
#define POWER_TIMER_H_
#include <stdlib.h>
#include <string.h>
#include <avr/sleep.h>
#include <avr/power.h>
#include <avr/interrupt.h>
#include <time.h>

// Питание сенсора
#define SENSOR_POWER				PORTB1			//Подача питания на датчик

//Питание экрана
#define SCREEN_POWER_DDR			DDRD
#define SCREEN_POWER_PORT			PORTD
#define SCREEN_POWER_PIN			PORTD2			//Подача питания на экран
#define SCREEN_POWER_ON()			do {SCREEN_POWER_DDR |= (1<<SCREEN_POWER_PIN); SCREEN_POWER_PORT |= (1<<SCREEN_POWER_PIN); } while (0)
#define SCREEN_POWER_OFF()			do {SCREEN_POWER_DDR |= (1<<SCREEN_POWER_PIN); SCREEN_POWER_PORT &= ~(1<<SCREEN_POWER_PIN); } while (0)

//Выключение ненужных блоков на время сна 0 ТАЙМЕР ПОКА ВКЛЮЧИЛ! для spi
#define PWR_TIMER_ONLY()			do{						\
										PRR0  = (1<<PRTWI)	\
											| (0<<PRTIM2)	\
											| (1<<PRTIM1)	\
											| (0<<PRTIM0)	\
											| (1<<PRUSART1)	\
											| (1<<PRUSART0)	\
											| (1<<PRSPI)	\
											| (1<<PRADC);	\
									} while(0)

#define PWR_ON						0
#define PWR_OFF						1

#define PWR_ADC(x)					do{ if(x==PWR_ON) {PRR0 &= ~(1<<PRADC);} else {PRR0 |= (1<<PRADC);} } while(0)
#define PWR_USART1(x)				do{ if(x==PWR_ON) {PRR0 &= ~(1<<PRUSART1);} else {PRR0 |= (1<<PRUSART1);} } while(0)

void power_init(void);

#endif /* POWER_TIMER_H_ */