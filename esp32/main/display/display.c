/*
 * display.c
 *
 */ 

#include <avr/io.h>
#include <stdlib.h>
#include <string.h>
#include <avr/sleep.h>
#include <avr/power.h>
#include <avr/interrupt.h>
#include <time.h>
#include "epd1in54.h"
#include "epdpaint.h"

#include "power_timer.h"
#include "adc_diver.h"
#include "wifi.h"
#include "display.h"

//������� �������
uint16_t countPos,					//����� ��������
		powerPosX, powerPosY,		//����� ������� �� X � �� Y
		sCheckBatPos,				//������� "���������" ��� �������
		dtCheckPos,					//���� �������� �������
		sSendPos,					//������� "��������" ��� wifi
		dtSendPos					//���� �������� ������ �� ������
		;

volatile uint8_t disp_state;		//������ �������
#define DISP_PROCESS_REFRESH_BIT	0	//���� ������� ���������� �������
#define DISP_NEED_REFRESH_BIT		1	//��������� �������� �������
#define displayBusy()				do {disp_state |= (1<<DISP_PROCESS_REFRESH_BIT); } while (0);
#define displayFree()				do {disp_state &= ~(1<<DISP_PROCESS_REFRESH_BIT); } while (0);

void DispleyNeedRefresh(void){
	disp_state |= (1<<DISP_NEED_REFRESH_BIT);
}

uint8_t DisplayRefreshProcess(){
	return disp_state & (1<<DISP_PROCESS_REFRESH_BIT);
}

inline void twoDigit(int value, char* str, size_t size){
	char cnv[10];
	if (value<10){
		strlcat(str, "0", size);
	}
	strlcat(str, itoa(value, cnv, 10), size);
}

// ������� ����, ���������� ������� � �� ������ ��� �������� �����
int dtShow(time_t* value, int x){
	char s[2+1+2+1+4 +1 +2+1+2+1+2 +1]; //dd.mm.yyyy hh:mm:ss(NULL)
	char cnv[10];
	
	memset(s, 0, sizeof(s));
	if (*value == NULL_DATE){
		strlcpy(s, NULL_DATE_STR, sizeof(s));
	} 
	else {
		struct tm* t = localtime(value);
		twoDigit(t->tm_mday, s, sizeof(s));
		strlcat(s, ".", sizeof(s));
		twoDigit(t->tm_mon+1, s, sizeof(s));
		strlcat(s, ".", sizeof(s));
		strlcat(s, itoa((t->tm_year+1900), cnv, 10), sizeof(s));
		strlcat(s, " ", sizeof(s));
		twoDigit(t->tm_hour, s, sizeof(s));
		strlcat(s, ":", sizeof(s));
		twoDigit(t->tm_min, s, sizeof(s));
		strlcat(s, ":", sizeof(s));
		twoDigit(t->tm_sec, s, sizeof(s));
	}
	epdClear(UNCOLORED);//�������� paint ��� ���������
	return epdDrawStringAt(x, 0, s, &FontStreched72, COLORED);
}

//������� ���������� �� ������� �� ���� ������ ����� �������, ���������� ������� � �� ������ ��� �������� �����
int batShow(int x){
	char s[10], cnv[10];
	memset(s, 0, sizeof(s));
	if (batVoltage){
		uint32_t mV = ((uint32_t)REF_1V1_ACCURATE*1023)/batVoltage;
		uint8_t intV = mV/1000;
		uint8_t fracV = (mV-(intV*1000))/10;
		strlcat(s, " ", sizeof(s));
		strlcat(s, itoa(intV, cnv, 10), sizeof(s));
		strlcat(s, ",", sizeof(s));
		strlcat(s, itoa(fracV, cnv, 10), sizeof(s));
		strlcat(s, " V", sizeof(s));
	}
	else{
		strlcat(s, " NA", sizeof(s));
	}
	return epdDrawStringAt(x, 0, s, &FontStreched72, COLORED);
}

