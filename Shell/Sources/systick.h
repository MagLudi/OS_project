#ifndef _SYSTICK
#define _SYSTICK

#include <stdint.h>
#include <utl.h>

#define Systick_MaxPriority 15
#define Systick_PriorityShift 4

#define PendSV_MaxPriority 15
#define PendSV_PriorityShift 4

/* Second lowest priority (14): quantum interrupt */
#define Systick_Priority 14
#define PendSV_Priority 14

#define PROCESS_QUANTUM 19200000

void systickInit_SetSystickPriority(unsigned char priority);
void setPendSV(unsigned char priority);
void systickInit();
void stHandler();

#endif /* ifndef _SYSTICK */
