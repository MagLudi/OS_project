/*
 * profont.h
 * Font Header File
 *
 * ARM-based K70F120M microcontroller board
 *   for educational purposes only
 * CSCI E-92 Spring 2017, Professor James L. Frankel, Harvard Extension School
 *
 * Written by James L. Frankel (frankel@seas.harvard.edu) & Daniel Willenson
 *
 * Copyright © 2017, 2015, 2014, 2012 James L. Frankel.  All rights reserved.
 */

#ifndef _PROFONT_H
#define _PROFONT_H

#define PROFONT_CHARS_IN_FONT 256
#define PROFONT_FONT_HEIGHT 12
#define PROFONT_FONT_WIDTH 6

extern struct profont_row {
	uint8_t col0 :1;
	uint8_t col1 :1;
	uint8_t col2 :1;
	uint8_t col3 :1;
	uint8_t col4 :1;
	uint8_t col5 :1;
} profont[PROFONT_CHARS_IN_FONT][PROFONT_FONT_HEIGHT];

#endif /* ifndef _PROFONT_H */
