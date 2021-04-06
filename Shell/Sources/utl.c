/* Contains all the util functions pertaining to strings, errors, linked lists, and date/time */

#include <sys/time.h>
#include <stdio.h>
#include <stdlib.h>
#include "utl.h" 
#include "flexTimer.h"
#include "svc.h"

const utlMonth_t month[] = {{31,"January"},
							{28,"February"},
							{31,"March"},
							{30,"April"},
							{31,"May"},
							{30,"June"},
							{31,"July"},
							{31,"August"},
							{30,"September"},
							{31,"October"},
							{30,"November"},
							{31,"December"}};

const unsigned utlBitSizeMax = sizeof(UtlAddress_t) * CHAR_BIT;

int diCnt = 0;

/* initialize utl global variables */
void utlInit(void) {
	utlProgramName_p = NULL;
	utlStatus = utlSUCCESS;
	utlErrno = utlNoERROR;

	/* setup error code  messages */
	utlErrorMsg_a[0] = "%s::invalid error num\r\n";
	utlErrorMsg_a[utlMemERROR - utlBaseERROR] =
			"%s:: error num: %d - failed to allocate memory\r\n";
	utlErrorMsg_a[utlNoInputERROR - utlBaseERROR] =
			"%s:: error num: %d -  incorrect num of arguments\r\n";
	utlErrorMsg_a[utlArgNumERROR - utlBaseERROR] =
			"%s:: error num: %d -  incorrect num of arguments\r\n";
	utlErrorMsg_a[utlArgValERROR - utlBaseERROR] =
			"%s:: error num: %d -  invalid  argument\n";
	utlErrorMsg_a[utlArgFormatERROR - utlBaseERROR] =
			"%s:: error num: %d -  invalid format for set command\r\n";
	utlErrorMsg_a[utlInvCmdERROR - utlBaseERROR] =
			"%s:: error num: %d -  invalid  command\r\n";
	utlErrorMsg_a[utlFailERROR - utlBaseERROR] =
			"%s:: error num: %d -  failed to execute\r\n";
	utlErrorMsg_a[utlSetVarERROR - utlBaseERROR] =
			"%s:: error num: %d -  already set\r\n";
	utlErrorMsg_a[utlVarNotFoundERROR - utlBaseERROR] =
			"%s:: error num: %d - env var not found\r\n";
	utlErrorMsg_a[utlValSubERROR - utlBaseERROR] =
			"%s:: error num: %d -  substitution failed\r\n";
	utlErrorMsg_a[utlPrivERROR - utlBaseERROR] =
			"%s:: error num: %d - admin privilege required\r\n";
	utlErrorMsg_a[utlInvUserNameERROR - utlBaseERROR] =
			"%s:: error num: %d - invalid user name\r\n";
	utlErrorMsg_a[utlInvGrpNameERROR - utlBaseERROR] =
			"%s:: error num: %d - invalid group name\r\n";
	utlErrorMsg_a[utlInvPasswordERROR - utlBaseERROR] =
			"%s:: error num: %d - invalid password\r\n";
	utlErrorMsg_a[utlMaxUsersERROR - utlBaseERROR] =
			"%s:: error num: %d - cannot add more users\r\n";
	utlErrorMsg_a[utlNoSuchUserERROR - utlBaseERROR] =
			"%s:: error num: %d - no such user registered\r\n";
	utlErrorMsg_a[utlNotUniqueUserERROR - utlBaseERROR] =
			"%s:: error num: %d - user name not unique\r\n";
	utlErrorMsg_a[utlNoCmdERROR - utlBaseERROR] = "";
}

/* the following string manipulation functions were adapted  from The C Programming Language by Kernighan and Ritchie
 * with some modifications
 */

/* converts char string to int; string must be delimited by '\0'
 */
