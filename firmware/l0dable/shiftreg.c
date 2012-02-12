#include <sysinit.h>

#include "basic/basic.h"
#include "basic/config.h"
#include "lcd/lcd.h"
#include "lcd/render.h"

#include "usetable.h"

#define GPIO_DATA  2,5
#define GPIO_SHIFT 2,4
#define GPIO_STORE 2,8

void ram(void) {
	IOCON_PIO2_5  = IOCON_PIO2_5_FUNC_GPIO;
	IOCON_PIO2_4  = IOCON_PIO2_4_FUNC_GPIO;
	IOCON_PIO2_8  = IOCON_PIO2_8_FUNC_GPIO;

	gpioSetDir(GPIO_DATA , gpioDirection_Output);
	gpioSetDir(GPIO_SHIFT, gpioDirection_Output);
	gpioSetDir(GPIO_STORE, gpioDirection_Output);

	/*
	 * A6 2_5
	 * A8 2_4
	 * A10 2_8
	 * B6 3_2
	 * B8 3_1
	 * B10 2_10
	 */

	int pos = 0;
	uint16_t state = 0;

	while (1) {
		lcdClear();

		for (int y = 0; y < 2; y++) {
			for (int x = 0; x < 8; x++) {
				int i = 8 * y + x;
				lcdSetCrsr(10 * x, 5 + y * 20);

				if (i < 10) {
					lcdPrintInt(i);
				} else {
					char foo[2] = { 'a' + i - 10, 0 };
					lcdPrint(foo);
				}

				if (pos == i) {
					for (int xx = 0; xx <= 7; xx++) {
						lcdSetPixel(10 * x + xx, 13 + y * 20, 1);
					}
				}

				lcdSetCrsr(10 * x, 15 + y * 20);
				lcdPrintInt((state >> i) & 1);
			}
		}

		lcdRefresh();

		int key = getInputWaitRepeat();
		if (key == BTN_ENTER) {
			return;
		} else if (key == BTN_LEFT) {
			if (pos == 0) {
				pos = 15;
			} else {
				pos--;
			}
		} else if (key == BTN_RIGHT) {
			if (pos == 15) {
				pos = 0;
			} else {
				pos++;
			}
		} else if (key == BTN_DOWN) {
			state ^= 1 << pos;
		} else if (key == BTN_UP) {
			shiftreg_set(state);
		}
	}
}

void shiftreg_set(uint16_t data) {
	gpioSetValue(GPIO_SHIFT, 0);
	gpioSetValue(GPIO_STORE, 0);

	for (int i = 0; i < 16; i++) {
		gpioSetValue(GPIO_DATA , (data >> i) & 1);
		gpioSetValue(GPIO_SHIFT, 1);
		gpioSetValue(GPIO_SHIFT, 0);
	}

	gpioSetValue(GPIO_STORE, 1);
	gpioSetValue(GPIO_STORE, 0);
}

