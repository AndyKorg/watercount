/*
 * wifi.c
 *
 * Created: 21.07.2019 10:48:56
 *  Author: Administrator
 * ������������ ���� wifi
 */ 

#include "wifi.h"
#include "adc_diver.h"

// Command
#define	COMMAND_START_ST			0x8A	//������ ����������� � WiFi � cayenn � �������� � ������ �������� �������� ����
#define COMMAND_START_ST_DATA_LEN	4
#define	COMMAND_START_AP			0x8B	//�������� AP
#define	COMMAND_WHITE_AP			0x8C	//�������� ����������� �������� AP, ������ ���������� ��� ������������, ������ �������
#define COMMAND_WHITE_AP_DATA_LEN	4		//���������� ���������� �������� �������� ����. �������� ������������� ������ ���� ���� ������ ������ �������
#define	COMMAND_WHITE_ST			0x8D	//������ ���������� ����������� � wifi � �������� ������ � ������
#define	COMMAND_STOP_WIFI			0x8E	//��������� ��� ���������� � ��������� wifi

typedef enum {
	COMMAND_SEND_READY = 0,					//������� ������ � ��������
	COMMAND_SENDING,						//������� ������������
	COMMAND_SENDED,							//������� ���������� � ������� �����
	COMMAND_EMPTY							//������� ��� �� ������������ � esp
} command_esp_state_t;

typedef struct {
	uint8_t command;						//�������������� �������
	command_esp_state_t state;
	uint16_t timeout_sec;			//������� �������� � ��������, 0 - ������� ����� ��� �� �������
	uint8_t	attemt;					//���������� ������� ���������� �������
} command_esp_t;

volatile command_esp_t command_esp;			//������� ��� esp � ��������� ���������

//Answer
#define ANSWER_OK					0
#define ANSWER_NO_WIFI_PARAM  		0x01	//��� ��������� Wifi
#define ANSWER_NO_CAYENN_PARAM  	0x02	//��� ���������� ����������� � cayenn
#define ANSWER_ERR_ACS_D			0x03	//�� ������� ����������� � �������
#define	ANSWER_PROCESS				0x04	//���������� ����, �� ������� ��� �� ��������, ������� �.�. �����-�� ������ ���������� ������������ ����������
#define ANSWER_FAIL					0xff	//������������ ������ ������� ��� ����������� �������
#define	ANSWER_FIRST				0xaa	//������ ����� �� �������
//answer for COMMAND_WHITE_AP
#define ANSWER_AP_STATUS_MASK		0b00000001	//��� ������� AP
#define ANSWER_AP_STATUS_NUM		0
#define ANSWER_AP_CLIENT_MASK		0b00001110	//����� �������� ������������ ��������
#define ANSWER_AP_CLIENT_NUM		1		//����� �������� ���� �������
#define ANSWER_AP_FLAG_MASK			0b11110000	//����� ������������� �������� ��������
#define ANSWER_AP_FLAG_WATCNT_SAVE	0b00100000	//������� ������� ����

time_t	dtSend = NULL_DATE;					//����� ��������� ��������
wifi_state_t wifi_state;					//��������� ������ wifi

//������� ���������� esp �� ����������
ISR(WIFI_ISR){
	static uint8_t pin_hystory = 0xff;
	uint8_t pin = WIFI_READY_PORT_IN;
	uint8_t changepin = pin ^ pin_hystory;
	pin_hystory = pin;
	if (changepin & (1<<WIFI_READY)){	//��������� ������ pin?
		if ((pin_hystory & (1<<WIFI_READY)) == 0){	//esp �������� ��������
			if (command_esp.state == COMMAND_SENDING){	//�����-�� ������ ������, �.�. ������� ��� �� ��� ����
				return;
			}
			stdoutT("I", 0);
			command_esp.timeout_sec = 0; //���� �����, ������������� ������
			if (command_esp.state == COMMAND_EMPTY){	//������� ��� �� ������������, ���� ��������� ������� �� esp
				//������� ��� �� ����������! ��� ��������� �� ������
				if WIFI_MODE_IS_STA{
					command_esp.command = COMMAND_START_ST;
					FIFO_PUSH(sendBuf, command_esp.command);		//�������
					FIFO_PUSH(sendBuf, 4);							//4 ����� ��������
					FIFO_PUSH(sendBuf, (uint8_t)(countWater >> 24));
					FIFO_PUSH(sendBuf, (uint8_t)(countWater >> 16));
					FIFO_PUSH(sendBuf, (uint8_t)(countWater >> 8));
					FIFO_PUSH(sendBuf, (uint8_t)(countWater));
				}
				else {
					command_esp.command = COMMAND_START_AP;
					FIFO_PUSH(sendBuf, command_esp.command);		//�������
					FIFO_PUSH(sendBuf, 0);							//�������� �����
				}
				command_esp.state = COMMAND_SEND_READY;				//����� ���������� ������� �� esp
				return;
			}
			//���������� ����� ������� ���� ���������
			switch (command_esp.command){
				case COMMAND_START_AP: //�������� ���-�� ����������� � AP ��� ���-�� ��������
					command_esp.command = COMMAND_WHITE_AP;		//���������� ���
					command_esp.attemt = 0;
					FIFO_PUSH(sendBuf, command_esp.command);
					FIFO_PUSH(sendBuf, 4);//� ����� ���� 4 �����
					FIFO_PUSH(sendBuf, 0);//����� ��������� �������� ���� �� ������� 1-� ����
					FIFO_PUSH(sendBuf, 0);//2
					FIFO_PUSH(sendBuf, 0);//3
					FIFO_PUSH(sendBuf, 0);//4
					command_esp.state = COMMAND_SEND_READY;
					break;
				case COMMAND_START_ST:	//���-�� ��������� � ��������� � ������
					command_esp.command = COMMAND_WHITE_ST;	//������ ���
					FIFO_PUSH(sendBuf, command_esp.command);
					FIFO_PUSH(sendBuf, 0);//� ����� ���� ������ �����
					command_esp.state = COMMAND_SEND_READY;
					break;
				case COMMAND_WHITE_ST:	//��������� ������� �������� � �� �������, ������������� wifi
					command_esp.command = COMMAND_STOP_WIFI;
					FIFO_PUSH(sendBuf, command_esp.command);
					FIFO_PUSH(sendBuf, 0);//� ����� ���� ������ �����
					command_esp.state = COMMAND_SEND_READY;
					break;
				default:
					break;
			}
		}
	}
}

