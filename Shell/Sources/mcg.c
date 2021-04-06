/**
 * mcg.c
 * MCG initialization routines
 *
 * ARM-based K70F120M microcontroller board
 *   for educational purposes only
 * CSCI E-92 Spring 2017, Professor James L. Frankel, Harvard Extension School
 *
 * Written by James L. Frankel (frankel@seas.harvard.edu)
 *
 * Copyright © 2017, 2015, 2014, 2012 James L. Frankel.  All rights reserved.
 */

#include "derivative.h"
#include "mcg.h"

void mcgInit(void) {
	/* This routine will transition from FEI -> FBE -> PBE -> PEE */

	/* Initialize SIM dividers */

	/* Turn on the clock for PORT A because it is used for EXTAL */
	SIM_SCGC5 |= SIM_SCGC5_PORTA_MASK;

	/* Set core system clock to be xxx divided by 1 */
	/* Set peripheral clock to be xxx divided by 2 */
	/* Set FlexBus clock to be xxx divided by 3 */
	/* Set FLASH clock to be xxx divided by 6 */
	SIM_CLKDIV1 = SIM_CLKDIV1_OUTDIV1(SIM_CLKDIV1_OUTDIV_DIVIDE_BY_1) |
	SIM_CLKDIV1_OUTDIV2(SIM_CLKDIV1_OUTDIV_DIVIDE_BY_2) |
	SIM_CLKDIV1_OUTDIV3(SIM_CLKDIV1_OUTDIV_DIVIDE_BY_3) |
	SIM_CLKDIV1_OUTDIV4(SIM_CLKDIV1_OUTDIV_DIVIDE_BY_6);

	/* Initialize PLL0 */

	/* Set crystal oscillator to high frequency range */
	MCG_C2 = MCG_C2_RANGE0(1);

	/* Select the clock source for MCGOUTCLK to be the external reference clock */
	/* The FLL external reference divider is 1024 */
	/* The internal reference clock (MCGIRCLK) is inactive */
	MCG_C1 = MCG_C1_CLKS(2) | MCG_C1_FRDIV(5);

	/* MCGOUTCLK is now 50 MHz */

	/* The PLL0 external reference divide factor is 5 */
	MCG_C5 = MCG_C5_PRDIV0(4);

	/* PLL0 is now at 10 MHz */

	/* PLL0 output clock is selected when MCG_C1_CLKS is set to 0 */
	/* Enables the loss of clock monitoring */
	/* The VCO output of PLL0 is multiplied by 24 */
	MCG_C6 = MCG_C6_PLLS_MASK | MCG_C6_CME0_MASK | MCG_C6_VDIV0(8);

	/* The PLL0 external reference divide factor is 5 */
	/* Enable PLL0 for use as MCGPLL0CLK and MCGPLL0CLK2X */
	MCG_C5 = MCG_C5_PRDIV0(4) | MCG_C5_PLLCLKEN0_MASK;

	while (!(MCG_S & MCG_S_LOCK0_MASK)) {
	}

	/* Select the clock source for MCGOUTCLK to be PLLCS */
	/* The FLL external reference divider is 1024 */
	/* The internal reference clock (MCGIRCLK) is inactive */
	MCG_C1 = MCG_C1_FRDIV(5);

	/* MCGOUTCLK is now (((EXTAL / PRDIV) * VDIV) / 2) */
	/* MCGOUTCLK is now (((50 MHz / 5) * 24) / 2) = 120 MHz */

	/* Initialize PLL1 */

	/* Set OSC1 crystal oscillator to high frequency range */
	/* Select the oscillator to be the source for the OSC1 external reference
	 * clock */
	MCG_C10 = MCG_C10_RANGE1(1) | MCG_C10_EREFS1_MASK;

	/* The VCO output of PLL1 is multiplied by 30 */
	MCG_C12 = MCG_C12_VDIV1(14);

	/* The PLL1 external reference divide factor is 5 */
	/* Enable PLL1 for use as MCGPLL1CLK and MCGPLL1CLK2X and MCGDDRCLK2X */
	MCG_C11 = MCG_C11_PRDIV1(4) | MCG_C11_PLLCLKEN1_MASK;

	while (!(MCG_S2 & MCG_S2_LOCK1_MASK)) {
	}

	/* MCGPLL1CLK and MCGPLL1CLK2X and MCGDDRCLK2X are now
	 * (((EXTAL / PRDIV) * VDIV) / 2) */
	/* MCGPLL1CLK and MCGPLL1CLK2X and MCGDDRCLK2X are now
	 * (((50 MHz / 5) * 30) / 2) = 150 MHz */

	/* So now,
	 *  Core clock = 120 MHz
	 *  Bus (peripheral) clock = 60 MHz
	 *  FlexBus clock = 40 MHz
	 *  FLASH clock = 20 MHz
	 *  DDR clock = 150 MHz
	 *  MCGIRCLK (internal reference clock) is inactive */
}

