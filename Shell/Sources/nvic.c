/**
 * nvic.c
 * routines to manage the Nested Vectored Interrupt Controller, NVIC
 *
 * ARM-based K70F120M microcontroller board
 *   for educational purposes only
 * CSCI E-251 Fall 2012, Professor James L. Frankel, Harvard Extension School
 *
 * Written by James L. Frankel (frankel@seas.harvard.edu)
 */

#include <derivative.h>
#include "nvic.h"

/* Directs the Nested Vectored Interrupt Controller (NVIC) to enable interrupts
 * from the specified device at the specified priority level
 * 
 * Parameters:
 *   IRQ		identifies the device through its interrupt request number (IRQ)
 *   priority	the priority level requested
 *   
 * If either the IRQ number or the priority is invalid, this function performs no
 * action
 */ 
void NVICEnableIRQ(int IRQ, unsigned char priority) {
	int nonIPRRegisterNumber;
	int bitWithinNonIPRRegister;
	
	if(IRQ < 0 || IRQ > NVIC_MaxIRQ)
		return;
	
	if(priority > NVIC_MaxPriority)
		return;
	
	nonIPRRegisterNumber = IRQ/NVIC_FieldsPerNonIPRRegister;
	bitWithinNonIPRRegister = IRQ%NVIC_FieldsPerNonIPRRegister; 
	
	/* The IRQ for a device is its interrupt vector number minus 16.
	 * The first 16 vectors are reserved for ARM core interrupts. */
	
	/* Enable interrupts from the requested IRQ using the appropriate NVIC_ISER
	 * register (Interrupt Set-Enable Registers) (See B3.4.3 on page B3-755 of
	 * the ARM®v7-M Architecture Reference Manual, Errata markup,
	 * ARM DDI 0403Derrata 2010_Q3 (ID100710)) */
	NVIC_ISER_REG(NVIC_BASE_PTR, nonIPRRegisterNumber) =
			NVIC_ISER_SETENA(1<<bitWithinNonIPRRegister);

	/* Set the interrupt priority of the requested IRQ using the appropriate
	 * NVIC_IPR register (Interrupt Priority Registers) (See B3.4.8 on page B3-760
	 * of the ARM®v7-M Architecture Reference Manual, Errata markup,
	 * ARM DDI 0403Derrata 2010_Q3 (ID100710)) */

	/* The ARMv7-M Architecture Reference Manual in section "B1.5.4 Exception
	 * priorities and preemption" on page B1-635 states, "The number of
	 * supported priority values is an IMPLEMENTATION DEFINED power of two in
	 * the range 8 to 256, and the minimum supported priority value is always 0.
	 * All priority value fields are 8-bits, and if an implementation supports
	 * fewer than 256 priority levels then low-order bits of these fields are RAZ."
	 * 
	 * In the K70 Sub-Family Reference Manual in section "3.2.2.1 Interrupt
	 * priority levels" on page 85, it states, "This device supports 16 priority
	 * levels for interrupts.  Therefore, in the NVIC each source in the IPR
	 * registers contains 4 bits."  The diagram that follows goes on to confirm
	 * that only the high-order 4 bits of each 8 bit field is used.  It doesn't
	 * explicitly mention the System Handler (like the SVC handler) priorities,
	 * but they should be handled consistently with the interrupt priorities.
	 */

	NVIC_IP_REG(NVIC_BASE_PTR, IRQ) = priority << NVIC_PriorityShift;
}
