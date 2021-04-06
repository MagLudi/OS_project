/*
 * lcdcConsole.c
 * LCD Console Implementation
 *
 * ARM-based K70F120M microcontroller board
 *   for educational purposes only
 * CSCI E-92 Spring 2017, Professor James L. Frankel, Harvard Extension School
 *
 * Written by James L. Frankel (frankel@seas.harvard.edu) & Daniel Willenson
 *
 * Copyright © 2017, 2015, 2014, 2012 James L. Frankel.  All rights reserved.
 */

#include <stdint.h>
#include <string.h>
#include "lcdc.h"
#include "profont.h"
#include "lcdcConsole.h"

void lcdcConsoleInit(struct console *console) {
	console->framebuffer = (uint32_t *) LCDC_FRAME_START_ADDRESS;
	console->width = LCDC_SCREEN_XSIZE;
	console->height = LCDC_SCREEN_YSIZE;
	console->x = 0;
	console->y = LCDC_SCREEN_YSIZE - PROFONT_FONT_HEIGHT;

	lcdcConsoleClear(console);
}

void lcdcConsoleClear(struct console *console) {
	uint8_t byte;
	uint32_t *p;

	// Initialize the frame buffer to the background color
	//	If the red, green, and blue bytes all are the same value, use memset to initialize the background color because
	//	it is more efficient
	byte = LCDC_PIXEL_RED(LCDC_SCREEN_BACKGROUND_COLOR);
	if ((byte == LCDC_PIXEL_GREEN(LCDC_SCREEN_BACKGROUND_COLOR))
			&& (byte == LCDC_PIXEL_BLUE(LCDC_SCREEN_BACKGROUND_COLOR))) {
		memset(console->framebuffer, byte,
				LCDC_BYTES_PER_PIXEL * console->width * console->height);
	} else {
		for (p = console->framebuffer;
				p < console->framebuffer + console->width * console->height;
				p++) {
			*p = LCDC_SCREEN_BACKGROUND_COLOR;
		}
	}
}

void lcdcConsoleScroll(struct console *console) {
	uint8_t byte;
	uint32_t *p;

	// Scroll up all of the screen except for the bottom text line
	memmove(&console->framebuffer[0],
			&console->framebuffer[PROFONT_FONT_HEIGHT * console->width],
			LCDC_BYTES_PER_PIXEL * console->y * console->width);

	// Clear the bottom text line to the background color
	//	If the red, green, and blue bytes all are the same value, use memset to initialize the background color because
	//	it is more efficient
	byte = LCDC_PIXEL_RED(LCDC_SCREEN_BACKGROUND_COLOR);
	if ((byte == LCDC_PIXEL_GREEN(LCDC_SCREEN_BACKGROUND_COLOR))
			&& (byte == LCDC_PIXEL_BLUE(LCDC_SCREEN_BACKGROUND_COLOR))) {
		memset(&console->framebuffer[console->y * console->width], byte,
		LCDC_BYTES_PER_PIXEL * (console->height - console->y) * console->width);
	} else {
		for (p = &console->framebuffer[console->y * console->width];
				p < (console->framebuffer + console->height * console->width);
				p++) {
			*p = LCDC_SCREEN_BACKGROUND_COLOR;
		}
	}
}

void lcdcConsolePutc(struct console *console, int ch) {
	int x, y;

	switch (ch) {
	case '\r': // carriage return
		console->x = 0;
		break;
	case '\n': // new line (linefeed)
		lcdcConsoleScroll(console);
		break;
	case '\t': // horizontal tab
		console->x = ((console->x
				+ LCDCCONSOLE_TAB_STOP_SPACING * PROFONT_FONT_WIDTH)
				/ (LCDCCONSOLE_TAB_STOP_SPACING * PROFONT_FONT_WIDTH))
				* (LCDCCONSOLE_TAB_STOP_SPACING * PROFONT_FONT_WIDTH);
		break;
	case '\b': // backspace
		if (console->x >= PROFONT_FONT_WIDTH) {
			console->x -= PROFONT_FONT_WIDTH;
		}
		break;
	case '\f': // formfeed
		lcdcConsoleClear(console);
		console->x = 0;
		console->y = LCDC_SCREEN_YSIZE - PROFONT_FONT_HEIGHT;
		break;
	default:
		for (y = 0; y < PROFONT_FONT_HEIGHT; y++) {
			console->framebuffer[(y + console->y) * console->width
					+ (0 + console->x)] =
					profont[ch][y].col0 ?
							LCDC_SCREEN_FOREGROUND_COLOR :
							LCDC_SCREEN_BACKGROUND_COLOR;
			console->framebuffer[(y + console->y) * console->width
					+ (1 + console->x)] =
					profont[ch][y].col1 ?
							LCDC_SCREEN_FOREGROUND_COLOR :
							LCDC_SCREEN_BACKGROUND_COLOR;
			console->framebuffer[(y + console->y) * console->width
					+ (2 + console->x)] =
					profont[ch][y].col2 ?
							LCDC_SCREEN_FOREGROUND_COLOR :
							LCDC_SCREEN_BACKGROUND_COLOR;
			console->framebuffer[(y + console->y) * console->width
					+ (3 + console->x)] =
					profont[ch][y].col3 ?
							LCDC_SCREEN_FOREGROUND_COLOR :
							LCDC_SCREEN_BACKGROUND_COLOR;
			console->framebuffer[(y + console->y) * console->width
					+ (4 + console->x)] =
					profont[ch][y].col4 ?
							LCDC_SCREEN_FOREGROUND_COLOR :
							LCDC_SCREEN_BACKGROUND_COLOR;
			console->framebuffer[(y + console->y) * console->width
					+ (5 + console->x)] =
					profont[ch][y].col5 ?
							LCDC_SCREEN_FOREGROUND_COLOR :
							LCDC_SCREEN_BACKGROUND_COLOR;
		}
		console->x += PROFONT_FONT_WIDTH;
		break;
	}

	if (console->x > console->width - PROFONT_FONT_WIDTH) {
		lcdcConsoleScroll(console);
		console->x = 0;
	}
}
