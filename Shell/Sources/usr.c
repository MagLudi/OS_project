#include <errno.h>
#include <limits.h>

#include "utl.h"
#include "fioutl.h"
#include "mem.h"
#include "pcb.h"
#include "usr.h"
#include "svc.h"

/*static*/usr_t user[USR_MAX];
/*static*/uint16_t password[USR_MAX][USR_MAX_PASSWORD_LEN];
/*static*/unsigned int userNum;
/*static*/int currentUserIdx;

void usrInit(void) {
	/* to be described in readme: password is stored as an exponent of each byte */

	/* init user data */
	int i;
	for (i = 0; i < USR_MAX; i++) {
		user[0].set = false;

		int j;
		for (j = 0; j < USR_MAX_PASSWORD_LEN; j++) {
			password[i][j] = 0;
		}
	}

	/* set admin user data; admin is the first entry int the user table */
	int nl, gl;
	nl = utlStrLen(USR_ADMIN);
	gl = utlStrLen(USR_0_GROUP);
	utlStrMCpy(user[0].name, USR_ADMIN, nl, USR_MAX_NAME_LEN);
	utlStrMCpy(user[0].group, USR_0_GROUP, gl, USR_MAX_NAME_LEN);

	/* to be described in readme: password = "anna" = 94, 110, 110, 94 */
	/* to be stored as = 94**2, 110**2, 110**2, 94**2 */
	password[0][0] = 9409;
	password[0][1] = 12100;
	password[0][2] = 12100;
	password[0][3] = 9409;

	user[0].fiLog = -1;

	user[0].set = true;
	userNum = 1;

	/* no one is logged in yet*/
	currentUserIdx = USR_NO_ACTIVE;
}

void usrList(void) {
	/* the current user is not admin */
	if (!usrAdmin()){
		logWrite(FIO_LAST_ERROR, user[currentUserIdx].name,
				"attempted to execute 'adduser'");
		return;
	}

	int i, j;
	SVCprintStr("USERS\r\n");
	char str[shMAX_BUFFERSIZE + 1];
	for (i = 0, j = 0; (i < USR_MAX) && (j < userNum); i++) {
		if (user[i].set) {
			j++;
			snprintf(str, shMAX_BUFFERSIZE, "name %-20s   group %s\r\n",
					user[i].name, user[i].group);
			SVCprintStr(str);
		}
	}
}

void usrGetCurrent(usr_t *user_p) {
	if (!user_p) {
		return;
	}
	if (currentUserIdx == USR_NO_ACTIVE) {
		/*shell in charge*/
		utlStrMCpy(user_p->name, USR_0_NAME, USR_MAX_NAME_LEN,
				USR_MAX_NAME_LEN);
		utlStrMCpy(user_p->group, USR_0_GROUP, USR_MAX_NAME_LEN,
				USR_MAX_NAME_LEN);
		user_p->fiLog = -1;
	} else {
		utlStrMCpy(user_p->name, user[currentUserIdx].name, USR_MAX_NAME_LEN,
				USR_MAX_NAME_LEN);
		utlStrMCpy(user_p->group, user[currentUserIdx].group, USR_MAX_NAME_LEN,
				USR_MAX_NAME_LEN);
		user_p->fiLog = user[currentUserIdx].fiLog;

	}
}

/* saves password in encripted form 
 * input password char string, index into passowrd table;
 * returns a fail status if password is invalid or index out of bounds,
 * else returns success.
 * 
 * NOTE: currently very simple encoding is implemented
 */
utlErrno_t usrSetPassword(char *password_p, int idx) {
	if (!password_p || (idx < 1) || (idx >= USR_MAX)) {
		return (utlArgValERROR);
	}

	int i;
	int len = utlStrLen(password_p);
	if (len > USR_MAX_PASSWORD_LEN) {
		return (utlArgValERROR);
	}

	for (i = 0; i < len; i++) {
		password[idx][i] = usrEncript((uint8_t) password_p[i]);
	}

	for (; i < USR_MAX_PASSWORD_LEN; i++) {
		password[idx][i] = 0;
	}

	return (utlNoERROR);
}
bool usrMatchPassword(char *password_p, int idx) {
	if (!password_p || (idx < 0) || (idx >= USR_MAX)) {
		return false;
	}

	int i;
	bool match = true;
	unsigned len = utlStrLen(password_p);
	for (i = 0; (i < USR_MAX_PASSWORD_LEN) && (i < len) && match; i++) {

		if (password[idx][i] != usrEncript((uint8_t) password_p[i])) {
			match = false;
		}
	}

	for (; (i < USR_MAX_PASSWORD_LEN) && match; i++) {
		/* the rest of encrypted password must be 0 to match */
		if (password[idx][i] != 0) {
			match = false;
		}
	}

	return match;
}

