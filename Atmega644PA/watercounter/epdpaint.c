/*
* high driver E-Paper
*/
#include <avr/pgmspace.h>
#include <stdlib.h>
#include <stddef.h>
#include "epdpaint.h"

struct sPaint Paint;

inline int getWidth(int width){
	return width % 8 ? (width + 8 - (width % 8)) : width;
}

/**
*  @brief: this draws a pixel by absolute coordinates.
*          this function won't be affected by the rotate parameter.
*/
void epdDrawAbsolutePixel(int x, int y, int colored) {
	int width = getWidth(Paint.width);
	if (x < 0 || x >= width || y < 0 || y >= Paint.height) {
		return;
	}
	if (colored) {
		Paint.image[(x + y * width) / 8] &= ~(0x80 >> (x % 8));
		} else {
		Paint.image[(x + y * width) / 8] |= 0x80 >> (x % 8);
	}
}

/**
*  @brief: clear the image
*/
void epdClear(int colored) {
	int width = getWidth(Paint.width);
	for (int x = 0; x < width; x++) {
		for (int y = 0; y < Paint.height; y++) {
			epdDrawAbsolutePixel(x, y, colored);
		}
	}
}

/**
*  @brief: this draws a pixel by the coordinates
*  
*/
void epdDrawPixel(int x, int y, int colored) {
	int point_temp;
	
	int width = getWidth(Paint.width);
	
	if (Paint.rotate == ROTATE_0) {
		if(x < 0 || x >= width || y < 0 || y >= Paint.height) {
			return;
		}
		epdDrawAbsolutePixel(x, y, colored);
		} else if (Paint.rotate == ROTATE_90) {
		if(x < 0 || x >= Paint.height || y < 0 || y >= width) {
			return;
		}
		point_temp = x;
		x = width - y;
		y = point_temp;
		epdDrawAbsolutePixel(x, y, colored);
		} else if (Paint.rotate == ROTATE_180) {
		if(x < 0 || x >= width || y < 0 || y >= Paint.height) {
			return;
		}
		x = width - x;
		y = Paint.height - y;
		epdDrawAbsolutePixel(x, y, colored);
		} else if (Paint.rotate == ROTATE_270) {
		if(x < 0 || x >= Paint.height || y < 0 || y >= width) {
			return;
		}
		point_temp = x;
		x = y;
		y = Paint.height - point_temp;
		epdDrawAbsolutePixel(x, y, colored);
	}
}

/**
*  @brief: this draws a character on the pattern buffer but not refresh
*          returns the x position of the end character
*/
int epdDrawCharAt(int x, int y, char ascii_char, sFONT* font, int colored) {
	int i, j;
	unsigned int char_offset = 0;
	sPROP_SYMBOL symbol;

	#define NO_SYMBOL			0xff
	#define OffsetCalc(w)		(font->Height * (w / 8 + (w % 8 ? 1 : 0)))

	//get symbol and offset if table not full ascii table
	if (font->tableSymbolSize){
		for (i=0; i<= font->tableSymbolSize; i++){
			memcpy_P(&symbol, &(font->tableSymbol[i]), sizeof(sPROP_SYMBOL));
			symbol.Width = (font->FontType == monospaced)?font->Width:symbol.Width; //proportional font may be
			if(ascii_char == symbol.Symbol){
				break;
			}
			char_offset += OffsetCalc(symbol.Width);
		}
		if(ascii_char != symbol.Symbol){
			char_offset = 0;
		}
	}
	else{
		char_offset = (ascii_char-' ') * OffsetCalc(font->Width);//Only fixed font
		symbol.Width = font->Width;
	}
	
	const unsigned char* ptr = &font->table[char_offset];

	for (j = 0; j < font->Height; j++) {
		for (i = 0; i < symbol.Width; i++) {
			if (pgm_read_byte(ptr) & (0x80 >> (i % 8))) {
				epdDrawPixel(x + i, y + j, colored);
			}
			if (i % 8 == 7) {
				ptr++;
			}
		}
		if (symbol.Width % 8 != 0) {
			ptr++;
		}
	}

	return x+symbol.Width;
}

/**
*  @brief: this displays a string on the pattern buffer but not refresh
*          returns the x position of the end string
*/
int epdDrawStringAt(int x, int y, const char* text, sFONT* font, int colored) {
	const char* p_text = text;
	unsigned int counter = 0;
	int refcolumn = x;
	
	/* Send the string character by character on EPD */
	while (*p_text != 0) {
		/* Display one character on EPD */
		refcolumn = epdDrawCharAt(refcolumn, y, *p_text, font, colored);
		/* Decrement the column position by 16 */
		//refcolumn += font->Width;
		/* Point on the next character */
		p_text++;
		counter++;
	}
	return refcolumn;
}