//������� ���������� � esp � ������� �����-�� ����� - ����� �� ���������� ������� spi!
inline uint8_t esp_send_cb(void){
	uint8_t answer;
	uint32_t watercnt;
	
	SPI_CS_OFF();
	
	command_esp.state = COMMAND_SENDED;
	
	dtSend = time(NULL);
	answer = ANSWER_FAIL;
	if (!FIFO_IS_EMPTY(recivBuf)){
		if (FIFO_COUNT(recivBuf) > 1){
			FIFO_POP(recivBuf);//���������� �������� ����
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
			if (answer == ANSWER_OK){	//������ AP ��������
			}
			break;
		case COMMAND_WHITE_AP:			//��������� ������� AP
			if (FIFO_COUNT(recivBuf) == 4){ //������� ����������
				watercnt = ((uint32_t)FIFO_FRONT(recivBuf)) << 24;
				FIFO_POP(recivBuf);
				watercnt = (watercnt & 0xFF000000) | ((uint32_t)FIFO_FRONT(recivBuf)) << 16;
				FIFO_POP(recivBuf);
				watercnt = (watercnt & 0xFFFF0000) | ((uint32_t)FIFO_FRONT(recivBuf)) << 8;
				FIFO_POP(recivBuf);
				watercnt = (watercnt & 0xFFFFFF00) | ((uint32_t)FIFO_FRONT(recivBuf));
				FIFO_POP(recivBuf);
				if (answer & ANSWER_AP_STATUS_MASK){ //AP ����������
					command_esp.attemt = COMMAND_ATTEMPT_MAX+1;	//������� ��� ��� ������� ��������� ��� �� ���������� ������������ ����� �������������� ���������
					command_esp.timeout_sec = COMMAND_TIMEOUT_WHITE_CLIENT;	//���� ����������� �������� � ����������� �� ������
					if (answer & ANSWER_AP_CLIENT_MASK){	//���� ������������ �������
						if ((answer & ANSWER_AP_FLAG_MASK) ==  ANSWER_AP_FLAG_WATCNT_SAVE){ //���� ������ ��������
							countWater = watercnt;
						}
					}
				}
			}
			break;
		case COMMAND_START_ST:		//���� ������� ������� ST � �������� ������ � ������
			if (FIFO_COUNT(recivBuf) == 4){ //������� ����������
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
					command_esp.timeout_sec = COMMAND_TIMEOUT_SEC; //������� ���� ��� ���������
					break;
				default:									//�����-�� ��������, ����������� esp � ���������� �������� ��� ��� ���� ���� �������
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
					WIFI_POWER_OFF();						//���� �������� ��������
					break;
				default:									//�����-�� ��������, ����������� esp � ���������� �������� ��� ��� ���� ���� �������
					command_esp.state = COMMAND_EMPTY;
					WIFI_POWER_OFF();
					break;
			}
		default:
			break;
	}
	FIFO_FLUSH(recivBuf);//��� ��������� ���������
	return ERR_OK;
}

//�������� �������, ��� �� ����������� �� ����������
inline void command_reset(void){
	command_esp.state = COMMAND_EMPTY;
	command_esp.timeout_sec = COMMAND_TIMEOUT_SEC;
}

//��������� ����� ��������� � ������� ������� � ��������� � ��� ���������
inline uint8_t command_timeout_check_and_off(void){
	if (command_esp.timeout_sec) {
		command_esp.timeout_sec--;
		if (command_esp.timeout_sec == 0){						//������� �������� ������ �� esp �����
			command_esp.attemt++;
			WIFI_POWER_OFF();
			if (command_esp.attemt <= COMMAND_ATTEMPT_MAX){	//������� ��� ����?
				return WIFI_NEED_ON; //���� �������� wifi ����� �������
			}
		}
	}
	return WIFI_NOT_NEED_ON;
}

//��������� ������� �� esp ���� ��� ���� � ������
inline void wifi_go(void){
	if (command_esp.state == COMMAND_SEND_READY){	//������� ������� � �������� � esp
		if (!FIFO_IS_EMPTY(sendBuf)){				//������� � ������
			if (WIFI_IS_READY()){
				command_esp.state = COMMAND_SENDING;
				command_esp.timeout_sec = COMMAND_TIMEOUT_SEC;	//����������� ��� ���
				SPI_CS_SELECT();					//go!
				spi_send_start(NULL);
				dtSend = time(NULL);
			}
		}
	}
}


#ifdef DEBUG
uint16_t timout(void){		//���������� ����� timout
	return command_esp.timeout_sec;
}
#endif

void wifi_init(void){
	//����� wifi
	command_esp.state = COMMAND_EMPTY;
	command_esp.attemt = 0;
	stdoutT("start to", 0);
	WIFI_INIT_READY();
	WIFI_MODE_INI();
	WIFI_POWER_ON();
	wifi_state = WIFI_FAULT;
	command_esp.timeout_sec = COMMAND_TIMEOUT_SEC;//�������� ������ ��������
}