unsigned utlAtoI(char *string_p) {
	if (string_p == NULL) {
		return 0;
	}

	unsigned j, n;

	for (j = 0, n = 0; string_p[j] >= '0' && string_p[j] <= '9'; j++) {
		n = 10 * n + (string_p[j] - '0');
	}

	return (n);

}

/* converts char int to octal string; adapted from The C Programming Language by Kernighan and Ritchie */
char * utlItoOA(UtlAddress_t octalNum) {
	static char octalStr_a[UTL_MAX_DIGITS + 2];
	unsigned i;

	do {
		octalStr_a[i++] = octalNum % 10 + '0';
	} while (((octalNum /= 10) > 0) && (i < UTL_MAX_DIGITS));

	octalStr_a[i++] = 'O';
	octalStr_a[i] = '\0';

	return (octalStr_a);
}

/* compares char strings; returns <0 if str1_p<str2_p, 0 - equal, and >0 if str1_p > str2_p
 */
bool utlStrCmp(char *str1_p, char *str2_p) {
	if (!str1_p || !str2_p) {
		return false;
	}
	for (; *str1_p == *str2_p; str1_p++, str2_p++) {
		if (*str1_p == '\0') {
			return (true);
		}
	}
	return (false);

}

/* copies char string; str2_p into str1_p */
void utlStrCpy(char *str1_p, char *str2_p) {
	if (!str1_p || !str2_p) {
		return;
	}

	while ((*str1_p++ = *str2_p++) != '\0'){;}
}

/* copies N char froms str2 to str1; terminates str1 with '\0' */
void utlStrNCpy(char *str1_p, char *str2_p, int n) {
	if (!str1_p || !str2_p) {
		return;
	}

	int i = 0;
	for (i = 0; i < n; i++) {
		*str1_p++ = *str2_p++;
	}
	*str1_p++ = '\0';

}

bool utlStrNCmp(char *str1_p, char *str2_p, int n) {
	if (!str1_p || !str2_p) {
		return false;
	}

	int i = 0;

	for (i = 0; (*str1_p == *str2_p) && (i < n); str1_p++, str2_p++, i++){
		if (*str1_p == '\0'){
			return (true);
		}
	}
	if (i == n){
		return (true);
	}else{
		return (false);
	}

}

/* n - the number of char tha must be matched m = max len
 * if str1 len < m it is terminated by '\0', else may not be terminated
 * n = str2 len to compare, and str2 may not be terminated by '\0';
 */
bool utlStrMCmp(char *str1_p, char *str2_p, int n, int m) {
	if (!str1_p || !str2_p) {
		return false;
	}

	int i = 0;

	if (n > m){
		return false;
	}

	for (i = 0; (i < n) && (*str1_p == *str2_p); str1_p++, str2_p++, i++){;}

	if (i < n){
		return false;
	}
	if (n == m){
		return true;
	}
	if (*str1_p != '\0'){
		return false;
	}else{
		return true;
	}

}

void utlStrMCpy(char *str1_p, char *str2_p, int n, int m) {
	if (!str1_p || !str2_p){
		return;
	}

	int i = 0;
	for (i = 0; i < n && (*str2_p != '\0'); i++){
		*str1_p++ = *str2_p++;
	}
	for (; i < m; i++){
		*str1_p++ = '\0';
	}
}

/* takes in a string and returns its length */
int utlStrLen(char *str_p) {
	if (!str_p) {
		return -1;
	}

	int i = 0;
	for (i = 0; str_p[i] != '\0'; i++){;}
	return (i);
}

/* reverses strings; from The C Programming Language by Kernighan and Ritchie */
void utlStrReverse(char *str_p) {
	if (!str_p) {
		return;
	}
	int c, i, j;
	for (i = 0, j = utlStrLen(str_p) - 1; i < j; i++, j--) {
		c = str_p[i];
		str_p[i] = str_p[j];
		str_p[j] = c;
	}
}

/* checks to see if a particular char is in a string and returns
 * its index. Returns -1 if not found
 */
