/*
Copyright 2011,2012 Jun Wako <wakojun@gmail.com>

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 2 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#ifndef REPORT_H
#define REPORT_H

#include <stdint.h>
#include "keycode.h"


/* report id */
#define REPORT_ID_MOUSE     1
#define REPORT_ID_SYSTEM    2
#define REPORT_ID_CONSUMER  3
#define REPORT_ID_GAMEPAD   4

/* mouse buttons */
#define MOUSE_BTN1 (1<<0)
#define MOUSE_BTN2 (1<<1)
#define MOUSE_BTN3 (1<<2)
#define MOUSE_BTN4 (1<<3)
#define MOUSE_BTN5 (1<<4)

// /* Consumer Page(0x0C)
//  * following are supported by Windows:
//  http://msdn.microsoft.com/en-us/windows/hardware/gg463372.aspx
//  */
// #define AUDIO_MUTE              0x00E2
// #define AUDIO_VOL_UP            0x00E9
// #define AUDIO_VOL_DOWN          0x00EA
// #define TRANSPORT_NEXT_TRACK    0x00B5
// #define TRANSPORT_PREV_TRACK    0x00B6
// #define TRANSPORT_STOP          0x00B7
// #define TRANSPORT_STOP_EJECT    0x00CC
// #define TRANSPORT_PLAY_PAUSE    0x00CD
// #define SCREEN_BRIGHT_UP        0x006F
// #define SCREEN_BRIGHT_DOWN      0x0070
// /* application launch */
// #define APPLAUNCH_CC_CONFIG     0x0183
// #define APPLAUNCH_EMAIL         0x018A
// #define APPLAUNCH_CALCULATOR    0x0192
// #define APPLAUNCH_LOCAL_BROWSER 0x0194
// /* application control */
// #define APPCONTROL_SEARCH       0x00B4
// #define APPCONTROL_HOME         0x00B5
// #define APPCONTROL_BACK         0x00B6
// #define APPCONTROL_FORWARD      0x00B7
// #define APPCONTROL_STOP         0x00B8
// #define APPCONTROL_REFRESH      0x00B9
// #define APPCONTROL_BOOKMARKS    0x00BA
// /* supplement for Bluegiga iWRAP HID(not supported by Windows?) */
// #define APPLAUNCH_LOCK          0x019E
// #define TRANSPORT_RECORD        0x00B2
// #define TRANSPORT_FAST_FORWARD  0x00BB
// #define TRANSPORT_REWIND        0x00BC
// #define TRANSPORT_EJECT         0x00B8
// #define APPCONTROL_MINIMIZE     0x0206

enum consumer_usages {
    // 15.5 Display Controls
    SNAPSHOT               = 0x065,
    BRIGHTNESS_UP          = 0x06F, // https://www.usb.org/sites/default/files/hutrr41_0.pdf
    BRIGHTNESS_DOWN        = 0x070,
    // 15.7 Transport Controls
    TRANSPORT_RECORD       = 0x0B2,
    TRANSPORT_FAST_FORWARD = 0x0B3,
    TRANSPORT_REWIND       = 0x0B4,
    TRANSPORT_NEXT_TRACK   = 0x0B5,
    TRANSPORT_PREV_TRACK   = 0x0B6,
    TRANSPORT_STOP         = 0x0B7,
    TRANSPORT_EJECT        = 0x0B8,
    TRANSPORT_RANDOM_PLAY  = 0x0B9,
    TRANSPORT_STOP_EJECT   = 0x0CC,
    TRANSPORT_PLAY_PAUSE   = 0x0CD,
    // 15.9.1 Audio Controls - Volume
    AUDIO_MUTE             = 0x0E2,
    AUDIO_VOL_UP           = 0x0E9,
    AUDIO_VOL_DOWN         = 0x0EA,
    // 15.15 Application Launch Buttons
    AL_CC_CONFIG           = 0x183,
    AL_EMAIL               = 0x18A,
    AL_CALCULATOR          = 0x192,
    AL_LOCAL_BROWSER       = 0x194,
    AL_LOCK                = 0x19E,
    AL_CONTROL_PANEL       = 0x19F,
    AL_ASSISTANT           = 0x1CB,
    AL_KEYBOARD_LAYOUT     = 0x1AE,
    // 15.16 Generic GUI Application Controls
    AC_NEW                 = 0x201,
    AC_OPEN                = 0x202,
    AC_CLOSE               = 0x203,
    AC_EXIT                = 0x204,
    AC_MAXIMIZE            = 0x205,
    AC_MINIMIZE            = 0x206,
    AC_SAVE                = 0x207,
    AC_PRINT               = 0x208,
    AC_PROPERTIES          = 0x209,
    AC_UNDO                = 0x21A,
    AC_COPY                = 0x21B,
    AC_CUT                 = 0x21C,
    AC_PASTE               = 0x21D,
    AC_SELECT_ALL          = 0x21E,
    AC_FIND                = 0x21F,
    AC_SEARCH              = 0x221,
    AC_HOME                = 0x223,
    AC_BACK                = 0x224,
    AC_FORWARD             = 0x225,
    AC_STOP                = 0x226,
    AC_REFRESH             = 0x227,
    AC_BOOKMARKS           = 0x22A
};

