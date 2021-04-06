/*
 * File:        rtc.c
 * Purpose:     Provide common RTC routines
 *
 * Notes:       
 *              
 */

#include "rtc.h"
#include "utl.h"

/********************************************************************/
/*
 * Initialize the RTC
 *
 *
 * Parameters:
 *  seconds         Start value of seconds register
 *  alarm           Time in seconds of first alarm. Set to 0xFFFFFFFF to effectively disable alarm
 *  c_interval      Interval at which to apply time compensation can range from 1 second (0x0) to 256 (0xFF)
 *  c_value         Compensation value ranges from -127 32kHz cycles to +128 32 kHz cycles
 *                  80h Time prescaler register overflows every 32896 clock cycles.
 *                  FFh Time prescaler register overflows every 32769 clock cycles.
 *                  00h Time prescaler register overflows every 32768 clock cycles.
 *                  01h Time prescaler register overflows every 32767 clock cycles.
 *                  7Fh Time prescaler register overflows every 32641 clock cycles.
 *  interrupt       TRUE or FALSE
 */
bool set;
void rtc_init() {
	int i;
	set = false;

	/*enable the clock to SRTC module register space*/
	SIM_SCGC6 |= SIM_SCGC6_RTC_MASK;

	if ((RTC_SR & RTC_SR_TIF_MASK) == 0x01) {
		RTC_CR = RTC_CR_SWR_MASK;
		RTC_CR &= ~RTC_CR_SWR_MASK;
		RTC_SR &= 0x07;  //clear TCE, or RTC_TSR can  not be written
		RTC_TSR = 0x00000000; //clear TIF
		/*clear the software reset bit*/

	} /*Only VBAT_POR has an effect on the SRTC, RESET to the part does not, so you must manually reset the SRTC to make sure everything is in a known state*/

	/*Enable the oscillator*/
	RTC_CR |= RTC_CR_OSCE_MASK;

	/*Wait to all the 32 kHz to stabilize, refer to the crystal startup time in the crystal datasheet*/
	for (i = 0; i < 0x600000; i++)
		;

	/*Set time compensation parameters to 0 since compensation not needed*/
	RTC_TCR = RTC_TCR_CIR(0) | RTC_TCR_TCR(0);

	/*Configure the timer seconds with value passed in and alarm to 0 since not needed*/
	RTC_TAR = 0;

	if(RTC_TSR > 0){
		set = true;
	}

	/*Enable the counter*/
	RTC_SR |= RTC_SR_TCE_MASK;

}

void rtc_set(uint32_t seconds){
	di();
	set = true;
	RTC_SR &= 0x07;  //clear TCE, or RTC_TSR can  not be written
	RTC_TSR = seconds;
	RTC_SR |= RTC_SR_TCE_MASK;
	ei();
}

void rtc_isr(void) {

	/*if((RTC_SR & RTC_SR_TIF_MASK)== 0x01)
	 {
	 RTC_SR &= 0x07;  //clear TCE, or RTC_TSR can  not be written
	 RTC_TSR = 0x00000000;  //clear TIF
	 }
	 else if((RTC_SR & RTC_SR_TOF_MASK) == 0x02)
	 {
	 RTC_SR &= 0x07;  //clear TCE, or RTC_TSR can  not be written
	 RTC_TSR = 0x00000000;  //clear TOF
	 }
	 else if((RTC_SR & RTC_SR_TAF_MASK) == 0x04)
	 {
	 RTC_TAR = 0;// Write new alarm value, to generate an alarm every second add 1
	 }*/

	return;
}

uint32_t get_time(void) {
	return (uint32_t) RTC_TSR;
}

bool is_set(void){
	return set;
}
