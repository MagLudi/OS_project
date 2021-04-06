/* Contains the source code for the different shell commands. This includes
 * any helper functions needed to complete functionality.
 *
 * Functions:
 *		cmd_date(): prints data
 *		cmd_echo(): echos arguments
 *		cmd_exit(): exit shell
 *		cmd_help(): print valid commands
 *		cmd_set(): set a variable to a particular value
 *		cmd_unset(): unset a variable
 *		cmd_malloc(): allocate some memory of a certain size for use
 *		cmd_free(): deallocate memory for future use
 *		cmd_memorymap(): print a table of free and used memory
 *		cmd_memset(): assigns a value to a certain region of memory
 *		cmd_memchk(): check to see if a certain value is stored in a particular region of memory
 *
 *		cmdExecute(): execute command
 *		cmdAddVar(): add new variable to list of set vars
 *		cmdRemoveVar(): remove set variable
 *		cmdValidateVar(): confirm variable is a set one
 *		cmdMatch(): confirm command is valid
 *		cmdPrintVars(): print the list of set variables
 *		cmdReplaceName_p(): match command to the appropriate function
 *		cmdValidateField_p(): handle quoted strings and escape characters
 *		cmdMoveFieldLeft(): shift all the characters one pointer to the left
 *		cmdValidNum(): confirm if a number (decimal, hex, or octal) was passed
 *
 *		cmd_fopen(): opens a file; if the requested file does not exist, it creates it first
 *		cmd_fclose(): closes a file
 *		cmd_create(): creates a file without opening it
 *		cmd_delete(): deletes a file
 *		cmd_rewind(): rewinds a file
 *		cmd_fputc(): writes a character to a file;
 *		cmd_fgetc(): reads a character from a file
 *		cmd_ls(): lists files in the specified directory
 */

#include <errno.h>
#include <stdio.h>
#include "utl.h"
#include "shell.h"
#include "cmd.h"
#include "mem.h"
#include "uart.h"
#include "fio.h"
#include "fioutl.h"
#include "led.h"
#include "pushbutton.h"
#include "lcdcConsole.h"
#include "adc.h"
#include "ctp.h"
#include "delay.h"
#include "svc.h"
#include "intSerialIO.h"
#include "PDB.h"
#include "tm.h"
#include "pcb.h"
#include "usr.h"
#include "help.h"

const unsigned long int delayCount = 0xe1a7f;

const cmdCommandEntry_t commands[] = {{"date", cmd_date},
									  {"echo", cmd_echo},
									  {"exit", cmd_exit},
									  {"help", cmd_help},
									  {"set", cmd_set},
									  {"unset", cmd_unset},
									  {"malloc", cmd_malloc},
									  {"free", cmd_free},
									  {"memorymap", cmd_memorymap},
									  {"memset", cmd_memset},
									  {"memchk", cmd_memchk},
									  {"fopen",cmd_fopen},
									  {"fclose",cmd_fclose},
									  {"create",cmd_create},
									  {"delete",cmd_delete},
									  {"rewind",cmd_rewind},
									  {"fputc",cmd_fputc},
									  {"fgetc",cmd_fgetc},
									  {"ls",cmd_ls},
									  {"ser2lcd",cmd_ser2lcd},
									  {"touch2led",cmd_touch2led},
									  {"pot2ser",cmd_pot2ser},
									  {"therm2ser",cmd_therm2ser},
									  {"pb2led",cmd_pb2led},
									  {"flashled",cmd_flashled},
									  {"examine",cmd_examine},
									  {"deposit",cmd_deposit},
									  {"multitask",cmd_multitask},
									  {"fputs",cmd_fputs},
									  {"fgets",cmd_fgets},
									  {"adduser",cmd_adduser},
									  {"remuser",cmd_remuser},
									  {"logout",cmd_logout},
									  {"purge",cmd_purge},
									  {"logread",cmd_logread},
									  {"logrewind",cmd_logrewind},
									  {"logpurge",cmd_logpurge},
									  {"spawnFlashGB", cmd_spawnFlashGB},
									  {"killFlashGB", cmd_killFlashGB},
									  {"spawn", cmd_spawn},
									  {"", NULL }};

int toBool(int i);
void flash();
int flashGBpid = -1;

/* function definitions */

/* prints out the current day in Month Day, Year Hour:Minute:Second format
 * in UTC timezone. Takes in an int and a pointer to a char array. Accepts a
 * second argument to set the system clock. Second argument must be in
 * milliseconds from epoch, or in ISO 8601 date/time string (YYY-MM-DDTHH:MM).
 * If an argument is passed in an error message gets displayed.
 *
 * param: int argc, char *argv[]
 * return: utlErrno_t
 */
utlErrno_t cmd_date(int argc, char *argv[]) {
	utlDate_t date;
	utlErrno_t sts = utlNoERROR;

	if(argc == 2){
		/* set date */
		sts = setDate(argc, argv);
	}else if(argc == 1){
		date = tmCurrent(false);

		if (date.set) {
			utlPrintTIME(date.month_p, date.day, date.year, date.hour, date.min, date.sec, date.msec);
		} else {
			sts = utlFailERROR;
		}
	}else{
		sts = utlArgNumERROR;
	}
	return sts;
}

/* sets system date/time;
 * takes in argc and argv; and returns an execution status
 * date/time can be specified either as a number of milliseconds since
 * since our epoch of  midnight (zero hours) on January 1, 1970, or
 * as a implified ISO 8601 date/time string.
 *
 */
utlErrno_t setDate(int argc, char *argv[]) {
	utlErrno_t sts;

	/* check arguments */
	if (argc != 2) {
		return utlArgNumERROR;
	}

	char *dateString_p;
	uint64_t msec = 0;
	unsigned long long t = 0;
	int len = 0;

	/* process the argument */
	dateString_p = cmdReplaceName_p(argv[1]);
	if (dateString_p == NULL) {
		return (utlArgValERROR);
	} else {
		len = utlStrLen(dateString_p);
	}

	/* check for the type of argument: num string or ISO string  */
	if (dateString_p && (*dateString_p == '-')) {
		return (utlArgValERROR);
	}
	sts = cmdValidNum(dateString_p);
	if (sts == utlNoERROR) {
		/* validate num string */
		t = strtoll(dateString_p, (char **)NULL, 10);
		msec = (uint64_t) t;
	} else {
		/* process iso format */
		msec = tmISO(dateString_p);
	}
	if (msec == 0) {
		return (utlArgValERROR);
	}

	/* set system date/time */
	sts = tmSet(msec);

	return sts;
}

/* echo a list of arguments. Takes in an int (number of arguments)
 * and a pointer to an array of characters (the list of arguments)
 * and prints out each argument except the first one - command name.
 * If a set variable is passed in, it will instead print its
 * corresponding value. If no arguments are given then nothing gets
 * printed out. Returns the error status.
 *
 * param: int argc, char *argv[]
 * return: utlErrno_t
 */
utlErrno_t cmd_echo(int argc, char *argv[]) {
	if (argc > 1) {
		int i;
		char *arg_p;

		/* print out each argument */
		for (i = 1; (i < argc); i++) {
			arg_p = argv[i];
			arg_p = cmdValidateField_p(arg_p);
			if (arg_p == NULL) {
				return (utlArgValERROR);
			}

			if (*arg_p == '$') {
				/* set variable passed in. replace with corresponding value */
				if ((arg_p = cmdReplaceName_p(arg_p)) != NULL) {
					arg_p = cmdValidateField_p(arg_p);
				}
			}

			if (arg_p != NULL) {
				SVCprintStr(arg_p);
				SVCprintStr(" ");
			} else {
				utlErrno = utlValSubERROR;
			}
		}
		SVCprintStr("\r\n");
	}
	return utlErrno;
}

/* exit the shell. Takes in an int and a pointer to a char array
 * but doesn't use them beyond checking to make sure that no
 * arguments were given. If an argument is passed in an error
 * message gets displayed and it will return the error status.
 *
 * param: int argc, char *argv[]
 * return: utlErrno_t
 */
utlErrno_t cmd_exit(int argc, char *argv[]) {
	if (argc > 1) {
		return utlArgNumERROR;
	}
	delay(delayCount);
	exit(0);
}

/* print out all the available commands, what they do, and
 * how to use them. Takes in an int and a pointer to a char
 * array but doesn't use them beyond checking to make sure
 * that no arguments were given. If an argument is passed in
 * an error message gets displayed. Returns the error status.
 *
 * param: int argc, char *argv[]
 * return: utlErrno_t
 */
