/**
 * delay.h
 * routines to delay via looping
 *
 * ARM-based K70F120M microcontroller board
 *   for educational purposes only
 * CSCI E-92 Spring 2017, Professor James L. Frankel, Harvard Extension School
 *
 * Written by James L. Frankel (frankel@seas.harvard.edu)
 *
 * Copyright © 2017, 2015, 2014, 2012 James L. Frankel.  All rights reserved.
 */

#ifndef _DELAY_H
#define _DELAY_H

/* Routine to delay for user specified interval */
void delay(unsigned long int limit);

/* Assembler routine to delay for user specified interval
 * 
 * With 120 MHz clock, each loop is 4 cycles * (1/120,000,000) seconds cycle
 * time.  So, each loop is .0000000333 seconds = 33.333 nanoseconds. */
void asmDelay(unsigned long int limit);

#endif /* ifndef _DELAY_H */

