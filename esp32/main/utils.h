/*
 * utils.h
 */

#ifndef MAIN_UTILS_H_
#define MAIN_UTILS_H_

#include <stdint.h>
#include <stdlib.h>
#include <string.h>

/**
 * Description  : Копирует слово в ready из строки текста pstr с заданными начальным символом
 *                и конечным терминатором. Терминатор и стартовый символ не копирует, если заданы.
 * Parameters   : При задании начального символа = '\0' берется любой символ (>' ').
                  Копирует до символа <' ' или терминатора.
                  Задается ограничение размера буфера для копируемого слова (с дописыванием в буфер '\0'!).
 * Returns      : Указывает на терминатор в строке, если терминатор найден.
                  NULL, если начальный или конечный терминатор не найден.
**/
char * cmpcpystr(char *pstr, char start, char termintor, char *ready, uint16_t len_ready, uint32_t maxlenReady);

/*
 * Копирует строку. Если размер строки приемника меньше или больше источника, то перезахыватывается память
 * Возвращает количество захваченых байт или 0 если ошибка
 */
size_t strset(char* to, char* from);

#endif /* MAIN_UTILS_H_ */

void twoDigit(int value, char *str, size_t size);
uint8_t DecToBCD(uint8_t value);