utlErrno_t cmd_help(int argc, char *argv[]) {
	char *mode_p;

	if (argc > 2) {
		return utlArgNumERROR;
	}

	if (argc < 2) {
		SVCprintStr(HELP_HELP);
		return utlNoERROR;
	}

	mode_p = cmdReplaceName_p(argv[1]);
	if (mode_p == NULL) {
		return (utlValSubERROR);
	}

	if (utlStrCmp("demo", mode_p)) {
		/* Demo apps */
		SVCprintStr(HELP_SER2LCD);
		SVCprintStr(HELP_TOUCH);
		SVCprintStr(HELP_POT);
		SVCprintStr(HELP_THERM);
		SVCprintStr(HELP_PB2LED);
		SVCprintStr(HELP_FLASH);
		SVCprintStr(HELP_MULTI);
		SVCprintStr(HELP_FLASHGB_START);
		SVCprintStr(HELP_FLASHGB_STOP);
		return utlNoERROR;
	}

	if (utlStrCmp("mem", mode_p)) {
		/* MemMgmt */
		SVCprintStr(HELP_DEPOSIT);
		SVCprintStr(HELP_EXAM);
		SVCprintStr(HELP_MALLOC);
		SVCprintStr(HELP_FREE);
		SVCprintStr(HELP_MEMMAP);
		SVCprintStr(HELP_MEMSET);
		SVCprintStr(HELP_MEMCHK);
		return utlNoERROR;
	}

	if (utlStrCmp("env", mode_p)) {
		/* Environment */
		SVCprintStr(HELP_DATE);
		SVCprintStr(HELP_ECHO);
		SVCprintStr(HELP_SET);
		SVCprintStr(HELP_UNSET);
		SVCprintStr(HELP_LOG_OUT);
		SVCprintStr(HELP_EXIT);
		return utlNoERROR;
	}

	if (utlStrCmp("admin", mode_p)) {
		/* Sys admin */
		SVCprintStr(HELP_USR_ADD);
		SVCprintStr(HELP_USR_RM);
		SVCprintStr(HELP_LOG_REW);
		SVCprintStr(HELP_LOG_PUR);
		SVCprintStr(HELP_LOG_RD);
		SVCprintStr(HELP_SPAWN);
		return utlNoERROR;
	}

	if (utlStrCmp("file", mode_p)) {
		/* fILE USAGE AND MGMT */
		SVCprintStr(HELP_FOPEN);
		SVCprintStr(HELP_FCLOSE);
		SVCprintStr(HELP_CREATE);
		SVCprintStr(HELP_DELETE);
		SVCprintStr(HELP_REWIND);
		SVCprintStr(HELP_FPUTC);
		SVCprintStr(HELP_FGETC);
		SVCprintStr(HELP_FPUT);
		SVCprintStr(HELP_FGET);
		SVCprintStr(HELP_FPUR);
		SVCprintStr(HELP_PRINT);
		return utlNoERROR;
	}

	if (utlStrCmp("all", mode_p)) {
		SVCprintStr(HELP_DATE);
		SVCprintStr(HELP_ECHO);
		SVCprintStr(HELP_SET);
		SVCprintStr(HELP_UNSET);
		SVCprintStr(HELP_LOG_OUT);
		SVCprintStr(HELP_EXIT);

		SVCprintStr(HELP_SER2LCD);
		SVCprintStr(HELP_TOUCH);
		SVCprintStr(HELP_POT);
		SVCprintStr(HELP_THERM);
		SVCprintStr(HELP_PB2LED);
		SVCprintStr(HELP_FLASH);
		SVCprintStr(HELP_MULTI);
		SVCprintStr(HELP_FLASHGB_START);
		SVCprintStr(HELP_FLASHGB_STOP);

		SVCprintStr(HELP_FOPEN);
		SVCprintStr(HELP_FCLOSE);
		SVCprintStr(HELP_CREATE);
		SVCprintStr(HELP_DELETE);
		SVCprintStr(HELP_REWIND);
		SVCprintStr(HELP_FPUTC);
		SVCprintStr(HELP_FGETC);
		SVCprintStr(HELP_FPUT);
		SVCprintStr(HELP_FGET);
		SVCprintStr(HELP_FPUR);
		SVCprintStr(HELP_PRINT);

		SVCprintStr(HELP_DEPOSIT);
		SVCprintStr(HELP_EXAM);
		SVCprintStr(HELP_MALLOC);
		SVCprintStr(HELP_FREE);
		SVCprintStr(HELP_MEMMAP);
		SVCprintStr(HELP_MEMSET);
		SVCprintStr(HELP_MEMCHK);

		SVCprintStr(HELP_USR_ADD);
		SVCprintStr(HELP_USR_RM);
		SVCprintStr(HELP_LOG_REW);
		SVCprintStr(HELP_LOG_PUR);
		SVCprintStr(HELP_LOG_RD);
		SVCprintStr(HELP_SPAWN);

		return utlNoERROR;
	}
	return (utlValSubERROR);
}

/* create an environmental variable with a particular value. Takes in the number
 * of arguments and the list of arguments. If the number of arguments is not what
 * is expected, an error message is given. If no arguments are given beyond the
 * set command, then a list of all set variable is displayed.
 *
 * param: int argc, char *argv[]
 * return: utlErrno_t
 */
utlErrno_t cmd_set(int argc, char *argv[]) {
	utlErrno_t sts;
	shVar_t *var_p;
	char *name_p;
	int nameLen = 0;

	if (argc > 2) {
		return (utlArgFormatERROR);
	}

	if (argc == 1) {
		/* no variable given. print out all set vars */
		cmdPrintVars();
	} else {
		int fieldLen, varNameLen, valStrLen;
		fieldLen = utlStrLen(argv[1]);
		varNameLen = utlCharFind(argv[1], '=', fieldLen);
		if ((varNameLen <= 0) || (varNameLen == fieldLen - 1)) {
			return (utlArgValERROR);
		}

		argv[1][varNameLen] = '\0';

		name_p = cmdReplaceName_p(argv[1]);

		if (name_p == NULL) {
			utlRETURN(utlValSubERROR, utlValSubERROR, "set")
		} else {
			nameLen = utlStrLen(name_p);
		}

		sts = cmdValidateVar(name_p, nameLen);
		if (sts != utlNoERROR) {
			return (sts);
		}

		valStrLen = fieldLen - varNameLen - 1;

		/* set variable node */
		unsigned size = sizeof(shVar_t);
		var_p = (shVar_t *) SVCMalloc(size);
		myMemset(var_p, 0, sizeof(shVar_t));
		if (var_p == NULL) {
			return (utlMemERROR);
		}

		/* set var name and value */
		size = (nameLen + 1) * sizeof(char);
		var_p->name_p = SVCMalloc(size);
		myMemset(var_p->name_p, 0, size);

		size = (valStrLen + 1) * sizeof(char);
		var_p->valString_p = SVCMalloc(size);
		myMemset(var_p->valString_p, 0, size);
		if ((var_p->name_p == NULL) || (var_p->valString_p == NULL)) {
			return (utlMemERROR);
		}

		utlStrNCpy(var_p->name_p, name_p, nameLen);
		utlStrNCpy(var_p->valString_p, (argv[1] + varNameLen + 1), valStrLen);

		return (cmdAddVar(var_p));
	}
	return (utlNoERROR);
}

/* removes a variable from the list of set variable, thereby making it unset. Takes
 * in the number of arguments and the list of them. If the incorrect number of
 * arguments is given (only the command and the variable name are allowed), or the
 * variable isn't on the list, then an error message is given.
 *
 * param: int argc, char *argv[]
 * return: utlErrno_t
 */
utlErrno_t cmd_unset(int argc, char *argv[]) {
	utlErrno_t sts;
	char *name_p;
	int nameLen = 0;

	if (argc != 2) {
		return (utlArgNumERROR);
	} else {
		name_p = cmdReplaceName_p(argv[1]);
		if (name_p == NULL) {
			return (utlValSubERROR);
		} else {
			nameLen = utlStrLen(name_p);
		}

		/* if invalid var name, it was not on the list */
		sts = cmdValidateVar(name_p, nameLen);
		if (sts != utlNoERROR) {
			return (utlNoERROR);
		}

		return (cmdRemoveVar(name_p));
	}

	return (utlNoERROR);
}

/* checks if a name is an environmental variable, and if so, it is replaced
 * with the corresponding value; otherwise, the name is not changed; if
 * problems are encountered - NULL is returned.
 *
 * param: char *name_p
 * return: char *
 */
char *cmdReplaceName_p(char *name_p) {
	if (name_p == NULL) {
		return NULL;
	}

	if (*name_p == '$') {
		name_p++;
		utlLnkdListNode_t *node_p = NULL;
		shVar_t *varOnList_p;
		bool replaced = false;

		for (node_p = shVarList.firstNode_p; (node_p != NULL) && !replaced;
				node_p = node_p->next_p) {
			varOnList_p = (shVar_t *) node_p->data_p;
			if (utlStrCmp(name_p, varOnList_p->name_p)) {
				/* found the variable, swaping it for its value */
				replaced = true;
				name_p = varOnList_p->valString_p;

			}
		}
		if (!replaced) {
			/* variable not found. returning NULL */
			name_p = NULL;
		}
	}

	return name_p;

}

/* checks if the command given is valid. If it is, its index from the array
 * of commands is returnrd.
 *
 * param: char *cmd_p
 * return: int
 */
