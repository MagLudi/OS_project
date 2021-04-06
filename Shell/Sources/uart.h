/**
 * uart.h
 * UART routines for polled serial I/O
 * 
 * ARM-based K70F120M microcontroller board
 *   for educational purposes only
 * CSCI E-92 Spring 2017, Professor James L. Frankel, Harvard Extension School
 *
 * Written by James L. Frankel (frankel@seas.harvard.edu)
 *
 * Copyright © 2017, 2015, 2014, 2012 James L. Frankel.  All rights reserved.
 */

#ifndef _UART_H
#define _UART_H

#include "derivative.h"

/* IRQs for UART status and error sources */
/*   IRQ number is the corresponding vector number minus 16 */
#define UART0_STATUS_IRQ_NUMBER 45
#define UART0_ERROR_IRQ_NUMBER 46
#define UART1_STATUS_IRQ_NUMBER 47
#define UART1_ERROR_IRQ_NUMBER 48
#define UART2_STATUS_IRQ_NUMBER 49
#define UART2_ERROR_IRQ_NUMBER 50
#define UART3_STATUS_IRQ_NUMBER 51
#define UART3_ERROR_IRQ_NUMBER 52
#define UART4_STATUS_IRQ_NUMBER 53
#define UART4_ERROR_IRQ_NUMBER 54
#define UART5_STATUS_IRQ_NUMBER 55
#define UART5_ERROR_IRQ_NUMBER 56

/* Interrupt priorities for UART status and error sources (range is 0 to 15) */
#define UART0_STATUS_INTERRUPT_PRIORITY 7
#define UART0_ERROR_INTERRUPT_PRIORITY 7
#define UART1_STATUS_INTERRUPT_PRIORITY 7
#define UART1_ERROR_INTERRUPT_PRIORITY 7
#define UART2_STATUS_INTERRUPT_PRIORITY 6
#define UART2_ERROR_INTERRUPT_PRIORITY 7
#define UART3_STATUS_INTERRUPT_PRIORITY 7
#define UART3_ERROR_INTERRUPT_PRIORITY 7
#define UART4_STATUS_INTERRUPT_PRIORITY 7
#define UART4_ERROR_INTERRUPT_PRIORITY 7
#define UART5_STATUS_INTERRUPT_PRIORITY 7
#define UART5_ERROR_INTERRUPT_PRIORITY 7

void uartInit(UART_MemMapPtr uartChannel, int clockInKHz, int baud);
char uartGetchar(UART_MemMapPtr uartChannel);
void uartPutchar(UART_MemMapPtr uartChannel, char ch);
int uartGetcharPresent(UART_MemMapPtr uartChannel);
void uartPuts(UART_MemMapPtr uartChannel, char *p);

#endif /* ifndef _UART_H */

