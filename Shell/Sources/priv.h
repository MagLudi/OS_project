/**
 * priv.h
 * routines to manipulate privilege execution state
 *
 * ARM-based K70F120M microcontroller board
 *   for educational purposes only
 * CSCI E-92 Spring 2014, Professor James L. Frankel, Harvard Extension School
 *
 * Written by James L. Frankel (frankel@seas.harvard.edu)
 */

#ifndef _PRIV_H
#define _PRIV_H

/* Function prototypes */

/* Routine to cause the processor to run in unprivileged execution
 * state */
void privUnprivileged(void);

#endif /* ifndef _PRIV_H */