/* finds and free indes in user table; if table is full, returns max idx */
int usrGetFreeIdx(void) {
	int i, j;
	bool found = false;
	/* the first entry is reserved for admin */
	for (i = 1, j = 0; (i < USR_MAX) && (j < userNum) && !found; i++) {
		if (!user[i].set) {
			found = true;
		} else {
			j++;
		}
	}

	if (found) {
		i--;
	} else {
		i = USR_MAX;
	}
	return i;

}

/* checks if name is reserved
 */
bool usrReservedName(char *name_p) {
	if (!name_p) {
		return false;
	}

	if (utlStrNCmp(name_p, USR_0_NAME, USR_MAX_NAME_LEN)) {
		return true;
	} else {
		return false;
	}
}

bool usrReservedGroup(char *group_p) {
	if (!group_p) {
		return false;
	}

	if (utlStrNCmp(group_p, USR_0_GROUP, USR_MAX_NAME_LEN)) {
		return true;
	} else {
		return false;
	}
}

int usrMatch(char *name_p) {
	int i, j;
	bool found = false;
	if (!name_p) {
		return (USR_MAX);
	}

	/* NOTE: userNum is at least 1, since admi user is registered during shell startup */
	for (i = 0, j = 0; (i < USR_MAX) && (j < userNum) && !found; i++) {
		if (user[i].set) {
			j++;
			if (utlStrNCmp(name_p, user[i].name, USR_MAX_NAME_LEN)) {
				found = true;
			}
		}
	}
	if (found) {
		i--;
	} else {
		i = USR_MAX;
	}
	return i;
}

bool usrAdmin(void) {
	if (currentUserIdx == 0) {
		return true;
	} else {
		return false;
	}
}

bool usrShell(void) {
	if (currentUserIdx == USR_NO_ACTIVE) {
		return true;
	} else {
		return false;
	}
}

utlErrno_t usrAdd(char *name_p, char *group_p, char *password_p) {

	uint16_t ulen = 0;
	uint16_t glen = 0;
	uint16_t plen = 0;

	utlErrno_t sts;

	/* the current user is not admin */
	if (!usrAdmin()) {
		logWrite(FIO_LAST_ERROR, "unprivileged user",
				"attempted to execute 'adduser'");
		return (utlPrivERROR);
	}

	/* no more users can be added */
	if (userNum == USR_MAX) {
		return (utlMaxUsersERROR);
	}

	/* validate name, group, password */
	ulen = usrValidName(name_p);
	glen = usrValidGroup(group_p);
	plen = usrValidPassword(password_p);
	if (!ulen || !glen || !plen) {
		return (utlArgValERROR);
	}

	/* check if already exists */

	int idx = usrMatch(name_p);
	if (idx != USR_MAX) {
		/* user name already added */
		return (utlNotUniqueUserERROR);
	}

	/* check if there is a free slot in user table */
	idx = usrGetFreeIdx();
	if (idx == USR_MAX) {
		/* no free slot left */
		return (utlMaxUsersERROR);
	}

	/* check and set password */
	sts = usrSetPassword(password_p, idx);
	if (sts != utlNoERROR) {
		return sts;
	}

	/* set user name/group info */
	utlStrMCpy(user[idx].name, name_p, ulen, USR_MAX_NAME_LEN);
	utlStrMCpy(user[idx].group, group_p, glen, USR_MAX_NAME_LEN);
	user[idx].fiLog = -1;

	/* update number of users */
	user[idx].set = true;
	userNum++;

	return (utlNoERROR);
}
uint16_t usrValidName(char *name_p) {

	if (!name_p) {
		return 0;
	}
	uint16_t len = utlStrLen(name_p);
	if ((len < USR_MIN_NAME_LEN) || (len > USR_MAX_NAME_LEN)) {
		return 0;
	}

	int i;

	for (i = 0; i < len; i++) {
		if (((name_p[i] < 'A') || (name_p[i] > 'z'))
				&& ((name_p[i] < '0') || (name_p[i] > '9')))
			return (0);
	}

	if (usrReservedName(name_p)) {
		return 0;
	}

	return len;
}
uint16_t usrValidGroup(char *group_p) {
	if (!group_p) {
		return 0;
	}
	uint16_t len = utlStrLen(group_p);
	if ((len < USR_MIN_NAME_LEN) || (len > USR_MAX_NAME_LEN)) {
		return 0;
	}

	int i;

	for (i = 0; i < len; i++) {
		if (((group_p[i] < 'A') || (group_p[i] > 'z'))
				&& ((group_p[i] < '0') || (group_p[i] > '9')))
			return 0;
	}

	if (usrReservedGroup(group_p)) {
		return 0;
	}

	return len;
}

