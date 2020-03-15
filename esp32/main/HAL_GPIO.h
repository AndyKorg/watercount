/*
 * define only gpio
 */

#ifndef MAIN_HAL_GPIO_H_
#define MAIN_HAL_GPIO_H_

#define STARTUP_MODE_PIN	GPIO_NUM_36	//start up mode: AP start or ST start

//adc sensor - ADC1 only!
#define SENSOR_NAMUR_CHANAL ADC1_CHANNEL_6	//GPIO_NUM_34
#define SENSOR_NAMUR_ATTEN	ADC_ATTEN_11db
#define SENSOR_BAT_CHANAL 	ADC1_CHANNEL_5	//GPIO_NUM_33
#define SENSOR_BAT_ATTEN	ADC_ATTEN_11db
#define ADC_WIDTH_SENSOR	ADC_WIDTH_11Bit
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

#endif /* MAIN_HAL_GPIO_H_ */
