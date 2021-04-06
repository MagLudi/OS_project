/**
 * flexTimer.h
 * definitions for controlling FlexTimer interrupt timer
 * 
 * ARM-based K70F120M microcontroller board
 *   for educational purposes only
 * CSCI E-92 Fall 2014, Professor James L. Frankel, Harvard Extension School
 *
 * Written by James L. Frankel (frankel@seas.harvard.edu)
 * 
 * Last revised 2:21 PM 22-Apr-2014
 */

#ifndef _FLEXTIMER_H
#define _FLEXTIMER_H

#include <stdint.h>
#include <utl.h>

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

/* IRQ for FTM0 is 62 */
#define FTM0_IRQ_NUMBER 62
/* Interrupt priority for FTM0 is 7 (range is 0 to 15) */
#define FTM0_INTERRUPT_PRIORITY 5

/* Values for the FTM_SC_CLKS field in FTM0_SC */
#define FTM_SC_CLKS_NO_CLOCK 0
#define FTM_SC_CLKS_SYSTEM_CLOCK 1
  /* The fixed frequency clock for each FTM is MCGFFCLK (See 3.8.2.3 on
   * page 158 of the K70 Sub-Family Reference Manual, Rev. 2, Dec 2011) */
  /* See 25.4.5 on page 661 for a discussion of the MCG Fixed Frequency
   * Clock (MCGFFCLK).  It is driven by either the slow clock from the
   * internal reference clock generator or the external reference clock
   * from the Crystal Oscillator, divided by the FLL reference clock
   * divider as directed by MCG_C1[IREFS]. Additionally, this clock is
   * divided by two */
  /* On reset, field IREFS in MCG_C1 (See 25.3.1 on page 638) is 1 which
   * selects the slow internal reference clock (32 kHz) as the reference
   * clock for the FLL */
#define FTM_SC_CLKS_FIXED_FREQUENCY_CLOCK 2
  /* The external clock options for each FTM are selected using the
   * SIM_SOPT4 register (See 3.8.2.2 on page 158 and 12.2.4 on page 327
   * of the K70 Sub-Family Reference Manual, Rev. 2, Dec 2011) */

/* Values for the FTM_SC_PS field in FTM0_SC */
#define FTM_SC_PS_DIVIDE_BY_1 0
#define FTM_SC_PS_DIVIDE_BY_2 1
#define FTM_SC_PS_DIVIDE_BY_4 2
#define FTM_SC_PS_DIVIDE_BY_8 3
#define FTM_SC_PS_DIVIDE_BY_16 4
#define FTM_SC_PS_DIVIDE_BY_32 5
#define FTM_SC_PS_DIVIDE_BY_64 6
#define FTM_SC_PS_DIVIDE_BY_128 7
	
  /* The schematic shows J18 is used to select the clock connected to
   * FTM_CLKIN0, but that this jumper is permanently connected to all
   * three pads (see bottom of the K70 PC board next to J19).  Therefore, on
   * the TWR-K70F120M, FTM_CLKIN0 is connected to a 50MHz clock oscillator
   * as long as J19 is installed to provide power to that oscillator
   * (jumper J19 should already be installed). */
#define FTM_SC_CLKS_EXTERNAL_CLOCK 3

/* Function prototypes */
void flexTimer0Init(uint16_t count);

void flexTimer0Start(void);

void flexTimer0Stop(void);

void flexTimer0Isr(void);

void flexTimer0Action(void);

void flexTimerSetClock(uint32_t hi32, uint32_t low32);
uint32_t flexTimerGetClockHi();
uint32_t flexTimerGetClockLow();

#endif /* ifndef _FLEXTIMER_H */