int cmdMatch(char *cmd_p) {
	unsigned i;

	if (cmd_p == NULL) {
		return -1;
	}

	if ((cmd_p = cmdReplaceName_p(cmd_p)) == NULL) {
		/* command passed in is an unset var */
		return (-1);
	}

	for (i = 0;
			(commands[i].functionp != NULL)
					&& !utlStrCmp(commands[i].name, cmd_p); i++) {
		;
	}
	if (commands[i].functionp == NULL) {
		/* invalid command was given */
		return (-1);
	}
	return (i);
}

/* calls up a function to execute a corresponding command. Takes in the index of
 * where the command is stored in the commands array, the number of arguments,
 * and the list of arguments.
 *
 * param: int i, int argc, char *argv[]
 * return: utlErrno_t
 */
utlErrno_t cmdExecute(int i, int argc, char *argv_p[]) {
	utlErrno_t sts;

	if (argv_p == NULL) {
		return utlFailERROR;
	}

	sts = (*(commands[i].functionp))(argc, argv_p);
	if (sts != utlNoERROR) {
		utlRETURN(sts, sts, commands[i].name);
	}

	return (sts);
}

/* Adds a new set variable to the list of environmental variables sorted in
 * alphabetical order. Returns the error status
 *
 * param: shVar_t *var_p
 * return: utlErrno_t
 */
utlErrno_t cmdAddVar(shVar_t *var_p) {
	utlLnkdListNode_t *node_p = NULL;

	bool added = false;
	bool match;
	utlErrno_t sts = utlNoERROR;

	if (var_p == NULL) {
		return utlFailERROR;
	}

	/* add var to a sorted list */
	for (node_p = shVarList.firstNode_p; (node_p != NULL) && !added; node_p =
			node_p->next_p) {
		shVar_t *varOnList_p;
		match = true;
		varOnList_p = (shVar_t *) node_p->data_p;
		unsigned int i;

		for (i = 0; match && !added && (var_p->name_p[i] != '\0'); i++) {
			if (var_p->name_p[i] < varOnList_p->name_p[i]) {
				/* found its location on the list. adding the new var */
				utlInsertLink_p((void *) var_p, 1, node_p->prev_p, &shVarList);
				match = false;
				added = true;
			} else if (var_p->name_p[i] > varOnList_p->name_p[i]) {
				match = false;
			}
		}

		if (match) {
			if (varOnList_p->name_p[i] != '\0') {
				/* found its location on the list. adding the new var */
				utlInsertLink_p((void *) var_p, 1, node_p->prev_p, &shVarList);
			} else {
				/* variable has already been set */
				sts = utlSetVarERROR;
			}
			added = true;
		}
	}

	if (!added && !match) {
		/* variable is last in the alphabet */
		utlAppendLink_p((void *) var_p, 1, &shVarList);
	}

	return (sts);

}

/* Print out the list of set environmental variables 
 *
 * param: void
 * return: void
 */

void cmdPrintVars() {
	utlLnkdListNode_t *node_p = NULL;
	shVar_t *var_p;

	for (node_p = shVarList.firstNode_p; node_p != NULL; node_p =
			node_p->next_p) {
		var_p = (shVar_t *) node_p->data_p;
		SVCprintStr(var_p->name_p);
		SVCprintStr("=");
		SVCprintStr(var_p->valString_p);
		SVCprintStr("\r\n");
	}
}

/* takes in a string and removes outer quotes (if present) and replaces special
 * characters with their ACII equivalent.
 *
 * param: char *name_p
 * return: char *
 */
char *cmdValidateField_p(char *name_p) {
	if (name_p == NULL) {
		return NULL;
	}

	int n = utlStrLen(name_p);

	/* strip outer double quotes */
	if (name_p[0] == '"') {
		if (name_p[n - 1] != '"') {
			/* invalid field */
			return NULL;
		}
		/* strip field of quotes */
		name_p[n - 1] = '\0';
		name_p++;
		n = n - 2;
	}

	/* replace double quotes or special characters */
	int i;
	for (i = 0; (i < n) && (name_p[i] != '\0'); i++) {

		if (name_p[i] == '"') {
			cmdMoveFieldLeft(&(name_p[i]), n - i); /* only one double. ommit */
		}

		/* check and adjust for '\' */
		if (name_p[i] == '\\') {
			switch (name_p[i + 1]) {
			case '0': {
				name_p[i + 1] = (char) 0;
				break;
			}
			case 'a': {
				name_p[i + 1] = (char) 7;
				break;
			}
			case 'b': {
				name_p[i + 1] = (char) 8;
				break;
			}
			case 'e': {
				name_p[i + 1] = (char) 27;
				break;
			}
			case 'f': {
				name_p[i + 1] = (char) 12;
				break;
			}
			case 'n': {
				name_p[i + 1] = (char) 10;
				break;
			}
			case 'r': {
				name_p[i + 1] = (char) 13;
				break;
			}
			case 't': {
				name_p[i + 1] = (char) 9;
				break;
			}
			case 'v': {
				name_p[i + 1] = (char) 11;
				break;
			}
			default:
				break;
			}

			cmdMoveFieldLeft(&(name_p[i]), n - i);
		}

	}

	return name_p;

}

/* Shifts all the characters one byte to the left, therby removing the first
 * character in the string.
 *
 * param: char *field_p, int len
 * return: void
 */
void cmdMoveFieldLeft(char *field_p, int len) {
	int j;

	if ((field_p != NULL) && (len != 0)) {
		for (j = 0; (j < len) && (field_p[j] != '\0'); j++) {
			field_p[j] = field_p[j + 1];
		}
	}
}

/* checks to see if a given variable name given is valid. The name must
 * begin with a letter (upper or lower case). Any combination of characters
 * (upper & lower), numbers, and underscores can follow after. Returns an
 * error status.
 *
 * param: char *varName_p, int len
 * return: utlErrno_t
 */
utlErrno_t cmdValidateVar(char *varName_p, int len) {
	int i;
	bool valid = true;

	if (varName_p == NULL) {
		return utlFailERROR;
	}

	if ((*varName_p < 'A') || (*varName_p > 'z')) {
		return (utlArgValERROR);
	}

	for (i = 1; i < len && valid; i++) {
		if (((varName_p[i] < 'A') || (varName_p[i] > 'z'))
				&& ((varName_p[i] < '0') || (varName_p[i] > '9'))
				&& (varName_p[i] != '_')) {
			return (utlArgValERROR);
		}
	}
	return (utlNoERROR);
}

/* removes a set variable from the list of environmental vars.
 * Variable name is passed in, and if it's not on the list,
 * an error is printed.
 *
 * param: char *varName_p
 * return: utlErrno_t
 */
utlErrno_t cmdRemoveVar(char *varName_p) {
	utlLnkdListNode_t *node_p = NULL;
	shVar_t *varOnList_p;
	bool removed = false;

	if (varName_p == NULL) {
		return (utlFailERROR);
	}

	/* remove var from a sorted list */
	for (node_p = shVarList.firstNode_p; (node_p != NULL) && !removed; node_p =
			node_p->next_p) {
		varOnList_p = (shVar_t *) node_p->data_p;
		if (utlStrCmp(varName_p, varOnList_p->name_p)) {
			utlRemoveLink_p(node_p, &shVarList);

			removed = true;
		}
	}

	if (!removed) {
		return utlVarNotFoundERROR;
	}
	return (utlErrno);
}

/* Memory commands */

/* Confirms if a valid number is given. Takes in a char pointer and checks to see if it
 * represents a decimal, hex, or octal number. Returns and error status. decimals could
 * be signed or unsigned, octal and hex are expected to be unsigned.
 *
 * param: char *string_p
 * return: utlErrno_t
 */
utlErrno_t cmdValidNum(char *string_p) {
	char *str_p = string_p;
	int len;
	bool valid = true;
	int i;

	if (string_p == NULL) {
		return (utlArgValERROR);
	}

	len = utlStrLen(string_p);

	if ((*str_p == 'O') || (*str_p == 'o')  || (*str_p == '0')) {
		str_p++;
		len--;
		if (*str_p == 'x') {
			str_p++;
			len--;
			for (i = 0; i < len && valid; i++) {
				if (((str_p[i] < 'A') || ((str_p[i] > 'F') && (str_p[i] < 'a'))
						|| (str_p[i] > 'f'))
						&& ((str_p[i] < '0') || (str_p[i] > '9'))) {
					return (utlArgValERROR);
				}
			}
		} else {
			for (i = 0; i < len && valid; i++) {
				if ((str_p[i] < '0') || (str_p[i] > '8')) {
					return (utlArgValERROR);
				}
			}
		}
	} else {
		if (*str_p == '-') {
			str_p++;
			len--;
		}

		for (i = 0; i < len && valid; i++) {
			if ((str_p[i] < '0') || (str_p[i] > '9')) {
				return (utlArgValERROR);
			}
		}
	}
	return (utlNoERROR);
}

/* allocate a certain amount of memory. Takes in an int (number of
 * arguments) and a pointer to an array of characters (the list of
 * arguments). Takes the second argument (the number of bytes to
 * allocate, which can be represented as decimal, hex, or octal)
 * and calls myMalloc in "mem.c". If the wrong number of arguments
 * are given or the second argument is an invalid number, an error
 * will be returned. Returns the error status indicating the success
 * of myAlloc call.
 *
 * param: int argc, char *argv[]
 * return: utlErrno_t
 */
