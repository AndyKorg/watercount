/*
 * wifi.h
 *
 * Created: 21.07.2019 10:48:44
 *  Author: Administrator
 */ 


#ifndef WIFI_H_
#define WIFI_H_

#include <stdlib.h>
#include <string.h>
#include <avr/io.h>
#include <avr/interrupt.h>
#include <time.h>
#include "display.h"
#include "FIFO.h"
#include "spi_bitbang.h"

//Питание esp
#define WIFI_POWER_DDR				DDRB
#define WIFI_POWER_PORT				PORTB
#define WIFI_POWER_PIN				PORTB3
#define WIFI_POWER_ON()				do {WIFI_POWER_DDR |= (1<<WIFI_POWER_PIN); WIFI_POWER_PORT |= (1<<WIFI_POWER_PIN); } while (0)
#define WIFI_POWER_OFF()			do {WIFI_POWER_DDR |= (1<<WIFI_POWER_PIN); WIFI_POWER_PORT &= ~(1<<WIFI_POWER_PIN); } while (0)
#ifdef DEBUG
#define WIFI_POWER_PORTIN			PINB
#define WIFI_POWER_IS_OFF()			((WIFI_POWER_PORTIN & (1<<WIFI_POWER_PIN)) == 0)
#endif

//wifi
#define WIFI_READY_PORT				PORTC
#define WIFI_READY_PORT_IN			PINC
#define WIFI_READY					PINC5
#define WIFI_IS_READY()				((WIFI_READY_PORT_IN & (1<<WIFI_READY)) == 0)
#define WIFI_ISR					PCINT2_vect			//Может пересекатся с прерыванием от SPI!
#define WIFI_READY_INTERUPT()		do {PCMSK2 |= (1<<PCINT21); PCICR |= (1<<PCIE2);} while (0)
#define WIFI_INIT_READY()			do{\
										WIFI_READY_PORT |= (1<<WIFI_READY);\
										WIFI_READY_INTERUPT();\
									}while(0)

//Опредление режима работы wifi
#define WIFI_MODE_PORT				PINB
#define WIFI_MODE_PIN				PINB4
#define WIFI_MODE_PULL_UP			PORTB
#define WIFI_MODE_INI()				do {WIFI_MODE_PULL_UP |= (1<<WIFI_MODE_PIN); } while (0);
#define WIFI_MODE_IS_STA			(WIFI_MODE_PORT & (1<<WIFI_MODE_PIN))	//Порт в воздухе - режим STA, посажен на землю - режим AP
//Тамаут в секундах
#define	COMMAND_TIMEOUT_SEC			20		//Максимальное время ожидания выполнния команды от esp практически во всех случаях кроме перечисленных ниже
#define COMMAND_TIMEOUT_WHITE_CLIENT (5*60)	//Ожидание подключения клиентов к АP при настройке устройства
#define COMMAND_ATTEMPT_MAX			3		//максимальное количество попыток выполнеия команды
#define WIFI_NEED_ON				1		//Надо включить wifi
#define WIFI_NOT_NEED_ON			0		//не надо

extern time_t dtSend;						//Время последней передачи

typedef enum{
	WIFI_STATE_OK,							//Все операции благополучно завершены
	WIFI_PARAM_NO,							//нет праметров Wifi
	CAYENN_PARAM_NO,						//нет парамтеров подключения к cayenn
	CAYENN_CONNECT_NO,						//не удалось подключится к брокеру
	WIFI_FAULT								//Какая то ошибка связи
} wifi_state_t;

extern wifi_state_t wifi_state;				//Состояние канала wifi

void wifi_init(void);
void wifi_go(void);			//Отправить команду на wifi если она есть в буфере
uint8_t esp_send_cb(void);	//Команда отправлена в esp и получен какой-то ответ - вызов из прерывания таймера spi!
void command_reset(void);	//Сбросить команду, что бы формировать ёе перезапуск
uint8_t command_timeout_check_and_off(void);
#ifdef DEBUG
uint16_t timout(void);		//Оставшееся время timout
#endif



#endif /* WIFI_H_ */