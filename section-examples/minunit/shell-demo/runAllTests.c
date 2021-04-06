//
// Created by Mark Ford on 9/18/18.
//

#include "minunit.h"

#include <stdio.h>
#include <stdbool.h>
#include "minunit.h"
#include "shell.h"

int tests_run = 0;
int assertions_run = 0;

static bool isEqual(struct timefields *one, struct timefields *two);
static bool isEqualInEpochMillis(struct timefields *expected, uint64_t timeInMillis);


static char * test_calcArgc() {
    mu_assert("2 args", calcArgc("two args") == 2);
    mu_assert("2 args with spaces", calcArgc("two       args") == 2);
    mu_assert("2 args with tab", calcArgc("two   \t    args") == 2);
    return 0;
}

static char * test_populateTimeFields() {

    struct timefields epoch = {1970,1,1,0,0,0,0};
    struct timefields epoch_plus_1024_seconds = {1970,1,1,0,17,4,0};

    mu_assert("epoch", isEqualInEpochMillis(&epoch, 0));
    mu_assert("epoch", isEqualInEpochMillis(&epoch_plus_1024_seconds, 1024*1000));

    return 0;
}

static char * all_tests() {
    mu_run_test(test_calcArgc);
    mu_run_test(test_populateTimeFields);
    return 0;
}

static bool isEqual(struct timefields *one, struct timefields *two) {
    return one->year == two->year &&
           one->month == two->month &&
           one->dayOfYear == two->dayOfYear &&
           one->hourOfDay == two->hourOfDay &&
           one->minuteOfHour == two->minuteOfHour &&
           one->secondOfHour == two->secondOfHour &&
           one->millisecondOfHour == two->millisecondOfHour;
}

static bool isEqualInEpochMillis(struct timefields *expected, uint64_t timeInMillis) {
    struct timefields actual;
    populateTimeFields(&actual, timeInMillis);

    return isEqual(expected, &actual);
}

int main(int argc, char **argv) {
    char *errMessage = all_tests();
    printf("Assertions run: %d\n", assertions_run);
    if (errMessage != 0) {
        printf("**** TEST FAILURE ****\n");
        printf("%s\n", errMessage);
    }
    else {
        printf("ALL TESTS PASSED\n");
    }
    printf("Tests run: %d\n", tests_run);

    return errMessage != 0;
}

