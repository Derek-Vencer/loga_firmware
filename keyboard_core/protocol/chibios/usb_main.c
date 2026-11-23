#include "ch.h"
#include "hal.h"

#ifndef USB_SUSPEND_WAKEUP_DELAY
#    define USB_SUSPEND_WAKEUP_DELAY 0U
#endif

#include "usb_main.h"
#include <string.h>
#include "host.h"
#include "debug.h"
#include "suspend.h"
#ifdef SLEEP_LED_ENABLE
#include "sleep_led.h"
#include "led.h"
#endif
#include "hook.h"
#include "keycode.h"
#include "matrix.h"
extern int uprintf(const char *fmt, ...);

#define HID_COMMUNICATION_INTERFACE_CLASS 0x03U
#define VENDOR_COMMUNICATION_INTERFACE_CLASS 0x0ffU
#define CDC_DATA_INTERFACE_CLASS 0x0AU

#define IAD_HID_IF_DESC_SET(first, count, class) \
    /* Interface Association Descriptor.*/       \
    USB_DESC_INTERFACE_ASSOCIATION(              \
        first, /* bFirstInterface.         */    \
        count, /* bInterfaceCount.         */    \
        class, /* bFunctionClass.          */    \
        0,     /* bFunctionSubClass.       */    \
        0,     /* bFunctionProcotol.       */    \
        0      /* iInterface.              */    \
        ),
#ifdef reboot
volatile bool g_usb_configured_flag = false;   // ★ 全域旗標（非 static）lyon
#endif
extern bool autonomousMode;
extern mutex_t analogStickMutex;
extern led_runMode_t currentLED_RunMode;
// extern binary_semaphore_t gamepadAnalogSem,gamepadAnalogSemWait;
#ifdef LAMP_ARRAY_ENABLE
uint8_t dynamaticReciveBuff[LAMP_ARRAY_EPSIZE];
#endif
#ifdef CONTROL_ENABLE
uint8_t controlReciveBuff[CONTROL_EPSIZE];
#endif
// keyboard
//const uint16_t lampCount = 0x51; // 81 Lamps
//const uint32_t lampBoxWidth = 0x00055730;
//const uint32_t lampBoxHeight = 0x000252ec;
//const uint32_t lampBoxDepth = 0x00009290;

const uint16_t lampCount = 119; // 0x51 Lamps
const uint32_t lampBoxWidth =  340000;		//0x50910
const uint32_t lampBoxHeight = 140000;		//0x249F0
const uint32_t lampBoxDepth =   35000;


const uint32_t lampKind = 0x01;                // 0x01:Keyboard 0x02:Mouse
const uint32_t lampMinUpdateInterval = 0x2710; // 0x001F4 5ms //0x1388 5ms	//0x2710	10ms //0x08235; 33,333	33ms	 //0x0186a0	100ms

typedef enum {
    USB_POLL_8000 = 0,
    USB_POLL_4000,
    USB_POLL_2000,
    USB_POLL_1000
} usb_poll_mode_t;

static usb_poll_mode_t g_usb_poll_mode = USB_POLL_8000;

// const uint32_t lampPostionX[] = {
//      3000, 4280, 5560, 6840, 8120, 9400,10680,11960,13240,14520,15800,17080,18360,19640,20920,22200,23480,24760,26040,27320,28600,29880,31160,32440,33720,35000,
//      3000, 5133, 7266, 9399,11532,13665,15798,17931,20064,22197,24330,26463,28596,30729,32862,34995,
//      3000, 5000, 7000, 9000,11000,13000,15000,17000,19000,21000,23000,25000,27000,29000,31000,33000,35000,
//      3000, 5000, 7000, 9000,11000,13000,15000,17000,19000,21000,23000,25000,27000,29000,31000,33000,35000,
//      3000, 5133, 7266, 9399,11532,13665,15798,17931,20064,22197,24330,26463,28596,30729,32862,34995,
//      3000, 5285, 7570, 9855,12140,14425,16710,18995,21280,23565,25850,28135,30420,32705,34990,
//      3000, 5909, 8818,11727,14636,17545,20454,23363,26272,29181,29181,32090,
//     };	

const uint32_t lampPostionX[] = {
     8560, 12560, 15060, 25060, 35060, 45060, 59560, 74260, 88960,103660,108360,133360,148330,163360,178360,193360,208360,223360,238360,253360,268360,283360,296290,310680,316580,320580,
     8560, 17550, 41360, 60410, 79460, 98510,122320,141370,160420,179470,203280,222330,241380,260430,284240,320580,
     8560, 17550, 36600, 55650, 74700, 93750,112800,131850,150900,169950,189530,208050,227100,246150,274700,311670,320580,
     6560, 22300, 46100, 65150, 84200,103250,122300,141350,160400,149450,198500,217550,236600,255650,279450,311670,332580,
     6560, 24670, 50870, 69920, 88970,108020,127070,146120,165170,184220,203270,222320,241370,272300,311670,332580,
     5560, 29420, 60350, 79400, 98450,117500,136550,115600,174650,193700,212750,231800,258000,282670,332580,
     3970, 19720, 43470, 67220,138470,207350,226400,245450,269620,282670,307720,325170,

   };	// 10

// const uint32_t lampPostionY[] = {
//      2000, 2000, 2000, 2000, 2000, 2000, 2000, 2000, 2000, 2000, 2000, 2000, 2000, 2000, 2000, 2000, 2000, 2000, 2000, 2000, 2000, 2000, 2000, 2000, 2000, 2000,
//      3750, 3750, 3750, 3750, 3750, 3750, 3750, 3750, 3750, 3750, 3750, 3750, 3750, 3750, 3750, 3750,
//      5500, 5500, 5500, 5500, 5500, 5500, 5500, 5500, 5500, 5500, 5500, 5500, 5500, 5500, 5500, 5500, 5500,
//      7250, 7250, 7250, 7250, 7250, 7250, 7250, 7250, 7250, 7250, 7250, 7250, 7250, 7250, 7250, 7250, 7250,
//      9000, 9000, 9000, 9000, 9000, 9000, 9000, 9000, 9000, 9000, 9000, 9000, 9000, 9000, 9000, 9000,
//     10750,10750,10750,10750,10750,10750,10750,10750,10750,10750,10750,10750,10750,10750,10750,
//     12500,12500,12500,12500,12500,12500,12500,12500,12500,12500,12500,12500,
//    };

const uint32_t lampPostionY[] = {
    33650, 33650, 7600, 7600, 7600, 7600, 7600, 7600, 7600, 7600, 7600, 7600, 7600, 7600, 7600, 7600, 7600, 7600, 7600, 7600, 7600, 7600, 7600, 7600, 33650, 33650,
    40550, 20340, 20340, 20340, 20340, 20340, 20340, 20340, 20340, 20340, 20340, 20340, 20340, 20340, 20340, 40550,
    51050, 43850, 43850, 43850, 43850, 43850, 43850, 43850, 43850, 43850, 43850, 43850, 43850, 43850, 43850, 43850, 51050,
    61550, 62840, 62840, 62840, 62840, 62840, 62840, 62840, 62840, 62840, 62840, 62840, 62840, 62840, 62840, 62840, 61550,
    72050, 81840, 81840, 81840, 81840, 81840, 81840, 81840, 81840, 81840, 81840, 81840, 81840, 81840, 81840, 72050,
    82550,100840,100840,100840,100840,100840,100840,100840,100840,100840,100840,100840,100840,100840, 82550,
    93050,119840,119840,119840,119840,119840,119840,119840,119840,119840,119840, 93050,
  };

; // 10

const uint32_t lampPostionZ[] = {
     14900,14900,14900,14900,14900,14900,14900,14900,14900,14900,14900,14900,14900,14900,14900,14900,14900,14900,14900,14900,14900,14900,14900,14900,14900,14900,
     14900,11300,11300,11300,11300,11300,11300,11300,11300,11300,11300,11300,11300,11300,11300,14900,
     14900,11300,11300,11300,11300,11300,11300,11300,11300,11300,11300,11300,11300,11300,11300,11300,14900,
     14900,11300,11300,11300,11300,11300,11300,11300,11300,11300,11300,11300,11300,11300,11300,11300,14900,
     14900,11300,11300,11300,11300,11300,11300,11300,11300,11300,11300,11300,11300,11300,11300,14900,
     14900,11300,11300,11300,11300,11300,11300,11300,11300,11300,11300,11300,11300,11300,14900,
     14900,11300,11300,11300,11300,11300,11300,11300,11300,11300,11300,14900,
   };


const uint32_t lampPurpose[] = {
    0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01,
    0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 
    0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01,
    0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01,
    0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 
    0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01,
    0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01,
};

const uint32_t lampUpdateLatency = 10000; //
const uint8_t lampRedLevelCount = 0xff;
const uint8_t lampGreenLevelCount = 0xff;
const uint8_t lampBlueLevelCount = 0xff;
const uint8_t lampIntensityLevelCount = 0xff;
const uint8_t lampIsProgrammable = 0x64;

// const uint8_t lampInputBinding[] = {
//     KC_NO, KC_NO, KC_NO, KC_NO, KC_NO, KC_NO, KC_NO, KC_NO, KC_NO, KC_NO, KC_NO, KC_NO, KC_NO, KC_NO, KC_NO, KC_NO, KC_NO, KC_NO, KC_NO, KC_NO, KC_NO, KC_NO, KC_NO, KC_NO, KC_NO, KC_NO, 
//     KC_ESC, KC_F1, KC_F2, KC_F3, KC_F4, KC_F5, KC_F6, KC_F7, KC_F8, KC_F9, KC_F10, KC_F11, KC_F12, KC_PSCREEN,KC_SCROLLLOCK,KC_PAUS, 
//     KC_GRAVE, KC_1, KC_2, KC_3, KC_4, KC_5, KC_6, KC_7, KC_8, KC_9, KC_0, KC_MINUS, KC_EQUAL, KC_BSPC,KC_INS, KC_HOME, KC_PGUP,
//     KC_TAB, KC_Q, KC_W, KC_E, KC_R, KC_T, KC_Y, KC_U, KC_I, KC_O, KC_P, KC_LBRACKET, KC_RBRACKET, KC_BSLASH, KC_DEL, KC_END, KC_PGDN,
//     KC_NO, KC_CAPSLOCK, KC_A, KC_S, KC_D, KC_F, KC_G, KC_H, KC_J, KC_K, KC_L, KC_SCOLON, KC_QUOTE, KC_ENTER, KC_NO, KC_NO,
//     KC_NO, KC_LSHIFT, KC_Z, KC_X, KC_C, KC_V, KC_B, KC_N, KC_M, KC_COMMA, KC_DOT, KC_SLASH, KC_RSHIFT, KC_UP, KC_NO,
//     KC_NO, KC_LCTRL, KC_LGUI, KC_LALT, KC_SPACE, KC_RALT, KC_FN0, KC_APP, KC_RCTRL, KC_LEFT, KC_DOWN, KC_RIGHT,
// 	};

const uint8_t lampInputBinding[] = {
    KC_NO, KC_NO, KC_NO, KC_NO, KC_NO, KC_NO, KC_NO, KC_NO, KC_NO, KC_NO, KC_NO, KC_NO, KC_NO, KC_NO, KC_NO, KC_NO, KC_NO, KC_NO, KC_NO, KC_NO, KC_NO, KC_NO, KC_NO, KC_NO, KC_NO, KC_NO, 
    KC_NO, KC_ESC, KC_F1, KC_F2, KC_F3, KC_F4, KC_F5, KC_F6, KC_F7, KC_F8, KC_F9, KC_F10, KC_F11, KC_F12, KC_PSCREEN, KC_NO, 
    KC_NO, KC_GRAVE, KC_1, KC_2, KC_3, KC_4, KC_5, KC_6, KC_7, KC_8, KC_9, KC_0, KC_MINUS, KC_EQUAL, KC_BSPC,KC_DEL, KC_NO, 
    KC_NO, KC_TAB, KC_Q, KC_W, KC_E, KC_R, KC_T, KC_Y, KC_U, KC_I, KC_O, KC_P, KC_LBRACKET, KC_RBRACKET, KC_BSLASH, KC_PGUP, KC_NO,
    KC_NO, KC_CAPSLOCK, KC_A, KC_S, KC_D, KC_F, KC_G, KC_H, KC_J, KC_K, KC_L, KC_SCOLON, KC_QUOTE, KC_ENTER, KC_PGDN, KC_NO,
    KC_NO, KC_LSHIFT, KC_Z, KC_X, KC_C, KC_V, KC_B, KC_N, KC_M, KC_COMMA, KC_DOT, KC_SLASH, KC_RSHIFT, KC_UP, KC_NO,
    KC_NO, KC_LCTRL, KC_LGUI, KC_LALT, KC_SPACE, KC_RALT, KC_FN0, KC_RCTRL, KC_LEFT, KC_DOWN, KC_RIGHT, KC_NO,
	};

extern binary_semaphore_t ledDynamaticLightingSem, usbControlSem,rawSendSem;
extern binary_semaphore_t VIA_ReciveSem;
/* TMK hooks */
__attribute__((weak)) void hook_usb_wakeup(void)
{
#ifdef SLEEP_LED_ENABLE
    sleep_led_disable();
    // NOTE: converters may not accept this
    led_set(host_keyboard_leds());
#endif /* SLEEP_LED_ENABLE */
}

__attribute__((weak)) void hook_usb_suspend_entry(void)
{
#ifdef SLEEP_LED_ENABLE
    sleep_led_enable();
#endif /* SLEEP_LED_ENABLE */
}

/* ---------------------------------------------------------
 *       Global interface variables and declarations
 * ---------------------------------------------------------
 */

uint8_t keyboard_idle __attribute__((aligned(2))) = 0;
uint8_t keyboard_protocol __attribute__((aligned(2))) = 1;
uint16_t keyboard_led_stats __attribute__((aligned(2))) = 0;
volatile uint16_t keyboard_idle_count = 0;
static virtual_timer_t keyboard_idle_timer;
static void keyboard_idle_timer_cb(virtual_timer_t *vtp, void *arg);
#ifdef NKRO_ENABLE
extern bool keyboard_nkro;
#endif /* NKRO_ENABLE */

report_keyboard_t keyboard_report_sent = {{0}};
#ifdef MOUSE_ENABLE
report_mouse_t mouse_report_blank = {0};
#endif /* MOUSE_ENABLE */
#ifdef EXTRAKEY_ENABLE
uint8_t extra_report_blank[3] = {0};
report_gamepad_report extra_game_report = {.report_ID = REPORT_ID_GAMEPAD};
#endif /* EXTRAKEY_ENABLE */
#ifdef LAMP_ARRAY_ENABLE
report_lamp_array_t lamp_array_report = {0};
report_lamp_array_t lamp_array_data_report = {0};
#endif /* LAMP_ARRAY_ENABLE */
#ifdef CONTROL_ENABLE
report_lamp_array_t control_report = {0};
report_lamp_array_t control_data_report = {0};
#endif /* CONTROL_ENABLE */

#ifdef RAW_ENABLE
uint8_t raw_report[RAW_EPSIZE];
#endif

#ifdef CONSOLE_ENABLE
/* The emission buffers queue */
output_buffers_queue_t console_buf_queue;
static uint8_t console_queue_buffer[BQ_BUFFER_SIZE(CONSOLE_QUEUE_CAPACITY, CONSOLE_EPSIZE)];

static virtual_timer_t console_flush_timer;
void console_queue_onotify(io_buffers_queue_t *bqp);
static void console_flush_cb(void *arg);
#endif /* CONSOLE_ENABLE */


/* ---------------------------------------------------------
 *            Descriptors and USB driver objects
 * ---------------------------------------------------------
 */

/* HID specific constants */
#define USB_DESCRIPTOR_HID 0x21
#define USB_DESCRIPTOR_HID_REPORT 0x22

#define HID_GET_REPORT 0x01
#define HID_GET_IDLE 0x02
#define HID_GET_PROTOCOL 0x03
#define HID_SET_REPORT 0x09
#define HID_SET_IDLE 0x0A
#define HID_SET_PROTOCOL 0x0B

/* USB Device Descriptor */
static const uint8_t usb_device_descriptor_data[] = {
    USB_DESC_DEVICE(0x0200,     // bcdUSB
                    0x00,       // bDeviceClass: defined per interface
                    0x00,       // bDeviceSubClass
                    0x00,       // bDeviceProtocol
                    64,         // bMaxPacketSize0
                    VENDOR_ID,
                    PRODUCT_ID,
                    DEVICE_VER,
                    1,
                    2,
                    3,
                    1)
};

/* Device Descriptor wrapper */
static const USBDescriptor usb_device_descriptor = {
    sizeof usb_device_descriptor_data,
    usb_device_descriptor_data};

		/* USB 2.0 Device Qualifier Descriptor */
static const uint8_t usb_device_qualifier_data[] = {
    0x0A,       // bLength
    0x06,       // bDescriptorType
    0x00, 0x02, // bcdUSB 2.00
    0x00,       // bDeviceClass: defined per interface
    0x00,       // bDeviceSubClass
    0x00,       // bDeviceProtocol
    0x40,       // bMaxPacketSize0
    0x01,       // bNumConfigurations
    0x00        // bReserved
};

static const USBDescriptor usb_device_qualifier_descriptor = {
    sizeof(usb_device_qualifier_data),
    usb_device_qualifier_data
};

static const uint8_t usb_device_OS_descriptor_data[] =
    {0x12, 0x03, 'M', 0, 'S', 0, 'F', 0, 'T', 0, '1', 0, '0', 0, '0', 0, 0x34, 0};

static const USBDescriptor usb_device_OSdescriptor = {
    sizeof usb_device_OS_descriptor_data,
    usb_device_OS_descriptor_data};

static const uint8_t WINUSB_ExtendedCompatId_Descritpor_data[] =
    {
        // WCID descriptor = 16byte
        0x28, 0x00, 0x00, 0x00,                   // dwLength
        0x00, 0x01,                               // bcdVersion
        0x04, 0x00,                               // wIndex=4
        0x01,                                     // bCount
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // Reserved[7]

// WCID function descriptor
#ifdef CONTROL_ENABLE
        0x06, // bFirstInterfaceNumber
#else
        0x02, // bFirstInterfaceNumber
#endif
        0x01,                                           // RESERVED ( 0x01 )
        'W', 'I', 'N', 'U', 'S', 'B', 0x00, 0x00,       // compactiableID[8]
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // subCompactiableID[8]
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00              // Reserved[6]
};

static const USBDescriptor WINUSB_ExtendedCompatId_Descritpor = {
    sizeof WINUSB_ExtendedCompatId_Descritpor_data,
    WINUSB_ExtendedCompatId_Descritpor_data};

static const uint8_t WINUSB_ExtendedProperty_InterfaceGUID_Descritpor_data[] =
    {
        // WCID property descriptor = 10 byte
        0x8E, 0x00, 0x00, 0x00, // dwTotalSize = Header + All sections
        0x00, 0x01,             // bcdVersion
        0x05, 0x00,             // wIndex=5
        0x01, 0x00,             // wCount

        0x84, 0x00, 0x00, 0x00, // dwSize -- this section

        0x01, 0x00, 0x00, 0x00, // dwPropertyDataType

        0x28, 0x00, // wPropertyNameLength

        'D', 0x00, 'e', 0x00,  // bProperytName : WCHAR : L"DeviceInterfaceGUID"
        'v', 0x00, 'i', 0x00,  // bProperytName : WCHAR
        'c', 0x00, 'e', 0x00,  // bProperytName : WCHAR
        'I', 0x00, 'n', 0x00,  // bProperytName : WCHAR
        't', 0x00, 'e', 0x00,  // bProperytName : WCHAR
        'r', 0x00, 'f', 0x00,  // bProperytName : WCHAR
        'a', 0x00, 'c', 0x00,  // bProperytName : WCHAR
        'e', 0x00, 'G', 0x00,  // bProperytName : WCHAR
        'U', 0x00, 'I', 0x00,  // bProperytName : WCHAR
        'D', 0x00, 0x00, 0x00, // bProperytName : WCHAR

        0x4E, 0x00, 0x00, 0x00, // dwPropertyDataLength : 78 Bytes = 0x0000004E
                                // bb8ba7f3-5dae-4e74-88c1-114d378b9807
        '{', 0x00, 'b', 0x00,   // bPropertyData : WCHAR : L"{12345678-1234-1234-1234-123456789ABC}"
        'b', 0x00, '8', 0x00,   // bPropertyData
        'b', 0x00, 'a', 0x00,   // bPropertyData
        '7', 0x00, 'f', 0x00,   // bPropertyData
        '3', 0x00, '-', 0x00,   // bPropertyData
        '5', 0x00, 'd', 0x00,   // bPropertyData
        'a', 0x00, 'e', 0x00,   // bPropertyData
        '-', 0x00, '4', 0x00,   // bPropertyData
        'e', 0x00, '7', 0x00,   // bPropertyData
        '4', 0x00, '-', 0x00,   // bPropertyData
        '8', 0x00, '8', 0x00,   // bPropertyData
        'c', 0x00, '1', 0x00,   // bPropertyData
        '-', 0x00, '1', 0x00,   // bPropertyData
        '1', 0x00, '4', 0x00,   // bPropertyData
        'd', 0x00, '3', 0x00,   // bPropertyData
        '7', 0x00, '8', 0x00,   // bPropertyData
        'b', 0x00, '9', 0x00,   // bPropertyData
        '8', 0x00, '0', 0x00,   // bPropertyData
        '7', 0x00, '}', 0x00,   // bPropertyData
        0x00, 0x00              // bPropertyData
                                //{dee824ef-729b-4a0e-9c14-b7117d33a817}
                                // '{', 0x00, 'd', 0x00,       // bPropertyData : WCHAR : L"{12345678-1234-1234-1234-123456789ABC}"
                                // 'e', 0x00, 'e', 0x00,       // bPropertyData
                                // '8', 0x00, '2', 0x00,       // bPropertyData
                                // '4', 0x00, 'e', 0x00,       // bPropertyData
                                // 'f', 0x00, '-', 0x00,       // bPropertyData
                                // '7', 0x00, '2', 0x00,       // bPropertyData
                                // '9', 0x00, 'b', 0x00,       // bPropertyData
                                // '-', 0x00, '4', 0x00,       // bPropertyData
                                // 'a', 0x00, '0', 0x00,       // bPropertyData
                                // 'e', 0x00, '-', 0x00,       // bPropertyData
                                // '9', 0x00, 'c', 0x00,       // bPropertyData
                                // '1', 0x00, '4', 0x00,       // bPropertyData
                                // '-', 0x00, 'b', 0x00,       // bPropertyData
                                // '7', 0x00, '1', 0x00,       // bPropertyData
                                // '1', 0x00, '7', 0x00,       // bPropertyData
                                // 'd', 0x00, '3', 0x00,       // bPropertyData
                                // '3', 0x00, 'a', 0x00,       // bPropertyData
                                // '8', 0x00, '1', 0x00,       // bPropertyData
                                // '8', 0x00, '}', 0x00,       // bPropertyData
                                // 0x00, 0x00                  // bPropertyData
};

static const USBDescriptor WINUSB_ExtendedProperty_InterfaceGUID_Descritpor = {
    sizeof WINUSB_ExtendedProperty_InterfaceGUID_Descritpor_data,
    WINUSB_ExtendedProperty_InterfaceGUID_Descritpor_data};

/*
 * HID Report Descriptor
 *
 * See "Device Class Definition for Human Interface Devices (HID)"
 * (http://www.usb.org/developers/hidpage/HID1_11.pdf) for the
 * detailed descrition of all the fields
 */

/* Keyboard Protocol 1, HID 1.11 spec, Appendix B, page 59-60 */
static const uint8_t keyboard_hid_report_desc_data[] = {
    0x05, 0x01,            // Usage Page (Generic Desktop),
    0x09, 0x06,            // Usage (Keyboard),
    0xA1, 0x01,            // Collection (Application),
    0x75, 0x01,            //   Report Size (1),
    0x95, 0x08,            //   Report Count (8),
    0x05, 0x07,            //   Usage Page (Key Codes),
    0x19, 0xE0,            //   Usage Minimum (224),
    0x29, 0xE7,            //   Usage Maximum (231),
    0x15, 0x00,            //   Logical Minimum (0),
    0x25, 0x01,            //   Logical Maximum (1),
    0x81, 0x02,            //   Input (Data, Variable, Absolute), ;Modifier byte
    0x95, 0x01,            //   Report Count (1),
    0x75, 0x08,            //   Report Size (8),
    0x81, 0x03,            //   Input (Constant),                 ;Reserved byte
    0x95, 0x05,            //   Report Count (5),
    0x75, 0x01,            //   Report Size (1),
    0x05, 0x08,            //   Usage Page (LEDs),
    0x19, 0x01,            //   Usage Minimum (1),
    0x29, 0x05,            //   Usage Maximum (5),
    0x91, 0x02,            //   Output (Data, Variable, Absolute), ;LED report
    0x95, 0x01,            //   Report Count (1),
    0x75, 0x03,            //   Report Size (3),
    0x91, 0x03,            //   Output (Constant),                 ;LED report padding
    0x95, KBD_REPORT_KEYS, //   Report Count (),
    0x75, 0x08,            //   Report Size (8),
    0x15, 0x00,            //   Logical Minimum (0),
    0x26, 0xFF, 0x00,      //   Logical Maximum(255),
    0x05, 0x07,            //   Usage Page (Key Codes),
    0x19, 0x00,            //   Usage Minimum (0),
    0x29, 0xFF,            //   Usage Maximum (255),
    0x81, 0x00,            //   Input (Data, Array),
    0xc0                   // End Collection
};
/* wrapper */
static const USBDescriptor keyboard_hid_report_descriptor = {
    sizeof keyboard_hid_report_desc_data,
    keyboard_hid_report_desc_data};

#ifdef NKRO_ENABLE
static const uint8_t nkro_hid_report_desc_data[] = {
    0x05, 0x01, // Usage Page (Generic Desktop),
    0x09, 0x06, // Usage (Keyboard),
    0xA1, 0x01, // Collection (Application),
    // bitmap of modifiers
    0x75, 0x01, //   Report Size (1),
    0x95, 0x08, //   Report Count (8),
    0x05, 0x07, //   Usage Page (Key Codes),
    0x19, 0xE0, //   Usage Minimum (224),
    0x29, 0xE7, //   Usage Maximum (231),
    0x15, 0x00, //   Logical Minimum (0),
    0x25, 0x01, //   Logical Maximum (1),
    0x81, 0x02, //   Input (Data, Variable, Absolute), ;Modifier byte
    // LED output report
    0x95, 0x05, //   Report Count (5),
    0x75, 0x01, //   Report Size (1),
    0x05, 0x08, //   Usage Page (LEDs),
    0x19, 0x01, //   Usage Minimum (1),
    0x29, 0x05, //   Usage Maximum (5),
    0x91, 0x02, //   Output (Data, Variable, Absolute),
    0x95, 0x01, //   Report Count (1),
    0x75, 0x03, //   Report Size (3),
    0x91, 0x03, //   Output (Constant),
    // bitmap of keys
    0x95, NKRO_REPORT_KEYS * 8,     //   Report Count (),
    0x75, 0x01,                     //   Report Size (1),
    0x15, 0x00,                     //   Logical Minimum (0),
    0x25, 0x01,                     //   Logical Maximum(1),
    0x05, 0x07,                     //   Usage Page (Key Codes),
    0x19, 0x00,                     //   Usage Minimum (0),
    0x29, NKRO_REPORT_KEYS * 8 - 1, //   Usage Maximum (),
    0x81, 0x02,                     //   Input (Data, Variable, Absolute),
    0xc0                            // End Collection
};
/* wrapper */
static const USBDescriptor nkro_hid_report_descriptor = {
    sizeof nkro_hid_report_desc_data,
    nkro_hid_report_desc_data};
#endif /* NKRO_ENABLE */

#ifdef LAMP_ARRAY_ENABLE
static const uint8_t lamp_array_hid_report_desc_data[] = {
    0x05, 0x59,                   // Usage Page
    0x09, 0x01,                   // Usage
    0xA1, 0x01,                   // Collection (Application)
    0x85, 0x01,                   //       Report ID (1)
    0x09, 0x02,                   //       Usage
    0xA1, 0x02,                   //       Collection (Logical)
    0x09, 0x03,                   //             Usage
    0x15, 0x00,                   //             Logical Minimum (0)
    0x27, 0xFF, 0xFF, 0x00, 0x00, //             Logical Maximum (65535)
    0x75, 0x10,                   //             Report Size (16)
    0x95, 0x01,                   //             Report Count (1)
    0xB1, 0x03,                   //             Feature (Cnst,Var,Abs,NWrp,Lin,Pref,NNul,NVol,Bit)
    0x09, 0x04,                   //             Usage
    0x09, 0x05,                   //             Usage
    0x09, 0x06,                   //             Usage
    0x09, 0x07,                   //             Usage
    0x09, 0x08,                   //             Usage
    0x15, 0x00,                   //             Logical Minimum (0)
    0x27, 0xFF, 0xFF, 0xFF, 0x7F, //             Logical Maximum (2147483647)
    0x75, 0x20,                   //             Report Size (32)
    0x95, 0x05,                   //             Report Count (5)
    0xB1, 0x03,                   //             Feature (Cnst,Var,Abs,NWrp,Lin,Pref,NNul,NVol,Bit)
    0xC0,                         //       End Collection
    0x85, 0x02,                   //       Report ID (2)
    0x09, 0x20,                   //       Usage
    0xA1, 0x02,                   //       Collection (Logical)
    0x09, 0x21,                   //             Usage
    0x15, 0x00,                   //             Logical Minimum (0)
    0x27, 0xFF, 0xFF, 0x00, 0x00, //             Logical Maximum (65535)
    0x75, 0x10,                   //             Report Size (16)
    0x95, 0x01,                   //             Report Count (1)
    0xB1, 0x02,                   //             Feature (Data,Var,Abs,NWrp,Lin,Pref,NNul,NVol,Bit)
    0xC0,                         //       End Collection
    0x85, 0x03,                   //       Report ID (3)
    0x09, 0x22,                   //       Usage
    0xA1, 0x02,                   //       Collection (Logical)
    0x09, 0x21,                   //             Usage
    0x15, 0x00,                   //             Logical Minimum (0)
    0x27, 0xFF, 0xFF, 0x00, 0x00, //             Logical Maximum (65535)
    0x75, 0x10,                   //             Report Size (16)
    0x95, 0x01,                   //             Report Count (1)
    0xB1, 0x02,                   //             Feature (Data,Var,Abs,NWrp,Lin,Pref,NNul,NVol,Bit)
    0x09, 0x23,                   //             Usage
    0x09, 0x24,                   //             Usage
    0x09, 0x25,                   //             Usage
    0x09, 0x27,                   //             Usage
    0x09, 0x26,                   //             Usage
    0x15, 0x00,                   //             Logical Minimum (0)
    0x27, 0xFF, 0xFF, 0xFF, 0x7F, //             Logical Maximum (2147483647)
    0x75, 0x20,                   //             Report Size (32)
    0x95, 0x05,                   //             Report Count (5)
    0xB1, 0x02,                   //             Feature (Data,Var,Abs,NWrp,Lin,Pref,NNul,NVol,Bit)
    0x09, 0x28,                   //             Usage
    0x09, 0x29,                   //             Usage
    0x09, 0x2A,                   //             Usage
    0x09, 0x2B,                   //             Usage
    0x09, 0x2C,                   //             Usage
    0x09, 0x2D,                   //             Usage
    0x15, 0x00,                   //             Logical Minimum (0)
    0x26, 0xFF, 0x00,             //             Logical Maximum (255)
    0x75, 0x08,                   //             Report Size (8)
    0x95, 0x06,                   //             Report Count (6)
    0xB1, 0x02,                   //             Feature (Data,Var,Abs,NWrp,Lin,Pref,NNul,NVol,Bit)
    0xC0,                         //       End Collection
    0x85, 0x04,                   //       Report ID (4)
    0x09, 0x50,                   //       Usage
    0xA1, 0x02,                   //       Collection (Logical)
    0x09, 0x03,                   //             Usage
    0x09, 0x55,                   //             Usage
    0x15, 0x00,                   //             Logical Minimum (0)
    0x25, 0x08,                   //             Logical Maximum (8)
    0x75, 0x08,                   //             Report Size (8)
    0x95, 0x02,                   //             Report Count (2)
    0xB1, 0x02,                   //             Feature (Data,Var,Abs,NWrp,Lin,Pref,NNul,NVol,Bit)
    0x09, 0x21,                   //             Usage
    0x15, 0x00,                   //             Logical Minimum (0)
    0x27, 0xFF, 0xFF, 0x00, 0x00, //             Logical Maximum (65535)
    0x75, 0x10,                   //             Report Size (16)
    0x95, 0x08,                   //             Report Count (8)
    0xB1, 0x02,                   //             Feature (Data,Var,Abs,NWrp,Lin,Pref,NNul,NVol,Bit)
    0x09, 0x51,                   //             Usage
    0x09, 0x52,                   //             Usage
    0x09, 0x53,                   //             Usage
    0x09, 0x54,                   //             Usage
    0x09, 0x51,                   //             Usage
    0x09, 0x52,                   //             Usage
    0x09, 0x53,                   //             Usage
    0x09, 0x54,                   //             Usage
    0x09, 0x51,                   //             Usage
    0x09, 0x52,                   //             Usage
    0x09, 0x53,                   //             Usage
    0x09, 0x54,                   //             Usage
    0x09, 0x51,                   //             Usage
    0x09, 0x52,                   //             Usage
    0x09, 0x53,                   //             Usage
    0x09, 0x54,                   //             Usage
    0x09, 0x51,                   //             Usage
    0x09, 0x52,                   //             Usage
    0x09, 0x53,                   //             Usage
    0x09, 0x54,                   //             Usage
    0x09, 0x51,                   //             Usage
    0x09, 0x52,                   //             Usage
    0x09, 0x53,                   //             Usage
    0x09, 0x54,                   //             Usage
    0x09, 0x51,                   //             Usage
    0x09, 0x52,                   //             Usage
    0x09, 0x53,                   //             Usage
    0x09, 0x54,                   //             Usage
    0x09, 0x51,                   //             Usage
    0x09, 0x52,                   //             Usage
    0x09, 0x53,                   //             Usage
    0x09, 0x54,                   //             Usage
    0x15, 0x00,                   //             Logical Minimum (0)
    0x26, 0xFF, 0x00,             //             Logical Maximum (255)
    0x75, 0x08,                   //             Report Size (8)
    0x95, 0x20,                   //             Report Count (32)
    0xB1, 0x02,                   //             Feature (Data,Var,Abs,NWrp,Lin,Pref,NNul,NVol,Bit)
    0xC0,                         //       End Collection
    0x85, 0x05,                   //       Report ID (5)
    0x09, 0x60,                   //       Usage
    0xA1, 0x02,                   //       Collection (Logical)
    0x09, 0x55,                   //             Usage
    0x15, 0x00,                   //             Logical Minimum (0)
    0x25, 0x08,                   //             Logical Maximum (8)
    0x75, 0x08,                   //             Report Size (8)
    0x95, 0x01,                   //             Report Count (1)
    0xB1, 0x02,                   //             Feature (Data,Var,Abs,NWrp,Lin,Pref,NNul,NVol,Bit)
    0x09, 0x61,                   //             Usage
    0x09, 0x62,                   //             Usage
    0x15, 0x00,                   //             Logical Minimum (0)
    0x27, 0xFF, 0xFF, 0x00, 0x00, //             Logical Maximum (65535)
    0x75, 0x10,                   //             Report Size (16)
    0x95, 0x02,                   //             Report Count (2)
    0xB1, 0x02,                   //             Feature (Data,Var,Abs,NWrp,Lin,Pref,NNul,NVol,Bit)
    0x09, 0x51,                   //             Usage
    0x09, 0x52,                   //             Usage
    0x09, 0x53,                   //             Usage
    0x09, 0x54,                   //             Usage
    0x15, 0x00,                   //             Logical Minimum (0)
    0x26, 0xFF, 0x00,             //             Logical Maximum (255)
    0x75, 0x08,                   //             Report Size (8)
    0x95, 0x04,                   //             Report Count (4)
    0xB1, 0x02,                   //             Feature (Data,Var,Abs,NWrp,Lin,Pref,NNul,NVol,Bit)
    0xC0,                         //       End Collection
    0x85, 0x06,                   //       Report ID (6)
    0x09, 0x70,                   //       Usage
    0xA1, 0x02,                   //       Collection (Logical)
    0x09, 0x71,                   //             Usage
    0x15, 0x00,                   //             Logical Minimum (0)
    0x25, 0x01,                   //             Logical Maximum (1)
    0x75, 0x08,                   //             Report Size (8)
    0x95, 0x01,                   //             Report Count (1)
    0xB1, 0x02,                   //             Feature (Data,Var,Abs,NWrp,Lin,Pref,NNul,NVol,Bit)
    0xC0,                         //       End Collection
    0x85, 0x07,                   //       Report ID (7)
    0x09, 0xFE,                   //       Usage
    0xA1, 0x02,                   //       Collection (Logical)
    0x09, 0x71,                   //             Usage
    0x15, 0x00,                   //             Logical Minimum (0)
    0x25, 0xff,                   //             Logical Maximum (0xFF)
    0x75, 0x08,                   //             Report Size (8)
    0x95, 0x3f,                   //             Report Count (63)
    0xB1, 0x02,                   //             Feature (Data,Var,Abs,NWrp,Lin,Pref,NNul,NVol,Bit)
    0xC0,                         //       End Collection
    0x85, 0x08,                   //       Report ID (8)

    0x09, 0x61, //       Usage (Vendor-Defined 97)
    0xA1, 0x01, //       Collection (Application)
    0x09, 0x62, //          Usageage (Vendor-Defined 98)
    0x15, 0x00, //             Logical Minimum (0)
    0x25, 0xff, //             Logical Maximum (0xFF)
    0x75, 0x08, //             Report Size (8)
    0x95, 0x3f, //             Report Count (63)
    0x81, 0x02, //             Input (Data,Var,Abs,NWrp,Lin,Pref,NNul,Bit)
    0xC0,       //       End Collection
    0xC0,       //  End Collection
};

