/*
 * utils.c
 *
 *  Created on: 22 июн. 2019 г.
 *      Author: Administrator
 */

#include  "utils.h"
#include "esp_log.h"

char * cmpcpystr(char *pstr, char a, char b, char *redy, uint16_t len_redy, uint32_t maxlen){

#define	IS_END_STR()	((c < ' ') || (maxlen-- <=0)) //Строка кончилась

  if(len_redy == 0)
    redy = NULL;
  if(pstr == NULL) {
    if(redy != NULL)
      *redy='\0';
    return NULL;
  };
  char c;
  //Поиск старта
  do {
    c = *pstr;
    if (maxlen-- <=0){ // текст кончился
      if(redy != NULL)
	*redy='\0';
      return NULL; // id не найден
    };
    if((a == '\0')&&(c > ' '))
      break; // не задан -> любой символ
    pstr++;
    if(c == a) break; // нашли стартовый символ (некопируемый в буфер)
  }while(1);
  //Поиск терминатора
  if(redy != NULL){
    while(len_redy--) {
      c = *pstr;
      if(c == b) { // нашли терминирующий символ (некопируемый в буфер)
	*redy='\0';
        return pstr; // конечный терминатор найден
      };
      if IS_END_STR() { // строка кончилась или пробел
	*redy='\0';
        return NULL; // конечный терминатор не найден
      };
      pstr++;
      *redy++ = c;
    };
    *--redy='\0'; // закрыть буфер
  };
  //Скользим до терминатора или конца строки
  do {
    c = *pstr;
    if(c == b) return pstr; // нашли терминирующий символ
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
