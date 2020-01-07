/*
 * Display of various parameters.
 */

#include "display.h"

#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "display/epd1in54.h"
#include "display/epdpaint.h"

#include "wifi.h"
#include "cayenne.h"
#include "ulp_sensor.h"


//fixed cursor position for
uint16_t countPos,					//Digit counter
		powerPosX, powerPosY,		// ����� ������� �� X � �� Y
		sCheckBatPos,				//������� "���������" ��� �������
		dtCheckPos,					//���� �������� �������
		sSendPos,					//������� "��������" ��� wifi
		dtSendPos					//���� �������� ������ �� ������
		;

//debug!
uint8_t REF_1V1_ACCURATE = 1, batVoltage = 1;
time_t dtCheckPower;
sensor_state_t alarmSensor = SENSOR_ALARM_NO;		//TODO: move to ulp_main
uint32_t countWater;		//TODO: move to ulp_main

//just turn off the display, because inclusion is controlled by this module.
void displayPowerOff(void){
	lcd_power(LCD_POWER_OFF);
}

inline void twoDigit(int value, char *str, size_t size) {
	char cnv[10];
	if (value < 10) {
		strlcat(str, "0", size);
	}
	strlcat(str, itoa(value, cnv, 10), size);
}

// Prints the date, returns the position x at which the output was finished
int dtShow(time_t value, int x) {
	char s[2 + 1 + 2 + 1 + 4 + 1 + 2 + 1 + 2 + 1 + 2 + 1]; //dd.mm.yyyy hh:mm:ss(NULL)
	char cnv[10];

	memset(s, 0, sizeof(s));
	if (value == NULL_DATE) {
		strlcpy(s, NULL_DATE_STR, sizeof(s));
	} else {
		struct tm *t = localtime(&value);
		if (t) {
			twoDigit(t->tm_mday, s, sizeof(s));
			strlcat(s, ".", sizeof(s));
			twoDigit(t->tm_mon + 1, s, sizeof(s));
			strlcat(s, ".", sizeof(s));
			strlcat(s, itoa((t->tm_year + 1900), cnv, 10), sizeof(s));
			strlcat(s, " ", sizeof(s));
			twoDigit(t->tm_hour, s, sizeof(s));
			strlcat(s, ":", sizeof(s));
			twoDigit(t->tm_min, s, sizeof(s));
			strlcat(s, ":", sizeof(s));
			twoDigit(t->tm_sec, s, sizeof(s));
		}
	}
	epdClear(UNCOLORED); //Clear paint
	return epdDrawStringAt(x, 0, s, &FontStreched72, COLORED);
}

//������� ���������� �� ������� �� ���� ������ ����� �������, ���������� ������� � �� ������ ��� �������� �����
int batShow(int x) {
	char s[10], cnv[10];
	memset(s, 0, sizeof(s));
	if (batVoltage) {
		uint32_t mV = ((uint32_t) REF_1V1_ACCURATE * 1023) / batVoltage;
		uint8_t intV = mV / 1000;
		uint8_t fracV = (mV - (intV * 1000)) / 10;
		strlcat(s, " ", sizeof(s));
		strlcat(s, itoa(intV, cnv, 10), sizeof(s));
		strlcat(s, ",", sizeof(s));
		strlcat(s, itoa(fracV, cnv, 10), sizeof(s));
		strlcat(s, " V", sizeof(s));
	} else {
		strlcat(s, " NA", sizeof(s));
	}
	return epdDrawStringAt(x, 0, s, &FontStreched72, COLORED);
}

//������������� ����� ������
void ePatternMemoryInit() {
	uint32_t i = 0;
	uint16_t cursor = 4;
	countPos = cursor;
	char s[100];

	//����� ��������
	epdClear(UNCOLORED);
	epdDrawStringAt(0, 0, itoa(i++, s, 10), &Fontfont39pixel_h_digit, COLORED);
	epdSetPatternMemory(Paint.image, 0, cursor, Paint.width, Fontfont39pixel_h_digit.Height + 5);

	//����� ����������
	cursor += Fontfont39pixel_h_digit.Height + 5;
	epdClear(UNCOLORED);
	epdDrawHorizontalLine(0, 0, 200, COLORED);
	epdSetPatternMemory(Paint.image, 0, cursor, Paint.width, 2); //������ � ������� epd

	//��������� �������
	cursor += 2;
	powerPosY = cursor;
	epdClear(UNCOLORED);
	memset(s, 0, sizeof(s));
	strlcpy(s, "�������", sizeof(s));
	powerPosX = epdDrawStringAt(0, 0, s, &FontStreched72, COLORED);
	batShow(powerPosX);
	epdSetPatternMemory(Paint.image, 0, powerPosY, Paint.width, FontStreched72.Height + 3);

	//��������� ���� ��������
	cursor += FontStreched72.Height + 3;
	sCheckBatPos = cursor;
	epdClear(UNCOLORED);
	memset(s, 0, sizeof(s));
	strlcpy(s, "���������", sizeof(s));
	epdDrawStringAt(0, 0, s, &FontStreched72, COLORED);
	epdSetPatternMemory(Paint.image, 0, cursor, Paint.width, FontStreched72.Height + 3);

	//���� ��������
	cursor += FontStreched72.Height + 3;
	dtCheckPos = cursor;
	dtShow(dtCheckPower, 0);
	epdSetPatternMemory(Paint.image, 0, cursor, Paint.width, FontStreched72.Height + 3);

	//��������� ���� ��������
	cursor += FontStreched72.Height + 3;
	sSendPos = cursor;
	epdClear(UNCOLORED);
	memset(s, 0, sizeof(s));
	strlcpy(s, "��������", sizeof(s));
	epdDrawStringAt(0, 0, s, &FontStreched72, COLORED);
	epdSetPatternMemory(Paint.image, 0, cursor, Paint.width, FontStreched72.Height + 3);

	//���� ��������
	cursor += FontStreched72.Height + 3;
	dtSendPos = cursor;
	epdClear(UNCOLORED);
	dtShow(CayenneGetLastLinkDate(), 0);
	epdSetPatternMemory(Paint.image, 0, cursor, Paint.width, FontStreched72.Height + 3);
}

