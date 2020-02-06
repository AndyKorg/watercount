/*
* HAL for 1.54inch e-Paper Module MH-ET live
*/

#ifndef _EPDIF_H
#define _EPDIF_H

#include "esp_err.h"

typedef enum {
	LCD_POWER_OFF,			//screen power is off
	LCD_POWER_ON
} lcd_pwr_mode_t;

void lcd_setup_pin(lcd_pwr_mode_t mode);
esp_err_t IfInit(const uint16_t max_buf_size);//reserved description for dma
void lcd_reset(void);
esp_err_t lcd_ready(void);					//ESP_OK if lcd free
void lcd_cmd(const uint8_t cmd, const uint8_t *data, const uint16_t len);
//void lcd_data(const uint8_t *data, int len);

#endif
