/**
 * intSerialIO.c
 * Serial input and output interrupt test program
 * 
 * ARM-based K70F120M microcontroller board
 *   for educational purposes only
 * CSCI E-92 Spring 2014, Professor James L. Frankel, Harvard Extension School
 *
 * Written by James L. Frankel (frankel@seas.harvard.edu)
 */

/*
 * Important note:
 * 
 * The file Project_Settings -> Startup_Code -> kinetis_sysinit.c needs to be modified so
 * that a pointer to the interruptSerialPort2 function is in the vector table at vector 65 (0x0000_0104)
 * for UART2 status sources.
 *
 * The following declaration needs to inserted earlier in the file:
 *   extern void interruptSerialPort2(void);
 *
 * If using the GCC Toolchain, the vector table is named "InterruptVector", and the line:
 *   Default_Handler,		(comment delimiters removed) Vector 65: UART2, Status Sources
 * needs to be changed to:
 *   interruptSerialPort2,	(comment delimiters removed) Vector 65: UART2, Status Sources
 *
 * If using the Freescale Toolchain, the vector table is named "__vect_table", and the line:
 *   (tIsrFunc)UNASSIGNED_ISR,       (comment delimiters removed) 65 (0x00000104) (prior: -)
 * needs to be changed to:
 *   (tIsrFunc)interruptSerialPort2, (comment delimiters removed) 65 (0x00000104) (prior: -)
 */

#include <stdio.h>
#include "derivative.h"
#include "intSerialIO.h"
#include "uart.h"
#include "nvic.h"
#include "priv.h"

/* The buffer to store characters input from serial port 2 */
char serialPort2InputBuffer[SERIAL_PORT_2_INPUT_BUFFER_SIZE];
int serialPort2InputEnqueueOffset = 0;
int serialPort2InputDequeueOffset = 0;
int serialPort2InputCharCount = 0;

/* The buffer to store characters to be output to serial port 2 */
char serialPort2OutputBuffer[SERIAL_PORT_2_OUTPUT_BUFFER_SIZE];
int serialPort2OutputEnqueueOffset = 0;
int serialPort2OutputDequeueOffset = 0;
int serialPort2OutputCharCount = 0;

volatile int interruptCount = 0;
volatile int interruptTDRECount = 0;
volatile int interruptRDRFCount = 0;
volatile int interruptNeitherTDREnorRDRFCount = 0;