/* wrapper */
static const USBDescriptor lamp_array_hid_report_descriptor = {
    sizeof lamp_array_hid_report_desc_data,
    lamp_array_hid_report_desc_data};

#endif /* LAMP_ARRAY */

#ifdef CONTROL_ENABLE
static const uint8_t control_hid_report_desc_data[] = {
    //// 0x06, 0x31, 0xFF, // Usage Page 0xFF31 (vendor defined)
    //// 0x09, 0x74,       // Usage 0x74
    //// 0xA1, 0x53,       // Collection 0x53
    //
    // 0x05,0x59,	                // Usage Page
    // 0x09,0x01,	                // Usage
    // 0xA1,0x01,	                // Collection (Application)
    //
    // 0x85,0x01,	                //       Report ID (1)
    // 0x09,0x02,	                //       Usage
    // 0xA1,0x02,	                //       Collection (Logical)
    // 0x09,0x03,	                //             Usage
    // 0x15,0x00,	                //             Logical Minimum (0)
    // 0x27,0xFF,0xFF,0x00,0x00,	//             Logical Maximum (65535)
    // 0x75,0x10,	                //             Report Size (16)
    // 0x95,0x01,	                //             Report Count (1)
    // 0xB1,0x03,               	//             Feature (Cnst,Var,Abs,NWrp,Lin,Pref,NNul,NVol,Bit)
    // 0x09,0x04,               	//             Usage
    // 0x09,0x05,                	//             Usage
    // 0x09,0x06,                	//             Usage
    // 0x09,0x07,                	//             Usage
    // 0x09,0x08,                 	//             Usage
    // 0x15,0x00,                	//             Logical Minimum (0)
    // 0x27,0xFF,0xFF,0xFF,0x7F, 	//             Logical Maximum (2147483647)
    // 0x75,0x20,                 	//             Report Size (32)
    // 0x95,0x05,                 	//             Report Count (5)
    // 0xB1,0x03,                 	//             Feature (Cnst,Var,Abs,NWrp,Lin,Pref,NNul,NVol,Bit)
    // 0xC0,	                    //       End Collection
    // 0x85,0x02,                 	//       Report ID (2)
    // 0x09,0x20,                	//       Usage
    // 0xA1,0x02,                 	//       Collection (Logical)
    // 0x09,0x21,                 	//             Usage
    // 0x15,0x00,                	//             Logical Minimum (0)
    // 0x27,0xFF,0xFF,0x00,0x00, 	//             Logical Maximum (65535)
    // 0x75,0x10,                	//             Report Size (16)
    // 0x95,0x01,                 	//             Report Count (1)
    // 0xB1,0x02,                 	//             Feature (Data,Var,Abs,NWrp,Lin,Pref,NNul,NVol,Bit)
    // 0xC0,                     	//       End Collection
    // 0x85,0x03,                 	//       Report ID (3)
    // 0x09,0x22,                 	//       Usage
    // 0xA1,0x02,                 	//       Collection (Logical)
    // 0x09,0x21,                 	//             Usage
    // 0x15,0x00,                 	//             Logical Minimum (0)
    // 0x27,0xFF,0xFF,0x00,0x00,	//             Logical Maximum (65535)
    // 0x75,0x10,                 	//             Report Size (16)
    // 0x95,0x01,                 	//             Report Count (1)
    // 0xB1,0x02,                 	//             Feature (Data,Var,Abs,NWrp,Lin,Pref,NNul,NVol,Bit)
    // 0x09,0x23,                 	//             Usage
    // 0x09,0x24,                 	//             Usage
    // 0x09,0x25,                 	//             Usage
    // 0x09,0x27,                 	//             Usage
    // 0x09,0x26,                 	//             Usage
    // 0x15,0x00,               	//             Logical Minimum (0)
    // 0x27,0xFF,0xFF,0xFF,0x7F,	//             Logical Maximum (2147483647)
    // 0x75,0x20,                 	//             Report Size (32)
    // 0x95,0x05,                 	//             Report Count (5)
    // 0xB1,0x02,                 	//             Feature (Data,Var,Abs,NWrp,Lin,Pref,NNul,NVol,Bit)
    // 0x09,0x28,                 	//             Usage
    // 0x09,0x29,                 	//             Usage
    // 0x09,0x2A,                 	//             Usage
    // 0x09,0x2B,                 	//             Usage
    // 0x09,0x2C,                 	//             Usage
    // 0x09,0x2D,               	//             Usage
    // 0x15,0x00,               	//             Logical Minimum (0)
    // 0x26,0xFF,0x00,	            //             Logical Maximum (255)
    // 0x75,0x08,                 	//             Report Size (8)
    // 0x95,0x06,                 	//             Report Count (6)
    // 0xB1,0x02,                 	//             Feature (Data,Var,Abs,NWrp,Lin,Pref,NNul,NVol,Bit)
    // 0xC0,	                    //       End Collection
    // 0x85,0x04,                 	//       Report ID (4)
    // 0x09,0x50,                 	//       Usage
    // 0xA1,0x02,                 	//       Collection (Logical)
    // 0x09,0x03,                 	//             Usage
    // 0x09,0x55,               	//             Usage
    // 0x15,0x00,                 	//             Logical Minimum (0)
    // 0x25,0x08,                 	//             Logical Maximum (8)
    // 0x75,0x08,                 	//             Report Size (8)
    // 0x95,0x02,                 	//             Report Count (2)
    // 0xB1,0x02,                 	//             Feature (Data,Var,Abs,NWrp,Lin,Pref,NNul,NVol,Bit)
    // 0x09,0x21,                 	//             Usage
    // 0x15,0x00,                 	//             Logical Minimum (0)
    // 0x27,0xFF,0xFF,0x00,0x00,	//             Logical Maximum (65535)
    // 0x75,0x10,               	//             Report Size (16)
    // 0x95,0x08,               	//             Report Count (8)
    // 0xB1,0x02,               	//             Feature (Data,Var,Abs,NWrp,Lin,Pref,NNul,NVol,Bit)
    // 0x09,0x51,               	//             Usage
    // 0x09,0x52,               	//             Usage
    // 0x09,0x53,               	//             Usage
    // 0x09,0x54,               	//             Usage
    // 0x09,0x51,               	//             Usage
    // 0x09,0x52,               	//             Usage
    // 0x09,0x53,               	//             Usage
    // 0x09,0x54,               	//             Usage
    // 0x09,0x51,               	//             Usage
    // 0x09,0x52,               	//             Usage
    // 0x09,0x53,               	//             Usage
    // 0x09,0x54,               	//             Usage
    // 0x09,0x51,               	//             Usage
    // 0x09,0x52,               	//             Usage
    // 0x09,0x53,               	//             Usage
    // 0x09,0x54,               	//             Usage
    // 0x09,0x51,               	//             Usage
    // 0x09,0x52,               	//             Usage
    // 0x09,0x53,               	//             Usage
    // 0x09,0x54,               	//             Usage
    // 0x09,0x51,               	//             Usage
    // 0x09,0x52,                 	//             Usage
    // 0x09,0x53,                 	//             Usage
    // 0x09,0x54,                 	//             Usage
    // 0x09,0x51,                 	//             Usage
    // 0x09,0x52,                 	//             Usage
    // 0x09,0x53,                 	//             Usage
    // 0x09,0x54,                 	//             Usage
    // 0x09,0x51,                 	//             Usage
    // 0x09,0x52,                 	//             Usage
    // 0x09,0x53,                 	//             Usage
    // 0x09,0x54,                 	//             Usage
    // 0x15,0x00,                 	//             Logical Minimum (0)
    // 0x26,0xFF,0x00,             //             Logical Maximum (255)
    // 0x75,0x08,                 	//             Report Size (8)
    // 0x95,0x20,                 	//             Report Count (32)
    // 0xB1,0x02,                 	//             Feature (Data,Var,Abs,NWrp,Lin,Pref,NNul,NVol,Bit)
    // 0xC0,                      	//       End Collection
    // 0x85,0x05,                 	//       Report ID (5)
    // 0x09,0x60,                 	//       Usage
    // 0xA1,0x02,                 	//       Collection (Logical)
    // 0x09,0x55,                 	//             Usage
    // 0x15,0x00,                 	//             Logical Minimum (0)
    // 0x25,0x08,                 	//             Logical Maximum (8)
    // 0x75,0x08,                 	//             Report Size (8)
    // 0x95,0x01,                 	//             Report Count (1)
    // 0xB1,0x02,                 	//             Feature (Data,Var,Abs,NWrp,Lin,Pref,NNul,NVol,Bit)
    // 0x09,0x61,                 	//             Usage
    // 0x09,0x62,                 	//             Usage
    // 0x15,0x00,                 	//             Logical Minimum (0)
    // 0x27,0xFF,0xFF,0x00,0x00,	//             Logical Maximum (65535)
    // 0x75,0x10,                 	//             Report Size (16)
    // 0x95,0x02,                 	//             Report Count (2)
    // 0xB1,0x02,                 	//             Feature (Data,Var,Abs,NWrp,Lin,Pref,NNul,NVol,Bit)
    // 0x09,0x51,                 	//             Usage
    // 0x09,0x52,                 	//             Usage
    // 0x09,0x53,                 	//             Usage
    // 0x09,0x54,                 	//             Usage
    // 0x15,0x00,                 	//             Logical Minimum (0)
    // 0x26,0xFF,0x00,             //             Logical Maximum (255)
    // 0x75,0x08,                 	//             Report Size (8)
    // 0x95,0x04,                 	//             Report Count (4)
    // 0xB1,0x02,                 	//             Feature (Data,Var,Abs,NWrp,Lin,Pref,NNul,NVol,Bit)
    // 0xC0,                     	//       End Collection
    // 0x85,0x06,                 	//       Report ID (6)
    // 0x09,0x70,                 	//       Usage
    // 0xA1,0x02,                 	//       Collection (Logical)
    // 0x09,0x71,                 	//             Usage
    // 0x15,0x00,               	//             Logical Minimum (0)
    // 0x25,0x01,                 	//             Logical Maximum (1)
    // 0x75,0x08,                 	//             Report Size (8)
    // 0x95,0x01,                 	//             Report Count (1)
    // 0xB1,0x02,                 	//             Feature (Data,Var,Abs,NWrp,Lin,Pref,NNul,NVol,Bit)
    // 0xC0,                      	//       End Collection
    // 0x85,0x07,                 	//       Report ID (7)
    // 0x09,0xFE,                 	//       Usage
    // 0xA1,0x02,                 	//       Collection (Logical)
    // 0x09,0x71,                 	//             Usage
    // 0x15,0x00,               	  //             Logical Minimum (0)
    // 0x25,0xff,                 	//             Logical Maximum (0xFF)
    // 0x75,0x08,                 	//             Report Size (8)
    // 0x95,0x3f,                 	//             Report Count (63)
    // 0xB1,0x02,                 	//             Feature (Data,Var,Abs,NWrp,Lin,Pref,NNul,NVol,Bit)
    // 0xC0,                      	//       End Collection
    //
    // 0x85,0x08,                 	//       Report ID (8)
    // 0x09 ,0x61,			            //       Usage (Vendor-Defined 97)
    // 0xA1 ,0x01,			            //       Collection (Application)
    // 0x09 ,0x62,			            //          Usageage (Vendor-Defined 98)
    // 0x15 ,0x00,               	//             Logical Minimum (0)
    // 0x25 ,0xff,                 //             Logical Maximum (0xFF)
    // 0x75 ,0x08,                 //             Report Size (8)
    // 0x95 ,0x3f,                 //             Report Count (63)
    // 0x81 ,0x02,                 //             Input (Data,Var,Abs,NWrp,Lin,Pref,NNul,Bit)
    // 0xC0,                      	//       End Collection
    // 0xC0,                     	//  End Collection

    0x06, 0x00, 0xFF, // Usage Page (Vendor-Defined 1)
    0x09, 0x01,       // Usage (Vendor-Defined 1)
    0xA1, 0x01,       // Collection (Application)
    0x15, 0x00,       //
    0x26, 0xFF, 0x00, //
    0x75, 0x08,       //
    0x95, 0x40,       //
    0x09, 0x01,       //
    0x81, 0x02,       //
    0x95, 0x40,       //
    0x09, 0x01,       //
    0x91, 0x02,       //
    0x95, 0x01,       //
    0x09, 0x01,       //
    0xB1, 0x02,       //
    0xc0              // End Collection

};

/* wrapper */
static const USBDescriptor control_hid_report_descriptor = {
    sizeof control_hid_report_desc_data,
    control_hid_report_desc_data};

#endif /* CONTROL */

#ifdef MOUSE_ENABLE
/* Mouse Protocol 1, HID 1.11 spec, Appendix B, page 59-60, with wheel extension
 * http://www.microchip.com/forums/tm.aspx?high=&m=391435&mpage=1#391521
 * http://www.keil.com/forum/15671/
 * http://www.microsoft.com/whdc/device/input/wheel.mspx */
static const uint8_t mouse_hid_report_desc_data[] = {
    /* mouse */
    0x05, 0x01, // USAGE_PAGE (Generic Desktop)
    0x09, 0x02, // USAGE (Mouse)
    0xa1, 0x01, // COLLECTION (Application)
    // 0x85, REPORT_ID_MOUSE,         //   REPORT_ID (1)
    0x09, 0x01,       //   USAGE (Pointer)
    0xa1, 0x00,       //   COLLECTION (Physical)
                      // ----------------------------  Buttons
    0x05, 0x09,       //     USAGE_PAGE (Button)
    0x19, 0x01,       //     USAGE_MINIMUM (Button 1)
    0x29, 0x05,       //     USAGE_MAXIMUM (Button 5)
    0x15, 0x00,       //     LOGICAL_MINIMUM (0)
    0x25, 0x01,       //     LOGICAL_MAXIMUM (1)
    0x75, 0x01,       //     REPORT_SIZE (1)
    0x95, 0x05,       //     REPORT_COUNT (5)
    0x81, 0x02,       //     INPUT (Data,Var,Abs)
    0x75, 0x03,       //     REPORT_SIZE (3)
    0x95, 0x01,       //     REPORT_COUNT (1)
    0x81, 0x03,       //     INPUT (Cnst,Var,Abs)
                      // ----------------------------  X,Y position
    0x05, 0x01,       //     USAGE_PAGE (Generic Desktop)
    0x09, 0x30,       //     USAGE (X)
    0x09, 0x31,       //     USAGE (Y)
    0x15, 0x81,       //     LOGICAL_MINIMUM (-127)
    0x25, 0x7f,       //     LOGICAL_MAXIMUM (127)
    0x75, 0x08,       //     REPORT_SIZE (8)
    0x95, 0x02,       //     REPORT_COUNT (2)
    0x81, 0x06,       //     INPUT (Data,Var,Rel)
                      // ----------------------------  Vertical wheel
    0x09, 0x38,       //     USAGE (Wheel)
    0x15, 0x81,       //     LOGICAL_MINIMUM (-127)
    0x25, 0x7f,       //     LOGICAL_MAXIMUM (127)
    0x35, 0x00,       //     PHYSICAL_MINIMUM (0)        - reset physical
    0x45, 0x00,       //     PHYSICAL_MAXIMUM (0)
    0x75, 0x08,       //     REPORT_SIZE (8)
    0x95, 0x01,       //     REPORT_COUNT (1)
    0x81, 0x06,       //     INPUT (Data,Var,Rel)
                      // ----------------------------  Horizontal wheel
    0x05, 0x0c,       //     USAGE_PAGE (Consumer Devices)
    0x0a, 0x38, 0x02, //     USAGE (AC Pan)
    0x15, 0x81,       //     LOGICAL_MINIMUM (-127)
    0x25, 0x7f,       //     LOGICAL_MAXIMUM (127)
    0x75, 0x08,       //     REPORT_SIZE (8)
    0x95, 0x01,       //     REPORT_COUNT (1)
    0x81, 0x06,       //     INPUT (Data,Var,Rel)
    0xc0,             //   END_COLLECTION
    0xc0,             // END_COLLECTION
};
/* wrapper */
static const USBDescriptor mouse_hid_report_descriptor = {
    sizeof mouse_hid_report_desc_data,
    mouse_hid_report_desc_data};
#endif /* MOUSE_ENABLE */

#ifdef RAW_ENABLE
static const uint8_t raw_hid_report_desc_data[] = {
    0x06, 0x60, 0xFF, // Usage Page (Vendor-Defined 97)
    0x09, 0x61,       // Usage (Vendor-Defined 97)
    0xA1, 0x01,       // Collection (Application)
    // 0x85, 0x01,                 //       Report ID (1)
    0x09, 0x62,       //    Usage (Vendor-Defined 98)
    0x15, 0x00,       //    Logical Minimum (0)
    0x26, 0xFF, 0x00, //    Logical Maximum (255)
    0x95, 0x20,       //    Report Count (32)
    0x75, 0x08,       //    Report Size (8)
    0x81, 0x02,       //    Input (Data,Var,Abs,NWrp,Lin,Pref,NNul,Bit)

    0x09, 0x63,       //    Usage (Vendor-Defined 99)
    0x15, 0x00,       //    Logical Minimum (0)
    0x26, 0xFF, 0x00, //    Logical Maximum (255)
    0x95, 0x20,       //    Report Count (32)
    0x75, 0x08,       //    Report Size (8)
    0x91, 0x02,       //    Output (Data,Var,Abs,NWrp,Lin,Pref,NNul,NVol,Bit)

    0xC0, // End Collection

	// 0xA1, 0x01,                 //Collection (Application)
    // 0x85, 0x0b,                 //       Report ID (1)
	// 0x09, 0x62,                 //    Usage (Vendor-Defined 98)
	// 0x15, 0x00,                 //    Logical Minimum (0)
	// 0x26, 0xFF, 0x00,           //    Logical Maximum (255)
	// 0x95, 0x20,                 //    Report Count (32)
	// 0x75, 0x08,                 //    Report Size (8)
	// 0x81, 0x02,                 //    Input (Data,Var,Abs,NWrp,Lin,Pref,NNul,Bit)

	// 0x09, 0x63,                 //    Usage (Vendor-Defined 99)
	// 0x15, 0x00,                 //    Logical Minimum (0)
	// 0x26, 0xFF, 0x00,           //    Logical Maximum (255)
	// 0x95, 0x20,                 //    Report Count (32)
	// 0x75, 0x08,                 //    Report Size (8)
	// 0x91, 0x02,                 //    Output (Data,Var,Abs,NWrp,Lin,Pref,NNul,NVol,Bit)

	// 0xC0,                       //End Collection    
};

static const USBDescriptor raw_hid_report_descriptor = {
    sizeof raw_hid_report_desc_data,
    raw_hid_report_desc_data};    

#endif

#ifdef CONSOLE_ENABLE
static const uint8_t console_hid_report_desc_data[] = {
    0x06, 0x31, 0xFF,     // Usage Page 0xFF31 (vendor defined)
    0x09, 0x74,           // Usage 0x74
    0xA1, 0x53,           // Collection 0x53
    0x75, 0x08,           // report size = 8 bits
    0x15, 0x00,           // logical minimum = 0
    0x26, 0xFF, 0x00,     // logical maximum = 255
    0x95, CONSOLE_EPSIZE, // report count
    0x09, 0x75,           // usage
    0x81, 0x02,           // Input (array)
    0xC0                  // end collection
};
/* wrapper */
static const USBDescriptor console_hid_report_descriptor = {
    sizeof console_hid_report_desc_data,
    console_hid_report_desc_data};
#endif /* CONSOLE_ENABLE */

#ifdef EXTRAKEY_ENABLE
/* audio controls & system controls
 * http://www.microsoft.com/whdc/archive/w2kbd.mspx */
