/* pid.c contains process related utilities */

/* sys include files */
#include <sys/time.h>
#include <stdio.h>
#include <stdlib.h>
#include <limits.h>

/* project headers  */
#include "utl.h"
#include "pcb.h"
#include "fio.h"
#include "mem.h"
#include "led.h"
#include "pushbutton.h"
#include "lcdc.h"
#include "lcdcConsole.h"
#include "adc.h"
#include "ctp.h"
#include "priv.h"
#include "shell.h"
#include "svc.h"
#include "usr.h"
#include "fioutl.h"

extern void *memAlloc(unsigned size, int pId);

bool pcbInitialized = false;
ProcessControlBlock *PCB_p = NULL;
int nextPID = 1;

void end(void);

/* returns the next available pid and updates */
int getNextPID(void){
	di();
	int i = nextPID;
	nextPID++;
	ei();
	return i;
}
/* get PID */
int pid(void) {
	if (PCB_p){
		return (PCB_p->processID);
	}
	return -1;
}

/* init process control block
 * returns FIO_NO_ERROR or FIO_INIT_FAILED if not successful
 */
FioErrno pcbInit(ProcessControlBlock *pcb_p, int pId) {
	PCB_p = pcb_p;

	if(pId = 0){
		PCB_p->processID = pid();
	}

	/* init preset streams */
	PCB_p->stream = (Stream *) memAlloc(sizeof(Stream) * FIO_MAX_STREAMS, PCB_p->processID);
	if (!PCB_p->stream) {
		return FIO_INIT_FAILED;
	}

	PCB_p->currentDirInode = -1;

	/* init stream table */
	uint8_t j;
	for (j = 0; j < FIO_MAX_STREAMS; j++) {
		PCB_p->stream[j].device_p = NULL;
		PCB_p->stream[j].nextFreeIdx = j + 1;
	}

	/* no stream entries are created for directories, unless openDir is called */
	/* since LAST_RESERVED_TYPE is ROOT, we can use that entry for reg files */
	PCB_p->firstFreeStream = LAST_RESERVED_TYPE + 1;

	/* create streams for reserved devices/files */

	myFILE fi;

	/* set root dir; current dir will be set to root  */

	fi = myfopen("/", "r+");
	if (fi != ROOT_DIR){
		return (FIO_INIT_FAILED);
	}
	/* NOTE: as long as dir exists in inode table it does not have to have a stream to be associated with it */
	PCB_p->currentDirInode =  ROOT_DIR;

	/* setup directories for hw devices */

	myfopen("/dev/", "r+");
	myfopen("/dev/led/", "r+");
	myfopen("/dev/pb/", "r+");
	myfopen("/dev/analog/", "r+");
	myfopen("/dev/ts/", "r+");

	/* open/create std file nodes */
	fi = myfopen("STDIN", "a"); /* don't need to save file pointer, since std has a reserved slot in stream table */
	if (fi != STDIN){
		return (FIO_INIT_FAILED);
	}
	fi = myfopen("STDOUT", "a"); /* don't need to save file pointer, since std has a reserved slot in stream table */
	if (fi != STDOUT){
		return (FIO_INIT_FAILED);
	}
	fi = myfopen("STDERR", "a"); /* don't need to save file pointer, since std has a reserved slot in stream table */
	if (fi != STDERR){
		return (FIO_INIT_FAILED);
	}

	pcbInitialized = true;

	return FIO_NO_ERROR;

}

/* get the index of the first free stream */
uint8_t pcbGetFreeStreamIdx(void) {
	uint8_t streamIdx = PCB_p->firstFreeStream;

	if (PCB_p->firstFreeStream < FIO_MAX_STREAMS) {
		PCB_p->firstFreeStream = PCB_p->stream[PCB_p->firstFreeStream].nextFreeIdx;
		PCB_p->stream[streamIdx].nextFreeIdx = FIO_MAX_STREAMS;
	}
	return (streamIdx);
}

/* new free stream is set to be the first free stream */
void pcbReleaseStreamIdx(uint8_t streamIdx) {
	if (streamIdx < FIO_MAX_STREAMS){
		/* check for valid index */
		PCB_p->stream[streamIdx].device_p = NULL;
		PCB_p->stream[streamIdx].id[0] = '\0';
		PCB_p->stream[streamIdx].position.currBlock_p = NULL;
		PCB_p->stream[streamIdx].position.offset = 0;

		PCB_p->stream[streamIdx].nextFreeIdx = PCB_p->firstFreeStream;
		PCB_p->firstFreeStream = streamIdx;
	}
}

ProcessControlBlock *getCurrentPCB(void) {
	return PCB_p;
}

