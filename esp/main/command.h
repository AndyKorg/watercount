/*
 * command.h
 *
 *  Created on: 2 ���. 2019 �.
 *      Author: Administrator
 */

#ifndef MAIN_COMMAND_H_
#define MAIN_COMMAND_H_

#include "driver/gpio.h"
#include "esp_err.h"

// Command
#define	COMMAND_START_ST_WIFI		0x8A	//������ ����������� � WiFi � cayenn � �������� � ������ �������� �������� ����
#define COMMAND_START_ST_WIFI_DATA_LEN	4
#define	COMMAND_START_AP			0x8B	//�������� AP
#define	COMMAND_WHITE_AP			0x8C	//�������� ����������� �������� AP, ������ ���������� ��� ������������, ������ �������
#define COMMAND_WHITE_AP_DATA_LEN	4		//���������� ���������� �������� �������� ����. �������� ������������� ������ ���� ���� ������ ������ �������
#define	COMMAND_WHITE_ST			0x8D	//������ ���������� ����������� � wifi � �������� ������ � ������
#define	COMMAND_STOP_WIFI			0x8E	//��������� ��� ���������� � ��������� wifi

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
#define ANSWER_AP_CLIENT_NUM		1			//����� �������� ���� �������
#define ANSWER_AP_FLAG_MASK			0b11110000	//����� ������������� �������� ��������
#define ANSWER_AP_FLAG_WATCNT_SAVE	0b00100000	//������� ������� ����

//Hardware confirm
#define CHIP_READY_PIN	GPIO_Pin_5
#define CHIP_READY_NUM	GPIO_NUM_5
#define CHIP_BUSY()		do{gpio_set_level(CHIP_READY_NUM, 1);}while(0)
#define CHIP_READY() 	do{gpio_set_level(CHIP_READY_NUM, 0);}while(0)

//�����-���� ������ �������
esp_err_t spi_start_command(uint8_t* send_byte);
esp_err_t spi_stop_command();

//������� ���� ������� �� SPI
esp_err_t spi_recive_command(uint8_t recive_byte);
//������ ���� ������
esp_err_t spi_answer(uint8_t* send_byte);

void cmd_init(void);

#endif /* MAIN_COMMAND_H_ */