/**
*  @brief: this draws a line on the pattern buffer
*/
void epdDrawLine(int x0, int y0, int x1, int y1, int colored) {
	/* Bresenham algorithm */
	int dx = x1 - x0 >= 0 ? x1 - x0 : x0 - x1;
	int sx = x0 < x1 ? 1 : -1;
	int dy = y1 - y0 <= 0 ? y1 - y0 : y0 - y1;
	int sy = y0 < y1 ? 1 : -1;
	int err = dx + dy;

	while((x0 != x1) && (y0 != y1)) {
		epdDrawPixel(x0, y0 , colored);
		if (2 * err >= dy) {
			err += dy;
			x0 += sx;
		}
		if (2 * err <= dx) {
			err += dx;
			y0 += sy;
		}
	}
}

/**
*  @brief: this draws a horizontal line on the pattern buffer
*/
void epdDrawHorizontalLine(int x, int y, int line_width, int colored) {
	int i;
	for (i = x; i < x + line_width; i++) {
		epdDrawPixel(i, y, colored);
	}
}

/**
*  @brief: this draws a vertical line on the pattern buffer
*/
void epdDrawVerticalLine(int x, int y, int line_height, int colored) {
	int i;
	for (i = y; i < y + line_height; i++) {
		epdDrawPixel(x, i, colored);
	}
}

/**
*  @brief: this draws a rectangle
*/
void epdDrawRectangle(int x0, int y0, int x1, int y1, int colored) {
	int min_x, min_y, max_x, max_y;
	min_x = x1 > x0 ? x0 : x1;
	max_x = x1 > x0 ? x1 : x0;
	min_y = y1 > y0 ? y0 : y1;
	max_y = y1 > y0 ? y1 : y0;
	
	epdDrawHorizontalLine(min_x, min_y, max_x - min_x + 1, colored);
	epdDrawHorizontalLine(min_x, max_y, max_x - min_x + 1, colored);
	epdDrawVerticalLine(min_x, min_y, max_y - min_y + 1, colored);
	epdDrawVerticalLine(max_x, min_y, max_y - min_y + 1, colored);
}

/**
*  @brief: this draws a filled rectangle
*/
void epdDrawFilledRectangle(int x0, int y0, int x1, int y1, int colored) {
	int min_x, min_y, max_x, max_y;
	int i;
	min_x = x1 > x0 ? x0 : x1;
	max_x = x1 > x0 ? x1 : x0;
	min_y = y1 > y0 ? y0 : y1;
	max_y = y1 > y0 ? y1 : y0;
	
	for (i = min_x; i <= max_x; i++) {
		epdDrawVerticalLine(i, min_y, max_y - min_y + 1, colored);
	}
}

/**
*  @brief: this draws a circle
*/
void epdDrawCircle(int x, int y, int radius, int colored) {
	/* Bresenham algorithm */
	int x_pos = -radius;
	int y_pos = 0;
	int err = 2 - 2 * radius;
	int e2;

	do {
		epdDrawPixel(x - x_pos, y + y_pos, colored);
		epdDrawPixel(x + x_pos, y + y_pos, colored);
		epdDrawPixel(x + x_pos, y - y_pos, colored);
		epdDrawPixel(x - x_pos, y - y_pos, colored);
		e2 = err;
		if (e2 <= y_pos) {
			err += ++y_pos * 2 + 1;
			if(-x_pos == y_pos && e2 <= x_pos) {
				e2 = 0;
			}
		}
		if (e2 > x_pos) {
			err += ++x_pos * 2 + 1;
		}
	} while (x_pos <= 0);
}

/**
*  @brief: this draws a filled circle
*/
void epdDrawFilledCircle(int x, int y, int radius, int colored) {
	/* Bresenham algorithm */
	int x_pos = -radius;
	int y_pos = 0;
	int err = 2 - 2 * radius;
	int e2;

	do {
		epdDrawPixel(x - x_pos, y + y_pos, colored);
		epdDrawPixel(x + x_pos, y + y_pos, colored);
		epdDrawPixel(x + x_pos, y - y_pos, colored);
		epdDrawPixel(x - x_pos, y - y_pos, colored);
		epdDrawHorizontalLine(x + x_pos, y + y_pos, 2 * (-x_pos) + 1, colored);
		epdDrawHorizontalLine(x + x_pos, y - y_pos, 2 * (-x_pos) + 1, colored);
		e2 = err;
		if (e2 <= y_pos) {
			err += ++y_pos * 2 + 1;
			if(-x_pos == y_pos && e2 <= x_pos) {
				e2 = 0;
			}
		}
		if(e2 > x_pos) {
			err += ++x_pos * 2 + 1;
		}
	} while(x_pos <= 0);
}

/**
*  @brief: Init memory image
*/
unsigned char epdImageInit(void){
	if (!(Paint.image == NULL)){
		free(Paint.image);
	}
	Paint.image = malloc((getWidth(Paint.width)/8) * Paint.height);
	if (Paint.image == NULL){
		return EXIT_FAILURE;
	}
	return EXIT_SUCCESS;
}

