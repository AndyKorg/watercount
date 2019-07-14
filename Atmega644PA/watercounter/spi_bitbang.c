/*
 * spi_bitbang.c
 *
 * Created: 16.05.2019 23:16:25
 *  Author: Administrator
 */ 
#include <avr/interrupt.h>
#include "spi_bitbang.h"

#define SPI_SPEED				0xff	//делитель тактовой частоты - т.е. скорость передачи

#define SPI_PORT_INIT()					do {\
	SPI_CS_DDR |= (1<<SPI_CS); SPI_CS_OFF();\
	SPI_MOSI_DDR |= (1<<SPI_MOSI); SPI_MOSI_OFF();\
	SPI_CLK_DDR |= (1<<SPI_CLK); SPI_CLK_ON();\
} while (0)


spi_event_reciv_cb_t recive_internal_func = NULL;
spi_event_send_cb_t send_internal_func = NULL;

ISR(SPI_TIMER){
	static uint8_t bitCount = 0, byteOut, byteIn = 0;
	
	TCNT0 = SPI_SPEED;
	
	if (bitCount){
		if (bitCount & 1){		//Нечетные это действующий перепад
			SPI_CLK_ON();
			byteIn <<= 1;
			byteIn |= (SPI_MISO_IS_ON)?1:0;
			if (bitCount == 1){
//				FIFO_PUSH(recivBuf, byteIn);
				if (recive_internal_func){
					recive_internal_func();
				}
			}
		}
		else{
			byteOut >>= 1;
			SPI_MOSI_SET_BIT0(byteOut);
			SPI_CLK_OFF();
		}
		bitCount--;
	}
	else{
		if FIFO_IS_EMPTY(sendBuf){
			SPI_TIMER_CLI();
			if (send_internal_func){
				send_internal_func();
			}
			return;
		}
		byteOut = FIFO_FRONT(sendBuf);
		FIFO_POP(sendBuf);
		byteIn = 0;
		bitCount = 15;
		SPI_MOSI_SET_BIT0(byteOut);
		SPI_CLK_OFF();
	}
}

err_t spi_send_start(spi_event_send_cb_t send_end_func){
	if (!FIFO_IS_EMPTY(sendBuf)){
		SPI_TIMER_SEI();
		send_internal_func = send_end_func;
	}
	return ERR_OK;
}

err_t spi_init(spi_event_reciv_cb_t reciv_func){
	SPI_PORT_INIT();
	SPI_CS_OFF();
	TCCR0B |= (0<<CS02) | (0<<CS01) | (1<<CS00); //No prescaling
	TCNT0 = SPI_SPEED;
		
	recive_internal_func = reciv_func;
	
	FIFO_FLUSH(sendBuf);
	FIFO_FLUSH(recivBuf);
	
	return ERR_OK;
}

