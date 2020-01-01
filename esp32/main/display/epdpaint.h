/*
* high driver E-Paper
*/

#ifndef _EPDPAINT_H
#define _EPDPAINT_H

#define COLORED     1
#define UNCOLORED   0

// Display Orientation
#define ROTATE_0            0
#define ROTATE_90           1
#define ROTATE_180          2
#define ROTATE_270          3

#include "fonts.h"

extern struct sPaint
{
	int width;
	int height;
	int rotate;
	unsigned char* image;
} Paint;

unsigned char epdImageInit(void);//Image init, for hard ini use epdInit(const unsigned char* lut);
void epdClear(int colored);
void epdDrawAbsolutePixel(int x, int y, int colored);
void epdDrawPixel(int x, int y, int colored);
int epdDrawCharAt(int x, int y, char ascii_char, sFONT* font, int colored);
int epdDrawStringAt(int x, int y, const char* text, sFONT* font, int colored);
void epdDrawLine(int x0, int y0, int x1, int y1, int colored);
void epdDrawHorizontalLine(int x, int y, int width, int colored);
void epdDrawVerticalLine(int x, int y, int height, int colored);
void epdDrawRectangle(int x0, int y0, int x1, int y1, int colored);
void epdDrawFilledRectangle(int x0, int y0, int x1, int y1, int colored);
void epdDrawCircle(int x, int y, int radius, int colored);
void epdDrawFilledCircle(int x, int y, int radius, int colored);

#endif

/* END OF FILE */

