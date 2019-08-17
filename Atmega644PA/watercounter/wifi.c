/*
 * wifi.c
 *
 * Created: 21.07.2019 10:48:56
 *  Author: Administrator
 * Обслуживание чипа wifi
 */ 

#include "wifi.h"
#include "adc_diver.h"

// Command
#define	COMMAND_START_ST			0x8A	//Начать подключение к WiFi и cayenn и передать в облако значение счетчика воды
#define COMMAND_START_ST_DATA_LEN	4
#define	COMMAND_START_AP			0x8B	//Включить AP
#define	COMMAND_WHITE_AP			0x8C	//Ожидание подключения клиентов AP, чтение количества уже подключенных, чтение событий
#define COMMAND_WHITE_AP_DATA_LEN	4		//Передается записанное занчение счетчика воды. Значение действительно только если флаг ответа записи взведен
#define	COMMAND_WHITE_ST			0x8D	//Чтение результата подключения к wifi и передачи данных в облако
#define	COMMAND_STOP_WIFI			0x8E	//Разорвать все соединения и выключить wifi

typedef enum {
	COMMAND_SEND_READY = 0,					//Команда готова к отправке
	COMMAND_SENDING,						//Команда отправляется
	COMMAND_SENDED,							//Команда отправлена и получен ответ
	COMMAND_EMPTY							//Команды еще не отправлялись в esp
} command_esp_state_t;

typedef struct {
	uint8_t command;						//Отрабатываемая команда
	command_esp_state_t state;
	uint16_t timeout_sec;			//счетчик таймаута в секундах, 0 - таймаут истек или не запущен
	uint8_t	attemt;					//Количество попыток выполнения команды
} command_esp_t;

volatile command_esp_t command_esp;			//Команда для esp и состояние обработки

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
#define ANSWER_AP_CLIENT_NUM		1		//Номер младшего бита счтчика
#define ANSWER_AP_FLAG_MASK			0b11110000	//Флаги непрочитанных действий клиентов
#define ANSWER_AP_FLAG_WATCNT_SAVE	0b00100000	//Изменен счетчик воды

time_t	dtSend = NULL_DATE;					//Время последней передачи
wifi_state_t wifi_state;					//Состояние канала wifi

//событие готовности esp из прерывания
ISR(WIFI_ISR){
	static uint8_t pin_hystory = 0xff;
	uint8_t pin = WIFI_READY_PORT_IN;
	uint8_t changepin = pin ^ pin_hystory;
	pin_hystory = pin;
	if (changepin & (1<<WIFI_READY)){	//Изменился нужный pin?
		if ((pin_hystory & (1<<WIFI_READY)) == 0){	//esp закончил операцию
			if (command_esp.state == COMMAND_SENDING){	//Какой-то ложный сигнал, т.к. команда еще не вся ушла
				return;
			}
			stdoutT("I", 0);
			command_esp.timeout_sec = 0; //есть ответ, останавливаем отсчет
			if (command_esp.state == COMMAND_EMPTY){	//команды еще не отправлялись, надо отправить команду на esp
				//попытки тут не сбрасываем! Они считаются от старта
				if WIFI_MODE_IS_STA{
					command_esp.command = COMMAND_START_ST;
					FIFO_PUSH(sendBuf, command_esp.command);		//Команда
					FIFO_PUSH(sendBuf, 4);							//4 байта счетчика
					FIFO_PUSH(sendBuf, (uint8_t)(countWater >> 24));
					FIFO_PUSH(sendBuf, (uint8_t)(countWater >> 16));
					FIFO_PUSH(sendBuf, (uint8_t)(countWater >> 8));
					FIFO_PUSH(sendBuf, (uint8_t)(countWater));
				}
				else {
					command_esp.command = COMMAND_START_AP;
					FIFO_PUSH(sendBuf, command_esp.command);		//Команда
					FIFO_PUSH(sendBuf, 0);							//Забираем ответ
				}
				command_esp.state = COMMAND_SEND_READY;				//Можно отправлять команду на esp
				return;
			}
			//Определяем какая команда была завершена
			switch (command_esp.command){
				case COMMAND_START_AP: //Возможно Кто-то подключился к AP или что-то сохранил
					command_esp.command = COMMAND_WHITE_AP;		//Спрашиваем кто
					command_esp.attemt = 0;
					FIFO_PUSH(sendBuf, command_esp.command);
					FIFO_PUSH(sendBuf, 4);//В ответ ждем 4 байта
					FIFO_PUSH(sendBuf, 0);//Ответ значением счетчика воды от клиента 1-й байт
					FIFO_PUSH(sendBuf, 0);//2
					FIFO_PUSH(sendBuf, 0);//3
					FIFO_PUSH(sendBuf, 0);//4
					command_esp.state = COMMAND_SEND_READY;
					break;
				case COMMAND_START_ST:	//Что-то завершено с передачей в облако
					command_esp.command = COMMAND_WHITE_ST;	//Узнаем что
					FIFO_PUSH(sendBuf, command_esp.command);
					FIFO_PUSH(sendBuf, 0);//в ответ ждем только ответ
					command_esp.state = COMMAND_SEND_READY;
					break;
				case COMMAND_WHITE_ST:	//Результат предачи порчитан и он хороший, останавливаем wifi
					command_esp.command = COMMAND_STOP_WIFI;
					FIFO_PUSH(sendBuf, command_esp.command);
					FIFO_PUSH(sendBuf, 0);//в ответ ждем только ответ
					command_esp.state = COMMAND_SEND_READY;
					break;
				default:
					break;
			}
		}
	}
}

