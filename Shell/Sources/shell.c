/* This program simulates a simple shell. It will prompt input from the user
 * with a "$ " and will accept a line of text up to 256 characters in length,
 * not counting any newline or null termination. From the input line, an
 * integer named "argc" and an array named "argv" will created. The integer
 * integer argc will contain the count of the number of white space separated
 * white space separated fields found in the input line. If text is enclosed in
 * double-quotes, then any whitespace within will be part of the argument.
 * The array argv will contain a list of pointers to copies of each of the
 * fields found in the input line as null-terminated strings. The follwoing
 * features are encluded in the shell:
 * Commands:
 *		date
 *		exit
 *		help
 *		echo
 *		set
 *		unset
 *
 * Escape characters:
 *		\0
 *		\a
 *		\b
 *		\e
 *		\f
 *		\n
 *		\r
 *		\t
 *		\v
 *
 * Functions:
 *		shInit(): initalize the shell
 *		shProcessCmdLine(): read in and process the command line
 *		shExecuteCmd(): execute the command
 *		shParseCmdLine(): parse the command line into its arguments
 *		shReset_p(): initalize storage for reading in the command line
 *		shGetWord_p(): retreive word from string
 *		shDone(): clear memory for next command
 *		shGetCmdLine(): get command line input
 */

#define _POSIX_SOURCE 1

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <float.h> 

#define ALLOCATE_
#include "shell.h"
#include "cmd.h"
#include "utl.h"
#include "mem.h"
#include "derivative.h"
#include "uart.h"
#include "fio.h"
#include "pcb.h"
#include "mcg.h"
#include "sdram.h"
#include "priv.h"
#include "svc.h"
#include "intSerialIO.h"
#include "flexTimer.h"
#include "PDB.h"
#include "systick.h"
#include "led.h"
#include "pushbutton.h"
#include "lcdc.h"
#include "lcdcConsole.h"
#include "adc.h"
#include "ctp.h"
#include "usr.h"
#include "delay.h"
#include "rtc.h"

/* global variables */

/* configuration dependent:
 * checking at the compile if address size for this machine is what
 * is expected as define by utlAddress_t type in utl.h; if not
 * utl.h must be updated to reflect the architecture of the host machine.
 */
_Static_assert(sizeof(void *) == sizeof(uint32_t), "expected 32 bit architecture; update configuration");

void harwareInit();

/* main program. Runs a simple, interactive shell.
 *
 * param: int argc, char **argv
 * return: int
 */
int main(int argc, char **argv) {
const int peripheralClock = 60000000;
	const int KHzInHz = 1000;
	const int baud = 115200;
	utlStatus_t sts = utlSUCCESS;

	/* initialize */
	mcgInit();
	sdramInit();
	rtc_init();
	uartInit(UART2_BASE_PTR, peripheralClock / KHzInHz, baud);
	svcInit_SetSVCPriority(15);
	setPendSV(PendSV_Priority);
	__asm("ldr r0,=73");
	intSerialIOInit();
	flexTimer0Init(1875);
	systickInit();
	sts = shInit();
	if (sts != utlSUCCESS) {
		utlEXIT("shell");
	}

	ei();
	flexTimer0Start();
	harwareInit();
	privUnprivileged();
	while (true) {
		utlStatus_t sts = utlSUCCESS;

		if (!usrLogin()){
			SVCprintStr("\r\n");
			delay(0xe1a7f);
			exit(0);
		}

		/* process command line */
		sts = shProcessCmdLine();
		if (sts != utlSUCCESS) {
			utlPrintERROR("shell");
		}
	}

	exit(0);
}

/* initialize all general utilities and global variables. Returns the error status
 *
 * param: void
 * return: utlStatus_t
 */
