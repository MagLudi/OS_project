/* Pushbutton LED rotation with debouncing. After initlaization, the
 * orange LED will be on. Each time pushbutton SW1 is depressed, the
 * lit LED will rotate one position. If the last LED is on (blue), then
 * the process with start again the next time SW1 is depressed. A delay
 * has been added after each time the button is pressed to eliminate
 * false positives due to contact bounce.
 *
 * The code provided in the header and source files for "led", "delay",
 * and "pushbutton" are the files that were provided by Prof. Frankel on
 * the course website. The main routine was written from scratch.
 */

#include <stdio.h>
#include <stdbool.h>
#include "led.h"
#include "delay.h"
#include "pushbutton.h"

int main() {
	const unsigned long int delayCount = 0x4ffff;
	bool orange = true;
	bool yellow = false;
	bool green = false;
	bool blue = false;

	printf("Pushbutton Delay Project Starting\n");
	
	/* Initialize all of the LEDs */
	ledInitAll();
	/* Initialize both of the pushbuttons */
	pushbuttonInitAll();
	/* turn on orange LED */
	ledOrangeOn();

	while(1) {
		if(sw1In()) {
			if(orange){
				ledOrangeOff();
				ledYellowOn();
				orange = false;
				yellow = true;
			}else if(yellow){
				ledYellowOff();
				ledGreenOn();
				yellow = false;
				green = true;
			} else if(green){
				ledGreenOff();
				ledBlueOn();
				green = false;
				blue = true;
			}else {
				ledBlueOff();
				ledOrangeOn();
				blue = false;
				orange = true;
			}
		}
		delay(delayCount);
	}
	
	printf("Pushbutton Delay Project Completed\n");

	return 0;
}

