/**
 * svc.c
 * Routines for supervisor calls
 *
 * ARM-based K70F120M microcontroller board
 *   for educational purposes only
 * CSCI E-92 Spring 2014, Professor James L. Frankel, Harvard Extension School
 *
 * Written by James L. Frankel (frankel@seas.harvard.edu)
 */

/*
 * Size of the supervisor call stack frame (no FP extension):
 *   No alignment => 32 (0x20) bytes
 *   With alignment => 36 (0x24) bytes
 *   
 * Format of the supervisor call stack frame (no FP extension):
 *   SP Offset   Contents
 *   +0			 R0
 *   +4			 R1
 *   +8			 R2
 *   +12		 R3
 *   +16		 R12
 *   +20		 LR (R14)
 *   +24		 Return Address
 *   +28		 xPSR (bit 9 indicates the presence of a reserved alignment
 *   				   word at offset +32)
 *   +32		 Possible Reserved Word for Alignment on 8 Byte Boundary
 *   
 * Size of the supervisor call stack frame (with FP extension):
 *   No alignment => 104 (0x68) bytes
 *   With alignment => 108 (0x6C) bytes
 *   
 * Format of the supervisor call stack frame (with FP extension):
 *   SP Offset   Contents
 *   +0			 R0
 *   +4			 R1
 *   +8			 R2
 *   +12		 R3
 *   +16		 R12
 *   +20		 LR (R14)
 *   +24		 Return Address
 *   +28		 xPSR (bit 9 indicates the presence of a reserved alignment
 *   				   word at offset +104)
 *   +32		 S0
 *   +36		 S1
 *   +40		 S2
 *   +44		 S3
 *   +48		 S4
 *   +52		 S5
 *   +56		 S6
 *   +60		 S7
 *   +64		 S8
 *   +68		 S9
 *   +72		 S10
 *   +76		 S11
 *   +80		 S12
 *   +84		 S13
 *   +88		 S14
 *   +92		 S15
 *   +96		 FPSCR
 *   +100		 Reserved Word for 8 Byte Boundary of Extended Frame
 *   +104		 Possible Reserved Word for Alignment on 8 Byte Boundary
 */

#include <derivative.h>
#include <stdio.h>
#include "svc.h"
#include "fio.h"
#include "mem.h"
#include "uart.h"
#include "PDB.h"
#include "pushbutton.h"
#include "flexTimer.h"
#include "utl.h"
#include "shell.h"
#include "rtc.h"

#define XPSR_FRAME_ALIGNED_BIT 9
#define XPSR_FRAME_ALIGNED_MASK (1<<XPSR_FRAME_ALIGNED_BIT)

struct frame {
	union {
		int r0;
		int arg0;
		char *str0;
		char c; //for fputc
		unsigned int size0; //for myMalloc
		void *address; //for myFree/myFreeErrorCode and the return for myMalloc
		int returnVal;
		MemErrno err; //for myFreeErrorCode
		myFILE fi1; //for file commands
		uint16_t frequ;
		uint32_t hi32;
		uint32_t *hi32_p;
		pid_t targetPid;
		utlErrno_t (*main)(int argc, char *argv[]);
	};
	union {
		int r1;
		int arg1;
		char *str1;
		myFILE fi2; //for file commands
		uint32_t low32;
		uint32_t *low32_p;
		void (*func_p)();
		shArg_t *arg;
	};
	union {
		int r2;
		int arg2;
		char *str2;
		myFILE fi3;
		uint32_t stackSize;
	};
	union {
		int r3;
		int arg3;
		char *str3;
		pid_t *spawnedPidPtr;
	};
	int r12;
	union {
		int r14;
		int lr;
	};
	int returnAddr;
	int xPSR;
};

