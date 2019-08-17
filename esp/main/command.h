/*
 * command.h
 *
 *  Created on: 2 июн. 2019 г.
 *      Author: Administrator
 */

#ifndef MAIN_COMMAND_H_
#define MAIN_COMMAND_H_

#include "driver/gpio.h"
#include "esp_err.h"

// Command
#define	COMMAND_START_ST_WIFI		0x8A	//Начать подключение к WiFi и cayenn и передать в облако значение счетчика воды
#define COMMAND_START_ST_WIFI_DATA_LEN	4
#define	COMMAND_START_AP			0x8B	//Включить AP
#define	COMMAND_WHITE_AP			0x8C	//Ожидание подключения клиентов AP, чтение количества уже подключенных, чтение событий
#define COMMAND_WHITE_AP_DATA_LEN	4		//Передается записанное занчение счетчика воды. Значение действительно только если флаг ответа записи взведен
#define	COMMAND_WHITE_ST			0x8D	//Чтение результата подключения к wifi и передачи данных в облако
#define	COMMAND_STOP_WIFI			0x8E	//Разорвать все соединения и выключить wifi

//Answer
#define ANSWER_OK					0
#define ANSWER_NO_WIFI_PARAM  		0x01	//нет праметров Wifi
#define ANSWER_NO_CAYENN_PARAM  	0x02	//нет парамтеров подключения к cayenn
#define ANSWER_ERR_ACS_D			0x03	//не удалось подключится к брокеру
#define	ANSWER_PROCESS				0x04	//Соединение есть, но процесс еще не закончен, добавил т.к. какой-то сигнал готовности проскакивает непонятный
#define ANSWER_FAIL					0xff	//Неправильный формат команды или неизвестная команда
#define	ANSWER_FIRST				0xaa	//Первый ответ на команду
//answer for COMMAND_WHITE_AP
#define ANSWER_AP_STATUS_MASK		0b00000001	//Бит статуса AP
#define ANSWER_AP_STATUS_NUM		0
#define ANSWER_AP_CLIENT_MASK		0b00001110	//Маска счетчика подключенных клиентов
#define ANSWER_AP_CLIENT_NUM		1			//Номер младшего бита счтчика
#define ANSWER_AP_FLAG_MASK			0b11110000	//Флаги непрочитанных действий клиентов
#define ANSWER_AP_FLAG_WATCNT_SAVE	0b00100000	//Изменен счетчик воды

//Hardware confirm
#define CHIP_READY_PIN	GPIO_Pin_5
#define CHIP_READY_NUM	GPIO_NUM_5
#define CHIP_BUSY()		do{gpio_set_level(CHIP_READY_NUM, 1);}while(0)
#define CHIP_READY() 	do{gpio_set_level(CHIP_READY_NUM, 0);}while(0)

//Старт-стоп приема команды
esp_err_t spi_start_command(uint8_t* send_byte);
esp_err_t spi_stop_command();

//принять байт команды от SPI
esp_err_t spi_recive_command(uint8_t recive_byte);
//Отдать байт ответа
esp_err_t spi_answer(uint8_t* send_byte);

void cmd_init(void);

#endif /* MAIN_COMMAND_H_ */
