/*
 * cayenne.h
 *
 *  Created on: 30 РґРµРє. 2018 Рі.
 *      Author: Administrator
 */

#ifndef APP_WEB_INCLUDE_CAYENNE_H_
#define APP_WEB_INCLUDE_CAYENNE_H_

#include <stddef.h>
#include "mqtt_client.h"

#define CAYENN_MAX_LEN		64	//Максимальная длина идентификаторов

typedef struct {
  char host[CAYENN_MAX_LEN];
  uint16_t port;
  char user[CAYENN_MAX_LEN];
  char pass[CAYENN_MAX_LEN];
  char	client_id[CAYENN_MAX_LEN];
  char deviceName[CAYENN_MAX_LEN];
  esp_err_t LastState;
} cayenne_t;

//MQTT Cayenne default
#define MQTT_HOST "mqtt://mqtt.mydevices.com"
#define MQTT_PORT 1883
#define MQTT_USER "123-abc"
#define MQTT_PASS "pass"
#define MQTT_CLIENT_ID "abc123"
#define MQTT_MODEL_NAME "modelname"
#define MQTT_PROTOKOL "mqtt://"

//different params
#define MQTT_CAYENNE_VER	"v1/"
#define MQTT_CAYENNE_DELEMITER	"/things/"
#define	MQTT_CAYENNE_TYPE_SYS_MODEL "/sys/model"
#define	MQTT_CAYENNE_TYPE_SYS_VER "/sys/version"
#define	MQTT_CAYENNE_TYPE_SYS_CPU_MODEL "/sys/cpu/model"
#define	MQTT_CAYENNE_TYPE_SYS_CPU_SPEED "/sys/cpu/speed"
#define	MQTT_CAYENNE_TYPE_DATA "/data/"
#define	MQTT_CAYENNE_TYPE_CMD "/cmd/"
//#define	MQTT_CAYENNE_CHANNAL "0"
#define	MQTT_CAYENNE_MEASURE_PERIOD "1"
#define	MQTT_CAYENNE_MEASURE_START "2"
#define	MQTT_CAYENNE_MEASURE_LEFT "3"

#define PROTOCOL_NAMEv31		/*MQTT version 3.1 compatible with Mosquitto v0.15*/
//PROTOCOL_NAMEv311			/*MQTT version 3.11 compatible with https://eclipse.org/paho/clients/testing/*/

//MQTT flags
#define MQTT_QOS_TYPE_AT_MOST_ONCE	0 	//Без контроля
#define MQTT_QOS_TYPE_AT_LEAST_ONCE	1	//Хотя бы один подписчик получил топик
#define MQTT_QOS_TYPE_EXACTLY_ONE	2	//Гарантировано получен топик

#define MQTT_RETAIN_ON			1	//Сообщать новому подписчику этот топик
#define MQTT_RETAIN_OFF			0	//не сообщать

extern esp_mqtt_client_handle_t mqtt_client;	//Клиент для каена
#define CAYENN_PROCESS			(ESP_OK+1)		//Идет процесс передачи
extern cayenne_t cayenn_cfg;		//Текущая конфигурация каена и состояние

/*
 * Sensor type's
 */
