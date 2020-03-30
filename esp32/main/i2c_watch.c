/*
 * master i2c and slave DS1337S
 * NO POWER CONTROL!
 */

#include "esp_err.h"
#include "driver/i2c.h"
#include "i2c_watch.h"
#include "HAL_GPIO.h"

#include "esp_log.h"

#include "time.h"
#include "utils.h"

#define I2C_MASTER_RX_BUF_DISABLE	0 	//length buffer
#define I2C_MASTER_TX_BUF_DISABLE	0

//for DS1337S
#define WATCH_ADRESS	0b1101000
#define WATCH_REG_ADR	0x0				//start register address
#define	WATCH_BUF_LEN	16				//data length for

static uint32_t i2c_frequency = 100000;
static i2c_port_t i2c_port = I2C_NUM_0;

static const char *TAG = "I2C";

esp_err_t watchInit(void) {

	ESP_LOGI(TAG, "i2c init start");
	esp_err_t ret = i2c_driver_install(i2c_port, I2C_MODE_MASTER, I2C_MASTER_RX_BUF_DISABLE, I2C_MASTER_TX_BUF_DISABLE, 0);
	if (ret == ESP_OK) {

		ESP_LOGI(TAG, "i2c driver init");
		// @formatter:off
		i2c_config_t conf = {
			.mode = I2C_MODE_MASTER,
			.sda_io_num = I2C_MASTER_SDA,
			.sda_pullup_en = GPIO_PULLUP_ENABLE,
			.scl_io_num = I2C_MASTER_SCL,
			.scl_pullup_en = GPIO_PULLUP_ENABLE,
			.master.clk_speed = i2c_frequency
		};
		// @formatter:on
		ret = i2c_param_config(i2c_port, &conf);
	}
	return ret;
}

esp_err_t watchWrite(time_t value) {
	setenv("TZ", "MSK-3", 1);
	tzset();
	struct tm t;
	localtime_r(&value, &t);
	i2c_cmd_handle_t cmd = i2c_cmd_link_create();
	i2c_master_start(cmd);
	i2c_master_write_byte(cmd, (WATCH_ADRESS << 1) | I2C_MASTER_WRITE, I2C_MASTER_ACK);
	i2c_master_write_byte(cmd, WATCH_REG_ADR, I2C_MASTER_ACK);
	i2c_master_write_byte(cmd, DecToBCD((uint8_t) t.tm_sec), I2C_MASTER_ACK);
	i2c_master_write_byte(cmd, DecToBCD((uint8_t) t.tm_min), I2C_MASTER_ACK);
	i2c_master_write_byte(cmd, DecToBCD((uint8_t) t.tm_hour), I2C_MASTER_ACK);
	i2c_master_write_byte(cmd, DecToBCD((uint8_t) t.tm_wday + 1), I2C_MASTER_ACK);
	i2c_master_write_byte(cmd, DecToBCD((uint8_t) t.tm_mday), I2C_MASTER_ACK);
	i2c_master_write_byte(cmd, DecToBCD((uint8_t) t.tm_mon + 1), I2C_MASTER_ACK);
	i2c_master_write_byte(cmd, DecToBCD((uint8_t) ((t.tm_year + 1900) - 2000)), I2C_MASTER_ACK);
	i2c_master_stop(cmd);
	esp_err_t ret = i2c_master_cmd_begin(i2c_port, cmd, 50 / portTICK_RATE_MS);
	i2c_cmd_link_delete(cmd);
	return ret;
}
//NO WORK!
esp_err_t watchRead(struct tm *value) {

	uint8_t buf[WATCH_BUF_LEN];

	i2c_cmd_handle_t cmd = i2c_cmd_link_create();
	i2c_master_start(cmd);
	i2c_master_write_byte(cmd, (WATCH_ADRESS << 1) | I2C_MASTER_WRITE, I2C_MASTER_ACK);
	i2c_master_write_byte(cmd, WATCH_REG_ADR, I2C_MASTER_ACK);
	i2c_master_start(cmd);
	i2c_master_write_byte(cmd, (WATCH_ADRESS << 1) | I2C_MASTER_READ, I2C_MASTER_ACK);
	i2c_master_read(cmd, buf, WATCH_BUF_LEN - 1, I2C_MASTER_ACK);
	i2c_master_read_byte(cmd, buf + WATCH_BUF_LEN - 1, I2C_MASTER_NACK);
	i2c_master_stop(cmd);
	esp_err_t ret = i2c_master_cmd_begin(i2c_port, cmd, 50 / portTICK_RATE_MS);
	i2c_cmd_link_delete(cmd);
	if (ret == ESP_OK) {
		uint8_t i = 0;
		for (; i < WATCH_BUF_LEN; i++) {
			ESP_LOGI(TAG, "w=%x", buf[i]);
		}
	}
	return ret;
}