int utlCharFind(char *str_p, char c, int n) {
	if (!str_p) {
		return -1;
	}
	bool found = false;
	int i = 0;
	while ((i < n) && !found && (str_p[i] != '\0')) {
		if (str_p[i] == c) {
			found = true;
		} else {
			i++;
		}
	}
	if (!found) {
		i = -1;
	}
	return (i);
}

/* power function */
unsigned utlPower(unsigned x, unsigned n) {
	unsigned p;
	for (p = 1; n > 0; --n) {
		p = p * x;
	}
	return (p);
}

/* to ensure that an arbitrary length number string can be parsed, the following conversion functions
 * are created, instead of using strtol or strtoul, since they limit conversion to 32 bit number;
 * if running on 64 bit machine, allocated address may exeede 32; string must be delimited by '\0'.
 */
UtlAddress_t utlAtoD(char *string_p) {
	if (string_p == NULL) {
		return (UtlAddress_t) NULL;
	}

	if ((string_p[0] == '0') || (string_p[0] == 'O') || (string_p[0] == 'o')) {
		if ((string_p[1] == 'x') || (string_p[1] == 'X')) {
			/* hex representation */
			return (utlAXtoD(&string_p[2]));
		} else {
			/* octal representation*/
			return (utlAOtoD(&string_p[1]));
		}
	} else if ((string_p[0] == '0') /* code added for 0 start for hex from PS2. Currently untested*/
			&& ((string_p[1] == 'x') || (string_p[1] == 'X'))) {
		return (utlAXtoD(&string_p[2]));
	} else if (string_p[0] == '-') {
		/* signed decimal representation */
		return ((UtlAddress_t) utlADtoD(string_p));
	} else {
		/* unsigned decimal representation */
		return (utlADtoUD(string_p));
	}
}

/* converts signed decimal char string. Checks for overflow/underflow, and returns maximum/minimum
 * possible value if overflow/underflow is detected
 */
UtlInt_t utlADtoD(char *string_p) {
	UtlInt_t num = 0;

	if (string_p == NULL) {
		return 0;
	}

	unsigned j;
	bool negative = false;
	bool overflow = false;

	if (*string_p == '-') {
		negative = true;
		string_p++;
	}

	for (j = 0, num = 0;
			(string_p[j] >= '0') && (string_p[j] <= '9') && !overflow; j++) {
		UtlAddress_t addent = (string_p[j] - '0');

		if (negative
				&& ((num > (UTL_INT_MIN / (-10)))
						|| ((UTL_INT_MIN + (10 * num)) > ((-1) * addent)))) {
			overflow = true;
			num = UTL_INT_MIN;
		} else if ((num > (UTL_INT_MAX / 10))
				|| ((UTL_ADDR_MAX - (10 * num)) < addent)) {
			overflow = true;
			num = UTL_INT_MAX;
		} else {
			num = 10 * num + addent;
		}
	}

	if (negative) {
		num = num * (-1);
	}
	return (num);
}

/* converts 'unsigned' decimal char string checks for overflow, and returns maximum possible
 * value if overflow is detected
 */
UtlAddress_t utlADtoUD(char *string_p) {
	UtlAddress_t num = 0;

	if (string_p == NULL) {
		return 0;
	}

	unsigned j;
	bool overflow = false;
	for (j = 0, num = 0;
			(string_p[j] >= '0') && (string_p[j] <= '9') && !overflow; j++) {
		UtlAddress_t addent = string_p[j] - '0';
		if ((num > (UTL_ADDR_MAX / 10))
				|| ((UTL_ADDR_MAX - (10 * num)) < addent)) {
			overflow = true;
			num = UTL_ADDR_MAX;
		} else {
			num = 10 * num + addent;
		}
	}
	return (num);
}

/* converts non-negative octal char string; if invalid digit is encountered, whatever has been converted so far
 * is returned. Since octal digit takes 3 bits, the total number of bits is counted to check for overflow
 */