utlStatus_t shInit() {
	utlStatus_t sts = utlSUCCESS;

	/* initialize general utilities */
	utlInit();
	memInit();

	/* init registered users table */
	usrInit();

	/* NOTE: spawned processes WILL NOT CALL fioInit */
	FioErrno fioSts;
	fioSts = fioInit();
	if (fioSts != FIO_NO_ERROR)
		return sts;

	/* create the first pcb link:  PCB for shell with pid = 0 */
	shPcbLink_p = pcbAdd(0, RUNNING, 0, 0, NULL);
	if (!shPcbLink_p){
		utlRETURN(utlFAIL, utlFailERROR,
				"shInit: failed to initialize File System");
	}
	firstLink_p = shPcbLink_p;

	fioSts = pcbInit(&(shPcbLink_p->pcb), 0);
	if (fioSts != FIO_NO_ERROR){
		utlRETURN(utlFAIL, utlFailERROR,
				"shInit: failed to initialize PCB");
	}

	/* create log file: any user can write; only grp0 can read/purge */
	int logSts;
	logSts = mycreate(FIO_LOG_FILE, "rwxrwx-w-");
	if (logSts == -1) {
		utlRETURN(utlFAIL, utlFailERROR, "shInit: faied to create Log file");
	}

	/* setup shell variables list */
	sts = utlCreateLnkdList(&shVarList);
	if (sts != utlSUCCESS){
		utlRETURN(utlFAIL, utlFailERROR,
				"shInit: failed to create variable list");
	}

	return (sts);
}

/* after prompting the user for input, reads in the command line and processes it;
 * It breaks the line down into its arguments and then proceeds to execute the command.
 * If an error occurs reading in the user input or executing the command, an error
 * message is printed. Returns the error status.
 *
 * param: void
 * return: utlStatus_t
 */
utlStatus_t shProcessCmdLine() {
	char *cmdLine_p;
	shArg_t arg;
	utlStatus_t sts = utlSUCCESS;
	utlErrno_t cmdSts;

	/* allocate/init memory for command line */

	cmdLine_p = shReset_p();
	if (cmdLine_p == NULL){
		utlRETURN(utlFAIL, utlFailERROR, "shProcessCmdLine");
	}

	SVCprintStr("\r$ ");

	shGetCmdLine(cmdLine_p);

	cmdSts = shParseCmdLine(cmdLine_p, &arg);
	if (cmdSts == utlNoERROR){
		cmdSts = shExecuteCmd(arg.argc, arg.argv);
	}

	shDone(cmdLine_p, &arg);

	return (sts);
}

/* allocates and returns a character pointer to store command line input.
 *
 * param: void
 * return: char *
 */
char * shReset_p() {
	char * line_p;
	line_p = (char *) SVCMalloc(shMAX_BUFFERSIZE + 1);
	if (line_p == NULL) {
		utlPrintSysERROR("shReset_p:: myMalloc");
		utlRETURN(NULL, utlMemERROR, "shReset_p");
	}

	unsigned i;
	for (i = 0; i <= shMAX_BUFFERSIZE; i++){
		line_p[i] = '\0';
	}

	return (line_p);
}

/* command has been processed. The pointers used for reading in the command line
 * and storing the arguments are now being freed.
 *
 * param: char *line_p, shArg_t *arg_p
 * return: void
 */
void shDone(char *line_p, shArg_t *arg_p) {
	if (line_p != NULL){
		SVCFree(line_p);
	}
	
	if (arg_p->argv != NULL){
		SVCFree(arg_p->argv);
	}
	
	utlErrno = utlNoERROR;

}

/* gets the command line input. Can only hold up to 256 characters, not counting newlines.
 *
 * param: char *line_p
 * return: void
 */
void shGetCmdLine(char *line_p) {
	/* check arguments */
	if (line_p == NULL){
		utlVRETURN(utlArgValERROR, "shGetCmdLine");
	}

	bool eol = false;
	unsigned i;
	char *char_p;
	for (i = 0, char_p = line_p; !eol && (i < shMAX_BUFFERSIZE);
			i++, char_p++) {
		*char_p = SVCgetChar();
		if ((*char_p == EOF) || (*char_p == '\n') || (*char_p == '\r')) {
			*char_p = '\0';
			eol = true;
			SVCprintStr("\r\n");
		}

	}
}

/* parses the command line into arguments. Takes in a character pointer representing
 * the input and a pointer to the arg information. Returns the error status.
 * Note: the line_p string must be '\0' terminated
 *
 * param: char *line_p, shArg_t *arg_p
 * return: utlErrno_t
 */