static const uint8_t extra_hid_report_desc_data[] = {
    /* system control */
    0x05,
    0x01, // USAGE_PAGE (Generic Desktop)
    0x09,
    0x80, // USAGE (System Control)
    0xa1,
    0x01, // COLLECTION (Application)
    0x85,
    REPORT_ID_SYSTEM, //   REPORT_ID (2)
    0x15,
    0x01, //   LOGICAL_MINIMUM (0x1)
    0x26, 0xb7,
    0x00, //   LOGICAL_MAXIMUM (0xb7)
    0x19,
    0x01, //   USAGE_MINIMUM (0x1)
    0x29,
    0xb7, //   USAGE_MAXIMUM (0xb7)
    0x75,
    0x10, //   REPORT_SIZE (16)
    0x95,
    0x01, //   REPORT_COUNT (1)
    0x81,
    0x00, //   INPUT (Data,Array,Abs)
    0xc0, // END_COLLECTION
    /* consumer */
    0x05,
    0x0c, // USAGE_PAGE (Consumer Devices)
    0x09,
    0x01, // USAGE (Consumer Control)
    0xa1,
    0x01, // COLLECTION (Application)
    0x85,
    
    // REPORT_ID_CONSUMER, //   REPORT_ID (3)
    // 0x15, 0x01,                 //   LOGICAL_MINIMUM (0x1)
    // 0x26, 0x9c, 0x02,           //   LOGICAL_MAXIMUM (0x29c)
    // 0x19, 0x01,                 //   USAGE_MINIMUM (0x1)
    // 0x2a, 0x9c, 0x02,           //   USAGE_MAXIMUM (0x29c)
    // 0x75, 0x10,                 //   REPORT_SIZE (16)
    // 0x95, 0x01,                 //   REPORT_COUNT (1)
    // 0x81, 0x00,                 //   INPUT (Data,Array,Abs)
    // 0xc0,                       // END_COLLECTION

    REPORT_ID_CONSUMER,         //   REPORT_ID (3)
    0x19, 0x01,                 //   Usage Minimum (Consumer Control)
    0x2A, 0xA0, 0x02,           //   Logical Minimum
    0x15, 0x01,                 //   USAGE_MINIMUM (0x1)
    0x26, 0xA0, 0x02,           //   USAGE_MAXIMUM (0x29c)
    0x95, 0x01,                 //   Report Count (1)
    0x75, 0x10,                 //   Report Size (16)
    0x81, 0x00,                 //   INPUT (Data,Array,Abs)
    0xc0,                       // END_COLLECTION    
    
    
    /* game pad*/
    //  //Corsair
    //  0x05,0x01,						//Usage Page (Generic Desktop)
    //  0x09,0x05,							//Usage (Game Pad)
    //  0xA1,0x01,						//Collection (Application)
    //  0x85,REPORT_ID_GAMEPAD,			//Report ID (4)
    //  0xA1,0x00,						//Collection (Physical)
    //  0x09,0x30,						//Usage (X)
    //  0x09,0x31,						//Usage (Y)
    //  0x15,0x00,						//Logical Minimum (0)
    //  0x26,0xFF,0xFF,					//Logical Maximum (-1)
    //  0x35,0x00,					//Physical Minimum (0)
    //  0x46,0xFF,0xFF,					//Physical Maximum (-1)
    //  0x95,0x02,						//Report Count (2)
    //  0x75,0x10,						//Report Size (16)
    //  0x81,0x02,						//Input  �� �蕭謕� �蕭豲� ��� ��� �� �� ����� ��  �蕭�� �� �����  �� �� �� �� ��  �蕭謕� �蕭豲 ��  �� �� �� �曹�� ����蕭�� �� ��  �� �蕭豯� ����  �蕭�� �� ��  �� �� ���� ��  ��� �  �� �� �� ��  �� �蕭謕� �蕭��  �蕭謍� �蕭��蕭��  �  �� �蕭�� ����� �� �� ����� �� ��� �� �揭  �� �蕭謕� �蕭���  ����  �蕭��迭���  �蕭謚啾 �����  ��� �  �� �蕭謢� ��� ���� �蕭�豱� �蕭  �� �� �� ����  �� �� �� ���� �� �� �蕭謕� �蕭豲� ����  ��  ���  �蕭��  ����  �蕭謖� �  �� �蕭謢� ���� ��  ��蕭�� �� �����  �蕭謖� �  �� �� �� ��  �� �蕭�貔� ���蕭�豱� �蕭  �� �� �� ����  �� �� �� ���� �� �� �蕭謕� �蕭豲� ����  ���  ��  �蕭��  �  ��� �  �船 ������  �蕭�� �� ��  �� ��� (Data,Var,Abs,NWrp,Lin,Pref,NNul,Bit)
    //  0xC0,						    //End Collection
    //  0xA1,0x00,						//Collection (Physical)
    //  0x09,0x33,						//Usage (Rx)
    //  0x09,0x34,						//Usage (Ry)
    //  0x15,0x00,						//Logical Minimum (0)
    //  0x26,0xFF,0xFF,					//Logical Maximum (-1)
    //  0x35,0x00,						//Physical Minimum (0)
    //  0x46,0xFF,0xFF,					//Physical Maximum (-1)
    //  0x95,0x02,						//Report Count (2)
    //  0x75,0x10,						//Report Size (16)
    //  0x81,0x02,						//Input  �� �蕭謕� �蕭豲� ��� ��� �� �� ����� ��  �蕭�� �� �����  �� �� �� �� ��  �蕭謕� �蕭豲 ��  �� �� �� �曹�� ����蕭�� �� ��  �� �蕭豯� ����  �蕭�� �� ��  �� �� ���� ��  ��� �  �� �� �� ��  �� �蕭謕� �蕭��  �蕭謍� �蕭��蕭��  �  �� �蕭�� ����� �� �� ����� �� ��� �� �揭  �� �蕭謕� �蕭���  ����  �蕭��迭���  �蕭謚啾 �����  ��� �  �� �蕭謢� ��� ���� �蕭�豱� �蕭  �� �� �� ����  �� �� �� ���� �� �� �蕭謕� �蕭豲� ����  ��  ���  �蕭��  ����  �蕭謖� �  �� �蕭謢� ���� ��  ��蕭�� �� �����  �蕭謖� �  �� �� �� ��  �� �蕭�貔� ���蕭�豱� �蕭  �� �� �� ����  �� �� �� ���� �� �� �蕭謕� �蕭豲� ����  ���  ��  �蕭��  �  ��� �  �船 ������  �蕭�� �� ��  �� ��� (Data,Var,Abs,NWrp,Lin,Pref,NNul,Bit)
    //  0xC0,						    //End Collection
    //  0xA1,0x00,						//Collection (Physical)
    //  0x09,0x32,						//Usage (Z)
    //  0x15,0x00,						//Logical Minimum (0)
    //  0x26,0xFF,0xFF,					//Logical Maximum (-1)
    //  0x35,0x00,						//Physical Minimum (0)
    //  0x46,0xFF,0xFF,					//Physical Maximum (-1)
    //  0x95,0x01,						//Report Count (1)
    //  0x75,0x10,						//Report Size (16)
    //  0x81,0x02,						//Input  �� �蕭謕� �蕭豲� ��� ��� �� �� ����� ��  �蕭�� �� �����  �� �� �� �� ��  �蕭謕� �蕭豲 ��  �� �� �� �曹�� ����蕭�� �� ��  �� �蕭豯� ����  �蕭�� �� ��  �� �� ���� ��  ��� �  �� �� �� ��  �� �蕭謕� �蕭��  �蕭謍� �蕭��蕭��  �  �� �蕭�� ����� �� �� ����� �� ��� �� �揭  �� �蕭謕� �蕭���  ����  �蕭��迭���  �蕭謚啾 �����  ��� �  �� �蕭謢� ��� ���� �蕭�豱� �蕭  �� �� �� ����  �� �� �� ���� �� �� �蕭謕� �蕭豲� ����  ��  ���  �蕭��  ����  �蕭謖� �  �� �蕭謢� ���� ��  ��蕭�� �� �����  �蕭謖� �  �� �� �� ��  �� �蕭�貔� ���蕭�豱� �蕭  �� �� �� ����  �� �� �� ���� �� �� �蕭謕� �蕭豲� ����  ���  ��  �蕭��  �  ��� �  �船 ������  �蕭�� �� ��  �� ��� (Data,Var,Abs,NWrp,Lin,Pref,NNul,Bit)
    //  0xC0,						    //End Collection
    //  0x05,0x09,						//Usage Page (Button)
    //  0x19,0x01,						//Usage Minimum (Button 1)
    //  0x29,0x0A,						//Usage Maximum (Button 10)
    //  0x95,0x0A,						//Report Count (10)
    //  0x75,0x01,						//Report Size (1)
    //  0x81,0x02,						//Input  �� �蕭謕� �蕭豲� ��� ��� �� �� ����� ��  �蕭�� �� �����  �� �� �� �� ��  �蕭謕� �蕭豲 ��  �� �� �� �曹�� ����蕭�� �� ��  �� �蕭豯� ����  �蕭�� �� ��  �� �� ���� ��  ��� �  �� �� �� ��  �� �蕭謕� �蕭��  �蕭謍� �蕭��蕭��  �  �� �蕭�� ����� �� �� ����� �� ��� �� �揭  �� �蕭謕� �蕭���  ����  �蕭��迭���  �蕭謚啾 �����  ��� �  �� �蕭謢� ��� ���� �蕭�豱� �蕭  �� �� �� ����  �� �� �� ���� �� �� �蕭謕� �蕭豲� ����  ��  ���  �蕭��  ����  �蕭謖� �  �� �蕭謢� ���� ��  ��蕭�� �� �����  �蕭謖� �  �� �� �� ��  �� �蕭�貔� ���蕭�豱� �蕭  �� �� �� ����  �� �� �� ���� �� �� �蕭謕� �蕭豲� ����  ���  ��  �蕭��  �  ��� �  �船 ������  �蕭�� �� ��  �� ��� (Data,Var,Abs,NWrp,Lin,Pref,NNul,Bit)
    //  0x05,0x01,						//Usage Page (Generic Desktop)
    //  0x09,0x39,						//Usage (Hat Switch)
    //  0x15,0x01,						//Logical Minimum (1)
    //  0x25,0x08,						//Logical Maximum (8)
    //  0x35,0x00,						//Physical Minimum (0)
    //  0x46,0x3B,0x10, 				//Physical Maximum (4155)
    //  0x66,0x0E,0x00, 					//Unit (None)
    //  0x75,0x04,						//Report Size (4)
    //  0x95,0x01,						//Report Count (1)
    //  0x81,0x42,						//Input  �� �蕭謕� �蕭豲� ��� ��� �� �� ����� ��  �蕭�� �� �����  �� �� �� �� ��  �蕭謕� �蕭豲 ��  �� �� �� �曹�� ����蕭�� �� ��  �� �蕭豯� ����  �蕭�� �� ��  �� �� ���� ��  ��� �  �� �� �� ��  �� �蕭謕� �蕭��  �蕭謍� �蕭��蕭��  �  �� �蕭�� ����� �� �� ����� �� ��� �� �揭  �� �蕭謕� �蕭���  ����  �蕭��迭���  �蕭謚啾 �����  ��� �  �� �蕭謢� ��� ���� �蕭�豱� �蕭  �� �� �� ����  �� �� �� ���� �� �� �蕭謕� �蕭豲� ����  ��  ���  �蕭��  ����  �蕭謖� �  �� �蕭謢� ���� ��  ��蕭�� �� �����  �蕭謖� �  �� �� �� ��  �� �蕭�貔� ���蕭�豱� �蕭  �� �� �� ����  �� �� �� ���� �� �� �蕭謕� �蕭豲� ����  ���  ��  �蕭��  �  ��� �  �船 ������  �蕭�� �� ��  �� ��� (Data,Var,Abs,NWrp,Lin,Pref,Null,Bit)
    //  0x75,0x02,						//Report Size (2)
    //  0x95,0x01,						//Report Count (1)
    //  0x81,0x03,						//Input  �� �蕭謕� �蕭豲� ��� ��� �� �� ����� ��  �蕭�� �� �����  �� �� �� �� ��  �蕭謕� �蕭豲 ��  �� �� �� �曹�� ����蕭�� �� ��  �� �蕭豯� ����  �蕭�� �� ��  �� �� ���� ��  ��� �  �� �� �� ��  �� �蕭謕� �蕭��  �蕭謍� �蕭��蕭��  �  �� �蕭�� ����� �� �� ����� �� ��� �� �揭  �� �蕭謕� �蕭���  ����  �蕭��迭���  �蕭謚啾 �����  ��� �  �� �蕭謢� ��� ���� �蕭�豱� �蕭  �� �� �� ����  �� �� �� ���� �� �� �蕭謕� �蕭豲� ����  ��  ���  �蕭��  ����  �蕭謖� �  �� �蕭謢� ���� ��  ��蕭�� �� �����  �蕭謖� �  �� �� �� ��  �� �蕭�貔� ���蕭�豱� �蕭  �� �� �� ����  �� �� �� ���� �� �� �蕭謕� �蕭豲� ����  ���  ��  �蕭��  �  ��� �  �船 ������  �蕭�� �� ��  �� ��� (Cnst,Var,Abs,NWrp,Lin,Pref,NNul,Bit)
    //  0x75,0x08,						//Report Size (8)
    //  0x95,0x02,						//Report Count (2)
    //  0x81,0x03,						//Input  �� �蕭謕� �蕭豲� ��� ��� �� �� ����� ��  �蕭�� �� �����  �� �� �� �� ��  �蕭謕� �蕭豲 ��  �� �� �� �曹�� ����蕭�� �� ��  �� �蕭豯� ����  �蕭�� �� ��  �� �� ���� ��  ��� �  �� �� �� ��  �� �蕭謕� �蕭��  �蕭謍� �蕭��蕭��  �  �� �蕭�� ����� �� �� ����� �� ��� �� �揭  �� �蕭謕� �蕭���  ����  �蕭��迭���  �蕭謚啾 �����  ��� �  �� �蕭謢� ��� ���� �蕭�豱� �蕭  �� �� �� ����  �� �� �� ���� �� �� �蕭謕� �蕭豲� ����  ��  ���  �蕭��  ����  �蕭謖� �  �� �蕭謢� ���� ��  ��蕭�� �� �����  �蕭謖� �  �� �� �� ��  �� �蕭�貔� ���蕭�豱� �蕭  �� �� �� ����  �� �� �� ���� �� �� �蕭謕� �蕭豲� ����  ���  ��  �蕭��  �  ��� �  �船 ������  �蕭�� �� ��  �� ��� (Cnst,Var,Abs,NWrp,Lin,Pref,NNul,Bit)
    //  0xC0,         		      //End Collection

    /* game pad*/
    //  //Corsair
    //  0x05,0x01,						//Usage Page (Generic Desktop)
    //  0x09,0x05,							//Usage (Game Pad)
    //  0xA1,0x01,						//Collection (Application)
    //  0x85,REPORT_ID_GAMEPAD,			//Report ID (4)
    //  0xA1,0x00,						//Collection (Physical)
    //  0x09,0x30,						//Usage (X)
    //  0x09,0x31,						//Usage (Y)
    //  0x15,0x00,						//Logical Minimum (0)
    //  0x26,0xFF,0xFF,					//Logical Maximum (-1)
    //  0x35,0x00,					//Physical Minimum (0)
    //  0x46,0xFF,0xFF,					//Physical Maximum (-1)
    //  0x95,0x02,						//Report Count (2)
    //  0x75,0x10,						//Report Size (16)
    //  0x81,0x02,						//Input  �� �蕭謕� �蕭豲� ��� ��� �� �� ����� ��  �蕭�� �� �����  �� �� �� �� ��  �蕭謕� �蕭豲 ��  �� �� �� �曹�� ����蕭�� �� ��  �� �蕭豯� ����  �蕭�� �� ��  �� �� ���� ��  ��� �  �� �� �� ��  �� �蕭謕� �蕭��  �蕭謍� �蕭��蕭��  �  �� �蕭�� ����� �� �� ����� �� ��� �� �揭  �� �蕭謕� �蕭���  ����  �蕭��迭���  �蕭謚啾 �����  ��� �  �� �蕭謢� ��� ���� �蕭�豱� �蕭  �� �� �� ����  �� �� �� ���� �� �� �蕭謕� �蕭豲� ����  ��  ���  �蕭��  ����  �蕭謖� �  �� �蕭謢� ���� ��  ��蕭�� �� �����  �蕭謖� �  �� �� �� ��  �� �蕭�貔� ���蕭�豱� �蕭  �� �� �� ����  �� �� �� ���� �� �� �蕭謕� �蕭豲� ����  ���  ��  �蕭��  �  ��� �  �船 ������  �蕭�� �� ��  �� ��� (Data,Var,Abs,NWrp,Lin,Pref,NNul,Bit)
    //  0xC0,						    //End Collection
    //  0xA1,0x00,						//Collection (Physical)
    //  0x09,0x33,						//Usage (Rx)
    //  0x09,0x34,						//Usage (Ry)
    //  0x15,0x00,						//Logical Minimum (0)
    //  0x26,0xFF,0xFF,					//Logical Maximum (-1)
    //  0x35,0x00,						//Physical Minimum (0)
    //  0x46,0xFF,0xFF,					//Physical Maximum (-1)
    //  0x95,0x02,						//Report Count (2)
    //  0x75,0x10,						//Report Size (16)
    //  0x81,0x02,						//Input  �� �蕭謕� �蕭豲� ��� ��� �� �� ����� ��  �蕭�� �� �����  �� �� �� �� ��  �蕭謕� �蕭豲 ��  �� �� �� �曹�� ����蕭�� �� ��  �� �蕭豯� ����  �蕭�� �� ��  �� �� ���� ��  ��� �  �� �� �� ��  �� �蕭謕� �蕭��  �蕭謍� �蕭��蕭��  �  �� �蕭�� ����� �� �� ����� �� ��� �� �揭  �� �蕭謕� �蕭���  ����  �蕭��迭���  �蕭謚啾 �����  ��� �  �� �蕭謢� ��� ���� �蕭�豱� �蕭  �� �� �� ����  �� �� �� ���� �� �� �蕭謕� �蕭豲� ����  ��  ���  �蕭��  ����  �蕭謖� �  �� �蕭謢� ���� ��  ��蕭�� �� �����  �蕭謖� �  �� �� �� ��  �� �蕭�貔� ���蕭�豱� �蕭  �� �� �� ����  �� �� �� ���� �� �� �蕭謕� �蕭豲� ����  ���  ��  �蕭��  �  ��� �  �船 ������  �蕭�� �� ��  �� ��� (Data,Var,Abs,NWrp,Lin,Pref,NNul,Bit)
    //  0xC0,						    //End Collection
    //  0xA1,0x00,						//Collection (Physical)
    //  0x09,0x32,						//Usage (Z)
    //  0x15,0x00,						//Logical Minimum (0)
    //  0x26,0xFF,0xFF,					//Logical Maximum (-1)
    //  0x35,0x00,						//Physical Minimum (0)
    //  0x46,0xFF,0xFF,					//Physical Maximum (-1)
    //  0x95,0x01,						//Report Count (1)
    //  0x75,0x10,						//Report Size (16)
    //  0x81,0x02,						//Input  �� �蕭謕� �蕭豲� ��� ��� �� �� ����� ��  �蕭�� �� �����  �� �� �� �� ��  �蕭謕� �蕭豲 ��  �� �� �� �曹�� ����蕭�� �� ��  �� �蕭豯� ����  �蕭�� �� ��  �� �� ���� ��  ��� �  �� �� �� ��  �� �蕭謕� �蕭��  �蕭謍� �蕭��蕭��  �  �� �蕭�� ����� �� �� ����� �� ��� �� �揭  �� �蕭謕� �蕭���  ����  �蕭��迭���  �蕭謚啾 �����  ��� �  �� �蕭謢� ��� ���� �蕭�豱� �蕭  �� �� �� ����  �� �� �� ���� �� �� �蕭謕� �蕭豲� ����  ��  ���  �蕭��  ����  �蕭謖� �  �� �蕭謢� ���� ��  ��蕭�� �� �����  �蕭謖� �  �� �� �� ��  �� �蕭�貔� ���蕭�豱� �蕭  �� �� �� ����  �� �� �� ���� �� �� �蕭謕� �蕭豲� ����  ���  ��  �蕭��  �  ��� �  �船 ������  �蕭�� �� ��  �� ��� (Data,Var,Abs,NWrp,Lin,Pref,NNul,Bit)
    //  0xC0,						    //End Collection
    //  0x05,0x09,						//Usage Page (Button)
    //  0x19,0x01,						//Usage Minimum (Button 1)
    //  0x29,0x0A,						//Usage Maximum (Button 10)
    //  0x95,0x0A,						//Report Count (10)
    //  0x75,0x01,						//Report Size (1)
    //  0x81,0x02,						//Input  �� �蕭謕� �蕭豲� ��� ��� �� �� ����� ��  �蕭�� �� �����  �� �� �� �� ��  �蕭謕� �蕭豲 ��  �� �� �� �曹�� ����蕭�� �� ��  �� �蕭豯� ����  �蕭�� �� ��  �� �� ���� ��  ��� �  �� �� �� ��  �� �蕭謕� �蕭��  �蕭謍� �蕭��蕭��  �  �� �蕭�� ����� �� �� ����� �� ��� �� �揭  �� �蕭謕� �蕭���  ����  �蕭��迭���  �蕭謚啾 �����  ��� �  �� �蕭謢� ��� ���� �蕭�豱� �蕭  �� �� �� ����  �� �� �� ���� �� �� �蕭謕� �蕭豲� ����  ��  ���  �蕭��  ����  �蕭謖� �  �� �蕭謢� ���� ��  ��蕭�� �� �����  �蕭謖� �  �� �� �� ��  �� �蕭�貔� ���蕭�豱� �蕭  �� �� �� ����  �� �� �� ���� �� �� �蕭謕� �蕭豲� ����  ���  ��  �蕭��  �  ��� �  �船 ������  �蕭�� �� ��  �� ��� (Data,Var,Abs,NWrp,Lin,Pref,NNul,Bit)
    //  0x05,0x01,						//Usage Page (Generic Desktop)
    //  0x09,0x39,						//Usage (Hat Switch)
    //  0x15,0x01,						//Logical Minimum (1)
    //  0x25,0x08,						//Logical Maximum (8)
    //  0x35,0x00,						//Physical Minimum (0)
    //  0x46,0x3B,0x10, 				//Physical Maximum (4155)
    //  0x66,0x0E,0x00, 					//Unit (None)
    //  0x75,0x04,						//Report Size (4)
    //  0x95,0x01,						//Report Count (1)
    //  0x81,0x42,						//Input  �� �蕭謕� �蕭豲� ��� ��� �� �� ����� ��  �蕭�� �� �����  �� �� �� �� ��  �蕭謕� �蕭豲 ��  �� �� �� �曹�� ����蕭�� �� ��  �� �蕭豯� ����  �蕭�� �� ��  �� �� ���� ��  ��� �  �� �� �� ��  �� �蕭謕� �蕭��  �蕭謍� �蕭��蕭��  �  �� �蕭�� ����� �� �� ����� �� ��� �� �揭  �� �蕭謕� �蕭���  ����  �蕭��迭���  �蕭謚啾 �����  ��� �  �� �蕭謢� ��� ���� �蕭�豱� �蕭  �� �� �� ����  �� �� �� ���� �� �� �蕭謕� �蕭豲� ����  ��  ���  �蕭��  ����  �蕭謖� �  �� �蕭謢� ���� ��  ��蕭�� �� �����  �蕭謖� �  �� �� �� ��  �� �蕭�貔� ���蕭�豱� �蕭  �� �� �� ����  �� �� �� ���� �� �� �蕭謕� �蕭豲� ����  ���  ��  �蕭��  �  ��� �  �船 ������  �蕭�� �� ��  �� ��� (Data,Var,Abs,NWrp,Lin,Pref,Null,Bit)
    //  0x75,0x02,						//Report Size (2)
    //  0x95,0x01,						//Report Count (1)
    //  0x81,0x03,						//Input  �� �蕭謕� �蕭豲� ��� ��� �� �� ����� ��  �蕭�� �� �����  �� �� �� �� ��  �蕭謕� �蕭豲 ��  �� �� �� �曹�� ����蕭�� �� ��  �� �蕭豯� ����  �蕭�� �� ��  �� �� ���� ��  ��� �  �� �� �� ��  �� �蕭謕� �蕭��  �蕭謍� �蕭��蕭��  �  �� �蕭�� ����� �� �� ����� �� ��� �� �揭  �� �蕭謕� �蕭���  ����  �蕭��迭���  �蕭謚啾 �����  ��� �  �� �蕭謢� ��� ���� �蕭�豱� �蕭  �� �� �� ����  �� �� �� ���� �� �� �蕭謕� �蕭豲� ����  ��  ���  �蕭��  ����  �蕭謖� �  �� �蕭謢� ���� ��  ��蕭�� �� �����  �蕭謖� �  �� �� �� ��  �� �蕭�貔� ���蕭�豱� �蕭  �� �� �� ����  �� �� �� ���� �� �� �蕭謕� �蕭豲� ����  ���  ��  �蕭��  �  ��� �  �船 ������  �蕭�� �� ��  �� ��� (Cnst,Var,Abs,NWrp,Lin,Pref,NNul,Bit)
    //  0x75,0x08,						//Report Size (8)
    //  0x95,0x02,						//Report Count (2)
    //  0x81,0x03,						//Input  �� �蕭謕� �蕭豲� ��� ��� �� �� ����� ��  �蕭�� �� �����  �� �� �� �� ��  �蕭謕� �蕭豲 ��  �� �� �� �曹�� ����蕭�� �� ��  �� �蕭豯� ����  �蕭�� �� ��  �� �� ���� ��  ��� �  �� �� �� ��  �� �蕭謕� �蕭��  �蕭謍� �蕭��蕭��  �  �� �蕭�� ����� �� �� ����� �� ��� �� �揭  �� �蕭謕� �蕭���  ����  �蕭��迭���  �蕭謚啾 �����  ��� �  �� �蕭謢� ��� ���� �蕭�豱� �蕭  �� �� �� ����  �� �� �� ���� �� �� �蕭謕� �蕭豲� ����  ��  ���  �蕭��  ����  �蕭謖� �  �� �蕭謢� ���� ��  ��蕭�� �� �����  �蕭謖� �  �� �� �� ��  �� �蕭�貔� ���蕭�豱� �蕭  �� �� �� ����  �� �� �� ���� �� �� �蕭謕� �蕭豲� ����  ���  ��  �蕭��  �  ��� �  �船 ������  �蕭�� �� ��  �� ��� (Cnst,Var,Abs,NWrp,Lin,Pref,NNul,Bit)
    //  0xC0,         		      //End Collection

    // PS5 DualSense
    //   0x05, 0x01,        // Usage Page (Generic Desktop Ctrls)
    //   0x09, 0x05,        // Usage (Game Pad)
    //   0xA1, 0x01,        // Collection (Application)
    //   0x85, 0x01,        //   Report ID (1)
    //   0x09, 0x30,        //   Usage (X)
    //   0x09, 0x31,        //   Usage (Y)
    //   0x09, 0x32,        //   Usage (Z)
    //   0x09, 0x35,        //   Usage (Rz)
    //   0x15, 0x00,        //   Logical Minimum (0)
    //   0x26, 0xFF, 0x00,  //   Logical Maximum (255)
    //   0x75, 0x08,        //   Report Size (8)
    //   0x95, 0x04,        //   Report Count (4)
    //   0x81, 0x02,        //   Input (Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position)
    //   0x09, 0x39,        //   Usage (Hat switch)
    //   0x15, 0x00,        //   Logical Minimum (0)
    //   0x25, 0x07,        //   Logical Maximum (7)
    //   0x35, 0x00,        //   Physical Minimum (0)
    //   0x46, 0x3B, 0x01,  //   Physical Maximum (315)
    //   0x65, 0x14,        //   Unit (System: English Rotation, Length: Centimeter)
    //   0x75, 0x04,        //   Report Size (4)
    //   0x95, 0x01,        //   Report Count (1)
    //   0x81, 0x42,        //   Input (Data,Var,Abs,No Wrap,Linear,Preferred State,Null State)
    //   0x65, 0x00,        //   Unit (None)
    //   0x05, 0x09,        //   Usage Page (Button)
    //   0x19, 0x01,        //   Usage Minimum (0x01)
    //   0x29, 0x0E,        //   Usage Maximum (0x0E)
    //   0x15, 0x00,        //   Logical Minimum (0)
    //   0x25, 0x01,        //   Logical Maximum (1)
    //   0x75, 0x01,        //   Report Size (1)
    //   0x95, 0x0E,        //   Report Count (14)
    //   0x81, 0x02,        //   Input (Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position)
    //   0x06, 0x00, 0xFF,  //   Usage Page (Vendor Defined 0xFF00)
    //   0x09, 0x20,        //   Usage (0x20)
    //   0x75, 0x06,        //   Report Size (6)
    //   0x95, 0x01,        //   Report Count (1)
    //   0x15, 0x00,        //   Logical Minimum (0)
    //   0x25, 0x7F,        //   Logical Maximum (127)
    //   0x81, 0x02,        //   Input (Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position)
    //   0x05, 0x01,        //   Usage Page (Generic Desktop Ctrls)
    //   0x09, 0x33,        //   Usage (Rx)
    //   0x09, 0x34,        //   Usage (Ry)
    //   0x15, 0x00,        //   Logical Minimum (0)
    //   0x26, 0xFF, 0x00,  //   Logical Maximum (255)
    //   0x75, 0x08,        //   Report Size (8)
    //   0x95, 0x02,        //   Report Count (2)
    //   0x81, 0x02,        //   Input (Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position)
    //   0x06, 0x00, 0xFF,  //   Usage Page (Vendor Defined 0xFF00)
    //   0x09, 0x21,        //   Usage (0x21)
    //   0x95, 0x36,        //   Report Count (54)
    //   0x81, 0x02,        //   Input (Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position)
    //   0x85, 0x05,        //   Report ID (5)
    //   0x09, 0x22,        //   Usage (0x22)
    //   0x95, 0x1F,        //   Report Count (31)
    //   0x91, 0x02,        //   Output (Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position,Non-volatile)
    //   0x85, 0x04,        //   Report ID (4)
    //   0x09, 0x23,        //   Usage (0x23)
    //   0x95, 0x24,        //   Report Count (36)
    //   0xB1, 0x02,        //   Feature (Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position,Non-volatile)
    //   0x85, 0x02,        //   Report ID (2)
    //   0x09, 0x24,        //   Usage (0x24)
    //   0x95, 0x24,        //   Report Count (36)
    //   0xB1, 0x02,        //   Feature (Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position,Non-volatile)
    //   0x85, 0x08,        //   Report ID (8)
    //   0x09, 0x25,        //   Usage (0x25)
    //   0x95, 0x03,        //   Report Count (3)
    //   0xB1, 0x02,        //   Feature (Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position,Non-volatile)
    //   0x85, 0x10,        //   Report ID (16)
    //   0x09, 0x26,        //   Usage (0x26)
    //   0x95, 0x04,        //   Report Count (4)
    //   0xB1, 0x02,        //   Feature (Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position,Non-volatile)
    //   0x85, 0x11,        //   Report ID (17)
    //   0x09, 0x27,        //   Usage (0x27)
    //   0x95, 0x02,        //   Report Count (2)
    //   0xB1, 0x02,        //   Feature (Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position,Non-volatile)
    //   0x85, 0x12,        //   Report ID (18)
    //   0x06, 0x02, 0xFF,  //   Usage Page (Vendor Defined 0xFF02)
    //   0x09, 0x21,        //   Usage (0x21)
    //   0x95, 0x0F,        //   Report Count (15)
    //   0xB1, 0x02,        //   Feature (Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position,Non-volatile)
    //   0x85, 0x13,        //   Report ID (19)
    //   0x09, 0x22,        //   Usage (0x22)
    //   0x95, 0x16,        //   Report Count (22)
    //   0xB1, 0x02,        //   Feature (Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position,Non-volatile)
    //   0x85, 0x14,        //   Report ID (20)
    //   0x06, 0x05, 0xFF,  //   Usage Page (Vendor Defined 0xFF05)
    //   0x09, 0x20,        //   Usage (0x20)
    //   0x95, 0x10,        //   Report Count (16)
    //   0xB1, 0x02,        //   Feature (Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position,Non-volatile)
    //   0x85, 0x15,        //   Report ID (21)
    //   0x09, 0x21,        //   Usage (0x21)
    //   0x95, 0x2C,        //   Report Count (44)
    //   0xB1, 0x02,        //   Feature (Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position,Non-volatile)
    //   0x06, 0x80, 0xFF,  //   Usage Page (Vendor Defined 0xFF80)
    //   0x85, 0x80,        //   Report ID (128)
    //   0x09, 0x20,        //   Usage (0x20)
    //   0x95, 0x06,        //   Report Count (6)
    //   0xB1, 0x02,        //   Feature (Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position,Non-volatile)
    //   0x85, 0x81,        //   Report ID (129)
    //   0x09, 0x21,        //   Usage (0x21)
    //   0x95, 0x06,        //   Report Count (6)
    //   0xB1, 0x02,        //   Feature (Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position,Non-volatile)
    //   0x85, 0x82,        //   Report ID (130)
    //   0x09, 0x22,        //   Usage (0x22)
    //   0x95, 0x05,        //   Report Count (5)
    //   0xB1, 0x02,        //   Feature (Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position,Non-volatile)
    //   0x85, 0x83,        //   Report ID (131)
    //   0x09, 0x23,        //   Usage (0x23)
    //   0x95, 0x01,        //   Report Count (1)
    //   0xB1, 0x02,        //   Feature (Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position,Non-volatile)
    //   0x85, 0x84,        //   Report ID (132)
    //   0x09, 0x24,        //   Usage (0x24)
    //   0x95, 0x04,        //   Report Count (4)
    //   0xB1, 0x02,        //   Feature (Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position,Non-volatile)
    //   0x85, 0x85,        //   Report ID (133)
    //   0x09, 0x25,        //   Usage (0x25)
    //   0x95, 0x06,        //   Report Count (6)
    //   0xB1, 0x02,        //   Feature (Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position,Non-volatile)
    //   0x85, 0x86,        //   Report ID (134)
    //   0x09, 0x26,        //   Usage (0x26)
    //   0x95, 0x06,        //   Report Count (6)
    //   0xB1, 0x02,        //   Feature (Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position,Non-volatile)
    //   0x85, 0x87,        //   Report ID (135)
    //   0x09, 0x27,        //   Usage (0x27)
    //   0x95, 0x23,        //   Report Count (35)
    //   0xB1, 0x02,        //   Feature (Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position,Non-volatile)
    //   0x85, 0x88,        //   Report ID (136)
    //   0x09, 0x28,        //   Usage (0x28)
    //   0x95, 0x22,        //   Report Count (34)
    //   0xB1, 0x02,        //   Feature (Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position,Non-volatile)
    //   0x85, 0x89,        //   Report ID (137)
    //   0x09, 0x29,        //   Usage (0x29)
    //   0x95, 0x02,        //   Report Count (2)
    //   0xB1, 0x02,        //   Feature (Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position,Non-volatile)
    //   0x85, 0x90,        //   Report ID (144)
    //   0x09, 0x30,        //   Usage (0x30)
    //   0x95, 0x05,        //   Report Count (5)
    //   0xB1, 0x02,        //   Feature (Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position,Non-volatile)
    //   0x85, 0x91,        //   Report ID (145)
    //   0x09, 0x31,        //   Usage (0x31)
    //   0x95, 0x03,        //   Report Count (3)
    //   0xB1, 0x02,        //   Feature (Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position,Non-volatile)
    //   0x85, 0x92,        //   Report ID (146)
    //   0x09, 0x32,        //   Usage (0x32)
    //   0x95, 0x03,        //   Report Count (3)
    //   0xB1, 0x02,        //   Feature (Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position,Non-volatile)
    //   0x85, 0x93,        //   Report ID (147)
    //   0x09, 0x33,        //   Usage (0x33)
    //   0x95, 0x0C,        //   Report Count (12)
    //   0xB1, 0x02,        //   Feature (Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position,Non-volatile)
    //   0x85, 0xA0,        //   Report ID (160)
    //   0x09, 0x40,        //   Usage (0x40)
    //   0x95, 0x06,        //   Report Count (6)
    //   0xB1, 0x02,        //   Feature (Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position,Non-volatile)
    //   0x85, 0xA1,        //   Report ID (161)
    //   0x09, 0x41,        //   Usage (0x41)
    //   0x95, 0x01,        //   Report Count (1)
    //   0xB1, 0x02,        //   Feature (Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position,Non-volatile)
    //   0x85, 0xA2,        //   Report ID (162)
    //   0x09, 0x42,        //   Usage (0x42)
    //   0x95, 0x01,        //   Report Count (1)
    //   0xB1, 0x02,        //   Feature (Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position,Non-volatile)
    //   0x85, 0xA3,        //   Report ID (163)
    //   0x09, 0x43,        //   Usage (0x43)
    //   0x95, 0x30,        //   Report Count (48)
    //   0xB1, 0x02,        //   Feature (Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position,Non-volatile)
    //   0x85, 0xA4,        //   Report ID (164)
    //   0x09, 0x44,        //   Usage (0x44)
    //   0x95, 0x0D,        //   Report Count (13)
    //   0xB1, 0x02,        //   Feature (Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position,Non-volatile)
    //   0x85, 0xA5,        //   Report ID (165)
    //   0x09, 0x45,        //   Usage (0x45)
    //   0x95, 0x15,        //   Report Count (21)
    //   0xB1, 0x02,        //   Feature (Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position,Non-volatile)
    //   0x85, 0xA6,        //   Report ID (166)
    //   0x09, 0x46,        //   Usage (0x46)
    //   0x95, 0x15,        //   Report Count (21)
    //   0xB1, 0x02,        //   Feature (Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position,Non-volatile)
    //   0x85, 0xF0,        //   Report ID (240)
    //   0x09, 0x47,        //   Usage (0x47)
    //   0x95, 0x3F,        //   Report Count (63)
    //   0xB1, 0x02,        //   Feature (Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position,Non-volatile)
    //   0x85, 0xF1,        //   Report ID (241)
    //   0x09, 0x48,        //   Usage (0x48)
    //   0x95, 0x3F,        //   Report Count (63)
    //   0xB1, 0x02,        //   Feature (Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position,Non-volatile)
    //   0x85, 0xF2,        //   Report ID (242)
    //   0x09, 0x49,        //   Usage (0x49)
    //   0x95, 0x0F,        //   Report Count (15)
    //   0xB1, 0x02,        //   Feature (Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position,Non-volatile)
    //   0x85, 0xA7,        //   Report ID (247)
    //   0x09, 0x4A,        //   Usage (0x4A)
    //   0x95, 0x01,        //   Report Count (1)
    //   0xB1, 0x02,        //   Feature (Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position,Non-volatile)
    //   0x85, 0xA8,        //   Report ID (250)
    //   0x09, 0x4B,        //   Usage (0x4B)
    //   0x95, 0x01,        //   Report Count (1)
    //   0xB1, 0x02,        //   Feature (Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position,Non-volatile)
    //   0x85, 0xA9,        //   Report ID (251)
    //   0x09, 0x4C,        //   Usage (0x4C)
    //   0x95, 0x08,        //   Report Count (8)
    //   0xB1, 0x02,        //   Feature (Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position,Non-volatile)
    //   0x85, 0xAA,        //   Report ID (252)
    //   0x09, 0x4E,        //   Usage (0x4E)
    //   0x95, 0x01,        //   Report Count (1)
    //   0xB1, 0x02,        //   Feature (Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position,Non-volatile)
    //   0x85, 0xAB,        //   Report ID (253)
    //   0x09, 0x4F,        //   Usage (0x4F)
    //   0x95, 0x39,        //   Report Count (57)
    //   0xB1, 0x02,        //   Feature (Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position,Non-volatile)
    //   0x85, 0xAC,        //   Report ID (254)
    //   0x09, 0x50,        //   Usage (0x50)
    //   0x95, 0x39,        //   Report Count (57)
    //   0xB1, 0x02,        //   Feature (Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position,Non-volatile)
    //   0x85, 0xAD,        //   Report ID (255)
    //   0x09, 0x51,        //   Usage (0x51)
    //   0x95, 0x0B,        //   Report Count (11)
    //   0xB1, 0x02,        //   Feature (Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position,Non-volatile)
    //   0x85, 0xAE,        //   Report ID (256)
    //   0x09, 0x52,        //   Usage (0x52)
    //   0x95, 0x01,        //   Report Count (1)
    //   0xB1, 0x02,        //   Feature (Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position,Non-volatile)
    //   0x85, 0xAF,        //   Report ID (175)
    //   0x09, 0x53,        //   Usage (0x53)
    //   0x95, 0x02,        //   Report Count (2)
    //   0xB1, 0x02,        //   Feature (Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position,Non-volatile)
    //   0x85, 0xB0,        //   Report ID (176)
    //   0x09, 0x54,        //   Usage (0x54)
    //   0x95, 0x3F,        //   Report Count (63)
    //   0xB1, 0x02,        //   Feature (Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position,Non-volatile)
    //   0xC0,

    //  0x05,0x01,						//Usage Page (Generic Desktop)
    //  0x09,0x05,							//Usage (Game Pad)
    //  0xA1,0x01,						//Collection (Application)
    //  0x85,REPORT_ID_GAMEPAD,			//Report ID (4)

    // XBOX360 XboxOne
    0x05,
    0x01, //(GLOBAL) USAGE_PAGE         0x0001 Generic Desktop Page
    0x09,
    0x05, //(LOCAL)  USAGE              0x00010005 Game Pad (Application Collection)
    0xA1,
    0x01, //(MAIN)   COLLECTION         0x01 Application (Usage=0x00010005: Page=Generic Desktop Page, Usage=Game Pad, Type=Application Collection)
    0xA1,
    0x00, //(MAIN)   COLLECTION         0x00 Physical (Usage=0x0: Page=, Usage=, Type=) <-- Error: COLLECTION must be preceded by a USAGE <-- Warning: USAGE type should be CP (Physical Collection)
    0x85,
    REPORT_ID_GAMEPAD, // Report ID (4)
    0x09,
    0x30, //(LOCAL)  USAGE              0x00010030 X (Dynamic Value)
    0x09,
    0x31, //(LOCAL)  USAGE              0x00010031 Y (Dynamic Value)
    0x15,
    0x00, //(GLOBAL) LOGICAL_MINIMUM    0x00 (0)  <-- Info: Consider replacing 15 00 with 14
    0x27,
    0xB0,
    0x04,
    0x00,
    0x00, //(GLOBAL) LOGICAL_MAXIMUM    0x0000FFFF (65535)
    0x95,
    0x02, //(GLOBAL) REPORT_COUNT       0x02 (2) Number of fields
    0x75,
    0x10, //(GLOBAL) REPORT_SIZE        0x10 (16) Number of bits per field
    0x81,
    0x02, //(MAIN)   INPUT              0x00000002 (2 fields x 16 bits) 0=Data 1=Variable 0=Absolute 0=NoWrap 0=Linear 0=PrefState 0=NoNull 0=NonVolatile 0=Bitmap
    0xC0, //(MAIN)   END_COLLECTION     Physical
    0xA1,
    0x00, //(MAIN)   COLLECTION         0x00 Physical (Usage=0x0: Page=, Usage=, Type=) <-- Error: COLLECTION must be preceded by a USAGE <-- Warning: USAGE type should be CP (Physical Collection)
    0x09,
    0x33, //(LOCAL)  USAGE              0x00010033 Rx (Dynamic Value)
    0x09,
    0x34, //(LOCAL)  USAGE              0x00010034 Ry (Dynamic Value)
    0x15,
    0x00, //(GLOBAL) LOGICAL_MINIMUM    0x00 (0) <-- Redundant: LOGICAL_MINIMUM is already 0 <-- Info: Consider replacing 15 00 with 14
    0x27,
    0xB0,
    0x04,
    0x00,
    0x00, //(GLOBAL) LOGICAL_MAXIMUM    0x0000FFFF (65535) <-- Redundant: LOGICAL_MAXIMUM is already 65535
    0x95,
    0x02, //(GLOBAL) REPORT_COUNT       0x02 (2) Number of fields <-- Redundant: REPORT_COUNT is already 2
    0x75,
    0x10, //(GLOBAL) REPORT_SIZE        0x10 (16) Number of bits per field <-- Redundant: REPORT_SIZE is already 16
    0x81,
    0x02, //(MAIN)   INPUT              0x00000002 (2 fields x 16 bits) 0=Data 1=Variable 0=Absolute 0=NoWrap 0=Linear 0=PrefState 0=NoNull 0=NonVolatile 0=Bitmap
    0xC0, //(MAIN)   END_COLLECTION     Physical
    0x05,
    0x01, //(GLOBAL) USAGE_PAGE         0x0001 Generic Desktop Page <-- Redundant: USAGE_PAGE is already 0x0001
    0x09,
    0x32, //(LOCAL)  USAGE              0x00010032 Z (Dynamic Value)
    0x15,
    0x00, //(GLOBAL) LOGICAL_MINIMUM    0x00 (0) <-- Redundant: LOGICAL_MINIMUM is already 0 <-- Info: Consider replacing 15 00 with 14
    0x26,
    0xFF,
    0x03, //(GLOBAL) LOGICAL_MAXIMUM    0x03FF (1023)
    0x95,
    0x01, //(GLOBAL) REPORT_COUNT       0x01 (1) Number of fields
    0x75,
    0x0A, //(GLOBAL) REPORT_SIZE        0x0A (10) Number of bits per field
    0x81,
    0x02, //(MAIN)   INPUT              0x00000002 (1 field x 10 bits) 0=Data 1=Variable 0=Absolute 0=NoWrap 0=Linear 0=PrefState 0=NoNull 0=NonVolatile 0=Bitmap
    0x15,
    0x00, //(GLOBAL) LOGICAL_MINIMUM    0x00 (0) <-- Redundant: LOGICAL_MINIMUM is already 0 <-- Info: Consider replacing 15 00 with 14
    0x25,
    0x00, //(GLOBAL) LOGICAL_MAXIMUM    0x00 (0)  <-- Info: Consider replacing 25 00 with 24
    0x75,
    0x06, //(GLOBAL) REPORT_SIZE        0x06 (6) Number of bits per field
    0x95,
    0x01, //(GLOBAL) REPORT_COUNT       0x01 (1) Number of fields <-- Redundant: REPORT_COUNT is already 1
    0x81,
    0x03, //(MAIN)   INPUT              0x00000003 (1 field x 6 bits) 1=Constant 1=Variable 0=Absolute 0=NoWrap 0=Linear 0=PrefState 0=NoNull 0=NonVolatile 0=Bitmap
    0x05,
    0x01, //(GLOBAL) USAGE_PAGE         0x0001 Generic Desktop Page <-- Redundant: USAGE_PAGE is already 0x0001
    0x09,
    0x35, //(LOCAL)  USAGE              0x00010035 Rz (Dynamic Value)
    0x15,
    0x00, //(GLOBAL) LOGICAL_MINIMUM    0x00 (0) <-- Redundant: LOGICAL_MINIMUM is already 0 <-- Info: Consider replacing 15 00 with 14
    0x26,
    0xFF,
    0x03, //(GLOBAL) LOGICAL_MAXIMUM    0x03FF (1023)
    0x95,
    0x01, //(GLOBAL) REPORT_COUNT       0x01 (1) Number of fields <-- Redundant: REPORT_COUNT is already 1
    0x75,
    0x0A, //(GLOBAL) REPORT_SIZE        0x0A (10) Number of bits per field
    0x81,
    0x02, //(MAIN)   INPUT              0x00000002 (1 field x 10 bits) 0=Data 1=Variable 0=Absolute 0=NoWrap 0=Linear 0=PrefState 0=NoNull 0=NonVolatile 0=Bitmap
    0x15,
    0x00, //(GLOBAL) LOGICAL_MINIMUM    0x00 (0) <-- Redundant: LOGICAL_MINIMUM is already 0 <-- Info: Consider replacing 15 00 with 14
    0x25,
    0x00, //(GLOBAL) LOGICAL_MAXIMUM    0x00 (0)  <-- Info: Consider replacing 25 00 with 24
    0x75,
    0x06, //(GLOBAL) REPORT_SIZE        0x06 (6) Number of bits per field
    0x95,
    0x01, //(GLOBAL) REPORT_COUNT       0x01 (1) Number of fields <-- Redundant: REPORT_COUNT is already 1
    0x81,
    0x03, //(MAIN)   INPUT              0x00000003 (1 field x 6 bits) 1=Constant 1=Variable 0=Absolute 0=NoWrap 0=Linear 0=PrefState 0=NoNull 0=NonVolatile 0=Bitmap
    0x05,
    0x09, //(GLOBAL) USAGE_PAGE         0x0009 Button Page
    0x19,
    0x01, //(LOCAL)  USAGE_MINIMUM      0x00090001 Button 1 Primary/trigger (Selector, On/Off Control, Momentary Control, or One Shot Control)
    0x29,
    0x0A, //(LOCAL)  USAGE_MAXIMUM      0x0009000A Button 10 (Selector, On/Off Control, Momentary Control, or One Shot Control)
    0x95,
    0x0A, //(GLOBAL) REPORT_COUNT       0x0A (10) Number of fields
    0x75,
    0x01, //(GLOBAL) REPORT_SIZE        0x01 (1) Number of bits per field
    0x81,
    0x02, //(MAIN)   INPUT              0x00000002 (10 fields x 1 bit) 0=Data 1=Variable 0=Absolute 0=NoWrap 0=Linear 0=PrefState 0=NoNull 0=NonVolatile 0=Bitmap
    0x15,
    0x00, //(GLOBAL) LOGICAL_MINIMUM    0x00 (0) <-- Redundant: LOGICAL_MINIMUM is already 0 <-- Info: Consider replacing 15 00 with 14
    0x25,
    0x00, //(GLOBAL) LOGICAL_MAXIMUM    0x00 (0) <-- Redundant: LOGICAL_MAXIMUM is already 0 <-- Info: Consider replacing 25 00 with 24
    0x75,
    0x06, //(GLOBAL) REPORT_SIZE        0x06 (6) Number of bits per field
    0x95,
    0x01, //(GLOBAL) REPORT_COUNT       0x01 (1) Number of fields
    0x81,
    0x03, //(MAIN)   INPUT              0x00000003 (1 field x 6 bits) 1=Constant 1=Variable 0=Absolute 0=NoWrap 0=Linear 0=PrefState 0=NoNull 0=NonVolatile 0=Bitmap

    0x05,
    0x01, //(GLOBAL) USAGE_PAGE         0x0001 Generic Desktop Page
    0x09,
    0x39, //(LOCAL)  USAGE              0x00010039 Hat switch (Dynamic Value)
    0x15,
    0x01, //(GLOBAL) LOGICAL_MINIMUM    0x01 (1)
    0x25,
    0x08, //(GLOBAL) LOGICAL_MAXIMUM    0x08 (8)
    0x35,
    0x00, //(GLOBAL) PHYSICAL_MINIMUM   0x00 (0)  <-- Info: Consider replacing 35 00 with 34
    0x46,
    0x3B,
    0x01, //(GLOBAL) PHYSICAL_MAXIMUM   0x013B (315)
    0x66,
    0x14,
    0x00, //(GLOBAL) UNIT               0x0014 Rotation in degrees [1  �� �蕭謕� �蕭豲� ��� ��� �� �� ����� ��  �蕭�� �� �����  �� �� �� �� ��  �蕭謕� �蕭豲 ��  �� �� �� �曹�� ����蕭�� �� ��  �� �蕭豯� ����  �蕭�� �� ��  �� �� ���� ��  ��� �  �� �� �� ��  �� �蕭謕� �蕭��  �蕭謍� �蕭��蕭��  �  �� �蕭�� ����� �� �� ����� �� ��� �� �揭  �� �蕭謕� �蕭���  ����  �蕭��迭���  �蕭謚啾 �����  ��� �  �� �蕭謢� ��� ���� �蕭�豱� �蕭  �� �� �� ����  �� �� �� ���� �� �� �蕭謕� �蕭豲� ����  ����  �蕭�  �� ��蕭��  �� �蕭謕� �蕭豲� ��� ��� �� �� ����� ��  �姘�� ��  �� ��蕭豲� ��� ��� �� �� ����� ��  �蕭�� �� �����  �� �� �� �� ��  �蕭謕� �蕭豲 ��  �� �� �� ����   units] (4=System=English Rotation, 1=Rotation=Degrees)  <-- Info: Consider replacing 66 1400 with 65 14
    0x75,
    0x04, //(GLOBAL) REPORT_SIZE        0x04 (4) Number of bits per field
    0x95,
    0x01, //(GLOBAL) REPORT_COUNT       0x01 (1) Number of fields <-- Redundant: REPORT_COUNT is already 1
    0x81,
    0x42, //(MAIN)   INPUT              0x00000042 (1 field x 4 bits) 0=Data 1=Variable 0=Absolute 0=NoWrap 0=Linear 0=PrefState 1=Null 0=NonVolatile 0=Bitmap
    0x75,
    0x04, //(GLOBAL) REPORT_SIZE        0x04 (4) Number of bits per field <-- Redundant: REPORT_SIZE is already 4
    0x95,
    0x01, //(GLOBAL) REPORT_COUNT       0x01 (1) Number of fields <-- Redundant: REPORT_COUNT is already 1
    0x15,
    0x00, //(GLOBAL) LOGICAL_MINIMUM    0x00 (0)  <-- Info: Consider replacing 15 00 with 14
    0x25,
    0x00, //(GLOBAL) LOGICAL_MAXIMUM    0x00 (0)  <-- Info: Consider replacing 25 00 with 24
    0x35,
    0x00, //(GLOBAL) PHYSICAL_MINIMUM   0x00 (0) <-- Redundant: PHYSICAL_MINIMUM is already 0 <-- Info: Consider replacing 35 00 with 34
    0x45,
    0x00, //(GLOBAL) PHYSICAL_MAXIMUM   0x00 (0)  <-- Info: Consider replacing 45 00 with 44
    0x65,
    0x00, //(GLOBAL) UNIT               0x00 No unit (0=None)  <-- Info: Consider replacing 65 00 with 64
    0x81,
    0x03, //(MAIN)   INPUT              0x00000003 (1 field x 4 bits) 1=Constant 1=Variable 0=Absolute 0=NoWrap 0=Linear 0=PrefState 0=NoNull 0=NonVolatile 0=Bitmap
    0xA1,
    0x02, //(MAIN)   COLLECTION         0x02 Logical (Usage=0x0: Page=, Usage=, Type=) <-- Error: COLLECTION must be preceded by a USAGE <-- Warning: USAGE type should be CL (Logical Collection)

    0x05,
    0x0F, //(GLOBAL) USAGE_PAGE         0x000F Physical Interface Device Page
    0x09,
    0x97, //(LOCAL)  USAGE              0x000F0097 DC Enable Actuators (Selector)
    0x15,
    0x00, //(GLOBAL) LOGICAL_MINIMUM    0x00 (0) <-- Redundant: LOGICAL_MINIMUM is already 0 <-- Info: Consider replacing 15 00 with 14
    0x25,
    0x01, //(GLOBAL) LOGICAL_MAXIMUM    0x01 (1)
    0x75,
    0x04, //(GLOBAL) REPORT_SIZE        0x04 (4) Number of bits per field <-- Redundant: REPORT_SIZE is already 4
    0x95,
    0x01, //(GLOBAL) REPORT_COUNT       0x01 (1) Number of fields <-- Redundant: REPORT_COUNT is already 1
    0x91,
    0x02, //(MAIN)   OUTPUT             0x00000002 (1 field x 4 bits) 0=Data 1=Variable 0=Absolute 0=NoWrap 0=Linear 0=PrefState 0=NoNull 0=NonVolatile 0=Bitmap
    0x15,
    0x00, //(GLOBAL) LOGICAL_MINIMUM    0x00 (0) <-- Redundant: LOGICAL_MINIMUM is already 0 <-- Info: Consider replacing 15 00 with 14
    0x25,
    0x00, //(GLOBAL) LOGICAL_MAXIMUM    0x00 (0)  <-- Info: Consider replacing 25 00 with 24
    0x91,
    0x03, //(MAIN)   OUTPUT             0x00000003 (1 field x 4 bits) 1=Constant 1=Variable 0=Absolute 0=NoWrap 0=Linear 0=PrefState 0=NoNull 0=NonVolatile 0=Bitmap
    0x09,
    0x70, //(LOCAL)  USAGE              0x000F0070 Magnitude (Dynamic Value)
    0x15,
    0x00, //(GLOBAL) LOGICAL_MINIMUM    0x00 (0) <-- Redundant: LOGICAL_MINIMUM is already 0 <-- Info: Consider replacing 15 00 with 14
    0x25,
    0x64, //(GLOBAL) LOGICAL_MAXIMUM    0x64 (100)
    0x75,
    0x08, //(GLOBAL) REPORT_SIZE        0x08 (8) Number of bits per field
    0x95,
    0x04, //(GLOBAL) REPORT_COUNT       0x04 (4) Number of fields
    0x91,
    0x02, //(MAIN)   OUTPUT             0x00000002 (4 fields x 8 bits) 0=Data 1=Variable 0=Absolute 0=NoWrap 0=Linear 0=PrefState 0=NoNull 0=NonVolatile 0=Bitmap
    0x09,
    0x50, //(LOCAL)  USAGE              0x000F0050 Duration (Dynamic Value)
    0x66,
    0x01,
    0x10, //(GLOBAL) UNIT               0x1001 Time in seconds [1 s units] (1=System=SI Linear, 1=Time=Seconds)
    0x55,
    0x0E, //(GLOBAL) UNIT_EXPONENT      0x0E (Unit Value x 10  �� �蕭謕� �蕭豲� ��� ��� �� �� ����� ��  �蕭�� �� �����  �� �� �� �� ��  �蕭謕� �蕭豲 ��  �� �� �� �曹�� ����蕭�� �� ��  �� �蕭豯� ����  �蕭�� �� ��  �� �� ���� ��  ��� �  �� �� �� ��  �� �蕭謕� �蕭��  �蕭謍� �蕭��蕭��  �  �� �蕭�� ����� �� �� ����� �� ��� �� �揭  �� �蕭謕� �蕭���  ����  �蕭��迭���  �蕭謚啾 �����  ��� �  �船 ������  �蕭��蕭謕� �蕭��  �� ���� ��  �揭  �� �蕭��蕭謜� ��  �� ��蕭��  �� �� ����� ��  �� �� �� �� �蕭�� �� �� ��蕭豲� ��� ��� �� �� ����� ��  �蕭�� �� �����  �� �� �� �� ��  �蕭謕� �蕭豲 ��  �� �� �� �曹�� ����蕭�� �� �� �� �� �� ��蕭��  �船 ������  �� ��� ��� �� �� ����� ��  �蕭�� �� �����  �� �� �� �� ��  �蕭謕� �蕭豲 ��  �� �� �� �曹�� ����  �蕭謖� �  �� �� �� ��  �� �蕭��� ���� ��  �蕭��� �� �� �蕭謕� �蕭豲� ��� ��� �� �� ����� ��  �� �蕭謢� ����  �� �蕭�� ����� �� �� ����� ��  �姘���� ��  ��� �  �� �� �� ��  �� �蕭謕� �蕭��  �蕭�����  �蕭謖� �  �船�� �蕭  �� �蕭謕� �蕭��  �� ����  �蕭謖� �  �� �� �� ��  �� �蕭�貔� ���蕭�豱� �蕭  �� �� ��� ��豱� �� ��  �揭  �� �蕭��  �蕭�� ��  ��  �  �� �蕭謕� �蕭豲� ��� ��� �� �� ����� �� �� �� �此����  �� �� �揭  �� �蕭謕� �蕭豲� ��� ���� �蕭�豱� �� ��  ��� �  �� �蕭謢� ���  ��  �蕭�� �� ��)
    0x26,
    0xFF,
    0x00, //(GLOBAL) LOGICAL_MAXIMUM    0x00FF (255)
    0x95,
    0x01, //(GLOBAL) REPORT_COUNT       0x01 (1) Number of fields
    0x91,
    0x02, //(MAIN)   OUTPUT             0x00000002 (1 field x 8 bits) 0=Data 1=Variable 0=Absolute 0=NoWrap 0=Linear 0=PrefState 0=NoNull 0=NonVolatile 0=Bitmap
    0x09,
    0xA7, //(LOCAL)  USAGE              0x000F00A7 Start Delay (Dynamic Value)
    0x91,
    0x02, //(MAIN)   OUTPUT             0x00000002 (1 field x 8 bits) 0=Data 1=Variable 0=Absolute 0=NoWrap 0=Linear 0=PrefState 0=NoNull 0=NonVolatile 0=Bitmap
    0x65,
    0x00, //(GLOBAL) UNIT               0x00 No unit (0=None)  <-- Info: Consider replacing 65 00 with 64
    0x55,
    0x00, //(GLOBAL) UNIT_EXPONENT      0x00 (Unit Value x 10  �� �蕭謕� �蕭豲� ��� ��� �� �� ����� ��  �蕭�� �� �����  �� �� �� �� ��  �蕭謕� �蕭豲 ��  �� �� �� �曹�� ����蕭�� �� ��  �� �蕭豯� ����  �蕭�� �� ��  �� �� ���� ��  ��� �  �� �� �� ��  �� �蕭謕� �蕭��  �蕭謍� �蕭��蕭��  �  �� �蕭�� ����� �� �� ����� �� ��� �� �揭  �� �蕭謕� �蕭���  ����  �蕭��迭���  �蕭謚啾 �����  ��� �  �船 ������  �蕭��蕭謕� �蕭��  �� ���� ��  �揭  �� �蕭��蕭謜� ��  �� ��蕭��  �� �� ����� ��  �� �� �� �� �蕭�� �� �� ��蕭豲� ��� ��� �� �� ����� ��  �蕭�� �� �����  �� �� �� �� ��  �蕭謕� �蕭豲 ��  �� �� �� �曹�� ����蕭�� �� �� �� �� �� ��蕭��  �船 ������  �� �� �揭���  ��� �  �� �蕭謢� ��� ���� �蕭�豱� �蕭  �� �� �� ����  �� �� �� ���� �� �� �蕭謕� �蕭豲� ����  ���  ��  �蕭��  �  ��� �  �船 ������  ��  ��  �  �� ��  �� �蕭謕� �蕭��  �� �� ����� ��  �蕭�� �� �����  �� �� �� �� ��  �蕭  �� ��蕭��  �� �蕭蹐� �� ���� ��  ��� �  �� �� �� ��  �� �蕭謕� �蕭豲� ����  �蕭�� �� ��  �� �揭  �� �蕭謕� �蕭豲� ���  ��  �� ��  ����  ��� �  �� �蕭謢� ����  �蕭�� �� ��  �� �揭  �� �蕭謕� �蕭����蕭)  <-- Info: Consider replacing 55 00 with 54
    0x09,
    0x7C, //(LOCAL)  USAGE              0x000F007C Loop Count (Dynamic Value)
    0x91,
    0x02, //(MAIN)   OUTPUT             0x00000002 (1 field x 8 bits) 0=Data 1=Variable 0=Absolute 0=NoWrap 0=Linear 0=PrefState 0=NoNull 0=NonVolatile 0=Bitmap
    0xC0, //(MAIN)   END_COLLECTION     Logical
    0x05,
    0x01, //(GLOBAL) USAGE_PAGE         0x0001 Generic Desktop Page
    0x09,
    0x80, //(LOCAL)  USAGE              0x00010080 System Control (Application Collection)
    0xA1,
    0x00, //(MAIN)   COLLECTION         0x00 Physical (Usage=0x00010080: Page=Generic Desktop Page, Usage=System Control, Type=Application Collection) <-- Warning: USAGE type should be CP (Physical Collection)
    0x09,
    0x85, //(LOCAL)  USAGE              0x00010085 System Main Menu (One Shot Control)
    0x15,
    0x00, //(GLOBAL) LOGICAL_MINIMUM    0x00 (0) <-- Redundant: LOGICAL_MINIMUM is already 0 <-- Info: Consider replacing 15 00 with 14
    0x25,
    0x01, //(GLOBAL) LOGICAL_MAXIMUM    0x01 (1)
    0x95,
    0x01, //(GLOBAL) REPORT_COUNT       0x01 (1) Number of fields <-- Redundant: REPORT_COUNT is already 1
    0x75,
    0x01, //(GLOBAL) REPORT_SIZE        0x01 (1) Number of bits per field
    0x81,
    0x02, //(MAIN)   INPUT              0x00000002 (1 field x 1 bit) 0=Data 1=Variable 0=Absolute 0=NoWrap 0=Linear 0=PrefState 0=NoNull 0=NonVolatile 0=Bitmap
    0x15,
    0x00, //(GLOBAL) LOGICAL_MINIMUM    0x00 (0) <-- Redundant: LOGICAL_MINIMUM is already 0 <-- Info: Consider replacing 15 00 with 14
    0x25,
    0x00, //(GLOBAL) LOGICAL_MAXIMUM    0x00 (0)  <-- Info: Consider replacing 25 00 with 24
    0x75,
    0x07, //(GLOBAL) REPORT_SIZE        0x07 (7) Number of bits per field
    0x95,
    0x01, //(GLOBAL) REPORT_COUNT       0x01 (1) Number of fields <-- Redundant: REPORT_COUNT is already 1
    0x81,
    0x03, //(MAIN)   INPUT              0x00000003 (1 field x 7 bits) 1=Constant 1=Variable 0=Absolute 0=NoWrap 0=Linear 0=PrefState 0=NoNull 0=NonVolatile 0=Bitmap
    0xC0, //(MAIN)   END_COLLECTION     Physical
    0x05,
    0x06, //(GLOBAL) USAGE_PAGE         0x0006 Generic Device Controls Page
    0x09,
    0x20, //(LOCAL)  USAGE              0x00060020 Battery Strength (Dynamic Value)
    0x15,
    0x00, //(GLOBAL) LOGICAL_MINIMUM    0x00 (0) <-- Redundant: LOGICAL_MINIMUM is already 0 <-- Info: Consider replacing 15 00 with 14
    0x26,
    0xFF,
    0x00, //(GLOBAL) LOGICAL_MAXIMUM    0x00FF (255)
    0x75,
    0x08, //(GLOBAL) REPORT_SIZE        0x08 (8) Number of bits per field
    0x95,
    0x01, //(GLOBAL) REPORT_COUNT       0x01 (1) Number of fields <-- Redundant: REPORT_COUNT is already 1
    0x81,
    0x02, //(MAIN)   INPUT              0x00000002 (1 field x 8 bits) 0=Data 1=Variable 0=Absolute 0=NoWrap 0=Linear 0=PrefState 0=NoNull 0=NonVolatile 0=Bitmap
    0xC0,

};
/* wrapper */
static const USBDescriptor extra_hid_report_descriptor = {
    sizeof extra_hid_report_desc_data,
    extra_hid_report_desc_data};
