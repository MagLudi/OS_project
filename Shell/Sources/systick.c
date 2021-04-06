#include <stdio.h>
#include <stdint.h>
#include "derivative.h"
#include "systick.h"
#include "nvic.h"
#include "shell.h"
#include "pcb.h"

void setPendSV(unsigned char priority) {
	if (priority > PendSV_MaxPriority) {
		return;
	}
	SCB_SHPR3 = (SCB_SHPR3 & ~SCB_SHPR3_PRI_14_MASK)
			| SCB_SHPR3_PRI_14(priority << PendSV_PriorityShift);
}

void systickInit_SetSystickPriority(unsigned char priority) {
	if (priority > Systick_MaxPriority) {
		return;
	}

	SCB_SHPR3 = (SCB_SHPR3 & ~SCB_SHPR3_PRI_15_MASK)
			| SCB_SHPR3_PRI_15(priority << Systick_PriorityShift);
}

void systickInit() {
	systickInit_SetSystickPriority(Systick_Priority);

	/* must use the processor clock - this might always be set on the K70 */
	SYST_CSR |= SysTick_CSR_CLKSOURCE_MASK;

	/*
	 * Before enabling the SysTick counter, software must
	 * write the required counter value to SYST_RVR, and then write to SYST_CVR.
	 * This clears SYST_CVR to zero. When enabled, the counter reloads
	 * the value from SYST_RVR, and counts down from that value,
	 * rather than from an arbitrary value.
	 */
	SYST_RVR |= SysTick_RVR_RELOAD(PROCESS_QUANTUM);
	/* Any write to the register clears the register to zero. */
	SYST_CVR = 0;

	/* 1 = Count to 0 changes the SysTick exception status to pending. */
	SYST_CSR |= SysTick_CSR_TICKINT_MASK;

	/* enable the counter */
	SYST_CSR |= SysTick_CSR_ENABLE_MASK;
}

/* code from website */

uint32_t *scheduler(uint32_t *oldSP);

/* This section is applicable to the GCC Toolchain.
 *
 * Note that the entry code for this function generates the following
 * assembly code:
 *   push {r4, r7, lr}
 *   sub  sp, sp, #12
 *   add  r7, sp, #0
 *
 * The above code pushes the values of r4, r7, and lr on the stack.
 * r4 is saved because it is used as a temporary register in the
 * generated code.  r7 will be used in the function as a frame pointer
 * to access local automatic variables allocated on the stack.  lr
 * needs to be saved because this function calls other functions
 * (i.e., it is not a leaf function).  It then decrements the sp by
 * 12, thus reserving three 4-byte words on the stack.  Then, it
 * copies the sp into r7.  This allows the generated code to use
 * offsets off of r7 to address automatic variables on the stack.  12
 * bytes are reserved (in addition to the 12 bytes pushed for r4, r7,
 * and lr): (1) one word for copyOfSP, (2) one additional word
 * (unused), and (3) one word in order to maintain double-word
 * alignment (because this function calls other functions).
 *
 * The code generated from "copyOfSP = 0;" is:
 *   mov.w r3, #0
 *   str   r3, [r7, #4]
 *
 * The first line in the code above loads register r3 with the value 0
 * of the expression.  The second line stores register r3 into a word
 * of memory at the address given by the sum of the value in r7 and
 * the constant 4.  This shows us that the compiler is using an offset
 * of 4 off of the fp as the address for the automatic variable
 * copyOfSP.
 *
 * Note that the exit code for this function generates the following
 * assembly code:
 *   add.w r7, r7, #12
 *   mov   sp, r7
 *   pop   {r4, r7, pc}
 *
 * The above code increments r7 by 12 -- this will result in the value
 * that was in the sp just before the entry code of this function was
 * generated.  The value is r7 is copied into the sp -- this
 * effectively pops off the three 4-byte words that were reserved on
 * the stack on entry and restores the sp to its value just before the
 * entry code was executed.  Then, the pop instruction restores the
 * values of r4 and r7 and loads the pc with the value that was in the
 * lr register.  Loading the pc from lr will jump to the return
 * address.
 */

/* This section is applicable to the Freescale Toolchain.
 *
 * Note that the entry code for this function generates the following
 * assembly code:
 *   push {r3,lr}
 *
 * This push instruction saves the value of register LR so that the
 * return address will not be overwritten when other functions (in this
 * case printf) are called.  The push instruction also saves the value
 * of R3 on the stack.  This is *not* done to allow R3 to be saved and
 * restored, but rather so that a single word at the top-of-stack is
 * reserved for my local variable 'copyOfSP'.  If this function were
 * invoked because of an interrupt, the saving and restoring of R3 is
 * already accomplished through the stack frame.
 *
 * The code generated from "copyOfSP = 0;" is:
 *   mov r0,#0
 *   str r0,[sp,#0]
 *
 * The first line in the code above loads register r0 with the value 0
 * of the expression.  The second line stores register r0 into a word
 * of memory at the address given by the sum of the value in sp and
 * the constant 0.  This shows us that the compiler is using an offset
 * of 0 off of the sp as the address for the automatic variable
 * copyOfSP.
 *
 * Note that the exit code for this function generates the following
 * assembly code:
 *   pop {r3,lr}
 *   bx  lr
 *
 * The above code pops the values of r3 and lr off the stack that were
 * pushed by the entry code.  Then, the function returns by jumping to
 * the return address in lr.
 */

