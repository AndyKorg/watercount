/*
 * i2c_clock.h
 *
 *  Created on: 29 мар. 2020 г.
 *      Author: Administrator
 */

#ifndef MAIN_I2C_WATCH_H_
#define MAIN_I2C_WATCH_H_

#include <time.h>

esp_err_t watchInit(void);
esp_err_t watchWrite(time_t value);
esp_err_t watchRead(struct tm *value);

#endif /* MAIN_I2C_WATCH_H_ */
