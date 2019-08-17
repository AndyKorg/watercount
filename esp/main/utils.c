/*
 * utils.c
 *
 *  Created on: 22 ���. 2019 �.
 *      Author: Administrator
 */

#include  "utils.h"
#include "esp_log.h"

char * cmpcpystr(char *pstr, char a, char b, char *redy, uint16_t len_redy, uint32_t maxlen){

#define	IS_END_STR()	((c < ' ') || (maxlen-- <=0)) //������ ���������

  if(len_redy == 0)
    redy = NULL;
  if(pstr == NULL) {
    if(redy != NULL)
      *redy='\0';
    return NULL;
  };
  char c;
  //����� ������
  do {
    c = *pstr;
    if (maxlen-- <=0){ // ����� ��������
      if(redy != NULL)
	*redy='\0';
      return NULL; // id �� ������
    };
    if((a == '\0')&&(c > ' '))
      break; // �� ����� -> ����� ������
    pstr++;
    if(c == a) break; // ����� ��������� ������ (������������ � �����)
  }while(1);
  //����� �����������
  if(redy != NULL){
    while(len_redy--) {
      c = *pstr;
      if(c == b) { // ����� ������������� ������ (������������ � �����)
	*redy='\0';
        return pstr; // �������� ���������� ������
      };
      if IS_END_STR() { // ������ ��������� ��� ������
	*redy='\0';
        return NULL; // �������� ���������� �� ������
      };
      pstr++;
      *redy++ = c;
    };
    *--redy='\0'; // ������� �����
  };
  //�������� �� ����������� ��� ����� ������
  do {
    c = *pstr;
    if(c == b) return pstr; // ����� ������������� ������
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
