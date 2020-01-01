/*
 * HAL for 1.54inch e-Paper Module MH-ET live
 */

#include <string.h>
#include "driver/gpio.h"
#include "driver/spi_master.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "epdif.h"

#define EPD_RST_PIN			GPIO_NUM_26		//External reset pin (Low for reset)
#define EPD_DC_PIN			GPIO_NUM_27		//Data/Command control pin (High for data, and low for command)
#define EPD_SPI_HOST		VSPI_HOST		//Data/Command control pin (High for data, and low for command)
#define EPD_BUSY_PIN		GPIO_NUM_25		//Busy pin (high is busy)
#define EPD_DMA_CHAN    	2
#define EPD_NUM_MOSI		GPIO_NUM_23
#define EPD_NUM_CLK			GPIO_NUM_18
#define EPD_NUM_CS			GPIO_NUM_5

spi_device_handle_t spi;

void lcd_cmd(const uint8_t cmd, const uint8_t *data, const uint16_t len) {
	esp_err_t ret;
	spi_transaction_t t;
	memset(&t, 0, sizeof(t));       	//Zero out the transaction
	t.length = 8;                     	//Command is 8 bits
	t.tx_buffer = &cmd;               	//The data is the cmd itself
	t.user = (void*) 0;                	//D/C needs to be set to 0
	ret = spi_device_polling_transmit(spi, &t);  //Transmit!
	assert(ret == ESP_OK);            	//Should have had no issues.
	if (len == 0)
		return;
	memset(&t, 0, sizeof(t));       	//Zero out the transaction
	t.length = len * 8;                 //Len is in bytes, transaction length is in bits.
	t.tx_buffer = data;               	//Data
	t.user = (void*) 1;                	//D/C needs to be set to 1
	ret = spi_device_polling_transmit(spi, &t);  //Transmit!
	assert(ret == ESP_OK);            	//Should have had no issues.
}

/* Send data to the LCD. Uses spi_device_polling_transmit, which waits until the
 * transfer is complete.
 *
 * Since data transactions are usually small, they are handled in polling
 * mode for higher speed. The overhead of interrupt transactions is more than
 * just waiting for the transaction to complete.
 */
void lcd_data(const uint8_t *data, int len) {
	esp_err_t ret;
	spi_transaction_t t;
	if (len == 0)
		return;             //no need to send anything
	memset(&t, 0, sizeof(t));       //Zero out the transaction
	t.length = len * 8;                 //Len is in bytes, transaction length is in bits.
	t.tx_buffer = data;               //Data
	t.user = (void*) 1;                //D/C needs to be set to 1
	ret = spi_device_polling_transmit(spi, &t);  //Transmit!
	assert(ret == ESP_OK);            //Should have had no issues.
}

esp_err_t lcd_ready(void) {
	if (gpio_get_level(EPD_BUSY_PIN)) {
		return ESP_OK;
	}
	return ESP_FAIL;
}

IRAM_ATTR void dc_mode_cb(spi_transaction_t *tr) {
	int dc = (int) tr->user;
	gpio_set_level(EPD_DC_PIN, dc);
}

void lcd_reset(void) {
	gpio_set_level(EPD_RST_PIN, 0);
	vTaskDelay(200 / portTICK_RATE_MS);
	gpio_set_level(EPD_RST_PIN, 1);
	vTaskDelay(200 / portTICK_RATE_MS);
}

esp_err_t IfInit(const uint16_t max_buf_size) {
	gpio_set_direction(EPD_DC_PIN, GPIO_MODE_OUTPUT);
	gpio_set_direction(EPD_RST_PIN, GPIO_MODE_OUTPUT);
	gpio_set_direction(EPD_BUSY_PIN, GPIO_MODE_INPUT);

	esp_err_t ret;
	spi_bus_config_t buscfg = { // @formatter:off
        .miso_io_num = -1,
        .mosi_io_num = EPD_NUM_MOSI,
        .sclk_io_num = EPD_NUM_CLK,
        .quadwp_io_num = -1,
        .quadhd_io_num = -1,
        .max_transfer_sz = max_buf_size+4,
		// @formatter:on
			};
	spi_device_interface_config_t devcfg = {
#ifdef CONFIG_LCD_OVERCLOCK
        .clock_speed_hz=26*1000*1000,           //Clock out at 26 MHz
#else
			.clock_speed_hz = 1 * 1000 * 1000,        //Clock out at 10 MHz 4 - 125 us
#endif
			.mode = 0,									//SPI mode 0
			.spics_io_num = EPD_NUM_CS,					//CS pin
			.queue_size = 7,							//We want to be able to queue 7 transactions at a time
			.pre_cb = dc_mode_cb,  						//Specify pre-transfer callback to handle D/C line
			};
	//Initialize the SPI bus
	ret = spi_bus_initialize(EPD_SPI_HOST, &buscfg, EPD_DMA_CHAN);
	ESP_ERROR_CHECK(ret);
	//Attach the LCD to the SPI bus
	ret = spi_bus_add_device(EPD_SPI_HOST, &devcfg, &spi);
	ESP_ERROR_CHECK(ret);

	return ESP_OK;
}
