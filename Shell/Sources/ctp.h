#ifndef CTP_
#define CTP_

#include <stdio.h>

#define PORT_PCR_MUX_ANALOG 0
#define PORT_PCR_MUX_GPIO 1

#define ELECTRODE_COUNT 4
#define THRESHOLD_OFFSET 0x200

void TSI_Init(void);
void TSI_Calibrate(void);
int electrode_in(int electrodeNumber);

#endif /* CTP_ */
