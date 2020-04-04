/*
 * Display of various parameters.
 */

#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "esp_attr.h"
#include "utils.h"
#include "display/epd1in54.h"
#include "display/epdpaint.h"

#include "esp_log.h"

#include "display.h"

//fixed cursor position for
RTC_SLOW_ATTR uint16_t countPos;		//Digit counter
RTC_SLOW_ATTR uint16_t freeAreaY;		//Free area after digit counter
RTC_SLOW_ATTR uint16_t powerPosX;		//battery voltage position x

//Positions cursor
int16_t powerPosY,					//Заряд батареи по X и по Y
		CheckBatPos,				//Надпись "проверено" для батареи
		dtCheckPos,					//Дата проверки батареи
		SendPos,					//Надпись "передано" для wifi
		dtSendPos					//Дата передачи данных на сервер
		;

//static const char *TAG = "ULP";

//just turn off the display, because inclusion is controlled by this module.
void displayPowerOff(void) {
	lcd_setup_pin(LCD_POWER_OFF);
}

// Prints the date, returns the position x at which the output was finished
int dtShow(time_t value, int x) {
	char s[2 + 1 + 2 + 1 + 4 + 1 + 2 + 1 + 2 + 1 + 2 + 1]; //dd.mm.yyyy hh:mm:ss(NULL)
	char cnv[10];

	memset(s, 0, sizeof(s));
	if (value == NULL_DATE) {
		strlcpy(s, NULL_DATE_STR, sizeof(s));
	} else {
		setenv("TZ", TIMEZONE, 1);
		tzset();
		struct tm t;
		localtime_r(&value, &t);
		twoDigit(t.tm_mday, s, sizeof(s));
		strlcat(s, ".", sizeof(s));
		twoDigit(t.tm_mon + 1, s, sizeof(s));
		strlcat(s, ".", sizeof(s));
		strlcat(s, itoa((t.tm_year + 1900), cnv, 10), sizeof(s));
		strlcat(s, " ", sizeof(s));
		twoDigit(t.tm_hour, s, sizeof(s));
		strlcat(s, ":", sizeof(s));
		twoDigit(t.tm_min, s, sizeof(s));
		strlcat(s, ":", sizeof(s));
		twoDigit(t.tm_sec, s, sizeof(s));
	}
	epdClear(UNCOLORED); //Clear paint
	return epdDrawStringAt(x, 0, s, &FontStreched72, COLORED);
}

//Show battery voltage from x position.
//return end position
int batShow(int x, uint32_t bat_mV) {
	char s[10], tmp[10];
	memset(s, 0, sizeof(s));
	memset(tmp, 0, sizeof(tmp));
	strlcat(s, " ", sizeof(s));
	if (bat_mV > 1000) {
		strlcat(s, itoa((bat_mV / 1000), tmp, 10), sizeof(s));
	} else {
		strlcat(s, "0", sizeof(s));
	}
	strlcat(s, ".", sizeof(s));
	strlcat(s, itoa(((bat_mV - ((bat_mV / 1000) * 1000)) / 10), tmp, 10), sizeof(s));
	strlcat(s, " V", sizeof(s));
	return epdDrawStringAt(x, 0, s, &FontStreched72, COLORED);
}

//Initialise one buffer display
void ePatternMemoryInit() {
	uint32_t i = 0;
	uint16_t cursor = 4;
	countPos = cursor;
	char s[100];

	//Цифры счетчика
	epdClear(UNCOLORED);
	epdDrawStringAt(0, 0, itoa(i++, s, 10), &Fontfont39pixel_h_digit, COLORED);
	epdSetPatternMemory(Paint.image, 0, cursor, Paint.width, Fontfont39pixel_h_digit.Height + 5);

	//Линия разделения
	cursor += Fontfont39pixel_h_digit.Height + 5;
	epdClear(UNCOLORED);
	epdDrawHorizontalLine(0, 0, 200, COLORED);
	epdSetPatternMemory(Paint.image, 0, cursor, Paint.width, 2); //Рисуем в памаяти epd

	cursor += 2;
	freeAreaY = cursor;
	//Состояние батареи
	powerPosY = cursor;
	epdClear(UNCOLORED);
	memset(s, 0, sizeof(s));
	strlcpy(s, "батарея", sizeof(s));
	powerPosX = epdDrawStringAt(0, 0, s, &FontStreched72, COLORED);
	//batShow(0, 0);
	epdSetPatternMemory(Paint.image, 0, powerPosY, Paint.width, FontStreched72.Height + 3);

	//Заголовок даты проверки
	cursor += FontStreched72.Height + 3;
	CheckBatPos = cursor;
	epdClear(UNCOLORED);
	memset(s, 0, sizeof(s));
	strlcpy(s, "проверено", sizeof(s));
	epdDrawStringAt(0, 0, s, &FontStreched72, COLORED);
	epdSetPatternMemory(Paint.image, 0, cursor, Paint.width, FontStreched72.Height + 3);

	//Дата проверки
	cursor += FontStreched72.Height + 3;
	dtCheckPos = cursor;
	memset(s, 0, sizeof(s));
	strlcpy(s, NULL_DATE_STR, sizeof(s));
	epdSetPatternMemory(Paint.image, 0, cursor, Paint.width, FontStreched72.Height + 3);

	//Заголовок даты отправки
	cursor += FontStreched72.Height + 3;
	SendPos = cursor;
	epdClear(UNCOLORED);
	memset(s, 0, sizeof(s));
	strlcpy(s, "передано", sizeof(s));
	epdDrawStringAt(0, 0, s, &FontStreched72, COLORED);
	epdSetPatternMemory(Paint.image, 0, cursor, Paint.width, FontStreched72.Height + 3);

	//Дата отправки
	cursor += FontStreched72.Height + 3;
	dtSendPos = cursor;
	epdClear(UNCOLORED);
	//dtShow(CayenneGetLastLinkDate(), 0);
	epdSetPatternMemory(Paint.image, 0, cursor, Paint.width, FontStreched72.Height + 3);
}

