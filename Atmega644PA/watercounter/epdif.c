/*
* HAL for 1.54inch e-Paper Module MH-ET live
*/

#include "epdif.h"
#include <util/delay.h>

#define		csEnable()		do {ClearBit(EPD_PORT_CS, EPD_CS_PIN); } while (0)
#define		csDisable()		do {SetBit(EPD_PORT_CS, EPD_CS_PIN); } while (0)

#ifndef EPD_SPI
#define		clkDown()		do {ClearBit(EPD_PORT_OUT_SPI_BITBANG, EPD_CLK_PIN_SPI_BITBANG);} while (0)
#define		clkUp()			do {SetBit(EPD_PORT_OUT_SPI_BITBANG, EPD_CLK_PIN_SPI_BITBANG); } while (0)
#endif

void Transfer(unsigned char data) {
#ifdef EPD_SPI
	while (EpdSendBusy());
	csEnable();
	EPD_UDR = data;
	while (!( UCSR1A & (1<<TXC1)) );
#else
	u08 i = 0, dt = data;
	clkUp();
	csEnable();
	//_delay_us(300);
	for(;i<8;i++){
		clkDown();
		SetBitVal(EPD_PORT_OUT_SPI_BITBANG, EPD_DATA_PIN_SPI_BITBANG, ((dt & 0x80)?1:0));
		dt = dt << 1;
		//_delay_us(300);
		clkUp();
		//_delay_us(300);
	}
#endif
	csDisable();
	//_delay_us(300);
}

unsigned char IfInit(void) {
	EPD_DDR_CS |= ((1<<EPD_CS_PIN));
	csDisable();
	EPD_PORT_DDR_OUT |= ((1<<EPD_RST_PIN) | (1<<EPD_DC_PIN));
	EPD_PORT_DDR_IN &= (~(1<<EPD_BUSY_PIN));
	EPD_PORT_IN = (1<<EPD_BUSY_PIN);

#ifdef EPD_SPI
	InitSPI();
#else
	EPD_DDR_OUT_SPI_BITBANG |= ((1<<EPD_DATA_PIN_SPI_BITBANG) | (1<<EPD_CLK_PIN_SPI_BITBANG));
	SetBit(EPD_PORT_OUT_SPI_BITBANG, EPD_DATA_PIN_SPI_BITBANG);
	clkUp();
#endif
	return 0;
}
