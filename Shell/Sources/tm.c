/*
 * this module contains date/time utilities:
 *
 * tmISO - converts from ISO format to milliseconds
 * tmSet - sets system time
 * tmCurrent - gets current sys time
 */

#include <sys/time.h>
#include "utl.h"
#include "tm.h"
#include "svc.h"
#include "rtc.h"

extern utlMonth_t month[];

/* convert ts from ISO format to milliseconds since our epoch of  midnight (zero hours) on January 1, 19;
 * takes in a character date/time string in ISO format, and returns milliseconds if no errors encountered,
 * otherwise returns 0;
 *  errors include  year < 1970,or any invalid date/time specification
 */

uint64_t tmISO(const char *isoTime_p) {
	utlDate_t date;
	unsigned short n;

	struct timeval setTime;
	uint64_t msec = 0;
	date.set = false;
	n = sscanf(isoTime_p, "%u-%u-%uT%u:%u", &date.year, &date.month, &date.day,
			&date.hour, &date.min);

	if ((n == 5) && (date.year >= utlYearStart) && (date.month > 0)
			&& (date.month <= 12) && (date.day > 0)
			&& ((date.day <= month[date.month - 1].days)
					|| (utlLeap(date.year) && ((date.month - 1) == FEB)
							&& (date.day <= (month[FEB].days + 1))))
			&& (date.hour < 24) && (date.min < 60)) {
		date.month -= 1; /* adjust to math month enum */
		date.sec = 0;
		date.msec = 0;
		date.set = true;

		utlUtcYear(&date, &setTime.tv_sec);
		utlUtcMonthDayTime(&date, &setTime.tv_sec);

		msec = ((uint64_t) setTime.tv_sec) * ((uint64_t) 1000);
	} else {
		msec = 0;
	}

	return msec;
}

/* sets system time to requested value;
 * takes in milliseconds since our epoch of  midnight (zero hours) on January 1, 1970;
 * returns execution status
 */
utlErrno_t tmSet(uint64_t msec) {
	uint32_t low32, high32;
	high32 = (uint32_t) (msec >> 32);
	low32 = (uint32_t) msec;
	SVCsetClock(high32, low32);
	SVCsetRTC((uint32_t) (msec/((uint32_t) 1000)));
	//SVCsetRTC(1500000000);

	return utlNoERROR;
}

/*
 * gets current system time and returns it in a date structure variable;
 * has no inputs
 */
utlDate_t tmCurrent(bool inSVC) {
	uint64_t msec;
	struct timeval currentTime;

	utlDate_t date;
	date.set = false;
	uint32_t h, l;
	if (!inSVC) {
		SVCgetClock(&h, &l);
	} else {
		di();
		h = flexTimerGetClockHi();
		l = flexTimerGetClockLow();
		ei();
	}
	msec = (((uint64_t) h) << 32) | ((uint64_t) l);

	if (msec == 0){
		return date;
	}

	currentTime.tv_sec = msec / 1000;
	currentTime.tv_usec = 0;

	utlSetYear(&currentTime.tv_sec, &date);

	utlSetMonthDayTime(&currentTime.tv_sec, &date);

	date.msec = msec % 1000;
	date.set = true;

	return (date);
}

utlDate_t tmCurrentRTC(bool inSVC) {
	struct timeval currentTime;

	utlDate_t date;
	date.set = false;
	if (!inSVC) {
		currentTime.tv_sec = SVCgetRTC();
	} else {
		currentTime.tv_sec = get_time();
	}
	currentTime.tv_usec = 0;

	if (currentTime.tv_sec == 0) {
		return date;
	}

	utlSetYear(&currentTime.tv_sec, &date);

	utlSetMonthDayTime(&currentTime.tv_sec, &date);

	date.msec = 0;
	date.set = true;

	return (date);
}
