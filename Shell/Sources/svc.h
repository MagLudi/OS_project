/**
 * svc.h
 * Routines for supervisor calls
 *
 * ARM-based K70F120M microcontroller board
 *   for educational purposes only
 * CSCI E-92 Spring 2014, Professor James L. Frankel, Harvard Extension School
 *
 * Written by James L. Frankel (frankel@seas.harvard.edu)
 */

#ifndef _SVC_H
#define _SVC_H

#include "fio.h"
#include "mem.h"
#include "shell.h"

#define SVC_MaxPriority 15
#define SVC_PriorityShift 4

// Implemented SVC numbers

#define SVC_FOPEN 0
#define SVC_FCLOSE 1
#define SVC_CREATE 2
#define SVC_DELETE 3
#define SVC_REWIND 4
#define SVC_FPUTC 5
#define SVC_FGETC 6
#define SVC_MALLOC 7
#define SVC_FREE 8
#define SVC_PRINTSTR 9
#define SVC_GETCHAR 10
#define SVC_PDB 11
#define SVC_PDB_START 12
#define SVC_SET_CLOCK 13
#define SVC_GET_CLOCK 14
#define SVC_SPAWN 15
#define SVC_YIELD 16
#define SVC_BLOCK 17
#define SVC_BLOCK_PID 18
#define SVC_WAKE 19
#define SVC_KILL 20
#define SVC_WAIT 21
#define SVC_PID 22
#define SVC_GETCHAR_NOECH 23
#define SVC_FPUTS 24
#define SVC_FGETS 25
#define SVC_SET_RTC 26
#define SVC_GET_RTC 27


void svcInit_SetSVCPriority(unsigned char priority);
void svcHandler(void);

myFILE SVCFOpen(char *str0, char *str1);
int SVCFClose(myFILE fi1);
int SVCCreate(char *str0, char *str1);
int SVCDelete(char *str0);
int SVCRewind(myFILE fi1);
int SVCFPutc(char c, myFILE fi2);
int SVCFGetc(myFILE fi1);
void *SVCMalloc(unsigned int size0);
MemErrno SVCFree(void *address);
void SVCprintStr(char *str0);
char SVCgetChar();
void SVCpdb(uint16_t frequ, void (*func_p)());
void SVCpdbStart();
void SVCsetClock(uint32_t hi32, uint32_t low32);
void SVCgetClock(uint32_t *hi32_p, uint32_t *low32_p);
int SVCspawn(utlErrno_t main(int argc, char *argv[]), shArg_t *arg,
		uint32_t stackSize, pid_t *spawnedPidPtr);
void SVCyield(void);
void SVCblock(void);
int SVCblockPid(pid_t targetPid);
int SVCwake(pid_t targetPid);
int SVCmyKill(pid_t targetPid);
void SVCwait(pid_t targetPid);
pid_t SVCpid(void);
char SVCgetCharNoEch();
int SVCFPuts(char *str0, myFILE fi2);
int SVCFGets(char *str0, int arg1, myFILE fi3);
void SVCsetRTC(uint32_t hi32);
uint32_t SVCgetRTC(void);

void SvcGetClockImpl(uint32_t *h, uint32_t *l);

#endif /* ifndef _SVC_H */
