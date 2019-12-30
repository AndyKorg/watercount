/*
* HAL for 1.54inch e-Paper Module MH-ET live
*/

#ifndef _EPDIF_H
#define _EPDIF_H

#include "esp_err.h"

esp_err_t IfInit(void);
void lcd_reset(void);
void lcd_cmd(const uint8_t cmd, const uint8_t *data, const uint16_t len);
//void lcd_data(const uint8_t *data, int len);
esp_err_t lcd_ready(void);					//ESP_OK if lcd free

#endif
