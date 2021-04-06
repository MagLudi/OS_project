#ifndef CMD_
#define CMD_


#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <float.h>
#include <string.h> 
#include <stdbool.h>
#include "utl.h"
#include "shell.h"

/* data structure for the commands */
typedef struct commandEntry {
	char *name;
	utlErrno_t (*functionp)(int argc, char *argv[]);
} cmdCommandEntry_t;

#define CHAR_EOF 4

#ifndef ALLOCATE_
 #define EXTERN_ extern
#else
 #define EXTERN_ 
#endif  /* ALLOCATE_ */

/* command functions. Modified for error handling */
utlErrno_t cmd_date(int argc, char *argv[]);
utlErrno_t cmd_echo(int argc, char *argv[]);
utlErrno_t cmd_exit(int argc, char *argv[]);
utlErrno_t cmd_help(int argc, char *argv[]);
utlErrno_t cmd_set(int argc, char *argv[]);
utlErrno_t cmd_unset(int argc, char *argv[]);
utlErrno_t cmd_malloc(int argc, char *argv[]);
utlErrno_t cmd_free(int argc, char *argv[]);
utlErrno_t cmd_memorymap(int argc, char *argv[]);
utlErrno_t cmd_memset(int argc, char *argv[]);
utlErrno_t cmd_memchk(int argc, char *argv[]);
utlErrno_t cmd_fopen(int argc, char *argv[]);
utlErrno_t cmd_fclose(int argc, char *argv[]);
utlErrno_t cmd_create(int argc, char *argv[]);
utlErrno_t cmd_delete(int argc, char *argv[]);
utlErrno_t cmd_rewind(int argc, char *argv[]);
utlErrno_t cmd_fputc(int argc, char *argv[]);
utlErrno_t cmd_fgetc(int argc, char *argv[]);
utlErrno_t cmd_ls(int argc, char *argv[]);
utlErrno_t cmd_ser2lcd(int argc, char *argv[]);
utlErrno_t cmd_touch2led(int argc, char *argv[]);
utlErrno_t cmd_pot2ser(int argc, char *argv[]);
utlErrno_t cmd_therm2ser(int argc, char *argv[]);
utlErrno_t cmd_pb2led(int argc, char *argv[]);
utlErrno_t cmd_flashled(int argc, char *argv[]);
utlErrno_t cmd_examine(int argc, char *argv[]);
utlErrno_t cmd_deposit(int argc, char *argv[]);
utlErrno_t cmd_multitask(int argc, char *argv[]);
utlErrno_t cmd_fputs(int argc, char *argv[]);
utlErrno_t cmd_fgets(int argc, char *argv[]);
utlErrno_t cmd_adduser(int argc, char *argv[]);
utlErrno_t cmd_remuser(int argc, char *argv[]);
utlErrno_t cmd_logout(int argc, char *argv[]);
utlErrno_t cmd_purge(int argc, char *argv[]);
utlErrno_t cmd_logread(int argc, char *argv[]);
utlErrno_t cmd_logrewind(int argc, char *argv[]);
utlErrno_t cmd_logpurge(int argc, char *argv[]);
utlErrno_t cmd_spawnFlashGB(int argc, char *argv[]);
utlErrno_t cmd_killFlashGB(int argc, char *argv[]);
utlErrno_t cmd_spawn(int argc, char *argv[]);

/* helper functions */
utlErrno_t cmdExecute(int i, int argc, char *argv_p[]);
utlErrno_t cmdAddVar(shVar_t  *var_p);
utlErrno_t cmdRemoveVar(char *varName_p);
utlErrno_t cmdValidateVar(char *varName_p, int len);
int cmdMatch(char *cmd_p);
void cmdPrintVars(void);
char *cmdReplaceName_p(char *name_p);
char *cmdValidateField_p(char *name_p);
void cmdMoveFieldLeft(char *field_p, int len);
utlErrno_t cmdValidNum(char *string_p);
utlErrno_t setDate(int argc, char *argv[]);

utlErrno_t pb2uart(int argc, char *argv[]);
utlErrno_t flashGB(int argc, char *argv[]);

#endif /* CMD_ */
