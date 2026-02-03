/*
 * (c) 2015 flabberast <s3+flabbergast@sdfeu.org>
 *
 * Based on the following work:
 *  - Guillaume Duc's raw hid example (MIT License)
 *    https://github.com/guiduc/usb-hid-chibios-example
 *  - PJRC Teensy examples (MIT License)
 *    https://www.pjrc.com/teensy/usb_keyboard.html
 *  - hasu's TMK keyboard code (GPL v2 and some code Modified BSD)
 *    https://github.com/tmk/tmk_keyboard/
 *  - ChibiOS demo code (Apache 2.0 License)
 *    http://www.chibios.org
 *
 * Since some GPL'd code is used, this work is licensed under
 * GPL v2 or later.
 */


#ifndef _USB_MAIN_H_
#define _USB_MAIN_H_

// TESTING
// extern uint8_t blinkLed;

#include "ch.h"
#include "hal.h"

#include "matrix.h" // include all the customize define by lyon

/* -------------------------
 * General USB driver header
 * -------------------------
 */

/* The USB driver to use */
#define USB_DRIVER USBD2

/* Initialize the USB driver and bus */
void init_usb_driver(USBDriver *usbp);

/* Send remote wakeup packet */
void send_remote_wakeup(USBDriver *usbp);

void restart_usb_driver(USBDriver *usbp);

/* ---------------
 * Keyboard header
 * ---------------
 */

/* main keyboard (6kro) */
#define KBD_INTERFACE   0
#define KBD_ENDPOINT    1
#define KBD_EPSIZE      8
#define KBD_REPORT_KEYS (KBD_EPSIZE - 2)

/* secondary keyboard */
#ifdef NKRO_ENABLE
#define NKRO_INTERFACE    4
#define NKRO_ENDPOINT     5
#define NKRO_EPSIZE       16
#define NKRO_REPORT_KEYS  (NKRO_EPSIZE - 1)
#endif

#ifdef LAMP_ARRAY_ENABLE
/* Lamp Array */
#define LAMP_ARRAY_INTERFACE  5
#define LAMP_ARRAY_ENDPOINT   6
#define LAMP_ARRAY_EPSIZE     64
#endif

#ifdef CONTROL_ENABLE
/* USB Control */
#define CONTROL_INTERFACE  6
#define CONTROL_ENDPOINT   7
#define CONTROL_EPSIZE     64
#endif

/* extern report_keyboard_t keyboard_report_sent; */

/* keyboard IN request callback handler */
void kbd_in_cb(USBDriver *usbp, usbep_t ep);

/* start-of-frame handler */
void kbd_sof_cb(USBDriver *usbp);

#ifdef NKRO_ENABLE
/* nkro IN callback hander */
void nkro_in_cb(USBDriver *usbp, usbep_t ep);
#endif /* NKRO_ENABLE */

#ifdef LAMP_ARRAY_ENABLE
/* nkro IN callback hander */
void lamp_array_in_cb(USBDriver *usbp, usbep_t ep);
#endif /* LAMP_ARRAY_ENABLE */

#ifdef CONTROL_ENABLE
/* controlcallback hander */
void control_in_cb(USBDriver *usbp, usbep_t ep);
#endif /* CONTROL_ENABLE */

/* ------------
 * Mouse header
 * ------------
 */

#ifdef MOUSE_ENABLE

#define MOUSE_INTERFACE         1
#define MOUSE_ENDPOINT          2
#define MOUSE_EPSIZE            8

/* mouse IN request callback handler */
void mouse_in_cb(USBDriver *usbp, usbep_t ep);
#endif /* MOUSE_ENABLE */

/* ---------------
 * Extrakey header
 * ---------------
 */

#ifdef EXTRAKEY_ENABLE

#define EXTRA_INTERFACE         3
#define EXTRA_ENDPOINT          3
#define EXTRA_EPSIZE            32

/* extrakey IN request callback handler */
void extra_in_cb(USBDriver *usbp, usbep_t ep);

/* extra report structure */
typedef struct {
  uint8_t report_id;
  uint16_t usage;
} __attribute__ ((packed)) report_extra_t;
#endif /* EXTRAKEY_ENABLE */

/* --------------
 * Raw header
 * --------------
 */

#ifdef RAW_ENABLE

#define RAW_INTERFACE         2
#define RAW_IN_ENDPOINT       4
#define RAW_OUT_ENDPOINT      4
#define RAW_EPSIZE            32

/* raw IN request callback handler */
void raw_in_cb(USBDriver *usbp, usbep_t ep);

/* raw OUT request callback handler */
void raw_out_cb(USBDriver *usbp, usbep_t ep);


#endif /* RAW_ENABLE */

/* --------------
 * Console header
 * --------------
 */

#ifdef CONSOLE_ENABLE

#define CONSOLE_INTERFACE      2
#define CONSOLE_ENDPOINT       4
#define CONSOLE_EPSIZE         32

/* Number of IN reports that can be stored inside the output queue */
#define CONSOLE_QUEUE_CAPACITY 4

/* Console flush time */
#define CONSOLE_FLUSH_MS 50

/* Putchar over the USB console */
int8_t sendchar(uint8_t c);


/* Flush output (send everything immediately) */
void console_flush_output(void);

/* console IN request callback handler */
void console_in_cb(USBDriver *usbp, usbep_t ep);
#endif /* CONSOLE_ENABLE */

#ifdef reboot
extern volatile bool g_usb_configured_flag;   // ★ 全域旗標（非 static）lyon
#endif

void sendchar_pf(void *p, char c);

#endif /* _USB_MAIN_H_ */
