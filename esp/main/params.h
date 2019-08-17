/*
 * ќбработка всех известных переменных в виде списка или по имени
 *  онвертирование в строки и обратно.
 */

#ifndef MAIN_PARAMS_H_
#define MAIN_PARAMS_H_

#include <stddef.h>
#include "esp_err.h"

enum paramType_t{
  PARAM_TYPE_NONE = -1,	//ѕараметр не отноститс€ ни к одной группе и не сохран€етс€ во flash
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

#define PARAM_WATERCOUNT	"watercount"	//ѕеременна€ не сохран€етс€ в посто€нной пам€ти
#define	PARAM_CHANAL_CAYEN	1		//Ќомер канала обслуживающий переменную, определ€етс€ облаком cayenn
#define PARAM_NAME_SENSOR	"counter"	//»м€ сенсора дл€ облака

typedef enum{
  READ_SPI,
  NO_READ_SPI
} watercount_state_t;

typedef struct{
  uint32_t		count;			//—обственно сами показани€ счетчика
  watercount_state_t 	state;			//прочитано через SPI
} watercount_t;

extern watercount_t watercount;			//—обственно сами показани€ счетчика

typedef struct{
  uint8_t pos;
  enum paramType_t paramType;
} seechRecord_t;

/*
 * —тартует выборку известных имен переменных
 * ¬озвращает указатель ESP_OK если список не пустой
 * ¬ передаваемый параметр заносит текущую позицию в списке переменных
 */
esp_err_t getFirstVarName(seechRecord_t* sr);
/*
 * Ѕерет следующий элемент списка переменных
 * ¬озвращает указатель на им€ переменной или null если достигнут конец списка.
 * ”величивает парметр позиции на одну позицию
 */
char* getNextVarName(seechRecord_t* sr);

/*
 * «аписывает значение переменой указанную в позиции sr списка переменных
 */
esp_err_t saveValue(seechRecord_t sr, char* value);

/*
 * добавл€ет в строку toStr значение переменной valName если найдет.
 * увеличивает lenVal на количество добавленых символов
 * возвращает указатель на конец добавленой строки
 */
char* putsValue(char* toStr, char* varName, size_t *lenVal);


#endif /* MAIN_PARAMS_H_ */
