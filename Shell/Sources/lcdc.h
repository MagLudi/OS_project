/*
 * lcdc.h
 * LCD Controller Driver Header File
 *
 * ARM-based K70F120M microcontroller board
 *   for educational purposes only
 * CSCI E-92 Spring 2014, Professor James L. Frankel, Harvard Extension School
 *
 * Written by James L. Frankel (frankel@seas.harvard.edu)
 */

#ifndef _LCDC_H
#define _LCDC_H

#include "sdram.h"

#define REVE 1

// Screen size
#define LCDC_SCREEN_XSIZE 480
#define LCDC_SCREEN_YSIZE 272

// Graphics window size
#define LCDC_GW_XSIZE 16
#define LCDC_GW_YSIZE 16

// In 24 bits per pixel mode, the upper byte of each word is ignored
#define LCDC_BYTES_PER_PIXEL 4

// Accessors for R (Red), G (Green), and B (Blue) bytes in 24 bits per pixel mode
#define LCDC_RED_MASK 0x00ff0000
#define LCDC_GREEN_MASK 0x0000ff00
#define LCDC_BLUE_MASK 0x000000ff
#define LCDC_RED_SHIFT 16
#define LCDC_GREEN_SHIFT 8
#define LCDC_BLUE_SHIFT 0
// LCDC_PIXEL_RED returns the red byte of a 24 bit per pixel word
#define LCDC_PIXEL_RED(pixel) ((pixel & LCDC_RED_MASK) >> LCDC_RED_SHIFT)
// LCDC_PIXEL_GREEN returns the green byte of a 24 bit per pixel word
#define LCDC_PIXEL_GREEN(pixel) ((pixel & LCDC_GREEN_MASK) >> LCDC_GREEN_SHIFT)
// LCDC_PIXEL_BLUE returns the blue byte of a 24 bit per pixel word
#define LCDC_PIXEL_BLUE(pixel) ((pixel & LCDC_BLUE_MASK) >> LCDC_BLUE_SHIFT)

// White:
//#define LCDC_SCREEN_FOREGROUND_COLOR 0x00ffffff
// Black:
//#define LCDC_SCREEN_BACKGROUND_COLOR 0

// Black:
#define LCDC_SCREEN_FOREGROUND_COLOR 0
// Pretty blue:
#define LCDC_SCREEN_BACKGROUND_COLOR 0x0094b0f2

#define LCDC_FRAME_START_ADDRESS SDRAM_START
#define LCDC_FRAME_BUFFER_SIZE (LCDC_SCREEN_XSIZE * LCDC_SCREEN_YSIZE * LCDC_BYTES_PER_PIXEL)

#define LCDC_GW_START_ADDRESS (LCDC_FRAME_START_ADDRESS + LCDC_FRAME_BUFFER_SIZE)
#define LCDC_GW_BUFFER_SIZE (LCDC_GW_XSIZE * LCDC_GW_YSIZE * LCDC_BYTES_PER_PIXEL)

/* Pin multiplexer (mux) control & drive strength enable constants */
#define LCDC_ALT2 (PORT_PCR_MUX(2) | PORT_PCR_DSE_MASK)
#define LCDC_ALT5 (PORT_PCR_MUX(5) | PORT_PCR_DSE_MASK)
#define LCDC_ALT7 (PORT_PCR_MUX(7) | PORT_PCR_DSE_MASK)

/* Init Functions */
void lcdcInit();
void lcdcInitScreen();
void lcdcInitPins();

#endif /* ifndef _LCDC_H */