void displayInit(void) {
	/**
	 *  there are 2 memory areas embedded in the e-paper display
	 *  and once the display is refreshed, the memory area will be auto-toggled,
	 *  i.e. the next action of SetPatternMemory will set the other memory area
	 *  therefore you have to clear the pattern memory twice.
	 */
	epdClearPatternMemory(0xff); // bit set = white, bit reset = black
	epdDisplayPattern();
	epdClearPatternMemory(0xff); //������� ������ ������
	epdDisplayPattern();

	Paint.rotate = ROTATE_0;
	Paint.width = 200;			//����� ��� ��������� ��� ����� ������
	Paint.height = 80;

	if (epdImageInit() == EXIT_SUCCESS) {

		ePatternMemoryInit();	//������ ���������
		epdDisplayPattern();	//out display
		ePatternMemoryInit();	//������ ���������
		epdDisplayPattern();	//out display
	}
}

int digitalShowDebug(char *value) {
	//������� �� ����� ���� �������� �� ����� ��������
	static char prev[10];
	char s[10];
	memset(s, 0, sizeof(s));
	strlcpy(s, value, sizeof(s));
	if (strncmp(s, prev, sizeof(s))) {
		memset(prev, 0, sizeof(s));
		strlcpy(prev, s, sizeof(s));
		epdClear(UNCOLORED);	//�������� paint ��� ���������
		epdDrawStringAt(0, 0, s, &FontStreched72, COLORED);
		epdSetPatternMemory(Paint.image, 0, sCheckBatPos, Paint.width, FontStreched72.Height + 3);
		epdDisplayPattern();	//out display
	}
	return 0;
}

void displayShow(void) {
	char s[100];

	lcd_power(LCD_POWER_ON);

	//����� �������
	epdClear(UNCOLORED);
	batShow(0);				//����� "�������" ��� ���� �� ������, ������� � ����� ��������� �� 0
	epdSetPatternMemory(Paint.image, powerPosX, powerPosY, Paint.width, FontStreched72.Height + 3);

	//����� ��������
	uint32_t tmp = countWater;
	if (tmp > COUNT_SHOW_MAX) {
		tmp = COUNT_SHOW_MAX;
		epdClear(UNCOLORED);
		memset(s, 0, sizeof(s));
		strlcpy(s, "������������", sizeof(s));
		epdDrawStringAt(0, 0, s, &FontStreched72, COLORED);
		epdSetPatternMemory(Paint.image, 0, sCheckBatPos, Paint.width, FontStreched72.Height + 3);
	}
	epdClear(UNCOLORED);
	memset(s, 0, sizeof(s));
	epdDrawStringAt(0, 0, itoa(tmp, s, 10), &Fontfont39pixel_h_digit, COLORED);
	epdSetPatternMemory(Paint.image, 0, countPos, Paint.width, Fontfont39pixel_h_digit.Height + 5);
	//���� ��������� ������� ��� ������ �������
	epdClear(UNCOLORED);
	switch (alarmSensor) {
	case SENSOR_ALARM_BREAK:
		epdDrawStringAt(0, 0, "�����", &FontStreched72, COLORED);
		break;
	case SENSOR_ALARM_SHOT_CIR:
		epdDrawStringAt(0, 0, "���������", &FontStreched72, COLORED);
		break;
	case SENSOR_ALARM_NO:
		//���� ��������� �������
		dtShow(dtCheckPower, 0);
		break;
	default:
		break;
	}
	epdSetPatternMemory(Paint.image, 0, dtCheckPos, Paint.width, FontStreched72.Height + 3);

	//wifi and client mqtt state
	epdClear(UNCOLORED);
	if (wifi_paramIsEmpty()) {
		epdDrawStringAt(0, 0, "��������", &FontStreched72, COLORED);
	}
	else{
		epdDrawStringAt(0, 0, "wifi �� ��������", &FontStreched72, COLORED);
	}
	epdSetPatternMemory(Paint.image, 0, sSendPos, Paint.width, FontStreched72.Height + 3);

	//time send to broker
	epdClear(UNCOLORED);
	dtShow(CayenneGetLastLinkDate(), 0);
	epdSetPatternMemory(Paint.image, 0, dtSendPos, Paint.width, FontStreched72.Height + 3);

	epdDisplayPattern();	//out display
}