//Команда отправлена в esp и получен какой-то ответ - вызов из прерывания таймера spi!
inline uint8_t esp_send_cb(void){
	uint8_t answer;
	uint32_t watercnt;
	
	SPI_CS_OFF();
	
	command_esp.state = COMMAND_SENDED;
	
	dtSend = time(NULL);
	answer = ANSWER_FAIL;
	if (!FIFO_IS_EMPTY(recivBuf)){
		if (FIFO_COUNT(recivBuf) > 1){
			FIFO_POP(recivBuf);//Вытягиваем мусорный байт
			answer = FIFO_FRONT(recivBuf);
			//stdoutT("garb", answer);
		}
		answer = FIFO_FRONT(recivBuf);
		FIFO_POP(recivBuf);
	}
	//stdoutT("cmd", command_esp.command);
	stdoutT("good", answer);
	switch (command_esp.command){
		case COMMAND_START_AP:
			if (answer == ANSWER_OK){	//Запуск AP успешный
			}
			break;
		case COMMAND_WHITE_AP:			//Результат запуска AP
			if (FIFO_COUNT(recivBuf) == 4){ //Команда правильная
				watercnt = ((uint32_t)FIFO_FRONT(recivBuf)) << 24;
				FIFO_POP(recivBuf);
				watercnt = (watercnt & 0xFF000000) | ((uint32_t)FIFO_FRONT(recivBuf)) << 16;
				FIFO_POP(recivBuf);
				watercnt = (watercnt & 0xFFFF0000) | ((uint32_t)FIFO_FRONT(recivBuf)) << 8;
				FIFO_POP(recivBuf);
				watercnt = (watercnt & 0xFFFFFF00) | ((uint32_t)FIFO_FRONT(recivBuf));
				FIFO_POP(recivBuf);
				if (answer & ANSWER_AP_STATUS_MASK){ //AP Запустился
					command_esp.attemt = COMMAND_ATTEMPT_MAX+1;	//Считаем что все попытки исчерпаны что бы выключится окончательно после первоначальной настройки
					command_esp.timeout_sec = COMMAND_TIMEOUT_WHITE_CLIENT;	//Ждем подключения клиентов и результатов их работы
					if (answer & ANSWER_AP_CLIENT_MASK){	//Есть подключенные клиенты
						if ((answer & ANSWER_AP_FLAG_MASK) ==  ANSWER_AP_FLAG_WATCNT_SAVE){ //Была запись счетчика
							countWater = watercnt;
						}
					}
				}
			}
			break;
		case COMMAND_START_ST:		//Была команда запуска ST и отправки данных в облако
			if (FIFO_COUNT(recivBuf) == 4){ //Команда правильная
				switch (answer){
					case ANSWER_OK:
						wifi_state = WIFI_STATE_OK;
						command_esp.attemt = 0;
						break;
					case ANSWER_NO_WIFI_PARAM:
						wifi_state = WIFI_PARAM_NO;
						break;
					case ANSWER_NO_CAYENN_PARAM:
						wifi_state = CAYENN_PARAM_NO;
						break;
					default:
						command_esp.state = COMMAND_EMPTY;
						WIFI_POWER_OFF();
						break;
				}
			}
			break;
		case COMMAND_WHITE_ST:
			switch (answer){
				case ANSWER_OK:
					wifi_state = WIFI_STATE_OK;
					command_esp.attemt = 0;
					break;
				case ANSWER_ERR_ACS_D:
					wifi_state = CAYENN_CONNECT_NO;
					break;
				case ANSWER_PROCESS:
					command_esp.timeout_sec = COMMAND_TIMEOUT_SEC; //Процесс идет еще подождать
					break;
				default:									//Какая-то проблема, перегружаем esp и отправляем передачу еще раз если есть попытки
					command_esp.state = COMMAND_EMPTY;
					WIFI_POWER_OFF();
					break;
			}
		case COMMAND_STOP_WIFI:
			switch (answer){
				case ANSWER_OK:
					wifi_state = WIFI_STATE_OK;
					command_esp.state = COMMAND_EMPTY;
					command_esp.attemt = 0;
					WIFI_POWER_OFF();						//Цикл передачи закончен
					break;
				default:									//Какая-то проблема, перегружаем esp и отправляем передачу еще раз если есть попытки
					command_esp.state = COMMAND_EMPTY;
					WIFI_POWER_OFF();
					break;
			}
		default:
			break;
	}
	FIFO_FLUSH(recivBuf);//Все остальное отбросить
	return ERR_OK;
}