#endif /* EXTRAKEY_ENABLE */

/*
 * Configuration Descriptor tree for a HID device
 *
 * The HID Specifications version 1.11 require the following order:
 * - Configuration Descriptor
 * - Interface Descriptor
 * - HID Descriptor
 * - Endpoints Descriptors
 */
#define KBD_HID_DESC_NUM 0
#define KBD_HID_DESC_OFFSET (9 + (9 + 9 + 7) * KBD_HID_DESC_NUM + 9)

#ifdef MOUSE_ENABLE
#define MOUSE_HID_DESC_NUM (KBD_HID_DESC_NUM + 1)
#define MOUSE_HID_DESC_OFFSET (9 + (9 + 9 + 7) * MOUSE_HID_DESC_NUM + 9)
#else /* MOUSE_ENABLE */
#define MOUSE_HID_DESC_NUM (KBD_HID_DESC_NUM + 0)
#endif /* MOUSE_ENABLE */

#ifdef RAW_ENABLE       //!!
#define RAW_HID_DESC_NUM (MOUSE_HID_DESC_NUM + 1)
//#define RAW_HID_DESC_OFFSET (9 + (9 + 9 + 7) * RAW_HID_DESC_NUM + 9)
#define RAW_HID_DESC_OFFSET (9 + (9 + 9 + 7 + 7) * RAW_HID_DESC_NUM + 9)
#else /* MOUSE_ENABLE */
#define RAW_HID_DESC_NUM (MOUSE_HID_DESC_NUM + 0)
#endif /* MOUSE_ENABLE */

#ifdef CONSOLE_ENABLE
#define CONSOLE_HID_DESC_NUM (MOUSE_HID_DESC_NUM + 1)
#define CONSOLE_HID_DESC_OFFSET (9 + (9 + 9 + 7) * CONSOLE_HID_DESC_NUM + 9)
#else /* CONSOLE_ENABLE */
#define CONSOLE_HID_DESC_NUM (MOUSE_HID_DESC_NUM + 0)
#endif /* CONSOLE_ENABLE */

#ifdef RAW_ENABLE 
    #ifdef EXTRAKEY_ENABLE
    #define EXTRA_HID_DESC_NUM (RAW_HID_DESC_NUM + 1)
    #define EXTRA_HID_DESC_OFFSET (9 + (9 + 9 + 7) * EXTRA_HID_DESC_NUM + 9)
    #else /* EXTRAKEY_ENABLE */
    #define EXTRA_HID_DESC_NUM (RAW_HID_DESC_NUM + 0)
    #endif /* EXTRAKEY_ENABLE */
#endif
#ifdef CONSOLE_ENABLE
    #ifdef EXTRAKEY_ENABLE
    #define EXTRA_HID_DESC_NUM (CONSOLE_HID_DESC_NUM + 1)
    #define EXTRA_HID_DESC_OFFSET (9 + (9 + 9 + 7) * EXTRA_HID_DESC_NUM + 9)
    #else /* EXTRAKEY_ENABLE */
    #define EXTRA_HID_DESC_NUM (CONSOLE_HID_DESC_NUM + 0)
    #endif /* EXTRAKEY_ENABLE */
#endif

// #ifdef EXTRAKEY_ENABLE
// #define EXTRA_HID_DESC_NUM (CONSOLE_HID_DESC_NUM + 1)
// #define EXTRA_HID_DESC_OFFSET (9 + (9 + 9 + 7) * EXTRA_HID_DESC_NUM + 9)
// #else /* EXTRAKEY_ENABLE */
// #define EXTRA_HID_DESC_NUM (CONSOLE_HID_DESC_NUM + 0)
// #endif /* EXTRAKEY_ENABLE */

#ifdef NKRO_ENABLE
#define NKRO_HID_DESC_NUM (EXTRA_HID_DESC_NUM + 1)
#define NKRO_HID_DESC_OFFSET (9 + (9 + 9 + 7) * EXTRA_HID_DESC_NUM + 9)
#else /* NKRO_ENABLE */
#define NKRO_HID_DESC_NUM (EXTRA_HID_DESC_NUM + 0)
#endif /* NKRO_ENABLE */

#ifdef LAMP_ARRAY_ENABLE
#define LAMP_ARRAY_HID_DESC_NUM (NKRO_HID_DESC_NUM + 1)
#define LAMP_ARRAY_HID_DESC_OFFSET (9 + (9 + 9 + 7) * NKRO_HID_DESC_NUM + 9)
#else /* LAMP_ARRAY_ENABLE */
#define LAMP_ARRAY_HID_DESC_NUM (NKRO_HID_DESC_NUM + 0)
#endif /* LAMP_ARRAY_ENABLE */

#ifdef CONTROL_ENABLE
#define CONTROL_HID_DESC_NUM (LAMP_ARRAY_HID_DESC_NUM + 1)
#define CONTROL_HID_DESC_OFFSET (9 + (9 + 9 + 7) * LAMP_ARRAY_HID_DESC_NUM + 9)
#else /* CONTROL_ENABLE */
#define CONTROL_HID_DESC_NUM (LAMP_ARRAY_HID_DESC_NUM + 0)
#endif /* CONTROL_ENABLE */

#define NUM_INTERFACES (CONTROL_HID_DESC_NUM + 1)
#define CONFIG1_DESC_SIZE (9 + (9 + 9 + 7) * NUM_INTERFACES - 9 + 7)		///////!!!! in out 9 -7 = 2

// static const uint8_t hid_configuration_descriptor_data[] = {
//     /* Configuration Descriptor (9 bytes) USB spec 9.6.3, page 264-266, Table 9-10 */
//     USB_DESC_CONFIGURATION(CONFIG1_DESC_SIZE + USB_DESC_INTERFACE_ASSOCIATION_SIZE * 2, // wTotalLength
//                            NUM_INTERFACES,                                              // bNumInterfaces
//                            1,                                                           // bConfigurationValue
//                            0,                                                           // iConfiguration
//                            0xA0,                                                        // bmAttributes (RESERVED|REMOTEWAKEUP)
//                            0xFA),                                                       // bMaxPower (500mA)

//     IAD_HID_IF_DESC_SET(0, 1, HID_COMMUNICATION_INTERFACE_CLASS)

//     /* Interface Descriptor (9 bytes) USB spec 9.6.5, page 267-269, Table 9-12 */
//     USB_DESC_INTERFACE(KBD_INTERFACE, // bInterfaceNumber
//                        0,             // bAlternateSetting
//                        1,             // bNumEndpoints
//                        0x03,          // bInterfaceClass: HID
//                        0x01,          // bInterfaceSubClass: Boot
//                        0x01,          // bInterfaceProtocol: Keyboard
//                        0),            // iInterface

//     /* HID descriptor (9 bytes) HID 1.11 spec, section 6.2.1 */
//     USB_DESC_BYTE(9),                                     // bLength
//     USB_DESC_BYTE(0x21),                                  // bDescriptorType (HID class)
//     USB_DESC_BCD(0x0111),                                 // bcdHID: HID version 1.11
//     USB_DESC_BYTE(0),                                     // bCountryCode
//     USB_DESC_BYTE(1),                                     // bNumDescriptors
//     USB_DESC_BYTE(0x22),                                  // bDescriptorType (report desc)
//     USB_DESC_WORD(sizeof(keyboard_hid_report_desc_data)), // wDescriptorLength

//     /* Endpoint Descriptor (7 bytes) USB spec 9.6.6, page 269-271, Table 9-13 */
//     USB_DESC_ENDPOINT(KBD_ENDPOINT | 0x80, // bEndpointAddress
//                       0x03,                // bmAttributes (Interrupt)
//                       KBD_EPSIZE,          // wMaxPacketSize
//                       1),                  // bInterval

// #ifdef MOUSE_ENABLE
//     /* Interface Descriptor (9 bytes) USB spec 9.6.5, page 267-269, Table 9-12 */
//     USB_DESC_INTERFACE(MOUSE_INTERFACE, // bInterfaceNumber
//                        0,               // bAlternateSetting
//                        1,               // bNumEndpoints
//                        0x03,            // bInterfaceClass (0x03 = HID)
//                        // ThinkPad T23 BIOS doesn't work with boot mouse.
//                        0x00, // bInterfaceSubClass (0x01 = Boot)
//                        0x00, // bInterfaceProtocol (0x02 = Mouse)
//                        /*
//                           0x01,      // bInterfaceSubClass (0x01 = Boot)
//                           0x02,      // bInterfaceProtocol (0x02 = Mouse)
//                         */
//                        0), // iInterface

//     /* HID descriptor (9 bytes) HID 1.11 spec, section 6.2.1 */
//     USB_DESC_BYTE(9),                                  // bLength
//     USB_DESC_BYTE(0x21),                               // bDescriptorType (HID class)
//     USB_DESC_BCD(0x0111),                              // bcdHID: HID version 1.11
//     USB_DESC_BYTE(0),                                  // bCountryCode
//     USB_DESC_BYTE(1),                                  // bNumDescriptors
//     USB_DESC_BYTE(0x22),                               // bDescriptorType (report desc)
//     USB_DESC_WORD(sizeof(mouse_hid_report_desc_data)), // wDescriptorLength

//     /* Endpoint Descriptor (7 bytes) USB spec 9.6.6, page 269-271, Table 9-13 */
//     USB_DESC_ENDPOINT(MOUSE_ENDPOINT | 0x80, // bEndpointAddress
//                       0x03,                  // bmAttributes (Interrupt)
//                       MOUSE_EPSIZE,          // wMaxPacketSize
//                       1),                    // bInterval
// #endif                                       /* MOUSE_ENABLE */

// #ifdef RAW_ENABLE   //!!
//     /* Interface Descriptor (9 bytes) USB spec 9.6.5, page 267-269, Table 9-12 */
//     USB_DESC_INTERFACE(RAW_INTERFACE, // bInterfaceNumber
//                        0,                 // bAlternateSetting
//                         2,                 // bNumEndpoints
// //                       1,                 // bNumEndpoints
//                        0x03,              // bInterfaceClass: HID
//                        0x00,              // bInterfaceSubClass: None
//                        0x00,              // bInterfaceProtocol: None
//                        0),                // iInterface

//     /* HID descriptor (9 bytes) HID 1.11 spec, section 6.2.1 */
//     USB_DESC_BYTE(9),                                    // bLength
//     USB_DESC_BYTE(0x21),                                 // bDescriptorType (HID class)
//     USB_DESC_BCD(0x0111),                                // bcdHID: HID version 1.11
//     USB_DESC_BYTE(0),                                    // bCountryCode
//     USB_DESC_BYTE(1),                                    // bNumDescriptors
//     USB_DESC_BYTE(0x22),                                 // bDescriptorType (report desc)
//     USB_DESC_WORD(sizeof(raw_hid_report_desc_data)),     // wDescriptorLength

//     /* Endpoint Descriptor (7 bytes) USB spec 9.6.6, page 269-271, Table 9-13 */
//      USB_DESC_ENDPOINT(RAW_IN_ENDPOINT | 0x80,   // bEndpointAddress
//                        0x03,                     // bmAttributes (Interrupt)
//                        RAW_EPSIZE,               // wMaxPacketSize
//                        //!!
//                        1),                       // bInterval

//     /* Endpoint Descriptor (7 bytes) USB spec 9.6.6, page 269-271, Table 9-13 */                      
//     USB_DESC_ENDPOINT(RAW_OUT_ENDPOINT,         // bEndpointAddress
//                       0x03,                     // bmAttributes (Interrupt)
//                       RAW_EPSIZE,               // wMaxPacketSize
//                       1),                       // bInterval

// #endif                                          /* RAW_ENABLE */

// #ifdef CONSOLE_ENABLE
//     /* Interface Descriptor (9 bytes) USB spec 9.6.5, page 267-269, Table 9-12 */
//     USB_DESC_INTERFACE(CONSOLE_INTERFACE, // bInterfaceNumber
//                        0,                 // bAlternateSetting
//                        1,                 // bNumEndpoints
//                        0x03,              // bInterfaceClass: HID
//                        0x00,              // bInterfaceSubClass: None
//                        0x00,              // bInterfaceProtocol: None
//                        0),                // iInterface

//     /* HID descriptor (9 bytes) HID 1.11 spec, section 6.2.1 */
//     USB_DESC_BYTE(9),                                    // bLength
//     USB_DESC_BYTE(0x21),                                 // bDescriptorType (HID class)
//     USB_DESC_BCD(0x0111),                                // bcdHID: HID version 1.11
//     USB_DESC_BYTE(0),                                    // bCountryCode
//     USB_DESC_BYTE(1),                                    // bNumDescriptors
//     USB_DESC_BYTE(0x22),                                 // bDescriptorType (report desc)
//     USB_DESC_WORD(sizeof(console_hid_report_desc_data)), // wDescriptorLength

//     /* Endpoint Descriptor (7 bytes) USB spec 9.6.6, page 269-271, Table 9-13 */
//     USB_DESC_ENDPOINT(CONSOLE_ENDPOINT | 0x80, // bEndpointAddress
//                       0x03,                    // bmAttributes (Interrupt)
//                       CONSOLE_EPSIZE,          // wMaxPacketSize
//                       1),                      // bInterval
// #endif                                         /* CONSOLE_ENABLE */

// #ifdef EXTRAKEY_ENABLE
//     /* Interface Descriptor (9 bytes) USB spec 9.6.5, page 267-269, Table 9-12 */
//     USB_DESC_INTERFACE(EXTRA_INTERFACE, // bInterfaceNumber
//                        0,               // bAlternateSetting
//                        1,               // bNumEndpoints
//                        0x03,            // bInterfaceClass: HID
//                        0x00,            // bInterfaceSubClass: None
//                        0x00,            // bInterfaceProtocol: None
//                        0),              // iInterface

//     /* HID descriptor (9 bytes) HID 1.11 spec, section 6.2.1 */
//     USB_DESC_BYTE(9),                                  // bLength
//     USB_DESC_BYTE(0x21),                               // bDescriptorType (HID class)
//     USB_DESC_BCD(0x0111),                              // bcdHID: HID version 1.11
//     USB_DESC_BYTE(0),                                  // bCountryCode
//     USB_DESC_BYTE(1),                                  // bNumDescriptors
//     USB_DESC_BYTE(0x22),                               // bDescriptorType (report desc)
//     USB_DESC_WORD(sizeof(extra_hid_report_desc_data)), // wDescriptorLength

//     /* Endpoint Descriptor (7 bytes) USB spec 9.6.6, page 269-271, Table 9-13 */
//     USB_DESC_ENDPOINT(EXTRA_ENDPOINT | 0x80, // bEndpointAddress
//                       0x03,                  // bmAttributes (Interrupt)
//                       EXTRA_EPSIZE,          // wMaxPacketSize
//                       2),                    // bInterval
// #endif                                       /* EXTRAKEY_ENABLE */

