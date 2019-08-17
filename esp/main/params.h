/*
 * ��������� ���� ��������� ���������� � ���� ������ ��� �� �����
 * ��������������� � ������ � �������.
 */

#ifndef MAIN_PARAMS_H_
#define MAIN_PARAMS_H_

#include <stddef.h>
#include "esp_err.h"

enum paramType_t{
  PARAM_TYPE_NONE = -1,	//�������� �� ���������� �� � ����� ������ � �� ����������� �� flash
  PARAM_TYPE_WIFI = 0,
  PARAM_TYPE_CAEN = 1
};

enum param_Num_t{
  PARAM_SSID_NAME_NUM = 0,
  PARAM_PASWRD_NUM,
  PARAM_MQTT_HOST_NUM,
  PARAM_MQTT_PORT_NUM,
  PARAM_MQTT_USER_NUM,
  PARAM_MQTT_PASS_NUM,
  PARAM_MQTT_CLIENT_ID_NUM,
  PARAM_MQTT_MODEL_NAME_NUM,
  PARAM_WATERCOUNT_NUM
};

#define PARAM_WATERCOUNT	"watercount"	//���������� �� ����������� � ���������� ������
#define	PARAM_CHANAL_CAYEN	1		//����� ������ ������������� ����������, ������������ ������� cayenn
#define PARAM_NAME_SENSOR	"counter"	//��� ������� ��� ������

typedef enum{
  READ_SPI,
  NO_READ_SPI
} watercount_state_t;

typedef struct{
  uint32_t		count;			//���������� ���� ��������� ��������
  watercount_state_t 	state;			//��������� ����� SPI
} watercount_t;

extern watercount_t watercount;			//���������� ���� ��������� ��������

typedef struct{
  uint8_t pos;
  enum paramType_t paramType;
} seechRecord_t;

/*
 * �������� ������� ��������� ���� ����������
 * ���������� ��������� ESP_OK ���� ������ �� ������
 * � ������������ �������� ������� ������� ������� � ������ ����������
 */
esp_err_t getFirstVarName(seechRecord_t* sr);
/*
 * ����� ��������� ������� ������ ����������
 * ���������� ��������� �� ��� ���������� ��� null ���� ��������� ����� ������.
 * ����������� ������� ������� �� ���� �������
 */
char* getNextVarName(seechRecord_t* sr);

/*
 * ���������� �������� ��������� ��������� � ������� sr ������ ����������
 */
esp_err_t saveValue(seechRecord_t sr, char* value);

/*
 * ��������� � ������ toStr �������� ���������� valName ���� ������.
 * ����������� lenVal �� ���������� ���������� ��������
 * ���������� ��������� �� ����� ���������� ������
 */
char* putsValue(char* toStr, char* varName, size_t *lenVal);


#endif /* MAIN_PARAMS_H_ */
