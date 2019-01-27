/*
* watercounter.c
*
* Created: 08.01.2019 13:49:01
* Author : Administrator
* Only ASCII! for font 
*/
#include <avr/io.h>
#include <stdlib.h>
#include <string.h>
#include <avr/sleep.h>
#include <avr/power.h>
#include "epd1in54.h"
#include "epdpaint.h"

#include <util/delay.h>



int main(void)
{
epdDrawCharAt(0, 0, '0', &FontStreched72, 0);
	
	epdInit(lut_full_update);
	/**
	*  there are 2 memory areas embedded in the e-paper display
	*  and once the display is refreshed, the memory area will be auto-toggled,
	*  i.e. the next action of SetPatternMemory will set the other memory area
	*  therefore you have to clear the pattern memory twice.
	*/
	epdClearPatternMemory(0xff);// bit set = white, bit reset = black
	epdDisplayPattern();
	epdClearPatternMemory(0xff);
	epdDisplayPattern();
	
	Paint.rotate = ROTATE_0;
	Paint.width = 200;
	Paint.height = 80;
	
	uint32_t i = 12345;
	uint16_t cursor = 4;
	char s[100];

	if (epdImageInit() == EXIT_SUCCESS){
		epdClear(UNCOLORED);
		//epdDrawStringAt(0, 0, "Hello world!", &Font24, COLORED);
		epdDrawStringAt(0, 0, itoa(i++, s, 10), &Fontfont39pixel_h_digit, COLORED);
		epdSetPatternMemory(Paint.image, 0, cursor, Paint.width, Fontfont39pixel_h_digit.Height+5);
		cursor += Fontfont39pixel_h_digit.Height+5;
		
		epdClear(UNCOLORED);
		epdDrawHorizontalLine(0, 0, 200, COLORED);
		epdSetPatternMemory(Paint.image, 0, cursor, Paint.width, 2);
		cursor += 2;

		epdClear(UNCOLORED);
		memset(s, 0, sizeof(s));
		strlcpy(s, "батарея 100%", sizeof("батарея 100%"));
		epdDrawStringAt(0, 0, s, &FontStreched72, COLORED);
		epdSetPatternMemory(Paint.image, 0, cursor, Paint.width, FontStreched72.Height+3);
		cursor += FontStreched72.Height+3;

		epdClear(UNCOLORED);
		memset(s, 0, sizeof(s));
		strlcpy(s, "проверено", sizeof("проверено"));
		epdDrawStringAt(0, 0, s, &FontStreched72, COLORED);
		epdSetPatternMemory(Paint.image, 0, cursor, Paint.width, FontStreched72.Height+3);
		cursor += FontStreched72.Height+3;

		epdClear(UNCOLORED);
		memset(s, 0, sizeof(s));
		strlcpy(s, "10.10.2019", sizeof("10.10.2019"));
		epdDrawStringAt(0, 0, s, &FontStreched72, COLORED);
		epdSetPatternMemory(Paint.image, 0, cursor, Paint.width, FontStreched72.Height+3);
		cursor += FontStreched72.Height+3;
		
		epdClear(UNCOLORED);
		memset(s, 0, sizeof(s));
		strlcpy(s, "передано", sizeof("передано"));
		epdDrawStringAt(0, 0, s, &FontStreched72, COLORED);
		epdSetPatternMemory(Paint.image, 0, cursor, Paint.width, FontStreched72.Height+3);
		cursor += FontStreched72.Height+3;

		epdClear(UNCOLORED);
		memset(s, 0, sizeof(s));
		strlcpy(s, "10.10.2019", sizeof("10.10.2019"));
		epdDrawStringAt(0, 0, s, &FontStreched72, COLORED);
		epdSetPatternMemory(Paint.image, 0, cursor, Paint.width, FontStreched72.Height+3);
		cursor += FontStreched72.Height+3;

		epdDisplayPattern();
	}
	
	while(1){
		/*		if (i>=100000){
		i = 0;
		}
		epdClear(UNCOLORED);
		epdDrawStringAt(0, 0, itoa(i++, s, 10), &Fontfont39pixel_h_digit, COLORED);
		epdSetPatternMemory(Paint.image, 0, 0, Paint.width, Paint.height);
		epdDisplayPattern();
		_delay_ms(1000);*/
	}
}