// #ifdef NKRO_ENABLE
//     /* Interface Descriptor (9 bytes) USB spec 9.6.5, page 267-269, Table 9-12 */
//     USB_DESC_INTERFACE(NKRO_INTERFACE, // bInterfaceNumber
//                        0,              // bAlternateSetting
//                        1,              // bNumEndpoints
//                        0x03,           // bInterfaceClass: HID
//                        0x00,           // bInterfaceSubClass: None
//                        0x00,           // bInterfaceProtocol: None
//                        0),             // iInterface

//     /* HID descriptor (9 bytes) HID 1.11 spec, section 6.2.1 */
//     USB_DESC_BYTE(9),                                 // bLength
//     USB_DESC_BYTE(0x21),                              // bDescriptorType (HID class)
//     USB_DESC_BCD(0x0111),                             // bcdHID: HID version 1.11
//     USB_DESC_BYTE(0),                                 // bCountryCode
//     USB_DESC_BYTE(1),                                 // bNumDescriptors
//     USB_DESC_BYTE(0x22),                              // bDescriptorType (report desc)
//     USB_DESC_WORD(sizeof(nkro_hid_report_desc_data)), // wDescriptorLength

//     /* Endpoint Descriptor (7 bytes) USB spec 9.6.6, page 269-271, Table 9-13 */
//     USB_DESC_ENDPOINT(NKRO_ENDPOINT | 0x80, // bEndpointAddress
//                       0x03,                 // bmAttributes (Interrupt)
//                       NKRO_EPSIZE,          // wMaxPacketSize
//                       1),                   // bInterval
// #endif                                      /* NKRO_ENABLE */
// #ifdef LAMP_ARRAY_ENABLE
//     /* Interface Descriptor (9 bytes) USB spec 9.6.5, page 267-269, Table 9-12 */
//     USB_DESC_INTERFACE(LAMP_ARRAY_INTERFACE, // bInterfaceNumber
//                        0,                    // bAlternateSetting
//                        1,                    // bNumEndpoints
//                        0x03,                 // bInterfaceClass: HID
//                        0x00,                 // bInterfaceSubClass
//                        0x00,                 // bInterfaceProtocol
//                        0),                   // iInterface

//     /* HID descriptor (9 bytes) HID 1.11 spec, section 6.2.1 */
//     USB_DESC_BYTE(9),                                       // bLength
//     USB_DESC_BYTE(0x21),                                    // bDescriptorType (HID class)
//     USB_DESC_BCD(0x0111),                                   // bcdHID: HID version 1.11
//     USB_DESC_BYTE(0),                                       // bCountryCode
//     USB_DESC_BYTE(1),                                       // bNumDescriptors
//     USB_DESC_BYTE(0x22),                                    // bDescriptorType (report desc)
//     USB_DESC_WORD(sizeof(lamp_array_hid_report_desc_data)), // wDescriptorLength

//     /* Endpoint Descriptor (7 bytes) USB spec 9.6.6, page 269-271, Table 9-13 */
//     USB_DESC_ENDPOINT(LAMP_ARRAY_ENDPOINT | 0x80, // bEndpointAddress
//                       0x03,                       // bmAttributes (Interrupt)
//                       LAMP_ARRAY_EPSIZE,          // wMaxPacketSize
//                       1),                         // bInterval

// #endif /* LAMP_ARRAY */

// #ifdef CONTROL_ENABLE
//     /* Interface Descriptor (9 bytes) USB spec 9.6.5, page 267-269, Table 9-12 */
//     IAD_HID_IF_DESC_SET(6, 1, VENDOR_COMMUNICATION_INTERFACE_CLASS)
//         USB_DESC_INTERFACE(CONTROL_INTERFACE, // bInterfaceNumber
//                            0,                 // bAlternateSetting
//                            1,                 // bNumEndpoints
//                            0xff,              // bInterfaceClass: HID
//                            0x00,              // bInterfaceSubClass
//                            0x00,              // bInterfaceProtocol
//                            0),                // iInterface

//     /* HID descriptor (9 bytes) HID 1.11 spec, section 6.2.1 */
//     //  USB_DESC_BYTE(9),            // bLength
//     //  USB_DESC_BYTE(0x21),         // bDescriptorType (HID class)
//     //  USB_DESC_BCD(0x0111),        // bcdHID: HID version 1.11
//     //  USB_DESC_BYTE(0),            // bCountryCode
//     //  USB_DESC_BYTE(1),            // bNumDescriptors
//     //  USB_DESC_BYTE(0x22),         // bDescriptorType (report desc)
//     //  USB_DESC_WORD(sizeof(control_hid_report_desc_data)), // wDescriptorLength

//     /* Endpoint Descriptor (7 bytes) USB spec 9.6.6, page 269-271, Table 9-13 */
//     USB_DESC_ENDPOINT(CONTROL_ENDPOINT | 0x80, // bEndpointAddress
//                       0x03,                    // bmAttributes (Interrupt)
//                       CONTROL_EPSIZE,          // wMaxPacketSize
//                       1),                      // bInterval

// #endif /* CONTROL */
// };
/***********************/
static const uint8_t hid_configuration_descriptor_data_8000hz[] = { //_8000hz
    // Configuration Descriptor (9 bytes) USB spec 9.6.3, page 264-266, Table 9-10
		USB_DESC_CONFIGURATION(CONFIG1_DESC_SIZE,
													 NUM_INTERFACES,
													 1,
													 0,
													 0xA0,
													 0xFA),                                               // bMaxPower (500mA)

    //IAD_HID_IF_DESC_SET(0, 1, HID_COMMUNICATION_INTERFACE_CLASS)

    // Interface Descriptor (9 bytes) USB spec 9.6.5, page 267-269, Table 9-12
    USB_DESC_INTERFACE(KBD_INTERFACE, // bInterfaceNumber
                       0,             // bAlternateSetting
                       1,             // bNumEndpoints
                       0x03,          // bInterfaceClass: HID
                       0x01,          // bInterfaceSubClass: Boot
                       0x01,          // bInterfaceProtocol: Keyboard
                       0),            // iInterface

    // HID descriptor (9 bytes) HID 1.11 spec, section 6.2.1
    USB_DESC_BYTE(9),                                     // bLength
    USB_DESC_BYTE(0x21),                                  // bDescriptorType (HID class)
    USB_DESC_BCD(0x0111),                                 // bcdHID: HID version 1.11
    USB_DESC_BYTE(0),                                     // bCountryCode
    USB_DESC_BYTE(1),                                     // bNumDescriptors
    USB_DESC_BYTE(0x22),                                  // bDescriptorType (report desc)
    USB_DESC_WORD(sizeof(keyboard_hid_report_desc_data)), // wDescriptorLength

    // Endpoint Descriptor (7 bytes) USB spec 9.6.6, page 269-271, Table 9-13
    USB_DESC_ENDPOINT(KBD_ENDPOINT | 0x80, // bEndpointAddress
                      0x03,                // bmAttributes (Interrupt)
                      KBD_EPSIZE,          // wMaxPacketSize
                      1),                  // bInterval

#ifdef MOUSE_ENABLE
    // Interface Descriptor (9 bytes) USB spec 9.6.5, page 267-269, Table 9-12
    USB_DESC_INTERFACE(MOUSE_INTERFACE, // bInterfaceNumber
                       0,               // bAlternateSetting
                       1,               // bNumEndpoints
                       0x03,            // bInterfaceClass (0x03 = HID)
                       // ThinkPad T23 BIOS doesn't work with boot mouse.
                       0x00, // bInterfaceSubClass (0x01 = Boot)
                       0x00, // bInterfaceProtocol (0x02 = Mouse)
                       /*
                          0x01,      // bInterfaceSubClass (0x01 = Boot)
                          0x02,      // bInterfaceProtocol (0x02 = Mouse)
                        */
                       0), // iInterface

    /* HID descriptor (9 bytes) HID 1.11 spec, section 6.2.1 */
    USB_DESC_BYTE(9),                                  // bLength
    USB_DESC_BYTE(0x21),                               // bDescriptorType (HID class)
    USB_DESC_BCD(0x0111),                              // bcdHID: HID version 1.11
    USB_DESC_BYTE(0),                                  // bCountryCode
    USB_DESC_BYTE(1),                                  // bNumDescriptors
    USB_DESC_BYTE(0x22),                               // bDescriptorType (report desc)
    USB_DESC_WORD(sizeof(mouse_hid_report_desc_data)), // wDescriptorLength

    /* Endpoint Descriptor (7 bytes) USB spec 9.6.6, page 269-271, Table 9-13 */
    USB_DESC_ENDPOINT(MOUSE_ENDPOINT | 0x80, // bEndpointAddress
                      0x03,                  // bmAttributes (Interrupt)
                      MOUSE_EPSIZE,          // wMaxPacketSize
                      1),                    // bInterval
#endif                                       /* MOUSE_ENABLE */

#ifdef RAW_ENABLE   //!!
    /* Interface Descriptor (9 bytes) USB spec 9.6.5, page 267-269, Table 9-12 */
    USB_DESC_INTERFACE(RAW_INTERFACE, // bInterfaceNumber
                       0,                 // bAlternateSetting
                        2,                 // bNumEndpoints
//                       1,                 // bNumEndpoints
                       0x03,              // bInterfaceClass: HID
                       0x00,              // bInterfaceSubClass: None
                       0x00,              // bInterfaceProtocol: None
                       0),                // iInterface

    /* HID descriptor (9 bytes) HID 1.11 spec, section 6.2.1 */
    USB_DESC_BYTE(9),                                    // bLength
    USB_DESC_BYTE(0x21),                                 // bDescriptorType (HID class)
    USB_DESC_BCD(0x0111),                                // bcdHID: HID version 1.11
    USB_DESC_BYTE(0),                                    // bCountryCode
    USB_DESC_BYTE(1),                                    // bNumDescriptors
    USB_DESC_BYTE(0x22),                                 // bDescriptorType (report desc)
    USB_DESC_WORD(sizeof(raw_hid_report_desc_data)),     // wDescriptorLength

    /* Endpoint Descriptor (7 bytes) USB spec 9.6.6, page 269-271, Table 9-13 */
     USB_DESC_ENDPOINT(RAW_IN_ENDPOINT | 0x80,   // bEndpointAddress
                       0x03,                     // bmAttributes (Interrupt)
                       RAW_EPSIZE,               // wMaxPacketSize
                       //!!
                       1),                       // bInterval

    /* Endpoint Descriptor (7 bytes) USB spec 9.6.6, page 269-271, Table 9-13 */                      
    USB_DESC_ENDPOINT(RAW_OUT_ENDPOINT,         // bEndpointAddress
                      0x03,                     // bmAttributes (Interrupt)
                      RAW_EPSIZE,               // wMaxPacketSize
                      1),                       // bInterval

#endif                                          /* RAW_ENABLE */

#ifdef CONSOLE_ENABLE
    /* Interface Descriptor (9 bytes) USB spec 9.6.5, page 267-269, Table 9-12 */
    USB_DESC_INTERFACE(CONSOLE_INTERFACE, // bInterfaceNumber
                       0,                 // bAlternateSetting
                       1,                 // bNumEndpoints
                       0x03,              // bInterfaceClass: HID
                       0x00,              // bInterfaceSubClass: None
                       0x00,              // bInterfaceProtocol: None
                       0),                // iInterface

    /* HID descriptor (9 bytes) HID 1.11 spec, section 6.2.1 */
    USB_DESC_BYTE(9),                                    // bLength
    USB_DESC_BYTE(0x21),                                 // bDescriptorType (HID class)
    USB_DESC_BCD(0x0111),                                // bcdHID: HID version 1.11
    USB_DESC_BYTE(0),                                    // bCountryCode
    USB_DESC_BYTE(1),                                    // bNumDescriptors
    USB_DESC_BYTE(0x22),                                 // bDescriptorType (report desc)
    USB_DESC_WORD(sizeof(console_hid_report_desc_data)), // wDescriptorLength

    /* Endpoint Descriptor (7 bytes) USB spec 9.6.6, page 269-271, Table 9-13 */
    USB_DESC_ENDPOINT(CONSOLE_ENDPOINT | 0x80, // bEndpointAddress
                      0x03,                    // bmAttributes (Interrupt)
                      CONSOLE_EPSIZE,          // wMaxPacketSize
                      1),                      // bInterval
#endif                                         /* CONSOLE_ENABLE */

#ifdef EXTRAKEY_ENABLE
    /* Interface Descriptor (9 bytes) USB spec 9.6.5, page 267-269, Table 9-12 */
    USB_DESC_INTERFACE(EXTRA_INTERFACE, // bInterfaceNumber
                       0,               // bAlternateSetting
                       1,               // bNumEndpoints
                       0x03,            // bInterfaceClass: HID
                       0x00,            // bInterfaceSubClass: None
                       0x00,            // bInterfaceProtocol: None
                       0),              // iInterface

    /* HID descriptor (9 bytes) HID 1.11 spec, section 6.2.1 */
    USB_DESC_BYTE(9),                                  // bLength
    USB_DESC_BYTE(0x21),                               // bDescriptorType (HID class)
    USB_DESC_BCD(0x0111),                              // bcdHID: HID version 1.11
    USB_DESC_BYTE(0),                                  // bCountryCode
    USB_DESC_BYTE(1),                                  // bNumDescriptors
    USB_DESC_BYTE(0x22),                               // bDescriptorType (report desc)
    USB_DESC_WORD(sizeof(extra_hid_report_desc_data)), // wDescriptorLength

    /* Endpoint Descriptor (7 bytes) USB spec 9.6.6, page 269-271, Table 9-13 */
    USB_DESC_ENDPOINT(EXTRA_ENDPOINT | 0x80, // bEndpointAddress
                      0x03,                  // bmAttributes (Interrupt)
                      EXTRA_EPSIZE,          // wMaxPacketSize
                      2),                    // bInterval
#endif                                       /* EXTRAKEY_ENABLE */

#ifdef NKRO_ENABLE
    /* Interface Descriptor (9 bytes) USB spec 9.6.5, page 267-269, Table 9-12 */
    USB_DESC_INTERFACE(NKRO_INTERFACE, // bInterfaceNumber
                       0,              // bAlternateSetting
                       1,              // bNumEndpoints
                       0x03,           // bInterfaceClass: HID
                       0x00,           // bInterfaceSubClass: None
                       0x00,           // bInterfaceProtocol: None
                       0),             // iInterface

    /* HID descriptor (9 bytes) HID 1.11 spec, section 6.2.1 */
    USB_DESC_BYTE(9),                                 // bLength
    USB_DESC_BYTE(0x21),                              // bDescriptorType (HID class)
    USB_DESC_BCD(0x0111),                             // bcdHID: HID version 1.11
    USB_DESC_BYTE(0),                                 // bCountryCode
    USB_DESC_BYTE(1),                                 // bNumDescriptors
    USB_DESC_BYTE(0x22),                              // bDescriptorType (report desc)
    USB_DESC_WORD(sizeof(nkro_hid_report_desc_data)), // wDescriptorLength

    /* Endpoint Descriptor (7 bytes) USB spec 9.6.6, page 269-271, Table 9-13 */
    USB_DESC_ENDPOINT(NKRO_ENDPOINT | 0x80, // bEndpointAddress
                      0x03,                 // bmAttributes (Interrupt)
                      NKRO_EPSIZE,          // wMaxPacketSize
                      1),                   // bInterval
#endif                                      /* NKRO_ENABLE */
#ifdef LAMP_ARRAY_ENABLE
    /* Interface Descriptor (9 bytes) USB spec 9.6.5, page 267-269, Table 9-12 */
    USB_DESC_INTERFACE(LAMP_ARRAY_INTERFACE, // bInterfaceNumber
                       0,                    // bAlternateSetting
                       1,                    // bNumEndpoints
                       0x03,                 // bInterfaceClass: HID
                       0x00,                 // bInterfaceSubClass
                       0x00,                 // bInterfaceProtocol
                       0),                   // iInterface

    /* HID descriptor (9 bytes) HID 1.11 spec, section 6.2.1 */
    USB_DESC_BYTE(9),                                       // bLength
    USB_DESC_BYTE(0x21),                                    // bDescriptorType (HID class)
    USB_DESC_BCD(0x0111),                                   // bcdHID: HID version 1.11
    USB_DESC_BYTE(0),                                       // bCountryCode
    USB_DESC_BYTE(1),                                       // bNumDescriptors
    USB_DESC_BYTE(0x22),                                    // bDescriptorType (report desc)
    USB_DESC_WORD(sizeof(lamp_array_hid_report_desc_data)), // wDescriptorLength

    /* Endpoint Descriptor (7 bytes) USB spec 9.6.6, page 269-271, Table 9-13 */
    USB_DESC_ENDPOINT(LAMP_ARRAY_ENDPOINT | 0x80, // bEndpointAddress
                      0x03,                       // bmAttributes (Interrupt)
                      LAMP_ARRAY_EPSIZE,          // wMaxPacketSize
                      1),                         // bInterval

#endif /* LAMP_ARRAY */

#ifdef CONTROL_ENABLE
    /* Interface Descriptor (9 bytes) USB spec 9.6.5, page 267-269, Table 9-12 */
    //IAD_HID_IF_DESC_SET(6, 1, VENDOR_COMMUNICATION_INTERFACE_CLASS)
        USB_DESC_INTERFACE(CONTROL_INTERFACE, // bInterfaceNumber
                           0,                 // bAlternateSetting
                           1,                 // bNumEndpoints
                           0xff,              // bInterfaceClass: HID
                           0x00,              // bInterfaceSubClass
                           0x00,              // bInterfaceProtocol
                           0),                // iInterface

    /* HID descriptor (9 bytes) HID 1.11 spec, section 6.2.1 */
    //  USB_DESC_BYTE(9),            // bLength
    //  USB_DESC_BYTE(0x21),         // bDescriptorType (HID class)
    //  USB_DESC_BCD(0x0111),        // bcdHID: HID version 1.11
    //  USB_DESC_BYTE(0),            // bCountryCode
    //  USB_DESC_BYTE(1),            // bNumDescriptors
    //  USB_DESC_BYTE(0x22),         // bDescriptorType (report desc)
    //  USB_DESC_WORD(sizeof(control_hid_report_desc_data)), // wDescriptorLength

    /* Endpoint Descriptor (7 bytes) USB spec 9.6.6, page 269-271, Table 9-13 */
    USB_DESC_ENDPOINT(CONTROL_ENDPOINT | 0x80, // bEndpointAddress
                      0x03,                    // bmAttributes (Interrupt)
                      CONTROL_EPSIZE,          // wMaxPacketSize
                      1),                      // bInterval

#endif /* CONTROL */
};

static const uint8_t hid_configuration_descriptor_data_4000hz[] = {
    /* Configuration Descriptor (9 bytes) USB spec 9.6.3, page 264-266, Table 9-10 */
		USB_DESC_CONFIGURATION(CONFIG1_DESC_SIZE,
													 NUM_INTERFACES,
													 1,
													 0,
													 0xA0,
													 0xFA),                                                  // bMaxPower (500mA)

    //IAD_HID_IF_DESC_SET(0, 1, HID_COMMUNICATION_INTERFACE_CLASS)

    /* Interface Descriptor (9 bytes) USB spec 9.6.5, page 267-269, Table 9-12 */
    USB_DESC_INTERFACE(KBD_INTERFACE, // bInterfaceNumber
                       0,             // bAlternateSetting
                       1,             // bNumEndpoints
                       0x03,          // bInterfaceClass: HID
                       0x01,          // bInterfaceSubClass: Boot
                       0x01,          // bInterfaceProtocol: Keyboard
                       0),            // iInterface

    /* HID descriptor (9 bytes) HID 1.11 spec, section 6.2.1 */
    USB_DESC_BYTE(9),                                     // bLength
    USB_DESC_BYTE(0x21),                                  // bDescriptorType (HID class)
    USB_DESC_BCD(0x0111),                                 // bcdHID: HID version 1.11
    USB_DESC_BYTE(0),                                     // bCountryCode
    USB_DESC_BYTE(1),                                     // bNumDescriptors
    USB_DESC_BYTE(0x22),                                  // bDescriptorType (report desc)
    USB_DESC_WORD(sizeof(keyboard_hid_report_desc_data)), // wDescriptorLength

    /* Endpoint Descriptor (7 bytes) USB spec 9.6.6, page 269-271, Table 9-13 */
    USB_DESC_ENDPOINT(KBD_ENDPOINT | 0x80, // bEndpointAddress
                      0x03,                // bmAttributes (Interrupt)
                      KBD_EPSIZE,          // wMaxPacketSize
                      2),                  // bInterval

#ifdef MOUSE_ENABLE
    /* Interface Descriptor (9 bytes) USB spec 9.6.5, page 267-269, Table 9-12 */
    USB_DESC_INTERFACE(MOUSE_INTERFACE, // bInterfaceNumber
                       0,               // bAlternateSetting
                       1,               // bNumEndpoints
                       0x03,            // bInterfaceClass (0x03 = HID)
                       // ThinkPad T23 BIOS doesn't work with boot mouse.
                       0x00, // bInterfaceSubClass (0x01 = Boot)
                       0x00, // bInterfaceProtocol (0x02 = Mouse)
                       /*
                          0x01,      // bInterfaceSubClass (0x01 = Boot)
                          0x02,      // bInterfaceProtocol (0x02 = Mouse)
                        */
                       0), // iInterface

    /* HID descriptor (9 bytes) HID 1.11 spec, section 6.2.1 */
    USB_DESC_BYTE(9),                                  // bLength
    USB_DESC_BYTE(0x21),                               // bDescriptorType (HID class)
    USB_DESC_BCD(0x0111),                              // bcdHID: HID version 1.11
    USB_DESC_BYTE(0),                                  // bCountryCode
    USB_DESC_BYTE(1),                                  // bNumDescriptors
    USB_DESC_BYTE(0x22),                               // bDescriptorType (report desc)
    USB_DESC_WORD(sizeof(mouse_hid_report_desc_data)), // wDescriptorLength

    /* Endpoint Descriptor (7 bytes) USB spec 9.6.6, page 269-271, Table 9-13 */
    USB_DESC_ENDPOINT(MOUSE_ENDPOINT | 0x80, // bEndpointAddress
                      0x03,                  // bmAttributes (Interrupt)
                      MOUSE_EPSIZE,          // wMaxPacketSize
                      1),                    // bInterval
#endif                                       /* MOUSE_ENABLE */

#ifdef RAW_ENABLE   //!!
    /* Interface Descriptor (9 bytes) USB spec 9.6.5, page 267-269, Table 9-12 */
    USB_DESC_INTERFACE(RAW_INTERFACE, // bInterfaceNumber
                       0,                 // bAlternateSetting
                        2,                 // bNumEndpoints
//                       1,                 // bNumEndpoints
                       0x03,              // bInterfaceClass: HID
                       0x00,              // bInterfaceSubClass: None
                       0x00,              // bInterfaceProtocol: None
                       0),                // iInterface

    /* HID descriptor (9 bytes) HID 1.11 spec, section 6.2.1 */
    USB_DESC_BYTE(9),                                    // bLength
    USB_DESC_BYTE(0x21),                                 // bDescriptorType (HID class)
    USB_DESC_BCD(0x0111),                                // bcdHID: HID version 1.11
    USB_DESC_BYTE(0),                                    // bCountryCode
    USB_DESC_BYTE(1),                                    // bNumDescriptors
    USB_DESC_BYTE(0x22),                                 // bDescriptorType (report desc)
    USB_DESC_WORD(sizeof(raw_hid_report_desc_data)),     // wDescriptorLength

    /* Endpoint Descriptor (7 bytes) USB spec 9.6.6, page 269-271, Table 9-13 */
     USB_DESC_ENDPOINT(RAW_IN_ENDPOINT | 0x80,   // bEndpointAddress
                       0x03,                     // bmAttributes (Interrupt)
                       RAW_EPSIZE,               // wMaxPacketSize
                       //!!
                       1),                       // bInterval

    /* Endpoint Descriptor (7 bytes) USB spec 9.6.6, page 269-271, Table 9-13 */                      
    USB_DESC_ENDPOINT(RAW_OUT_ENDPOINT,         // bEndpointAddress
                      0x03,                     // bmAttributes (Interrupt)
                      RAW_EPSIZE,               // wMaxPacketSize
                      1),                       // bInterval

#endif                                          /* RAW_ENABLE */

#ifdef CONSOLE_ENABLE
    /* Interface Descriptor (9 bytes) USB spec 9.6.5, page 267-269, Table 9-12 */
    USB_DESC_INTERFACE(CONSOLE_INTERFACE, // bInterfaceNumber
                       0,                 // bAlternateSetting
                       1,                 // bNumEndpoints
                       0x03,              // bInterfaceClass: HID
                       0x00,              // bInterfaceSubClass: None
                       0x00,              // bInterfaceProtocol: None
                       0),                // iInterface

    /* HID descriptor (9 bytes) HID 1.11 spec, section 6.2.1 */
    USB_DESC_BYTE(9),                                    // bLength
    USB_DESC_BYTE(0x21),                                 // bDescriptorType (HID class)
    USB_DESC_BCD(0x0111),                                // bcdHID: HID version 1.11
    USB_DESC_BYTE(0),                                    // bCountryCode
    USB_DESC_BYTE(1),                                    // bNumDescriptors
    USB_DESC_BYTE(0x22),                                 // bDescriptorType (report desc)
    USB_DESC_WORD(sizeof(console_hid_report_desc_data)), // wDescriptorLength

    /* Endpoint Descriptor (7 bytes) USB spec 9.6.6, page 269-271, Table 9-13 */
    USB_DESC_ENDPOINT(CONSOLE_ENDPOINT | 0x80, // bEndpointAddress
                      0x03,                    // bmAttributes (Interrupt)
                      CONSOLE_EPSIZE,          // wMaxPacketSize
                      1),                      // bInterval
#endif                                         /* CONSOLE_ENABLE */

#ifdef EXTRAKEY_ENABLE
    /* Interface Descriptor (9 bytes) USB spec 9.6.5, page 267-269, Table 9-12 */
    USB_DESC_INTERFACE(EXTRA_INTERFACE, // bInterfaceNumber
                       0,               // bAlternateSetting
                       1,               // bNumEndpoints
                       0x03,            // bInterfaceClass: HID
                       0x00,            // bInterfaceSubClass: None
                       0x00,            // bInterfaceProtocol: None
                       0),              // iInterface

    /* HID descriptor (9 bytes) HID 1.11 spec, section 6.2.1 */
    USB_DESC_BYTE(9),                                  // bLength
    USB_DESC_BYTE(0x21),                               // bDescriptorType (HID class)
    USB_DESC_BCD(0x0111),                              // bcdHID: HID version 1.11
    USB_DESC_BYTE(0),                                  // bCountryCode
    USB_DESC_BYTE(1),                                  // bNumDescriptors
    USB_DESC_BYTE(0x22),                               // bDescriptorType (report desc)
    USB_DESC_WORD(sizeof(extra_hid_report_desc_data)), // wDescriptorLength

    /* Endpoint Descriptor (7 bytes) USB spec 9.6.6, page 269-271, Table 9-13 */
    USB_DESC_ENDPOINT(EXTRA_ENDPOINT | 0x80, // bEndpointAddress
                      0x03,                  // bmAttributes (Interrupt)
                      EXTRA_EPSIZE,          // wMaxPacketSize
                      2),                    // bInterval
#endif                                       /* EXTRAKEY_ENABLE */

#ifdef NKRO_ENABLE
    /* Interface Descriptor (9 bytes) USB spec 9.6.5, page 267-269, Table 9-12 */
    USB_DESC_INTERFACE(NKRO_INTERFACE, // bInterfaceNumber
                       0,              // bAlternateSetting
                       1,              // bNumEndpoints
                       0x03,           // bInterfaceClass: HID
                       0x00,           // bInterfaceSubClass: None
                       0x00,           // bInterfaceProtocol: None
                       0),             // iInterface

    /* HID descriptor (9 bytes) HID 1.11 spec, section 6.2.1 */
    USB_DESC_BYTE(9),                                 // bLength
    USB_DESC_BYTE(0x21),                              // bDescriptorType (HID class)
    USB_DESC_BCD(0x0111),                             // bcdHID: HID version 1.11
    USB_DESC_BYTE(0),                                 // bCountryCode
    USB_DESC_BYTE(1),                                 // bNumDescriptors
    USB_DESC_BYTE(0x22),                              // bDescriptorType (report desc)
    USB_DESC_WORD(sizeof(nkro_hid_report_desc_data)), // wDescriptorLength

    /* Endpoint Descriptor (7 bytes) USB spec 9.6.6, page 269-271, Table 9-13 */
    USB_DESC_ENDPOINT(NKRO_ENDPOINT | 0x80, // bEndpointAddress
                      0x03,                 // bmAttributes (Interrupt)
                      NKRO_EPSIZE,          // wMaxPacketSize
                      2),                   // bInterval
#endif                                      /* NKRO_ENABLE */
#ifdef LAMP_ARRAY_ENABLE
    /* Interface Descriptor (9 bytes) USB spec 9.6.5, page 267-269, Table 9-12 */
    USB_DESC_INTERFACE(LAMP_ARRAY_INTERFACE, // bInterfaceNumber
                       0,                    // bAlternateSetting
                       1,                    // bNumEndpoints
                       0x03,                 // bInterfaceClass: HID
                       0x00,                 // bInterfaceSubClass
                       0x00,                 // bInterfaceProtocol
                       0),                   // iInterface

    /* HID descriptor (9 bytes) HID 1.11 spec, section 6.2.1 */
    USB_DESC_BYTE(9),                                       // bLength
    USB_DESC_BYTE(0x21),                                    // bDescriptorType (HID class)
    USB_DESC_BCD(0x0111),                                   // bcdHID: HID version 1.11
    USB_DESC_BYTE(0),                                       // bCountryCode
    USB_DESC_BYTE(1),                                       // bNumDescriptors
    USB_DESC_BYTE(0x22),                                    // bDescriptorType (report desc)
    USB_DESC_WORD(sizeof(lamp_array_hid_report_desc_data)), // wDescriptorLength

    /* Endpoint Descriptor (7 bytes) USB spec 9.6.6, page 269-271, Table 9-13 */
    USB_DESC_ENDPOINT(LAMP_ARRAY_ENDPOINT | 0x80, // bEndpointAddress
                      0x03,                       // bmAttributes (Interrupt)
                      LAMP_ARRAY_EPSIZE,          // wMaxPacketSize
                      1),                         // bInterval

#endif /* LAMP_ARRAY */

#ifdef CONTROL_ENABLE
    /* Interface Descriptor (9 bytes) USB spec 9.6.5, page 267-269, Table 9-12 */
    //IAD_HID_IF_DESC_SET(6, 1, VENDOR_COMMUNICATION_INTERFACE_CLASS)
        USB_DESC_INTERFACE(CONTROL_INTERFACE, // bInterfaceNumber
                           0,                 // bAlternateSetting
                           1,                 // bNumEndpoints
                           0xff,              // bInterfaceClass: HID
                           0x00,              // bInterfaceSubClass
                           0x00,              // bInterfaceProtocol
                           0),                // iInterface

    /* HID descriptor (9 bytes) HID 1.11 spec, section 6.2.1 */
    //  USB_DESC_BYTE(9),            // bLength
    //  USB_DESC_BYTE(0x21),         // bDescriptorType (HID class)
    //  USB_DESC_BCD(0x0111),        // bcdHID: HID version 1.11
    //  USB_DESC_BYTE(0),            // bCountryCode
    //  USB_DESC_BYTE(1),            // bNumDescriptors
    //  USB_DESC_BYTE(0x22),         // bDescriptorType (report desc)
    //  USB_DESC_WORD(sizeof(control_hid_report_desc_data)), // wDescriptorLength

    /* Endpoint Descriptor (7 bytes) USB spec 9.6.6, page 269-271, Table 9-13 */
    USB_DESC_ENDPOINT(CONTROL_ENDPOINT | 0x80, // bEndpointAddress
                      0x03,                    // bmAttributes (Interrupt)
                      CONTROL_EPSIZE,          // wMaxPacketSize
                      1),                      // bInterval

#endif /* CONTROL */
};

