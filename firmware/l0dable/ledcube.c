#include <sysinit.h>

#include "basic/basic.h"
#include "basic/config.h"
#include "lcd/lcd.h"
#include "lcd/render.h"

#include "usetable.h"

void ram(void) {
	IOCON_PIO2_5  = IOCON_PIO2_5_FUNC_GPIO;
	IOCON_PIO2_4  = IOCON_PIO2_4_FUNC_GPIO;
	IOCON_PIO2_8  = IOCON_PIO2_8_FUNC_GPIO;
	IOCON_PIO3_2  = IOCON_PIO3_2_FUNC_GPIO;
	IOCON_PIO3_1  = IOCON_PIO3_1_FUNC_GPIO;
	IOCON_PIO2_10 = IOCON_PIO2_10_FUNC_GPIO;

	gpioSetDir(2, 5, gpioDirection_Output);
	gpioSetDir(2, 4, gpioDirection_Output);
	gpioSetDir(2, 8, gpioDirection_Output);
	gpioSetDir(2,10, gpioDirection_Output);
	gpioSetDir(3, 2, gpioDirection_Output);
	gpioSetDir(3, 1, gpioDirection_Output);

	int column = 0;
	int layer = 0;

	/*
	 * A6 2_5
	 * A8 2_4
	 * A10 2_8
	 * B6 3_2
	 * B8 3_1
	 * B10 2_10
	 */

	while (1) {
		lcdClear();

		lcdPrint("C: ");
		lcdPrintInt(column);
		lcdNl();
		lcdPrint("L: ");
		lcdPrintInt(layer);
		lcdNl();

		lcdRefresh();

		set_layer(layer);
		set_column(column);

		int key = getInputWaitRepeat();
		if (key == BTN_ENTER) {
			return;
		} else if (key == BTN_LEFT) {
			if (column == 1) {
				column = 9;
			} else {
				column--;
			}
		} else if (key == BTN_RIGHT) {
			if (column == 9) {
				column = 1;
			} else {
				column++;
			}
		} else if (key == BTN_DOWN) {
			if (layer == 0) {
				layer = 3;
			} else {
				layer--;
			}
		} else if (key == BTN_UP) {
			if (layer == 3) {
				layer = 0;
			} else {
				layer++;
			}
		}
	}
}

void pin_layer(int a, int b) {
	gpioSetValue(3, 2, a);
	gpioSetValue(3, 1, b);
}

void set_layer(int layer) {
	if (layer == 1) {
		pin_layer(0, 0);
	} else if (layer == 2) {
		pin_layer(0, 1);
	} else if (layer == 3) {
		pin_layer(1, 0);
	} else {
		pin_layer(1, 1);
	}
}

void pin_column(int a, int b, int c, int d) {
	gpioSetValue(2, 5, a);
	gpioSetValue(2, 4, b);
	gpioSetValue(2, 8, c);
	gpioSetValue(2,10, d);
}

void set_column(int column) {
	if (column == 1) {
		pin_column(0, 1, 1, 1);
	} else if (column == 2) {
		pin_column(0, 0, 0, 0);
	} else if (column == 3) {
		pin_column(1, 0, 0, 1);
	} else if (column == 4) {
		pin_column(1, 1, 1, 1);
	} else if (column == 5) {
		pin_column(0, 0, 1, 1);
	} else if (column == 6) {
		pin_column(0, 1, 0, 0);
	} else if (column == 7) {
		pin_column(1, 1, 1, 0);
	} else if (column == 8) {
		pin_column(1, 1, 0, 1);
	} else if (column == 9) {
		pin_column(1, 0, 1, 1);
	}
}
