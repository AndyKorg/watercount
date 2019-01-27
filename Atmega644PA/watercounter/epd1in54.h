/*
 * low driver E-Paper
 */ 
#ifndef _EPD1IN54_H
#define _EPD1IN54_H

#include "epdif.h"

// Display resolution
#define EPD_WIDTH       200
#define EPD_HEIGHT      200

// EPD 1.54 Inch commands
#define DRIVER_OUTPUT_CONTROL                       0x01
#define BOOSTER_SOFT_START_CONTROL                  0x0C
#define GATE_SCAN_START_POSITION                    0x0F
#define DEEP_SLEEP_MODE                             0x10
#define DATA_ENTRY_MODE_SETTING                     0x11
#define SW_RESET                                    0x12
#define TEMPERATURE_SENSOR_CONTROL                  0x1A
#define MASTER_ACTIVATION                           0x20
#define DISPLAY_UPDATE_CONTROL_1                    0x21
#define DISPLAY_UPDATE_CONTROL_2                    0x22
#define WRITE_RAM                                   0x24
#define WRITE_VCOM_REGISTER                         0x2C
#define WRITE_LUT_REGISTER                          0x32
#define SET_DUMMY_LINE_PERIOD                       0x3A
#define SET_GATE_LINE_WIDTH                         0x3B
#define BORDER_WAVEFORM_CONTROL                     0x3C
#define SET_RAM_X_ADDRESS_START_END_POSITION        0x44
#define SET_RAM_Y_ADDRESS_START_END_POSITION        0x45
#define SET_RAM_X_ADDRESS_COUNTER                   0x4E
#define SET_RAM_Y_ADDRESS_COUNTER                   0x4F
#define NOP                                         0xFF

extern const unsigned char lut_full_update[];
extern const unsigned char lut_partial_update[];

unsigned char epdInit(const unsigned char* lut); //Этим инициализировать!
void epdSendCommand(unsigned char command);
void epdSendData(unsigned char data);
void epdWaitUntilIdle(void);
void epdReset(void);
void epdSetPatternMemory(
    const unsigned char* image_buffer,
    int x,
    int y,
    int image_width,
    int image_height
    );
void epdSetPatternMemoryFlash(const unsigned char* image_buffer);
void epdClearPatternMemory(unsigned char color);
void epdDisplayPattern(void);
void epdSleep(void);

void epdSetLut(const unsigned char* lut);
void epdSetMemoryArea(int x_start, int y_start, int x_end, int y_end);
void epdSetMemoryPointer(int x, int y);

#endif
