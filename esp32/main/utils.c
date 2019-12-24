/*
 * utils.c
 *
 *  Created on: 22 ���. 2019 �.
 *      Author: Administrator
 */

#include  "utils.h"
#include "esp_log.h"

char * cmpcpystr(char *pstr, char start, char termintor, char *ready, uint16_t len_ready, uint32_t maxlenReady){

#define	IS_END_STR()	((c < ' ') || (maxlenReady-- <=0)) //������ ���������

	if(pstr == NULL) {
		if(ready != NULL)
			*ready='\0';
		return NULL;
	};
	char c;
	//����� ������
	do {
		c = *pstr;
		if (maxlenReady-- <=0){ // ����� ��������
			if(ready != NULL)
				*ready='\0';
		return NULL; // id �� ������
		}
		if((start == '\0')&&(c > ' '))
			break; // �� ����� -> ����� ������
		pstr++;
		if(c == start) break; // ����� ��������� ������ (������������ � �����)
	}while(1);
	//����� �����������
	if(ready != NULL){
		while(len_ready--) {
			c = *pstr;
			if(c == termintor) { // ����� ������������� ������ (������������ � �����)
				*ready='\0';
			return pstr; // �������� ���������� ������
			}
			if IS_END_STR() { // ������ ��������� ��� ������
				*ready='\0';
				return NULL; // �������� ���������� �� ������
			}
			pstr++;
			*ready++ = c;
		}
		*--ready='\0'; // ������� �����
	}
	//�������� �� ����������� ��� ����� ������
	do {
		c = *pstr;
		if(c == termintor) return pstr; // ����� ������������� ������
		if IS_END_STR() return NULL; // ������ ���������
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
