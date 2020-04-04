/*
 * define only gpio
 * define pin and adc chanal in ulp\sensor.S
 */

#ifndef MAIN_HAL_GPIO_H_
#define MAIN_HAL_GPIO_H_

#define STARTUP_MODE_PIN	GPIO_NUM_36	//start up mode: AP start or ST start

//adc sensor - ADC1 only!
#define SENSOR_NAMUR_CHANAL ADC1_CHANNEL_6	//GPIO_NUM_34
#define SENSOR_NAMUR_ATTEN	ADC_ATTEN_11db
#define SENSOR_BAT_CHANAL 	ADC1_CHANNEL_5	//GPIO_NUM_33
#define SENSOR_BAT_ATTEN	ADC_ATTEN_11db
#define ADC_WIDTH_SENSOR	ADC_WIDTH_12Bit
//threshold sensors
#define BAT_LOW				2000 	//3v low threshold only SENSOR_BAT_ATTEN = ADC_ATTEN_11db and ADC_WIDTH_SENSOR = ADC_WIDTH_12Bit
#define ADC_COEFF_BAT		1692946 //correction factor of the voltage divider
#define DEFAULT_VREF		1090	//measure VREF voltage, example adc2_vref_to_gpio(GPIO_NUM_25);
//default threshold NAMUR sensor
#define SENSOR_THR_HIGH_MAX	1700 	//more than this threshold is considered a shot circuit
#define SENSOR_THR_HIGH		1300 	//is considered a line high
#define SENSOR_THR_LOW		700 	//less than this threshold is considered a line low
#define SENSOR_THR_LOW_MIN	300 	//less than this threshold is considered a line break


//power enable sensor
#define SENSOR_POWER_EN		GPIO_NUM_4
//dispaly
#define EPD_RST_PIN			GPIO_NUM_26		//External reset pin (Low for reset)
#define EPD_DC_PIN			GPIO_NUM_27		//Data/Command control pin (High for data, and low for command)
#define EPD_BUSY_PIN		GPIO_NUM_25		//Busy pin (high is busy)
#define EPD_MOSI_PIN		GPIO_NUM_23
#define EPD_CLK_PIN			GPIO_NUM_18
#define EPD_CS_PIN			GPIO_NUM_5
#define EPD_POWER_PIN		GPIO_NUM_32		//power display
#define EPD_SPI_HOST		VSPI_HOST		//Data/Command control pin (High for data, and low for command)
#define EPD_DMA_CHAN    	2				//Only 1 or 2, channel 0 has limitations

//i2c master
#define I2C_MASTER_SDA		GPIO_NUM_16
#define I2C_MASTER_SCL		GPIO_NUM_17
#endif /* MAIN_HAL_GPIO_H_ */
