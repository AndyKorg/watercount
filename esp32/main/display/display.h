/*
 * display.h
 *
 * Created: 20.07.2019 10:35:27
 *  Author: Administrator
 */ 


#ifndef DISPLAY_H_
#define DISPLAY_H_

#define STRING_MARGIN_HIGHT			5				//Отступ
#define STRING_MARGIN_LOW			3
#define	FONT_BIG
#define FONT_LOW

#define NULL_DATE					0				//Неустановленное время
#define NULL_DATE_STR				"неизвестно"	//Выводится вместо 11.11.1111

#define	COUNT_SHOW_MAX				99999			//Максимальное отображаемое число на дисплее для большого шрифта

void eInkInit(void);
void displayShow(void);
void DispleyNeedRefresh(void);						//Требуется обновить дисплей
uint8_t DisplayRefreshProcess();					//Идет обновление

int digitalShowDebug(char* value);
#ifdef DEBUG
void stdoutT(char* str, uint8_t value);
#endif


#endif /* DISPLAY_H_ */