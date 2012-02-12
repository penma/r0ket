#include <sysinit.h>

#include "basic/basic.h"
#include "basic/config.h"
#include "lcd/lcd.h"
#include "lcd/render.h"

#include "usetable.h"

#define GPIO_DATA  2,5
#define GPIO_SHIFT 2,4
#define GPIO_STORE 2,8

static void delayus(int);
static void gpioSetValue2(int, int, int);
void shiftreg_set(uint16_t);

void ram(void) {
	IOCON_PIO2_5  = IOCON_PIO2_5_FUNC_GPIO;
	IOCON_PIO2_4  = IOCON_PIO2_4_FUNC_GPIO;
	IOCON_PIO2_8  = IOCON_PIO2_8_FUNC_GPIO;

	gpioSetDir(GPIO_DATA , gpioDirection_Output);
	gpioSetDir(GPIO_SHIFT, gpioDirection_Output);
	gpioSetDir(GPIO_STORE, gpioDirection_Output);

	gpioSetValue2(GPIO_SHIFT, 0);
	gpioSetValue2(GPIO_STORE, 0);

	/*
	 * A6 2_5
	 * A8 2_4
	 * A10 2_8
	 * B6 3_2
	 * B8 3_1
	 * B10 2_10
	 */

	int t = 0;

	int mode = 0;
	int dim = 1;

	while (1) {
		uint16_t v = 0;

		if (mode == 0) {
			t %= 6;
			/* 3x55544433222111 */
			if (t == 0 || t == 1 || t == 5) {
				v |= 0b0000100101001001;
			}
			if (t == 1 || t == 2 || t == 3) {
				v |= 0b0001001010010010;
			}
			if (t == 3 || t == 4 || t == 5) {
				v |= 0b1010010000100100;
			}
		} else if (mode == 1) {
			v = 0b1011111111111111;
		} else if (mode == 2) {
			v = getRandom();
		}
		#define max_mode 2

		t++;

		if (dim) {
			for (int i = 0; i < 500; i++) {
				shiftreg_set(v & ~(0b0001001010010010));
				delayus(3);
				shiftreg_set(v);
				delayus(2);
				shiftreg_set(0);
				delayus(500);
			}
		} else {
			shiftreg_set(v);
			delayus(250000);
		}

		int key = getInputRaw();
		if (key != BTN_NONE) {
			if (key == BTN_ENTER) {
				return;
			} else if (key == BTN_LEFT) {
				if (mode == 0) {
					mode = max_mode;
				} else {
					mode--;
				}
			} else if (key == BTN_RIGHT) {
				if (mode == max_mode) {
					mode = 0;
				} else {
					mode++;
				}
			} else if (key == BTN_DOWN) {
				dim = !dim;
			}

			getInputWaitRelease();
		}
	}
}

static void delayus(int us) {
	/*
	for (long int i = 72 * us; i != 0; i--) {
		__asm volatile ("nop");
	}
	*/

	/* 72 MHz CPU, 1us = 72cycles -> 3*24. appears to work o.o */
	__asm volatile ("0: subs %0, #1 \n bne 0b" : : "r" (24 * us));
}

static void gpioSetValue2(int port /* assume 2 */, int pin, int val) {
	REG32 *gpiodata = &GPIO_GPIO2DATA;
	if (val) {
		*(pREG32 (0x50023FFC)) |=  (1 << pin);
	} else {
		*(pREG32 (0x50023FFC)) &= ~(1 << pin);
	}
}

void shiftreg_set(uint16_t data) {
	for (int i = 0; i < 16; i++) {
		gpioSetValue2(GPIO_DATA , data & 1);
		data >>= 1;
		gpioSetValue2(GPIO_SHIFT, 1);
		gpioSetValue2(GPIO_SHIFT, 0);
	}

	gpioSetValue2(GPIO_STORE, 1);
	gpioSetValue2(GPIO_STORE, 0);
}

