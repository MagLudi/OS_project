/*
 * lcdc.c
 * LCD Controller Driver Implementation
 *
 * This is the implementation of using the LCD controller (LCDC) in the K70 to drive the 
 *   TWR-LCD-RGB display
 *
 * ARM-based K70F120M microcontroller board
 *   for educational purposes only
 * CSCI E-92 Spring 2014, Professor James L. Frankel, Harvard Extension School
 *
 * Written by James L. Frankel (frankel@seas.harvard.edu)
 */

#include <string.h>
#include "derivative.h"
#include "lcdc.h"

void lcdcInit(void) {
	/* Errata ID 5234 in Mask Set Errata for Mask 0N96B, Rev. 05 OCT 2012
	 * states that "GPIO: PORTF registers are not accessible unless
	 * SIM_SCGC5[PORTE] is set." */

	/* Enable Port clocks */
	SIM_SCGC5 |= SIM_SCGC5_PORTE_MASK | SIM_SCGC5_PORTF_MASK;

	/* Enable LCDC module */
	SIM_SCGC3 |= SIM_SCGC3_LCDC_MASK;

	/* Disable MPU */
	MPU_CESR &= ~MPU_CESR_VLD_MASK;

	/* Set up LCD pin muxing */
	lcdcInitPins();

	/* Configure & enable LCDC for TWR-LCD-RGB */
	lcdcInitScreen();
}

void lcdcInitPins() {
	PORTF_PCR4 = LCDC_ALT7; // Graphic LCD D[0],  Schematic PTF4
	PORTF_PCR5 = LCDC_ALT7; // Graphic LCD D[1],  Schematic PTF5
	PORTF_PCR6 = LCDC_ALT7; // Graphic LCD D[2],  Schematic PTF6
	PORTF_PCR7 = LCDC_ALT7; // Graphic LCD D[3],  Schematic PTF7
	PORTF_PCR8 = LCDC_ALT7; // Graphic LCD D[4],  Schematic PTF8
	PORTF_PCR9 = LCDC_ALT7; // Graphic LCD D[5],  Schematic PTF9
	PORTF_PCR10 = LCDC_ALT7; // Graphic LCD D[6],  Schematic PTF10
	PORTF_PCR11 = LCDC_ALT7; // Graphic LCD D[7],  Schematic PTF11
	PORTF_PCR12 = LCDC_ALT7; // Graphic LCD D[8],  Schematic PTF12
	PORTF_PCR13 = LCDC_ALT7; // Graphic LCD D[9],  Schematic PTF13
	PORTF_PCR14 = LCDC_ALT7; // Graphic LCD D[10], Schematic PTF14
	PORTF_PCR15 = LCDC_ALT7; // Graphic LCD D[11], Schematic PTF15

	PORTF_PCR16 = LCDC_ALT5; // Graphic LCD D[12], Schematic PTF16
	PORTF_PCR17 = LCDC_ALT5; // Graphic LCD D[13], Schematic PTF17
	PORTF_PCR18 = LCDC_ALT5; // Graphic LCD D[14], Schematic PTF18
	PORTF_PCR19 = LCDC_ALT5; // Graphic LCD D[15], Schematic PTF19
	PORTF_PCR20 = LCDC_ALT5; // Graphic LCD D[16], Schematic PTF20

	PORTF_PCR21 = LCDC_ALT7; // Graphic LCD D[17], Schematic PTF21
	PORTF_PCR22 = LCDC_ALT7; // Graphic LCD D[18], Schematic PTF22
	PORTF_PCR23 = LCDC_ALT7; // Graphic LCD D[19], Schematic PTF23
	PORTF_PCR24 = LCDC_ALT7; // Graphic LCD D[20], Schematic PTF24
	PORTF_PCR25 = LCDC_ALT7; // Graphic LCD D[21], Schematic PTF25
	PORTF_PCR26 = LCDC_ALT7; // Graphic LCD D[22], Schematic PTF26
	PORTF_PCR27 = LCDC_ALT7; // Graphic LCD D[23], Schematic PTF27

	PORTF_PCR0 = LCDC_ALT7; // Graphic LCD PCLK,  Schematic PTF0
	PORTF_PCR1 = LCDC_ALT7; // Graphic LCD DE,    Schematic PTF1
	PORTF_PCR2 = LCDC_ALT7; // Graphic LCD HSYNC, Schematic PTF2
	PORTF_PCR3 = LCDC_ALT7; // Graphic LCD VSYNC, Schematic PTF3

	// set LCD_CONTRAST
//  PORTB_PCR4  = LCDC_ALT2; // Graphic LCD CONTRAST, Schematic PTB4
}

/*
 * Set up the LCDC for the LCD screen on the TWR-LCD-RGB
 */
