#ifndef TM__
#define TM__

/* header file for date/time utilities */

/* system headers */
#include <sys/time.h>
#include "utl.h"
#include "flexTimer.h"

/* function declarations */

utlDate_t tmCurrent(bool inSVC);
uint64_t tmISO(const char *isoTime_p);
utlErrno_t tmSet(uint64_t msec);
utlDate_t tmCurrentRTC(bool inSVC);

#endif /* TM__ */
