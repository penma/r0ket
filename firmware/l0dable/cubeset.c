#include <sysinit.h>

#include "basic/basic.h"
#include "basic/config.h"
#include "lcd/lcd.h"
#include "lcd/render.h"
#include "core/timer32/timer32.h"

#include "usetable.h"

#define GPIO_DATA  2,5
#define GPIO_SHIFT 2,4
#define GPIO_STORE 2,8

#define LAYERS 3
#define MENU_AUTOUPDATE LAYERS /* layers = 0..n-1, autoupdate = n */
#define MENU_MAX 3
static int menu_sel = 0;
#define layer_sel (LAYERS - menu_sel)
static int in_menu = 1;

static int auto_update = 1;

int led_x = 0;
int led_y = 0;

         uint32_t cube_config  = 0;
volatile uint32_t cube_display = 0;
volatile int plex_layer = 0;

void cube_plex();

void ram(void) {
	IOCON_PIO2_5  = IOCON_PIO2_5_FUNC_GPIO;
	IOCON_PIO2_4  = IOCON_PIO2_4_FUNC_GPIO;
	IOCON_PIO2_8  = IOCON_PIO2_8_FUNC_GPIO;

	gpioSetDir(GPIO_DATA , gpioDirection_Output);
	gpioSetDir(GPIO_SHIFT, gpioDirection_Output);
	gpioSetDir(GPIO_STORE, gpioDirection_Output);

	setup_timer();

	/*
	 * A6 2_5
	 * A8 2_4
	 * A10 2_8
	 * B6 3_2
	 * B8 3_1
	 * B10 2_10
	 */

	while (1) {
		render_interface();

		int key = getInputWaitRepeat();

		if (in_menu) {
			if (key == BTN_ENTER) {
				unsetup_timer();
				return;
			} else if (key == BTN_UP) {
				if (menu_sel == 0) {
					menu_sel = MENU_MAX;
				} else {
					menu_sel--;
				}
			} else if (key == BTN_DOWN) {
				if (menu_sel == MENU_MAX) {
					menu_sel = 0;
				} else {
					menu_sel++;
				}
			} else if (key == BTN_RIGHT) {
				if (menu_sel < LAYERS) {
					in_menu = 0;
					led_x = 0;
					led_y = layer_sel - 1;
				} else if (menu_sel == MENU_AUTOUPDATE) {
					auto_update = !auto_update;
				}
			}
		} else {
			if (key == BTN_UP) {
				if (led_y == 0) {
					led_y = 2;
				} else {
					led_y--;
				}
			} else if (key == BTN_DOWN) {
				if (led_y == 2) {
					led_y = 0;
				} else {
					led_y++;
				}
			} else if (key == BTN_LEFT) {
				if (led_x == 0) {
					in_menu = 1;
				} else {
					led_x--;
				}
			} else if (key == BTN_RIGHT) {
				if (led_x == 2) {
					in_menu = 1;
				} else {
					led_x++;
				}
			} else if (key == BTN_ENTER) {
				/* toggle */
				cube_config ^= 1 << cube_bit(led_x, led_y, layer_sel);
			}
		}

		/* potentially update stuff... */
		if (auto_update) {
			cube_display = cube_config;
		}
	}
}

