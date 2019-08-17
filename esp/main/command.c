/*
 * command.c
 *
 *  Created on: 2 июн. 2019 г.
 *      Author: Administrator
 */

#include <string.h>
#include "sdkconfig.h"
#include "esp_err.h"
#include "command.h"
#include "wifi.h"
#include "esp_log.h"
#include "cayenne.h"
#include "params.h"

static uint8_t answerByte, countByte;

static const char *TAG = "CMD";

#include "portmacro.h"
#include "projdefs.h"
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "freertos/task.h"
#include "freertos/semphr.h"

typedef struct{
  uint8_t buf;
  uint8_t code;
} deb_t;

xQueueHandle qCommand, qDebug;
SemaphoreHandle_t reciveEnd;			//Прием закончен ногой CS

#define CMD_DATA_LEN_MAX 	8 			//Максимальная длина данных в команде

struct Command_t{
  uint8_t Command;
  uint8_t LenData;
  uint8_t DataReciv[CMD_DATA_LEN_MAX];	//Приятые данные
  uint8_t DataSend[CMD_DATA_LEN_MAX];	//Данные для передачи
};


//Interrapt!
//Начало передачи на spi - cs активирован, нужен первый байт для ответа
esp_err_t spi_start_command(uint8_t* send_byte){
  countByte = 0;
  answerByte = ANSWER_FIRST;
  *send_byte = answerByte;
  return ESP_OK;
}

//Interrapt!
//Конец передачи на spi - cs деактивирован
esp_err_t spi_stop_command(void){
	BaseType_t xHigherPriorityTaskWoken = pdFALSE;
	xSemaphoreGiveFromISR(reciveEnd, &xHigherPriorityTaskWoken);
	return ESP_OK;
}

//Interrapt!
//Принят байт
esp_err_t spi_recive_command(uint8_t recive_byte){

  static struct Command_t command;
  BaseType_t xHigherPriorityTaskWoken = pdFALSE;

  deb_t debu;

  debu.code = countByte;
  debu.buf = recive_byte;
  xQueueSendToBackFromISR(qDebug, &debu, &xHigherPriorityTaskWoken);
  //Команда и ответ esp
  if (countByte == 0){
    command.Command = recive_byte;
    command.LenData = CMD_DATA_LEN_MAX+1;//Длина данных больше максимальныой, чтобы не срабатывало сравнение при проверке окончания пакета
    switch (recive_byte){
      case COMMAND_START_ST_WIFI:
		answerByte = ANSWER_OK;
		if (*wifi_sta_param.ssid == 0){
		  answerByte = ANSWER_NO_WIFI_PARAM;
		}
		else if (*cayenn_cfg.host == 0){
		  answerByte = ANSWER_NO_CAYENN_PARAM;
		}
		break;
      case COMMAND_START_AP:
		answerByte = ANSWER_OK;
		break;
      case COMMAND_WHITE_AP:
		answerByte = (wifi_isOn() << ANSWER_AP_STATUS_NUM);
		answerByte = (answerByte & ~ANSWER_AP_CLIENT_MASK) | (wifi_ap_count_client() << ANSWER_AP_CLIENT_NUM);
		answerByte = (answerByte & ~ANSWER_AP_FLAG_MASK) | (watercount.state == NO_READ_SPI?ANSWER_AP_FLAG_WATCNT_SAVE:0);
		break;
      case COMMAND_WHITE_ST:
		answerByte = ANSWER_ERR_ACS_D;
		if (wifi_isOn() && !wifi_AP_isOn()){
			answerByte = ANSWER_PROCESS;
			if (cayenn_cfg.LastState == ESP_OK){//Все таки все отправилось
				answerByte = ANSWER_OK;
			}
		}
		break;
      case COMMAND_STOP_WIFI:
    	if (Cayenne_app_stop() == ESP_OK){
    		answerByte = ANSWER_OK;
    	}
    	else {
    		answerByte = ANSWER_FAIL;
    	}
    	break;
      default:
		answerByte = ANSWER_FAIL;
		break;
    }
    debu.code = 0xff;
    debu.buf = answerByte;
    xQueueSendToBackFromISR(qDebug, &debu, &xHigherPriorityTaskWoken);
  }
  //Длина данных и первый байт данных esp
  else if (countByte == 1){
    if (recive_byte <= CMD_DATA_LEN_MAX){
    	command.LenData = recive_byte;
    	if (command.Command == COMMAND_WHITE_AP){
			command.DataSend[0] = watercount.count >> 24;
			command.DataSend[1] = watercount.count >> 16;
			command.DataSend[2] = watercount.count >> 8;
			command.DataSend[3] = watercount.count;
			answerByte = command.DataSend[0];
    	}
		else{
			answerByte = command.LenData;
		}
    }
    else{
    	answerByte = ANSWER_FAIL;
    }
  }
  else{	//Данные
    if (command.LenData){
      command.DataReciv[countByte-2] = recive_byte;
      answerByte = command.DataSend[countByte - 2];
    }
  }
  if (countByte == (command.LenData+1)){	//Команда принята
	  xQueueSendFromISR(qCommand, &command, &xHigherPriorityTaskWoken);	//TODO: если очередь будет полна, то сбрасывать готовность и ждать особождения очереди
	  if (command.Command == COMMAND_WHITE_AP){
		  watercount.state = READ_SPI;
	  }
	  countByte = 0;
  }
  else{
	  countByte++;
  }
  return ESP_OK;
}

