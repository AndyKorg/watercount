#include <string.h>
#include "esp_system.h"
#include "freertos/FreeRTOS.h"
#include "rom/ets_sys.h"
#include "esp_log.h"
#include "nvs_flash.h"

#include "driver/uart.h"

#include "wifi.h"
#include "cayenne.h"
#include "command.h"
#include "spi_bitbang.h"

#undef __ESP_FILE__
#define __ESP_FILE__	NULL

void app_main()
{
    //Initialize NVS
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES) {
      ESP_ERROR_CHECK(nvs_flash_erase());
      ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    rtc_clk_cpu_freq_set(RTC_CPU_FREQ_160M);

    if (gpio_install_isr_service(0) == ESP_OK){//Перед настройкой прерываний обязательно запустить сервис
      spi_init_bitbang(spi_recive_command, spi_answer, spi_start_command, spi_stop_command);

      cmd_init();

      wifi_init_param();
      Cayenne_Init();

      CHIP_READY();
    }

}
