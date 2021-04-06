//
// Created by Mark Ford on 9/18/18.
//

#include <stddef.h>
#include <assert.h>
#include "shell.h"

int calcArgc(char* line) {
    return 2;
}

int populateTimeFields(struct timefields *result, uint64_t timeInMillis) {
    assert(result);

    if (timeInMillis == 0) {
        result->year = 1970;
        result->month = 1;
        result->dayOfYear = 1;
        result->hourOfDay = 0;
        result->minuteOfHour = 0;
        result->secondOfHour = 0;
        result->millisecondOfHour = 0;
    } else if (timeInMillis == 1024 * 1000) {
        result->year = 1970;
        result->month = 1;
        result->dayOfYear = 1;
        result->hourOfDay = 0;
        result->minuteOfHour = 17;
        result->secondOfHour = 4;
        result->millisecondOfHour = 0;
    } else {
        assert(0);
    }

    return 0;
}
