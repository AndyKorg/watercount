/*
* watercounter.c
* Таймеры:
* 0 - используется bitbang SPI 
* 1 - не используется
* 2 - ассинхронный режим для пробуждения и измерения датчика воды
* Последовательность вызовов работы с SPI-ESP
* Отправляем команду из send_buf в main, после окончания передачи и получения ответа вызов esp_send_cb. В буфере recive_buf полученный ответ
* Ждем готовности esp на выводе ready вычитая время таймаута в прерывании отсчета времени
* Возникает прерывание готовности, в нем смотрим какую команду отработал esp и в зависимости от этого отправляем следующую команду.
*/

#include <avr/io.h>
#include <avr/power.h>
#include "epd1in54.h"
#include "epdpaint.h"

#include "spi_bitbang.h"
#include "power_timer.h"
#include "display.h"
#include "wifi.h"

#ifdef DEBUG
inline void
uart_9600(void)
{
	#define BAUD 19200
	#include <util/setbaud.h>
	UBRR1H = UBRRH_VALUE;
	UBRR1L = UBRRL_VALUE;
	#if USE_2X
	UCSR1A |= (1 << U2X1);
	#else
	UCSR1A &= ~(1 << U2X1);
	#endif
	UCSR1B = (1<<TXEN1);
}

#endif

int main(void){
	
	MCUCR = (1<<JTD);//jtag off
	MCUCR = (1<<JTD);//обязательно 2 раза сразу!

	WIFI_POWER_OFF();
	clock_prescale_set(clock_div_1);

	spi_init(esp_send_cb);
	power_init();
	
	sei();
#ifdef DEBUG
	uart_9600();
#else
	eInkInit();//Полная инициализация дисплея и отображение всего состояния
	epdInit(lut_partial_update);//Режим обновления кусками
#endif
	wifi_init();

	while(1){
		wifi_go();
#ifdef DEBUG
	if ( UCSR1A & (1<<UDRE1)){
		if (!(FIFO_IS_EMPTY(debugBuf))){
			UDR1 = FIFO_FRONT(debugBuf);
			FIFO_POP(debugBuf);
		}
	}
#else
		displayShow();
#endif
	}
}
