//
// Created by Mark Ford on 9/18/18.
//

#ifndef SHELL_DEMO_SHELL_H
#define SHELL_DEMO_SHELL_H

#include <stdint.h>

struct timefields {
    int year;
    int month;
    int dayOfYear;
    int hourOfDay;
    int minuteOfHour;
    int secondOfHour;
    int millisecondOfHour;
};

int calcArgc(char* line);

int populateTimeFields(struct timefields *result, uint64_t timeInMillis);

#endif //SHELL_DEMO_SHELL_H