//Сбросить команду, что бы формировать ёе перезапуск
inline void command_reset(void){
	command_esp.state = COMMAND_EMPTY;
	command_esp.timeout_sec = COMMAND_TIMEOUT_SEC;
}

//Проверить время прошедшее с прошлой команды и выключить и при истечении
inline uint8_t command_timeout_check_and_off(void){
	if (command_esp.timeout_sec) {
		command_esp.timeout_sec--;
		if (command_esp.timeout_sec == 0){						//Таймаут ожидания ответа от esp истек
			command_esp.attemt++;
			WIFI_POWER_OFF();
			if (command_esp.attemt <= COMMAND_ATTEMPT_MAX){	//Попытки еще есть?
				return WIFI_NEED_ON; //Надо включить wifi через секунду
			}
		}
	}
	return WIFI_NOT_NEED_ON;
}

//Отправить команду на esp если она есть в буфере
inline void wifi_go(void){
	if (command_esp.state == COMMAND_SEND_READY){	//Команда готовка к отправке в esp
		if (!FIFO_IS_EMPTY(sendBuf)){				//Команда в буфере
			if (WIFI_IS_READY()){
				command_esp.state = COMMAND_SENDING;
				command_esp.timeout_sec = COMMAND_TIMEOUT_SEC;	//Попробывать еще раз
				SPI_CS_SELECT();					//go!
				spi_send_start(NULL);
				dtSend = time(NULL);
			}
		}
	}
}


#ifdef DEBUG
uint16_t timout(void){		//Оставшееся время timout
	return command_esp.timeout_sec;
}
#endif

void wifi_init(void){
	//Старт wifi
	command_esp.state = COMMAND_EMPTY;
	command_esp.attemt = 0;
	stdoutT("start to", 0);
	WIFI_INIT_READY();
	WIFI_MODE_INI();
	WIFI_POWER_ON();
	wifi_state = WIFI_FAULT;
	command_esp.timeout_sec = COMMAND_TIMEOUT_SEC;//Начинаем отсчет таймаута
}