void setCurrentPCB(ProcessControlBlock *pcb_p){
	PCB_p = pcb_p;
}

/* adds a link to a circular list */
PcbLink *pcbAdd(pid_t pid, ProcessState state, UtlAddress_t sp, int size,
		PcbLink *first_p) {

	PcbLink *pcbLink_p = memAlloc(sizeof(PcbLink), (int) pid);
	if (!pcbLink_p){
		return NULL;
	}
	/* set PCB structure */

	pcbLink_p->pcb.processID = pid;
	pcbLink_p->pcb.state = state;
	pcbLink_p->pcb.sp = sp;
	pcbLink_p->pcb.stackSize = size;
	pcbLink_p->pcb.cpuTime = 0;

	if (first_p) {
		di();
		pcbLink_p->prev_p = first_p->prev_p;
		pcbLink_p->next_p = first_p;
		pcbLink_p->prev_p->next_p = pcbLink_p;
		first_p->prev_p = pcbLink_p;
		ei();

	} else {
		/* first link created (shell PCB); points to itself */
		pcbLink_p->next_p = pcbLink_p->prev_p = pcbLink_p;
	}

	return pcbLink_p;
}

/* remove a link from a circular list */
void pcbRemove(PcbLink *pcbLink_p) {
	if (!pcbLink_p){
		return;
	}

	pcbLink_p->prev_p->next_p = pcbLink_p->next_p;
	pcbLink_p->next_p->prev_p = pcbLink_p->prev_p;
}

void pcbExit(ProcessControlBlock *pcb_p) {
	if (pcb_p == NULL){
		return;
	}

	pcbCloseStreams(pcb_p);
	di();
	memExit(pcb_p->processID);
	ei();
}

/* close all streams */
void pcbCloseStreams(ProcessControlBlock *pcb_p) {
	if (!pcb_p) {
		pcb_p = getCurrentPCB();
	}

	myFILE fi = FIO_MAX_STREAMS - 1;
	for (; fi >= 0; fi--) {
		if (pcb_p->stream[fi].device_p != NULL) {
			fileCloseX(fi, pcb_p);
		}
	}

	pcb_p->fiLog = -1;
}

void pcbCloseUsrStreams(ProcessControlBlock *pcb_p) {
	if (!pcb_p){
		pcb_p = getCurrentPCB();
	}

	/* preset devices are opened for the process, not per user, so no need to close them at logout */
	myFILE fi = FIO_MAX_STREAMS - 1;
	for (; fi > LAST_RESERVED_TYPE; fi--) {
		if (pcb_p->stream[fi].device_p != NULL) {
			fileCloseX(fi, pcb_p);
		}
	}

	pcb_p->fiLog = -1;
}

/* spawn a new process. Returns indication of success.
 * The first argument, main, is the function to be run
 * by the newly created process. The second argument is
 * the argc to be passed to main. Argument 3 is the argv
 * to be passed to main. The stackSize argument is the
 * size of the stack to be allocated for the new process.
 * The final argument sets spawnedPid to pid of spawned
 * process.
 */
int spawn(utlErrno_t main(int argc, char *argv[]), int argc, char *argv[],
		uint32_t stackSize, pid_t *spawnedPidPtr) {
	/* checking that stackSize is a multiple of 4 */
	unsigned mod = (stackSize % 4);
	if (mod) {
		stackSize = stackSize + (4 - mod);
	}

	PcbLink *link = pcbAdd(*spawnedPidPtr, BLOCKED, 0, stackSize, firstLink_p);
	if (link == NULL) {
		return -1;
	}

	link->pcb.stackAddress = memAlloc(stackSize, (int) *spawnedPidPtr);
	if (link->pcb.stackAddress == NULL) {
		return -1;
	}

	uint32_t *tp = (uint32_t *) ((link->pcb.stackAddress) + stackSize);
	link->pcb.stackPointer = --tp;

	/*loading the stack*/
	*(link->pcb.stackPointer) = 0x01000000; /* XPSR set thumb bit */
	*(--link->pcb.stackPointer) = (uint32_t) main; /* Return address - function pointer to start this process */
	*(--link->pcb.stackPointer) = (uint32_t) end; /* LR return to main stack, thread mode, basic frame */

	*(--link->pcb.stackPointer) = 12; /* R12 */
	*(--link->pcb.stackPointer) = 3; /* R3 */
	*(--link->pcb.stackPointer) = 2; /* R2 */
	*(--link->pcb.stackPointer) = (uint32_t) argv; /* R1 - argv */
	*(--link->pcb.stackPointer) = argc; /* R0 - argc */

	*(--link->pcb.stackPointer) = 0xFFFFFFF9; /* LR that would be pushed for ISR, handler mode, main stack, basic frame */
	*(--link->pcb.stackPointer) = 0; /* R7 - would be pushed by systick ISR (and set to stack pointer) */
	*(--link->pcb.stackPointer) = 0; /* spCopy */
	*(--link->pcb.stackPointer) = 0;

	uint32_t *systicR7 = link->pcb.stackPointer;

	*(--link->pcb.stackPointer) = 11; /* R11 */
	*(--link->pcb.stackPointer) = 10; /* R10 */
	*(--link->pcb.stackPointer) = 9; /* R9 */
	*(--link->pcb.stackPointer) = 8; /* R8 */
	*(--link->pcb.stackPointer) = (uint32_t) systicR7; /* R7 */
	*(--link->pcb.stackPointer) = 6; /* R6 */
	*(--link->pcb.stackPointer) = 5; /* R5 */
	*(--link->pcb.stackPointer) = 4; /* R4 */

	*(--link->pcb.stackPointer) = 0; /* SVCALLACT */

	/* final initializations */
	pcbInit(&link->pcb, (int) *spawnedPidPtr);
	uint32_t h, l;
	di();
	SvcGetClockImpl(&h, &l);
	ei();
	link->pcb.startTime = (((uint64_t) h) << 32) | ((uint64_t) l);
	link->pcb.state = READY;
	return 0;
}

