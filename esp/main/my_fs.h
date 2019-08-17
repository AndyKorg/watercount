/*
 * "�������� �������"
 * ������� ������ ������. �� ������������ ������, ������ ������
 * �������� ������ ������ ������� ���� "������" � �������� �������� ����������
 */

#ifndef MAIN_MY_FS_H_
#define MAIN_MY_FS_H_

#include "esp_err.h"

#define MAX_FILES_COUNT		20	//������������ ���������� ������
#define MAX_LEN_FILE_NAME	32 	//������ 4 � ������ ������������ '/0'
//������ � �����
typedef struct {
  char FileName[MAX_LEN_FILE_NAME];	//���
  uint32_t fileOffset;			//����� ������ ����� �� flash
  uint32_t fileSize;			//������ �����
} files_t;

typedef struct {
  files_t files[MAX_FILES_COUNT];
  uint8_t count;
}file_system_t;

file_system_t* myfsInit(void);
char* read_file(file_system_t* fs, const char *FileName, size_t* size, esp_err_t* err);

#endif /* MAIN_MY_FS_H_ */