utlErrno_t cmd_malloc(int argc, char *argv[]) {
	unsigned size;
	void *address_p;

	if (argc != 2) {
		return utlArgNumERROR;
	}

	/* check if valid number string */
	utlErrno_t sts = cmdValidNum(argv[1]);
	if (sts != utlNoERROR) {
		return (utlArgValERROR);
	}

	size = (unsigned) utlAtoD(argv[1]);
	address_p = SVCMalloc(size);
	if (address_p == NULL) {
		return utlFailERROR;
	} else {
		char str[shMAX_BUFFERSIZE + 1];
		snprintf(str, shMAX_BUFFERSIZE, "%p\r\n", address_p);
		SVCprintStr(str);
		return (utlNoERROR);
	}
}

/* free up a region in memory that's previously been allocated.
 * Takes in an int (number of arguments) and a pointer to an
 * array of characters (the list of arguments). Takes the second
 * argument (the address to free up in memory, which can be
 * represented as decimal, hex, or octal) and calls myFreeErrorCode
 * in "mem.c". If the wrong number of arguments are given or the
 * second argument is an invalid number, an error will be given.
 * returns the error status indicating the success of myFreeErrorCode call.
 *
 * param: int argc, char *argv[]
 * return: utlErrno_t
 */
utlErrno_t cmd_free(int argc, char *argv[]) {
	void *address_p;
	MemErrno err;
	utlErrno_t sts;

	if (argc != 2) {
		return utlArgNumERROR;
	}

	/* check if valid number string */
	sts = cmdValidNum(argv[1]);
	if (sts != utlNoERROR) {
		return (utlArgValERROR);
	}

	address_p = (void *) utlAtoD(argv[1]);
	if (address_p == NULL) {
		return (utlFailERROR);
	}

	err = SVCFree(address_p);
	if (err != MEM_NO_ERROR) {
		MEM_PRINT_ERROR_CODE("free", err);
		return (utlFailERROR);
	} else {
		char str[shMAX_BUFFERSIZE + 1];
		snprintf(str, shMAX_BUFFERSIZE, "freed: %p\r\n", address_p);
		SVCprintStr(str);
		return (utlNoERROR);
	}
}

/* outputs a map of both allocated and free memory regions Takes
 * in an int and a pointer to a char array but doesn't use them
 * beyond checking to make that no arguments were given. Calls
 * memoryMap from mem.c to complete execution of the command.
 *
 * param: int argc, char *argv[]
 * return: utlErrno_t
 */
utlErrno_t cmd_memorymap(int argc, char *argv[]) {
	if (argc > 1) {
		return utlArgNumERROR;
	}

	memoryMap();
	return (utlNoERROR);
}

/* assigns a value to a specified region of memory. It takes  three
 * arguments: the start address of a memory region, the value
 * to which each byte in the specified region will be set, and
 * the length (in bytes) of the memory region. Each of these values
 * can be represented as decimal, hex, or octal numbers. After
 * proccessing the arguments, it calls myMemset from mem.c to complete
 * the execution of the command. Returns the error status indicating
 * the success of myMemset call.
 *
 * param: int argc, char *argv[]
 * return: utlErrno_t
 */
utlErrno_t cmd_memset(int argc, char *argv[]) {
	utlErrno_t sts;
	void *address_p;
	int val;
	unsigned size;
	MemErrno err;

	if (argc != 4) {
		return utlArgNumERROR;
	}

	/* check if first arg is valid number string */
	sts = cmdValidNum(argv[1]);
	if (sts != utlNoERROR) {
		return (sts);
	}
	address_p = (void *) utlAtoD(argv[1]);
	if (address_p == NULL) {
		return (utlFailERROR);
	}

	/* check if second arg is valid number string */
	sts = cmdValidNum(argv[2]);
	if (sts != utlNoERROR) {
		return (sts);
	}
	val = (int) utlAtoD(argv[2]);
	if (((val > 0) && (val > UCHAR_MAX)) || ((val < 0) && (val < CHAR_MIN))) {
		return (utlFailERROR);
	}

	/* check if third arg is valid number string */
	sts = cmdValidNum(argv[3]);
	if (sts != utlNoERROR) {
		return (sts);
	}
	size = (unsigned) utlAtoD(argv[3]);

	/* set memory */
	err = myMemset(address_p, val, size);

	if (err != MEM_NO_ERROR) {
		MEM_PRINT_ERROR_CODE("memset", err);
		return (utlFailERROR);
	}

	return (utlNoERROR);
}

/* checks if a requested value is assigned to a specified region
 * of memory. Ttakes three arguments: the start address of a memory
 * region, the value to check for, and the length (in bytes) of the
 * memory region. Each of these values can be represented as decimal,
 * hex, or octal numbers. After proccessing the arguments, it calls
 * myMemchk from mem.c to complete  execution of the command. Returns
 * the error status indicating the success of myMemchk call.
 *
 * param: int argc, char *argv[]
 * return: utlErrno_t
 */
utlErrno_t cmd_memchk(int argc, char *argv[]) {
	utlErrno_t sts;
	void *address_p;
	int val;
	unsigned size;
	MemErrno err;

	if (argc != 4) {
		return utlArgNumERROR;
	}

	/* check if first arg is valid number string */
	sts = cmdValidNum(argv[1]);
	if (sts != utlNoERROR) {
		return (sts);
	}
	address_p = (void *) utlAtoD(argv[1]);
	if (address_p == NULL) {
		return (utlFailERROR);
	}

	/* check if second arg is valid number string */
	sts = cmdValidNum(argv[2]);
	if (sts != utlNoERROR) {
		return (sts);
	}
	val = (int) utlAtoD(argv[2]);
	if (((val > 0) && (val > UCHAR_MAX)) || ((val < 0) && (val < CHAR_MIN))) {
		return (utlFailERROR);
	}

	/* check if third arg is valid number string */
	sts = cmdValidNum(argv[3]);
	if (sts != utlNoERROR) {
		return (sts);
	}
	size = (unsigned) utlAtoD(argv[3]);

	err = myMemchk(address_p, val, size);

	if (err != MEM_NO_ERROR) {
		MEM_PRINT_ERROR_CODE("memchk", err);
		SVCprintStr("memchk failed\r\n");
		return (utlFailERROR);
	}

	SVCprintStr("memchk successful\r\n");
	return (utlNoERROR);
}

/* File commands */

/* opens a steam. Takes in a file name and permissions. If the file doesn't exist then it
 * will create the file. Returns an integer representation of the stream. Note: the device
 * dependent files (the STDs, LEDs, and pushbuttons), are open on initialization and aren't
 * capable of being closed. The steam IDs for those are: STDIN, STDOUT, STDERR, SW1, SW2,
 * ORANGE, YELLOW, GREEN, and BLUE.
 *
 * param: int argc, char *argv[]
 * return: utlErrno_t
 */
utlErrno_t cmd_fopen(int argc, char *argv[]) {
	char *fileName_p, *mode_p;

	myFILE fi;

	if (argc != 3){
		return (utlArgNumERROR);
	}

	utlErrno = utlNoERROR;

	/* file system will check as well for a valid name placing its own restrictions */
	fileName_p = cmdReplaceName_p(argv[1]);
	if (fileName_p == NULL){
		return (utlValSubERROR);
	}

	mode_p = cmdReplaceName_p(argv[2]);
	if (mode_p == NULL){
		return (utlValSubERROR);
	}

	fi = SVCFOpen(fileName_p, mode_p);

	if (fi == -1){
		utlErrno = utlFailERROR;
	}else{
		char str[shMAX_BUFFERSIZE + 1];
		snprintf(str, shMAX_BUFFERSIZE, "\r\n%d\r\n", fi);
		SVCprintStr(str);
	}

	return (utlErrno);
}

/* close a steam. Takes in an integer representation of the stream. Will not close
 * any of the dedicated steams (i.e. those connected to the device). Returns error status.
 *
 * param: int argc, char *argv[]
 * return: utlErrno_t
 */
utlErrno_t cmd_fclose(int argc, char *argv[]) {

	utlErrno_t sts;
	int fioSts;
	int len;
	char *numString_p;

	if (argc != 2){
		return (utlArgNumERROR);
	}

	utlErrno = utlNoERROR;

	numString_p = cmdReplaceName_p(argv[1]);
	if (numString_p == NULL){
		return (utlValSubERROR);
	}else{
		len = utlStrLen(numString_p);
	}

	/* invalid number string */
	sts = cmdValidNum(numString_p);
	if (sts != utlNoERROR){
		return (sts);
	}

	myFILE fi = (myFILE) utlAtoD(numString_p);

	fioSts = SVCFClose(fi);

	if (fioSts == -1){
		utlErrno = utlFailERROR;
	}

	return (utlErrno);
}

/* create a new file or directory. Takes only two arguments. Returns error status.
 *
 * For a file type /<dir path>/<file name>
 * For a dir type /<dir path>/<dir name>/
 *
 * param: int argc, char *argv[]
 * return: utlErrno_t
 */
