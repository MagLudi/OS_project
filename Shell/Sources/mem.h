#ifndef MEM_
#define MEM_

/* System headers */
#include <sys/time.h>
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <float.h>
#include <string.h> 
#include <stdbool.h>
#include <errno.h>
#include "utl.h"
#include "uart.h"
#include "shell.h"
#include "sdram.h"
#include "lcdc.h"

/* constants */
#define WORD 4
#define DWORD (WORD*2)
#define KBYTE 1024
#define MBYTE (KBYTE*KBYTE)
#define MEM_START (SDRAM_START + LCDC_FRAME_BUFFER_SIZE + LCDC_GW_BUFFER_SIZE)
#define MEM_MAX_SIZE (SDRAM_SIZE-(LCDC_FRAME_BUFFER_SIZE+LCDC_GW_BUFFER_SIZE))
#define MEM_END (SDRAM_END+1) /*program treats last address as non working so pushing it to the next*/

/* enums */
typedef enum{
	MEM_NO_ERROR = 0,
	MEM_NOT_ALLOCATED,
	MEM_INSUFFICIENT_SPACE,
	MEM_NOT_INITIALIZED,
	MEM_NOT_OWNED,
	MEM_INV_ADDRESS,
	MEM_INV_SIZE,
	MEM_INV_VAL,
	MEM_BASE_ERROR ,
	MEM_ALLOC_FAILED,
	MEM_DEALLOC_FAILED,
	MEM_FATAL_ERROR,
	MEM_UNRECOGNIZED_ERROR
} MemErrno;

/* type definitions */

typedef struct memFreeLink_s{
	unsigned size;
	struct memFreeLink_s *prev_p;
	struct memFreeLink_s *next_p;
} MemFreeLink ;

typedef struct{
	MemFreeLink *first_p;
	MemFreeLink *last_p;
} MemFreeHeader ;

typedef struct memAllocLink_s{
	pid_t processID ;
	unsigned size;
	struct memAllocLink_s *prev_p;
	struct memAllocLink_s *next_p;
} MemAllocLink ;

typedef struct {
	MemAllocLink *first_p;
	MemAllocLink *last_p;
} MemAllocHeader ;

/* global variable declarations */
#ifndef ALLOCATE_
#define EXTERN_ extern
extern char *strErrno[];
#else
#define EXTERN_
const char *strErrno[] = {"%s:: error num: %d - successful execution\r\n",
						  "%s:: error num: %d - memory was not allocated\r\n",
						  "%s:: error num: %d - insufficient memory for allocation\r\n",
						  "%s:: error num: %d - memory was not initialized\r\n",
						  "%s:: error num: %d - memory not deallocated: owned by another PID\r\n",
						  "%s:: error num: %d - invalid address\r\n",
						  "%s:: error num: %d - invalid size\r\n",
						  "%s:: error num: %d - invalid set value\r\n",
						  "%s:: error num: %d - erro encoutered\r\n",
						  "%s:: error num: %d - memory allocation failed\r\n",
						  "%s:: error num: %d - memory deallocation failed\r\n",
						  "%s:: error num: %d - fatal error encountered\r\n",
						  "%s:: error num: %d - unrecognized error\r\n"};

#endif  /* ALLOCATE_ */

EXTERN_ MemErrno memErrno;

/* function declarations */
void memInit(void);
void *myMalloc (unsigned int size);
void myFree (void *address_p);
MemErrno myFreeErrorCode (void *address_p, int pId);
void memoryMap(void);
MemErrno myMemset (void *address_p, int val, unsigned nbytes);
MemErrno myMemchk (void *address_p, int val, unsigned nbytes);
MemErrno myMemcpy(void *dst_p, const void *src_p, unsigned nbytes);
void memExit(pid_t pid);

/* macros */
#define MEM_ERRNO {if ((memErrno < MEM_NO_ERROR) || (memErrno > MEM_FATAL_ERROR)) memErrno = MEM_FATAL_ERROR;}
#define MEM_PRINT_SYS_ERROR(F) {SVCprintStr(F); SVCprintStr("::"); SVCprintStr(strerror(errno)); SVCprintStr("\r\n");}
#define MEM_PRINT_ERROR(F) {char str[shMAX_BUFFERSIZE+1]; snprintf(str, shMAX_BUFFERSIZE,strErrno[memErrno], F, memErrno); SVCprintStr(str);}
#define MEM_PRINT_ERROR_CODE(F,E) {char str[shMAX_BUFFERSIZE+1]; snprintf(str, shMAX_BUFFERSIZE,strErrno[E], F, E); SVCprintStr(str);}
#define MEM_EXIT(F) {memErrno = MEM_FATAL_ERROR; MEM_PRINT_ERROR(F); exit(0);}
#define MEM_RETURN(E,S) {memErrno=E; return(S);}
#define MEM_VRETURN(E) {memErrno=E;  return;}
#define MEM_RETURN_STR(E,S,F) {memErrno=E;  MEM_PRINT_ERROR(F); return(S);}
#define MEM_VRETURN_STR(E,F) {memErrno=E;  MEM_PRINT_ERROR(F); return;}

#endif /* MEM_ */