esp_err_t spi_answer(uint8_t* send_byte){
  *send_byte = answerByte;
  return ESP_OK;
}

//выполнение принятой команды
static void command_task(void *pvParameters){
  struct Command_t cmd;

  while(1){
	xSemaphoreTake(reciveEnd, portMAX_DELAY);
    if (xQueueReceive(qCommand, &cmd, portMAX_DELAY) == pdPASS){
    	ESP_LOGE(TAG, "cmd = %02x %02x %02x %02x %02x %02x", cmd.Command, cmd.LenData, cmd.DataReciv[0], cmd.DataReciv[1], cmd.DataReciv[2], cmd.DataReciv[3]);
    	CHIP_BUSY();
    	switch (cmd.Command){
			case COMMAND_START_AP:
				wifi_init(WIFI_MODE_AP);
				break;
			case COMMAND_START_ST_WIFI:
				watercount.count = ((uint32_t)cmd.DataReciv[0]<<24) + ((uint32_t)cmd.DataReciv[1]<<16) + ((uint32_t)cmd.DataReciv[2]<<8) + ((uint32_t)cmd.DataReciv[3]);
				ESP_LOGE(TAG, "wc = %04x", watercount.count);
				if (wifi_isOn() && !wifi_AP_isOn()){
					cayenn_cfg.LastState = CayenneChangeInteger(&cayenn_cfg, PARAM_CHANAL_CAYEN, PARAM_NAME_SENSOR, watercount.count);
					ESP_LOGE(TAG, "ca state = %x", cayenn_cfg.LastState);
					continue;
				}
				wifi_init(WIFI_MODE_STA);
				break;
			case COMMAND_WHITE_ST:
				CHIP_READY();
				break;
			case COMMAND_STOP_WIFI:
				CHIP_READY();
				break;
			default:
				break;
    	}
    }
  }
}

void debug_task(void *Params){
  deb_t buf;
  while(1){
    if (xQueueReceive(qDebug, &buf, portMAX_DELAY) == pdPASS){
      ESP_LOGW(TAG, "deb code = %x buf = %x", buf.code, buf.buf);
    }
  }
}

void cmd_init(void){
  gpio_config_t io_conf;
  io_conf.intr_type = GPIO_INTR_DISABLE;	//disable interrupt
  io_conf.mode = GPIO_MODE_OUTPUT;		//set as output mode
  io_conf.pin_bit_mask = CHIP_READY_PIN;	//bit mask of the pins that you want to set,e.g.GPIO15/16
  io_conf.pull_down_en = 0;			//disable pull-down mode
  io_conf.pull_up_en = 0;			//disable pull-up mode
  gpio_config(&io_conf);
  CHIP_BUSY();

  TaskHandle_t xHandle = NULL;

  answerByte = ANSWER_FIRST;
  qCommand = xQueueCreate(5, sizeof(struct Command_t)); //Очередь на 5 команд непосредственно для исполнения
  reciveEnd = xSemaphoreCreateBinary();
  if ((qCommand != 0) && (reciveEnd != NULL)){
	  xTaskCreate(command_task, "command_task", 4096, ( void * ) 1, 5, &xHandle);
  }

  qDebug = xQueueCreate(50, sizeof(deb_t));
  xTaskCreate(debug_task, "debug_task", 4096, ( void * ) 1, 5, &xHandle);
}