utlErrno_t cmd_create(int argc, char *argv[]) {
	char *fileName_p = NULL;
	char *permissions_p = NULL;
	int sts;

	if ((argc < 2) || (argc > 3)){
		return (utlArgNumERROR);
	}

	utlErrno = utlNoERROR;

	/* file system will check as well for a valid name placing its own restrictions */
	fileName_p = cmdReplaceName_p(argv[1]);
	if (fileName_p == NULL){
		return (utlValSubERROR);
	}

	if (argc == 3) {
		permissions_p = cmdReplaceName_p(argv[2]);
	}

	sts = SVCCreate(fileName_p, permissions_p);

	if (sts == -1){
		utlErrno = utlFailERROR;
	}


	return (utlErrno);
}

/* deletes a file/directory. Prints out an error if it's unable to do so.
 *
 * param: int argc, char *argv[]
 * return: utlErrno_t
 */
utlErrno_t cmd_delete(int argc, char *argv[]) {
	char *fileName_p;
	int fioSts;

	if (argc != 2){
		return (utlArgNumERROR);
	}

	utlErrno = utlNoERROR;

	/* file system will check as well for a valid name placing its own restrictions */
	fileName_p = cmdReplaceName_p(argv[1]);
	if (fileName_p == NULL){
		return (utlValSubERROR);
	}

	fioSts = SVCDelete(fileName_p);

	if (fioSts == -1){
		utlErrno = utlFailERROR;
	}

	return (utlErrno);
}

/* resets a file pointer to the beginning of the file. Takes in two
 * arguments where the second is an integer id of an open stream that
 * points to a file. Returns the error status.
 *
 * param: int argc, char *argv[]
 * return: utlErrno_t
 */
utlErrno_t cmd_rewind(int argc, char *argv[]) {
	utlErrno_t sts;
	int fioSts;
	int len;
	char *numString_p;

	if (argc != 2){
		return (utlArgNumERROR);
	}

	utlErrno = utlNoERROR;

	numString_p = cmdReplaceName_p(argv[1]);
	if (numString_p == NULL){
		return (utlValSubERROR);
	}else{
		len = utlStrLen(numString_p);
	}

	myFILE fi = getPresetDev(numString_p);
	if (fi == -1) {
		/* validate number string */
		sts = cmdValidNum(numString_p);
		if (sts != utlNoERROR) {
			return (sts);
		}
		fi = (myFILE) utlAtoD(numString_p);
	}

	fioSts = SVCRewind(fi);

	if (fioSts == -1){
		utlErrno = utlFailERROR;
	}

	return (utlErrno);

}

/* puts a character present at the position indicated by file pointer, moves to
 * the next position, and returns the location of the entered character. Exhibits
 * different behavior if interacting with a device. Takes in three arguments, where
 * the second argument is the character to pass in, and the third is the integer
 * representation of an open stream. If it is one of the dedicated strings for the
 * device, then the following are allowed as steam ids: STDIN, STDOUT, STDERR, SW1,
 * SW2, ORANGE, YELLOW, GREEN, and BLUE. If the steam is for a pushbutton (SW1 and SW2),
 * a message will pop up saying that it isn't possible. If it is a LED (ORANGE,
 * YELLOW, GREEN, and BLUE), then it will turn on if a '1' is passed, off if it's a
 * '0", or do nothing if any other character is passed in. If it is STDIN, then a
 * message will appear sating that the operation isn't possible. If it's STDOUT or
 * STDERR, then the character will be printed to the screen. If it's STDERR, then
 * it will be preceded by "Error:". In the case of the device streams, a 0 will be
 * returned if successful.
 *
 * param: int argc, char *argv[]
 * return: utlErrno_t
 */
utlErrno_t cmd_fputc(int argc, char *argv[]) {
	utlErrno_t sts;

	if (argc != 3){
		return (utlArgNumERROR);
	}

	utlErrno = utlNoERROR;

	/* file system will check as well for a valid name placing its own restrictions */
	char *char_p;
	int c;
	char_p = cmdReplaceName_p(argv[1]);
	if (char_p == NULL){
		return (utlValSubERROR);
	}

	if (utlStrLen(char_p) > 1){
		return (utlValSubERROR);
	}

	c = (int) char_p[0];

	int len;
	char *numString_p;
	numString_p = cmdReplaceName_p(argv[2]);
	if (numString_p == NULL){
		return (utlValSubERROR);
	}else{
		len = utlStrLen(numString_p);
	}

	myFILE   fi = getPresetDev(numString_p);
	if (fi == -1) {
		/* invalid number string */
		sts = cmdValidNum(numString_p);
		if (sts != utlNoERROR) {
			return (sts);
		}
		fi = (myFILE) utlAtoD(numString_p);
	}

	int cRet;
	cRet = SVCFPutc(c, fi);
	if (cRet == -1){
		utlErrno = utlFailERROR;
	}else{
		char str[shMAX_BUFFERSIZE + 1];
		snprintf(str, shMAX_BUFFERSIZE, "\r\n%d\r\n", cRet);
		SVCprintStr(str);
	}
	return (utlErrno);
}

/* returns the character present at position indicated by file pointer. If there is
 * none/EOF an error is returned. Exhibits different behavior if interacting with
 * a device. Takes in two arguments, where the second argument is the integer
 * representation of an open stream. If it is one of the dedicated strings for the
 * device, then the following are allowed as steam ids: STDIN, STDOUT, STDERR, SW1, SW2,
 * ORANGE, YELLOW, GREEN, and BLUE. If the steam is for a pushbutton (SW1 and SW2) or
 * an LED (ORANGE, YELLOW, GREEN, and BLUE), then it will return a 1 if pushed/on or
 * a 0 if it isn't. If it is STDIN, then a prompt will appear requesting a char be
 * entered, which will then be printed out. If it's STDOUT or STDERR, then a message
 * will pop up sating that this operation isn't possible and will return a 0.
 *
 * param: int argc, char *argv[]
 * return: utlErrno_t
 */
utlErrno_t cmd_fgetc(int argc, char *argv[]) {
	utlErrno_t sts;

	if (argc != 2){
		return (utlArgNumERROR);
	}

	utlErrno = utlNoERROR;

	int len;
	char *numString_p;
	numString_p = cmdReplaceName_p(argv[1]);
	if (numString_p == NULL){
		return (utlValSubERROR);
	}else{
		len = utlStrLen(numString_p);
	}

	myFILE fi = getPresetDev(numString_p);
	if (fi == -1) {
		/* validate number string */
		sts = cmdValidNum(numString_p);
		if (sts != utlNoERROR) {
			return (sts);
		}
		fi = (myFILE) utlAtoD(numString_p);
	}

	int cRet;
	cRet = SVCFGetc(fi);
	if (cRet == -1) {
		utlErrno = utlFailERROR;
	}else{
		char str[shMAX_BUFFERSIZE + 1];
		if (isAnlg(fi)) {
			snprintf(str, shMAX_BUFFERSIZE, "\r\n%4u\r\n", (unsigned int) cRet);
		} else if ((cRet < 32) || (cRet > 126)){
			snprintf(str, shMAX_BUFFERSIZE, "\r\n%x\r\n", cRet);
		}else{
			snprintf(str, shMAX_BUFFERSIZE, "\r\n%c\r\n", (char) cRet);
		}
		SVCprintStr(str);
	}

	return (utlErrno);
}

/* prints out the files and sub-directories in a given directory. Takes in two
 * arguments, where the second argument is the directory path. If it's root, then
 * the path should just be '/'. Will print out the files/directories there with
 * 4 per line. Returns the error status.
 *
 * param: int argc, char *argv[]
 * return: utlErrno_t
 */
utlErrno_t cmd_ls(int argc, char *argv[]) {
	char *fileName_p;

	myFILE fi;

	if (argc > 2){
		return (utlArgNumERROR);
	}

	utlErrno = utlNoERROR;

	if (argc == 2) {
		/* file system will check as well for a valid name placing its own restrictions */
		fileName_p = cmdReplaceName_p(argv[1]);
		if (fileName_p == NULL) {
			return (utlValSubERROR);
		}
	} else {
		fileName_p = NULL;
	}

	fi = myopendir(fileName_p);

	if (fi == -1) {
		utlErrno = utlFailERROR;
	} else {
		fileName_p = getDirRecord(fi, NULL);

		int i = 0;
		while ((fileName_p != NULL)) {
			char str[shMAX_BUFFERSIZE + 1];
			if (i % 4 == 0){
				/* NUM_FN_ON_LINE - how many file names to be displayed on one line */
				SVCprintStr("\r\n");
			}
			i++;
			snprintf(str, shMAX_BUFFERSIZE, "%-16s  ", fileName_p);
			SVCprintStr(str);
			fileName_p = getDirRecord(fi, fileName_p);
		}
		SVCprintStr("\r\n");
	}

	/* getDriRecord allocates space at the callers request (i.e. second arg to getDirRecord is NULL)*/
	/* therefore, the caller must free the memory allocated on its behalf */
	myFree(fileName_p);
	myfclose(fi);

	return (utlErrno);
}