#define CAYENNE_DIGITAL_SENSOR	"digital_sensor,d=%d"	//Digital Sensor,Digital (0/1)
#define CAYENNE_ANALOG_SENSOR	"analog_sensor,null=%d"	//Analog Sensor,Analog
#define CAYENNE_ACCELERATION	"accel,g=%d"		//Acceleration,Acceleration, gx,gy,gz
#define CAYENNE_BANDWIDTH_KBPS	"bw,kbps=%d"		//Bandwidth,Kbps
#define CAYENNE_BANDWIDTH_GBPS	"bw,gbps=%d"		//Bandwidth,,Gbps
#define CAYENNE_BANDWIDTH_MBPS	"bw,mbps=%d"		//Bandwidth,,Mbps
#define CAYENNE_BAROMETRIC_PRESSURE_PA	"bp,pa=%d"	//Barometric pressure,Pascal
#define CAYENNE_BAROMETRIC_PRESSURE_HPA	"bp,hpa=%d"	//Barometric pressure,Hecto Pascal
#define CAYENNE_BATTERY_P	"batt,p=%d"		//Battery,% (0 to 100)
#define CAYENNE_BATTERY_R	"batt,r=%d"		//Battery,Ratio
#define CAYENNE_BATTERY_V	"batt,v=%d"		//Battery,Volts
#define CAYENNE_CO		"co,ppm=%d"		//Carbon Monoxide,Parts per milliion
#define CAYENNE_CO2		"co2,ppm=%d"		//Carbon Dioxide,Parts per milliion
#define CAYENNE_CPU_LOAD	"cpuload,p=%d"		//CPU Load,Percent (%)
#define CAYENNE_COUNTER		"counter,null=%d"	//Counter,Analog
#define CAYENNE_CURRENT_A	"current,a=%d"		//Current,Ampere
#define CAYENNE_CURRENT_MA	"current,ma=%d"		//Current,Milliampere
#define CAYENNE_ENERGY		"energy,kwh=%d"		//Energy,Killowatt Hour
#define CAYENNE_EXT_WATERLEAK	"ext_wleak,null=%d"	//External Waterleak,Analog
#define CAYENNE_FREQUENCY	"freq,hz=%d"		//Frequency,Hertz
#define CAYENNE_GPS		"gps,m=%d"		//GPS, GPS	lat,long,alt
#define CAYENNE_GYROSCOPE_RPM	"gyro,rpm=%d"		//Gyroscope,Rotation per minute	gyro_x,gyro_y,gyro_z
#define CAYENNE_GYROSCOPE_DPS	"gyro,dps=%d"		//Gyroscope,Degree per second	gyro_x,gyro_y,gyro_z
#define CAYENNE_LOW_BATTERY	"low_battery,d=%d"	//low_battery,Digital (0/1)
#define CAYENNE_LT100GPS	"LT100GPS,m=%d"		//LT100GPS,LT100 GPS
#define CAYENNE_LUMINOSITY_LUX	"lum,lux=%d"		//Luminosity, Lux
#define CAYENNE_LUMINOSITY_V	"lum,v=%d"		//Luminosity, Volts
#define CAYENNE_LUMINOSITY_P	"lum,p=%d"		//Luminosity,% (0 to 100)
#define CAYENNE_LUMINOSITY_R	"lum,r=%d"		//Luminosity,Ratio
#define CAYENNE_INTRUSION	"intrusion,d=%d"	//Intrusion,Digital (0/1)
#define CAYENNE_MEMORY_B	"memory,b=%d"		//Memory,* Bytes
#define CAYENNE_MEMORY_KB	"memory,kb=%d"		// ,KB
#define CAYENNE_MEMORY_GB	"memory,gb=%d"		// ,GB
#define CAYENNE_MEMORY_MB	"memory,mb=%d"		// ,MB
#define CAYENNE_MOTION		"motion,d=%d"		//Motion,Digital (0/1)
#define CAYENNE_PARKING		"parking,d=%d"		//Parking,Digital (0/1)
#define CAYENNE_PARTICULATE_MATTER	"particulate_matter,mgpcm=%d"	//Particulate Matter,Micrograms per cubic meter
#define CAYENNE_POWER_W		"pow,w=%d"		//Power,* Watts
#define CAYENNE_POWER_KW	"pow,kw=%d"		//Power,Kilowatts
#define CAYENNE_PROXIMITY_CM	"prox,cm=%d"		//Proximity,* Centimeter
#define CAYENNE_PROXIMITY_M	"prox,m=%d"		//Proximity,Meter
#define CAYENNE_PROXIMITY_D	"prox,d=%d"		//Proximity,Digital (0/1)
#define CAYENNE_ORIENT_ROLL	"ori_roll,deg=%d"	//Orientation.Roll,Degree Angle
#define CAYENNE_ORIENT_PITCH	"ori_pitch,deg=%d"	//Orientation.Pitch,Degree Angle
#define CAYENNE_ORIENT_AZIMUTH	"ori_azim,deg=%d"	//Orientation.Azimuth,Degree Angle
#define CAYENNE_RAIN_LEVEL_CM	"rain_level,cm=%d"	//Rain Level,Centimeter
#define CAYENNE_RAIN_LEVEL_MM	"rain_level,mm=%d"	//Rain Level,* Millimeter
#define CAYENNE_RSSI		"rssi,dbm=%d"		//Received signal strength indicator,dBm
#define CAYENNE_RELATIVE_HUMIDITY_P	"rel_hum,p=%d"	//Relative Humidity,* Percent (%)
#define CAYENNE_RELATIVE_HUMIDITY_R	"rel_hum,r=%d"	//Relative Humidity,Ratio
#define CAYENNE_RESISTANCE	"res,ohm=%d"		//Resistance,Ohm
#define CAYENNE_SNR		"snr,db=%d"		//Signal Noise Ratio,Decibels
#define CAYENNE_SOIL_MOISTURE	"soil_moist,p=%d"	//Soil Moisture,Percent (%)
#define CAYENNE_SOIL_PH		"soil_ph,null=%d"	//Soil pH,Analog
#define CAYENNE_SOIL_WATER_TENSION_KPA	"soil_w_ten,kpa=%d"	//Soil Water Tension,* Kilopascal
#define CAYENNE_SOIL_WATER_TENSION_PA	"soil_w_ten,pa=%d"	// ,Pascal
#define CAYENNE_TANK_LEVEL	"tl,null=%d"		//Tank Level,Analog
#define CAYENNE_TEMPERATURE_F	"temp,f=%d"		//Temperature,Fahrenheit
#define CAYENNE_TEMPERATURE_C	"temp,c=%d"		//Temperature,* Celsius
#define CAYENNE_TEMPERATURE_K	"temp,k=%d"		//Temperature,Kelvin
#define CAYENNE_VOLTAGE_V	"voltage,v=%d"		//Voltage,* Volts
#define CAYENNE_VOLTAGE_MV	"voltage,mv=%d"		//Voltage,Millivolts
#define CAYENNE_TRAP		"trap,d=%d"		//Trap,Digital (-1/0/1/2)
#define CAYENNE_WATERLEAK	"waterleak,d=%d"	//Waterleak,Digital (0/1)
#define CAYENNE_WIND_SPEED	"wind_speed,kmh=%d"	//Wind Speed,Kilometer per hour

char* CayenneTopic (cayenne_t* cfg, const char* type, const char* channal);
esp_err_t CayenneChangeInteger(cayenne_t* cfg, const uint8_t chanal, const char* nameSensor, const uint32_t value);
void Cayenne_app_start (void);				//запуск mqtt клиента
esp_err_t Cayenne_app_stop (void);			//Оторвать все подключения, ESP_OK - закрытие соединения начато
esp_err_t Cayenne_Init (void);				//Иницилизация конфигурации, чтение из nvs если оно там есть


#endif /* APP_WEB_INCLUDE_CAYENNE_H_ */