/* Issue the SVC (Supervisor Call) instruction (See A7.7.175 on page A7-503 of the
 * ARM®v7-M Architecture Reference Manual, ARM DDI 0403Derrata 2010_Q3 (ID100710)) */

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wreturn-type"
myFILE __attribute__((naked)) __attribute__((noinline)) SVCFOpen(char *str0, char *str1) {
	__asm("svc %0" : : "I" (SVC_FOPEN));
	__asm("bx lr");
}
#pragma GCC diagnostic pop

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wreturn-type"
int __attribute__((naked)) __attribute__((noinline)) SVCFClose(myFILE fi1) {
	__asm("svc %0" : : "I" (SVC_FCLOSE));
	__asm("bx lr");
}
#pragma GCC diagnostic pop

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wreturn-type"
int __attribute__((naked)) __attribute__((noinline)) SVCCreate(char *str0, char *str1) {
	__asm("svc %0" : : "I" (SVC_CREATE));
	__asm("bx lr");
}
#pragma GCC diagnostic pop

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wreturn-type"
int __attribute__((naked)) __attribute__((noinline)) SVCDelete(char *str0) {
	__asm("svc %0" : : "I" (SVC_DELETE));
	__asm("bx lr");
}
#pragma GCC diagnostic pop

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wreturn-type"
int __attribute__((naked)) __attribute__((noinline)) SVCRewind(myFILE fi1) {
	__asm("svc %0" : : "I" (SVC_REWIND));
	__asm("bx lr");
}
#pragma GCC diagnostic pop

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wreturn-type"
int __attribute__((naked)) __attribute__((noinline)) SVCFPutc(char c, myFILE fi2) {
	__asm("svc %0" : : "I" (SVC_FPUTC));
	__asm("bx lr");
}
#pragma GCC diagnostic pop

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wreturn-type"
int __attribute__((naked)) __attribute__((noinline)) SVCFGetc(myFILE fi1) {
	__asm("svc %0" : : "I" (SVC_FGETC));
	__asm("bx lr");
}
#pragma GCC diagnostic pop

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wreturn-type"
void * __attribute__((naked)) __attribute__((noinline)) SVCMalloc(unsigned int size0) {
	__asm("svc %0" : : "I" (SVC_MALLOC));
	__asm("bx lr");
}
#pragma GCC diagnostic pop

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wreturn-type"
MemErrno __attribute__((naked)) __attribute__((noinline)) SVCFree(void *address) {
	__asm("svc %0" : : "I" (SVC_FREE));
	__asm("bx lr");
}
#pragma GCC diagnostic pop

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wreturn-type"
void __attribute__((naked)) __attribute__((noinline)) SVCprintStr(char *str0) {
	__asm("svc %0" : : "I" (SVC_PRINTSTR));
	__asm("bx lr");
}
#pragma GCC diagnostic pop

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wreturn-type"
char __attribute__((naked)) __attribute__((noinline)) SVCgetChar() {
	__asm("svc %0" : : "I" (SVC_GETCHAR));
	__asm("bx lr");
}
#pragma GCC diagnostic pop

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wreturn-type"
void __attribute__((naked)) __attribute__((noinline)) SVCpdb(uint16_t frequ, void (*func_p)()) {
	__asm("svc %0" : : "I" (SVC_PDB));
	__asm("bx lr");
}
#pragma GCC diagnostic pop

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wreturn-type"
void __attribute__((naked)) __attribute__((noinline)) SVCpdbStart() {
	__asm("svc %0" : : "I" (SVC_PDB_START));
	__asm("bx lr");
}
#pragma GCC diagnostic pop

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wreturn-type"
void __attribute__((naked)) __attribute__((noinline)) SVCsetClock(uint32_t hi32, uint32_t low32) {
	__asm("svc %0" : : "I" (SVC_SET_CLOCK));
	__asm("bx lr");
}
#pragma GCC diagnostic pop

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wreturn-type"
void __attribute__((naked)) __attribute__((noinline)) SVCgetClock(uint32_t *hi32_p, uint32_t *low32_p) {
	__asm("svc %0" : : "I" (SVC_GET_CLOCK));
	__asm("bx lr");
}
#pragma GCC diagnostic pop

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wreturn-type"
int __attribute__((naked)) __attribute__((noinline)) SVCspawn(utlErrno_t main(int argc, char *argv[]), shArg_t *arg,
		uint32_t stackSize, pid_t *spawnedPidPtr) {
	__asm("svc %0" : : "I" (SVC_SPAWN));
	__asm("bx lr");
}
#pragma GCC diagnostic pop

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wreturn-type"
void __attribute__((naked)) __attribute__((noinline)) SVCyield(void) {
	__asm("svc %0" : : "I" (SVC_YIELD));
	__asm("bx lr");
}
#pragma GCC diagnostic pop

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wreturn-type"
void __attribute__((naked)) __attribute__((noinline)) SVCblock(void) {
	__asm("svc %0" : : "I" (SVC_BLOCK));
	__asm("bx lr");
}
#pragma GCC diagnostic pop

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wreturn-type"
int __attribute__((naked)) __attribute__((noinline)) SVCblockPid(pid_t targetPid) {
	__asm("svc %0" : : "I" (SVC_BLOCK_PID));
	__asm("bx lr");
}
#pragma GCC diagnostic pop

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wreturn-type"
int __attribute__((naked)) __attribute__((noinline)) SVCwake(pid_t targetPid) {
	__asm("svc %0" : : "I" (SVC_WAKE));
	__asm("bx lr");
}
#pragma GCC diagnostic pop

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wreturn-type"
int __attribute__((naked)) __attribute__((noinline)) SVCmyKill(pid_t targetPid) {
	__asm("svc %0" : : "I" (SVC_KILL));
	__asm("bx lr");
}
#pragma GCC diagnostic pop

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wreturn-type"
void __attribute__((naked)) __attribute__((noinline)) SVCwait(pid_t targetPid) {
	__asm("svc %0" : : "I" (SVC_WAIT));
	__asm("bx lr");
}
#pragma GCC diagnostic pop

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wreturn-type"
pid_t __attribute__((naked)) __attribute__((noinline)) SVCpid() {
	__asm("svc %0" : : "I" (SVC_PID));
	__asm("bx lr");
}
#pragma GCC diagnostic pop

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wreturn-type"
char __attribute__((naked)) __attribute__((noinline)) SVCgetCharNoEch() {
	__asm("svc %0" : : "I" (SVC_GETCHAR_NOECH));
	__asm("bx lr");
}
#pragma GCC diagnostic pop

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wreturn-type"
int __attribute__((naked)) __attribute__((noinline)) SVCFPuts(char *str0, myFILE fi2) {
	__asm("svc %0" : : "I" (SVC_FPUTS));
	__asm("bx lr");
}
#pragma GCC diagnostic pop

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wreturn-type"
int __attribute__((naked)) __attribute__((noinline)) SVCFGets(char *str0, int arg1, myFILE fi3) {
	__asm("svc %0" : : "I" (SVC_FGETS));
	__asm("bx lr");
}
#pragma GCC diagnostic pop

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wreturn-type"
void __attribute__((naked)) __attribute__((noinline)) SVCsetRTC(uint32_t hi32) {
	__asm("svc %0" : : "I" (SVC_SET_RTC));
	__asm("bx lr");
}
#pragma GCC diagnostic pop

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wreturn-type"
uint32_t __attribute__((naked)) __attribute__((noinline)) SVCgetRTC(void) {
	__asm("svc %0" : : "I" (SVC_GET_RTC));
	__asm("bx lr");
}
#pragma GCC diagnostic pop