/* Continuously copy characters from serial input to LCD.  End on
 * a ^D (control-D) input character.  The behavior of the LCD device
 * should mirror the behavior of our terminal emulator.  That is,
 * it should: (1) return to the beginning of the current line when
 * it is sent a carriage-return (\r, ^M, or control-M) and (2) go
 * to the next line in the same column when it is sent a line-feed
 * (\n, ^J, or control-J). Returns the error status.
 *
 * IMPORTANT: due to current set up, what is typed into the lcd will
 * also be echoed back on to the UART
 *
 * param: int argc, char *argv[]
 * return: utlErrno_t
 */
utlErrno_t cmd_ser2lcd(int argc, char *argv[]){
	if (argc > 1) {
		return utlArgNumERROR;
	}

	myFILE fi = myfopen("LCD", "a");
	if (fi != LCD) {
		return (FIO_INIT_FAILED);
	}

	while (1) {
		char ch = SVCgetCharNoEch();

		// Output the character on the TWR_LCD_RGB
		SVCFPutc(ch, getPresetDev("LCD"));

		// Exit if character typed was a Control-D (EOF)
		if (ch == CHAR_EOF) {
			SVCFClose(fi);
			return (utlNoERROR);
		}
	}
}

/* Continuously copy from each touch sensor to the corresponding LED.
 * End when all four touch sensors are "depressed" (i.e. all lights
 * are on). Returns the error status.
 *
 * param: int argc, char *argv[]
 * return: utlErrno_t
 */
utlErrno_t cmd_touch2led(int argc, char *argv[]){
	if (argc > 1) {
		return utlArgNumERROR;
	}

	myFILE fi1 = myfopen("TS1", "a");
	if (fi1 != TS1) {
		return (utlFailERROR);
	}
	myFILE fi2 = myfopen("TS2", "a");
	if (fi2 != TS2) {
		SVCFClose(fi1);
		return (utlFailERROR);
	}
	myFILE fi3 = myfopen("TS3", "a");
	if (fi3 != TS3) {
		SVCFClose(fi1);
		SVCFClose(fi2);
		return (utlFailERROR);
	}
	myFILE fi4 = myfopen("TS4", "a");
	if (fi4 != TS4) {
		SVCFClose(fi1);
		SVCFClose(fi2);
		SVCFClose(fi3);
		return (utlFailERROR);
	}
	myFILE fi5 = myfopen("ORANGE", "a");
	if (fi5 != ORANGE) {
		SVCFClose(fi1);
		SVCFClose(fi2);
		SVCFClose(fi3);
		SVCFClose(fi4);
		return (utlFailERROR);
	}
	myFILE fi6 = myfopen("YELLOW", "a");
	if (fi6 != YELLOW) {
		SVCFClose(fi1);
		SVCFClose(fi2);
		SVCFClose(fi3);
		SVCFClose(fi4);
		SVCFClose(fi5);
		return (utlFailERROR);
	}
	myFILE fi7 = myfopen("GREEN", "a");
	if (fi7 != GREEN) {
		SVCFClose(fi1);
		SVCFClose(fi2);
		SVCFClose(fi3);
		SVCFClose(fi4);
		SVCFClose(fi5);
		SVCFClose(fi6);
		return (utlFailERROR);
	}
	myFILE fi8 = myfopen("BLUE", "a");
	if (fi8 != BLUE) {
		SVCFClose(fi1);
		SVCFClose(fi2);
		SVCFClose(fi3);
		SVCFClose(fi4);
		SVCFClose(fi5);
		SVCFClose(fi6);
		SVCFClose(fi7);
		return (utlFailERROR);
	}

	while (!(toBool(SVCFGetc(getPresetDev("ORANGE")))
			&& toBool(SVCFGetc(getPresetDev("YELLOW")))
			&& toBool(SVCFGetc(getPresetDev("GREEN"))) && toBool(SVCFGetc(getPresetDev("BLUE"))))) {
		if (toBool(SVCFGetc(getPresetDev("TS1")))) {
			if (!(toBool(SVCFGetc(getPresetDev("ORANGE"))))) {
				SVCFPutc('1', getPresetDev("ORANGE"));
			} else {
				SVCFPutc('0', getPresetDev("ORANGE"));
			}
		}
		if (toBool(SVCFGetc(getPresetDev("TS2")))) {
			if (!(toBool(SVCFGetc(getPresetDev("YELLOW"))))) {
				SVCFPutc('1', getPresetDev("YELLOW"));
			} else {
				SVCFPutc('0', getPresetDev("YELLOW"));
			}
		}
		if (toBool(SVCFGetc(getPresetDev("TS3")))) {
			if (!(toBool(SVCFGetc(getPresetDev("GREEN"))))) {
				SVCFPutc('1', getPresetDev("GREEN"));
			} else {
				SVCFPutc('0', getPresetDev("GREEN"));
			}
		}
		if (toBool(SVCFGetc(getPresetDev("TS4")))) {
			if (!(toBool(SVCFGetc(getPresetDev("BLUE"))))) {
				SVCFPutc('1', getPresetDev("BLUE"));
			} else {
				SVCFPutc('0', getPresetDev("BLUE"));
			}
		}
		delay(delayCount);
	}

	delay(delayCount);
	SVCFPutc('0', getPresetDev("ORANGE"));
	SVCFPutc('0', getPresetDev("YELLOW"));
	SVCFPutc('0', getPresetDev("GREEN"));
	SVCFPutc('0', getPresetDev("BLUE"));

	SVCFClose(fi1);
	SVCFClose(fi2);
	SVCFClose(fi3);
	SVCFClose(fi4);
	SVCFClose(fi5);
	SVCFClose(fi6);
	SVCFClose(fi7);
	SVCFClose(fi8);

	return (utlNoERROR);
}

/* Continuously output the value of the analog potentiomemter
 * to the serial device as a decimal number followed by a newline.
 * End when SW1 is depressed. Returns the error status.
 *
 * param: int argc, char *argv[]
 * return: utlErrno_t
 */
utlErrno_t cmd_pot2ser(int argc, char *argv[]){
	if (argc > 1) {
		return utlArgNumERROR;
	}

	myFILE fi1 = SVCFOpen("POTENTIOMETER", "a");
	if (fi1 != POTENTIOMETER) {
		return (utlFailERROR);
	}

	myFILE fi2 = myfopen("SW1", "a");
	if (fi2 != SW1) {
		SVCFClose(fi1);
		return (utlFailERROR);
	}

	char str[shMAX_BUFFERSIZE + 1];
	while (!(toBool(SVCFGetc(getPresetDev("SW1"))))) {
		snprintf(str, shMAX_BUFFERSIZE, "pot: %4u\r\n",
				(unsigned int) SVCFGetc(getPresetDev("POTENTIOMETER")));
		SVCprintStr(str);
		delay(delayCount);
	}

	SVCFClose(fi1);
	SVCFClose(fi2);

	return (utlNoERROR);
}

/* Continuously output the value of the thermistor to the serial device
 * as a decimal number followed by a newline. End when SW1 is depressed.
 * Returns the error status.
 *
 * param: int argc, char *argv[]
 * return: utlErrno_t
 */
utlErrno_t cmd_therm2ser(int argc, char *argv[]){
	if (argc > 1) {
		return utlArgNumERROR;
	}

	myFILE fi1 = myfopen("THERMISTOR", "a");
	if (fi1 != THERMISTOR) {
		return (utlFailERROR);
	}

	myFILE fi2 = myfopen("SW1", "a");
	if (fi2 != SW1) {
		SVCFClose(fi1);
		return (utlFailERROR);
	}

	char str[shMAX_BUFFERSIZE + 1];
	while (!(toBool(SVCFGetc(getPresetDev("SW1"))))) {
		snprintf(str, shMAX_BUFFERSIZE, "temp: %4u\r\n",
				(unsigned int) SVCFGetc(getPresetDev("THERMISTOR")));
		SVCprintStr(str);
		delay(delayCount);
	}

	SVCFClose(fi1);
	SVCFClose(fi2);

	return (utlNoERROR);
}

/* Continuously copy from SW1 to orange LED and SW2 to yellow LED.
 * End when both SW1 and SW2 are depressed (i.e. both lights are on).
 * Returns the error status.
 *
 * param: int argc, char *argv[]
 * return: utlErrno_t
 */