utlErrno_t shParseCmdLine(char *line_p, shArg_t *arg_p) {
	/* check arguments */
	if ((line_p == NULL) || (arg_p == NULL)){
		utlRETURN(utlNoCmdERROR, utlNoCmdERROR, "shParseCmdLine");
	}

	/* set argc  */

	char *input_p = line_p;

	for (arg_p->argc = 0; shGetWord_p(&input_p) != NULL; arg_p->argc++){
		;
	}

	if ((arg_p->argc == 0) || (utlErrno == utlArgValERROR)) {
		arg_p->argv = NULL;
		utlRETURN(utlArgNumERROR, utlArgNumERROR, "shParseCmdLine");
	}

	/* set argv */

	arg_p->argv = SVCMalloc(sizeof(char *) * (arg_p->argc + 1));
	if (arg_p->argv == NULL) {
		arg_p->argc = 0;
		arg_p->argv = NULL;
		utlRETURN(utlArgNumERROR, utlNoCmdERROR, "shParseCmdLine");
	}

	unsigned i;

	for (i = 0; (i < arg_p->argc); i++) {
		arg_p->argv[i] = shGetWord_p(&line_p);
		if (arg_p->argv[i] == NULL){
			utlRETURN(utlArgValERROR, utlErrno, "shParseCmdLine");
		}
	}

	/* final NULL pointer for argv array, as required by C standard */
	arg_p->argv[arg_p->argc] = NULL;

	return (utlNoERROR);

}

/* executes the command given by the user. This command must be the first
 * argument passed in. Takes in the number of arguments and the array of
 * arguments. Returns the error status.
 *
 * param: int argc, char *argv_p[]
 * return: utlErrno_t
 */
utlStatus_t shExecuteCmd(int argc, char *argv_p[]) {
	utlStatus_t sts = utlSUCCESS;

	int i;
	i = cmdMatch(argv_p[0]);
	if (i < 0){
		utlRETURN(sts, utlInvCmdERROR, "shExecuteCmd: command not found");
	}

	sts = cmdExecute(i, argc, argv_p);

	return (sts);
}

/* helper function for parsing the line. Finds the next word in the line
 * (determined by whitespace) and returns it.
 *
 * param: char **line_p[]
 * return: char *
 */
char * shGetWord_p(char **line_p) {
	/* check arguments */
	if ((line_p == NULL) || (*line_p == NULL)){
		utlRETURN(NULL, utlNoInputERROR, "shGetWord_p");
	}

	/* skip leading white spaces */

	for (; (**line_p != '\0') && shWhiteSPACE(**line_p); (*line_p)++){
		;
	}

	unsigned wordLen = 0;
	char *word_p = NULL;
	char *char_p;
	bool quoted = false;
	bool endQuote = false;

	char_p = *line_p;

	/* check if field is quoted */
	if (*char_p == '"') {
		quoted = true;
		endQuote = false;
	}

	if (!quoted){
		for (wordLen = 0; (*char_p != '\0') && !shWhiteSPACE(*char_p);
				wordLen++, char_p++){
			;
		}

	}else{
		for (wordLen = 0; (*char_p != '\0') && !endQuote; wordLen++, char_p++) {
			if ((wordLen > 0) && (*char_p == '"') && (*(char_p - 1) != '"')
					&& (*(char_p - 1) != '\\')
					&& (shWhiteSPACE(*(char_p + 1)) || (*(char_p + 1) == '\0'))){
				endQuote = true;
			}
		};
	}

	if (quoted && !endQuote){
		utlRETURN(NULL, utlArgValERROR, "shGetWord_p");
	}

	if (wordLen != 0) {
		*char_p = '\0'; /*terminate this word*/
		word_p = *line_p;
		*line_p = *line_p + wordLen + 1; /* move pointer to the next word */
	}

	return (word_p);
}

void harwareInit(){
	ledInitAll();
	pushbuttonInitAll();
	lcdcInit();
	lcdcConsoleInit(&console);
	adc_init();
	TSI_Init();
	TSI_Calibrate();
}