uint16_t usrValidPassword(char *password_p) {
	if (!password_p) {
		return 0;
	}
	uint16_t len = utlStrLen(password_p);
	if ((len < USR_MIN_PASSWORD_LEN) || (len > USR_MAX_PASSWORD_LEN)) {
		return 0;
	}

	int i;

	for (i = 0; i < len; i++) {
		if (((password_p[i] < 'A') || (password_p[i] > 'z'))
				&& ((password_p[i] < '0') || (password_p[i] > '9'))
				&& (password_p[i] != '$') && (password_p[i] != '@')
				&& (password_p[i] != '_') && (password_p[i] != '-'))
			return 0;
	}
	return len;
}

utlErrno_t usrRemove(char *name_p) {

	/* the current user is not admin */
	if (!usrAdmin()) {
		logWrite(FIO_LAST_ERROR, "unprivileged user",
				"attempted to execute 'remuser'");
		return (utlPrivERROR);
	}

	/* no users other than admin registered */
	if (userNum <= 1) {
		return (utlFailERROR);
	}

	uint16_t ulen = 0;
	ulen = usrValidName(name_p);
	if (!ulen) {
		return (utlArgValERROR);
	}

	/* check if exists and is not "admin" */
	int idx = usrMatch(name_p);
	if ((idx == USR_MAX) || (idx == 0)) {
		return (utlNoSuchUserERROR);
	}

	/* remove user */
	user[idx].set = false;
	userNum--;

	return (utlNoERROR);
}

#define USER_NAME    SVCprintStr("\rUSER NAME: ")
#define PASSWORD     SVCprintStr("\r\nPASSWORD: ")
#define LOG_ERR      SVCprintStr("\r\nincorrect USER NAME or PASSWORD\r\n")
bool gettingPasswrd;

bool usrLogin(void) {

	/* someone already logged in */
	if (currentUserIdx != USR_NO_ACTIVE) {
		return true;
	}

	char userName[USR_MAX_NAME_LEN + 1];
	char userPassword[USR_MAX_PASSWORD_LEN + 1];

	bool exit = false;
	bool in = false;
	bool gotUser;

	while (!in && !exit) {

		while (!in) {
			/* get user name */
			USER_NAME;
			gettingPasswrd = false;
			gotUser = usrGetWord(userName, USR_MAX_NAME_LEN);
			if (gotUser && utlStrCmp(userName, "exit")) {
				exit = true;
				break;
			}

			/* get password */
			/* prompt for password before checking if user name is correct */
			/* for security reasons: to make less obvious where the mistakes is */

			PASSWORD;
			gettingPasswrd = true;
			if (!gotUser || !usrGetWord(userPassword, USR_MAX_PASSWORD_LEN)) {
				break;
			}
			SVCprintStr("\r\n");
			/* match user name/password */

			uint16_t ulen;
			ulen = usrValidName(userName);
			if (!ulen) {
				break;
			}

			uint16_t plen;
			plen = usrValidPassword(userPassword);
			if (!plen) {
				break;
			}

			int idx;
			idx = usrMatch(userName);
			if (idx == USR_MAX) {
				break;
			}

			if (!usrMatchPassword(userPassword, idx)) {
				break;
			}

			in = true;
			currentUserIdx = idx;
		}
		if (!in && !exit) {
			LOG_ERR;
		}
	}

	/* open log file */
	if (in) {
		ProcessControlBlock *pcb_p = getCurrentPCB();
		char *mode_p;
		if (usrShell() || usrAdmin()) {
			mode_p = "r";
			user[currentUserIdx].fiLog = myfopen(FIO_LOG_FILE, mode_p);
		}
		mode_p = "a";
		char str[shMAX_BUFFERSIZE + 1];
		if (pcb_p) {
			pcb_p->fiLog = myfopen(FIO_LOG_FILE, mode_p);
		}

		logWrite(FIO_LAST_ERROR, userName ,"logged in");
	}
	return in;
}

uint16_t usrEncript(uint8_t num) {
	return ((uint16_t) utlPower((unsigned) num, 2));
}

void usrLogout(void) {
	if (currentUserIdx != USR_NO_ACTIVE) {
		logWrite(FIO_LAST_ERROR, user[currentUserIdx].name, "logged out");
	}

	/* close files opened by this user */
	pcbCloseUsrStreams(NULL);
	user[currentUserIdx].fiLog = -1;
	currentUserIdx = USR_NO_ACTIVE;
}

bool usrGetWord(char *word_p, unsigned len) {
	bool eol = false;
	unsigned i;

	if (!word_p) {
		return false;
	}

	for (i = 0; !eol && (i < len); i++, word_p++) {
		if (!gettingPasswrd) {
			*word_p = SVCgetChar();
		} else {
			*word_p = SVCgetCharNoEch();
		}

		if ((*word_p == ' ') || (*word_p == '\t')){
			break; /* must be a single word entered */
		}

		if ((*word_p == EOF) || (*word_p == '\n') || (*word_p == '\r')) {
			*word_p = '\0';
			eol = true;
		}

	}

	if (!eol) {
		return false;
	}

	return true;
}
