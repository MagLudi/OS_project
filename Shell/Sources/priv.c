/**
 * priv.c
 * routines to manipulate privilege execution state
 *
 * ARM-based K70F120M microcontroller board
 *   for educational purposes only
 * CSCI E-92 Spring 2014, Professor James L. Frankel, Harvard Extension School
 *
 * Written by James L. Frankel (frankel@seas.harvard.edu)
 */

#include "priv.h"

/* Routine to cause the processor to run in unprivileged execution
 * state */

/* See Chapter B1, page B1-615, System Level Programmers’ Model in the
 * ARM®v7-M Architecture Reference Manual, Errata markup, DDI 0403Derrata
 * 2010_Q3 (ID100710) */
void privUnprivileged(void) {
	__asm(
			"mrs r0,CONTROL" "\n"
			"orr r0,r0,#1" "\n"
			"msr CONTROL,r0" "\n"
			"isb sy"
	);
}
