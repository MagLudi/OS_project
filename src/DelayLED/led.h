/**
 * led.h
 * routines to manipulate the LEDs
 *
 * ARM-based K70F120M microcontroller board
 *   for educational purposes only
 * CSCI E-92 Spring 2017, Professor James L. Frankel, Harvard Extension School
 *
 * Written by James L. Frankel (frankel@seas.harvard.edu)
 *
 * Copyright © 2017, 2015, 2014, 2012 James L. Frankel.  All rights reserved.
 */

#ifndef _LED_H
#define _LED_H

#define LED_ORANGE_PORTA_BIT 11
#define LED_YELLOW_PORTA_BIT 28
#define LED_GREEN_PORTA_BIT 29
#define LED_BLUE_PORTA_BIT 10

#ifndef PORT_PCR_MUX_ANALOG
#define PORT_PCR_MUX_ANALOG 0
#endif
#ifndef PORT_PCR_MUX_GPIO
#define PORT_PCR_MUX_GPIO 1
#endif

/* Routine to initialize all of the LEDs */
/* Note: This procedure *does* enable the appropriate port clock */
void ledInitAll(void);

/* Routine to configure the orange LED */
/* Note: This procedure does not enable the port clock */
void ledOrangeConfig(void);
/* Routine to configure the yellow LED */
/* Note: This procedure does not enable the port clock */
void ledYellowConfig(void);
/* Routine to configure the green LED */
/* Note: This procedure does not enable the port clock */
void ledGreenConfig(void);
/* Routine to configure the blue LED */
/* Note: This procedure does not enable the port clock */
void ledBlueConfig(void);

/* Routine to turn off the orange LED */
void ledOrangeOff(void);
/* Routine to turn off the yellow LED */
void ledYellowOff(void);
/* Routine to turn off the green LED */
void ledGreenOff(void);
/* Routine to turn off the blue LED */
void ledBlueOff(void);

/* Routine to turn on the orange LED */
void ledOrangeOn(void);
/* Routine to turn on the yellow LED */
void ledYellowOn(void);
/* Routine to turn on the green LED */
void ledGreenOn(void);
/* Routine to turn on the blue LED */
void ledBlueOn(void);

#endif /* ifndef _LED_H */
