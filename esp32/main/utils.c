/*
 * utils.c
 *
 *  Created on: 22 июн. 2019 г.
 *      Author: Administrator
 */

#include  "utils.h"
#include "esp_log.h"

char * cmpcpystr(char *pstr, char start, char termintor, char *ready, uint16_t len_ready, uint32_t maxlenReady){

#define	IS_END_STR()	((c < ' ') || (maxlenReady-- <=0)) //Строка кончилась

	if(pstr == NULL) {
		if(ready != NULL)
			*ready='\0';
		return NULL;
	};
	char c;
	//Поиск старта
	do {
		c = *pstr;
		if (maxlenReady-- <=0){ // текст кончился
			if(ready != NULL)
				*ready='\0';
		return NULL; // id не найден
		}
		if((start == '\0')&&(c > ' '))
			break; // не задан -> любой символ
		pstr++;
		if(c == start) break; // нашли стартовый символ (некопируемый в буфер)
	}while(1);
	//Поиск терминатора
	if(ready != NULL){
		while(len_ready--) {
			c = *pstr;
			if(c == termintor) { // нашли терминирующий символ (некопируемый в буфер)
				*ready='\0';
			return pstr; // конечный терминатор найден
			}
			if IS_END_STR() { // строка кончилась или пробел
				*ready='\0';
				return NULL; // конечный терминатор не найден
			}
			pstr++;
			*ready++ = c;
		}
		*--ready='\0'; // закрыть буфер
	}
	//Скользим до терминатора или конца строки
	do {
		c = *pstr;
		if(c == termintor) return pstr; // нашли терминирующий символ
		if IS_END_STR() return NULL; // строка кончилась
		pstr++;
	}while(1);
}

size_t strset(char* to, char* from){
  size_t sizeTo = 0;

  if (to)
    sizeTo = strlen(to);
  if (sizeTo != strlen(from)){
    if (to)
      free(to);
    sizeTo = 0;
    to = calloc(strlen(from), sizeof(char));
    if (to)
      sizeTo = strlen(from);
  }
  if (sizeTo)
    strcpy(to, from);
  return sizeTo;
}

void twoDigit(int value, char *str, size_t size) {
	char cnv[10];
	if (value < 10) {
		strlcat(str, "0", size);
	}
	strlcat(str, itoa(value, cnv, 10), size);
}