/* This function sets the priority at which the SVCall handler runs (See
 * B3.2.11, System Handler Priority Register 2, SHPR2 on page B3-723 of
 * the ARM®v7-M Architecture Reference Manual, ARM DDI 0403Derrata
 * 2010_Q3 (ID100710)).
 * 
 * If priority parameter is invalid, this function performs no action.
 * 
 * The ARMv7-M Architecture Reference Manual in section "B1.5.4 Exception
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
 * 
 * It is important to set the SVCall priority appropriately to allow
 * or disallow interrupts while the SVCall handler is running. */

void svcInit_SetSVCPriority(unsigned char priority) {
	if (priority > SVC_MaxPriority)
		return;

	/*
	 * MODIFICATION
	 *    The System handler Priority Registers are defined as part of the
	 *    System Control Block (SCB) in file core_cm4.h. The SCB structure is
	 *    documented in Sec B.3.2.1 of the ARM v7-M Arch Ref Manual as a set of
	 *    3 contiguous 32 bit registers (i.e. SHPR1, SHPR2, and SHPR3). Each of
	 *    these registers is structured as four 1-byte fields where each byte is
	 *    specifying the priority for 1 handler (see Sections B.3.2.10 thru
	 *    B.3.2.12). Thus 12 handler priorities may in total be specified.
	 *
	 *      The Kenetis SDK-1 however generates a SCB with a 12 byte array rather
	 *    than three 1-word registers. Thus, SCB_SHPR2 would align with SHP[4]
	 *    Furthermore, the byte for PRI_11 does not need to be obtained via
	 *    the use of a mask but can istead be directly referenced as SCB->SHP[7].
	 *
	 */
	// Original code:
	    SCB_SHPR2 = (SCB_SHPR2 & ~SCB_SHPR2_PRI_11_MASK)
	            | SCB_SHPR2_PRI_11(priority << SVC_PriorityShift);
	// Replacement:
	//SCB->SHP[7] = (priority << SVC_PriorityShift);
}

