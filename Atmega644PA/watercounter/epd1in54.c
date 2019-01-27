/*
 * low driver E-Paper
 */ 
#include <stdlib.h>
#include "util/delay.h"
#include "avr/pgmspace.h"
#include "epd1in54.h"

unsigned char epdInit(const unsigned char* lut)
{
    if (IfInit() != 0)
    {
        return -1;
    }

	epdReset();
    epdSendCommand(DRIVER_OUTPUT_CONTROL);
    epdSendData((EPD_HEIGHT - 1));
    epdSendData((EPD_HEIGHT - 1) >> 8);
    epdSendData(0x00);
    epdSendCommand(BOOSTER_SOFT_START_CONTROL);
    epdSendData(0xD7);
    epdSendData(0xD6);
    epdSendData(0x9D);
    epdSendCommand(WRITE_VCOM_REGISTER);
    epdSendData(0xA8);
    epdSendCommand(SET_DUMMY_LINE_PERIOD);
    epdSendData(0x1A);
    epdSendCommand(SET_GATE_LINE_WIDTH);
    epdSendData(0x08);
    epdSendCommand(DATA_ENTRY_MODE_SETTING);
    epdSendData(0x03);
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
    epdSendCommand(DEEP_SLEEP_MODE);
    epdWaitUntilIdle();
}

/**
 *  @brief: Wait until the busy_pin goes LOW
 */
void epdWaitUntilIdle(void)
{
    while(BitIsSet(EPD_PORT_IN, EPD_BUSY_PIN)) {      //LOW: idle, HIGH: busy
		//_delay_ms(100);
    }      
}

/**
 *  @brief: module reset.
 *          often used to awaken the module in deep sleep,
 *          see Sleep();
 */
void epdReset(void)
{
	ClearBit(EPD_PORT_OUT, EPD_RST_PIN);
	_delay_ms(200);
	SetBit(EPD_PORT_OUT, EPD_RST_PIN); //module reset    
	_delay_ms(200);
}

/**
 *  @brief: set the look-up table register
 */
void epdSetLut(const unsigned char* lut)
{
    epdSendCommand(WRITE_LUT_REGISTER);
    /* the length of look-up table is 30 bytes */
    for (int i = 0; i < 30; i++) {
        epdSendData(lut[i]);
    }
}

/**
 *  @brief: put an image buffer to the Pattern memory.
 *          this won't update the display.
 */
void epdSetPatternMemory(
    const unsigned char* image_buffer,
    int x, int y,
    int image_width, int image_height)
{
    int x_end;
    int y_end;

    if (
        image_buffer == NULL ||
        x < 0 || image_width < 0 ||
        y < 0 || image_height < 0
    ) {
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
    epdSendCommand(WRITE_RAM);
    /* send the image data */
    for (int j = 0; j < y_end - y + 1; j++) {
        for (int i = 0; i < (x_end - x + 1) / 8; i++) {
            epdSendData(image_buffer[i + j * (image_width / 8)]);
        }
    }
}

/**
 *  @brief: put an image buffer to the Pattern memory.
 *          this won't update the display.
 *
 *          Question: When do you use this function instead of 
 *          void SetPatternMemory(
 *              const unsigned char* image_buffer,
 *              int x,  int y,
 *              int image_width, int image_height );
 *          Answer: SetPatternMemory with parameters only reads image data
 *          from the RAM but not from the flash in AVR chips (for AVR chips,
 *          you have to use the function pgm_read_byte to read buffers 
 *          from the flash).
 */
void epdSetPatternMemoryFlash(const unsigned char* image_buffer)
{
    epdSetMemoryArea(0, 0, EPD_WIDTH - 1, EPD_HEIGHT - 1);
    epdSetMemoryPointer(0, 0);
    epdSendCommand(WRITE_RAM);
    /* send the image data */
    for (int i = 0; i < EPD_WIDTH / 8 * EPD_HEIGHT; i++)
    {
        epdSendData(~(pgm_read_byte(&image_buffer[i])));
    }
}

/**
 *  @brief: clear the Pattern memory with the specified color.
 *          this won't update the display.
 */
void epdClearPatternMemory(unsigned char color)
{
    epdSetMemoryArea(0, 0, EPD_WIDTH - 1, EPD_HEIGHT - 1);
    epdSetMemoryPointer(0, 0);
    epdSendCommand(WRITE_RAM);
    /* send the color data */
    for (int i = 0; i < EPD_WIDTH / 8 * EPD_HEIGHT; i++)
    {
        epdSendData(color);
    }
}

/**
 *  @brief: update the display
 *          there are 2 memory areas embedded in the e-paper display
 *          but once this function is called,
 *          the the next action of SetPatternMemory or ClearPattern will 
 *          set the other memory area.
 */
void epdDisplayPattern(void)
{
    epdSendCommand(DISPLAY_UPDATE_CONTROL_2);
    epdSendData(0xC4);
    epdSendCommand(MASTER_ACTIVATION);
    epdSendCommand(NOP);
    epdWaitUntilIdle();
}

/**
 *  @brief: function to specify the memory area for data R/W
 */
void epdSetMemoryArea(int x_start, int y_start,
                        int x_end, int y_end)
{
    epdSendCommand(SET_RAM_X_ADDRESS_START_END_POSITION);
    /* x point must be the multiple of 8 or the last 3 bits will be ignored */
    epdSendData((x_start >> 3) & 0xFF);
    epdSendData((x_end >> 3) & 0xFF);
    epdSendCommand(SET_RAM_Y_ADDRESS_START_END_POSITION);
    epdSendData(y_start & 0xFF);
    epdSendData((y_start >> 8) & 0xFF);
    epdSendData(y_end & 0xFF);
    epdSendData((y_end >> 8) & 0xFF);
}

/**
 *  @brief: function to specify the start point for data R/W
 */
void epdSetMemoryPointer(int x, int y)
{
    epdSendCommand(SET_RAM_X_ADDRESS_COUNTER);
    /* x point must be the multiple of 8 or the last 3 bits will be ignored */
    epdSendData((x >> 3) & 0xFF);
    epdSendCommand(SET_RAM_Y_ADDRESS_COUNTER);
    epdSendData(y & 0xFF);
    epdSendData((y >> 8) & 0xFF);
    epdWaitUntilIdle();
}


/**
 *  @brief: basic function for sending commands
 */
void epdSendCommand(unsigned char command)
{
	epdWaitUntilIdle();
    commandModeOn();
	//_delay_us(300);
    Transfer(command);
}

/**
 *  @brief: basic function for sending data
 */
void epdSendData(unsigned char data)
{
	epdWaitUntilIdle();
	dataModeOn();
	//_delay_us(300);
    Transfer(data);
}
/*
 * 
 */
const unsigned char lut_full_update[] =
{
    0x02, 0x02, 0x01, 0x11, 0x12, 0x12, 0x22, 0x22, 
    0x66, 0x69, 0x69, 0x59, 0x58, 0x99, 0x99, 0x88, 
    0x00, 0x00, 0x00, 0x00, 0xF8, 0xB4, 0x13, 0x51, 
    0x35, 0x51, 0x51, 0x19, 0x01, 0x00
};

const unsigned char lut_partial_update[] =
{
    0x10, 0x18, 0x18, 0x08, 0x18, 0x18, 0x08, 0x00, 
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
    0x00, 0x00, 0x00, 0x00, 0x13, 0x14, 0x44, 0x12, 
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00
};
