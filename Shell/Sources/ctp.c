#include <stdio.h>
#include "derivative.h"
#include "led.h"
#include "pushbutton.h"
#include "ctp.h"

/* Note below that the counterRegister field is declared to be a pointer
 * to an unsigned 16-bit value that is the counter register for the
 * appropriate channel of TSI0.  Further note that TSI0_CNTR5, for
 * example, is a 32-bit register that contains two 16-bit counters --
 * one for channel 4 in the low half and one for channel 5 in the high
 * half.  So, once &TSI0_CNTR5 is cast to be a pointer to a 16-bit
 * unsigned int, it is then a pointer to the 16-bit counter for
 * channel 4 of TSI0.  That pointer then needs to be incremented by
 * one so that it points to the 16-bit counter for channel 5 of TSI0.
 * This same technique is used for all the other counterRegister
 * field values as well. */
struct electrodeHW_s {
	int channel;
	uint32_t mask;
	uint16_t threshold;
	uint16_t *counterRegister;
} electrodeHW_a[ELECTRODE_COUNT] =
	{{5, TSI_PEN_PEN5_MASK, 0, (uint16_t *)&TSI0_CNTR5+1},	/* E1 */
	 {8, TSI_PEN_PEN8_MASK, 0, (uint16_t *)&TSI0_CNTR9},	/* E2 */
	 {7, TSI_PEN_PEN7_MASK, 0, (uint16_t *)&TSI0_CNTR7+1},	/* E3 */
	 {9, TSI_PEN_PEN9_MASK, 0, (uint16_t *)&TSI0_CNTR9+1}};	/* E4 */


/* Initialize the capacitive touch sensors */
void TSI_Init(void) {
	SIM_SCGC5 |= SIM_SCGC5_PORTA_MASK | SIM_SCGC5_PORTB_MASK
			| SIM_SCGC5_TSI_MASK;
	// Turn on the clock to ports A & B and
	//		to the TSI module
	PORTA_PCR4 = PORT_PCR_MUX(0);		// Enable port A, bit 4 as TSI0_CH5
	PORTB_PCR3 = PORT_PCR_MUX(0);		// Enable port B, bit 3 as TSI0_CH8
	PORTB_PCR2 = PORT_PCR_MUX(0);		// Enable port B, bit 2 as TSI0_CH7
	PORTB_PCR16 = PORT_PCR_MUX(0);		// Enable port B, bit 16 as TSI0_CH9

	TSI0_GENCS |= TSI_GENCS_EOSF_MASK;	// Clear the EOSF (End of Scan) flag

	TSI0_GENCS |= TSI_GENCS_NSCN(10) |	// Set number of consecutive scans per electrode to 11
			TSI_GENCS_PS(4) |// Set electrode oscillator prescaler to divide by 16
			TSI_GENCS_STPE_MASK |// Keep TSI running when MCU goes into low power mode
			TSI_GENCS_LPSCNITV(7);// Low power mode scan interval is set to 50 msec
	TSI0_SCANC |= (TSI_SCANC_EXTCHRG(8) |// Set ext oscillator charge current to 18 uA
			TSI_SCANC_REFCHRG(15) |// Set reference osc charge current to 32 uA
			TSI_SCANC_SMOD(10) |		// Set scan period modulus to 10
			TSI_SCANC_AMPSC(1) |		// Divide input clock by 2
			TSI_SCANC_AMCLKS(0));	// Set active mode clock source to LPOSCCLK

	//TSI0_GENCS |= TSI_GENCS_LPCLKS_MASK; // Set low power clock source to VLPOSCCLK

	/* Electrode E1 is aligned with the orange LED */
#define Electrode_E1_EN_MASK TSI_PEN_PEN5_MASK
	/* Electrode E2 is aligned with the yellow LED */
#define Electrode_E2_EN_MASK TSI_PEN_PEN8_MASK
	/* Electrode E3 is aligned with the green LED */
#define Electrode_E3_EN_MASK TSI_PEN_PEN7_MASK
	/* Electrode E4 is aligned with the blue LED */
#define Electrode_E4_EN_MASK TSI_PEN_PEN9_MASK

	TSI0_PEN = Electrode_E1_EN_MASK | Electrode_E2_EN_MASK |
	Electrode_E3_EN_MASK | Electrode_E4_EN_MASK;

	/* In low power mode only one pin may be active; this selects electrode E4 */
	TSI0_PEN |= TSI_PEN_LPSP(9);
	TSI0_GENCS |= TSI_GENCS_TSIEN_MASK;  // Enables TSI
}

/* Calibrate the capacitive touch sensors */
void TSI_Calibrate(void) {
	int i;
	uint16_t baseline;

	TSI0_GENCS |= TSI_GENCS_SWTS_MASK; /* Software Trigger Start */
	while (!(TSI0_GENCS & TSI_GENCS_EOSF_MASK)) {
	}
	TSI0_GENCS |= TSI_GENCS_EOSF_MASK;	// Clear the EOSF (End of Scan) flag

	for (i = 0; i < ELECTRODE_COUNT; i++) {
		baseline = *(electrodeHW_a[i].counterRegister);
		electrodeHW_a[i].threshold = baseline + THRESHOLD_OFFSET;
	}
}

int electrode_in(int electrodeNumber) {
	uint16_t oscCount;

	TSI0_GENCS |= TSI_GENCS_SWTS_MASK; /* Software Trigger Start */
	while (!(TSI0_GENCS & TSI_GENCS_EOSF_MASK)) {
	}
	TSI0_GENCS |= TSI_GENCS_EOSF_MASK;	// Clear the EOSF (End of Scan) flag

	oscCount = *(electrodeHW_a[electrodeNumber].counterRegister);

	/* Returns 1 when pushbutton is depressed and 0 otherwise */

	return oscCount > electrodeHW_a[electrodeNumber].threshold;
}