static const uint8_t hid_configuration_descriptor_data_2000hz[] = {
    /* Configuration Descriptor (9 bytes) USB spec 9.6.3, page 264-266, Table 9-10 */
		USB_DESC_CONFIGURATION(CONFIG1_DESC_SIZE,
													 NUM_INTERFACES,
													 1,
													 0,
													 0xA0,
													 0xFA),                                                    // bMaxPower (500mA)

    //IAD_HID_IF_DESC_SET(0, 1, HID_COMMUNICATION_INTERFACE_CLASS)

    /* Interface Descriptor (9 bytes) USB spec 9.6.5, page 267-269, Table 9-12 */
    USB_DESC_INTERFACE(KBD_INTERFACE, // bInterfaceNumber
                       0,             // bAlternateSetting
                       1,             // bNumEndpoints
                       0x03,          // bInterfaceClass: HID
                       0x01,          // bInterfaceSubClass: Boot
                       0x01,          // bInterfaceProtocol: Keyboard
                       0),            // iInterface

    /* HID descriptor (9 bytes) HID 1.11 spec, section 6.2.1 */
    USB_DESC_BYTE(9),                                     // bLength
    USB_DESC_BYTE(0x21),                                  // bDescriptorType (HID class)
    USB_DESC_BCD(0x0111),                                 // bcdHID: HID version 1.11
    USB_DESC_BYTE(0),                                     // bCountryCode
    USB_DESC_BYTE(1),                                     // bNumDescriptors
    USB_DESC_BYTE(0x22),                                  // bDescriptorType (report desc)
    USB_DESC_WORD(sizeof(keyboard_hid_report_desc_data)), // wDescriptorLength

    /* Endpoint Descriptor (7 bytes) USB spec 9.6.6, page 269-271, Table 9-13 */
    USB_DESC_ENDPOINT(KBD_ENDPOINT | 0x80, // bEndpointAddress
                      0x03,                // bmAttributes (Interrupt)
                      KBD_EPSIZE,          // wMaxPacketSize
                      4),                  // bInterval

#ifdef MOUSE_ENABLE
    /* Interface Descriptor (9 bytes) USB spec 9.6.5, page 267-269, Table 9-12 */
    USB_DESC_INTERFACE(MOUSE_INTERFACE, // bInterfaceNumber
                       0,               // bAlternateSetting
                       1,               // bNumEndpoints
                       0x03,            // bInterfaceClass (0x03 = HID)
                       // ThinkPad T23 BIOS doesn't work with boot mouse.
                       0x00, // bInterfaceSubClass (0x01 = Boot)
                       0x00, // bInterfaceProtocol (0x02 = Mouse)
                       /*
                          0x01,      // bInterfaceSubClass (0x01 = Boot)
                          0x02,      // bInterfaceProtocol (0x02 = Mouse)
                        */
                       0), // iInterface

    /* HID descriptor (9 bytes) HID 1.11 spec, section 6.2.1 */
    USB_DESC_BYTE(9),                                  // bLength
    USB_DESC_BYTE(0x21),                               // bDescriptorType (HID class)
    USB_DESC_BCD(0x0111),                              // bcdHID: HID version 1.11
    USB_DESC_BYTE(0),                                  // bCountryCode
    USB_DESC_BYTE(1),                                  // bNumDescriptors
    USB_DESC_BYTE(0x22),                               // bDescriptorType (report desc)
    USB_DESC_WORD(sizeof(mouse_hid_report_desc_data)), // wDescriptorLength

    /* Endpoint Descriptor (7 bytes) USB spec 9.6.6, page 269-271, Table 9-13 */
    USB_DESC_ENDPOINT(MOUSE_ENDPOINT | 0x80, // bEndpointAddress
                      0x03,                  // bmAttributes (Interrupt)
                      MOUSE_EPSIZE,          // wMaxPacketSize
                      1),                    // bInterval
#endif                                       /* MOUSE_ENABLE */

#ifdef RAW_ENABLE   //!!
    /* Interface Descriptor (9 bytes) USB spec 9.6.5, page 267-269, Table 9-12 */
    USB_DESC_INTERFACE(RAW_INTERFACE, // bInterfaceNumber
                       0,                 // bAlternateSetting
                        2,                 // bNumEndpoints
//                       1,                 // bNumEndpoints
                       0x03,              // bInterfaceClass: HID
                       0x00,              // bInterfaceSubClass: None
                       0x00,              // bInterfaceProtocol: None
                       0),                // iInterface

    /* HID descriptor (9 bytes) HID 1.11 spec, section 6.2.1 */
    USB_DESC_BYTE(9),                                    // bLength
    USB_DESC_BYTE(0x21),                                 // bDescriptorType (HID class)
    USB_DESC_BCD(0x0111),                                // bcdHID: HID version 1.11
    USB_DESC_BYTE(0),                                    // bCountryCode
    USB_DESC_BYTE(1),                                    // bNumDescriptors
    USB_DESC_BYTE(0x22),                                 // bDescriptorType (report desc)
    USB_DESC_WORD(sizeof(raw_hid_report_desc_data)),     // wDescriptorLength

    /* Endpoint Descriptor (7 bytes) USB spec 9.6.6, page 269-271, Table 9-13 */
     USB_DESC_ENDPOINT(RAW_IN_ENDPOINT | 0x80,   // bEndpointAddress
                       0x03,                     // bmAttributes (Interrupt)
                       RAW_EPSIZE,               // wMaxPacketSize
                       //!!
                       1),                       // bInterval

    /* Endpoint Descriptor (7 bytes) USB spec 9.6.6, page 269-271, Table 9-13 */                      
    USB_DESC_ENDPOINT(RAW_OUT_ENDPOINT,         // bEndpointAddress
                      0x03,                     // bmAttributes (Interrupt)
                      RAW_EPSIZE,               // wMaxPacketSize
                      1),                       // bInterval

#endif                                          /* RAW_ENABLE */

#ifdef CONSOLE_ENABLE
    /* Interface Descriptor (9 bytes) USB spec 9.6.5, page 267-269, Table 9-12 */
    USB_DESC_INTERFACE(CONSOLE_INTERFACE, // bInterfaceNumber
                       0,                 // bAlternateSetting
                       1,                 // bNumEndpoints
                       0x03,              // bInterfaceClass: HID
                       0x00,              // bInterfaceSubClass: None
                       0x00,              // bInterfaceProtocol: None
                       0),                // iInterface

    /* HID descriptor (9 bytes) HID 1.11 spec, section 6.2.1 */
    USB_DESC_BYTE(9),                                    // bLength
    USB_DESC_BYTE(0x21),                                 // bDescriptorType (HID class)
    USB_DESC_BCD(0x0111),                                // bcdHID: HID version 1.11
    USB_DESC_BYTE(0),                                    // bCountryCode
    USB_DESC_BYTE(1),                                    // bNumDescriptors
    USB_DESC_BYTE(0x22),                                 // bDescriptorType (report desc)
    USB_DESC_WORD(sizeof(console_hid_report_desc_data)), // wDescriptorLength

    /* Endpoint Descriptor (7 bytes) USB spec 9.6.6, page 269-271, Table 9-13 */
    USB_DESC_ENDPOINT(CONSOLE_ENDPOINT | 0x80, // bEndpointAddress
                      0x03,                    // bmAttributes (Interrupt)
                      CONSOLE_EPSIZE,          // wMaxPacketSize
                      1),                      // bInterval
#endif                                         /* CONSOLE_ENABLE */

#ifdef EXTRAKEY_ENABLE
    /* Interface Descriptor (9 bytes) USB spec 9.6.5, page 267-269, Table 9-12 */
    USB_DESC_INTERFACE(EXTRA_INTERFACE, // bInterfaceNumber
                       0,               // bAlternateSetting
                       1,               // bNumEndpoints
                       0x03,            // bInterfaceClass: HID
                       0x00,            // bInterfaceSubClass: None
                       0x00,            // bInterfaceProtocol: None
                       0),              // iInterface

    /* HID descriptor (9 bytes) HID 1.11 spec, section 6.2.1 */
    USB_DESC_BYTE(9),                                  // bLength
    USB_DESC_BYTE(0x21),                               // bDescriptorType (HID class)
    USB_DESC_BCD(0x0111),                              // bcdHID: HID version 1.11
    USB_DESC_BYTE(0),                                  // bCountryCode
    USB_DESC_BYTE(1),                                  // bNumDescriptors
    USB_DESC_BYTE(0x22),                               // bDescriptorType (report desc)
    USB_DESC_WORD(sizeof(extra_hid_report_desc_data)), // wDescriptorLength

    /* Endpoint Descriptor (7 bytes) USB spec 9.6.6, page 269-271, Table 9-13 */
    USB_DESC_ENDPOINT(EXTRA_ENDPOINT | 0x80, // bEndpointAddress
                      0x03,                  // bmAttributes (Interrupt)
                      EXTRA_EPSIZE,          // wMaxPacketSize
                      2),                    // bInterval
#endif                                       /* EXTRAKEY_ENABLE */

#ifdef NKRO_ENABLE
    /* Interface Descriptor (9 bytes) USB spec 9.6.5, page 267-269, Table 9-12 */
    USB_DESC_INTERFACE(NKRO_INTERFACE, // bInterfaceNumber
                       0,              // bAlternateSetting
                       1,              // bNumEndpoints
                       0x03,           // bInterfaceClass: HID
                       0x00,           // bInterfaceSubClass: None
                       0x00,           // bInterfaceProtocol: None
                       0),             // iInterface

    /* HID descriptor (9 bytes) HID 1.11 spec, section 6.2.1 */
    USB_DESC_BYTE(9),                                 // bLength
    USB_DESC_BYTE(0x21),                              // bDescriptorType (HID class)
    USB_DESC_BCD(0x0111),                             // bcdHID: HID version 1.11
    USB_DESC_BYTE(0),                                 // bCountryCode
    USB_DESC_BYTE(1),                                 // bNumDescriptors
    USB_DESC_BYTE(0x22),                              // bDescriptorType (report desc)
    USB_DESC_WORD(sizeof(nkro_hid_report_desc_data)), // wDescriptorLength

    /* Endpoint Descriptor (7 bytes) USB spec 9.6.6, page 269-271, Table 9-13 */
    USB_DESC_ENDPOINT(NKRO_ENDPOINT | 0x80, // bEndpointAddress
                      0x03,                 // bmAttributes (Interrupt)
                      NKRO_EPSIZE,          // wMaxPacketSize
                      4),                   // bInterval
#endif                                      /* NKRO_ENABLE */
#ifdef LAMP_ARRAY_ENABLE
    /* Interface Descriptor (9 bytes) USB spec 9.6.5, page 267-269, Table 9-12 */
    USB_DESC_INTERFACE(LAMP_ARRAY_INTERFACE, // bInterfaceNumber
                       0,                    // bAlternateSetting
                       1,                    // bNumEndpoints
                       0x03,                 // bInterfaceClass: HID
                       0x00,                 // bInterfaceSubClass
                       0x00,                 // bInterfaceProtocol
                       0),                   // iInterface

    /* HID descriptor (9 bytes) HID 1.11 spec, section 6.2.1 */
    USB_DESC_BYTE(9),                                       // bLength
    USB_DESC_BYTE(0x21),                                    // bDescriptorType (HID class)
    USB_DESC_BCD(0x0111),                                   // bcdHID: HID version 1.11
    USB_DESC_BYTE(0),                                       // bCountryCode
    USB_DESC_BYTE(1),                                       // bNumDescriptors
    USB_DESC_BYTE(0x22),                                    // bDescriptorType (report desc)
    USB_DESC_WORD(sizeof(lamp_array_hid_report_desc_data)), // wDescriptorLength

    /* Endpoint Descriptor (7 bytes) USB spec 9.6.6, page 269-271, Table 9-13 */
    USB_DESC_ENDPOINT(LAMP_ARRAY_ENDPOINT | 0x80, // bEndpointAddress
                      0x03,                       // bmAttributes (Interrupt)
                      LAMP_ARRAY_EPSIZE,          // wMaxPacketSize
                      1),                         // bInterval

#endif /* LAMP_ARRAY */

#ifdef CONTROL_ENABLE
    /* Interface Descriptor (9 bytes) USB spec 9.6.5, page 267-269, Table 9-12 */
    //IAD_HID_IF_DESC_SET(6, 1, VENDOR_COMMUNICATION_INTERFACE_CLASS)
        USB_DESC_INTERFACE(CONTROL_INTERFACE, // bInterfaceNumber
                           0,                 // bAlternateSetting
                           1,                 // bNumEndpoints
                           0xff,              // bInterfaceClass: HID
                           0x00,              // bInterfaceSubClass
                           0x00,              // bInterfaceProtocol
                           0),                // iInterface

    /* HID descriptor (9 bytes) HID 1.11 spec, section 6.2.1 */
    //  USB_DESC_BYTE(9),            // bLength
    //  USB_DESC_BYTE(0x21),         // bDescriptorType (HID class)
    //  USB_DESC_BCD(0x0111),        // bcdHID: HID version 1.11
    //  USB_DESC_BYTE(0),            // bCountryCode
    //  USB_DESC_BYTE(1),            // bNumDescriptors
    //  USB_DESC_BYTE(0x22),         // bDescriptorType (report desc)
    //  USB_DESC_WORD(sizeof(control_hid_report_desc_data)), // wDescriptorLength

    /* Endpoint Descriptor (7 bytes) USB spec 9.6.6, page 269-271, Table 9-13 */
    USB_DESC_ENDPOINT(CONTROL_ENDPOINT | 0x80, // bEndpointAddress
                      0x03,                    // bmAttributes (Interrupt)
                      CONTROL_EPSIZE,          // wMaxPacketSize
                      1),                      // bInterval

#endif /* CONTROL */
};

static const uint8_t hid_configuration_descriptor_data_1000hz[] = {
    /* Configuration Descriptor (9 bytes) USB spec 9.6.3, page 264-266, Table 9-10 */
		USB_DESC_CONFIGURATION(CONFIG1_DESC_SIZE,
													 NUM_INTERFACES,
													 1,
													 0,
													 0xA0,
													 0xFA),                                                  // bMaxPower (500mA)

    //IAD_HID_IF_DESC_SET(0, 1, HID_COMMUNICATION_INTERFACE_CLASS)

    /* Interface Descriptor (9 bytes) USB spec 9.6.5, page 267-269, Table 9-12 */
    USB_DESC_INTERFACE(KBD_INTERFACE, // bInterfaceNumber
                       0,             // bAlternateSetting
                       1,             // bNumEndpoints
                       0x03,          // bInterfaceClass: HID
                       0x01,          // bInterfaceSubClass: Boot
                       0x01,          // bInterfaceProtocol: Keyboard
                       0),            // iInterface

    /* HID descriptor (9 bytes) HID 1.11 spec, section 6.2.1 */
    USB_DESC_BYTE(9),                                     // bLength
    USB_DESC_BYTE(0x21),                                  // bDescriptorType (HID class)
    USB_DESC_BCD(0x0111),                                 // bcdHID: HID version 1.11
    USB_DESC_BYTE(0),                                     // bCountryCode
    USB_DESC_BYTE(1),                                     // bNumDescriptors
    USB_DESC_BYTE(0x22),                                  // bDescriptorType (report desc)
    USB_DESC_WORD(sizeof(keyboard_hid_report_desc_data)), // wDescriptorLength

    /* Endpoint Descriptor (7 bytes) USB spec 9.6.6, page 269-271, Table 9-13 */
    USB_DESC_ENDPOINT(KBD_ENDPOINT | 0x80, // bEndpointAddress
                      0x03,                // bmAttributes (Interrupt)
                      KBD_EPSIZE,          // wMaxPacketSize
                      8),                  // bInterval

#ifdef MOUSE_ENABLE
    /* Interface Descriptor (9 bytes) USB spec 9.6.5, page 267-269, Table 9-12 */
    USB_DESC_INTERFACE(MOUSE_INTERFACE, // bInterfaceNumber
                       0,               // bAlternateSetting
                       1,               // bNumEndpoints
                       0x03,            // bInterfaceClass (0x03 = HID)
                       // ThinkPad T23 BIOS doesn't work with boot mouse.
                       0x00, // bInterfaceSubClass (0x01 = Boot)
                       0x00, // bInterfaceProtocol (0x02 = Mouse)
                       /*
                          0x01,      // bInterfaceSubClass (0x01 = Boot)
                          0x02,      // bInterfaceProtocol (0x02 = Mouse)
                        */
                       0), // iInterface

    /* HID descriptor (9 bytes) HID 1.11 spec, section 6.2.1 */
    USB_DESC_BYTE(9),                                  // bLength
    USB_DESC_BYTE(0x21),                               // bDescriptorType (HID class)
    USB_DESC_BCD(0x0111),                              // bcdHID: HID version 1.11
    USB_DESC_BYTE(0),                                  // bCountryCode
    USB_DESC_BYTE(1),                                  // bNumDescriptors
    USB_DESC_BYTE(0x22),                               // bDescriptorType (report desc)
    USB_DESC_WORD(sizeof(mouse_hid_report_desc_data)), // wDescriptorLength

    /* Endpoint Descriptor (7 bytes) USB spec 9.6.6, page 269-271, Table 9-13 */
    USB_DESC_ENDPOINT(MOUSE_ENDPOINT | 0x80, // bEndpointAddress
                      0x03,                  // bmAttributes (Interrupt)
                      MOUSE_EPSIZE,          // wMaxPacketSize
                      1),                    // bInterval
#endif                                       /* MOUSE_ENABLE */

#ifdef RAW_ENABLE   //!!
    /* Interface Descriptor (9 bytes) USB spec 9.6.5, page 267-269, Table 9-12 */
    USB_DESC_INTERFACE(RAW_INTERFACE, // bInterfaceNumber
                       0,                 // bAlternateSetting
                        2,                 // bNumEndpoints
//                       1,                 // bNumEndpoints
                       0x03,              // bInterfaceClass: HID
                       0x00,              // bInterfaceSubClass: None
                       0x00,              // bInterfaceProtocol: None
                       0),                // iInterface

    /* HID descriptor (9 bytes) HID 1.11 spec, section 6.2.1 */
    USB_DESC_BYTE(9),                                    // bLength
    USB_DESC_BYTE(0x21),                                 // bDescriptorType (HID class)
    USB_DESC_BCD(0x0111),                                // bcdHID: HID version 1.11
    USB_DESC_BYTE(0),                                    // bCountryCode
    USB_DESC_BYTE(1),                                    // bNumDescriptors
    USB_DESC_BYTE(0x22),                                 // bDescriptorType (report desc)
    USB_DESC_WORD(sizeof(raw_hid_report_desc_data)),     // wDescriptorLength

    /* Endpoint Descriptor (7 bytes) USB spec 9.6.6, page 269-271, Table 9-13 */
     USB_DESC_ENDPOINT(RAW_IN_ENDPOINT | 0x80,   // bEndpointAddress
                       0x03,                     // bmAttributes (Interrupt)
                       RAW_EPSIZE,               // wMaxPacketSize
                       //!!
                       1),                       // bInterval

    /* Endpoint Descriptor (7 bytes) USB spec 9.6.6, page 269-271, Table 9-13 */                      
    USB_DESC_ENDPOINT(RAW_OUT_ENDPOINT,         // bEndpointAddress
                      0x03,                     // bmAttributes (Interrupt)
                      RAW_EPSIZE,               // wMaxPacketSize
                      1),                       // bInterval

#endif                                          /* RAW_ENABLE */

#ifdef CONSOLE_ENABLE
    /* Interface Descriptor (9 bytes) USB spec 9.6.5, page 267-269, Table 9-12 */
    USB_DESC_INTERFACE(CONSOLE_INTERFACE, // bInterfaceNumber
                       0,                 // bAlternateSetting
                       1,                 // bNumEndpoints
                       0x03,              // bInterfaceClass: HID
                       0x00,              // bInterfaceSubClass: None
                       0x00,              // bInterfaceProtocol: None
                       0),                // iInterface

    /* HID descriptor (9 bytes) HID 1.11 spec, section 6.2.1 */
    USB_DESC_BYTE(9),                                    // bLength
    USB_DESC_BYTE(0x21),                                 // bDescriptorType (HID class)
    USB_DESC_BCD(0x0111),                                // bcdHID: HID version 1.11
    USB_DESC_BYTE(0),                                    // bCountryCode
    USB_DESC_BYTE(1),                                    // bNumDescriptors
    USB_DESC_BYTE(0x22),                                 // bDescriptorType (report desc)
    USB_DESC_WORD(sizeof(console_hid_report_desc_data)), // wDescriptorLength

    /* Endpoint Descriptor (7 bytes) USB spec 9.6.6, page 269-271, Table 9-13 */
    USB_DESC_ENDPOINT(CONSOLE_ENDPOINT | 0x80, // bEndpointAddress
                      0x03,                    // bmAttributes (Interrupt)
                      CONSOLE_EPSIZE,          // wMaxPacketSize
                      1),                      // bInterval
#endif                                         /* CONSOLE_ENABLE */

#ifdef EXTRAKEY_ENABLE
    /* Interface Descriptor (9 bytes) USB spec 9.6.5, page 267-269, Table 9-12 */
    USB_DESC_INTERFACE(EXTRA_INTERFACE, // bInterfaceNumber
                       0,               // bAlternateSetting
                       1,               // bNumEndpoints
                       0x03,            // bInterfaceClass: HID
                       0x00,            // bInterfaceSubClass: None
                       0x00,            // bInterfaceProtocol: None
                       0),              // iInterface

    /* HID descriptor (9 bytes) HID 1.11 spec, section 6.2.1 */
    USB_DESC_BYTE(9),                                  // bLength
    USB_DESC_BYTE(0x21),                               // bDescriptorType (HID class)
    USB_DESC_BCD(0x0111),                              // bcdHID: HID version 1.11
    USB_DESC_BYTE(0),                                  // bCountryCode
    USB_DESC_BYTE(1),                                  // bNumDescriptors
    USB_DESC_BYTE(0x22),                               // bDescriptorType (report desc)
    USB_DESC_WORD(sizeof(extra_hid_report_desc_data)), // wDescriptorLength

    /* Endpoint Descriptor (7 bytes) USB spec 9.6.6, page 269-271, Table 9-13 */
    USB_DESC_ENDPOINT(EXTRA_ENDPOINT | 0x80, // bEndpointAddress
                      0x03,                  // bmAttributes (Interrupt)
                      EXTRA_EPSIZE,          // wMaxPacketSize
                      2),                    // bInterval
#endif                                       /* EXTRAKEY_ENABLE */

#ifdef NKRO_ENABLE
    /* Interface Descriptor (9 bytes) USB spec 9.6.5, page 267-269, Table 9-12 */
    USB_DESC_INTERFACE(NKRO_INTERFACE, // bInterfaceNumber
                       0,              // bAlternateSetting
                       1,              // bNumEndpoints
                       0x03,           // bInterfaceClass: HID
                       0x00,           // bInterfaceSubClass: None
                       0x00,           // bInterfaceProtocol: None
                       0),             // iInterface

    /* HID descriptor (9 bytes) HID 1.11 spec, section 6.2.1 */
    USB_DESC_BYTE(9),                                 // bLength
    USB_DESC_BYTE(0x21),                              // bDescriptorType (HID class)
    USB_DESC_BCD(0x0111),                             // bcdHID: HID version 1.11
    USB_DESC_BYTE(0),                                 // bCountryCode
    USB_DESC_BYTE(1),                                 // bNumDescriptors
    USB_DESC_BYTE(0x22),                              // bDescriptorType (report desc)
    USB_DESC_WORD(sizeof(nkro_hid_report_desc_data)), // wDescriptorLength

    /* Endpoint Descriptor (7 bytes) USB spec 9.6.6, page 269-271, Table 9-13 */
    USB_DESC_ENDPOINT(NKRO_ENDPOINT | 0x80, // bEndpointAddress
                      0x03,                 // bmAttributes (Interrupt)
                      NKRO_EPSIZE,          // wMaxPacketSize
                      8),                   // bInterval
#endif                                      /* NKRO_ENABLE */
#ifdef LAMP_ARRAY_ENABLE
    /* Interface Descriptor (9 bytes) USB spec 9.6.5, page 267-269, Table 9-12 */
    USB_DESC_INTERFACE(LAMP_ARRAY_INTERFACE, // bInterfaceNumber
                       0,                    // bAlternateSetting
                       1,                    // bNumEndpoints
                       0x03,                 // bInterfaceClass: HID
                       0x00,                 // bInterfaceSubClass
                       0x00,                 // bInterfaceProtocol
                       0),                   // iInterface

    /* HID descriptor (9 bytes) HID 1.11 spec, section 6.2.1 */
    USB_DESC_BYTE(9),                                       // bLength
    USB_DESC_BYTE(0x21),                                    // bDescriptorType (HID class)
    USB_DESC_BCD(0x0111),                                   // bcdHID: HID version 1.11
    USB_DESC_BYTE(0),                                       // bCountryCode
    USB_DESC_BYTE(1),                                       // bNumDescriptors
    USB_DESC_BYTE(0x22),                                    // bDescriptorType (report desc)
    USB_DESC_WORD(sizeof(lamp_array_hid_report_desc_data)), // wDescriptorLength

    /* Endpoint Descriptor (7 bytes) USB spec 9.6.6, page 269-271, Table 9-13 */
    USB_DESC_ENDPOINT(LAMP_ARRAY_ENDPOINT | 0x80, // bEndpointAddress
                      0x03,                       // bmAttributes (Interrupt)
                      LAMP_ARRAY_EPSIZE,          // wMaxPacketSize
                      1),                         // bInterval

#endif /* LAMP_ARRAY */

#ifdef CONTROL_ENABLE
    /* Interface Descriptor (9 bytes) USB spec 9.6.5, page 267-269, Table 9-12 */
    //IAD_HID_IF_DESC_SET(6, 1, VENDOR_COMMUNICATION_INTERFACE_CLASS)
        USB_DESC_INTERFACE(CONTROL_INTERFACE, // bInterfaceNumber
                           0,                 // bAlternateSetting
                           1,                 // bNumEndpoints
                           0xff,              // bInterfaceClass: HID
                           0x00,              // bInterfaceSubClass
                           0x00,              // bInterfaceProtocol
                           0),                // iInterface

    /* HID descriptor (9 bytes) HID 1.11 spec, section 6.2.1 */
    //  USB_DESC_BYTE(9),            // bLength
    //  USB_DESC_BYTE(0x21),         // bDescriptorType (HID class)
    //  USB_DESC_BCD(0x0111),        // bcdHID: HID version 1.11
    //  USB_DESC_BYTE(0),            // bCountryCode
    //  USB_DESC_BYTE(1),            // bNumDescriptors
    //  USB_DESC_BYTE(0x22),         // bDescriptorType (report desc)
    //  USB_DESC_WORD(sizeof(control_hid_report_desc_data)), // wDescriptorLength

    /* Endpoint Descriptor (7 bytes) USB spec 9.6.6, page 269-271, Table 9-13 */
    USB_DESC_ENDPOINT(CONTROL_ENDPOINT | 0x80, // bEndpointAddress
                      0x03,                    // bmAttributes (Interrupt)
                      CONTROL_EPSIZE,          // wMaxPacketSize
                      1),                      // bInterval

#endif /* CONTROL */
};

/* Configuration Descriptor wrapper */
// static const USBDescriptor hid_configuration_descriptor = {
//     sizeof hid_configuration_descriptor_data,
//     hid_configuration_descriptor_data};




static const USBDescriptor hid_configuration_descriptor_8000hz = {
    sizeof hid_configuration_descriptor_data_8000hz,
    hid_configuration_descriptor_data_8000hz
};

static const USBDescriptor hid_configuration_descriptor_4000hz = {
    sizeof hid_configuration_descriptor_data_4000hz,
    hid_configuration_descriptor_data_4000hz
};

static const USBDescriptor hid_configuration_descriptor_2000hz = {
    sizeof hid_configuration_descriptor_data_2000hz,
    hid_configuration_descriptor_data_2000hz
};

static const USBDescriptor hid_configuration_descriptor_1000hz = {
    sizeof hid_configuration_descriptor_data_1000hz,
    hid_configuration_descriptor_data_1000hz
};

// active descriptor pointer
//static const USBDescriptor *active_hid_configuration_descriptor =
//    &hid_configuration_descriptor_8000hz;

/* wrappers */
#define HID_DESCRIPTOR_SIZE 9
// static const USBDescriptor keyboard_hid_descriptor = {
//     HID_DESCRIPTOR_SIZE,
//     &hid_configuration_descriptor_data[KBD_HID_DESC_OFFSET]};
// #ifdef MOUSE_ENABLE
// static const USBDescriptor mouse_hid_descriptor = {
//     HID_DESCRIPTOR_SIZE,
//     &hid_configuration_descriptor_data[MOUSE_HID_DESC_OFFSET]};
// #endif /* MOUSE_ENABLE */
// #ifdef RAW_ENABLE
// static const USBDescriptor raw_hid_descriptor = {
//     HID_DESCRIPTOR_SIZE,
//     &hid_configuration_descriptor_data[RAW_HID_DESC_OFFSET]};
// #endif /* RAW_ENABLE */

// #ifdef CONSOLE_ENABLE
// static const USBDescriptor console_hid_descriptor = {
//     HID_DESCRIPTOR_SIZE,
//     &hid_configuration_descriptor_data[CONSOLE_HID_DESC_OFFSET]};
// #endif /* CONSOLE_ENABLE */
// #ifdef EXTRAKEY_ENABLE
// static const USBDescriptor extra_hid_descriptor = {
//     HID_DESCRIPTOR_SIZE,
//     &hid_configuration_descriptor_data[EXTRA_HID_DESC_OFFSET]};
// #endif /* EXTRAKEY_ENABLE */
// #ifdef NKRO_ENABLE
// static const USBDescriptor nkro_hid_descriptor = {
//     HID_DESCRIPTOR_SIZE,
//     &hid_configuration_descriptor_data[NKRO_HID_DESC_OFFSET]};
// #endif /* NKRO_ENABLE */

// #ifdef LAMP_ARRAY_ENABLE
// static const USBDescriptor lamp_array_hid_descriptor = {
//     HID_DESCRIPTOR_SIZE,
//     &hid_configuration_descriptor_data[LAMP_ARRAY_HID_DESC_OFFSET]};
// #endif /* NKRO_ENABLE */

// #ifdef CONTROL_ENABLE
// static const USBDescriptor control_hid_descriptor = {
//     HID_DESCRIPTOR_SIZE,
//     &hid_configuration_descriptor_data[CONTROL_HID_DESC_OFFSET]};
// #endif /* NKRO_ENABLE */


// static USBDescriptor active_hid_subdescriptor;

// static const USBDescriptor *get_active_hid_descriptor_by_offset(uint16_t offset)
// {
//     active_hid_subdescriptor.ud_size = HID_DESCRIPTOR_SIZE;
//     active_hid_subdescriptor.ud_string =
//         &active_hid_configuration_descriptor->ud_string[offset];
//     return &active_hid_subdescriptor;
// }


// switch (g_usb_poll_mode) { // this is another solution
//     case USB_POLL_8000:
//         return &hid_configuration_descriptor_8000hz;
//     case USB_POLL_4000:
//         return &hid_configuration_descriptor_4000hz;
//     case USB_POLL_2000:
//         return &hid_configuration_descriptor_2000hz;
//     case USB_POLL_1000:
//         return &hid_configuration_descriptor_1000hz;
//     default:
//         return &hid_configuration_descriptor_8000hz;
// }

/* U.S. English language identifier */
static const uint8_t usb_string_langid[] = {
    USB_DESC_BYTE(4),                     // bLength
    USB_DESC_BYTE(USB_DESCRIPTOR_STRING), // bDescriptorType
    USB_DESC_WORD(0x0409)                 // wLANGID (U.S. English)
};

/* ugly ugly hack */
#define PP_NARG(...) \
    PP_NARG_(__VA_ARGS__, PP_RSEQ_N())
#define PP_NARG_(...) \
    PP_ARG_N(__VA_ARGS__)
#define PP_ARG_N(                                     \
    _1, _2, _3, _4, _5, _6, _7, _8, _9, _10,          \
    _11, _12, _13, _14, _15, _16, _17, _18, _19, _20, \
    _21, _22, _23, _24, _25, _26, _27, _28, _29, _30, \
    _31, _32, _33, _34, _35, _36, _37, _38, _39, _40, \
    _41, _42, _43, _44, _45, _46, _47, _48, _49, _50, \
    _51, _52, _53, _54, _55, _56, _57, _58, _59, _60, \
    _61, _62, _63, N, ...) N
#define PP_RSEQ_N()                             \
    63, 62, 61, 60,                             \
        59, 58, 57, 56, 55, 54, 53, 52, 51, 50, \
        49, 48, 47, 46, 45, 44, 43, 42, 41, 40, \
        39, 38, 37, 36, 35, 34, 33, 32, 31, 30, \
        29, 28, 27, 26, 25, 24, 23, 22, 21, 20, \
        19, 18, 17, 16, 15, 14, 13, 12, 11, 10, \
        9, 8, 7, 6, 5, 4, 3, 2, 1, 0

/* Vendor string = manufacturer */
static const uint8_t usb_string_vendor[] = {
    USB_DESC_BYTE(PP_NARG(USBSTR_MANUFACTURER) + 2), // bLength
    USB_DESC_BYTE(USB_DESCRIPTOR_STRING),            // bDescriptorType
    USBSTR_MANUFACTURER};

/* Device Description string = product */
static const uint8_t usb_string_description[] = {
    USB_DESC_BYTE(PP_NARG(USBSTR_PRODUCT) + 2), // bLength
    USB_DESC_BYTE(USB_DESCRIPTOR_STRING),       // bDescriptorType
    USBSTR_PRODUCT};

/* Serial Number string (will be filled by the function init_usb_serial_string) */
static uint8_t usb_string_serial[] = {
    USB_DESC_BYTE(PP_NARG(USBSTR_SERIAL) + 2), // bLength
    USB_DESC_BYTE(USB_DESCRIPTOR_STRING),      // bDescriptorType
    USBSTR_SERIAL};

/* Strings wrappers array */
static const USBDescriptor usb_strings[] = {
    {sizeof usb_string_langid, usb_string_langid},
    {sizeof usb_string_vendor, usb_string_vendor},
    {sizeof usb_string_description, usb_string_description},
    {sizeof usb_string_serial, usb_string_serial}};

/*
 * Handles the GET_DESCRIPTOR callback
 *
 * Returns the proper descriptor
 */