/*****************************************************************************/
/*                                                                           */
/*  Name: interruptSerialPort2                                               */
/*                                                                           */
/*  Parameters:                                                              */
/*    None                                                                   */
/*                                                                           */
/*  Return value:                                                            */
/*    None                                                                   */
/*                                                                           */
/*  Side effects:                                                            */
/*    The serialPort2InputBuffer and associated data structures may be       */
/*    updated.                                                               */
/*                                                                           */
/*****************************************************************************/
void interruptSerialPort2(void) {
	uint32_t status;
	char ch;
	
	interruptCount++;

	status = UART2_S1;
	
	if(!(status & UART_S1_TDRE_MASK) && !(status & UART_S1_RDRF_MASK)) {
		interruptNeitherTDREnorRDRFCount++;
	}
	
	if((UART2_C2 & UART_C2_TIE_MASK) && (status & UART_S1_TDRE_MASK)) {
		char ch;
		
		interruptTDRECount++;
		
		/* The Transmit Data Register Empty Flag indicates that the amount of data in
		 * the transmit buffer is less than or equal to the value indicated by
		 * TWFIFO[TXWATER] at some point in time since the flag has been cleared
		 * (See 57.3.5 on page 1911 of the K70 Sub-Family Reference Manual,
		 * Rev. 2, Dec 2011) */

		/* The UART FIFO Transmit Watermark register (UARTx_TWFIFO) is initialized so
		 * that TXWATER is 0 on reset (See 57.3.19 on page 1929 of the K70 Sub-Family
		 * Reference Manual, Rev. 2, Dec 2011) */
		
		/* To clear TDRE, read S1 when TDRE is set and then write to the UART data
		 * register (D) (See 57.3.5 on page 1911 of the K70 Sub-Family Reference Manual,
		 * Rev. 2, Dec 2011) */

		/* Disable the transmitter interrupt for UART2 using the UART2_C2 register
		 * (UART Control Register 2) (See 57.3.4 on page 1909 of the K70 Sub-Family
		 * Reference Manual, Rev. 2, Dec 2011) */
	    
		if(serialPort2OutputCharCount > 0) {
			/* There is a character in the output buffer to be transmitted */
			
			/* Dequeue the character */
			ch = serialPort2OutputBuffer[serialPort2OutputDequeueOffset++];
			serialPort2OutputDequeueOffset = serialPort2OutputDequeueOffset %
					                         SERIAL_PORT_2_OUTPUT_BUFFER_SIZE;
			serialPort2OutputCharCount--;

			/* write the character to the UART */
			UART_D_REG(UART2_BASE_PTR) = ch;
		}

		/* If there are no more characters in the output buffer, disable the transmitter
		 * interrupt */
		if(serialPort2OutputCharCount <= 0) {
			UART2_C2 &= ~UART_C2_TIE_MASK;
		}
	}

	if((UART2_C2 & UART_C2_RIE_MASK) && (status & UART_S1_RDRF_MASK)) {
		interruptRDRFCount++;

		/* The Receive Data Register Full Flag indicates that the number of datawords
		 * in the receive buffer is equal to or more than the number indicated by
		 * RWFIFO[RXWATER] (See 57.3.5 on page 1911 of the K70 Sub-Family Reference Manual,
		 * Rev. 2, Dec 2011) */

		/* The UART FIFO Receive Watermark register (UARTx_RWFIFO) is initialized so
		 * that RXWATER is 1 on reset (See 57.3.21 on page 1930 of the K70 Sub-Family
		 * Reference Manual, Rev. 2, Dec 2011) */
		
		/* To clear RDRF, read S1 when RDRF is set and then read the UART data register
		 * (D) (See 57.3.5 on page 1911 of the K70 Sub-Family Reference Manual, Rev. 2,
		 * Dec 2011) */

		/* read the character that caused the interrupt */
		ch = UART_D_REG(UART2_BASE_PTR);
	
		if(serialPort2InputCharCount < SERIAL_PORT_2_INPUT_BUFFER_SIZE) {
			/* There is room in the input buffer for another character */
			serialPort2InputBuffer[serialPort2InputEnqueueOffset++] = ch;
			serialPort2InputEnqueueOffset = serialPort2InputEnqueueOffset %
                                       	   SERIAL_PORT_2_INPUT_BUFFER_SIZE;
			serialPort2InputCharCount++;
		}

		/* If there is no room in the input buffer for this character; discard it */
	}
}

/*****************************************************************************/
/*                                                                           */
/*  Name: getcharFromBuffer                                                  */
/*                                                                           */
/*  Parameters:                                                              */
/*    None                                                                   */
/*                                                                           */
/*  Return value:                                                            */
/*    Type    Description                                                    */
/*    char    the next character input from serial port 2.  This character   */
/*            will be retrieved from the serialPort2InputBuffer in FIFO      */
/*            fashion.                                                       */
/*                                                                           */
/*  Side effects:                                                            */
/*    The serialPort2InputBuffer and associated data structures may be       */
/*    updated.                                                               */
/*    Interrupts will be disabled and re-enabled by this routine.            */
/*                                                                           */
/*****************************************************************************/
char getcharFromBuffer(void) {
    char ch;

	/* Guarantee the following operations are atomic */

    /* Disable interrupts (PRIMASK is set) */
	__asm("cpsid i");

	while(serialPort2InputCharCount <= 0) {
    	/* No chars in the buffer; let's wait for at least one char to arrive */

		/* Allows interrupts (PRIMASK is cleared) */
		__asm("cpsie i");

		/* This is when an interrupt could occur */
		
		/* Disable interrupts (PRIMASK is set) */
		__asm("cpsid i");
    }

    /* A character should be in the buffer; remove the oldest one. */
    ch = serialPort2InputBuffer[serialPort2InputDequeueOffset++];
    serialPort2InputDequeueOffset = serialPort2InputDequeueOffset %
                                    SERIAL_PORT_2_INPUT_BUFFER_SIZE;
    serialPort2InputCharCount--;

    /* Allows interrupts (PRIMASK is cleared) */
	__asm("cpsie i");

	return ch;
}

