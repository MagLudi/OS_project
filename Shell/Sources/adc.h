#ifndef ADC_
#define ADC_

#include <stdio.h>
#include <stdint.h>

#define FALSE 0
#define TRUE 1

#define ADC_CHANNEL_POTENTIOMETER   	0x14
#define ADC_CHANNEL_TEMPERATURE_SENSOR  0x1A

#define ADC_CFG1_MODE_8_9_BIT       0x0
#define ADC_CFG1_MODE_12_13_BIT     0x1
#define ADC_CFG1_MODE_10_11_BIT     0x2
#define ADC_CFG1_MODE_16_BIT        0x3
#define ADC_SC3_AVGS_32_SAMPLES     0x3

void adc_init(void);
unsigned int adc_read(uint8_t channel);

#endif /* ADC_ */