UtlAddress_t utlAOtoD(char *string_p) {
	if (string_p == NULL) {
		return 0;
	}

	UtlAddress_t octalNum = utlADtoUD(string_p);

	if (octalNum < 0) {
		/* underflow */
		return 0;
	}

	UtlAddress_t decimalNum = 0;
	unsigned bitCount = 0;
	unsigned octBitSizeMax = utlBitSizeMax - (utlBitSizeMax % 3);
	unsigned j;
	for (j = 0, decimalNum = 0, bitCount = 3;
			string_p[j] >= '0' && string_p[j] <= '8'
					&& (bitCount <= octBitSizeMax); j++) {
		decimalNum = decimalNum * 8 + (string_p[j] - '0');
		bitCount += 3;
	}

	if (bitCount > octBitSizeMax) {
		/* in case of overflow, return max value */
		return UTL_ADDR_MAX;
	}

	return decimalNum;
}

/* convert a non-negative hex string to address size number; if invalid character is encountered,
 * the rest of the string is ignored * on overflow, the max number is returned. addapted from
 * https://stackoverflow.com/questions/10156409/convert-hex-string-char-to-int; added overflow checking.
 * since hex digit takes 4 bits, the number of bits is counted to check for overflow
 */
UtlAddress_t utlAXtoD(char *string_p) {
	UtlAddress_t num;
	num = 0;

	if (string_p == NULL) {
		return 0;
	}

	unsigned bitCount = 0;
	bool validChar = true;

	while ((*string_p != '\0') && (bitCount < utlBitSizeMax) && validChar) {
		uint8_t byte = *string_p++;

		/* transform hex character to the 4bit equivalent number, using the ascii table indexes*/
		if (byte >= '0' && byte <= '9') {
			byte = byte - '0';
		} else if (byte >= 'a' && byte <= 'f') {
			byte = byte - 'a' + 10;
		} else if (byte >= 'A' && byte <= 'F') {
			byte = byte - 'A' + 10;
		} else {
			/* modification the adapted code sample */
			validChar = false;
		}

		if (validChar) {
			/* shift 4 to make space for new digit, and add the 4 bits of the new digit */
			num = (num << 4) | (byte & 0xF);
			bitCount += 4;
		}
	}

	/* modification the adapted code sample */
	if ((*string_p != '\0') && (bitCount >= utlBitSizeMax)) {
		/* in case of overflow, return max value */
		return UTL_ADDR_MAX;
	}
	return (num);
}

/* linked list functions */

/* create linked list */
utlStatus_t utlCreateLnkdList(utlLnkdListHdr_t *list_p) {
	utlStatus_t sts = utlSUCCESS;

	if (list_p == NULL) {
		utlErrno = utlArgValERROR;
		utlPrintERROR("utlCreateLnkdList");
		return (utlFAIL);
	};

	list_p->firstNode_p = NULL;
	list_p->lastNode_p = NULL;
	return (sts);
}

/* append link */
utlLnkdListNode_t *utlAppendLink_p(void *data_p, int flag,
		utlLnkdListHdr_t *list_p) {
	utlLnkdListNode_t *node_p = NULL;

	if ((list_p == NULL)) {
		utlRETURN(NULL, utlArgValERROR, "utlAppendLink_p");
	}

	node_p = (utlLnkdListNode_t *) SVCMalloc(sizeof(utlLnkdListNode_t));
	if (node_p == NULL) {
		utlPrintSysERROR("utlAppendLink_p:: malloc");
		utlRETURN(NULL, utlMemERROR, "utlAppendLink_p");
	}

	node_p->flag = flag;
	node_p->data_p = data_p;
	node_p->next_p = NULL;
	node_p->prev_p = list_p->lastNode_p;

	if (list_p->lastNode_p != NULL) {
		list_p->lastNode_p->next_p = node_p;
	} else {
		(list_p->firstNode_p = node_p);
	}

	list_p->lastNode_p = node_p;

	return (node_p);
}

