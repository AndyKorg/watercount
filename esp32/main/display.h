/*
 * display.h
 * Show different parameter
 */ 


#ifndef DISPLAY_H_
#define DISPLAY_H_

#include "stdint.h"

#define STRING_MARGIN_HIGHT			5				//margin from border display
#define STRING_MARGIN_LOW			3
#define	FONT_BIG
#define FONT_LOW

#define NULL_DATE					-1				//undefined time
#define NULL_DATE_STR				"неизвестно"	//replace 11.11.1111

#define	COUNT_SHOW_MAX				99999			//maximum number for big font

void displayInit(void);
void displayShow(void);
void displayPowerOff(void);							//Only off screen!

#endif /* DISPLAY_H_ */
