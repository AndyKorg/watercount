/*
 * spi_bitbang.c
 *
 * Created: 16.05.2019 23:16:25
 *  Author: Administrator
 */ 
#include <avr/interrupt.h>
#include "spi_bitbang.h"

#define SPI_PORT_INIT()					do {\
	SPI_CS_DDR |= (1<<SPI_CS); SPI_CS_OFF();\
	SPI_MOSI_DDR |= (1<<SPI_MOSI); SPI_MOSI_ON();\
	SPI_CLK_DDR |= (1<<SPI_CLK); SPI_CLK_ON();\
	SPI_MISO_PULLUP();\
} while (0)


spi_event_send_cb_t end_internal_func = NULL;

ISR(SPI_TIMER){
	static uint8_t bitCount = 0, byteOut, byteIn = 0;
	
	TCNT0 = SPI_SPEED;

	if (bitCount){
		if (bitCount & 1){		//Нечетные это действующий перепад - читаем
			SPI_CLK_ON();
			byteIn >>= 1;
			byteIn &= 0x7f;
			byteIn |= (SPI_MISO_IS_ON)?0x80:0;
			if (bitCount == 1){
				if (!FIFO_IS_FULL(recivBuf)){
					FIFO_PUSH(recivBuf, byteIn);
					byteIn = 0;
				}
			}
		}
		else{ //
			byteOut >>= 1;
			SPI_CLK_OFF();
			SPI_MOSI_SET_BIT0(byteOut);
		}
		bitCount--;
	}
	else{
		if FIFO_IS_EMPTY(sendBuf){	//Передача закончена
			SPI_TIMER_CLI();
			SPI_TIMER_STOP();
			if (end_internal_func){
				end_internal_func();
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

err_t spi_send_start(){
	if (!FIFO_IS_EMPTY(sendBuf)){
		SPI_TIMER_SEI();
	}
	return ERR_OK;
}

err_t spi_init(spi_event_send_cb_t send_end_func){
	
	SPI_PORT_INIT();
	SPI_CS_OFF();
	SPI_TIMER_START();
		
	end_internal_func = send_end_func;
	
	FIFO_FLUSH(sendBuf);
	FIFO_FLUSH(recivBuf);
	
#ifdef DEBUG
FIFO_FLUSH(debugBuf);
FIFO_PUSH(debugBuf, 'R');
#endif
	
	return ERR_OK;
}

