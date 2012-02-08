#include <sysinit.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <stdint.h>
#include "basic/basic.h"
#include "lcd/render.h"
#include "lcd/allfonts.h"
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

#define Sync_CRC_EOP 0x20

#define Cmnd_STK_GET_SYNC 0x30
#define Cmnd_STK_GET_PARAMETER 0x41
#define Cmnd_STK_SET_DEVICE 0x42
#define Cmnd_STK_SET_DEVICE_EXT 0x45
#define Cmnd_STK_ENTER_PROGMODE 0x50
#define Cmnd_STK_LEAVE_PROGMODE 0x51
#define Cmnd_STK_LOAD_ADDRESS 0x55
#define Cmnd_STK_UNIVERSAL 0x56

#define Cmnd_STK_PROG_PAGE 0x64

#define Resp_STK_INSYNC 0x14
#define Resp_STK_OK 0x10
#define Resp_STK_FAILED 0x11
#define Resp_STK_NODEVICE 0x13

#define Parm_STK_HW_VER            0x80
#define Parm_STK_SW_MAJOR          0x81
#define Parm_STK_SW_MINOR          0x82
#define Parm_STK_LEDS              0x83
#define Parm_STK_VTARGET           0x84
#define Parm_STK_VADJUST           0x85

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
	static int do_ask = 0;
	if (cmd[0] == 0x40) { do_ask = 1; }

	uint8_t dummy[4];
	if (ret == NULL) {
		ret = dummy;
	}

	for (int i = 0; i < 4; i++) {
		lcdPrint(i == 0 ? "> " : " ");
		lcdPrint(IntToStrX(cmd[i], 2));
		ret[i] = avr_txrx(cmd[i]);
	}
	lcdNl();
	for (int i = 0; i < 4; i++) {
		lcdPrint(i == 0 ? "< " : " ");
		lcdPrint(IntToStrX(ret[i], 2));
	}
	lcdNl();
	lcdRefresh();
	if(do_ask) { getInputWait();getInputWaitRelease(); } /* dbg inp */
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

void main_avrflash(void) {
	usbCDCInit();
	delayms(500);

	avr_setup();

	uint16_t addr = 0;

	while (1) {
		uint8_t input = ser_rx();

		if (input == Cmnd_STK_GET_SYNC) {
			ser_rx(); /* should be 0x20 */
			ser_tx(Resp_STK_INSYNC);
			ser_tx(Resp_STK_OK);
		} else if (input == Cmnd_STK_GET_PARAMETER) {
			input = ser_rx();
			ser_rx(); /* should be 0x20 */

			ser_tx(Resp_STK_INSYNC);

			if (input == Parm_STK_HW_VER) {
				ser_tx(1);
				ser_tx(Resp_STK_OK);
			} else if (input == Parm_STK_SW_MAJOR) {
				ser_tx(0);
				ser_tx(Resp_STK_OK);
			} else if (input == Parm_STK_SW_MINOR) {
				ser_tx(1);
				ser_tx(Resp_STK_OK);
			} else {
				ser_tx(0xff);
				ser_tx(Resp_STK_FAILED);
			}
		} else if (input == Cmnd_STK_SET_DEVICE) {
			while (ser_rx() != Sync_CRC_EOP) {
			}

			ser_tx(Resp_STK_INSYNC);
			ser_tx(Resp_STK_OK);
		} else if (input == Cmnd_STK_SET_DEVICE_EXT) {
			while (ser_rx() != Sync_CRC_EOP) {
			}

			ser_tx(Resp_STK_INSYNC);
			ser_tx(Resp_STK_OK);
		} else if (input == Cmnd_STK_ENTER_PROGMODE) {
			ser_rx(); /* should be 0x20 */
			ser_tx(Resp_STK_INSYNC);

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
				ser_tx(Resp_STK_OK);
			} else {
				ser_tx(Resp_STK_NODEVICE);
			}
		} else if (input == Cmnd_STK_LEAVE_PROGMODE) {
			ser_rx(); /* should be 0x20 */
			ser_tx(Resp_STK_INSYNC);

			avr_reset(1);
			lcdPrintln("done!");
			lcdRefresh();

			ser_tx(Resp_STK_OK); /* hm... */
		} else if (input == Cmnd_STK_LOAD_ADDRESS) {
			addr = 0;
			addr |= ser_rx();
			addr |= ser_rx() << 8;
			ser_rx(); /* should be 0x20 */

			lcdPrint("ldaddr ");
			lcdPrintShortHex(addr);
			lcdNl();
			lcdRefresh();

			ser_tx(Resp_STK_INSYNC);
			ser_tx(Resp_STK_OK);
		} else if (input == Cmnd_STK_UNIVERSAL) {
			uint8_t buf[4];
			for (int i = 0; i < 4; i++) {
				buf[i] = ser_rx();
			}
			ser_rx(); /* should be 0x20 */
			ser_tx(Resp_STK_INSYNC);

			avr_cmd(buf, buf);
			ser_tx(buf[3]);

			avr_sync(); /* hm? */

			ser_tx(Resp_STK_OK); /* hm... */
		} else if (input == Cmnd_STK_PROG_PAGE) {
			uint16_t blocksize = 0;
			blocksize |= ser_rx() << 8;
			blocksize |= ser_rx();
			uint8_t memtype = ser_rx(); /* CHECK ME!! */

			uint8_t cmd[4];

			for (int i = 0; i < blocksize; i += 2) {
				uint8_t buf1 = ser_rx();
				uint8_t buf2 = ser_rx();

				lcdPrint("F:");
				lcdPrintShortHex(addr);
				lcdPrint("+");
				lcdPrintCharHex(i);
				lcdNl();
				lcdRefresh();

				cmd[0] = 0x40;
				cmd[1] = 0;
				cmd[2] = (addr + (i >> 1)) & 0xff;
				cmd[3] = buf1;
				avr_cmd(cmd, NULL);
				avr_sync();

				cmd[0] = 0x48;
				cmd[3] = buf2;
				avr_cmd(cmd, NULL);
				avr_sync();
			}

			lcdPrint("flash: ");
			lcdPrintShortHex(addr);
			lcdNl();
			lcdRefresh();

			cmd[0] = 0x4c;
			cmd[1] = (addr >> 8) & 0xff;
			cmd[2] = addr & 0xff;
			cmd[3] = 0;
			avr_cmd(cmd, NULL);
			avr_sync();

			ser_rx(); /* should be 0x20 */



			ser_tx(Resp_STK_INSYNC);
			ser_tx(Resp_STK_OK); /* hm... */
		} else {
/*			lcdPrint("cmd? ");
			lcdPrintCharHex(input);
			lcdNl();
			lcdRefresh(); */
		}
	}
}