void svcHandlerInC(struct frame *framePtr);

/* Exception return behavior is detailed in B1.5.8 on page B1-652 of the
 * ARM®v7-M Architecture Reference Manual, ARM DDI 0403Derrata 2010_Q3
 * (ID100710) */

/* When an SVC instruction is executed, the following steps take place:
 * (1) A stack frame is pushed on the current stack (either the main or
 *     the process stack, depending on how the system is configured)
 *     containing the current values of R0-R3, R12, LR, the return
 *     address for the SVC instruction, xPSR, and, if appropriate, the
 *     floating point registers,
 * (2) An appropriate value is loaded into the LR register indicating
 *     that a stack frame is present on the stack (this will cause a
 *     special return sequence later when this value is loaded into
 *     the PC),
 * (3) Values of R0-R3 and R12 are no longer valid,
 * (4) The PC is set to the address in the interrupt vector table for
 * 	   the interrupt service routine for the SVC instruction,
 * (5) The processor switches to Handler mode (code execution in
 *     Handler mode is always privileged),
 * (6) The xPSR is set to indicate appropriate SVC state,
 * (7) The processor switches to the main stack by now using the main
 * 	   stack pointer.
 *     
 * These steps are discussed in detail in the pseudo-code given for
 * processor action ExceptionEntry() on page B1-643 of the ARM®v7-M
 * Architecture Reference Manual, ARM DDI 0403Derrata 2010_Q3
 * (ID100710).  ExceptionEntry() invokes PushStack() and
 * ExceptionTaken() on page B1-643. */

/* The following assembler function makes R0 point to the top-of-stack
 * for the stack that was current (either the main or the process stack)
 * before the SVC interrupt service routine began running.  This is
 * where the stack frame was stored by the SVC instruction.  Then, this
 * function calls the svcHandlerInC function.  Note that when a C
 * function is called, R0 contains the first parameter.  Therefore, the
 * first parameter passed to svcHandlerInC is a pointer to the
 * top-of-stack of the stack containing the stack frame. */

#ifdef __GNUC__
void __attribute__((naked)) svcHandler(void) {
	__asm("\n\
            tst		lr, #4\n\
			ite		eq\n\
			mrseq	r0, msp\n\
			mrsne	r0, psp\n\
			push	{lr}\n\
			bl		svcHandlerInC\n\
			pop		{pc}\n\
			");
}
#else
__asm void svcHandler(void) {
	tst lr,
#4
	mrseq r0, msp
	mrsne r0, psp
	push lr
	bl svcHandlerInC
	pop pc
}
#endif

int SVCpcbInitImpl(uint16_t frequ, void (*func_p)()) {
	if((1000 >= frequ) && (50 <= frequ)){
		PDB0Init(((float) frequ)*23.438, PDBTimerOneShot);
		setFunc(func_p);
	} else {
		uartPuts(UART2_BASE_PTR,
				"PCBInit::invalid delay given. Only allows delays from 1 sec to 50 ms. Please enter value in millisec.\r\n");
	}
}

