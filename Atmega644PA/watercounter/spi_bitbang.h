/*
 * spi_bitbang.h
 *
 * Created: 16.05.2019 23:09:36
 *  Author: Administrator
 */ 


#ifndef SPI_BITBANG_H_
#define SPI_BITBANG_H_

#include <avr/io.h>
#include <stdlib.h>
#include <string.h>
#include "FIFO.h"

#define SPI_CS_DDR					DDRC
#define SPI_CS_PORT					PORTC
#define SPI_CS						PORTC4
#define SPI_CS_OFF()				do {SPI_CS_PORT |= (1<<SPI_CS);} while (0)
#define SPI_CS_SELECT()				do {SPI_CS_PORT &= ~(1<<SPI_CS);} while (0)

#define SPI_MISO_DDR				DDRD
#define SPI_MISO_PORT				PIND
#define SPI_MISO					PIND0
#define SPI_MISO_IS_ON				(SPI_MISO_PORT & (1<<SPI_MISO))

#define SPI_MOSI_DDR				DDRD
#define SPI_MOSI_PORT				PORTD
#define SPI_MOSI					PORTD1
#define SPI_MOSI_ON()				do {SPI_MOSI_PORT |= (1<<SPI_MOSI);} while (0)
#define SPI_MOSI_OFF()				do {SPI_MOSI_PORT &= ~(1<<SPI_MOSI);} while (0)
#define SPI_MOSI_SET_BIT0(x)		do {if (x & 1) {SPI_MOSI_ON();} else {SPI_MOSI_OFF();}} while (0);

#define SPI_CLK_DDR					DDRB
#define SPI_CLK_PORT				PORTB
#define SPI_CLK						PORTB0
#define SPI_CLK_ON()				do {SPI_CLK_PORT |= (1<<SPI_CLK);} while (0)
#define SPI_CLK_OFF()				do {SPI_CLK_PORT &= ~(1<<SPI_CLK);} while (0)

#define SPI_TIMER					TIMER0_OVF_vect
#define SPI_TIMER_SEI()				do{	TIMSK0 |= (1<<TOIE0);}while(0)
#define SPI_TIMER_CLI()				do{	TIMSK0 &= ~(1<<TOIE0);}while(0)

typedef int8_t err_t;

#define ERR_OK          0       /*!< esp_err_t value indicating success (no error) */
#define ERR_FAIL        -1      /*!< Generic esp_err_t code indicating failure */

#define SPI_BUF_SIZE				32
FIFO(SPI_BUF_SIZE) sendBuf, recivBuf;	

typedef uint8_t (*spi_event_reciv_cb_t)(void);
typedef uint8_t (*spi_event_send_cb_t)(void);

err_t spi_init(spi_event_reciv_cb_t reciv_func);
err_t spi_send_start(spi_event_send_cb_t send_end_func);

#endif /* SPI_BITBANG_H_ */