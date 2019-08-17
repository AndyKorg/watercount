/*
 * spi_bitbang.h
 * Довольно медленный spi
 *  61 ms на байт
 *  Последний бит первым
 *  CPOL = 1 CHPA =1 CS = low
 *
 */

#ifndef MAIN_SPI_BITBANG_H_
#define MAIN_SPI_BITBANG_H_

#include <stdint.h>
#include "driver/gpio.h"
#include "esp_err.h"

#define SPI_CS			GPIO_Pin_4
#define SPI_CS_NUM		GPIO_NUM_4
#define SPI_MISO		GPIO_Pin_12
#define SPI_MISO_NUM	GPIO_NUM_12
#define SPI_MOSI		GPIO_Pin_13
#define SPI_MOSI_NUM	GPIO_NUM_13
#define SPI_CLK			GPIO_Pin_14
#define SPI_CLK_NUM		GPIO_NUM_14

typedef esp_err_t (*spi_event_reciv_cb_t)(uint8_t recive_byte); //Принят байт
typedef esp_err_t (*spi_event_send_cb_t)(uint8_t* send_byte);	//Готов к передаче следующего байта
typedef esp_err_t (*spi_event_cs_cb_t)(uint8_t* send_first_byte); //Получен CS, чего передавать
typedef esp_err_t (*spi_event_cs_stop_cb_t)(void); //CS сброшен, передача закончена

esp_err_t spi_init_bitbang(
		spi_event_reciv_cb_t reciv_func,//Принят байт
		spi_event_send_cb_t send_func, 	//Нужен байт для передачи
		spi_event_cs_cb_t cs_func_start,//Мастер сказал - Старт приема
		spi_event_cs_stop_cb_t cs_func_stop	//Мастер сказал - стоп
		);

#endif /* MAIN_SPI_BITBANG_H_ */
