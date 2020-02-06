/*
 * low driver E-Paper
 */
#include <stdint.h>
#include <stdlib.h>
#include "esp_log.h"
#include "esp_err.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "epd1in54.h"

unsigned char epdInit(const unsigned char *lut) {
	if (IfInit(EPD_BYTE_MAX) != 0) {
		return -1;
	}

	epdReset();
	uint8_t tmp[3];
	tmp[0] = EPD_HEIGHT - 1;
	tmp[1] = ((EPD_HEIGHT - 1) >> 8);
	tmp[2] = 0x00;
	lcd_cmd(DRIVER_OUTPUT_CONTROL, tmp, 3);
	tmp[0] = 0xD7;
	tmp[1] = 0xD6;
	tmp[2] = 0x9D;
	lcd_cmd(BOOSTER_SOFT_START_CONTROL, tmp, 3);
	tmp[0] = 0xA8;
	lcd_cmd(WRITE_VCOM_REGISTER, tmp, 1);
	tmp[0] = 0x1A;
	lcd_cmd(SET_DUMMY_LINE_PERIOD, tmp, 1);
	tmp[0] = 0x08;
	lcd_cmd(SET_GATE_LINE_WIDTH, tmp, 1);
	tmp[0] = 0x03;
	lcd_cmd(DATA_ENTRY_MODE_SETTING, tmp, 1);
	epdSetLut(lut);
	return 0;
}

/**
 *  @brief: After this command is transmitted, the chip would enter the 
 *          deep-sleep mode to save power. 
 *          The deep sleep mode would return to standby by hardware reset. 
 *          You can use Epd::Init() to awaken
 */
void epdSleep() {
	//TODO: Check initalise spi interface
	lcd_cmd(DEEP_SLEEP_MODE, NULL, 0);
	epdWaitUntilIdle();
}

/**
 *  @brief: Wait until the busy_pin goes LOW
 */
void epdWaitUntilIdle(void) {
	while (lcd_ready() != ESP_OK) {
		vTaskDelay(20 / portTICK_RATE_MS);
	}
}

/**
 *  @brief: module reset.
 *          often used to awaken the module in deep sleep,
 *          see Sleep();
 */
void epdReset(void) {
	lcd_reset();
}

/**
 *  @brief: set the look-up table register
 */
void epdSetLut(const unsigned char *lut) {
	/* the length of look-up table is 30 bytes */
	lcd_cmd(WRITE_LUT_REGISTER, lut, 30);
}

/**
 *  @brief: put an image buffer to the Pattern memory.
 *          this won't update the display.
 */
void epdSetPatternMemory(const unsigned char *image_buffer, int x, int y, int image_width, int image_height) {
	int x_end;
	int y_end;

	if (image_buffer == NULL || x < 0 || image_width < 0 || y < 0 || image_height < 0) {
		return;
	}
	/* x point must be the multiple of 8 or the last 3 bits will be ignored */
	x &= 0xF8;
	image_width &= 0xF8;
	if (x + image_width >= EPD_WIDTH) {
		x_end = EPD_WIDTH - 1;
	} else {
		x_end = x + image_width - 1;
	}
	if (y + image_height >= EPD_HEIGHT) {
		y_end = EPD_HEIGHT - 1;
	} else {
		y_end = y + image_height - 1;
	}
	epdSetMemoryArea(x, y, x_end, y_end);
	epdSetMemoryPointer(x, y);
	uint8_t *tmp = malloc(EPD_BYTE_MAX); //get maximum memory
	uint8_t *tmp_ptr = tmp;
	if (tmp) {
		/* send the image data */
		for (int j = 0; j < y_end - y + 1; j++) {
			for (int i = 0; i < (x_end - x + 1) / 8; i++) {
				*(tmp_ptr++) = image_buffer[i + j * (image_width / 8)];
			}
		}
		lcd_cmd(WRITE_RAM, tmp, tmp_ptr - tmp);
		free(tmp);
	}
}

/**
 *  @brief: put an image buffer to the Pattern memory.
 *          this won't update the display.
 */