/* remove link */
utlLnkdListNode_t *utlRemoveLink_p(utlLnkdListNode_t *node_p,
		utlLnkdListHdr_t *list_p) {
	utlLnkdListNode_t *nextNode_p;
	if ((node_p == NULL) || (list_p == NULL)) {
		utlRETURN(NULL, utlArgValERROR, "utlRemoveLind");
	}

	if (node_p->prev_p != NULL) {
		node_p->prev_p->next_p = node_p->next_p;
	} else {
		(list_p->firstNode_p = node_p->next_p);
	}

	if (node_p->next_p != NULL) {
		node_p->next_p->prev_p = node_p->prev_p;
	} else {
		list_p->lastNode_p = node_p->prev_p;
	}

	nextNode_p = node_p->next_p;

	if (node_p->data_p != NULL) {
		SVCFree(node_p->data_p);
	}
	SVCFree(node_p);

	return (nextNode_p);
}

/* insert link */
utlLnkdListNode_t *utlInsertLink_p(void *data_p, int flag,
		utlLnkdListNode_t *prevNode_p, utlLnkdListHdr_t *list_p) {
	utlLnkdListNode_t *node_p = NULL;

	if (list_p == NULL) {
		utlRETURN(NULL, utlArgValERROR, "utlInsertLink");
	}

	node_p = (utlLnkdListNode_t *) SVCMalloc(sizeof(utlLnkdListNode_t));
	if (node_p == NULL) {
		utlPrintSysERROR("utlInsertLink_p:: malloc");
		utlRETURN(NULL, utlMemERROR, "utlInsertLink_p");
	}

	if ((list_p->firstNode_p == NULL) && (list_p->lastNode_p == NULL)) {
		/* this is the only node */
		node_p = utlAppendLink_p(data_p, flag, list_p);
	} else if (prevNode_p == NULL) {
		/* this is new first node */
		node_p->flag = flag;
		node_p->data_p = data_p;

		node_p->prev_p = NULL;
		node_p->next_p = list_p->firstNode_p;
		list_p->firstNode_p->prev_p = node_p;
		list_p->firstNode_p = node_p;

	} else {
		node_p->flag = flag;
		node_p->data_p = data_p;
		node_p->prev_p = prevNode_p;
		node_p->next_p = prevNode_p->next_p;

		prevNode_p->next_p = node_p;

		if (node_p->next_p != NULL) {
			node_p->next_p->prev_p = node_p;
		} else {
			list_p->lastNode_p = node_p;
		}
	}

	return (node_p);
}

/* date/time functions */

/* figures out the current year. Does so by using the number of msec from 1970
 * calculates the total number of sec that year (checks if it's a leap year);
 * if the remaining time is less than the amount of time for the year in question,
 * then it means that we have found the current year, else sec for the year are
 * removed from the total elpased time and the process is repeataed.
 * the values taken in are a pointer to the total amount of elapsed time in sec
 * since 1970 (which decreases as it looks for the current year) and a pointer
 * to the date variable.
 */
void utlSetYear(time_t *tv_sec, utlDate_t *date_p) {
	unsigned year;
	bool current = false;

	year = utlYearStart;

	while (!current) {
		if ((*tv_sec < utlSYEAR)
				|| ((!(year % 400) || (!(year % 4) && (year % 100)))
						&& (*tv_sec < utlSLYEAR))) {
			current = true;
		} else {
			*tv_sec -= utlSYEAR;
			if (!(year % 400) || (!(year % 4) && (year % 100)))
				*tv_sec -= utlSDAY;
			year++;
		}
	}
	date_p->year = year;
}

