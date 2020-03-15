/*
 * HAL for 1.54inch e-Paper Module MH-ET live
 */

#include <string.h>
#include "driver/gpio.h"
#include "driver/spi_master.h"
#include "esp_log.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "../HAL_GPIO.h"
#include "epdif.h"
#include "driver/rtc_io.h"

static const char *TAG = "EPDIF";

#define PERIOD_RESET_MS		10	//Initialize period reset

spi_device_handle_t spi;

void lcd_rtc_pin_down(gpio_num_t pin) {
	rtc_gpio_init(pin);
	rtc_gpio_set_direction(pin, RTC_GPIO_MODE_INPUT_ONLY);
	rtc_gpio_pullup_dis(pin);
	rtc_gpio_pulldown_en(pin);
	rtc_gpio_hold_en(pin);
}


void lcd_setup_pin(lcd_pwr_mode_t mode) {

	static lcd_pwr_mode_t prev = LCD_POWER_OFF;

	if (prev == mode){
		return;
	}

	if (mode == LCD_POWER_OFF) {
		ESP_LOGI(TAG, "POWER OFF START");
		//spi stop
		spi_bus_remove_device(spi);
		spi_bus_free(EPD_SPI_HOST);
		//Busy
		rtc_gpio_deinit(EPD_DC_PIN);
		gpio_set_direction(EPD_BUSY_PIN, GPIO_MODE_INPUT);
		gpio_pulldown_dis(EPD_BUSY_PIN);
		gpio_pullup_dis(EPD_BUSY_PIN);
		while (lcd_ready() != ESP_OK) {
			vTaskDelay(20 / portTICK_RATE_MS);
		}
		ESP_LOGI(TAG, "pin display sleep mode set");
		//pin control off. Avoid spurious supply voltage.
		gpio_set_direction(EPD_CS_PIN, GPIO_MODE_OUTPUT);
		gpio_set_level(EPD_CS_PIN, 0);
		gpio_set_direction(EPD_CLK_PIN, GPIO_MODE_OUTPUT);
		gpio_set_level(EPD_CLK_PIN, 0);
		gpio_set_direction(EPD_MOSI_PIN, GPIO_MODE_OUTPUT);
		gpio_set_level(EPD_MOSI_PIN, 0);
		lcd_rtc_pin_down(EPD_RST_PIN); //reset
		lcd_rtc_pin_down(EPD_DC_PIN);
		//display power off
		rtc_gpio_hold_dis(EPD_POWER_PIN);
		rtc_gpio_deinit(EPD_POWER_PIN);
		gpio_set_direction(EPD_POWER_PIN, GPIO_MODE_OUTPUT);
		gpio_set_level(EPD_POWER_PIN, 0);
		lcd_rtc_pin_down(EPD_POWER_PIN);
		lcd_rtc_pin_down(EPD_BUSY_PIN); //busy, after power down display!
	} else {
		ESP_LOGI(TAG, "POWER ON START");
		//Others pins for SPI in init function setting
		//Power
		rtc_gpio_hold_dis(EPD_POWER_PIN);
		rtc_gpio_deinit(EPD_POWER_PIN);
		gpio_set_direction(EPD_POWER_PIN, GPIO_MODE_OUTPUT);
		gpio_set_level(EPD_POWER_PIN, 1);
		gpio_pulldown_en(EPD_POWER_PIN);
		gpio_pullup_dis(EPD_POWER_PIN);
		//reset pin
		rtc_gpio_hold_dis(EPD_RST_PIN);
		rtc_gpio_deinit(EPD_RST_PIN);
		gpio_set_direction(EPD_RST_PIN, GPIO_MODE_OUTPUT);
		//Busy pin
		rtc_gpio_hold_dis(EPD_BUSY_PIN);
		rtc_gpio_deinit(EPD_BUSY_PIN);
		gpio_set_direction(EPD_BUSY_PIN, GPIO_MODE_INPUT);
		gpio_pulldown_dis(EPD_BUSY_PIN);
		gpio_pullup_dis(EPD_BUSY_PIN);
		//dc pin
		rtc_gpio_hold_dis(EPD_DC_PIN);
		rtc_gpio_deinit(EPD_DC_PIN);
		gpio_set_direction(EPD_DC_PIN, GPIO_MODE_OUTPUT);
	}

	prev = mode;
}

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
	return gpio_get_level(EPD_BUSY_PIN) ? ESP_FAIL : ESP_OK;
}

IRAM_ATTR void dc_mode_cb(spi_transaction_t *tr) {
	int dc = (int) tr->user;
	gpio_set_level(EPD_DC_PIN, dc);
}

void lcd_reset(void) {
	gpio_set_level(EPD_RST_PIN, 0);
	vTaskDelay(PERIOD_RESET_MS / portTICK_RATE_MS);
	gpio_set_level(EPD_RST_PIN, 1);
	vTaskDelay(PERIOD_RESET_MS / portTICK_RATE_MS);
}

esp_err_t IfInit(const uint16_t max_buf_size) {

	lcd_setup_pin(LCD_POWER_ON);
	//SPI Bus setting
	esp_err_t ret;
	// @formatter:off
	spi_bus_config_t buscfg = {
        .miso_io_num = -1,
        .mosi_io_num = EPD_MOSI_PIN,
        .sclk_io_num = EPD_CLK_PIN,
        .quadwp_io_num = -1,
        .quadhd_io_num = -1,
        .max_transfer_sz = max_buf_size+4, 				//Maximum transfer size, in bytes. Defaults to 4094 if 0
			};
	spi_device_interface_config_t devcfg = {
			.clock_speed_hz = 4 * 1000 * 1000,       	//Clock out at 4 MHz  - 125 us CLK
			.mode = 0,									//SPI mode 0
			.spics_io_num = EPD_CS_PIN,					//CS pin
			.queue_size = 7,							//We want to be able to queue 7 transactions at a time
			.pre_cb = dc_mode_cb,  						//Specify pre-transfer callback to handle D/C line
			};
	// @formatter:on
	//Initialize the SPI bus
	ret = spi_bus_initialize(EPD_SPI_HOST, &buscfg, EPD_DMA_CHAN);
	ESP_ERROR_CHECK(ret);
	//Attach the LCD to the SPI bus
	ret = spi_bus_add_device(EPD_SPI_HOST, &devcfg, &spi);
	ESP_ERROR_CHECK(ret);

	return ESP_OK;
}
