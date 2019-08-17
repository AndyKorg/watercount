/*
 * "Файловая система"
 * Простой массив файлов. Не поддерживает запись, только чтение
 * Загрузка файлов только целиком всем "диском" с помомщью штатного загрузчика
 */

#ifndef MAIN_MY_FS_H_
#define MAIN_MY_FS_H_

#include "esp_err.h"

#define MAX_FILES_COUNT		20	//Максимальное количество файлов
#define MAX_LEN_FILE_NAME	32 	//Кратно 4 с учетом завершающего '/0'
//Запись о файле
typedef struct {
  char FileName[MAX_LEN_FILE_NAME];	//Имя
  uint32_t fileOffset;			//Адрес начала файла во flash
  uint32_t fileSize;			//Размер файла
} files_t;

typedef struct {
  files_t files[MAX_FILES_COUNT];
  uint8_t count;
}file_system_t;

file_system_t* myfsInit(void);
char* read_file(file_system_t* fs, const char *FileName, size_t* size, esp_err_t* err);

#endif /* MAIN_MY_FS_H_ */
