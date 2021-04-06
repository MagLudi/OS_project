#ifndef PCB_
#define PCB_

/* systtem headers  */
#include <sys/time.h>
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <float.h>
#include <string.h> 
#include <stdbool.h>
#include "utl.h"
#include "fio.h"

/* constants */
typedef uint32_t Pid_t;

/* type definitions */
typedef enum {
	READY,
	RUNNING,
	BLOCKED,
	KILL
} ProcessState;

typedef struct {
	pid_t processID;
	ProcessState state;
	uint32_t sp;
	uint32_t *stackPointer;
	uint32_t *function;
	void *stackAddress;
	int stackSize;
	/* clock time in millis */
	uint64_t startTime;
	uint64_t endTime;
	uint64_t cpuTime;
	/* fio information */
	int currentDirInode;
	Stream *stream;
	uint8_t firstFreeStream;
	myFILE fiLog;
} ProcessControlBlock;

/* link definition for circular linked list of PCBs */
typedef struct PcbLink_s {
	ProcessControlBlock pcb;
	struct PcbLink_s *prev_p;
	struct PcbLink_s *next_p;
} PcbLink;

/* global variable declarations */

#ifndef ALLOCATE_
#define EXTERN_ extern

#else
#define EXTERN_
#endif  /* ALLOCATE_ */

#define NO_PID -1
#define STACK_SIZE 2048 //default stack size

/* function declarations */

ProcessControlBlock *getCurrentPCB(void);
pid_t pid(void);							/* returns pid of current process */
FioErrno pcbInit(ProcessControlBlock *pcb_p, int pId);
void setCurrentPCB(ProcessControlBlock *pcb_p); /* when scheduler triggers, this updates*/

int spawn(utlErrno_t main(int argc, char *argv[]), int argc, char *argv[],
		uint32_t stackSize, pid_t *spawnedPidPtr);
								/* returns indication of success */
								/* main is the function to be run by the
				   	   	   	   	   newly created process */
								/* argc is the argc to be passed to main */
								/* argv is the argv to be passed to main; */

								/* stackSize is the size of the stack to be
				   	   	   	   	   allocated for the new process */
								/* sets spawnedPid to pid of spawned process */
void yield(void);				/* yields remaining quantum */
void block(void);				/* sets the current process to blocked state */
int blockPid(pid_t targetPid);  /* sets the targetPid process to blocked state */
								/* returns indication of success */
int wake(pid_t targetPid);		/* sets the targetPid process to ready state */
								/* returns indication of success */
int myKill(pid_t targetPid);	/* prematurely terminates the targetPid process */
								/* returns indication of success */
void wait(pid_t targetPid);		/* waits for the targetPid process to end execution
								   (naturally or prematurely) */

uint8_t pcbGetFreeStreamIdx(void);
void pcbReleaseStreamIdx(uint8_t streamIdx);

PcbLink *pcbAdd(pid_t pid, ProcessState state, UtlAddress_t sp, int size, PcbLink *last_p);
void pcbRemove(PcbLink *pcbLink_p);

void pcbExit(ProcessControlBlock *pcb_p);
void pcbCloseStreams (ProcessControlBlock *pcb_p);
void pcbCloseUsrStreams (ProcessControlBlock *pcb_p);

ProcessControlBlock *findPCB(pid_t targetPid);
int getNextPID(void);

void pcbReleaseStreamIdxX(uint8_t streamIdx,ProcessControlBlock *pcb_p);

#endif /* PCB_ */
