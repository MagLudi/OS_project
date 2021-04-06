#ifndef UTL_
#define UTL_


#include <sys/time.h>
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <float.h>
#include <string.h> 
#include <stdbool.h>
#include <errno.h>
#include <limits.h>
#include <stdint.h>
#include "uart.h"
#include "lcdcConsole.h"

/* enums */

/* status enums */
typedef enum{
	utlFAIL = 0,
	utlSUCCESS,
	utlERROR
} utlStatus_t ;

/* error type enums */
typedef enum{
	utlNoERROR = 0,
	utlBaseERROR = 100,
	utlMemERROR = utlBaseERROR,
	utlNoInputERROR,
 	utlInvCmdERROR,
	utlArgNumERROR,
	utlArgValERROR,
	utlArgFormatERROR,
	utlFailERROR,
	utlSetVarERROR,
	utlVarNotFoundERROR,
	utlNoCmdERROR,
	utlValSubERROR,
	utlPrivERROR,
	utlInvUserNameERROR,
	utlInvGrpNameERROR,
	utlInvPasswordERROR,
	utlMaxUsersERROR,
	utlNoSuchUserERROR,
	utlNotUniqueUserERROR,
	utlReservedERROR,
	utlMaxERROR = utlReservedERROR
} utlErrno_t;

/* month enums */
typedef enum {
	JAN = 0,
	FEB,
	MAR,
	APR,
	MAY,
	JUN,
	JUL,
	AUG,
	SEP,
	OCT,
	NOV,
	DEC
} utlMonth_et;

/* constants */
#define shMAX_BUFFERSIZE  256

/* data structure for errors */
typedef struct utlError{
	void (*fprintf_p)(char *message_p, utlErrno_t errNum);
} utlError_t;

/* linked list data structures */
typedef struct linkedListNode{
	void *data_p;
	int flag;
	struct linkedListNode *prev_p;
	struct linkedListNode *next_p;
} utlLnkdListNode_t;

typedef struct{
	utlLnkdListNode_t *firstNode_p;
	utlLnkdListNode_t *lastNode_p;
} utlLnkdListHdr_t;

/* date/type types */
typedef struct month{
	int days;
	char *name_p;
} utlMonth_t;

typedef struct date{
	unsigned year;
	char *month_p;
	unsigned month;
	unsigned day;
	unsigned hour;
	unsigned min;
	unsigned sec;
	unsigned msec;
	bool set;
} utlDate_t;

/* machine dependent configuration; will be checked at compile time*/
/* typedef uint32_t UtlAddress_t; */
typedef uint32_t UtlAddress_t;
typedef int32_t UtlInt_t;
#define UTL_ADDR_SIZE_MAX ((sizeof(UtlAddress_t)) * CHAR_BIT)
#define UTL_ADDR_MAX UINT32_MAX
#define UTL_INT_MAX INT32_MAX
#define UTL_INT_MIN INT32_MIN
#define UTL_MAX_DIGITS 21

/* date/time constants */
#define utlSMIN 60
#define utlSHOUR (60*utlSMIN)
#define utlSDAY (24*utlSHOUR)
#define utlSYEAR (365*utlSDAY)
#define utlSLYEAR (utlSYEAR + utlSDAY)
#define utlMonthStart 1
#define utlYearStart 1970

/* global variable declarations */
#ifndef ALLOCATE_
 #define EXTERN_ extern
#else
 #define EXTERN_
#endif  /* ALLOCATE_ */

EXTERN_ char *utlProgramName_p;
EXTERN_ utlStatus_t utlStatus;
EXTERN_ char *utlErrorMsg_a[utlMaxERROR-utlBaseERROR];
EXTERN_ utlErrno_t utlErrno;
EXTERN_  struct console console;


/* macros */

#define utlRETURN(S,E,F) {utlErrno=E;  utlPrintERROR(F); return(S);}
#define utlVRETURN(E,F) {utlErrno=E;  utlPrintERROR(F); return;}
#define utlEXIT(F) {utlErrno = utlFailERROR; utlPrintERROR(F); exit(0);} 
#define utlLeap(N) (!(N%400) || (!(N%4) && (N%100 )))
#define utlPrintTIME(M,D,Y,H,MI,S,MS) {char str[shMAX_BUFFERSIZE+1]; snprintf(str, shMAX_BUFFERSIZE,"%s  %02d, %d %02d:%02d:%02d.%u\r\n", M,D,Y,H,MI,S,MS); SVCprintStr(str);}

/* error macros */
#define utlValidERRNO(A) ((A >= utlBaseERROR) && (A <= utlMaxERROR))
#define utlPrintERROR(F) {char str[shMAX_BUFFERSIZE+1]; if (utlValidERRNO(utlErrno)) { snprintf(str, shMAX_BUFFERSIZE, utlErrorMsg_a[utlErrno-utlBaseERROR],F, utlErrno);} else {snprintf(str, shMAX_BUFFERSIZE,utlErrorMsg_a[0],F);} SVCprintStr(str);}
#define utlPrintSysERROR(F) {SVCprintStr(F); SVCprintStr("::"); SVCprintStr(strerror(errno)); SVCprintStr("\r\n");}

/* function declarations */
void utlInit(void);

/* sys/lib functions 'replicated' to be used for this assignment */
unsigned utlAtoI(char *string_p) ; /* converts char string to int; from The C Programming Language by Kernighan and Ritchie */
bool utlStrCmp(char *str1_p, char *str2_p);/* compares char strings; from The C Programming Language by Kernighan and Ritchie */
void utlStrCpy(char *str1_p, char *str2_p);
void utlStrNCpy(char *str1_p, char *str2_p, int n);
int utlStrLen(char *str_p);
int utlCharFind(char *str_p, char c, int n);
unsigned utlPower(unsigned x, unsigned n);
UtlAddress_t utlAtoD(char *string_p);
UtlInt_t utlADtoD(char *string_p);
UtlAddress_t utlADtoUD(char *string_p);
UtlAddress_t utlAOtoD(char *string_p);
UtlAddress_t utlAXtoD(char *string_p);
bool utlStrNCmp(char *str1_p, char *str2_p, int n);
bool utlStrMCmp(char *str1_p, char *str2_p, int n, int m);
void utlStrMCpy(char *str1_p, char *str2_p, int n, int m);
void utlStrReverse(char *str_p);
char * utlItoOA(UtlAddress_t octalNum);

/* linked list utilities */
utlStatus_t utlCreateLnkdList(utlLnkdListHdr_t *list_p);
utlLnkdListNode_t *utlAppendLink_p (void *data_p, int flag,  utlLnkdListHdr_t *list_p);
utlLnkdListNode_t *utlRemoveLink_p (utlLnkdListNode_t *node_p, utlLnkdListHdr_t *list_p);
utlLnkdListNode_t *utlInsertLink_p (void *data_p, int flag, utlLnkdListNode_t *prevNode_p, utlLnkdListHdr_t *list_p);

/* date/time utilities */
void utlSetYear(time_t *tv_sec, utlDate_t *date_p);
void utlSetMonthDayTime(time_t *tv_sec, utlDate_t *date_p);
utlDate_t utlGetDate(void);
void utlUtcYear(utlDate_t *date_p, time_t *tv_sec);
void utlUtcMonthDayTime(utlDate_t *date_p, time_t *tv_sec);

/* Interrupt enablers/disablers */
void ei();
void di();

#endif /* UTL_ */