utlErrno_t cmd_pb2led(int argc, char *argv[]){
	if (argc > 1) {
		return utlArgNumERROR;
	}

	myFILE fi1 = myfopen("SW1", "a");
	if (fi1 != SW1) {
		return (utlFailERROR);
	}
	myFILE fi2 = myfopen("SW2", "a");
	if (fi2 != SW2) {
		SVCFClose(fi1);
		return (utlFailERROR);
	}
	myFILE fi3 = myfopen("ORANGE", "a");
	if (fi3 != ORANGE) {
		SVCFClose(fi1);
		SVCFClose(fi2);
		return (utlFailERROR);
	}
	myFILE fi4 = myfopen("YELLOW", "a");
	if (fi4 != YELLOW) {
		SVCFClose(fi1);
		SVCFClose(fi2);
		SVCFClose(fi3);
		return (utlFailERROR);
	}

	/* Correction from PS 4: changed below so that the LEDs are only one if you hold down the buttons */
	while(!(toBool(SVCFGetc(getPresetDev("ORANGE"))) && toBool(SVCFGetc(getPresetDev("YELLOW"))))){
		if (toBool(SVCFGetc(getPresetDev("SW1")))) {
			SVCFPutc('1', getPresetDev("ORANGE"));
		} else {
			SVCFPutc('0', getPresetDev("ORANGE"));
		}
		if (toBool(SVCFGetc(getPresetDev("SW2")))) {
			SVCFPutc('1', getPresetDev("YELLOW"));
		} else {
			SVCFPutc('0', getPresetDev("YELLOW"));
		}
	}
	delay(delayCount);
	SVCFPutc('0', getPresetDev("ORANGE"));
	SVCFPutc('0', getPresetDev("YELLOW"));

	SVCFClose(fi1);
	SVCFClose(fi2);
	SVCFClose(fi3);
	SVCFClose(fi4);

	return (utlNoERROR);
}

/* flash the orange LED on and off approximately every half a second
 * (the LED will light approximately once a second).  End when SW1 is
 * depressed. Takes in no arguments.
 */
utlErrno_t cmd_flashled(int argc, char *argv[]){
	if (argc > 1) {
		return utlArgNumERROR;
	}

	myFILE fi1 = myfopen("SW1", "a");
	if (fi1 != SW1) {
		return (utlFailERROR);
	}

	myFILE fi2 = myfopen("ORANGE", "a");
	if (fi2 != ORANGE) {
		SVCFClose(fi1);
		return (utlFailERROR);
	}

	SVCpdb(500, flash);

	while(!toBool(SVCFGetc(getPresetDev("SW1")))){
		if (act == 1) {
			SVCpdbStart();
		}
	}

	SVCFClose(fi1);
	SVCFClose(fi2);
	return (utlNoERROR);
}

/* sets a memory location with specified values;
 * takes in mem address (decimal, hex or octal) and one or more byte values
 * (decimal, hex or octal); if either specified location is not within an
 * allocated mem segment for the calling process or values are invalid or
 * too many, the error is returned, else utlNoERROR is returned.
 */
utlErrno_t cmd_deposit(int argc, char *argv[]) {
	if (argc < 3){
		return (utlArgNumERROR);
	}
	utlErrno = utlNoERROR;

	utlErrno_t sts;

	char *numString_p;
	int len = 0;

	/* check/set the first argument for memset: mem address */

	numString_p = cmdReplaceName_p(argv[1]);
	if (numString_p == NULL) {
		return (utlFailERROR);
	} else {
		len = utlStrLen(numString_p);
	}

	/* invalid number string */
	sts = cmdValidNum(numString_p);
	if (sts != utlNoERROR){
		return (sts);
	}

	void *address_p = (void *) utlAtoD(numString_p);
	if (address_p == NULL) {
		return (utlFailERROR);
	}

	/* check the rest of arguments -  the value bytes */
	unsigned int numVals = argc - 2;
	char *val_p = myMalloc(numVals);

	if (val_p == NULL) {
		return (utlFailERROR);
	}

	int i = 2;
	int j;
	for (j = 0; (i < argc); i++, j++) {
		numString_p = cmdReplaceName_p(argv[i]);
		if (numString_p == NULL) {
			utlErrno = utlFailERROR;
			break;
		} else
			len = utlStrLen(numString_p);

		sts = cmdValidNum(numString_p);
		if (sts != utlNoERROR) {
			utlErrno = sts;
			break;
		}

		int val = (int) utlAtoD(numString_p);
		if (((val > 0) && (val > UCHAR_MAX))
				|| ((val < 0) && (val < CHAR_MIN))) {
			utlErrno = utlFailERROR;
			break;
		}

		val_p[j] = val;
	}

	if (utlErrno == utlNoERROR) {
		if (myMemcpy(address_p, val_p, numVals) != MEM_NO_ERROR) {
			utlErrno = utlFailERROR;
		}
	}

	SVCFree(val_p);

	return (utlErrno);
}

/* displays values of 16 bytes (in hex and ascii) following the specified mem address;
 * takes in a mem address; if address is invalid, returns error, otherwise utlNoERROR is returned.
 * NOTE: does note check if the address has been previously allocated by mymalloc
 */
utlErrno_t cmd_examine(int argc, char *argv[]) {
	const int NumBytes = 16;
	utlErrno_t sts;

	if (argc != 2){
		return (utlArgNumERROR);
	}
	utlErrno = utlNoERROR;

	char *numString_p;
	int len = 0;

	/* check/set the first argument for memset: mem address */

	numString_p = cmdReplaceName_p(argv[1]);
	if (numString_p == NULL) {
		utlErrno = utlFailERROR;
		return (utlErrno);
	} else {
		len = utlStrLen(numString_p);
	}

	/* invalid number string */
	sts = cmdValidNum(numString_p);
	if (sts != utlNoERROR){
		return (sts);
	}

	void *address_p = (void *) utlAtoD(numString_p);
	if (address_p == NULL) {
		return (utlFailERROR);
	}

	char str[shMAX_BUFFERSIZE + 1];
	unsigned int i;
	snprintf(str, shMAX_BUFFERSIZE, "\r\nAddr          ");
	SVCprintStr(str);

	for (i = 0; i < NumBytes; i += 2) {
		snprintf(str, shMAX_BUFFERSIZE, " +%-4x", i);
		SVCprintStr(str);
	}

	snprintf(str, shMAX_BUFFERSIZE, "  ");
	for (i = 0; i < NumBytes; i += 2) {
		snprintf(str, shMAX_BUFFERSIZE, " %x", i);
		SVCprintStr(str);
	}

	snprintf(str, shMAX_BUFFERSIZE, "\r\n%p  ", address_p);
	SVCprintStr(str);

	for (i = 0; i < NumBytes; i += 2) {
		unsigned short xnum = *((unsigned short *) (address_p + i));
		snprintf(str, shMAX_BUFFERSIZE, " %-5x", xnum);
		SVCprintStr(str);
	}
	snprintf(str, shMAX_BUFFERSIZE, "   ");
	for (i = 0; i < NumBytes; i++) {
		char val = *(((char *) address_p) + i);
		if ((val < 32) || (val > 126)){
			val = '.';
		}

		snprintf(str, shMAX_BUFFERSIZE, "%c", val);
		SVCprintStr(str);
	}
	SVCprintStr("\r\n");
	return (utlErrno);
}

/* creates three processes to accomplish the following tasks, one per process:
 *
 * (First process) copying from UART2 input to the LCD display
 * (Second process) sending a message over UART2 output whenever
 * pushbutton S2 is depressed
 * Third process) using the supervisor call for user timer events,
 * flash the orange LED on and off every half a second (the LED will
 * light once a second
 *
 * The first process (the one that performs input from UART2), will
 * terminate when control-D is entered.  When the shell's multitask
 * command determines that that process has terminated, it will kill
 * the other two processes.
 */
utlErrno_t cmd_multitask(int argc, char *argv[]){
	utlErrno_t sts;

	if (argc != 1) {
		return (utlArgNumERROR);
	}
	utlErrno = utlNoERROR;

	pid_t first = getNextPID();
	pid_t second = getNextPID();
	pid_t third = getNextPID();
	shArg_t arg1;
	shArg_t arg2;
	shArg_t arg3;
	arg1.argc = 1;
	arg1.argv = NULL;
	arg2.argc = 1;
	arg2.argv = NULL;
	arg3.argc = 1;
	arg3.argv = NULL;

	int i = SVCspawn(cmd_ser2lcd, &arg1, STACK_SIZE, &first);
	if(i == -1){
		utlErrno = utlFailERROR;
		return (utlErrno);
	}

	i = SVCspawn(cmd_flashled, &arg2, STACK_SIZE, &second);
	if(i == -1){
		utlErrno = utlFailERROR;
		return (utlErrno);
	}

	i = SVCspawn(pb2uart, &arg3, STACK_SIZE, &third);
	if(i == -1){
		utlErrno = utlFailERROR;
		return (utlErrno);
	}

	SVCwait(first);
	SVCmyKill(second);
	SVCmyKill(third);

	return (utlErrno);
}

utlErrno_t cmd_fputs(int argc, char *argv[]) {
	utlErrno_t sts;

	if (argc != 3) {
		return (utlArgNumERROR);
	}

	utlErrno = utlNoERROR;

	/* file system will check as well for a valid name placing its own restrictions */
	char *str_p;

	str_p = cmdReplaceName_p(argv[1]);
	if (str_p == NULL) {
		return (utlValSubERROR);
	}

	int len;
	char *numString_p;

	numString_p = cmdReplaceName_p(argv[2]);
	if (numString_p == NULL) {
		return (utlValSubERROR);
	} else {
		len = utlStrLen(numString_p);
	}

	/* check if arg is std or hardware device first */
	myFILE fi = getPresetDev(numString_p);
	if (fi == -1) {
		/* validate number string */
		sts = cmdValidNum(numString_p);
		if (sts != utlNoERROR)
			return (sts);
		fi = (myFILE) utlAtoD(numString_p);
	}

	int rets;
	rets = SVCFPuts(str_p, fi);

	if (rets == -1) {
		utlErrno = utlFailERROR;
	}
	return (utlErrno);
}

