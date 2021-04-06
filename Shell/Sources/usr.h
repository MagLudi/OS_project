#ifndef USR_
#define USR_

/* systtem headers */
#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>

/* local headers */
#include "utl.h"

#define USR_MAX_NAME_LEN 8
#define USR_MIN_NAME_LEN 2
#define USR_MAX_PASSWORD_LEN 8
#define USR_MIN_PASSWORD_LEN 4

#define USR_MAX 20
#define USR_NO_ACTIVE -1

#define USR_0_NAME  "USR0"
#define USR_0_GROUP "GRP0"
#define USR_ADMIN   "admin"

typedef struct {
	char name[USR_MAX_NAME_LEN];
	char group[USR_MAX_NAME_LEN];
	bool set;
	int fiLog;
} usr_t;

#ifndef ALLOCATE_
#define EXTERN_ extern
#else
#define EXTERN_
#endif  /* ALLOCATE_ */

void usrInit(void);
bool usrLogin(void);
void usrLogout(void);
void usrGetCurrent(usr_t *user_p);

void usrList(void);
bool usrAdmin(void);
bool usrShell(void);
utlErrno_t usrRemove(char *name_p);
int usrGetFreeIdx(void);
int usrMatch(char *name_p);
bool usrReservedName(char *name_p);
bool usrReservedGroup(char *group_p);
bool usrMatchPassword(char *password_p, int idx);
utlErrno_t usrSetPassword(char *password_p, int idx);
utlErrno_t usrAdd(char *name_p, char *group_p, char *password_p);
uint16_t usrValidName(char *name_p);
uint16_t usrValidGroup(char *group_p);
uint16_t usrValidPassword(char *password_p);
uint16_t usrEncript(uint8_t num);
bool usrGetWord(char *word_p, unsigned len);

#endif /* USR_ */