void SvcGetClockImpl(uint32_t *h, uint32_t *l){
	*h = flexTimerGetClockHi();
	*l = flexTimerGetClockLow();
}

int SvcSpawnImpl(utlErrno_t main(int argc, char *argv[]), shArg_t *arg,
		uint32_t stackSize, pid_t *spawnedPidPtr){
	return spawn(main, arg->argc, arg->argv, stackSize, spawnedPidPtr);
}

void svcHandlerInC(struct frame *framePtr) {
	/* framePtr->returnAddr is the return address for the SVC interrupt
	 * service routine.  ((unsigned char *)framePtr->returnAddr)[-2]
	 * is the operand specified for the SVC instruction. */

	switch (((unsigned char *) framePtr->returnAddr)[-2]) {
	case SVC_FOPEN:
		framePtr->fi1 = myfopen(framePtr->str0, framePtr->str1);
		break;
	case SVC_FCLOSE:
		framePtr->returnVal = myfclose(framePtr->fi1);
		break;
	case SVC_CREATE:
		framePtr->returnVal = mycreate(framePtr->str0, framePtr->str1);
		break;
	case SVC_DELETE:
		framePtr->returnVal = mydelete(framePtr->str0);
		break;
	case SVC_REWIND:
		framePtr->returnVal = myrewind(framePtr->fi1);
		break;
	case SVC_FPUTC:
		framePtr->returnVal = myfputc(framePtr->c, framePtr->fi2);
		break;
	case SVC_FGETC:
		framePtr->returnVal = myfgetc(framePtr->fi1);
		break;
	case SVC_MALLOC:
		framePtr->address = myMalloc(framePtr->size0);
		break;
	case SVC_FREE:
		framePtr->err = myFreeErrorCode(framePtr->address, 0);
		break;
	case SVC_PRINTSTR:
		di();
		printStr(framePtr->str0);
		ei();
		break;
	case SVC_GETCHAR:
		di();
		framePtr->c = getChar(true);
		ei();
		break;
	case SVC_PDB:
		SVCpcbInitImpl(framePtr->frequ, framePtr->func_p);
		break;
	case SVC_PDB_START:
		PDB0Start();
		break;
	case SVC_SET_CLOCK:
		di();
		flexTimerSetClock(framePtr->hi32, framePtr->low32);
		ei();
		break;
	case SVC_GET_CLOCK:
		di();
		SvcGetClockImpl(framePtr->hi32_p, framePtr->low32_p);
		ei();
		break;
	case SVC_SPAWN:
		framePtr->returnVal = SvcSpawnImpl(framePtr->main, framePtr->arg,
				framePtr->stackSize, framePtr->spawnedPidPtr);
		break;
	case SVC_YIELD:
		yield();
		break;
	case SVC_BLOCK:
		block();
		break;
	case SVC_BLOCK_PID:
		framePtr->returnVal = blockPid(framePtr->targetPid);
		break;
	case SVC_WAKE:
		framePtr->returnVal = wake(framePtr->targetPid);
		break;
	case SVC_KILL:
		framePtr->returnVal = myKill(framePtr->targetPid);
		break;
	case SVC_WAIT:
		wait(framePtr->targetPid);
		break;
	case SVC_PID:
		framePtr->targetPid = pid();
		break;
	case SVC_GETCHAR_NOECH:
		di();
		framePtr->c = getChar(false);
		ei();
		break;
	case SVC_FPUTS:
		framePtr->returnVal = myfputs(framePtr->str0, framePtr->fi2);
		break;
	case SVC_FGETS:
		framePtr->returnVal = myfgets(framePtr->str0, framePtr->arg1, framePtr->fi3);
		break;
	case SVC_SET_RTC:
		rtc_set(framePtr->hi32);
		break;
	case SVC_GET_RTC:
		framePtr->hi32 = get_time();
		break;
	default:
		uartPuts(UART2_BASE_PTR, "Unknown SVC has been called\r\n");
	}

}
