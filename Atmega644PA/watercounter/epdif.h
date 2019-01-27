/*
* HAL for 1.54inch e-Paper Module MH-ET live
*/

#ifndef _EPDIF_H
#define _EPDIF_H

#include "avr/io.h"
#include "avrlibtypes.h"
#include "bits_macros.h"

//#define EPD_SPI					//SPI через USI не удалось запустить - cs не синхронизирован с окочанием передачи Comment if Bit Bang 

#define EPD_PORT_DDR_OUT	DDRD
#define EPD_PORT_OUT		PORTD
#define EPD_PORT_DDR_IN		DDRD
#define EPD_PORT_IN			PIND

#define EPD_DC_PIN			PORTD3	//Data/Command control pin (High for data, and low for command)
#define EPD_RST_PIN			PORTD4	//External reset pin (Low for reset)
#define EPD_BUSY_PIN		PIND5	//Busy state output pin (Low for busy)

#define	EPD_DDR_CS			DDRD
#define EPD_PORT_CS			PORTD
#define EPD_CS_PIN			PORTD2	//SPI chip select (Low active)

#ifdef EPD_SPI
// EPD Pin definition for SPI
#define	EPD_SPI_NUM	1
	#undef	BAUD
	#define BAUD	9600
	#include <util/setbaud.h>
	/* IMPORTANT: The Baud Rate must be set after the transmitter is enabled */
	#if EPD_SPI_NUM == 0
		#define		InitSPI()	do {UBRR0 = 0;  DDRB |= (1<<PORTB0); UCSR0C = (1<<UMSEL01) | (1<<UMSEL00) | (0<<UCSZ01/*UCPHA0*/) | (1<<UCPOL0);  UCSR0B = (1<<TXEN0); UBRR0 = 1;} while (0);
		#define		EpdSendBusy()	( !( UCSR0A & (1<<UDRE0)) )
		#define		EPD_UDR		UDR0
	#elif EPD_SPI_NUM == 1
		#define		InitSPI()	do {UBRR1 = 0;  DDRD |= (1<<PORTD4); UCSR1C = (1<<UMSEL10) | (1<<UMSEL11) | (0<<UCSZ10/*UCPHA1*/) | (1<<UCPOL1);  UCSR1B = (1<<TXEN1); UBRR1 = 1;} while (0);
		#define		EpdSendBusy()	( !( UCSR1A & (1<<UDRE1)) )
		#define		EPD_UDR		UDR1
	#else
	    #error "Number SPI not valid!"
	#endif
#else
	// EPD Pin definition for Bit Bang
	#define EPD_DDR_OUT_SPI_BITBANG			DDRD
	#define EPD_PORT_OUT_SPI_BITBANG		PORTD

	#define	EPD_CLK_PIN_SPI_BITBANG			PORTD1	//SPI SCK
	#define EPD_DATA_PIN_SPI_BITBANG		PORTD0	//SPI MOSI

#endif // EPD_SPI

#define commandModeOn()	ClearBit(EPD_PORT_OUT, EPD_DC_PIN)
#define dataModeOn()	SetBit(EPD_PORT_OUT, EPD_DC_PIN)

unsigned char IfInit(void);
void Transfer(unsigned char data);

#endif