/* Generic Desktop Page(0x01) - system power control */
#define SYSTEM_POWER_DOWN       0x0081
#define SYSTEM_SLEEP            0x0082
#define SYSTEM_WAKE_UP          0x0083


/* key report size(NKRO or boot mode) */
#if defined(PROTOCOL_PJRC) && defined(NKRO_ENABLE)
#   include "usb.h"
#   define KEYBOARD_REPORT_SIZE KBD2_SIZE
#   define KEYBOARD_REPORT_KEYS (KBD2_SIZE - 2)
#   define KEYBOARD_REPORT_BITS (KBD2_SIZE - 1)

#elif defined(PROTOCOL_LUFA) && defined(NKRO_ENABLE)
#   include "protocol/lufa/descriptor.h"
#   define KEYBOARD_REPORT_SIZE NKRO_EPSIZE
#   define KEYBOARD_REPORT_KEYS (NKRO_EPSIZE - 2)
#   define KEYBOARD_REPORT_BITS (NKRO_EPSIZE - 1)
#elif defined(PROTOCOL_CHIBIOS) && defined(NKRO_ENABLE)
#   include "protocol/chibios/usb_main.h"
#   define KEYBOARD_REPORT_SIZE NKRO_EPSIZE
#   define KEYBOARD_REPORT_KEYS (NKRO_EPSIZE - 2)
#   define KEYBOARD_REPORT_BITS (NKRO_EPSIZE - 1)

#else
#   define KEYBOARD_REPORT_SIZE 8
#   define KEYBOARD_REPORT_KEYS 6
#endif