void epdSetPatternMemoryFlash(const unsigned char *image_buffer) {
	epdSetMemoryArea(0, 0, EPD_WIDTH - 1, EPD_HEIGHT - 1);
	epdSetMemoryPointer(0, 0);
	epdSendCommand(WRITE_RAM);
	uint8_t *tmp = malloc(EPD_BYTE_MAX);
	if (tmp) {
		for (int i = 0; i < EPD_BYTE_MAX; i++) {
			*(tmp++) = (uint8_t) *(image_buffer++);
		}
		/* send the image data */
		lcd_cmd(WRITE_RAM, tmp, EPD_BYTE_MAX);
		free(tmp);
	}
}

/**
 *  @brief: clear the Pattern memory with the specified color.
 *          this won't update the display.
 */
void epdClearPatternMemory(unsigned char color) {
	epdSetMemoryArea(0, 0, EPD_WIDTH - 1, EPD_HEIGHT - 1);
	epdSetMemoryPointer(0, 0);
	uint8_t *tmp = malloc(EPD_BYTE_MAX);
	if (tmp) {
		for (int i = 0; i < EPD_BYTE_MAX; i++) {
			*(tmp + i) = color;
		}
		/* send the color data */
		lcd_cmd(WRITE_RAM, tmp, EPD_BYTE_MAX);
		free(tmp);
	} else {
		ESP_LOGE("EPD", "memorey out");
	}
}

/**
 *  @brief: update the display
 *          there are 2 memory areas embedded in the e-paper display
 *          but once this function is called,
 *          the the next action of SetPatternMemory or ClearPattern will
 *          set the other memory area.
 */
void epdDisplayPattern(void) {
	uint8_t tmp[1];
	tmp[0] = 0xC4;
	lcd_cmd(DISPLAY_UPDATE_CONTROL_2, tmp, 1);
	lcd_cmd(MASTER_ACTIVATION, tmp, 0);
	lcd_cmd(NOP, tmp, 0);
	epdWaitUntilIdle();
}

/**
 *  @brief: function to specify the memory area for data R/W
 */
void epdSetMemoryArea(int x_start, int y_start, int x_end, int y_end) {
	uint8_t tmp[4];
	tmp[0] = (x_start >> 3) & 0xFF;
	tmp[1] = (x_end >> 3) & 0xFF;
	lcd_cmd(SET_RAM_X_ADDRESS_START_END_POSITION, tmp, 2);
	/* x point must be the multiple of 8 or the last 3 bits will be ignored */
	tmp[0] = y_start & 0xFF;
	tmp[1] = (y_start >> 8) & 0xFF;
	tmp[2] = y_end & 0xFF;
	tmp[3] = (y_end >> 8) & 0xFF;
	lcd_cmd(SET_RAM_Y_ADDRESS_START_END_POSITION, tmp, 4);
}

/**
 *  @brief: function to specify the start point for data R/W
 */
void epdSetMemoryPointer(int x, int y) {
	/* x point must be the multiple of 8 or the last 3 bits will be ignored */
	uint8_t tmp[2];
	tmp[0] = (x >> 3) & 0xFF;
	lcd_cmd(SET_RAM_X_ADDRESS_COUNTER, tmp, 1);
	tmp[0] = y & 0xFF;
	tmp[1] = (y >> 8) & 0xFF;
	lcd_cmd(SET_RAM_Y_ADDRESS_COUNTER, tmp, 2);
}

DRAM_ATTR const unsigned char lut_full_update[] = { 0x02, 0x02, 0x01, 0x11, 0x12, 0x12, 0x22, 0x22, 0x66, 0x69, 0x69, 0x59, 0x58, 0x99, 0x99, 0x88, 0x00, 0x00,
		0x00, 0x00, 0xF8, 0xB4, 0x13, 0x51, 0x35, 0x51, 0x51, 0x19, 0x01, 0x00 };

DRAM_ATTR const unsigned char lut_partial_update[] = { 0x10, 0x18, 0x18, 0x08, 0x18, 0x18, 0x08, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x13, 0x14, 0x44, 0x12, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };
