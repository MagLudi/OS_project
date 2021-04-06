#ifndef SHL_
#define SHL_


#include <stdlib.h>
#include <stdio.h>
#include "utl.h"
#include "pcb.h"


/* type definitions */
typedef struct arg{
	int argc;
	char **argv;
} shArg_t;

typedef struct args{
	int argc;
	uint32_t stackSize;
} arg_t;

typedef struct var{
	char *name_p;
	char *valString_p;
	union{
		char *string;
		int integer;
		double floating;
	} val;
} shVar_t;
  

/* macros */
#define shWhiteSPACE(A) ((A == ' ') || (A == '\t') || (A == '\n') || (A == '\r'))


/* global variable declarations */
#ifndef ALLOCATE_
 #define EXTERN_ extern
 extern PcbLink *shPcbLink_p;
#else
 #define EXTERN_
 PcbLink *shPcbLink_p = NULL;
#endif  /* ALLOCATE_ */

EXTERN_ utlLnkdListHdr_t shVarList;
PcbLink *firstLink_p; //variable for the first pcb (the shell)


/* function declarations */

utlStatus_t shInit (void);
utlStatus_t shProcessCmdLine(void);
utlStatus_t shExecuteCmd(int argc, char *argv_p[]);
utlErrno_t shParseCmdLine(char *line_p, shArg_t *arg_p);
char * shReset_p (void);
char * shGetWord_p(char **line_p);
void shDone (char *line_p, shArg_t *arg_p );
void shGetCmdLine (char *line_p);
#endif /* SHL_ */
