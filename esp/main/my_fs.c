/*
 * html_spiffs.c
 *
 *  Created on: 14 июн. 2019 г.
 *      Author: Administrator
 */
#include "my_fs.h"

#include <stdlib.h>
#include <string.h>
#include "sdkconfig.h"
#include "esp_log.h"
#include "esp_partition.h"
#include "spi_flash.h"

#undef __ESP_FILE__
#define __ESP_FILE__	NULL

static const char *TAG = "MYFS";

/*
 * "Файловая система"
 * сначала метка файловой системы MAGIC без завершающего нуля!
 * затем байт количества файлов
 * затем 4 байта адреса с которого начинается следующий файл (OFFSET).
 * Имя файла заканчивающиеся нулевым байтом
 * Дальше содержимое файла до адреса указанного в OFFSET
 */
#define MAGIC_FS 		"MYFS1"
const esp_partition_t* partition;

#define SIZE_ADRESS_LEN		4	//Длина смещения в байтах
#define	SIZE_COUNTER_FILES	1	//Размерность счетчика файлов

//Проверяем строку метки файловой системы MAGIC
inline esp_err_t isMyFS(uint32_t* adr){
  esp_err_t ret = ESP_ERR_NOT_SUPPORTED;
  char* buf = calloc(strlen(MAGIC_FS)+1, sizeof(char));
  if (spi_flash_read(*adr, buf, strlen(MAGIC_FS)+1) == ESP_OK){	//Количество файлов в массиве
    if (strncmp(buf, MAGIC_FS, strlen(MAGIC_FS)) == 0){
      *adr += strlen(MAGIC_FS);
      ret = ESP_OK;
    }
  }
  return ret;
}

file_system_t* myfsInit(){
  partition = esp_partition_find_first(ESP_PARTITION_TYPE_DATA, ESP_PARTITION_SUBTYPE_DATA_SPIFFS, "storage");
  size_t offset;
  uint32_t buf = 1;
  uint8_t i = 0;

  file_system_t* fs = NULL;

  if (partition != NULL){
    ESP_LOGI(TAG, "start search");
    offset = partition->address;
    if (isMyFS(&offset) == ESP_OK ){					//Получить адрес счетчика файлов
      ESP_LOGI(TAG, "myfs adr = %x", offset);
      if (spi_flash_read(offset, &i, SIZE_COUNTER_FILES) == ESP_OK){	//Количество файлов в массиве
	ESP_LOGI(TAG, "countfiles = %d", i);
	if ((i>0) && (i<MAX_FILES_COUNT)){
	  fs = (file_system_t*) calloc(1, sizeof(file_system_t));
	  if (fs){
	    offset++;
	    fs->count = i;
	    for(i=0; i<fs->count; i++){
	      if (spi_flash_read(offset, &buf, SIZE_ADRESS_LEN) == ESP_OK){	//Смещение до следующего файла
		if (spi_flash_read(offset+SIZE_ADRESS_LEN, fs->files[i].FileName, MAX_LEN_FILE_NAME) == ESP_OK){//Читаем имя файла
		  fs->files[i].fileOffset = offset+SIZE_ADRESS_LEN+strlen(fs->files[i].FileName)+1;
		  fs->files[i].fileSize = buf-strlen(fs->files[i].FileName)-1;
		  ESP_LOGI(TAG, "fn = %s of = %x, sz = %x", fs->files[i].FileName, fs->files[i].fileOffset, fs->files[i].fileSize);
		}
	      }
	      offset += buf+SIZE_ADRESS_LEN;
	      ESP_LOGI(TAG, "buf = %x of = %x", buf, offset);
	    }//for
	  }
	}
      }
    }
  }
  if (fs){
    if ((i != fs->count) || (fs->count == 0)){	//Не удалось что-то прочитать или нет файлов
      free(fs);
      fs = NULL;
    }
  }
  return fs;
}

char* read_file(file_system_t* fs, const char *FileName, size_t* size, esp_err_t* err){
  *err = ESP_ERR_NOT_FOUND;
  uint8_t i = 0;
  char *content = NULL;

  if (fs){
    ESP_LOGI(TAG, "seek start count = %d", fs->count);
    while( (i < fs->count) && (i<MAX_FILES_COUNT) ){
      ESP_LOGI(TAG, "%s", fs->files[i].FileName);
      if ( !strcmp(fs->files[i].FileName, FileName)){
        ESP_LOGI(TAG, "seek ok = %d nm = %s", i, FileName);
        content = calloc(fs->files[i].fileSize+1, 1); //Байт для завершающего нуля для корректной обработки файла как строки
        if (content){
  	*err = spi_flash_read(fs->files[i].fileOffset, content, fs->files[i].fileSize);
  	*size = fs->files[i].fileSize;
  	ESP_LOGI(TAG, "read ok size = %d", *size);
  	return content;
        }
      }
      i++;
    }
  }
  return content;
}