//������������� ����� ������
void ePatternMemoryInit(){
	uint32_t i = 0;
	uint16_t cursor = 4;
	countPos = cursor;
	char s[100];

		//����� ��������
		epdClear(UNCOLORED);
		epdDrawStringAt(0, 0, itoa(i++, s, 10), &Fontfont39pixel_h_digit, COLORED);
		epdSetPatternMemory(Paint.image, 0, cursor, Paint.width, Fontfont39pixel_h_digit.Height+5);

		//����� ����������
		cursor += Fontfont39pixel_h_digit.Height+5;
		epdClear(UNCOLORED);
		epdDrawHorizontalLine(0, 0, 200, COLORED);
		epdSetPatternMemory(Paint.image, 0, cursor, Paint.width, 2);//������ � ������� epd
		
		//��������� �������
		cursor += 2;
		powerPosY = cursor;
		epdClear(UNCOLORED);
		memset(s, 0, sizeof(s));
		strlcpy(s, "�������", sizeof(s));
		powerPosX = epdDrawStringAt(0, 0, s, &FontStreched72, COLORED);
		batShow(powerPosX);
		epdSetPatternMemory(Paint.image, 0, powerPosY, Paint.width, FontStreched72.Height+3);
		
		//��������� ���� ��������
		cursor += FontStreched72.Height+3;
		sCheckBatPos = cursor;
		epdClear(UNCOLORED);
		memset(s, 0, sizeof(s));
		strlcpy(s, "���������", sizeof(s));
		epdDrawStringAt(0, 0, s, &FontStreched72, COLORED);
		epdSetPatternMemory(Paint.image, 0, cursor, Paint.width, FontStreched72.Height+3);
		
		//���� ��������
		cursor += FontStreched72.Height+3;
		dtCheckPos = cursor;
		dtShow(&dtCheckPower, 0);
		epdSetPatternMemory(Paint.image, 0, cursor, Paint.width, FontStreched72.Height+3);
		
		//��������� ���� ��������
		cursor += FontStreched72.Height+3;
		sSendPos = cursor;		
		epdClear(UNCOLORED);
		memset(s, 0, sizeof(s));
		strlcpy(s, "��������", sizeof(s));
		epdDrawStringAt(0, 0, s, &FontStreched72, COLORED);
		epdSetPatternMemory(Paint.image, 0, cursor, Paint.width, FontStreched72.Height+3);
		
		//���� ��������
		cursor += FontStreched72.Height+3;
		dtSendPos = cursor;
		epdClear(UNCOLORED);
		dtShow(&dtSend, 0);
		epdSetPatternMemory(Paint.image, 0, cursor, Paint.width, FontStreched72.Height+3);
}

void eInkInit(void){
	SCREEN_POWER_ON();
	displayBusy();
	epdInit(lut_full_update);
	/**
	*  there are 2 memory areas embedded in the e-paper display
	*  and once the display is refreshed, the memory area will be auto-toggled,
	*  i.e. the next action of SetPatternMemory will set the other memory area
	*  therefore you have to clear the pattern memory twice.
	*/
	epdClearPatternMemory(0xff);// bit set = white, bit reset = black
	epdDisplayPattern();
	epdClearPatternMemory(0xff); //������� ������ ������
	epdDisplayPattern();
	
	Paint.rotate = ROTATE_0;
	Paint.width = 200;			//����� ��� ��������� ��� ����� ������
	Paint.height = 80;
	
	if (epdImageInit() == EXIT_SUCCESS){
		
		ePatternMemoryInit();	//������ ���������
		epdDisplayPattern();	//out display
		ePatternMemoryInit();	//������ ���������
		epdDisplayPattern();	//out display
	}
	displayFree();
}

int digitalShowDebug(char* value){
	//������� �� ����� ���� �������� �� ����� ��������
	static char prev[10];
	char s[10];
	memset(s, 0, sizeof(s));
	strlcpy(s, value, sizeof(s));
	if (strncmp(s, prev, sizeof(s))){
		memset(prev, 0, sizeof(s));
		strlcpy(prev, s, sizeof(s));
		epdClear(UNCOLORED);//�������� paint ��� ���������
		epdDrawStringAt(0, 0, s, &FontStreched72, COLORED);
		epdSetPatternMemory(Paint.image, 0, sCheckBatPos, Paint.width, FontStreched72.Height+3);
		epdDisplayPattern();	//out display
	}
	return 0;
}

