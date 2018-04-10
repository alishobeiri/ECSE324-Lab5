
#include "./drivers/inc/vga.h"
#include "./drivers/inc/ISRs.h"
#include "./drivers/inc/LEDs.h"
#include "./drivers/inc/audio.h"
#include "./drivers/inc/HPS_TIM.h"
#include "./drivers/inc/int_setup.h"
#include "./drivers/inc/wavetable.h"
#include "./drivers/inc/pushbuttons.h"
#include "./drivers/inc/ps2_keyboard.h"
#include "./drivers/inc/HEX_displays.h"
#include "./drivers/inc/slider_switches.h"
int time = 0;
// Order of frequency:    A        S        D        F        J        K        L        ;
double frequencies[8] = {130.813, 146.832, 164.814, 174.614, 195.998, 220.000, 246.942, 261.626};
int keyPressed[8] = {0};

int volume = 1;

// Method headers
int audio_write_both_ASM(int audiodata);
int getSignal(double frequency, double time);
void setupTimer();
int makeSound();
void readKeyboard();

int main() {
	setupTimer();

	VGA_clear_charbuff_ASM();
	VGA_clear_pixelbuff_ASM();

	int xVal = -1, yVal = 0;
	int lastPixelYAtDispX[320];

	while(1) {
		readKeyboard();
		int signalValue = makeSound();

		// We draw a point on the screen every 120 us
		if(time % 8 == 0){ 

			// xVal holds the x location of the pixel we want to draw, max x value is 319
			xVal = (xVal + 1) % 320;
			

			VGA_draw_point_ASM(xVal, lastPixelYAtDispX[xVal], 0x00);
			
			//The y value must have an offset of 240/2 = 120, and scaling down arbitrary by 2^18 to fits on screen.
			yVal = 120 - (signalValue >> 20);	
			//insert the New pixel
			VGA_draw_point_ASM(xVal, yVal, xVal*5); 
			
			lastPixelYAtDispX[xVal] = yVal;
			//writeVolume(volume);
		}		
	}

	return 0;
}


int makeSound() {
	int signal = 0;
	int i;
	
	for (i = 0; i < 8; i++) {
		if(keyPressed[i]) {
			signal += getSignal(frequencies[i], time);
		}
	}

	if (hps_tim0_int_flag) {
		hps_tim0_int_flag = 0;

		while(audio_write_data_ASM(signal, signal) == 0) {}

		time += 1;
		if (time == 48000) {
			// time = 0;
		}
	}
			
	return signal;
}


void readKeyboard() {
	char key;
	if(read_ps2_data_ASM(&key)) {
		int val = 1;
		if (key == 0xF0) {
			while(!read_ps2_data_ASM(&key));
			val = 0;
		}
		if (key == 0x1C) { // A
			keyPressed[0] = val;
		} 
		else if (key == 0x1B) { // S
			keyPressed[1] = val;
		} 
		else if (key == 0x23) { // D
			keyPressed[2] = val;
		} 
		else if (key == 0x2B) { // F
			keyPressed[3] = val;
		} 
		else if (key == 0x3B) { // J
			keyPressed[4] = val;
		} 
		else if (key == 0x42) { // K
			keyPressed[5] = val;
		} 
		else if (key == 0x4B) { // L
			keyPressed[6] = val;
		} 
		else if (key == 0x4C) { // ;
			keyPressed[7] = val;
		} 
		else if (key == 0x55 && val) { // Volume down '-' key
			if(volume<32) volume += 1;
		} 
		else if (key == 0x4E && val) { // Volume up '+' key
			if (volume != 0) volume -= 1;
		} 
	}
}


int getSignal(double frequency, double time) {
	double product = (frequency * time);
	double diff = (product - (int)product);
	int index = (int)(product) % 48000;
	int sound;
	
	if (diff == 0) {
		sound = sine[index];
	} else {
		sound = (1 - diff)*sine[(int)(index)] + diff*sine[(int)(index + 1)]; 
	}

	return volume * sound;
}


void setupTimer() {
	// Sampling timer
	int_setup(2, (int[]){199});

	HPS_TIM_config_t hps_time0;

	hps_time0.tim = TIM0;
	hps_time0.timeout = 20;
	hps_time0.LD_en = 1;
	hps_time0.INT_en = 1;
	hps_time0.enable = 1;
	HPS_TIM_config_ASM(&hps_time0);
}

