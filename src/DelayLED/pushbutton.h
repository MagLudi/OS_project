/**
 * pushbutton.h
 * routines to manipulate the pushbuttons
 *
 * ARM-based K70F120M microcontroller board
 *   for educational purposes only
 * CSCI E-92 Spring 2017, Professor James L. Frankel, Harvard Extension School
 *
 * Written by James L. Frankel (frankel@seas.harvard.edu)
 *
 * Copyright © 2017, 2015, 2014, 2012 James L. Frankel.  All rights reserved.
 */

#ifndef _PUSHBUTTON_H
#define _PUSHBUTTON_H

#define PUSHBUTTON_SW1_PORTD_BIT 0
#define PUSHBUTTON_SW2_PORTE_BIT 26

#ifndef PORT_PCR_MUX_ANALOG
#define PORT_PCR_MUX_ANALOG 0
#endif
#ifndef PORT_PCR_MUX_GPIO
#define PORT_PCR_MUX_GPIO 1
#endif

/* Routine to initialize both of the pushbuttons */
/* Note: This procedure *does* enable the appropriate port clocks */
void pushbuttonInitAll(void);

/* Routine to configure pushbutton SW1 */
/* Note: This procedure does not enable the port clock */
void pushbuttonSW1Config(void);
/* Routine to configure pushbutton SW2 */
/* Note: This procedure does not enable the port clock */
void pushbuttonSW2Config(void);

/* Routine to read state of pushbutton SW1 */
int sw1In(void);
/* Routine to read state of pushbutton SW2 */
int sw2In(void);

#endif /* ifndef _PUSHBUTTON_H */