void displayShow(void){
	char s[100];
	
	if (disp_state & (1<<DISP_NEED_REFRESH_BIT)) {				//���� ������� ���������
		SCREEN_POWER_ON();
		//����� �������
		epdClear(UNCOLORED);
		batShow(0);//����� "�������" ��� ���� �� ������, ������� � ����� ��������� �� 0
		epdSetPatternMemory(Paint.image, powerPosX, powerPosY, Paint.width, FontStreched72.Height+3);
			
		//����� ��������
		uint32_t tmp;
#ifdef DEBUG
		tmp = countWater;
#elif
		tmp = countWater/100;
#endif
		if (tmp > COUNT_SHOW_MAX){
			tmp = COUNT_SHOW_MAX;
			epdClear(UNCOLORED);
			memset(s, 0, sizeof(s));
			strlcpy(s, "������������", sizeof(s));
			epdDrawStringAt(0, 0, s, &FontStreched72, COLORED);
			epdSetPatternMemory(Paint.image, 0, sCheckBatPos, Paint.width, FontStreched72.Height+3);
		}
		epdClear(UNCOLORED);
		memset(s, 0, sizeof(s));
		epdDrawStringAt(0, 0, itoa(tmp, s, 10), &Fontfont39pixel_h_digit, COLORED);
		epdSetPatternMemory(Paint.image, 0, countPos, Paint.width, Fontfont39pixel_h_digit.Height+5);
		//���� ��������� ������� ��� ������ �������
		epdClear(UNCOLORED);
#ifdef DEBUG
		time_t tCount = time(NULL);
		dtShow(&tCount, 0);
#else
		switch (alarmSensor) {
			case SENSOR_ALARM_BREAK:
				epdDrawStringAt(0,0,"�����", &FontStreched72, COLORED);
				break;
			case SENSOR_ALARM_SHOT_CIR:
				epdDrawStringAt(0,0,"���������", &FontStreched72, COLORED);
				break;
			case SENSOR_ALARM_NO:
				//���� ��������� �������
				dtShow(&dtCheckPower, 0);
				break;					
			default:
				break;
		}
#endif
		epdSetPatternMemory(Paint.image, 0, dtCheckPos, Paint.width, FontStreched72.Height+3);

		//������ wifi
		epdClear(UNCOLORED);
		switch (wifi_state){
			case WIFI_STATE_OK:
				epdDrawStringAt(0,0,"��������", &FontStreched72, COLORED);
				break;
			case WIFI_PARAM_NO:
				epdDrawStringAt(0,0,"��� �����.wifi", &FontStreched72, COLORED);
				break;			
			case CAYENN_PARAM_NO:						//��� ���������� ����������� � cayenn
				epdDrawStringAt(0,0,"��� �����.������", &FontStreched72, COLORED);
				break;
			case CAYENN_CONNECT_NO:
				epdDrawStringAt(0,0,"������ ����.", &FontStreched72, COLORED);
				break;
			case WIFI_FAULT:
				epdDrawStringAt(0,0,"wifi fault", &FontStreched72, COLORED);
				break;
			default:
				break;
		}
		epdSetPatternMemory(Paint.image, 0, sSendPos, Paint.width, FontStreched72.Height+3);
		
		//���� ��������
		epdClear(UNCOLORED);
		dtShow(&dtSend, 0);
		epdSetPatternMemory(Paint.image, 0, dtSendPos, Paint.width, FontStreched72.Height+3);

		epdDisplayPattern();	//out display
	}
/*	!if (!IsRun){ TODO: ��� ���� ��������� ������
		SCREEN_POWER_OFF();
		PWR_TIMER_ONLY();
		set_sleep_mode(SLEEP_MODE_PWR_SAVE);
		sleep_enable();
		sleep_cpu();
	}*/
}

#ifdef DEBUG
inline void stdoutT(char* str, uint8_t value){
	if (strlen(str)<20){
		while(*str){
			FIFO_PUSH(debugBuf, *str);
			str++;
		}
	}
	char tmp[5];
	itoa(value, tmp, 16);
	FIFO_PUSH(debugBuf, '=');
	uint8_t i=0;
	while(tmp[i]){
		FIFO_PUSH(debugBuf, tmp[i]);
		i++;
	}
	FIFO_PUSH(debugBuf, 0x0d);
	FIFO_PUSH(debugBuf, 0x0a);
}
#endif
