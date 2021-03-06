/*
 * utils.h
 */

#ifndef MAIN_UTILS_H_
#define MAIN_UTILS_H_

#include <stdint.h>
#include <stdlib.h>
#include <string.h>

/**
 * Description  : �������� ����� � ready �� ������ ������ pstr � ��������� ��������� ��������
 *                � �������� ������������. ���������� � ��������� ������ �� ��������, ���� ������.
 * Parameters   : ��� ������� ���������� ������� = '\0' ������� ����� ������ (>' ').
                  �������� �� ������� <' ' ��� �����������.
                  �������� ����������� ������� ������ ��� ����������� ����� (� ������������ � ����� '\0'!).
 * Returns      : ��������� �� ���������� � ������, ���� ���������� ������.
                  NULL, ���� ��������� ��� �������� ���������� �� ������.
**/
char * cmpcpystr(char *pstr, char start, char termintor, char *ready, uint16_t len_ready, uint32_t maxlenReady);

/*
 * �������� ������. ���� ������ ������ ��������� ������ ��� ������ ���������, �� ������������������ ������
 * ���������� ���������� ���������� ���� ��� 0 ���� ������
 */
size_t strset(char* to, char* from);

#endif /* MAIN_UTILS_H_ */

void twoDigit(int value, char *str, size_t size);
uint8_t DecToBCD(uint8_t value);