/* figures out the current month, day, hour, minute, sec, and msec; does so by
 * using the remaining number of msec from 1970 after finding the year. Calculates the
 * total number of msec that month (checkin for leap year Feb). If the remaining time
 * is less than the amount of time for the month in question, then it means that we have
 * found the current month, else and removes it from the total elpased time and the process
 * repeats. After finding the current month, the day/hour/min/seconds are 
 * deturmined through straitforward division and mod calculations.
 * The values taken in are a pointer to the total amount of elapsed time since 1970
 * and a pointer to the date variable.
 */
void utlSetMonthDayTime(time_t *tv_sec, utlDate_t *date_p) {
	unsigned year = date_p->year;
	utlMonth_et mon;

	unsigned daysLeft;
	bool current = false;

	mon = JAN;

	daysLeft = ((unsigned) *tv_sec) / utlSDAY;

	while (!current) {
		if ((daysLeft < month[mon].days)
				|| ((!(year % 400) || (!(year % 4) && (year % 100)))
						&& (mon == FEB) && (daysLeft < (month[mon].days + 1)))) {
			current = true;
		} else {
			daysLeft -= month[mon].days;
			*tv_sec -= month[mon].days * utlSDAY;
			if ((!(year % 400) || (!(year % 4) && (year % 100)))
					&& (mon == FEB)) {
				daysLeft -= 1;
				*tv_sec -= utlSDAY;
			}
			mon++;
		}
	}

	date_p->month_p = month[mon].name_p;
	date_p->day = daysLeft + 1;
	*tv_sec = ((unsigned) *tv_sec) % utlSDAY;
	date_p->hour = ((unsigned) *tv_sec) / utlSHOUR;
	*tv_sec = ((unsigned) *tv_sec) % utlSHOUR;
	date_p->min = ((unsigned) *tv_sec) / utlSMIN;
	date_p->sec = ((unsigned) *tv_sec) % utlSMIN;
}

/* Gets the current date.
 * date/time is UTC timezone: coordinated universal time 4 hours ahead of EST
 */
utlDate_t utlGetDate(void) {
	struct timeval currentTime;

	utlDate_t date;

	if (gettimeofday(&currentTime, NULL) == 0) {
		utlSetYear(&currentTime.tv_sec, &date);

		utlSetMonthDayTime(&currentTime.tv_sec, &date);

		date.msec = currentTime.tv_usec;
		date.set = true;

	} else {
		date.set = false;
		utlErrno = utlFailERROR;
		utlPrintSysERROR("utlGetDate:: gettimofday");
	}
	return (date);
}

/* from UTC format to seconds:
 * converts from integer representing a year,   to seconds since utlYearStart(=1970)
 * takes in a pointer to date struct and a pointer to sec variable
 * returns void
 */
void utlUtcYear(utlDate_t *date_p, time_t *tv_sec) {
	unsigned year = utlYearStart;
	*tv_sec = 0;

	while (year < date_p->year) {
		*tv_sec += utlSYEAR;
		if (utlLeap(year)){
			*tv_sec += utlSDAY;
		}
		year++;
	}
}

/* from UTC format to seconds:
 * adds  month,day,time seconds total to the seconds passed in a sec variable
 * takes in a pointer to date struct that specifies month, day an time,
 * and a pointer to sec variable;
 * returns void
 */
void utlUtcMonthDayTime(utlDate_t *date_p, time_t *tv_sec) {
	unsigned year = date_p->year;
	utlMonth_et mon = JAN;

	while (mon < (date_p->month)) {
		*tv_sec += (month[mon].days * utlSDAY);
		if (utlLeap(year) && (mon == FEB)) {
			*tv_sec += utlSDAY;
		}
		mon++;
	}

	*tv_sec += (date_p->day - 1) * utlSDAY;
	*tv_sec += date_p->hour * utlSHOUR;
	*tv_sec += date_p->min * utlSMIN;
	*tv_sec += date_p->sec;
}

/* Interrupt enablers/disablers */
void ei(){
	diCnt--;
	if (diCnt <= 0) {
		__asm("cpsie i");
	}
}
void di(){
	diCnt++;
	__asm("cpsid i");
}