static const USBDescriptor *usb_get_descriptor_cb(
    USBDriver *usbp,
    uint8_t dtype,
    uint8_t dindex,
    uint16_t lang
)
{
    (void)usbp;

    switch (dtype)
    {
        /* Generic descriptors */
        case USB_DESCRIPTOR_DEVICE:
            return &usb_device_descriptor;
				
				case USB_DESCRIPTOR_DEVICE_QUALIFIER:
						return &usb_device_qualifier_descriptor;

        case USB_DESCRIPTOR_CONFIGURATION:
            switch (g_usb_poll_mode)
            {
                case USB_POLL_8000:
                    return &hid_configuration_descriptor_8000hz;

                case USB_POLL_4000:
                    return &hid_configuration_descriptor_4000hz;

                case USB_POLL_2000:
                    return &hid_configuration_descriptor_2000hz;

                case USB_POLL_1000:
                    return &hid_configuration_descriptor_1000hz;

                default:
                    return &hid_configuration_descriptor_8000hz;
            }

        case USB_DESCRIPTOR_STRING:
            if (dindex < (sizeof(usb_strings) / sizeof(usb_strings[0])))
            {
                return &usb_strings[dindex];
            }

            /* Microsoft OS String Descriptor is only valid at index 0xEE. */
            if (dindex == 0xEEU)
            {
                return &usb_device_OSdescriptor;
            }

            return NULL;

        /* HID specific descriptors */
        case USB_DESCRIPTOR_HID:
            /*
             * lang is actually wIndex here, which contains the
             * requested HID interface number.
             */
            switch (lang)
            {
                case KBD_INTERFACE:
                    switch (g_usb_poll_mode)
                    {
                        case USB_POLL_8000:
                            return &hid_configuration_descriptor_8000hz;

                        case USB_POLL_4000:
                            return &hid_configuration_descriptor_4000hz;

                        case USB_POLL_2000:
                            return &hid_configuration_descriptor_2000hz;

                        case USB_POLL_1000:
                            return &hid_configuration_descriptor_1000hz;

                        default:
                            return &hid_configuration_descriptor_8000hz;
                    }

#ifdef MOUSE_ENABLE
                case MOUSE_INTERFACE:
                    switch (g_usb_poll_mode)
                    {
                        case USB_POLL_8000:
                            return &hid_configuration_descriptor_8000hz;

                        case USB_POLL_4000:
                            return &hid_configuration_descriptor_4000hz;

                        case USB_POLL_2000:
                            return &hid_configuration_descriptor_2000hz;

                        case USB_POLL_1000:
                            return &hid_configuration_descriptor_1000hz;

                        default:
                            return &hid_configuration_descriptor_8000hz;
                    }
#endif /* MOUSE_ENABLE */

#ifdef RAW_ENABLE
                case RAW_INTERFACE:
                    switch (g_usb_poll_mode)
                    {
                        case USB_POLL_8000:
                            return &hid_configuration_descriptor_8000hz;

                        case USB_POLL_4000:
                            return &hid_configuration_descriptor_4000hz;

                        case USB_POLL_2000:
                            return &hid_configuration_descriptor_2000hz;

                        case USB_POLL_1000:
                            return &hid_configuration_descriptor_1000hz;

                        default:
                            return &hid_configuration_descriptor_8000hz;
                    }
#endif /* RAW_ENABLE */

#ifdef CONSOLE_ENABLE
                case CONSOLE_INTERFACE:
                    switch (g_usb_poll_mode)
                    {
                        case USB_POLL_8000:
                            return &hid_configuration_descriptor_8000hz;

                        case USB_POLL_4000:
                            return &hid_configuration_descriptor_4000hz;

                        case USB_POLL_2000:
                            return &hid_configuration_descriptor_2000hz;

                        case USB_POLL_1000:
                            return &hid_configuration_descriptor_1000hz;

                        default:
                            return &hid_configuration_descriptor_8000hz;
                    }
#endif /* CONSOLE_ENABLE */

#ifdef EXTRAKEY_ENABLE
                case EXTRA_INTERFACE:
                    switch (g_usb_poll_mode)
                    {
                        case USB_POLL_8000:
                            return &hid_configuration_descriptor_8000hz;

                        case USB_POLL_4000:
                            return &hid_configuration_descriptor_4000hz;

                        case USB_POLL_2000:
                            return &hid_configuration_descriptor_2000hz;

                        case USB_POLL_1000:
                            return &hid_configuration_descriptor_1000hz;

                        default:
                            return &hid_configuration_descriptor_8000hz;
                    }
#endif /* EXTRAKEY_ENABLE */

#ifdef NKRO_ENABLE
                case NKRO_INTERFACE:
                    switch (g_usb_poll_mode)
                    {
                        case USB_POLL_8000:
                            return &hid_configuration_descriptor_8000hz;

                        case USB_POLL_4000:
                            return &hid_configuration_descriptor_4000hz;

                        case USB_POLL_2000:
                            return &hid_configuration_descriptor_2000hz;

                        case USB_POLL_1000:
                            return &hid_configuration_descriptor_1000hz;

                        default:
                            return &hid_configuration_descriptor_8000hz;
                    }
#endif /* NKRO_ENABLE */

#ifdef LAMP_ARRAY_ENABLE
                case LAMP_ARRAY_INTERFACE:
                    switch (g_usb_poll_mode)
                    {
                        case USB_POLL_8000:
                            return &hid_configuration_descriptor_8000hz;

                        case USB_POLL_4000:
                            return &hid_configuration_descriptor_4000hz;

                        case USB_POLL_2000:
                            return &hid_configuration_descriptor_2000hz;

                        case USB_POLL_1000:
                            return &hid_configuration_descriptor_1000hz;

                        default:
                            return &hid_configuration_descriptor_8000hz;
                    }
#endif /* LAMP_ARRAY_ENABLE */

#ifdef CONTROL_ENABLE
                case CONTROL_INTERFACE:
                    switch (g_usb_poll_mode)
                    {
                        case USB_POLL_8000:
                            return &hid_configuration_descriptor_8000hz;

                        case USB_POLL_4000:
                            return &hid_configuration_descriptor_4000hz;

                        case USB_POLL_2000:
                            return &hid_configuration_descriptor_2000hz;

                        case USB_POLL_1000:
                            return &hid_configuration_descriptor_1000hz;

                        default:
                            return &hid_configuration_descriptor_8000hz;
                    }
#endif /* CONTROL_ENABLE */

                default:
                    break;
            }

            /*
             * Important:
             * Do not fall through into USB_DESCRIPTOR_HID_REPORT.
             */
            break;

        case USB_DESCRIPTOR_HID_REPORT:
            switch (lang)
            {
                case KBD_INTERFACE:
                    return &keyboard_hid_report_descriptor;

#ifdef MOUSE_ENABLE
                case MOUSE_INTERFACE:
                    return &mouse_hid_report_descriptor;
#endif /* MOUSE_ENABLE */

#ifdef RAW_ENABLE
                case RAW_INTERFACE:
                    return &raw_hid_report_descriptor;
#endif /* RAW_ENABLE */

#ifdef CONSOLE_ENABLE
                case CONSOLE_INTERFACE:
                    return &console_hid_report_descriptor;
#endif /* CONSOLE_ENABLE */

#ifdef EXTRAKEY_ENABLE
                case EXTRA_INTERFACE:
                    return &extra_hid_report_descriptor;
#endif /* EXTRAKEY_ENABLE */

#ifdef NKRO_ENABLE
                case NKRO_INTERFACE:
                    return &nkro_hid_report_descriptor;
#endif /* NKRO_ENABLE */

#ifdef LAMP_ARRAY_ENABLE
                case LAMP_ARRAY_INTERFACE:
                    return &lamp_array_hid_report_descriptor;
#endif /* LAMP_ARRAY_ENABLE */

#ifdef CONTROL_ENABLE
                case CONTROL_INTERFACE:
                    return &control_hid_report_descriptor;
#endif /* CONTROL_ENABLE */

                default:
                    break;
            }
            break;

        default:
            break;
    }

    return NULL;
}

/* keyboard endpoint state structure */
static USBInEndpointState kbd_ep_state;
/* keyboard endpoint initialization structure (IN) */
static const USBEndpointConfig kbd_ep_config = {
    USB_EP_MODE_TYPE_INTR, /* Interrupt EP */
    NULL,                  /* SETUP packet notification callback */
    kbd_in_cb,             /* IN notification callback */
    NULL,                  /* OUT notification callback */
    KBD_EPSIZE,            /* IN maximum packet size */
    0,                     /* OUT maximum packet size */
    &kbd_ep_state,         /* IN Endpoint state */
    NULL,                  /* OUT endpoint state */
    2,                     /* IN multiplier */
    NULL                   /* SETUP buffer (not a SETUP endpoint) */
};

#ifdef MOUSE_ENABLE
/* mouse endpoint state structure */
static USBInEndpointState mouse_ep_state;

/* mouse endpoint initialization structure (IN) */
static const USBEndpointConfig mouse_ep_config = {
    USB_EP_MODE_TYPE_INTR, /* Interrupt EP */
    NULL,                  /* SETUP packet notification callback */
    mouse_in_cb,           /* IN notification callback */
    NULL,                  /* OUT notification callback */
    MOUSE_EPSIZE,          /* IN maximum packet size */
    0,                     /* OUT maximum packet size */
    &mouse_ep_state,       /* IN Endpoint state */
    NULL,                  /* OUT endpoint state */
    2,                     /* IN multiplier */
    NULL                   /* SETUP buffer (not a SETUP endpoint) */
};
#endif /* MOUSE_ENABLE */

#ifdef RAW_ENABLE
/* raw endpoint state structure */
static USBInEndpointState raw_in_ep_state;
static USBOutEndpointState raw_out_ep_state;

/* raw endpoint initialization structure (IN/OUT) */
static const USBEndpointConfig raw_ep_config = {
    USB_EP_MODE_TYPE_INTR, /* Interrupt EP */
    NULL,                  /* SETUP packet notification callback */
    raw_in_cb,         /* IN notification callback */
    raw_out_cb,        /* OUT notification callback */
    RAW_EPSIZE,        /* IN maximum packet size */
    RAW_EPSIZE,        /* OUT maximum packet size */
    &raw_in_ep_state,     /* IN Endpoint state */
    &raw_out_ep_state,     /* OUT endpoint state */
    2,                     /* IN multiplier */
    NULL                   /* SETUP buffer (not a SETUP endpoint) */
};
// static const USBEndpointConfig raw_ep_in_config = {
//     USB_EP_MODE_TYPE_INTR, /* Interrupt EP */
//     NULL,                  /* SETUP packet notification callback */
//     raw_in_cb,             /* IN notification callback */
//     NULL,                  /* OUT notification callback */
//     RAW_EPSIZE,            /* IN maximum packet size */
//     0,                     /* OUT maximum packet size */
//     &raw_in_ep_state,      /* IN Endpoint state */
//     NULL,                  /* OUT endpoint state */
//     2,                     /* IN multiplier */
//     NULL                   /* SETUP buffer (not a SETUP endpoint) */
// };
// static const USBEndpointConfig raw_ep_out_config = {
//     USB_EP_MODE_TYPE_INTR, /* Interrupt EP */
//     NULL,                  /* SETUP packet notification callback */
//     NULL,                  /* IN notification callback */
//     raw_out_cb,            /* OUT notification callback */
//     0,                     /* IN maximum packet size */
//     RAW_EPSIZE,            /* OUT maximum packet size */
//     NULL,                  /* IN Endpoint state */
//     &raw_out_ep_state,     /* OUT endpoint state */
//     2,                     /* IN multiplier */
//     NULL                   /* SETUP buffer (not a SETUP endpoint) */
// };
#endif /* RAW_ENABLE */

//#ifdef RAW_ENABLE
///* console endpoint state structure */
//static USBInEndpointState raw_ep_in_state;
//static USBInEndpointState raw_ep_out_state;
//
///* console endpoint initialization structure (IN) */
//static const USBEndpointConfig raw_ep_in_config = {
//    USB_EP_MODE_TYPE_INTR, /* Interrupt EP */
//    NULL,                  /* SETUP packet notification callback */
//    raw_in_cb,         /* IN notification callback */
//    NULL,                  /* OUT notification callback */
//    RAW_EPSIZE,        /* IN maximum packet size */
//    0,                     /* OUT maximum packet size */
//    &raw_ep_in_state,     /* IN Endpoint state */
//    NULL,                  /* OUT endpoint state */
//    2,                     /* IN multiplier */
//    NULL                   /* SETUP buffer (not a SETUP endpoint) */
//};
//static const USBEndpointConfig raw_ep_out_config = {
//    USB_EP_MODE_TYPE_INTR, /* Interrupt EP */
//    NULL,                  /* SETUP packet notification callback */
//    raw_out_cb,         /* IN notification callback */
//    NULL,                  /* OUT notification callback */
//    RAW_EPSIZE,        /* IN maximum packet size */
//    0,                     /* OUT maximum packet size */
//    &raw_ep_out_state,     /* IN Endpoint state */
//    NULL,                  /* OUT endpoint state */
//    2,                     /* IN multiplier */
//    NULL                   /* SETUP buffer (not a SETUP endpoint) */
//};
//#endif /* RAW_ENABLE */

#ifdef CONSOLE_ENABLE
/* console endpoint state structure */
static USBInEndpointState console_ep_state;

/* console endpoint initialization structure (IN) */
static const USBEndpointConfig console_ep_config = {
    USB_EP_MODE_TYPE_INTR, /* Interrupt EP */
    NULL,                  /* SETUP packet notification callback */
    console_in_cb,         /* IN notification callback */
    NULL,                  /* OUT notification callback */
    CONSOLE_EPSIZE,        /* IN maximum packet size */
    0,                     /* OUT maximum packet size */
    &console_ep_state,     /* IN Endpoint state */
    NULL,                  /* OUT endpoint state */
    2,                     /* IN multiplier */
    NULL                   /* SETUP buffer (not a SETUP endpoint) */
};
#endif /* CONSOLE_ENABLE */

#ifdef EXTRAKEY_ENABLE
/* extrakey endpoint state structure */
static USBInEndpointState extra_ep_state;

/* extrakey endpoint initialization structure (IN) */
static const USBEndpointConfig extra_ep_config = {
    USB_EP_MODE_TYPE_INTR, /* Interrupt EP */
    NULL,                  /* SETUP packet notification callback */
    extra_in_cb,           /* IN notification callback */
    NULL,                  /* OUT notification callback */
    EXTRA_EPSIZE,          /* IN maximum packet size */
    0,                     /* OUT maximum packet size */
    &extra_ep_state,       /* IN Endpoint state */
    NULL,                  /* OUT endpoint state */
    2,                     /* IN multiplier */
    NULL                   /* SETUP buffer (not a SETUP endpoint) */
};
#endif /* EXTRAKEY_ENABLE */

#ifdef NKRO_ENABLE
/* nkro endpoint state structure */
static USBInEndpointState nkro_ep_state;

/* nkro endpoint initialization structure (IN) */
static const USBEndpointConfig nkro_ep_config = {
    USB_EP_MODE_TYPE_INTR, /* Interrupt EP */
    NULL,                  /* SETUP packet notification callback */
    nkro_in_cb,            /* IN notification callback */
    NULL,                  /* OUT notification callback */
    NKRO_EPSIZE,           /* IN maximum packet size */
    0,                     /* OUT maximum packet size */
    &nkro_ep_state,        /* IN Endpoint state */
    NULL,                  /* OUT endpoint state */
    2,                     /* IN multiplier */
    NULL                   /* SETUP buffer (not a SETUP endpoint) */
};
#endif /* NKRO_ENABLE */

#ifdef LAMP_ARRAY_ENABLE
/* lamp array endpoint state structure */
static USBInEndpointState lamp_array_ep_state;

/* lamp array endpoint initialization structure (IN) */
static const USBEndpointConfig lamp_array_ep_config = {
    USB_EP_MODE_TYPE_INTR, /* Interrupt EP */
    NULL,                  /* SETUP packet notification callback */
    lamp_array_in_cb,      /* IN notification callback */
    NULL,                  /* OUT notification callback */
    LAMP_ARRAY_EPSIZE,     /* IN maximum packet size */
    0,                     /* OUT maximum packet size */
    &lamp_array_ep_state,  /* IN Endpoint state */
    NULL,                  /* OUT endpoint state */
    2,                     /* IN multiplier */
    NULL                   /* SETUP buffer (not a SETUP endpoint) */
};
#endif /* LAMP_ARRAY_ENABLE */
#ifdef CONTROL_ENABLE

/* Control endpoint state structure */
static USBInEndpointState control_ep_state;

/* Control endpoint initialization structure */
static const USBEndpointConfig control_ep_config = {
    USB_EP_MODE_TYPE_INTR,
    NULL,
    control_in_cb,
    NULL,
    CONTROL_EPSIZE,
    0,
    &control_ep_state,
    NULL,
    2,
    NULL
};

#endif /* CONTROL_ENABLE */
/* ---------------------------------------------------------
 *                  USB driver functions
 * ---------------------------------------------------------
 */

/* Handles the USB driver global events
 * TODO: maybe disable some things when connection is lost? */
static void usb_event_cb(USBDriver *usbp, usbevent_t event)
{
    switch (event)
    {
    case USB_EVENT_RESET:
        // TODO: from ISR! print("[R]");
        return;

    case USB_EVENT_ADDRESS:
        return;

    case USB_EVENT_CONFIGURED:
        osalSysLockFromISR();
        /* Enable the endpoints specified into the configuration. */
        usbInitEndpointI(usbp, KBD_ENDPOINT, &kbd_ep_config);
#ifdef MOUSE_ENABLE
        usbInitEndpointI(usbp, MOUSE_ENDPOINT, &mouse_ep_config);
#endif /* MOUSE_ENABLE */
#ifdef RAW_ENABLE
        usbInitEndpointI(usbp, RAW_IN_ENDPOINT, &raw_ep_config);
        // usbInitEndpointI(usbp, RAW_OUT_ENDPOINT, &raw_ep_out_config);
        usbStartReceiveI(usbp, RAW_OUT_ENDPOINT, raw_report, RAW_EPSIZE);
#endif /* RAW_ENABLE */
#ifdef CONSOLE_ENABLE
        usbInitEndpointI(usbp, CONSOLE_ENDPOINT, &console_ep_config);
        /* don't need to start the flush timer, it starts from console_in_cb automatically */
#endif /* CONSOLE_ENABLE */
#ifdef EXTRAKEY_ENABLE
        usbInitEndpointI(usbp, EXTRA_ENDPOINT, &extra_ep_config);
#endif /* EXTRAKEY_ENABLE */
#ifdef NKRO_ENABLE
        usbInitEndpointI(usbp, NKRO_ENDPOINT, &nkro_ep_config);
#endif /* NKRO_ENABLE */
#ifdef LAMP_ARRAY_ENABLE
        usbInitEndpointI(usbp, LAMP_ARRAY_ENDPOINT, &lamp_array_ep_config);
#endif /* LAMP_ARRAY_ENABLE */
#ifdef CONTROL_ENABLE
        //usbInitEndpointI(usbp, CONTROL_ENDPOINT, &lamp_array_ep_config);
				usbInitEndpointI(usbp, CONTROL_ENDPOINT, &control_ep_config);
#endif /* CONTROL_ENABLE */
#ifdef reboot
        g_usb_configured_flag = true;   // ★ 只設旗標，不做重活 lyon
#endif
        osalSysUnlockFromISR();
        return;
				
		case USB_EVENT_UNCONFIGURED:
				/*
				 * Host 移除 USB configuration。
				 * 若有 endpoint busy flag，可在此清除。
				 */
				break;

    case USB_EVENT_SUSPEND:
        // TODO: from ISR! print("[S]");
        hook_usb_suspend_entry();
        // uprintf ("Suspend in usb main\r\n");
        return;

    case USB_EVENT_WAKEUP:
        // TODO: from ISR! print("[W]");
        suspend_wakeup_init();
        hook_usb_wakeup();
        // uprintf ("Wakeup in usb main\r\n");
        // currentLED_RunMode.type = normalMode;
        // setRunMode(currentLED_RunMode);
        return;

    case USB_EVENT_STALLED:
        return;
    }
}

/* Function used locally in os/hal/src/usb.c for getting descriptors
 * need it here for HID descriptor */
static uint16_t get_hword(uint8_t *p)
{
    uint16_t hw;

    hw = (uint16_t)*p++;
    hw |= (uint16_t)*p << 8U;
    return hw;
}

/*
 * Appendix G: HID Request Support Requirements
 *
 * The following table enumerates the requests that need to be supported by various types of HID class devices.
 * Device type     GetReport   SetReport   GetIdle     SetIdle     GetProtocol SetProtocol
 * ------------------------------------------------------------------------------------------
 * Boot Mouse      Required    Optional    Optional    Optional    Required    Required
 * Non-Boot Mouse  Required    Optional    Optional    Optional    Optional    Optional
 * Boot Keyboard   Required    Optional    Required    Required    Required    Required
 * Non-Boot Keybrd Required    Optional    Required    Required    Optional    Optional
 * Other Device    Required    Optional    Optional    Optional    Optional    Optional
 */

// USB Send
uint8_t lamp_array_processing(uint8_t *usb_setup, report_lamp_array_t *report)
{
    report_lamp_array_attrib_report attrib_report;
    report_lamp_array_attrib_request_report attrib_request_report;
    static uint16_t lampID = 0;
    uint32_t blVersion;

    switch (usb_setup[0])
    {
    case 0xa1:
        memset(report->data, 0x00, sizeof(report->data));
        report->reportID = usb_setup[2];
        switch (usb_setup[2])
        {
        case 0x01:
            attrib_report.lampCount = lampCount;
            attrib_report.boxWidth = lampBoxWidth;
            attrib_report.boxHeight = lampBoxHeight;
            attrib_report.boxDepth = lampBoxDepth;
            attrib_report.kind = lampKind;
            attrib_report.minUpdateInterval = lampMinUpdateInterval;
            lampID = 0; // reset lampID
            memcpy(report->data, &attrib_report, usb_setup[6] - 1);
            break;
        case 0x03:
            attrib_request_report.lampID = lampID;
            attrib_request_report.postionX = lampPostionX[lampID];
            attrib_request_report.postionY = lampPostionY[lampID];
            attrib_request_report.postionZ = lampPostionZ[lampID];
            attrib_request_report.purpose = lampPurpose[lampID];
            attrib_request_report.updataLatency = lampUpdateLatency;
            attrib_request_report.redLevelCount = lampRedLevelCount;
            attrib_request_report.greenLevelCount = lampGreenLevelCount;
            attrib_request_report.blueLevelCount = lampBlueLevelCount;
            attrib_request_report.intensityLevelCount = lampIntensityLevelCount;
            attrib_request_report.isPorgrammable = lampIsProgrammable;
            attrib_request_report.inputBinding = lampInputBinding[lampID];
            lampID++;
            memcpy(report->data, &attrib_request_report, usb_setup[6] - 1);
            break;
        case 0x06:
            report->data[0] = autonomousMode;
            break;
        case 0x07:
            memset(report->data, 0x00, sizeof(report->data));
            report->data[0] = dynamaticReciveBuff[0];   //!!!
            switch (report->data[0])
            {
            case apiControl_LED_Mode:
                report->data[1] = 0x07; // Length
                report->data[2] = currentLED_RunMode.type;
                report->data[3] = currentLED_RunMode.mode;
                report->data[4] = currentLED_RunMode.brightness;
                report->data[5] = currentLED_RunMode.speed;
                break;
            case apiKeymatrix_GetControl:
                memcpy(&report->data[1], &dynamaticReciveBuff[1], sizeof(dynamaticReciveBuff) - 1);
                break;

            case apiRemap_getLayerCount:
                report->data[1] = 3;
                report->data[2] = KEYMAPS_COUNT;
                break;

            case apiKeymatrix_SetControl:
                //                report->data[1] = 0x07;							  //length
                //                report->data[2] = dynamaticReciveBuff[1];         //mode
                //                report->data[3] = dynamaticReciveBuff[2];         //enable
                memcpy(&report->data[1], &dynamaticReciveBuff[1], sizeof(dynamaticReciveBuff) - 1);
                break;

            case apiFactory_Version:
                report->data[1] = 0x05;
                switch (dynamaticReciveBuff[1])
                {
                case 0:
                    report->data[2] = VENDOR_ID & 0x0ff;
                    report->data[3] = VENDOR_ID >> 8;
                    break;
                case 1:
                    report->data[2] = PRODUCT_ID & 0x0ff;
                    report->data[3] = PRODUCT_ID >> 8;
                    break;
                case 2:
                    report->data[2] = DEVICE_VER & 0x0ff;
                    report->data[3] = DEVICE_VER >> 8;
                    break;
                case 3:
                    blVersion = *((uint32_t *)(0x0800580c));
                    report->data[2] = blVersion & 0xff;
                    report->data[3] = (blVersion >> 8) & 0xff;
                    break;
                }
                break;

            case apiMacro_Info:
                report->data[1] = 0x05;
                report->data[2] = DYNAMIC_MACRO_COUNT;
                report->data[3] = DYNAMIC_MACRO_SIZE;
                break;

            case apiRemap_GetKeyValue:
                report->data[1] = 0x09;
                report->data[2] = dynamaticReciveBuff[3];
                report->data[3] = dynamaticReciveBuff[4];
                report->data[4] = dynamaticReciveBuff[5];
                report->data[5] = dynamaticReciveBuff[6];
                report->data[6] = dynamaticReciveBuff[7];
                report->data[7] = dynamaticReciveBuff[8];
                break;

                // case apiRemap_GetKeyValues:
                //   report->data[1] = 0x05;
                //   report->data[2] = dynamaticReciveBuff[3];
                //   report->data[3] = dynamaticReciveBuff[4];
                //   break;

            case apiRemap_GetKeyValues:
            case apiMacro_Read:
                memcpy(&report->data[1], &dynamaticReciveBuff[1], sizeof(dynamaticReciveBuff) - 1);
                break;

            case apiRemap_SetKeyValue:
                report->data[1] = 0x09;                    // length
                report->data[2] = dynamaticReciveBuff[1];  // layer
                report->data[3] = dynamaticReciveBuff[2];  // row
                report->data[4] = dynamaticReciveBuff[3];  // column
                report->data[5] = dynamaticReciveBuff[4];  // modifier
                report->data[6] = dynamaticReciveBuff[5];  // keycode
                report->data[7] = dynamaticReciveBuff[6];  // trigger_H
                report->data[8] = dynamaticReciveBuff[7];  // trigger_L
                report->data[9] = dynamaticReciveBuff[8];  // release_H
                report->data[10] = dynamaticReciveBuff[9]; // release_L
                break;

            case apiRemap_Reset:
            case apiTrigger_Reset:
            case apiRapidTrigger_Reset:
                report->data[1] = 0x04;
                report->data[2] = dynamaticReciveBuff[3];
                break;

            case apiTrigger_GetValue:
            case apiRelease_GetValue:
                report->data[1] = 0x07;                   // length
                report->data[2] = dynamaticReciveBuff[1]; // row
                report->data[3] = dynamaticReciveBuff[2]; // column
                report->data[4] = dynamaticReciveBuff[3]; // trigger_H
                report->data[5] = dynamaticReciveBuff[4]; // trigger_L
                break;

            case apiTrigger_SetValue:
            case apiRelease_SetValue:
                report->data[1] = 0x07;                   // length
                report->data[2] = dynamaticReciveBuff[1]; // row
                report->data[3] = dynamaticReciveBuff[2]; // column
                report->data[4] = dynamaticReciveBuff[3]; // trigger_H
                report->data[5] = dynamaticReciveBuff[4]; // trigger_L
                break;

            case apiPostionRead:
                memcpy(&report->data[1], &dynamaticReciveBuff[1], sizeof(dynamaticReciveBuff) - 1);
                break;

            case apiPostionSet:
                report->data[1] = 0x05;
                report->data[2] = dynamaticReciveBuff[3]; // 0:Initial level	1:Travel level
                report->data[3] = dynamaticReciveBuff[4]; // max
                report->data[4] = dynamaticReciveBuff[5]; // update
                report->data[5] = dynamaticReciveBuff[6]; // update
                break;

            case apiRapidTrigger_GetValue:
                report->data[1] = 0x03 + sizeof(rapidTrigger_config_t) + 1; // length
                for (uint8_t i = 0; i < sizeof(rapidTrigger_config_t) + 1; i++)
                {
                    report->data[i + 2] = dynamaticReciveBuff[i + 3];
                }
                break;

            case apiRapidTrigger_SetValue:
                report->data[1] = 0x05 + sizeof(rapidTrigger_config_t); // length
                for (uint8_t i = 0; i < sizeof(rapidTrigger_config_t) + 2; i++)
                {
                    report->data[i + 2] = dynamaticReciveBuff[i + 3];
                }
                break;

            case apiFactory_Boot:
            case apiFactory_Reset:
                report->data[1] = 0x00;
                break;
            }
            break;
        case 0x08:
            report->data[0] = usb_setup[3];
            report->data[1] = 0x01;
            report->data[2] = 0x02;
            report->data[3] = 0x03;
            report->data[4] = 0x04;
            // uprintf ("read report id 0x08\r\n");
            break;
        }
    case 0x21:
        // uprintf ("test\r\n");
        // memcpy (report->data,&attrib_request_report,usb_setup[6]-1);
        // attrib_request_report.postionX = lampPostionX[lampID];

        break;
    }
		return FALSE;
}

uint8_t control_processing(uint8_t *usb_setup, report_lamp_array_t *report)
{
    report_lamp_array_attrib_report attrib_report;
    report_lamp_array_attrib_request_report attrib_request_report;
    static uint16_t lampID = 0;

    switch (usb_setup[0])
    {
    case 0xa1:
        memset(report->data, 0x00, sizeof(report->data));
        report->reportID = usb_setup[2];
        switch (usb_setup[2])
        {
        case 0x01:
            attrib_report.lampCount = lampCount;
            attrib_report.boxWidth = lampBoxWidth;
            attrib_report.boxHeight = lampBoxHeight;
            attrib_report.boxDepth = lampBoxDepth;
            attrib_report.kind = lampKind;
            attrib_report.minUpdateInterval = lampMinUpdateInterval;
            lampID = 0; // reset lampID
            memcpy(report->data, &attrib_report, usb_setup[6] - 1);
            break;
        case 0x03:
            attrib_request_report.lampID = lampID;
            attrib_request_report.postionX = lampPostionX[lampID];
            attrib_request_report.postionY = lampPostionY[lampID];
            attrib_request_report.postionZ = lampPostionZ[lampID];
            attrib_request_report.purpose = lampPurpose[lampID];
            attrib_request_report.updataLatency = lampUpdateLatency;
            attrib_request_report.redLevelCount = lampRedLevelCount;
            attrib_request_report.greenLevelCount = lampGreenLevelCount;
            attrib_request_report.blueLevelCount = lampBlueLevelCount;
            attrib_request_report.intensityLevelCount = lampIntensityLevelCount;
            attrib_request_report.isPorgrammable = lampIsProgrammable;
            attrib_request_report.inputBinding = lampInputBinding[lampID];
            lampID++;
            memcpy(report->data, &attrib_request_report, usb_setup[6] - 1);
            break;
        case 0x06:
            report->data[0] = autonomousMode;
            break;
        case 0x07:
            memset(report->data, 0x00, sizeof(report->data));
            report->data[0] = dynamaticReciveBuff[0];
            switch (report->data[0])
            {
            case apiControl_LED_Mode:
                report->data[1] = 0x07;
                report->data[2] = currentLED_RunMode.type;
                report->data[3] = currentLED_RunMode.mode;
                report->data[4] = currentLED_RunMode.brightness;
                report->data[5] = currentLED_RunMode.speed;
                break;
            case apiFactory_Version:
                report->data[1] = 0x05;
                switch (dynamaticReciveBuff[1])
                {
                case 0:
                    report->data[2] = VENDOR_ID & 0x0ff;
                    report->data[3] = VENDOR_ID >> 8;
                    break;
                case 1:
                    report->data[2] = PRODUCT_ID & 0x0ff;
                    report->data[3] = PRODUCT_ID >> 8;
                    break;
                case 2:
                    report->data[2] = DEVICE_VER & 0x0ff;
                    report->data[3] = DEVICE_VER >> 8;
                    break;
                }
                break;

            case apiRemap_GetKeyValue:
                report->data[1] = 0x05;
                report->data[2] = dynamaticReciveBuff[3];
                report->data[3] = dynamaticReciveBuff[4];
                break;

                // case apiRemap_GetKeyValues:
                //   report->data[1] = 0x05;
                //   report->data[2] = dynamaticReciveBuff[3];
                //   report->data[3] = dynamaticReciveBuff[4];
                //   break;

            case apiRemap_GetKeyValues:
                memcpy(&report->data[1], &dynamaticReciveBuff[1], sizeof(dynamaticReciveBuff) - 1);

                // report->data[1] = dynamaticReciveBuff[1];
                // for (uint8_t i=2;i<60;i++){
                //   report->data[i] = dynamaticReciveBuff[i];
                // }

                // report->data[2] = dynamaticReciveBuff[2];
                // report->data[3] = dynamaticReciveBuff[3];
                // report->data[4] = dynamaticReciveBuff[4];
                // report->data[5] = dynamaticReciveBuff[5];
                // report->data[6] = dynamaticReciveBuff[6];
                // report->data[7] = dynamaticReciveBuff[8];
                // report->data[8] = dynamaticReciveBuff[9];
                break;

            case apiRemap_SetKeyValue:
            case apiRemap_SetKeyValueTrigger:
                report->data[1] = 0x07;
                report->data[2] = dynamaticReciveBuff[1];
                report->data[3] = dynamaticReciveBuff[2];
                report->data[4] = dynamaticReciveBuff[3];
                report->data[5] = dynamaticReciveBuff[4];
                report->data[6] = dynamaticReciveBuff[5];
                break;

            case apiRemap_Reset:
                report->data[1] = 0x04;
                report->data[2] = dynamaticReciveBuff[3];
                break;

            case apiPostionRead:
                memcpy(&report->data[1], &dynamaticReciveBuff[1], sizeof(dynamaticReciveBuff) - 1);
                break;

                //            case apiTravelPostion:
                //                report->data[1] = 0x04;
                //                report->data[2] = dynamaticReciveBuff[3];         //row
                //                break;

            case apiFactory_Boot:
                report->data[1] = 0x00;
                break;
            }
            break;
        case 0x08:
            report->data[0] = usb_setup[3];
            report->data[1] = 0x01;
            report->data[2] = 0x02;
            report->data[3] = 0x03;
            report->data[4] = 0x04;
            // uprintf ("read report id 0x08\r\n");
            break;
        }
    case 0x21:
        // uprintf ("test\r\n");
        // memcpy (report->data,&attrib_request_report,usb_setup[6]-1);
        // attrib_request_report.postionX = lampPostionX[lampID];

        break;
    }
		return FALSE;
}

#ifdef RAW_ENABLE
static void rawReciveCB(USBDriver *usbp)
{
    (void)usbp;
    osalSysLockFromISR();
    // memcpy(dynamaticReciveBuff, lamp_array_data_report.data, sizeof(dynamaticReciveBuff));
//     chBSemSignalI(&ledDynamaticLightingSem);
    // lamp_array_data_report.updated = true;
//    printf("test\r\n");
    osalSysUnlockFromISR();
}
#endif

