#include <stdint.h>
#include <stdlib.h>
#include <math.h>

/*-----------------------------------------------------------------\\
				   CONTROLS - SWITCHES & BUTTONS
\\-----------------------------------------------------------------*/
	
uint32_t getCurrentSwitches(){
	volatile uint32_t *switches = (volatile uint32_t *) 0xFF200040;
	return *switches;
}

uint32_t getCurrentButtons(){
	volatile uint32_t *buttons = (volatile uint32_t *) 0xFF200050;
	return *buttons;
}

void delay(volatile int count) {
    while (count > 0) {
        count--;
    }
}

/*-----------------------------------------------------------------\\
							LED-METHODS
\\-----------------------------------------------------------------*/
	
#define LEDS ((volatile uint32_t *)0xFF200000)

uint32_t generateRandomLEDSeed() {
    uint32_t random_seed = 0; 
    for (int i = 0; i < 32; i++) {
        uint32_t bit = rand() % 2; 
        random_seed |= (bit << i); 
    }
    return random_seed;
}

void clearLEDS(){
	*LEDS = 0;
}

void rightToLeft(int delta_time, uint32_t led_value){
	for(int i = 0; i < 32; i++){
		*LEDS = led_value;
		led_value = (led_value << 1) | (led_value >> 31);
		delay(delta_time);
	}
}

void leftToRight(int delta_time, uint32_t led_value){
	for(int i = 0; i < 32; i++){
		*LEDS = led_value;
		led_value = (led_value >> 01) | (led_value << 31);
		delay(delta_time/2);
	}
}

/*-----------------------------------------------------------------\\
						SEVEN-SEGMENT-METHODS
\\-----------------------------------------------------------------*/

#define SEVEN_SEGMENTS ((volatile uint32_t *) 0xFF200020)
	
void printNumber(uint32_t number){
	*SEVEN_SEGMENTS = number;
}

void clearSegments(){
	printNumber(0x40404040);
}
	
char castIntDigitToSegmentDigit(char digit){
	switch(digit){
		case 0:
			return 0x3f;
		case 1:
			return 0x06;
		case 2:
			return 0x5b;
		case 3:
			return 0x4f;
		case 4:
			return 0x66;
		case 5:
			return 0x6d;
		case 6:
			return 0x7d;
		case 7:
			return 0x07;
		case 8:
			return 0x7f;
		case 9:
			return 0x6f;
		default:
			return 0x40;
	}
}

uint32_t castIntToSegmentNumber(uint32_t number){
	uint32_t segment_number = 0;
	int shift = 0;
	do {
        uint32_t digit = number % 10;
        uint32_t segment_digit = (uint32_t)castIntDigitToSegmentDigit(digit);
        segment_number |= (segment_digit << (shift * 8));
        shift++;
        number /= 10;
    } while(number > 0 && shift < 4);
	return segment_number;
}

/*-----------------------------------------------------------------\\
								MAIN
\\-----------------------------------------------------------------*/

#define AVAILABLE_SWITCHES ((uint16_t) 3)

int main(void)
{
	uint32_t delta_time = 200000;
	uint32_t current_switches = 0;
	uint32_t max_switch_value = (uint32_t) (pow(2, AVAILABLE_SWITCHES) - 1);
	uint32_t led_value = 0x00100010;
	uint32_t seven_segment_value = 0;
	uint32_t int_segment_value = 0;
	
	do {
		current_switches = getCurrentSwitches();
		
		if(current_switches > max_switch_value) return 1;
		if((current_switches & 1)){
			switch(getCurrentButtons()){
					case 1:
						rightToLeft(delta_time, led_value);
						int_segment_value++;
						break;
					case 2:
						leftToRight(delta_time, led_value);
						int_segment_value++;
						break;
					default:
						clearLEDS();
						int_segment_value = 0;
				}
		}else{
			clearLEDS();
		}
		
		if(current_switches & 2){
			seven_segment_value = castIntToSegmentNumber(int_segment_value);
			printNumber(seven_segment_value);
		}else{
			clearSegments();
			int_segment_value = 0;
		}
	} while(1);
	
	clearLEDS();
	clearSegments();
	int_segment_value = 0;
    return 0;
}