/*
 * File:		rtc.h
 * Purpose:     Provide common rtc functions
 *
 * Notes:
 */

#ifndef __RTC_H__
#define __RTC_H__
#include "derivative.h"
#include <stdbool.h>
/********************************************************************/

void rtc_init();
void rtc_isr(void);
uint32_t get_time(void);
void rtc_set(uint32_t seconds);
bool is_set(void);

/********************************************************************/

#endif /* __RTC_H__ */
