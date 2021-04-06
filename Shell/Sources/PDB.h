/**
 * PDB.h
 * definitions for controlling the Programmable Delay Block (PDB)
 * interrupt timer
 * 
 * ARM-based K70F120M microcontroller board
 *   for educational purposes only
 * CSCI E-92 Spring 2017, Professor James L. Frankel, Harvard Extension School
 *
 * Written by James L. Frankel (frankel@seas.harvard.edu)
 * 
 * Copyright © 2017, 2015, 2014, 2012 James L. Frankel.  All rights reserved.
 * 
 * Last revised 5:15 PM 11-Apr-2017
 */

#ifndef _PDB_H
#define _PDB_H

#include <stdint.h>
#include "utl.h"

EXTERN_  int act; //action is done: 1. action not done: 0.

/* In FEI (FLL Engaged Internal) mode and with default settings (DRST_DRS = 00,
 * DMX32 = 0), the MCGOUTCLK (MCG (Multipurpose Clock Generator) clock) is
 * 640*32000 [See K70 Sub-Family Reference Manual, Rev. 2, Section 25.4.1.1,
 * Table 25-22 on page 657 and MCG Control 4 Register (MCG_C4) Section 25.3.4
 * on page 641] */

/* Internal Reference Clock */
#define TIMER_IRC 32000
#define TIMER_FLL_Factor 640
#define TIMER_MCGOUTCLK (TIMER_FLL_Factor*TIMER_IRC)

/* The schematic shows J18 is used to select the clock connected to
 * FTM_CLKIN0, but that this jumper is permanently connected to all
 * three pads (see bottom of the K70 PC board next to J19).  Therefore, on
 * the TWR-K70F120M, FTM_CLKIN0 is connected to a 50MHz clock oscillator
 * as long as J19 is installed to provide power to that oscillator
 * (jumper J19 should already be installed). */
#define TIMER_FTM_CLKIN0 50000000

/* IRQ for PDB0 is 72 */
#define PDB0_IRQ_NUMBER 72
/* Interrupt priority for PDB0 is 7 (range is 0 to 15) */
#define PDB0_INTERRUPT_PRIORITY 4

/* Values for the PDB_SC_PRESCALER field in PDB0_SC */
#define PDB_SC_PRESCALER_DIVIDE_BY_1 0	
#define PDB_SC_PRESCALER_DIVIDE_BY_2 1	
#define PDB_SC_PRESCALER_DIVIDE_BY_4 2	
#define PDB_SC_PRESCALER_DIVIDE_BY_8 3	
#define PDB_SC_PRESCALER_DIVIDE_BY_16 4	
#define PDB_SC_PRESCALER_DIVIDE_BY_32 5	
#define PDB_SC_PRESCALER_DIVIDE_BY_64 6	
#define PDB_SC_PRESCALER_DIVIDE_BY_128 7	

/* Values for the PDB_SC_TRGSEL field in PDB0_SC */
#define PDB_SC_TRGSEL_TRIGGER_IN_0 0
#define PDB_SC_TRGSEL_TRIGGER_IN_1 1
#define PDB_SC_TRGSEL_TRIGGER_IN_2 2
#define PDB_SC_TRGSEL_TRIGGER_IN_3 3
#define PDB_SC_TRGSEL_TRIGGER_IN_4 4
#define PDB_SC_TRGSEL_TRIGGER_IN_5 5
#define PDB_SC_TRGSEL_TRIGGER_IN_6 6
#define PDB_SC_TRGSEL_TRIGGER_IN_7 7
#define PDB_SC_TRGSEL_TRIGGER_IN_8 8
#define PDB_SC_TRGSEL_TRIGGER_IN_9 9
#define PDB_SC_TRGSEL_TRIGGER_IN_10 10
#define PDB_SC_TRGSEL_TRIGGER_IN_11 11
#define PDB_SC_TRGSEL_TRIGGER_IN_12 12
#define PDB_SC_TRGSEL_TRIGGER_IN_13 13
#define PDB_SC_TRGSEL_TRIGGER_IN_14 14
#define PDB_SC_TRGSEL_SOFTWARE_TRIGGER 15

/* Values for the PDB_SC_MULT field in PDB0_SC */
#define PDB_SC_MULT_BY_1 0
#define PDB_SC_MULT_BY_10 1
#define PDB_SC_MULT_BY_20 2
#define PDB_SC_MULT_BY_40 3

#define PDBTimerOneShot 0
#define PDBTimerContinuous 1

/* Function prototypes */
void PDB0Init(uint16_t count, int continuous);

void PDB0Start(void);

void PDB0Stop(void);

void PDB0Isr(void);

void PDB0Action(void);

void setFunc(void (*funcEx_p)());

#endif /* ifndef _PDB_H */