#ifdef LAMP_ARRAY_ENABLE
static void dynamticReciveCB(USBDriver *usbp)
{
    (void)usbp;
    osalSysLockFromISR();
    memcpy(dynamaticReciveBuff, lamp_array_data_report.data, sizeof(dynamaticReciveBuff));
    chBSemSignalI(&ledDynamaticLightingSem);
    lamp_array_data_report.updated = true;
    osalSysUnlockFromISR();
}
#endif
/*
#ifdef CONTROL_ENABLE
static void controlReciveCB(USBDriver *usbp)
{
    (void)usbp;
    // chBSemSignal(&usbControlSem);
    osalSysLockFromISR();
    control_data_report.updated = true;
    memcpy(controlReciveBuff, control_data_report.data, sizeof(controlReciveBuff));
    osalSysUnlockFromISR();
}
#endif
*/
/* Callback for SETUP request on the endpoint 0 (control) */
static bool usb_request_hook_cb(USBDriver *usbp)
{
    const USBDescriptor *dp;

    /*
     * usbp->setup fields:
     *  0:   bmRequestType (bitmask)
     *  1:   bRequest
     *  2,3: (LSB,MSB) wValue
     *  4,5: (LSB,MSB) wIndex
     *  6,7: (LSB,MSB) wLength
     */

    /*
     * Handle vendor-specific requests.
     */
    switch (usbp->setup[0])
    {
        case USB_RTYPE_TYPE_VENDOR_1:
            usbSetupTransfer(
                usbp,
                (uint8_t *)&WINUSB_ExtendedCompatId_Descritpor + 8,
                usbp->setup[6],
                NULL
            );
            return TRUE;

        case USB_RTYPE_TYPE_VENDOR_2:
            usbSetupTransfer(
                usbp,
                (uint8_t *)&WINUSB_ExtendedProperty_InterfaceGUID_Descritpor + 8,
                usbp->setup[6],
                NULL
            );
            return TRUE;

        default:
            break;
    }

    /*
     * Handle HID class-specific interface requests.
     */
    if (((usbp->setup[0] & USB_RTYPE_TYPE_MASK) ==
         USB_RTYPE_TYPE_CLASS) &&
        ((usbp->setup[0] & USB_RTYPE_RECIPIENT_MASK) ==
         USB_RTYPE_RECIPIENT_INTERFACE))
    {
        switch (usbp->setup[0] & USB_RTYPE_DIR_MASK)
        {
            /*
             * Device-to-host requests.
             */
            case USB_RTYPE_DIR_DEV2HOST:
                switch (usbp->setup[1])
                {
                    case HID_GET_REPORT:
                        switch (usbp->setup[4])
                        {
                            case KBD_INTERFACE:
#ifdef NKRO_ENABLE
                            case NKRO_INTERFACE:
#endif /* NKRO_ENABLE */
                                usbSetupTransfer(
                                    usbp,
                                    (uint8_t *)&keyboard_report_sent,
                                    sizeof(keyboard_report_sent),
                                    NULL
                                );
                                return TRUE;

#ifdef LAMP_ARRAY_ENABLE
                            case LAMP_ARRAY_INTERFACE:
                                osalSysLockFromISR();

                                lamp_array_processing(
                                    usbp->setup,
                                    &lamp_array_report
                                );

                                usbSetupTransfer(
                                    usbp,
                                    (uint8_t *)&lamp_array_report,
                                    sizeof(lamp_array_report.data),
                                    NULL
                                );

                                osalSysUnlockFromISR();
                                return TRUE;
#endif /* LAMP_ARRAY_ENABLE */

#ifdef CONTROL_ENABLE
                            case CONTROL_INTERFACE:
                                osalSysLockFromISR();

                                /*
                                 * 保留目前原始流程，不在本次修改中
                                 * 改變 Control GET_REPORT 的處理邏輯。
                                 */
                                lamp_array_processing(
                                    usbp->setup,
                                    &lamp_array_report
                                );

                                usbSetupTransfer(
                                    usbp,
                                    (uint8_t *)&lamp_array_report,
                                    sizeof(lamp_array_report.data),
                                    NULL
                                );

                                osalSysUnlockFromISR();
                                return TRUE;
#endif /* CONTROL_ENABLE */

#ifdef MOUSE_ENABLE
                            case MOUSE_INTERFACE:
                                usbSetupTransfer(
                                    usbp,
                                    (uint8_t *)&mouse_report_blank,
                                    sizeof(mouse_report_blank),
                                    NULL
                                );
                                return TRUE;
#endif /* MOUSE_ENABLE */

#ifdef RAW_ENABLE
                            case RAW_INTERFACE:
                                usbSetupTransfer(
                                    usbp,
                                    (uint8_t *)&raw_report,
                                    usbp->setup[6],
                                    rawReciveCB
                                );
                                return TRUE;
#endif /* RAW_ENABLE */

#ifdef CONSOLE_ENABLE
                            case CONSOLE_INTERFACE:
                                usbSetupTransfer(
                                    usbp,
                                    console_queue_buffer,
                                    CONSOLE_EPSIZE,
                                    NULL
                                );
                                return TRUE;
#endif /* CONSOLE_ENABLE */

#ifdef EXTRAKEY_ENABLE
                            case EXTRA_INTERFACE:
                                /*
                                 * MSB(wValue) == 1 means Input Report.
                                 */
                                if (usbp->setup[3] == 1)
                                {
                                    switch (usbp->setup[2])
                                    {
                                        case REPORT_ID_SYSTEM:
                                            extra_report_blank[0] =
                                                REPORT_ID_SYSTEM;

                                            usbSetupTransfer(
                                                usbp,
                                                (uint8_t *)extra_report_blank,
                                                sizeof(extra_report_blank),
                                                NULL
                                            );
                                            return TRUE;

                                        case REPORT_ID_CONSUMER:
                                            extra_report_blank[0] =
                                                REPORT_ID_CONSUMER;

                                            usbSetupTransfer(
                                                usbp,
                                                (uint8_t *)extra_report_blank,
                                                sizeof(extra_report_blank),
                                                NULL
                                            );
                                            return TRUE;

                                        /*
                                        case REPORT_ID_GAMEPAD:
                                            extra_game_report.report_ID =
                                                REPORT_ID_GAMEPAD;

                                            usbSetupTransfer(
                                                usbp,
                                                (uint8_t *)&extra_game_report,
                                                sizeof(extra_game_report),
                                                NULL
                                            );
                                            return TRUE;
                                        */

                                        default:
                                            return FALSE;
                                    }
                                }

                                return FALSE;
#endif /* EXTRAKEY_ENABLE */

                            default:
                                usbSetupTransfer(
                                    usbp,
                                    NULL,
                                    0,
                                    NULL
                                );
                                return TRUE;
                        }

                    case HID_GET_PROTOCOL:
                        if ((usbp->setup[4] == KBD_INTERFACE) &&
                            (usbp->setup[5] == 0))
                        {
                            usbSetupTransfer(
                                usbp,
                                &keyboard_protocol,
                                1,
                                NULL
                            );
                            return TRUE;
                        }
                        break;

                    case HID_GET_IDLE:
                        usbSetupTransfer(
                            usbp,
                            &keyboard_idle,
                            1,
                            NULL
                        );
                        return TRUE;

                    default:
                        break;
                }
                break;

            /*
             * Host-to-device requests.
             */
            case USB_RTYPE_DIR_HOST2DEV:
                switch (usbp->setup[1])
                {
                    case HID_SET_REPORT:
                        /*
                         * HID_SET_REPORT selects the target interface
                         * through LSB(wIndex), setup[4].
                         */
                        switch (usbp->setup[4])
                        {
                            case KBD_INTERFACE:
#ifdef NKRO_ENABLE
                            case NKRO_INTERFACE:
#endif /* NKRO_ENABLE */
                                /*
                                 * keyboard_led_stats must be word/dword
                                 * aligned; otherwise F0 can cause an
                                 * exception.
                                 */
                                usbSetupTransfer(
                                    usbp,
                                    (uint8_t *)&keyboard_led_stats,
                                    1,
                                    NULL
                                );
                                return TRUE;

#ifdef LAMP_ARRAY_ENABLE
                            case LAMP_ARRAY_INTERFACE:
                                usbSetupTransfer(
                                    usbp,
                                    (uint8_t *)&lamp_array_data_report,
                                    usbp->setup[6],
                                    dynamticReciveCB
                                );

                                lamp_array_data_report.length =
                                    usbp->setup[6] - 1;

                                return TRUE;
#endif /* LAMP_ARRAY_ENABLE */

#ifdef CONTROL_ENABLE
                            case CONTROL_INTERFACE:
#ifdef LAMP_ARRAY_ENABLE
                                /*
                                 * v1505 ADC compatibility for Feature Report ID 7.
                                 * The HID Report ID is carried in wValue (setup[2]);
                                 * the control data stage contains only the payload.
                                 */
                                lamp_array_data_report.reportID = usbp->setup[2];
                                lamp_array_data_report.length = usbp->setup[6];

                                usbSetupTransfer(
                                    usbp,
                                    (uint8_t *)lamp_array_data_report.data,
                                    usbp->setup[6],
                                    dynamticReciveCB
                                );
#else
                                usbSetupTransfer(
                                    usbp,
                                    (uint8_t *)control_data_report.data,
                                    usbp->setup[6],
                                    controlReciveCB
                                );

                                control_data_report.reportID = usbp->setup[2];
                                control_data_report.length = usbp->setup[6];
#endif /* LAMP_ARRAY_ENABLE */
                                return TRUE;
#endif /* CONTROL_ENABLE */

                            default:
                                break;
                        }

                        /*
                         * HID_SET_REPORT was not handled by any known
                         * interface. Do not fall through to
                         * HID_SET_PROTOCOL.
                         */
                        break;

                    case HID_SET_PROTOCOL:
                        if ((usbp->setup[4] == KBD_INTERFACE) &&
                            (usbp->setup[5] == 0))
                        {
                            keyboard_protocol =
                                (usbp->setup[2] != 0x00);

#ifdef NKRO_ENABLE
                            keyboard_nkro = !!keyboard_protocol;

                            if (!keyboard_nkro && keyboard_idle)
                            {
#else
                            if (keyboard_idle)
                            {
#endif /* NKRO_ENABLE */
                                /*
                                 * Arm the idle timer when boot protocol
                                 * and idle rate are active.
                                 */
                                osalSysLockFromISR();

                                chVTSetI(
                                    &keyboard_idle_timer,
                                    4 * MS2ST(keyboard_idle),
                                    keyboard_idle_timer_cb,
                                    (void *)usbp
                                );

                                osalSysUnlockFromISR();
                            }
                        }

                        usbSetupTransfer(
                            usbp,
                            NULL,
                            0,
                            NULL
                        );
                        return TRUE;

                    case HID_SET_IDLE:
                        keyboard_idle = usbp->setup[3];

#ifdef NKRO_ENABLE
                        if (!keyboard_nkro && keyboard_idle)
                        {
#else
                        if (keyboard_idle)
                        {
#endif /* NKRO_ENABLE */
                            osalSysLockFromISR();

                            chVTSetI(
                                &keyboard_idle_timer,
                                4 * MS2ST(keyboard_idle),
                                keyboard_idle_timer_cb,
                                (void *)usbp
                            );

                            osalSysUnlockFromISR();
                        }

                        usbSetupTransfer(
                            usbp,
                            NULL,
                            0,
                            NULL
                        );
                        return TRUE;

                    default:
                        break;
                }
                break;

            default:
                break;
        }
    }

    /*
     * Handle HID class Get Descriptor requests that are not handled
     * by the default request hook.
     */
    if ((usbp->setup[0] == 0x81) &&
        (usbp->setup[1] == USB_REQ_GET_DESCRIPTOR))
    {
        dp = usbp->config->get_descriptor_cb(
            usbp,
            usbp->setup[3],
            usbp->setup[2],
            get_hword(&usbp->setup[4])
        );

        if (dp == NULL)
        {
            return FALSE;
        }

        usbSetupTransfer(
            usbp,
            (uint8_t *)dp->ud_string,
            dp->ud_size,
            NULL
        );

        return TRUE;
    }

    return FALSE;
}

/* Start-of-frame callback */
static void usb_sof_cb(USBDriver *usbp)
{
    kbd_sof_cb(usbp);
}

/* USB driver configuration */
static const USBConfig usbcfg = {
    usb_event_cb,          /* USB events callback */
    usb_get_descriptor_cb, /* Device GET_DESCRIPTOR request callback */
    usb_request_hook_cb,   /* Requests hook callback */
    usb_sof_cb             /* Start Of Frame callback */
};

/*
 * Initialize the USB driver
 */
void init_usb_driver(USBDriver *usbp)
{
    /*
     * Activates the USB driver and then the USB bus pull-up on D+.
     * Note, a delay is inserted in order to not have to disconnect the cable
     * after a reset.
     */
    usbDisconnectBus(usbp);
    chThdSleepMilliseconds(500);
    usbStart(usbp, &usbcfg);
    usbConnectBus(usbp);

    chVTObjectInit(&keyboard_idle_timer);
#ifdef CONSOLE_ENABLE
    obqObjectInit(&console_buf_queue, true, console_queue_buffer, CONSOLE_EPSIZE, CONSOLE_QUEUE_CAPACITY, console_queue_onotify, (void *)usbp);
    chVTObjectInit(&console_flush_timer);
#endif
}

/*
 * Send remote wakeup packet
 * Note: should not be called from ISR
 */
void send_remote_wakeup(USBDriver *usbp)
{
    (void)usbp;
#if defined(K20x) || defined(KL2x)
#if KINETIS_USB_USE_USB0
    USB0->CTL |= USBx_CTL_RESUME;
    chThdSleepMilliseconds(15);
    USB0->CTL &= ~USBx_CTL_RESUME;
#endif                                         /* KINETIS_USB_USE_USB0 */
#elif defined(STM32F0XX) || defined(STM32F1XX) /* K20x || KL2x */
    STM32_USB->CNTR |= CNTR_RESUME;
    chThdSleepMilliseconds(15);
    STM32_USB->CNTR &= ~CNTR_RESUME;
#else                                          /* STM32F0XX || STM32F1XX */
//#warning Sending remote wakeup packet not implemented for your platform.
#endif /* K20x || KL2x */
    usbWakeupHost(&USB_DRIVER);
}

/* ---------------------------------------------------------
 *                  Keyboard functions
 * ---------------------------------------------------------
 */

/* keyboard IN callback hander (a kbd report has made it IN) */
/* =====================================================
 * LOGA USB HID Latency Debug
 *
 * TP56 = PB12
 *
 * HIGH : USB IN Transfer Complete Callback
 * LOW  : Callback Debug Pulse End
 * ===================================================== */

#define LOGA_LATENCY_GPIO_DEBUG 1

/* Keyboard IN callback */
void kbd_in_cb(USBDriver *usbp, usbep_t ep)
{
#if LOGA_LATENCY_GPIO_DEBUG
    /* TP56 : USB Keyboard Transfer Complete */
    palSetPad(GPIOB, 12);
    palClearPad(GPIOB, 12);
#endif

    (void)usbp;
    (void)ep;
}

#ifdef NKRO_ENABLE

/* NKRO Keyboard IN callback */
void nkro_in_cb(USBDriver *usbp, usbep_t ep)
{
#if LOGA_LATENCY_GPIO_DEBUG
    /* TP56 : USB NKRO Transfer Complete */
    palSetPad(GPIOB, 12);
    palClearPad(GPIOB, 12);
#endif

    (void)usbp;
    (void)ep;
}

#endif /* NKRO_ENABLE */

/* start-of-frame handler
 * TODO: i guess it would be better to re-implement using timers,
 *  so that this is not going to have to be checked every 1ms */
void kbd_sof_cb(USBDriver *usbp)
{
#if LOGA_LATENCY_GPIO_DEBUG
    /* Test36 TP55/PB10: pulse on each USB Start Of Frame (SOF).
     * Compare SOF with TP51 submit and TP56 IN-complete callback.
     * SOF cadence is nominally 1 ms for full-speed USB.
     */
    palSetPad(GPIOB, 10);
    palClearPad(GPIOB, 10);
#endif
    (void)usbp;
}

/* Idle requests timer code
 * callback (called from ISR, unlocked state) */
static void keyboard_idle_timer_cb(virtual_timer_t *vtp, void *arg)
{
    USBDriver *usbp = (USBDriver *)arg;

    (void)vtp;

    osalSysLockFromISR();

    /* check that the states of things are as they're supposed to */
    if (usbGetDriverStateI(usbp) != USB_ACTIVE)
    {
        /* do not rearm the timer, should be enabled on IDLE request */
        osalSysUnlockFromISR();
        return;
    }

#ifdef NKRO_ENABLE
    if (!keyboard_nkro && keyboard_idle)
    {
#else  /* NKRO_ENABLE */
    if (keyboard_idle)
    {
#endif /* NKRO_ENABLE */
        /* TODO: are we sure we want the KBD_ENDPOINT? */
        if (!usbGetTransmitStatusI(usbp, KBD_ENDPOINT))
        {
            usbStartTransmitI(usbp, KBD_ENDPOINT, (uint8_t *)&keyboard_report_sent, KBD_EPSIZE);
        }
        /* rearm the timer */
        chVTSetI(&keyboard_idle_timer, 4 * MS2ST(keyboard_idle), keyboard_idle_timer_cb, (void *)usbp);
    }

    /* do not rearm the timer if the condition above fails
     * it should be enabled again on either IDLE or SET_PROTOCOL requests */
    osalSysUnlockFromISR();
}

/* LED status */
uint8_t keyboard_leds(void)
{
    return (uint8_t)(keyboard_led_stats & 0xFF);
}

/* prepare and start sending a report IN
 * not callable from ISR or locked state */
void send_keyboard(report_keyboard_t *report)
{
#if LOGA_LATENCY_GPIO_DEBUG
    /*
     * Test41 TP50 / PC4: keyboard report reaches send_keyboard().
     * One short pulse per call, BEFORE USB lock/endpoint wait.
     * Compare TP49 matrix transition -> TP50 entry ->
     * TP51 usbStartTransmitI -> TP56 IN completion.
     * Diagnostic only; do not modify report content or USB timing.
     */
    palSetPad(GPIOC, 4);
    palClearPad(GPIOC, 4);
#endif
    osalSysLock();
    if (usbGetDriverStateI(&USB_DRIVER) != USB_ACTIVE)
    {
        osalSysUnlock();
        return;
    }
    osalSysUnlock();

#ifdef NKRO_ENABLE
    if (keyboard_nkro)
    { /* NKRO protocol */
        /* need to wait until the previous packet has made it through */
        /* can rewrite this using the synchronous API, then would wait
         * until *after* the packet has been transmitted. I think
         * this is more efficient */
        /* busy wait, should be short and not very common */
        osalSysLock();
        if (usbGetTransmitStatusI(&USB_DRIVER, NKRO_ENDPOINT))
        {
            /* Need to either suspend, or loop and call unlock/lock during
             * every iteration - otherwise the system will remain locked,
             * no interrupts served, so USB not going through as well.
             * Note: for suspend, need USB_USE_WAIT == TRUE in halconf.h */
            osalThreadSuspendS(&(&USB_DRIVER)->epc[NKRO_ENDPOINT]->in_state->thread);
        }
				/* TP51 HIGH: USB HID transmission requested */
				palSetPad(GPIOC, 5);
				
        usbStartTransmitI(&USB_DRIVER, NKRO_ENDPOINT, (uint8_t *)report, sizeof(report_keyboard_t));
				
				/* TP51 LOW: USB transmit API returned */
				palClearPad(GPIOC, 5);
				
        osalSysUnlock();
    }
    else
#endif /* NKRO_ENABLE */
    {  /* boot protocol */
        /* need to wait until the previous packet has made it through */
        /* busy wait, should be short and not very common */
        osalSysLock();
        if (usbGetTransmitStatusI(&USB_DRIVER, KBD_ENDPOINT))
        {
            /* Need to either suspend, or loop and call unlock/lock during
             * every iteration - otherwise the system will remain locked,
             * no interrupts served, so USB not going through as well.
             * Note: for suspend, need USB_USE_WAIT == TRUE in halconf.h */
            osalThreadSuspendS(&(&USB_DRIVER)->epc[KBD_ENDPOINT]->in_state->thread);
        }
				/* TP51 HIGH: USB HID transmission requested */
				palSetPad(GPIOC, 5);
				
        usbStartTransmitI(&USB_DRIVER, KBD_ENDPOINT, (uint8_t *)report, KBD_EPSIZE);
				
				/* TP51 LOW: USB transmit API returned */
				palClearPad(GPIOC, 5);
				
        osalSysUnlock();
    }
    keyboard_report_sent = *report;
}

/* ---------------------------------------------------------
 *                     Mouse functions
 * ---------------------------------------------------------
 */

#ifdef MOUSE_ENABLE

/* mouse IN callback hander (a mouse report has made it IN) */
void mouse_in_cb(USBDriver *usbp, usbep_t ep)
{
    (void)usbp;
    (void)ep;
}

void send_mouse(report_mouse_t *report)
{
    osalSysLock();
    if (usbGetDriverStateI(&USB_DRIVER) != USB_ACTIVE)
    {
        osalSysUnlock();
        return;
    }
    osalSysUnlock();

    /* TODO: LUFA manually waits for the endpoint to become ready
     * for about 10ms for mouse, kbd, system; 1ms for nkro
     * is this really needed?
     */

    osalSysLock();
    usbStartTransmitI(&USB_DRIVER, MOUSE_ENDPOINT, (uint8_t *)report, sizeof(report_mouse_t));
    osalSysUnlock();
}

#else  /* MOUSE_ENABLE */
void send_mouse(report_mouse_t *report)
{
    (void)report;
}
#endif /* MOUSE_ENABLE */

/* ---------------------------------------------------------
 *                     Lamp Array functions
 * ---------------------------------------------------------
 */

#ifdef LAMP_ARRAY_ENABLE

/* lamp array IN callback hander (a lamp array report has made it IN) */
void lamp_array_in_cb(USBDriver *usbp, usbep_t ep)
{
    (void)usbp;
    (void)ep;
}

// void send_lamp_array(uint8_t *report) {
//   if(usbGetDriverStateI(&USB_DRIVER) != USB_ACTIVE) {
//     return;
//   }
//   usbStartTransmitI(&USB_DRIVER, LAMP_ARRAY_ENDPOINT, report, LAMP_ARRAY_EPSIZE);
//   printf ("endpoint:%d size:%d\r\n",LAMP_ARRAY_ENDPOINT,LAMP_ARRAY_EPSIZE);
// }

void send_lamp_array(uint8_t *report)
{
    osalSysLock();
    if (usbGetDriverStateI(&USB_DRIVER) != USB_ACTIVE)
    {
        osalSysUnlock();
        return;
    }
    osalSysUnlock();

    /* TODO: LUFA manually waits for the endpoint to become ready
     * for about 10ms for mouse, kbd, system; 1ms for nkro
     * is this really needed?
     */

    osalSysLock();
    usbStartTransmitI(&USB_DRIVER, LAMP_ARRAY_ENDPOINT, report, LAMP_ARRAY_EPSIZE);
    osalSysUnlock();
}

#else  /* LAMP ARRAY_ENABLE */
void send_lamp_array(report_lamp_array_t *report)
{
    (void)report;
}
#endif /* LAMP ARRAY */

/* ---------------------------------------------------------
 *                     Control functions
 * ---------------------------------------------------------
 */

#ifdef CONTROL_ENABLE

/* lamp array IN callback hander (a lamp array report has made it IN) */
void control_in_cb(USBDriver *usbp, usbep_t ep)
{
    (void)usbp;
    (void)ep;
}

void send_control(uint8_t *report)
{
    osalSysLock();
    if (usbGetDriverStateI(&USB_DRIVER) != USB_ACTIVE)
    {
        osalSysUnlock();
        return;
    }
    usbStartTransmitI(&USB_DRIVER, CONTROL_ENDPOINT, report, CONTROL_EPSIZE);
    osalSysUnlock();
}

#else  /* CONTROL_ENABLE */
void send_control(uint8_t *report)
{
    (void)report;
}
#endif /* CONTROL */

/* ---------------------------------------------------------
 *                   Extrakey functions
 * ---------------------------------------------------------
 */

#ifdef EXTRAKEY_ENABLE

/* extrakey IN callback hander */
void extra_in_cb(USBDriver *usbp, usbep_t ep)
{
    /* STUB */
    (void)usbp;
    (void)ep;
    // chBSemSignal(&gamepadAnalogSemWait);
    // osalMutexUnlock(&analogStickMutex);
}

static void send_extra_report(uint8_t report_id, uint16_t data)
{
    osalSysLock();
    if (usbGetDriverStateI(&USB_DRIVER) != USB_ACTIVE)
    {
        osalSysUnlock();
        return;
    }

    static report_extra_t report;
    report = (report_extra_t){.report_id = report_id, .usage = data};

    usbStartTransmitI(&USB_DRIVER, EXTRA_ENDPOINT, (uint8_t *)&report, sizeof(report_extra_t));
    osalSysUnlock();
}

static void send_extra_gamepad_report(uint8_t report_id,report_gamepad_report data)
{
    osalSysLock();

    if ((usbGetDriverStateI(&USB_DRIVER) != USB_ACTIVE) ||
        usbGetTransmitStatusI(&USB_DRIVER, EXTRA_ENDPOINT))
    {
        osalSysUnlock();
        return;
    }

    extra_game_report = data;
    extra_game_report.report_ID = report_id;

    usbStartTransmitI(
        &USB_DRIVER,
        EXTRA_ENDPOINT,
        (uint8_t *)&extra_game_report,
        sizeof(extra_game_report)
    );

    osalSysUnlock();
}

void send_system(uint16_t data)
{
    send_extra_report(REPORT_ID_SYSTEM, data);
}

void send_consumer(uint16_t data)
{
    send_extra_report(REPORT_ID_CONSUMER, data);
}

void send_gamepad(report_gamepad_report data)
{
    // osalMutexLock(&analogStickMutex);
    send_extra_gamepad_report(REPORT_ID_GAMEPAD, data);
    // osalMutexUnlock(&analogStickMutex);
}

#else  /* EXTRAKEY_ENABLE */
void send_system(uint16_t data)
{
    (void)data;
}
void send_consumer(uint16_t data)
{
    (void)data;
}
void send_gamepad(report_gamepad_report data)
{
    (void)data;
}
#endif /* EXTRAKEY_ENABLE */

/* ---------------------------------------------------------
 *                     Raw functions
 * ---------------------------------------------------------
 */

#ifdef RAW_ENABLE

/* raw IN callback hander (a raw report has made it IN) */
void raw_in_cb(USBDriver *usbp, usbep_t ep)
{
    (void)usbp;
    (void)ep;
    chBSemSignal(&rawSendSem);
}
/* raw OUT callback hander (a raw report has made it OUT) */
void raw_out_cb (USBDriver __attribute__ ((__unused__)) * usbp,
	   usbep_t __attribute__ ((__unused__)) ep)
// void raw_out_cb(USBDriver *usbp, usbep_t ep)
{
    usbStartReceiveI(usbp, RAW_OUT_ENDPOINT, raw_report, RAW_EPSIZE);
    chBSemSignalI(&VIA_ReciveSem);
}

void raw_hid_send(uint8_t *report, uint8_t length)
{
    (void)length;
    uint8_t i;
    bool usb_active;

    osalSysLock();

    usb_active =
        (usbGetDriverStateI(&USB_DRIVER) == USB_ACTIVE);

    osalSysUnlock();

    if (!usb_active)
    {
        uprintf("[RAW TX] USB not active, packet dropped\r\n");
        return;
    }

    /*
     * 印出真正送回 APP 的 32-byte Raw HID 封包。
     */
    uprintf("[RAW TX] ");

    for (i = 0; i < sizeof(raw_report); i++)
    {
        uprintf("%02X ", report[i]);
    }

    uprintf("\r\n");

    /*
     * Loga API report ID 固定為 0x19。
     * 不使用 id_loga_api，避免 usb_main.c 找不到 via.h 定義。
     */
    if (report[0] == 0x19U)
    {
        uprintf(
            "[API TX] ID=%02X LEN=%u CMD=%02X%02X STATUS=%02X\r\n",
            (unsigned int)report[0],
            (unsigned int)report[2],
            (unsigned int)report[3],
            (unsigned int)report[4],
            (unsigned int)report[5]
        );
    }

    osalSysLock();

    usbStartTransmitI(
        &USB_DRIVER,
        RAW_IN_ENDPOINT,
        report,
        sizeof(raw_report)
    );

    osalSysUnlock();

    chBSemWait(&rawSendSem);
}

#else  /* RAW_ENABLE */
void send_raw(uint8_t *report)
{
    (void)report;
}
#endif /* RAW_ENABLE */


/* ---------------------------------------------------------
 *                   Console functions
 * ---------------------------------------------------------
 */

#ifdef CONSOLE_ENABLE

/* console IN callback hander */
void console_in_cb(USBDriver *usbp, usbep_t ep)
{
    (void)ep; /* should have ep == CONSOLE_ENDPOINT, so use that to save time/space */
    uint8_t *buf;
    size_t n;

    osalSysLockFromISR();

    /* rearm the timer */
    chVTSetI(&console_flush_timer, MS2ST(CONSOLE_FLUSH_MS), console_flush_cb, (void *)usbp);

    /* Freeing the buffer just transmitted, if it was not a zero size packet.*/
    if (usbp->epc[CONSOLE_ENDPOINT]->in_state->txsize > 0U)
    {
        obqReleaseEmptyBufferI(&console_buf_queue);
    }

    /* Checking if there is a buffer ready for transmission.*/
    buf = obqGetFullBufferI(&console_buf_queue, &n);

    if (buf != NULL)
    {
        /* The endpoint cannot be busy, we are in the context of the callback,
           so it is safe to transmit without a check.*/
        /* Should have n == CONSOLE_EPSIZE; check it? */
        usbStartTransmitI(usbp, CONSOLE_ENDPOINT, buf, CONSOLE_EPSIZE);
    }
    else
    {
        /* Nothing to transmit.*/
    }

    osalSysUnlockFromISR();
}

/* Callback when data is inserted into the output queue
 * Called from a locked state */
void console_queue_onotify(io_buffers_queue_t *bqp)
{
    size_t n;
    USBDriver *usbp = bqGetLinkX(bqp);

    if (usbGetDriverStateI(usbp) != USB_ACTIVE)
        return;

    /* Checking if there is already a transaction ongoing on the endpoint.*/
    if (!usbGetTransmitStatusI(usbp, CONSOLE_ENDPOINT))
    {
        /* Trying to get a full buffer.*/
        uint8_t *buf = obqGetFullBufferI(&console_buf_queue, &n);
        if (buf != NULL)
        {
            /* Buffer found, starting a new transaction.*/
            /* Should have n == CONSOLE_EPSIZE; check this? */
            usbStartTransmitI(usbp, CONSOLE_ENDPOINT, buf, CONSOLE_EPSIZE);
        }
    }
}

/* Flush timer code
 * callback (called from ISR, unlocked state) */
static void console_flush_cb(void *arg)
{
    USBDriver *usbp = (USBDriver *)arg;
    osalSysLockFromISR();

    /* check that the states of things are as they're supposed to */
    if (usbGetDriverStateI(usbp) != USB_ACTIVE)
    {
        /* rearm the timer */
        chVTSetI(&console_flush_timer, MS2ST(CONSOLE_FLUSH_MS), console_flush_cb, (void *)usbp);
        osalSysUnlockFromISR();
        return;
    }

    /* If there is already a transaction ongoing then another one cannot be
       started.*/
    if (usbGetTransmitStatusI(usbp, CONSOLE_ENDPOINT))
    {
        /* rearm the timer */
        chVTSetI(&console_flush_timer, MS2ST(CONSOLE_FLUSH_MS), console_flush_cb, (void *)usbp);
        osalSysUnlockFromISR();
        return;
    }

    /* Checking if there only a buffer partially filled, if so then it is
       enforced in the queue and transmitted.*/
    if (obqTryFlushI(&console_buf_queue))
    {
        size_t n, i;
        uint8_t *buf = obqGetFullBufferI(&console_buf_queue, &n);

        osalDbgAssert(buf != NULL, "queue is empty");

        /* zero the rest of the buffer (buf should point to allocated space) */
        for (i = n; i < CONSOLE_EPSIZE; i++)
            buf[i] = 0;
        usbStartTransmitI(usbp, CONSOLE_ENDPOINT, buf, CONSOLE_EPSIZE);
    }

    /* rearm the timer */
    chVTSetI(&console_flush_timer, MS2ST(CONSOLE_FLUSH_MS), console_flush_cb, (void *)usbp);
    osalSysUnlockFromISR();
}

int8_t sendchar(uint8_t c)
{
    osalSysLock();
    if (usbGetDriverStateI(&USB_DRIVER) != USB_ACTIVE)
    {
        osalSysUnlock();
        return 0;
    }
    osalSysUnlock();
    /* Timeout after 100us if the queue is full.
     * Increase this timeout if too much stuff is getting
     * dropped (i.e. the buffer is getting full too fast
     * for USB/HIDRAW to dequeue). Another possibility
     * for fixing this kind of thing is to increase
     * CONSOLE_QUEUE_CAPACITY. */
    return (obqPutTimeout(&console_buf_queue, c, US2ST(100)));
}

#else  /* CONSOLE_ENABLE */
int8_t sendchar(uint8_t c)
{
    (void)c;
    return 0;
}
#endif /* CONSOLE_ENABLE */

void sendchar_pf(void *p, char c)
{
    (void)p;
    sendchar((uint8_t)c);
}

__attribute__((weak)) void restart_usb_driver(USBDriver *usbp)
{
    usbStop(usbp);
    usbDisconnectBus(usbp);

#if USB_SUSPEND_WAKEUP_DELAY > 0
    // Some hubs, kvm switches, and monitors do
    // weird things, with USB device state bouncing
    // around wildly on wakeup, yielding race
    // conditions that can corrupt the keyboard state.
    //
    // Pause for a while to let things settle...
    wait_ms(USB_SUSPEND_WAKEUP_DELAY);
#endif

    usbStart(usbp, &usbcfg);
    usbConnectBus(usbp);
}

// set function HZ
static bool usb_set_poll_mode_by_hz(uint16_t hz)
{
    switch (hz) {
        case 8000:
            g_usb_poll_mode = USB_POLL_8000;
            return true;
        case 4000:
            g_usb_poll_mode = USB_POLL_4000;
            return true;
        case 2000:
            g_usb_poll_mode = USB_POLL_2000;
            return true;
        case 1000:
            g_usb_poll_mode = USB_POLL_1000;
            return true;
        default:
            return false;
    }
}

// get polling rate
uint8_t get_usb_poll_rate(void)
{
    return g_usb_poll_mode;
}
uint16_t get_usb_poll_rate_hz(void)
{
    switch (g_usb_poll_mode){
        case USB_POLL_8000:  return 8000;
        case USB_POLL_4000:  return 4000;
        case USB_POLL_2000:  return 2000;
        case USB_POLL_1000:  return 1000;
        default: return 8000;
    }
}

void change_usb_driver(USBDriver *usbp, usb_poll_mode_t mode)
{
    g_usb_poll_mode = mode;
    restart_usb_driver(usbp);
}

void usb_force_reenumeration(void) {
    uprintf("usb_force_reenumeration\n");

    //wait_ms(50);
    //reset_keyboard();

    uprintf("Force USB Re-enumeration (No System Reset)...\n");

    // 1. Disconnect the USB bus (this pulls D+ low and makes the PC detect device removal)
    usbDisconnectBus(&USBD2);

    // 2. Wait 500ms to ensure the host OS has enough time to process the disconnect
    chThdSleepMilliseconds(500);

    // 3. Reconnect the USB bus (this triggers the host to re-read descriptors)
    usbConnectBus(&USBD2);

    uprintf("USB Re-connected with new settings.\n");
}

// void usb_update_polling_rate(uint16_t polling_rate) {
//     uint8_t interval_ms = polling_hz_to_interval_ms(polling_rate);

//     uprintf("[USB] usb_update_polling_rate request=%u Hz (interval=%u ms), current=%u Hz\n",
//             polling_rate, interval_ms, s_usb_polling_hz_current);

//     // If no change, do nothing
//     if (s_usb_polling_hz_current == polling_rate) {
//         uprintf("[USB] Polling unchanged -> skip re-enumeration\n");
//         return;
//     }

//     // Apply new interval (descriptor selection variable)
//     usb_set_shared_polling_ms(interval_ms);

//     uprintf("[USB] Polling change: %u Hz -> %u Hz (interval=%u ms)\n",
//             s_usb_polling_hz_current, polling_rate, interval_ms);

//     // Update current value BEFORE reset/reenum
//     s_usb_polling_hz_current = polling_rate;

//     uprintf("[USB] Force re-enumeration\n");
//     usb_force_reenumeration();
// }

// set polling rate
void usb_update_polling_rate(uint16_t polling_rate)
{
    uint16_t current = get_usb_poll_rate_hz();

    uprintf("[USB] request=%u Hz, current=%u Hz\r\n", polling_rate, current);

    if (current == polling_rate) {
        uprintf("[USB] unchanged, skip\r\n");
        return;
    }

    if (!usb_set_poll_mode_by_hz(polling_rate)) {
        uprintf("[USB] invalid polling rate: %u\r\n", polling_rate);
        return;
    }

    uprintf("[USB] switching to %u Hz\r\n", polling_rate);
    restart_usb_driver(&USBD2);
}

void usb_set_polling_rate(uint8_t idx)
{
    uint8_t new_mode;

    switch (idx) {
        case 0:
            new_mode = USB_POLL_8000;
            break;

        case 1:
            new_mode = USB_POLL_4000;
            break;

        case 2:
            new_mode = USB_POLL_2000;
            break;

        case 3:
            new_mode = USB_POLL_1000;
            break;

        default:
            return;
    }

    if (g_usb_poll_mode == new_mode) {
        return;
    }

    g_usb_poll_mode = new_mode;

    restart_usb_driver(&USBD2);
}




