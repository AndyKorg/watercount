/*
 * display.h
 * Show different parameter
 */

#ifndef DISPLAY_H_
#define DISPLAY_H_

#include <stdint.h>
#include <stdbool.h>
#include "common.h"

#define STRING_MARGIN_HIGHT			5				//margin from border display
#define STRING_MARGIN_LOW			3
#define	FONT_BIG
#define FONT_LOW

#define NULL_DATE					-1				//undefined time
#define NULL_DATE_STR				"неизвестно"	//replace 11.11.1111

#define	TIMEZONE					"MSK-3"

#define	COUNT_SHOW_MAX				99999			//maximum number for big font

typedef void (*display_oper_end)(void);				//callback function display module operation end
typedef enum {
	cdClear, cdIniOnly,
} clear_dispaly_t;

void displayInit(clear_dispaly_t Cmd);				//And on screen
void displayShow(				//
		uint32_t sensor_count,		//
		sensor_status_t sensr,		//
		bool wifiPramIsSet,			//
		time_t sendBroker,			//
		uint32_t bat_mV,			//
		bool wifiModeAP,			//
		char *wifiAP_NetName,		//
		bool wifAP_clientConnect	//
		);
void displayPowerOff(void);							//Only off screen!

#endif /* DISPLAY_H_ */