utlErrno_t cmd_fgets(int argc, char *argv[]) {
	utlErrno_t sts;

	if (argc != 2)
		return (utlArgNumERROR);

	utlErrno = utlNoERROR;

	int len;
	char *numString_p;

	/* get file descriptor */

	numString_p = cmdReplaceName_p(argv[1]);
	if (numString_p == NULL) {
		return (utlValSubERROR);
	} else {
		len = utlStrLen(numString_p);
	}

	/* check if arg is std or hardware device first */
	myFILE fi = getPresetDev(numString_p);
	if (fi == -1) {
		/* validate number string */
		sts = cmdValidNum(numString_p);
		if (sts != utlNoERROR) {
			return (sts);
		}
		fi = (myFILE) utlAtoD(numString_p);
	}

	char str[shMAX_BUFFERSIZE + 1];
	int rets;
	rets = SVCFGets(str, shMAX_BUFFERSIZE, fi);

	if (rets == -1) {
		utlErrno = utlFailERROR;
	} else {
		char str_a[shMAX_BUFFERSIZE + 1];
		snprintf(str_a, shMAX_BUFFERSIZE, "%s\r\n", str);
		SVCprintStr(str_a);
	}

	return (utlErrno);
}

/* adds user.
 * takes in: user name, group name, and password;
 * returns fail status if:
 *        current user is not admin, and therefore is not privileged to add a new user
 *        invalid name, group or  password - must not have '$' as the first character or exceed max length
 *        matches an existing user,
 *        no more free slots in the user table;
 */
utlErrno_t cmd_adduser(int argc, char *argv[]) {
	utlErrno_t sts;

	if (argc == 1) {
		/* list registered users */
		usrList();
		return (utlNoERROR);
	}

	/* check number of arguments */
	if (argc != 4) {
		return (utlArgNumERROR);
	}

	/* process arguments */
	/* argv: name, group, password */
	char *name_p = argv[1];
	char *group_p = argv[2];
	char *password_p = argv[3];

	sts = usrAdd(name_p, group_p, password_p);

	return (sts);
}

/* removed user
 * input user name
 * output: fail if not requested by privileged user (admin), or user not recognized
 */
utlErrno_t cmd_remuser(int argc, char *argv[]) {
	utlErrno_t sts;

	/* check num of arguments */
	if (argc != 2) {
		return (utlArgNumERROR);
	}

	char *name_p = argv[1];
	sts = usrRemove(name_p);

	return (sts);
}


/* logout
 * no inputs or outputs;
 * the current user data (managed by usr module) is reset
 */
utlErrno_t cmd_logout(int argc, char *argv[]) {
	/* check num of arguments */
	if (argc != 1) {
		return (utlArgNumERROR);
	}

	/* log out */
	usrLogout();

	return (utlNoERROR);
}

utlErrno_t cmd_purge(int argc, char *argv[]) {
	char *fileName_p;
	int fioSts;

	if (argc != 2) {
		return (utlArgNumERROR);
	}

	utlErrno = utlNoERROR;

	/* file system will check as well for a valid name placing its own restrictions */
	fileName_p = cmdReplaceName_p(argv[1]);
	if (fileName_p == NULL) {
		return (utlValSubERROR);
	}

	fioSts = mypurge(fileName_p);

	if (fioSts == -1) {
		utlErrno = utlFailERROR;
	}

	return (utlErrno);
}

utlErrno_t cmd_logpurge(int argc, char *argv[]) {
	int fioSts;
	if (argc != 1) {
		return (utlArgNumERROR);
	}

	utlErrno = utlNoERROR;

	fioSts = logPurge();

	if (fioSts == -1) {
		utlErrno = utlFailERROR;
	}

	return (utlErrno);
}

utlErrno_t cmd_logrewind(int argc, char *argv[]) {
	int fioSts;
	if (argc != 1) {
		return (utlArgNumERROR);
	}

	utlErrno = utlNoERROR;

	fioSts = logRewind();

	if (fioSts == -1) {
		utlErrno = utlFailERROR;
	}

	return (utlErrno);
}

utlErrno_t cmd_logread(int argc, char *argv[]) {
	if ((argc < 1) || (argc > 2)){
		return (utlArgNumERROR);
	}

	utlErrno = utlNoERROR;

	char str[shMAX_BUFFERSIZE + 1];
	int rets;
	bool readAll = false;

	if (argc == 2) {
		char *str_p;

		str_p = cmdReplaceName_p(argv[1]);
		if ((str_p == NULL) || !utlStrCmp(str_p, "all")) {
			return (utlValSubERROR);
		}

		int fioSts;

		fioSts = logRewind();
		if (fioSts == -1) {
			return utlFailERROR;
		}
		readAll = true;
	}

	do {
		rets = logRead(str, shMAX_BUFFERSIZE);
		if (rets == -1) {
			utlErrno = utlFailERROR;
		} else {
			SVCprintStr(str);
		}
	} while ((*str != '\0') && readAll && (rets != -1));

	return (utlErrno);
}

utlErrno_t cmd_spawnFlashGB(int argc, char *argv[]){
	if (!usrAdmin()) {
		logWrite(FIO_INVREQ_READ, "unprivileged user",
				"attempted to execute 'spawnFlashGB'");
		return (utlPrivERROR);
	}

	flashGBpid = getNextPID();
	shArg_t arg;
	arg.argc = 1;
	arg.argv = NULL;

	int i = SVCspawn(flashGB, &arg, STACK_SIZE, &flashGBpid);
	if (i == -1) {
		utlErrno = utlFailERROR;
		return (utlErrno);
	}
	return (utlNoERROR);
}

utlErrno_t cmd_killFlashGB(int argc, char *argv[]){
	if (!usrAdmin()) {
		logWrite(FIO_INVREQ_READ, "unprivileged user",
				"attempted to execute 'killFlashGB'");
		return (utlPrivERROR);
	}
	if(flashGBpid == -1){
		return utlFailERROR;
	}

	SVCmyKill(flashGBpid);
	flashGBpid = -1;
	return (utlNoERROR);
}

utlErrno_t cmd_spawn(int argc, char *argv[]){
	if (!usrAdmin()) {
		logWrite(FIO_INVREQ_READ, "unprivileged user",
				"attempted to execute 'spawn'");
		return (utlPrivERROR);
	}

	if (argc != 2) {
		return utlArgNumERROR;
	}

	shArg_t arg;
	arg.argc = 1;
	arg.argv = NULL;

	int j;
	if(utlStrCmp(argv[1], "flashGB")){
		flashGBpid = getNextPID();
		j = SVCspawn(flashGB, &arg, STACK_SIZE, &flashGBpid);
	} else if ((utlStrCmp(argv[1], "touch2led"))
			|| (utlStrCmp(argv[1], "flashled"))
			|| (utlStrCmp(argv[1], "pb2led"))) {
		int i = cmdMatch(argv[1]);
		int k = getNextPID();
		j = SVCspawn(commands[i].functionp, &arg, STACK_SIZE, &k);
	}else{
		return (utlFailERROR);
	}

	if (j == -1) {
		return (utlFailERROR);
	}

	return (utlNoERROR);
}
/*helpers*/
int toBool(int i){
	if (i == 49){
		return 1;
	}else{
		return 0;
	}
}

void flash(){
	if (toBool(myfgetc(getPresetDev("ORANGE")))) {
		myfputc('0', getPresetDev("ORANGE"));
	} else {
		myfputc('1', getPresetDev("ORANGE"));
	}
}

utlErrno_t pb2uart(int argc, char *argv[]){
	myFILE fi = myfopen("SW2", "a");
	if (fi != SW2) {
		return (utlFailERROR);
	}
	while(1){
		if(toBool(SVCFGetc(getPresetDev("SW2")))){
			SVCprintStr("Hello user!\r\n");
			delay(0x259BF); //to slow the output down
		}
	}
}

utlErrno_t flashGB(int argc, char *argv[]){
	myFILE fi1 = myfopen("GREEN", "a");
	if (fi1 != GREEN) {
		return (utlFailERROR);
	}
	myFILE fi2 = myfopen("BLUE", "a");
	if (fi2 != BLUE) {
		SVCFClose(fi1);
		return (utlFailERROR);
	}

	while(1){
		if (toBool(myfgetc(getPresetDev("BLUE")))) {
			myfputc('0', getPresetDev("BLUE"));
			myfputc('1', getPresetDev("GREEN"));
		} else {
			myfputc('1', getPresetDev("BLUE"));
			myfputc('0', getPresetDev("GREEN"));
		}
		delay(delayCount);
	}
}