void lcdcInitScreen() {
	uint8_t byte;
	uint32_t *p;

	// Set LCD Screen Start Address
	LCDC_LSSAR = LCDC_FRAME_START_ADDRESS;

	// Set LCD Size.  The XMAX bit field is the screen x-size/16.
	LCDC_LSR = LCDC_LSR_XMAX(
			LCDC_SCREEN_XSIZE/16) | LCDC_LSR_YMAX(LCDC_SCREEN_YSIZE);

	// Set LCD virtual page width
	LCDC_LVPWR = LCDC_LVPWR_VPW(LCDC_SCREEN_XSIZE);

	// Set LCD cursor position & settings (turn off)
	LCDC_LCPR = 0;
	LCDC_LCWHB = 0;

	// Set LCD panel configuration.  Use endianess to work with TWR-LCD-RGB lines.
	LCDC_LPCR =
	LCDC_LPCR_TFT_MASK |       // TFT Screen
			LCDC_LPCR_COLOR_MASK |       // Color
			LCDC_LPCR_BPIX(0x7) |       // 24 bpp
			LCDC_LPCR_FLMPOL_MASK |     // first line marker active low (VSYNC)
			LCDC_LPCR_LPPOL_MASK |       // line pulse active low (HSYNC)
			LCDC_LPCR_END_SEL_MASK | // Use big-endian mode (0xFFAA5500 means R=AA,G=55,B=00)
			LCDC_LPCR_SCLKIDLE_MASK |       // Enable LSCLK when vsync is idle
			LCDC_LPCR_SCLKSEL_MASK |       // Always enable clock
			LCDC_LPCR_PCD(11); // Divide 120 PLL0 clock (default clock) by (11+1)=12 to get 10MHz clock

#ifdef REVE  
	// If RevE or later TWR-LCD-RGB, need to adjust clock settings

	LCDC_LPCR |= LCDC_LPCR_CLKPOL_MASK; //In TFT mode, active on negative edge of LSCLK

	// Set LCD horizontal configuration based on panel data (Figure 3-3 in Seiko datasheet)
	LCDC_LHCR =
	LCDC_LHCR_H_WIDTH(41) |    // (41+1)=42 SCLK period for HSYNC activated
			LCDC_LHCR_H_WAIT_1(1) | // (1+1)=2 SCLK period between end of OE and beginning of HSYNC
			LCDC_LHCR_H_WAIT_2(0); // (0+3)=3 SCLK periods between end of HSYNC and beginning of OE

	// Set LCD vertical configuration based on panel data (Figure 3-3 in Seiko datasheet)
	LCDC_LVCR =
	LCDC_LVCR_V_WIDTH(2) |   // 2 lines period for VSYNC activated
			LCDC_LVCR_V_WAIT_1(1) | // 1 line period between end of OE and beginning of VSYNC
			LCDC_LVCR_V_WAIT_2(1); // 1 line periods between end of VSYNC and beginning of OE
#else
			//Earlier TWR-LCD-RGB versions (B, C, and D)

			LCDC_LHCR =
			LCDC_LHCR_H_WIDTH(40) |// (40+1)=41 SCLK period for HSYNC activated
			LCDC_LHCR_H_WAIT_1(1) |// (1+1)=2 SCLK period between end of OE and beginning of HSYNC
			LCDC_LHCR_H_WAIT_2(0);// (0+3)=3 SCLK periods between end of HSYNC and beginning of OE

			// set LCD vertical configuration based on panel data (Figure 3-3 in Seiko datasheet)
			LCDC_LVCR =
			LCDC_LVCR_V_WIDTH(10) |// 10 lines period for VSYNC activated
			LCDC_LVCR_V_WAIT_1(2) |// 2 line period between end of OE and beginning of VSYNC
			LCDC_LVCR_V_WAIT_2(2);// 2 line periods between end of VSYNC and beginning of OE
#endif

	// Set the LCD panning offset (not used in 24bpp mode)
	LCDC_LPOR = 0;

	// Set LCD interrupt configuration register
	LCDC_LICR = 0;

	// Disable LCDC interrupts
	LCDC_LIER = 0;

	// Disable the graphic window
	LCDC_LGWCR &= ~LCDC_LGWCR_GWE_MASK;

	// Set background plane DMA to burst mode
	LCDC_LDCR &= ~LCDC_LDCR_BURST_MASK;

	// Set graphic window DMA to burst mode
	LCDC_LGWDCR &= ~LCDC_LGWDCR_GWBT_MASK;

	// Initialize the frame buffer to the background color
	//	If the red, green, and blue bytes all are the same value, use memset to initialize the background color because
	//	it is more efficient
	byte = LCDC_PIXEL_RED(LCDC_SCREEN_BACKGROUND_COLOR);
	if ((byte == LCDC_PIXEL_GREEN(LCDC_SCREEN_BACKGROUND_COLOR))
			&& (byte == LCDC_PIXEL_BLUE(LCDC_SCREEN_BACKGROUND_COLOR))) {
		memset((void *) LCDC_FRAME_START_ADDRESS, byte,
				LCDC_BYTES_PER_PIXEL * LCDC_SCREEN_XSIZE * LCDC_SCREEN_YSIZE);
	} else {
		for (p = (uint32_t *) LCDC_FRAME_START_ADDRESS;
				p
						< (uint32_t *) LCDC_FRAME_START_ADDRESS
								+ LCDC_SCREEN_XSIZE * LCDC_SCREEN_YSIZE; p++) {
			*p = LCDC_SCREEN_BACKGROUND_COLOR;
		}
	}

	// Start the LCDC
	SIM_MCR |= SIM_MCR_LCDSTART_MASK;
}