/*****************************************************************************/
/*                                                                           */
/*  Name: putcharIntoBuffer                                                  */
/*                                                                           */
/*  Parameters:                                                              */
/*    Type    Description                                                    */
/*    char    the character to be output over serial port 2.  This character */
/*            will be buffered in the serialPort2OutputBuffer in FIFO        */
/*            fashion.                                                       */
/*                                                                           */
/*  Return value:                                                            */
/*    None                                                                   */
/*                                                                           */
/*  Side effects:                                                            */
/*    The serialPort2OutputBuffer and associated data structures may be      */
/*    updated.                                                               */
/*    Interrupts will be disabled and re-enabled by this routine.            */
/*                                                                           */
/*****************************************************************************/
void putcharIntoBuffer(char ch) {
	/* Guarantee the following operations are atomic */

    /* Disable interrupts (PRIMASK is set) */
	__asm("cpsid i");

	while(serialPort2OutputCharCount >= SERIAL_PORT_2_OUTPUT_BUFFER_SIZE) {
    	/* The buffer is full; let's wait for at least one char to be removed */

	    /* Allows interrupts (PRIMASK is cleared) */
		__asm("cpsie i");

		/* This is when an interrupt could occur */
		
		/* Disable interrupts (PRIMASK is set) */
		__asm("cpsid i");
	}

	/* There is room in the output buffer for another character */
	serialPort2OutputBuffer[serialPort2OutputEnqueueOffset++] = ch;
	serialPort2OutputEnqueueOffset = serialPort2OutputEnqueueOffset %
									 SERIAL_PORT_2_OUTPUT_BUFFER_SIZE;
	serialPort2OutputCharCount++;

	/* Enable the transmitter interrupt for UART2 using the UART2_C2 register
	 * (UART Control Register 2) (See 57.3.4 on page 1909 of the K70 Sub-Family Reference
	 * Manual, Rev. 2, Dec 2011) */
	UART2_C2 |= UART_C2_TIE_MASK;

	/* Allows interrupts (PRIMASK is cleared) */
	__asm("cpsie i");
}

/*****************************************************************************/
/*                                                                           */
/*  Name: putsIntoBuffer                                                     */
/*                                                                           */
/*  Parameters:                                                              */
/*    Type    Description                                                    */
/*    s       pointer to the string to be output over serial port 2.         */
/*            This string will be buffered in the serialPort2OutputBuffer    */
/*            in FIFO fashion.                                               */
/*                                                                           */
/*  Return value:                                                            */
/*    None                                                                   */
/*                                                                           */
/*  Side effects:                                                            */
/*    The serialPort2OutputBuffer and associated data structures may be      */
/*    updated.                                                               */
/*    Interrupts will be disabled and re-enabled by this routine.            */
/*                                                                           */
/*****************************************************************************/
void putsIntoBuffer(char *s) {
	while(*s) {
		putcharIntoBuffer(*s++);
	}
}

void intSerialIOInit(void) {
	/* On reset (i.e., before calling mcgInit), the processor clocking
	 * starts in FEI (FLL Engaged Internal) mode.  In FEI mode and with
	 * default settings (DRST_DRS = 00, DMX32 = 0), the MCGFLLCLK, the
	 * MCGOUTCLK (MCG (Multipurpose Clock Generator) clock), and the Bus
	 * (peripheral) clock are all set to 640 * IRC.  IRC is the Internal
	 * Reference Clock which runs at 32 KHz. [See K70 Sub-Family
	 * Reference Manual, Rev. 2, Section 25.4.1.1, Table 25-22 on
	 * page 657 and MCG Control 4 Register (MCG_C4) Section 25.3.4 on
	 * page 641] */
	
	/* After calling mcgInit, MCGOUTCLK is set to 120 MHz and the Bus
	 * (peripheral) clock is set to 60 MHz.*/

	/* Table 5-2 on page 221 indicates that the clock used by UART0 and
	 * UART1 is the System clock (i.e., MCGOUTCLK) and that the clock
	 * used by UART2-5 is the Bus clock. /
	const int IRC = 32000;					/* Internal Reference Clock /
	const int FLL_Factor = 640;
	const int moduleClock = FLL_Factor*IRC;
	const int KHzInHz = 1000;

	const int baud = 9600;

	uartInit(UART2_BASE_PTR, moduleClock/KHzInHz, baud);*/

	/* Enable the receiver full interrupt for UART2 using the UART2_C2 register
	 * (UART Control Register 2) (See 57.3.4 on page 1909 of the K70 Sub-Family Reference
	 * Manual, Rev. 2, Dec 2011) */
	UART2_C2 |= UART_C2_RIE_MASK;

	/* Enable interrupts from UART2 status sources and set its interrupt priority */
	NVICEnableIRQ(UART2_STATUS_IRQ_NUMBER, UART2_STATUS_INTERRUPT_PRIORITY);
}

bool hasOutput(){
	return serialPort2OutputCharCount > 1;
}


