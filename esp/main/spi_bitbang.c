/*
 * spi_bitbang.c
 *
 *  Created on: 10 мая 2019 г.
 *      Author: Administrator
 */

#include "spi_bitbang.h"
#include "esp_log.h"
#include "esp_err.h"

#undef CONFIG_LOG_DEFAULT_LEVEL
#define CONFIG_LOG_DEFAULT_LEVEL   ESP_LOG_INFO

spi_event_reciv_cb_t reciv_cb;
spi_event_send_cb_t send_cb;
spi_event_cs_cb_t cs_cb_start;
spi_event_cs_stop_cb_t cs_cb_stop;

volatile uint8_t countBit = 0;
uint8_t byteOut = 0;

//Interrupt!
static void clk_isr_handler(void *arg){
  static uint8_t byteIn = 0;
  gpio_set_level(SPI_MISO_NUM, 1);

  if (!gpio_get_level(SPI_CS_NUM)){ //Идет прием
    if (gpio_get_level(SPI_CLK_NUM)){ 	//0 -> 1
      byteIn |= gpio_get_level(SPI_MOSI_NUM)?0x80:0;
      countBit++;
      if (countBit == 8){
        if (reciv_cb){		//Прием закончен, вызываем функцию приема
          reciv_cb(byteIn);
        }
        byteOut = byteIn;	//отвечаем тем же если нечем ответить
        if (send_cb){		//и передачи
        	send_cb(&byteOut);
        }
        countBit = 0;
        byteIn = 0;
      }
      byteIn >>= 1;
      byteIn &= 0x7f;
    }
    else{ 				//1 -> 0
      gpio_set_level(SPI_MISO_NUM, byteOut &1);
      byteOut >>= 1;
    }
  }
}

//Interrupt!
static void cs_isr_handler(void *arg){
	if (gpio_get_level(SPI_CS_NUM)){
		if (cs_cb_stop){
			cs_cb_stop();
		}
	}
	else{
		countBit = 0;//Начать прием
		if (cs_cb_start){
			cs_cb_start(&byteOut);
		}
	}
}

esp_err_t spi_init_bitbang(spi_event_reciv_cb_t reciv_func, spi_event_send_cb_t send_func, spi_event_cs_cb_t cs_func_start, spi_event_cs_stop_cb_t cs_func_stop){

  gpio_config_t io_conf;
  io_conf.intr_type = GPIO_INTR_ANYEDGE;//interrupt
  io_conf.mode = GPIO_MODE_INPUT;		//set mode
  io_conf.pin_bit_mask = SPI_CLK;		//bit mask of the pins
  io_conf.pull_down_en = 0;				//pull-down mode
  io_conf.pull_up_en = 1;				//pull-up mode
  gpio_config(&io_conf);

  io_conf.intr_type = GPIO_INTR_ANYEDGE;//interrupt
  io_conf.mode = GPIO_MODE_INPUT;		//set mode
  io_conf.pin_bit_mask = (SPI_CS); 		//bit mask of the pins
  io_conf.pull_down_en = 0;				//pull-down mode
  io_conf.pull_up_en = 1;				//pull-up mode
  gpio_config(&io_conf);

  io_conf.intr_type = GPIO_INTR_DISABLE;//interrupt
  io_conf.mode = GPIO_MODE_INPUT;		//set mode
  io_conf.pin_bit_mask = (SPI_MOSI); 	//bit mask of the pins
  io_conf.pull_down_en = 0;				//pull-down mode
  io_conf.pull_up_en = 1;				//pull-up mode
  gpio_config(&io_conf);

  io_conf.intr_type = GPIO_INTR_DISABLE;//interrupt
  io_conf.mode = GPIO_MODE_OUTPUT;		//set mode
  io_conf.pin_bit_mask = (SPI_MISO); 	//bit mask of the pins
  io_conf.pull_down_en = 0;				//pull-down mode
  io_conf.pull_up_en = 1;				//pull-up mode
  gpio_config(&io_conf);

  if (gpio_isr_handler_add(SPI_CLK_NUM, clk_isr_handler, (void *) SPI_CLK) == ESP_OK){
	  if (gpio_isr_handler_add(SPI_CS_NUM, cs_isr_handler, (void *) SPI_CS) == ESP_OK){
		  reciv_cb = reciv_func;
		  send_cb = send_func;
		  cs_cb_start = cs_func_start;
		  cs_cb_stop = cs_func_stop;
		  return ESP_OK;
	  }
  }
  return ESP_ERR_INVALID_ARG;

}