#ifdef __cplusplus
extern "C" {
#endif

/*
 * keyboard report is 8-byte array retains state of 8 modifiers and 6 keys.
 *
 * byte |0       |1       |2       |3       |4       |5       |6       |7
 * -----+--------+--------+--------+--------+--------+--------+--------+--------
 * desc |mods    |reserved|keys[0] |keys[1] |keys[2] |keys[3] |keys[4] |keys[5]
 *
 * It is exended to 16 bytes to retain 120keys+8mods when NKRO mode.
 *
 * byte |0       |1       |2       |3       |4       |5       |6       |7        ... |15
 * -----+--------+--------+--------+--------+--------+--------+--------+--------     +--------
 * desc |mods    |bits[0] |bits[1] |bits[2] |bits[3] |bits[4] |bits[5] |bits[6]  ... |bit[14]
 *
 * mods retains state of 8 modifiers.
 *
 *  bit |0       |1       |2       |3       |4       |5       |6       |7
 * -----+--------+--------+--------+--------+--------+--------+--------+--------
 * desc |Lcontrol|Lshift  |Lalt    |Lgui    |Rcontrol|Rshift  |Ralt    |Rgui
 *
 */
typedef union {
    uint8_t raw[KEYBOARD_REPORT_SIZE];
    struct {
        uint8_t mods;
        uint8_t reserved;
        uint8_t keys[KEYBOARD_REPORT_KEYS];
    };
#ifdef NKRO_ENABLE
    struct {
        uint8_t mods;
        uint8_t bits[KEYBOARD_REPORT_BITS];
    } nkro;
#endif
} __attribute__ ((packed)) report_keyboard_t;
/*
typedef struct {
    uint8_t mods;
    uint8_t reserved;
    uint8_t keys[REPORT_KEYS];
} __attribute__ ((packed)) report_keyboard_t;
*/

typedef struct {
    uint8_t buttons;
    int8_t x;
    int8_t y;
    int8_t v;
    int8_t h;
} __attribute__ ((packed)) report_mouse_t;

typedef struct {
    uint8_t reportID;
    uint8_t data[63];
    bool updated;
    uint8_t length;
} __attribute__ ((packed)) report_lamp_array_t;

typedef struct {
    uint16_t lampCount;
    uint32_t boxWidth;
    uint32_t boxHeight;
    uint32_t boxDepth;
    uint32_t kind;
    uint32_t minUpdateInterval;
} __attribute__ ((packed)) report_lamp_array_attrib_report;

typedef struct {
    uint16_t lampID;
    uint32_t postionX;
    uint32_t postionY;
    uint32_t postionZ;
    uint32_t updataLatency;
    uint32_t purpose;
    uint8_t redLevelCount;
    uint8_t greenLevelCount;
    uint8_t blueLevelCount;
    uint8_t intensityLevelCount;
    uint8_t isPorgrammable;
    uint8_t inputBinding;
} __attribute__ ((packed)) report_lamp_array_attrib_request_report;

typedef struct {
	uint8_t red;
	uint8_t green;
	uint8_t blue;
} led_lamp_config_t;

 enum apiCmdCode{
	apiControl_LED_Mode	= 0x00,
    apiKeymatrix_GetControl,
    apiKeymatrix_SetControl,
    apiKeymatrix_Init,
    

	apiFactory_Version = 0x40,
    apiFactory_Boot = 0x41,
    apiFactory_Reset = 0x42,

    apiTrigger_Reset = 0x70,
    apiTrigger_GetValue = 0x71,
    apiTrigger_SetValue = 0x72,    
    apiRelease_GetValue = 0x73,
    apiRelease_SetValue = 0x74,

    apiRapidTrigger_Reset = 0x75,
    apiRapidTrigger_GetValue = 0x76,
    apiRapidTrigger_SetValue = 0x77,       

	apiReamp_Version = 0x80,
    apiRemap_GetKeyValue = 0x81,
    apiRemap_GetKeyValues = 0x82, //
    apiRemap_SetKeyValue = 0x83,
    apiRemap_SetKeyValueTrigger = 0x84,
    apiRemap_Reset = 0x85,
    apiMacro_Info = 0x86,
    apiMacro_Read = 0x87,
    apiMacro_Write = 0x88,
    apiPostionRead = 0x89,
    apiPostionSet = 0x8a,
    apiRemap_getLayerCount = 0x8b,

	apiReserved_code = 0xc0,
};


typedef struct {
	uint8_t type;
	uint8_t	mode;
	uint8_t	brightness;
	uint8_t speed;
} led_runMode_t;

typedef union {
    uint32_t raw;
    struct{
        uint8_t cmd : 4;
        uint8_t mode : 4;
        // uint8_t count : 8;
        // uint8_t row : 8;
        // uint8_t col : 8;
    }

} trm_calibration_t;

typedef struct {
    uint8_t lampUpdateFlags;
    uint16_t start;
    uint16_t end;
    led_lamp_config_t ledUpdateChannel;
    uint8_t intensityUpdateChannel;
} __attribute__ ((packed)) report_lamp_array_rangeUpdate;

typedef struct
{
  uint8_t report_ID;                                 // REPORT ID byte
                                                     // Collection: CA:GamePad CP:
  uint16_t GD_GamePadX;                              // Usage 0x00010030: X, Value = 0 to 65535
  uint16_t GD_GamePadY;                              // Usage 0x00010031: Y, Value = 0 to 65535
  uint16_t GD_GamePadRx;                             // Usage 0x00010033: Rx, Value = 0 to 65535
  uint16_t GD_GamePadRy;                             // Usage 0x00010034: Ry, Value = 0 to 65535
                                                     // Collection: CA:GamePad
  uint16_t GD_GamePadRz : 10;                         // Usage 0x00010032: Z, Value = 0 to 1023
  uint8_t  : 6;                                      // Pad
  uint16_t GD_GamePadZ : 10;                        // Usage 0x00010035: Rz, Value = 0 to 1023
  uint8_t  : 6;                                      // Pad
  uint8_t  BTN_GamePadButton1 : 1;                   // Usage 0x00090001: Button 1 Primary/trigger, Value = 0 to 0
  uint8_t  BTN_GamePadButton2 : 1;                   // Usage 0x00090002: Button 2 Secondary, Value = 0 to 0
  uint8_t  BTN_GamePadButton3 : 1;                   // Usage 0x00090003: Button 3 Tertiary, Value = 0 to 0
  uint8_t  BTN_GamePadButton4 : 1;                   // Usage 0x00090004: Button 4, Value = 0 to 0
  uint8_t  BTN_GamePadButton5 : 1;                   // Usage 0x00090005: Button 5, Value = 0 to 0
  uint8_t  BTN_GamePadButton6 : 1;                   // Usage 0x00090006: Button 6, Value = 0 to 0
  uint8_t  BTN_GamePadButton7 : 1;                   // Usage 0x00090007: Button 7, Value = 0 to 0
  uint8_t  BTN_GamePadButton8 : 1;                   // Usage 0x00090008: Button 8, Value = 0 to 0
  uint8_t  BTN_GamePadButton9 : 1;                   // Usage 0x00090009: Button 9, Value = 0 to 0
  uint8_t  BTN_GamePadButton10 : 1;                  // Usage 0x0009000A: Button 10, Value = 0 to 0
  uint8_t  : 6;                                      // Pad
  uint8_t  GD_GamePadHatSwitch : 4;                  // Usage 0x00010039: Hat switch, Value = 1 to 8, Physical = (Value - 1) x 45 in degrees
  uint8_t  : 4;                                      // Pad
                                                     // Collection: CA:GamePad CP:SystemControl
  uint8_t  GD_GamePadSystemControlSystemMainMenu : 1; // Usage 0x00010085: System Main Menu, Value = 0 to 1
  uint8_t  : 7;                                      // Pad
                                                     // Collection: CA:GamePad
  uint8_t  GEN_GamePadBatteryStrength;               // Usage 0x00060020: Battery Strength, Value = 0 to 255
} __attribute__ ((packed)) report_gamepad_report;


/* keycode to system usage */
#define KEYCODE2SYSTEM(key) \
    (key == KC_SYSTEM_POWER ? SYSTEM_POWER_DOWN : \
    (key == KC_SYSTEM_SLEEP ? SYSTEM_SLEEP : \
    (key == KC_SYSTEM_WAKE  ? SYSTEM_WAKE_UP : 0)))

// /* keycode to consumer usage */
// #define KEYCODE2CONSUMER(key) \
//     (key == KC_AUDIO_MUTE           ?  AUDIO_MUTE : \
//     (key == KC_AUDIO_VOL_UP         ?  AUDIO_VOL_UP : \
//     (key == KC_AUDIO_VOL_DOWN       ?  AUDIO_VOL_DOWN : \
//     (key == KC_MEDIA_NEXT_TRACK     ?  TRANSPORT_NEXT_TRACK : \
//     (key == KC_MEDIA_PREV_TRACK     ?  TRANSPORT_PREV_TRACK : \
//     (key == KC_MEDIA_FAST_FORWARD   ?  TRANSPORT_FAST_FORWARD : \
//     (key == KC_MEDIA_REWIND         ?  TRANSPORT_REWIND : \
//     (key == KC_MEDIA_STOP           ?  TRANSPORT_STOP : \
//     (key == KC_MEDIA_EJECT          ?  TRANSPORT_STOP_EJECT : \
//     (key == KC_MEDIA_PLAY_PAUSE     ?  TRANSPORT_PLAY_PAUSE : \
//     (key == KC_MEDIA_SELECT         ?  APPLAUNCH_CC_CONFIG : \
//     (key == KC_MAIL                 ?  APPLAUNCH_EMAIL : \
//     (key == KC_CALCULATOR           ?  APPLAUNCH_CALCULATOR : \
//     (key == KC_MY_COMPUTER          ?  APPLAUNCH_LOCAL_BROWSER : \
//     (key == KC_WWW_SEARCH           ?  APPCONTROL_SEARCH : \
//     (key == KC_WWW_HOME             ?  APPCONTROL_HOME : \
//     (key == KC_WWW_BACK             ?  APPCONTROL_BACK : \
//     (key == KC_WWW_FORWARD          ?  APPCONTROL_FORWARD : \
//     (key == KC_WWW_STOP             ?  APPCONTROL_STOP : \
//     (key == KC_WWW_REFRESH          ?  APPCONTROL_REFRESH : \
//     (key == KC_WWW_FAVORITES        ?  APPCONTROL_BOOKMARKS :  \
//     (key == KC_MEDIA_FAST_FORWARD   ?  APPCONTROL_BOOKMARKS :  \
//     (key == KC_MEDIA_REWIND         ?  APPCONTROL_BOOKMARKS :  \
//     (key == KC_BRIGHTNESS_UP        ?  SCREEN_BRIGHT_UP :  \
//     (key == KC_BRIGHTNESS_DOWN      ?  SCREEN_BRIGHT_DOWN :  \
//     (key == KC_WWW_FAVORITES        ?  APPCONTROL_BOOKMARKS :
//     0))))))))))))))))))))))))))

static inline uint16_t KEYCODE2CONSUMER(uint8_t key) {
    switch (key) {
        case KC_AUDIO_MUTE:
            return AUDIO_MUTE;
        case KC_AUDIO_VOL_UP:
            return AUDIO_VOL_UP;
        case KC_AUDIO_VOL_DOWN:
            return AUDIO_VOL_DOWN;
        case KC_MEDIA_NEXT_TRACK:
            return TRANSPORT_NEXT_TRACK;
        case KC_MEDIA_PREV_TRACK:
            return TRANSPORT_PREV_TRACK;
        case KC_MEDIA_FAST_FORWARD:
            return TRANSPORT_FAST_FORWARD;
        case KC_MEDIA_REWIND:
            return TRANSPORT_REWIND;
        case KC_MEDIA_STOP:
            return TRANSPORT_STOP;
        case KC_MEDIA_EJECT:
            return TRANSPORT_STOP_EJECT;
        case KC_MEDIA_PLAY_PAUSE:
            return TRANSPORT_PLAY_PAUSE;
        case KC_MEDIA_SELECT:
            return AL_CC_CONFIG;
        case KC_MAIL:
            return AL_EMAIL;
        case KC_CALCULATOR:
            return AL_CALCULATOR;
        case KC_MY_COMPUTER:
            return AL_LOCAL_BROWSER;
        case KC_WWW_SEARCH:
            return AC_SEARCH;
        case KC_WWW_HOME:
            return AC_HOME;
        case KC_WWW_BACK:
            return AC_BACK;
        case KC_WWW_FORWARD:
            return AC_FORWARD;
        case KC_WWW_STOP:
            return AC_STOP;
        case KC_WWW_REFRESH:
            return AC_REFRESH;
        case KC_BRIGHTNESS_UP:
            return BRIGHTNESS_UP;
        case KC_BRIGHTNESS_DOWN:
            return BRIGHTNESS_DOWN;
        case KC_WWW_FAVORITES:
            return AC_BOOKMARKS;
        default:
            return 0;
    }
}

#ifdef __cplusplus
}
#endif

#endif