void displayInit(clear_dispaly_t Cmd) {

	Paint.rotate = ROTATE_0;
	Paint.width = 200;			//Холст для рисования для одной строки
	Paint.height = 80;

	if (epdImageInit() == EXIT_SUCCESS) {
		if (Cmd == cdClear) {
			/**
			 *  there are 2 memory areas embedded in the e-paper display
			 *  and once the display is refreshed, the memory area will be auto-toggled,
			 *  i.e. the next action of SetPatternMemory will set the other memory area
			 *  therefore you have to clear the pattern memory twice.
			 */
			epdClearPatternMemory(0xff); // bit set = white, bit reset = black
			epdDisplayPattern();
			epdClearPatternMemory(0xff); //Очистка второй памяти

			ePatternMemoryInit();	//Первую заполнили
			epdDisplayPattern();	//out display
			ePatternMemoryInit();	//вторую заполнили
		}
	}
}

int digitalShowDebug(char *value) {
	//Выводим на место даты передачи на время отдладки
	static char prev[10];
	char s[10];
	memset(s, 0, sizeof(s));
	strlcpy(s, value, sizeof(s));
	if (strncmp(s, prev, sizeof(s))) {
		memset(prev, 0, sizeof(s));
		strlcpy(prev, s, sizeof(s));
		epdClear(UNCOLORED);	//Очистить paint для рисования
		epdDrawStringAt(0, 0, s, &FontStreched72, COLORED);
		epdSetPatternMemory(Paint.image, 0, CheckBatPos, Paint.width, FontStreched72.Height + 3);
		epdDisplayPattern();	//out display
	}
	return 0;
}

void displayShow(				//
		uint32_t sensor_count,		//
		sensor_status_t sensr,		//
		bool wifiPramIsSet,			//
		time_t sendBroker,			//
		uint32_t bat_mV,			//
		bool wifiModeAP,			//
		char *wifiAP_NetName,		//
		bool wifAP_clientConnect	//
		) {
	char s[100];

	lcd_setup_pin(LCD_POWER_ON);

	//battery voltage
	epdClear(UNCOLORED);
	batShow(0, bat_mV);	//from 0, word "battery" already show
	epdSetPatternMemory(Paint.image, powerPosX, powerPosY, Paint.width, FontStreched72.Height + 3);

	//Цифры счетчика
	uint32_t tmp = sensor_count;
	if (tmp > COUNT_SHOW_MAX) {
		tmp = COUNT_SHOW_MAX;
		epdClear(UNCOLORED);
		memset(s, 0, sizeof(s));
		strlcpy(s, "ПЕРЕПОЛНЕНИЕ", sizeof(s));
		epdDrawStringAt(0, 0, s, &FontStreched72, COLORED);
		epdSetPatternMemory(Paint.image, 0, CheckBatPos, Paint.width, FontStreched72.Height + 3);
	}
	epdClear(UNCOLORED);
	memset(s, 0, sizeof(s));
	epdDrawStringAt(0, 0, itoa(tmp, s, 10), &Fontfont39pixel_h_digit, COLORED);
	epdSetPatternMemory(Paint.image, 0, countPos, Paint.width, Fontfont39pixel_h_digit.Height + 5);
	//Дата измерения батареи или авария датчика
	epdClear(UNCOLORED);
	switch (sensr.status) {
	case SENSOR_ALARM_BREAK:
		epdDrawStringAt(0, 0, "ОБРЫВ", &FontStreched72, COLORED);
		break;
	case SENSOR_ALARM_SHOT_CIR:
		epdDrawStringAt(0, 0, "ЗАМЫКАНИЕ", &FontStreched72, COLORED);
		break;
	case SENSOR_ALARM_NO:
		//дата измерения батареи
		dtShow(sensr.timeCheck, 0);
		break;
	default:
		break;
	}
	epdSetPatternMemory(Paint.image, 0, dtCheckPos, Paint.width, FontStreched72.Height + 3);

	//wifi and client mqtt state
	epdClear(UNCOLORED);
	if (wifiModeAP) {
		memset(s, 0, sizeof(s));
		sprintf(s, "AP вкл.клиенты:%s", wifAP_clientConnect?"да":"нет");
		epdDrawStringAt(0, 0, s, &FontStreched72, COLORED);
	} else if (wifiPramIsSet) {
		epdDrawStringAt(0, 0, "передано", &FontStreched72, COLORED);
	} else {
		epdDrawStringAt(0, 0, "wifi не настроен", &FontStreched72, COLORED);
	}
	epdSetPatternMemory(Paint.image, 0, SendPos, Paint.width, FontStreched72.Height + 3);

	//time send to broker
	epdClear(UNCOLORED);
	if (wifiModeAP) {
		epdDrawStringAt(0, 0, wifiAP_NetName, &FontStreched72, COLORED);
	} else {
		dtShow(sendBroker, 0);
	}
	epdSetPatternMemory(Paint.image, 0, dtSendPos, Paint.width, FontStreched72.Height + 3);

	epdDisplayPattern();	//out display
}