void render_interface() {
	lcdClear();

	/* menu */

	for (int layer = 1; layer <= LAYERS; layer++) {
		int dy = 10 + 15 * (LAYERS - layer);
		lcdSetCrsr(7, dy - 3);
		lcdPrintInt(layer);

		if (menu_sel == (LAYERS - layer)) {
			if (!in_menu) {
				render_border(5, dy - 6, 16, dy + 6);
				render_border(16, 2, 65, 50);
			} else {
				render_border(5, dy - 6, 14, dy + 6);
			}
		}
	}

	/* auto-update menu entry */
	lcdSetCrsr(7, 52);
	if (auto_update) {
		lcdPrint("A");
	} else {
		lcdPrint("a");
	}

	if (menu_sel == MENU_AUTOUPDATE) {
		render_border(5, 55 - 6, 14, 55 + 6);
	}

	/* border the menu if it is selected */
	if (in_menu) {
		render_border(1, 2, 17, 63);
	}

	/* cube config */

	for (int y = 0; y < 3; y++) {
		for (int x = 0; x < 3; x++) {
			int dx = 25 + x * 15;
			int dy = 10 + y * 15;

			/* quasi-circle for the border of the led */
			render_line_hor(dx - 1, dx + 1, dy - 2);
			render_line_hor(dx - 1, dx + 1, dy + 2);
			render_line_ver(dx - 2, dy - 1, dy + 1);
			render_line_ver(dx + 2, dy - 1, dy + 1);

			/* fill it, if active */
			if (cube_config & (1 << cube_bit(x, y, layer_sel))) {
				render_line_hor(dx - 1, dx + 1, dy - 1);
				render_line_hor(dx - 1, dx + 1, dy);
				render_line_hor(dx - 1, dx + 1, dy + 1);
			}

			/* border it, if selected */
			if (!in_menu && x == led_x && y == led_y) {
				render_border(dx - 4, dy - 4, dx + 4, dy + 4);
			}
		}
	}

	lcdRefresh();
}

void render_line_hor(int x1, int x2, int y) {
	for (int x = x1; x <= x2; x++) {
		lcdSetPixel(x, y, 1);
	}
}

void render_line_ver(int x, int y1, int y2) {
	for (int y = y1; y <= y2; y++) {
		lcdSetPixel(x, y, 1);
	}
}

void render_border(int x1, int y1, int x2, int y2) {
	render_line_hor(x1, x2, y1);
	render_line_hor(x1, x2, y2);
	render_line_ver(x1, y1, y2);
	render_line_ver(x2, y1, y2);
}

int cube_bit(int x, int y, int z) {
	return x + (y * 3) + ((z-1) * 9);
}

void setup_timer() {
	timer32Callback0 = cube_plex;

	SCB_SYSAHBCLKCTRL |= (SCB_SYSAHBCLKCTRL_CT32B0);
	TMR_TMR32B0MR0  = 72E6/2000;
	TMR_TMR32B0MCR = (TMR_TMR32B0MCR_MR0_INT_ENABLED | TMR_TMR32B0MCR_MR0_RESET_ENABLED);
	NVIC_EnableIRQ(TIMER_32_0_IRQn);
	TMR_TMR32B0TC = 0;
	TMR_TMR32B0TCR = TMR_TMR32B0TCR_COUNTERENABLE_ENABLED;
}

void unsetup_timer() {
	NVIC_DisableIRQ(TIMER_32_0_IRQn);
	TMR_TMR32B0TCR = TMR_TMR32B0TCR_COUNTERENABLE_DISABLED;
}

void cube_plex() {
	plex_layer++;
	if (plex_layer == LAYERS) {
		plex_layer = 0;
	}

	uint16_t sreg = 0b0011100000000000; /* all three ground layers high */

	/* select layer */
	uint16_t layer_data = (cube_display >> (9 * plex_layer)) & 0b111111111;

	if (plex_layer == 0) {
		sreg ^= 1 << 11;
	} else if (plex_layer == 1) {
		sreg ^= 1 << 13;
	} else if (plex_layer == 2) {
		sreg ^= 1 << 12;
	}

	/* apply layer data */
	sreg |= ((layer_data >> 0) & 1) << 10;
	sreg |= ((layer_data >> 1) & 1) <<  2;
	sreg |= ((layer_data >> 2) & 1) << 14;
	sreg |= ((layer_data >> 3) & 1) <<  8;
	sreg |= ((layer_data >> 4) & 1) <<  6;
	sreg |= ((layer_data >> 5) & 1) <<  4;
	sreg |= ((layer_data >> 6) & 1) <<  9;
	sreg |= ((layer_data >> 7) & 1) <<  5;
	sreg |= ((layer_data >> 8) & 1) <<  3;

	shiftreg_set(sreg);
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

