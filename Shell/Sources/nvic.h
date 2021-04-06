/**
 * nvic.h
 * routines to manage the Nested Vectored Interrupt Controller, NVIC
 *
 * ARM-based K70F120M microcontroller board
 *   for educational purposes only
 * CSCI E-251 Fall 2012, Professor James L. Frankel, Harvard Extension School
 *
 * Written by James L. Frankel (frankel@seas.harvard.edu)
 */

#ifndef NVC_
#define NVC_

#define NVIC_MaxIRQ 105
#define NVIC_MaxPriority 15
#define NVIC_PriorityShift 4
#define NVIC_FieldsPerNonIPRRegister 32

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
void NVICEnableIRQ(int IRQ, unsigned char priority);

#endif /* ifndef NVC_ */