/* yields remaining quantum */
void yield(void){
	SCB_ICSR |= SCB_ICSR_PENDSVSET_MASK;
}

/* sets the current process to blocked state */
void block(void){
	PCB_p->state = BLOCKED;
	di();
	yield();
	ei();
}

/* sets the targetPid process to blocked state.
 * Returns indication of success.
 */
int blockPid(pid_t targetPid){
	int success = -1;

	if(PCB_p->processID == targetPid){
		block();
		success = 0;
	}else{
		ProcessControlBlock *pcb_p = findPCB(targetPid);
		if(pcb_p != NULL){
			if(pcb_p->state != KILL){
				pcb_p->state = BLOCKED;
				success = 0;
			}
		}
	}
	return success;
}

/* sets the targetPid process to ready state.
 * Returns indication of success
 */
int wake(pid_t targetPid){
	int success = -1;
	ProcessControlBlock *pcb_p = findPCB(targetPid);

	if (pcb_p != NULL) {
		if (pcb_p->state == BLOCKED) {
			pcb_p->state = READY;
			success = 0;
		}
	}
	return success;
}

/* prematurely terminates the targetPid process.
 * Returns indication of success
 */
int myKill(pid_t targetPid){
	int success = -1;
	ProcessControlBlock *pcb_p = findPCB(targetPid);
	if (pcb_p != NULL) {
		uint32_t h, l;
		di();
		SvcGetClockImpl(&h, &l);
		ei();
		pcb_p->endTime = (((uint64_t) h) << 32) | ((uint64_t) l);
		pcb_p->state = KILL;
		success = 0;
	}
	return success;
}

/* waits for the targetPid process to end execution
 * (naturally or prematurely) */
void wait(pid_t targetPid){
	ProcessControlBlock *pcb_p = findPCB(targetPid);
	if (pcb_p != NULL) {
		while (pcb_p->state != KILL) {;}
	}
}

ProcessControlBlock *findPCB(pid_t targetPid){
	pid_t start = shPcbLink_p->pcb.processID;
	PcbLink *link_p = shPcbLink_p->next_p;

	while(link_p->pcb.processID != start){
		if(link_p->pcb.processID == targetPid){
			return &link_p->pcb;
		}
		link_p = link_p->next_p;
	}
	return NULL;
}

/*current PCB has finished execution naturally*/
void end(void){
	uint32_t h, l;
	SVCgetClock(&h, &l);
	PCB_p->endTime = (((uint64_t) h) << 32) | ((uint64_t) l);
	PCB_p->state = KILL;
	SVCyield();
	while(1){;}
}

void pcbReleaseStreamIdxX(uint8_t streamIdx, ProcessControlBlock *pcb_p) {
	if (!pcb_p) {
		pcb_p = getCurrentPCB();
	}

	/* check for valid index */
	if (streamIdx < FIO_MAX_STREAMS) {
		pcb_p->stream[streamIdx].device_p = NULL;
		pcb_p->stream[streamIdx].id[0] = '\0';
		pcb_p->stream[streamIdx].position.currBlock_p = NULL;
		pcb_p->stream[streamIdx].position.offset = 0;

		pcb_p->stream[streamIdx].nextFreeIdx = PCB_p->firstFreeStream;
		pcb_p->firstFreeStream = streamIdx;
	}
}