/* The PUSH (Push Multiple Registers) instruction is described in
 * the ARMv7-M Architecture Reference Manual ARM DDI 0403E.b
 * (ID120114), Issue E.b, Section A7.7.99 PUSH on page A7-350 */

/* Note that, "The registers are stored in sequence, the
 * lowest-numbered register to the lowest memory address, through to the
 * highest-numbered register to the highest memory address."  This is
 * important for the GCC Toolchain exit code to ensure that the relative
 * position of the LR and PC registers (from the entry push and the exit pop),
 * respectively, is the same.  This is also important for the Freescale
 * Toolchain entry code "push {r3,lr}" because it means that R3 will be on
 * top-of-stack. */

void stHandler() {
	uint32_t *copyOfSP;

	copyOfSP = NULL;

	/* The following assembly language will push registers r4 through
	 * r11 onto the stack */
	__asm("push {r4,r5,r6,r7,r8,r9,r10,r11}");

	/* The following assembly language will push the SVCALLACT and
	 * SVCALLPENDED bits onto the stack (See Application Note AN6:
	 * Interrupt handlers in general and the quantum interrupt handler
	 * in particular) */
	__asm("ldr  r0, [%[shcsr]]" "\n"
			"mov  r1, %[active]" "\n"
			"orr  r1, r1, %[pended]" "\n"
			"and  r0, r0, r1" "\n"
			"push {r0}"
			:
			: [shcsr] "r" (&SCB_SHCSR),
			[active] "I" (SCB_SHCSR_SVCALLACT_MASK),
			[pended] "I" (SCB_SHCSR_SVCALLPENDED_MASK)
			: "r0", "r1", "memory", "sp");

	/* As stated in section B5.1 on page B5-728 of the ARMv7-M
	 * Architecture Reference Manual ARM DDI 0403E.b (ID120114), Issue
	 * E.b, "To support reading and writing the special-purpose
	 * registers under software control, ARMv7-M provides three system
	 * instructions, CPS, MRS, and MSR. */

	/* The MRS (Move to Register from Special Register) instruction is
	 * described in section B5.2.2 on page B5-733 and the MSR (Move to
	 * Special Register from ARM Register) instruction is described in
	 * section B5.2.3 on page B5-735 */

	/* One web site where the syntax for using advanced features in the
	 * 'asm' in-line assembler pseudo-function call is described is:
	 * http://www.ethernut.de/en/documents/arm-inline-asm.html */

	/* The LDR (Load Register) instruction is described in section
	 * A7.7.44 on page A7-256 and the STR (Store Register) instruction
	 * is described in section A7.7.159 on page A7-428 */

	/* The following assembly language will put the current main SP
	 * value into the local, automatic variable 'copyOfSP' */
	__asm("mrs %[mspDest],msp" : [mspDest]"=r"(copyOfSP));

	/* Call the scheduler to find the saved SP of the next process to be
	 * executed */
	copyOfSP = scheduler(copyOfSP);

	/* The following assembly language will write the value of the
	 * local, automatic variable 'copyOfSP' into the main SP */
	__asm("msr msp,%[mspSource]" : : [mspSource]"r"(copyOfSP) : "sp");

	/* The following assembly language will pop the SVCALLACT and
	 * SVCALLPENDED bits off the stack (See Application Note AN6:
	 * Interrupt handlers in general and the quantum interrupt handler
	 * in particular) */
	__asm("pop {r0}" "\n"
			"ldr r1, [%[shcsr]]" "\n"
			"bic r1, r1, %[active]" "\n"
			"bic r1, r1, %[pended]" "\n"
			"orr r0, r0, r1" "\n"
			"str r0, [%[shcsr]]"
			:
			: [shcsr] "r" (&SCB_SHCSR),
			[active] "I" (SCB_SHCSR_SVCALLACT_MASK),
			[pended] "I" (SCB_SHCSR_SVCALLPENDED_MASK)
			: "r0", "r1", "sp", "memory");

	/* The POP (Pop Multiple Registers) instruction is described in
	 * the ARMv7-M Architecture Reference Manual ARM DDI 0403E.b
	 * (ID120114), Issue E.b, Section A7.7.99 PUSH on page A7-348 */

	/* The following assembly language will pop registers r4 through
	 * r11 off of the stack */
	__asm("pop {r4,r5,r6,r7,r8,r9,r10,r11}");

}

uint32_t *scheduler(uint32_t *oldSP) {
	shPcbLink_p->pcb.stackPointer = oldSP; //current pcb
	if (shPcbLink_p->pcb.state != KILL) {
		shPcbLink_p->pcb.state = READY;
	}

	/* determining next process */
	while(1){
		if(shPcbLink_p->next_p->pcb.state == KILL){
			/*clean up*/
			ProcessControlBlock *pcb_p = &shPcbLink_p->next_p->pcb;
			pcbRemove(shPcbLink_p->next_p);
			pcbExit(pcb_p);
		}else{
			shPcbLink_p = shPcbLink_p->next_p;
			if(shPcbLink_p->pcb.state == READY){
				shPcbLink_p->pcb.state = RUNNING;
				setCurrentPCB(&shPcbLink_p->pcb);
				return shPcbLink_p->pcb.stackPointer;
			}
		}
	}
}

