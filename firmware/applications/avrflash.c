#include <sysinit.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <stdint.h>
#include "basic/basic.h"
#include "lcd/render.h"
#include "basic/config.h"
#include "basic/byteorder.h"
#include "lcd/lcd.h"
#include "lcd/print.h"
#include "usbcdc/usb.h"
#include "usbcdc/usbcore.h"
#include "usbcdc/usbhw.h"
#include "usbcdc/cdcuser.h"
#include "usbcdc/cdc_buf.h"
#include "usbcdc/util.h"

#if CFG_USBMSC
#error "MSC is defined"
#endif

#if !CFG_USBCDC
#error "CDC is not defined"
#endif

#define RP_CMD_IDENTIFY '?'
#define RP_ID_0         '<'
#define RP_ID_1         '3'

#define RP_CMD_ENABLE   'e'
#define RP_CMD_DISABLE  'd'

#define RP_CMD_AVR      'C'
#define RP_RES_AVR      'R'

#define AVR_RESET 2,5 /* A6 */
#define AVR_SCK   2,4 /* A8 */
#define AVR_MOSI  3,2 /* B6 */
#define AVR_MISO  3,1 /* B8 */

static uint8_t ser_rx() {
	uint8_t r;

	int len = 0;
	while (len == 0) {
		CDC_OutBufAvailChar(&len);
	}

	len = 1;
	CDC_RdOutBuf(&r, &len);
	return r;
}

static void ser_tx(uint8_t d) {
	int len = 1;
	CDC_WrInBuf(&d, &len);
}

uint8_t avr_txrx(uint8_t out) {
	uint8_t buf = 0;

	for (int i = 8; i > 0; i--) {
		gpioSetValue(AVR_MOSI, out & (1 << (i - 1)) ? 1 : 0);
		gpioSetValue(AVR_SCK, 1);

		for (int i = 0; i < 10; i++) {
			__asm volatile ("nop");
		}

		buf |= gpioGetValue(AVR_MISO) << (i - 1);
		gpioSetValue(AVR_SCK, 0);

		for (int i = 0; i < 10; i++) {
			__asm volatile ("nop");
		}

	}

	return buf;
}

static void avr_cmd(uint8_t cmd[4], uint8_t ret[4]) {
	uint8_t dummy[4];
	if (ret == NULL) {
		ret = dummy;
	}

	for (int i = 0; i < 4; i++) {
		ret[i] = avr_txrx(cmd[i]);
	}
}

static void avr_sync(void) {
	uint8_t r[4] = { 0, 0, 0, 1};
	while (r[3] & 1) {
		r[0] = 0xf0;
		r[1] = r[2] = r[3] = 0;
		avr_cmd(r, r);
	}
}

static void avr_reset(int x) {
	gpioSetValue(AVR_RESET, x);
	delayms(25);
}

void avr_setup(void) {
	gpioSetDir(AVR_RESET, gpioDirection_Output);
	avr_reset(1);

	gpioSetDir(AVR_MISO, gpioDirection_Input);
	gpioSetDir(AVR_SCK, gpioDirection_Output);
	gpioSetDir(AVR_MOSI, gpioDirection_Output);
}

static void led_wip(int i) {
	gpioSetValue(RB_LED0, i);
}

static void led_rdy(int i) {
	gpioSetValue(RB_LED2, i);
}

void main_avrflash(void) {
	GLOBAL(positionleds) = 0;
	led_wip(0);
	led_rdy(0);

	usbCDCInit();
	delayms(500);

	avr_setup();

	while (1) {
		uint8_t input = ser_rx();

		if (input == RP_CMD_IDENTIFY) {
			ser_tx(RP_ID_0);
			ser_tx(RP_ID_1);
		} else if (input == RP_CMD_ENABLE) {
			led_wip(1);

			int failed = 0;
			int dev_ok = 0;
			while (failed < 5) {
				uint8_t buf[4] = { 0xac, 0x53, 0, 0 };
				avr_cmd(buf, buf);
				if (buf[2] == 0x53) {
					dev_ok = 1;
					break;
				}
				else {
					failed++;
					/* repower */
					avr_reset(1);
					delayms(100);
					avr_reset(0);
					delayms(400);
				}
			}

			if (dev_ok) {
				ser_tx(0);
				led_wip(0);
				led_rdy(1);
			} else {
				ser_tx(0xff);
			}
		} else if (input == RP_CMD_DISABLE) {
			led_wip(1);
			avr_reset(1);
			led_wip(0);
			led_rdy(0);

			ser_tx(0);
		} else if (input == RP_CMD_AVR) {
			uint8_t buf[4];
			for (int i = 0; i < 4; i++) {
				buf[i] = ser_rx();
			}

			led_wip(1);
			avr_cmd(buf, buf);
			led_wip(0);

			ser_tx(RP_RES_AVR);
			for (int i = 0; i < 4; i++) {
				ser_tx(buf[i]);
			}

			avr_sync();
		} else {
			lcdPrint("cmd? ");
			lcdPrintCharHex(input);
			lcdNl();
			lcdRefresh();
		}
	}
}

