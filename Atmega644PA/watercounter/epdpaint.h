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
//Ёто только частична€ инициализаци€! ѕолна лежит в epdInit(const unsigned char* lut);
unsigned char epdImageInit(void);
void epdClear(int colored);
void epdDrawAbsolutePixel(int x, int y, int colored);
void epdDrawPixel(int x, int y, int colored);
void epdDrawCharAt(int x, int y, char ascii_char, sFONT* font, int colored);
void epdDrawStringAt(int x, int y, const char* text, sFONT* font, int colored);
void epdDrawLine(int x0, int y0, int x1, int y1, int colored);
void epdDrawHorizontalLine(int x, int y, int width, int colored);
void epdDrawVerticalLine(int x, int y, int height, int colored);
void epdDrawRectangle(int x0, int y0, int x1, int y1, int colored);
void epdDrawFilledRectangle(int x0, int y0, int x1, int y1, int colored);
void epdDrawCircle(int x, int y, int radius, int colored);
void epdDrawFilledCircle(int x, int y, int radius, int colored);

/*
class Paint {
public:
Paint(unsigned char* image, int width, int height);
~Paint();
void Clear(int colored);
int  GetWidth(void);
void SetWidth(int width);
int  GetHeight(void);
void SetHeight(int height);
int  GetRotate(void);
void SetRotate(int rotate);
unsigned char* GetImage(void);
void DrawAbsolutePixel(int x, int y, int colored);
void DrawPixel(int x, int y, int colored);
void DrawCharAt(int x, int y, char ascii_char, sFONT* font, int colored);
void DrawStringAt(int x, int y, const char* text, sFONT* font, int colored);
void DrawLine(int x0, int y0, int x1, int y1, int colored);
void DrawHorizontalLine(int x, int y, int width, int colored);
void DrawVerticalLine(int x, int y, int height, int colored);
void DrawRectangle(int x0, int y0, int x1, int y1, int colored);
void DrawFilledRectangle(int x0, int y0, int x1, int y1, int colored);
void DrawCircle(int x, int y, int radius, int colored);
void DrawFilledCircle(int x, int y, int radius, int colored);

private:
unsigned char* image;
int width;
int height;
int rotate;
};
*/
#endif

/* END OF FILE */

