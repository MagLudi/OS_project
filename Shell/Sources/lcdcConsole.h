/*
 * lcdcConsole.h
 * LCD Console Header File
 * 
 * ARM-based K70F120M microcontroller board
 *   for educational purposes only
 * CSCI E-92 Spring 2017, Professor James L. Frankel, Harvard Extension School
 *
 * Written by James L. Frankel (frankel@seas.harvard.edu)
 *
 * Copyright © 2017, 2015, 2014, 2012 James L. Frankel.  All rights reserved.
 */

#ifndef _LCDCCONSOLE_H
#define _LCDCCONSOLE_H

#include <stdint.h>

// Upper left corner of screen is coordinate (x: 0, y: 0)

#define LCDCCONSOLE_TAB_STOP_SPACING 8

struct console {
	uint32_t *framebuffer;	// Starting address of frame buffer in memory
	uint16_t width, height;	// Screen width & height (in pixels)
	uint16_t x, y;			// Current console output position x & y (in pixels)
};

void lcdcConsoleInit(struct console *console);
void lcdcConsoleClear(struct console *console);
void lcdcConsoleScroll(struct console *console);
void lcdcConsolePutc(struct console *console, int ch);

#endif /* ifndef _LCDCCONSOLE_H */

