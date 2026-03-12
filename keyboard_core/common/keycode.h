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

/*
 * Keycodes based on HID Usage Keyboard/Keypad Page(0x07) plus special codes
 * http://www.usb.org/developers/devclass_docs/Hut1_12.pdf
 */

#include "action_code.h"

#ifndef KEYCODE_H
#define KEYCODE_H


#define IS_ERROR(code)           (KC_ROLL_OVER <= (code) && (code) <= KC_UNDEFINED)
#define IS_ANY(code)             (KC_A         <= (code) && (code) <= 0xFF)
#define IS_KEY(code)             (KC_A         <= (code) && (code) <= KC_EXSEL)
#define IS_MOD(code)             (KC_LCTRL     <= (code) && (code) <= KC_RGUI)


#define IS_SPECIAL(code)         ((0xA5 <= (code) && (code) <= 0xDF) || (0xE8 <= (code) && (code) <= 0xFF))
#define IS_SYSTEM(code)          (KC_PWR <= (code) && (code) <= KC_WAKE)
#define IS_CONSUMER(code)        (KC_MUTE <= (code) && (code) <= KC_BRID)
#define IS_GAMEPAD(code)         (KC_GAME_PAD_DPAD_UP      <= (code) && (code) <= KC_GAME_PAD_BUTTON_31)
#define IS_FN(code)              (KC_FN0       <= (code) && (code) <= KC_FN31)
#define IS_MOUSEKEY(code)        (KC_MS_UP     <= (code) && (code) <= KC_MS_ACCEL2)
#define IS_MOUSEKEY_MOVE(code)   (KC_MS_UP     <= (code) && (code) <= KC_MS_RIGHT)
#define IS_MOUSEKEY_BUTTON(code) (KC_MS_BTN1   <= (code) && (code) <= KC_MS_BTN5)
#define IS_MOUSEKEY_WHEEL(code)  (KC_MS_WH_UP  <= (code) && (code) <= KC_MS_WH_RIGHT)
#define IS_MOUSEKEY_ACCEL(code)  (KC_MS_ACCEL0 <= (code) && (code) <= KC_MS_ACCEL2)

#define MOD_BIT(code)   (1<<MOD_INDEX(code))
#define MOD_INDEX(code) ((code) & 0x07)
#define FN_BIT(code)    (1<<FN_INDEX(code))
#define FN_INDEX(code)  ((code) - KC_FN0)
#define FN_MIN          KC_FN0
#define FN_MAX          KC_FN31


/*
 * Short names for ease of definition of keymap
 */
#define KC_LCTL KC_LCTRL
#define KC_RCTL KC_RCTRL
#define KC_LSFT KC_LSHIFT
#define KC_RSFT KC_RSHIFT
#define KC_ESC  KC_ESCAPE
#define KC_BSPC KC_BSPACE
#define KC_ENT  KC_ENTER
#define KC_DEL  KC_DELETE
#define KC_INS  KC_INSERT
#define KC_CAPS KC_CAPSLOCK
#define KC_CLCK KC_CAPSLOCK
#define KC_RGHT KC_RIGHT
#define KC_PGDN KC_PGDOWN
#define KC_PSCR KC_PSCREEN
#define KC_SLCK KC_SCROLLLOCK
#define KC_PAUS KC_PAUSE
#define KC_BRK  KC_PAUSE
#define KC_NLCK KC_NUMLOCK
#define KC_SPC  KC_SPACE
#define KC_MINS KC_MINUS
#define KC_EQL  KC_EQUAL
#define KC_GRV  KC_GRAVE
#define KC_RBRC KC_RBRACKET
#define KC_LBRC KC_LBRACKET
#define KC_COMM KC_COMMA
#define KC_BSLS KC_BSLASH
#define KC_SLSH KC_SLASH
#define KC_SCLN KC_SCOLON
#define KC_QUOT KC_QUOTE
#define KC_APP  KC_APPLICATION
#define KC_NUHS KC_NONUS_HASH
#define KC_NUBS KC_NONUS_BSLASH
#define KC_LCAP KC_LOCKING_CAPS
#define KC_LNUM KC_LOCKING_NUM
#define KC_LSCR KC_LOCKING_SCROLL
#define KC_ERAS KC_ALT_ERASE
#define KC_CLR  KC_CLEAR
/* Japanese specific */
#define KC_ZKHK KC_GRAVE
#define KC_RO   KC_INT1
#define KC_KANA KC_INT2
#define KC_JYEN KC_INT3
#define KC_JPY  KC_INT3
#define KC_HENK KC_INT4
#define KC_MHEN KC_INT5
/* Korean specific */
#define KC_HAEN KC_LANG1
#define KC_HANJ KC_LANG2
/* Keypad */
#define KC_P1   KC_KP_1
#define KC_P2   KC_KP_2
#define KC_P3   KC_KP_3
#define KC_P4   KC_KP_4
#define KC_P5   KC_KP_5
#define KC_P6   KC_KP_6
#define KC_P7   KC_KP_7
#define KC_P8   KC_KP_8
#define KC_P9   KC_KP_9
#define KC_P0   KC_KP_0
#define KC_PDOT KC_KP_DOT
#define KC_PCMM KC_KP_COMMA
#define KC_PSLS KC_KP_SLASH
#define KC_PAST KC_KP_ASTERISK
#define KC_PMNS KC_KP_MINUS
#define KC_PPLS KC_KP_PLUS
#define KC_PEQL KC_KP_EQUAL
#define KC_PENT KC_KP_ENTER
/* Unix function key */
#define KC_EXEC KC_EXECUTE
#define KC_SLCT KC_SELECT
#define KC_AGIN KC_AGAIN
#define KC_PSTE KC_PASTE
/* Mousekey */
#define KC_MS_U KC_MS_UP
#define KC_MS_D KC_MS_DOWN
#define KC_MS_L KC_MS_LEFT
#define KC_MS_R KC_MS_RIGHT
#define KC_BTN1 KC_MS_BTN1
#define KC_BTN2 KC_MS_BTN2
#define KC_BTN3 KC_MS_BTN3
#define KC_BTN4 KC_MS_BTN4
#define KC_BTN5 KC_MS_BTN5
#define KC_WH_U KC_MS_WH_UP
#define KC_WH_D KC_MS_WH_DOWN
#define KC_WH_L KC_MS_WH_LEFT
#define KC_WH_R KC_MS_WH_RIGHT
#define KC_ACL0 KC_MS_ACCEL0
#define KC_ACL1 KC_MS_ACCEL1
#define KC_ACL2 KC_MS_ACCEL2
/* Sytem Control */
#define KC_PWR  KC_SYSTEM_POWER
#define KC_SLEP KC_SYSTEM_SLEEP
#define KC_WAKE KC_SYSTEM_WAKE
/* Consumer Page */
#define KC_MUTE KC_AUDIO_MUTE
#define KC_VOLU KC_AUDIO_VOL_UP
#define KC_VOLD KC_AUDIO_VOL_DOWN
#define KC_MNXT KC_MEDIA_NEXT_TRACK
#define KC_MPRV KC_MEDIA_PREV_TRACK
#define KC_MFFD KC_MEDIA_FAST_FORWARD
#define KC_MRWD KC_MEDIA_REWIND
#define KC_MSTP KC_MEDIA_STOP
#define KC_MPLY KC_MEDIA_PLAY_PAUSE
#define KC_EJCT KC_MEDIA_EJECT
#define KC_MSEL KC_MEDIA_SELECT
#define KC_MAIL KC_MAIL
#define KC_CALC KC_CALCULATOR
#define KC_MYCM KC_MY_COMPUTER
#define KC_WSCH KC_WWW_SEARCH
#define KC_WHOM KC_WWW_HOME
#define KC_WBAK KC_WWW_BACK
#define KC_WFWD KC_WWW_FORWARD
#define KC_WSTP KC_WWW_STOP
#define KC_WREF KC_WWW_REFRESH
#define KC_WFAV KC_WWW_FAVORITES
#define KC_MFFD KC_MEDIA_FAST_FORWARD
#define KC_MRWD KC_MEDIA_REWIND
#define KC_BRIU KC_BRIGHTNESS_UP
#define KC_BRID KC_BRIGHTNESS_DOWN

/* System Specific */
#define KC_BRMU KC_PAUSE
#define KC_BRMD KC_SCROLL_LOCK


/* Jump to bootloader */
#define KC_BTLD QK_BOOTLOADER
#define KC_SYRST QK_SYSTEMRESET
/* Transparent */
#define KC_TRANSPARENT  1
#define KC_TRNS KC_TRANSPARENT


enum quantum_keycodes {
    // Ranges used in shortcuts - not to be used directly
    QK_BASIC                = 0x0000,
    QK_BASIC_MAX            = 0x00FF,
    QK_MODS                 = 0x0100,
    QK_LCTL                 = 0x0100,
    QK_LSFT                 = 0x0200,
    QK_LALT                 = 0x0400,
    QK_LGUI                 = 0x0800,
    QK_RMODS_MIN            = 0x1000,
    QK_RCTL                 = 0x1100,
    QK_RSFT                 = 0x1200,
    QK_RALT                 = 0x1400,
    QK_RGUI                 = 0x1800,
    QK_MODS_MAX             = 0x1FFF,
    QK_LAYER_TAP            = 0x4000,
    QK_LAYER_TAP_MAX        = 0x4FFF,
    QK_TO                   = 0x5000,
    QK_TO_MAX               = 0x50FF,
    QK_MOMENTARY            = 0x5100,
    QK_MOMENTARY_MAX        = 0x51FF,
    QK_DEF_LAYER            = 0x5200,
    QK_DEF_LAYER_MAX        = 0x52FF,
    QK_TOGGLE_LAYER         = 0x5300,
    QK_TOGGLE_LAYER_MAX     = 0x53FF,
    QK_ONE_SHOT_LAYER       = 0x5400,
    QK_ONE_SHOT_LAYER_MAX   = 0x54FF,
    QK_ONE_SHOT_MOD         = 0x5500,
    QK_ONE_SHOT_MOD_MAX     = 0x55FF,
    QK_SWAP_HANDS           = 0x5600,
    QK_SWAP_HANDS_MAX       = 0x56FF,
    QK_TAP_DANCE            = 0x5700,
    QK_TAP_DANCE_MAX        = 0x57FF,
    QK_LAYER_TAP_TOGGLE     = 0x5800,
    QK_LAYER_TAP_TOGGLE_MAX = 0x58FF,
    QK_LAYER_MOD            = 0x5900,
    QK_LAYER_MOD_MAX        = 0x59FF,
    QK_STENO                = 0x5A00,
    QK_STENO_BOLT           = 0x5A30,
    QK_STENO_GEMINI         = 0x5A31,
    QK_STENO_COMB           = 0x5A32,
    QK_STENO_COMB_MAX       = 0x5A3C,
    QK_STENO_MAX            = 0x5A3F,
    // 0x5C00 - 0x5FFF are reserved, see below
    QK_MOD_TAP             = 0x6000,

    QK_MAGIC                       = 0x7000,
    QK_MAGIC_MAX                   = 0x70FF,
    QK_MIDI                        = 0x7100,
    QK_MIDI_MAX                    = 0x71FF,
    QK_SEQUENCER                   = 0x7200,
    QK_SEQUENCER_MAX               = 0x73FF,
    KC_GAME_PAD                    = 0x7400,
    KC_GAME_PAD_MAX                = 0x743F,
    QK_PROGRAMMABLE_BUTTON         = 0x7440,
    QK_PROGRAMMABLE_BUTTON_MAX     = 0x747F,

    QK_MOD_TAP_MAX         = 0x7FFF,
    QK_UNICODE             = 0x8000,
    QK_UNICODE_MAX         = 0xFFFF,
    QK_UNICODEMAP          = 0x8000,
    QK_UNICODEMAP_MAX      = 0xBFFF,
    QK_UNICODEMAP_PAIR     = 0xC000,
    QK_UNICODEMAP_PAIR_MAX = 0xFFFF,

    // Loose keycodes - to be used directly
    QK_BOOTLOADER = 0x5C00,
    QK_DEBUG_TOGGLE, // 5C01
    QK_SYSTEMRESET,  // 5C02
};

#define TO(layer) (QK_TO | (ON_PRESS << 0x4) | ((layer)&0xFF))

// Momentary switch layer - 256 layer max
#define MO(layer) (QK_MOMENTARY | ((layer)&0xFF))

// Set default layer - 256 layer max
#define DF(layer) (QK_DEF_LAYER | ((layer)&0xFF))

// Toggle to layer - 256 layer max
#define TG(layer) (QK_TOGGLE_LAYER | ((layer)&0xFF))

// One-shot layer - 256 layer max
#define OSL(layer) (QK_ONE_SHOT_LAYER | ((layer)&0xFF))

// L-ayer M-od: Momentary switch layer with modifiers active - 16 layer max, left mods only
#define LM(layer, mod) (QK_LAYER_MOD | (((layer)&0xF) << 4) | ((mod)&0xF))

/* USB HID Keyboard/Keypad Usage(0x07) */
enum hid_keyboard_keypad_usage {
    KC_NO               = 0x00,
    KC_ROLL_OVER,
    KC_POST_FAIL,
    KC_UNDEFINED,
    KC_A,
    KC_B,
    KC_C,
    KC_D,
    KC_E,
    KC_F,
    KC_G,
    KC_H,
    KC_I,
    KC_J,
    KC_K,
    KC_L,
    KC_M,               /* 0x10 */
    KC_N,
    KC_O,
    KC_P,
    KC_Q,
    KC_R,
    KC_S,
    KC_T,
    KC_U,
    KC_V,
    KC_W,
    KC_X,
    KC_Y,
    KC_Z,
    KC_1,
    KC_2,
    KC_3,               /* 0x20 */
    KC_4,
    KC_5,
    KC_6,
    KC_7,
    KC_8,
    KC_9,
    KC_0,
    KC_ENTER,
    KC_ESCAPE,
    KC_BSPACE,
    KC_TAB,
    KC_SPACE,
    KC_MINUS,
    KC_EQUAL,
    KC_LBRACKET,
    KC_RBRACKET,        /* 0x30 */
    KC_BSLASH,          /* \ (and |) */
    KC_NONUS_HASH,      /* Non-US # and ~ (Typically near the Enter key) */
    KC_SCOLON,          /* ; (and :) */
    KC_QUOTE,           /* ' and " */
    KC_GRAVE,           /* Grave accent and tilde */
    KC_COMMA,           /* , and < */
    KC_DOT,             /* . and > */
    KC_SLASH,           /* / and ? */
    KC_CAPSLOCK,
    KC_F1,
    KC_F2,
    KC_F3,
    KC_F4,
    KC_F5,
    KC_F6,
    KC_F7,              /* 0x40 */
    KC_F8,
    KC_F9,
    KC_F10,
    KC_F11,
    KC_F12,
    KC_PSCREEN,
    KC_SCROLLLOCK,
    KC_PAUSE,
    KC_INSERT,
    KC_HOME,
    KC_PGUP,
    KC_DELETE,
    KC_END,
    KC_PGDOWN,
    KC_RIGHT,
    KC_LEFT,            /* 0x50 */
    KC_DOWN,
    KC_UP,
    KC_NUMLOCK,
    KC_KP_SLASH,
    KC_KP_ASTERISK,
    KC_KP_MINUS,
    KC_KP_PLUS,
    KC_KP_ENTER,
    KC_KP_1,
    KC_KP_2,
    KC_KP_3,
    KC_KP_4,
    KC_KP_5,
    KC_KP_6,
    KC_KP_7,
    KC_KP_8,            /* 0x60 */
    KC_KP_9,
    KC_KP_0,
    KC_KP_DOT,
    KC_NONUS_BSLASH,    /* Non-US \ and | (Typically near the Left-Shift key) */
    KC_APPLICATION,
    KC_POWER,
    KC_KP_EQUAL,
    KC_F13,
    KC_F14,
    KC_F15,
    KC_F16,
    KC_F17,
    KC_F18,
    KC_F19,
    KC_F20,
    KC_F21,             /* 0x70 */
    KC_F22,
    KC_F23,
    KC_F24,
    KC_EXECUTE,
    KC_HELP,
    KC_MENU,
    KC_SELECT,
    KC_STOP,
    KC_AGAIN,
    KC_UNDO,
    KC_CUT,
    KC_COPY,
    KC_PASTE,
    KC_FIND,
    KC__MUTE,
    KC__VOLUP,          /* 0x80 */
    KC__VOLDOWN,
    KC_LOCKING_CAPS,    /* locking Caps Lock */
    KC_LOCKING_NUM,     /* locking Num Lock */
    KC_LOCKING_SCROLL,  /* locking Scroll Lock */
    KC_KP_COMMA,
    KC_KP_EQUAL_AS400,  /* equal sign on AS/400 */
    KC_INT1,
    KC_INT2,
    KC_INT3,
    KC_INT4,
    KC_INT5,
    KC_INT6,
    KC_INT7,
    KC_INT8,
    KC_INT9,
    KC_LANG1,           /* 0x90 */
    KC_LANG2,
    KC_LANG3,
    KC_LANG4,
    KC_LANG5,
    KC_LANG6,
    KC_LANG7,
    KC_LANG8,
    KC_LANG9,
    KC_ALT_ERASE,
    KC_SYSREQ,
    KC_CANCEL,
    KC_CLEAR,
    KC_PRIOR,
    KC_RETURN,
    KC_SEPARATOR,
    KC_OUT,             /* 0xA0 */
    KC_OPER,
    KC_CLEAR_AGAIN,
    KC_CRSEL,
    KC_EXSEL,           /* 0xA4 */

    /* NOTE: 0xA5-DF are used for internal special purpose */

#if 0
    /* NOTE: Following codes(0xB0-DD) are not used. Leave them for reference. */
    KC_KP_00            = 0xB0,
    KC_KP_000,
    KC_THOUSANDS_SEPARATOR,
    KC_DECIMAL_SEPARATOR,
    KC_CURRENCY_UNIT,
    KC_CURRENCY_SUB_UNIT,
    KC_KP_LPAREN,
    KC_KP_RPAREN,
    KC_KP_LCBRACKET,    /* { */
    KC_KP_RCBRACKET,    /* } */
    KC_KP_TAB,
    KC_KP_BSPACE,
    KC_KP_A,
    KC_KP_B,
    KC_KP_C,
    KC_KP_D,
    KC_KP_E,            /* 0xC0 */
    KC_KP_F,
    KC_KP_XOR,
    KC_KP_HAT,
    KC_KP_PERC,
    KC_KP_LT,
    KC_KP_GT,
    KC_KP_AND,
    KC_KP_LAZYAND,
    KC_KP_OR,
    KC_KP_LAZYOR,
    KC_KP_COLON,
    KC_KP_HASH,
    KC_KP_SPACE,
    KC_KP_ATMARK,
    KC_KP_EXCLAMATION,
    KC_KP_MEM_STORE,    /* 0xD0 */
    KC_KP_MEM_RECALL,
    KC_KP_MEM_CLEAR,
    KC_KP_MEM_ADD,
    KC_KP_MEM_SUB,
    KC_KP_MEM_MUL,
    KC_KP_MEM_DIV,
    KC_KP_PLUS_MINUS,
    KC_KP_CLEAR,
    KC_KP_CLEAR_ENTRY,
    KC_KP_BINARY,
    KC_KP_OCTAL,
    KC_KP_DECIMAL,
    KC_KP_HEXADECIMAL,  /* 0xDD */
#endif

    /* Modifiers */
    KC_LCTRL            = 0xE0,
    KC_LSHIFT,
    KC_LALT,
    KC_LGUI,
    KC_RCTRL,
    KC_RSHIFT,
    KC_RALT,
    KC_RGUI,

    /* NOTE: 0xE8-FF are used for internal special purpose */
};

/* Special keycodes */
/* NOTE: 0xA5-DF and 0xE8-FF are used for internal special purpose */
enum internal_special_keycodes {
  /* System Control */
  KC_SYSTEM_POWER = 0xA5,
  KC_SYSTEM_SLEEP,
  KC_SYSTEM_WAKE,

  /* Consumer Page (0x0C) */
  KC_AUDIO_MUTE,
  KC_AUDIO_VOL_UP,
  KC_AUDIO_VOL_DOWN,
  KC_MEDIA_NEXT_TRACK,
  KC_MEDIA_PREV_TRACK,
  KC_MEDIA_STOP,
  KC_MEDIA_PLAY_PAUSE,
  KC_MEDIA_SELECT,
  KC_MEDIA_EJECT, // 0xB0
  KC_MAIL,
  KC_CALCULATOR, //
  KC_MY_COMPUTER,
  KC_WWW_SEARCH,
  KC_WWW_HOME,
  KC_WWW_BACK,
  KC_WWW_FORWARD,
  KC_WWW_STOP,
  KC_WWW_REFRESH,
  KC_WWW_FAVORITES,
  KC_MEDIA_FAST_FORWARD,
  KC_MEDIA_REWIND,
  KC_BRIGHTNESS_UP,
  KC_BRIGHTNESS_DOWN,

  /* Jump to bootloader */
  //   KC_SYSTEMRESET = 0xBE,
  //   KC_BOOTLOADER = 0xBF,

  /* Fn key */
  KC_FN0 = 0xC0,
  KC_FN1,
  KC_FN2,
  KC_FN3,
  KC_FN4,
  KC_FN5,
  KC_FN6,
  KC_FN7,
  KC_FN8,
  KC_FN9,
  KC_FN10,
  KC_FN11,
  KC_FN12,
  KC_FN13,
  KC_FN14,
  KC_FN15,

  KC_FN16 = 0xD0,
  KC_FN17,
  KC_FN18,
  KC_FN19,
  KC_FN20,
  KC_FN21,
  KC_FN22,
  KC_FN23,
  KC_FN24,
  KC_FN25,
  KC_FN26,
  KC_FN27,
  KC_FN28,
  KC_FN29,
  KC_FN30,
  KC_FN31, /* 0xDF */

  /**************************************/
  /* 0xE0-E7 for Modifiers. DO NOT USE. */
  /**************************************/

  /* Mousekey */
  KC_MS_UP = 0xF0,
  KC_MS_DOWN,
  KC_MS_LEFT,
  KC_MS_RIGHT,
  KC_MS_BTN1,
  KC_MS_BTN2,
  KC_MS_BTN3,
  KC_MS_BTN4,
  KC_MS_BTN5, /* 0xF8 */
  /* Mousekey wheel */
  KC_MS_WH_UP,
  KC_MS_WH_DOWN,
  KC_MS_WH_LEFT,
  KC_MS_WH_RIGHT, /* 0xFC */
  /* Mousekey accel */
  KC_MS_ACCEL0,
  KC_MS_ACCEL1,
  KC_MS_ACCEL2, /* 0xFF */

  KC_TO0 = TO(0), // 5010
  KC_TO1,
  KC_TO2,
  KC_TO3,
  KC_TO4,
  KC_TO5,

  KC_MO0 = MO(0), // 5100
  KC_MO1,
  KC_MO2,
  KC_MO3,
  KC_MO4,
  KC_MO5,
  KC_MO6,
  KC_MO7,
  KC_MO8,
  KC_MO9,
  KC_MO10,
  KC_MO11,
  KC_MO12,
  KC_MO13,
  KC_MO14,
  KC_MO15,


  KC_MACRO_0,
  KC_MACRO_1,
  KC_MACRO_2,
  KC_MACRO_3,
  KC_MACRO_4,
  KC_MACRO_5,
  KC_MACRO_6,
  KC_MACRO_7,
  KC_MACRO_8,
  KC_MACRO_9,
  KC_MACRO_10,
  KC_MACRO_11,
  KC_MACRO_12,
  KC_MACRO_13,
  KC_MACRO_14,
  KC_MACRO_15,
  KC_MACRO_16,
  KC_MACRO_17,
  KC_MACRO_18,
  KC_MACRO_19,
  KC_MACRO_20,
  KC_MACRO_21,
  KC_MACRO_22,
  KC_MACRO_23,
  KC_MACRO_24,
  KC_MACRO_25,
  KC_MACRO_26,
  KC_MACRO_27,
  KC_MACRO_28,
  KC_MACRO_29,
  KC_MACRO_30,
  KC_MACRO_31,

  // RGB underglow/matrix
  KC_RGB_TOG = 0x5CC2,     // 5CC2
  KC_RGB_MODE_FORWARD,     // 5CC3
  KC_RGB_MODE_REVERSE,     // 5CC4
  KC_RGB_HUI,              // 5CC5//
  KC_RGB_HUD,              // 5CC6
  KC_RGB_SAI,              // 5CC7
  KC_RGB_SAD,              // 5CC8
  KC_RGB_VAI,              // 5CC9
  KC_RGB_VAD,              // 5CCA
  KC_RGB_SPI,              // 5CCB
  KC_RGB_SPD,              // 5CCC
  KC_RGB_MODE_RAINBOWFLOW, // 5CCD
  KC_RGB_MODE_COLORSHIFT,  // 5CCE
  KC_RGB_MODE_COLORBREATH, // 5CCF
  KC_RGB_MODE_FLASH,       // 5CD0
  KC_RGB_MODE_SWIRL,       // 5CD1
  KC_RGB_MODE_KNIGHT,      // 5CD2
  KC_RGB_MODE_XMAS,        // 5CD3
  KC_RGB_MODE_GRADIENT,    // 5CD4
  KC_RGB_MODE_RGBTEST,     // 5CD5
  KC_RGB_ON,               // 5CD6
  KC_RGB_OFF,              // 5CD7

  // KC_RGB_TOG = 0x5CC3,  // 5CC3
  // KC_RGB_ON,            // 5CC4
  // KC_RGB_OFF,           // 5CC5
  // KC_RGB_MODE_FORWARD,  // 5CC6
  // KC_RGB_MODE_REVERSE,  // 5CC7
  // KC_RGB_HUI,           // 5CC8
  // KC_RGB_HUD,           // 5CC9
  // KC_RGB_SAI,           // 5CCA
  // KC_RGB_SAD,           // 5CCB
  // KC_RGB_VAI,           // 5CCC
  // KC_RGB_VAD,           // 5CCD
  // KC_RGB_SPI,           // 5CCE
  // KC_RGB_SPD,           // 5CCF
  // KC_RGB_MODE_RAINBOWFLOW,    // 5CD0
  // KC_RGB_MODE_COLORSHIFT,     // 5CD1
  // KC_RGB_MODE_COLORBREATH,   // 5CD2
  // KC_RGB_MODE_FLASH,          // 5CD3
  // KC_RGB_MODE_SWIRL,          // 5CD4
  // KC_RGB_MODE_KNIGHT,         // 5CD5
  // KC_RGB_MODE_XMAS,           // 5CD6
  // KC_RGB_MODE_GRADIENT,       // 5CD7
  // KC_RGB_MODE_RGBTEST,        // 5CD8

  KC_RAPID_TOG = 0x5CE0,
  KC_RAPID_ON,
  KC_RAPID_OFF,
  KC_RAPID_LVI,
  KC_RAPID_LVD,


  KC_GAME_PAD_DPAD_UP = 0x5CF0,
  KC_GAME_PAD_DPAD_DOWN,
  KC_GAME_PAD_DPAD_LEFT,
  KC_GAME_PAD_DPAD_RIGHT,

  KC_GAME_PAD_ANALOG_X_PLUSE = 0x5D00,
  KC_GAME_PAD_ANALOG_X_MINUS,
  KC_GAME_PAD_ANALOG_Y_PLUSE,
  KC_GAME_PAD_ANALOG_Y_MINUS,
  KC_GAME_PAD_ANALOG_Z_PLUSE,
  KC_GAME_PAD_ANALOG_Z_MINUS,
  KC_GAME_PAD_ANALOG_RZ_PLUSE,
  KC_GAME_PAD_ANALOG_RZ_MINUS,
  KC_GAME_PAD_ANALOG_RX_PLUSE,
  KC_GAME_PAD_ANALOG_RX_MINUS,
  KC_GAME_PAD_ANALOG_RY_PLUSE,
  KC_GAME_PAD_ANALOG_RY_MINUS,

  KC_GAME_PAD_BUTTON_0,
  KC_GAME_PAD_BUTTON_1,
  KC_GAME_PAD_BUTTON_2,
  KC_GAME_PAD_BUTTON_3,
  KC_GAME_PAD_BUTTON_4,
  KC_GAME_PAD_BUTTON_5,
  KC_GAME_PAD_BUTTON_6,
  KC_GAME_PAD_BUTTON_7,
  KC_GAME_PAD_BUTTON_8,
  KC_GAME_PAD_BUTTON_9,
  KC_GAME_PAD_BUTTON_10,
  KC_GAME_PAD_BUTTON_11,
  KC_GAME_PAD_BUTTON_12,
  KC_GAME_PAD_BUTTON_13,
  KC_GAME_PAD_BUTTON_14,
  KC_GAME_PAD_BUTTON_15,
  KC_GAME_PAD_BUTTON_16,
  KC_GAME_PAD_BUTTON_17,
  KC_GAME_PAD_BUTTON_18,
  KC_GAME_PAD_BUTTON_19,
  KC_GAME_PAD_BUTTON_20,
  KC_GAME_PAD_BUTTON_21,
  KC_GAME_PAD_BUTTON_22,
  KC_GAME_PAD_BUTTON_23,
  KC_GAME_PAD_BUTTON_24,
  KC_GAME_PAD_BUTTON_25,
  KC_GAME_PAD_BUTTON_26,
  KC_GAME_PAD_BUTTON_27,
  KC_GAME_PAD_BUTTON_28,
  KC_GAME_PAD_BUTTON_29,
  KC_GAME_PAD_BUTTON_30,
  KC_GAME_PAD_BUTTON_31,


  SAFE_RANGE = 0x5DA0,
  KC_DESK_TOP,
  KC_PROFILE0,
  KC_PROFILE1,
  KC_PROFILE2,
  KC_PROFILE3,
  KC_PROFILE4,

    
};



// Range Helpers
#define IS_QK_BASIC(code) ((code) >= QK_BASIC && (code) <= QK_BASIC_MAX)
#define IS_QK_MODS(code) ((code) >= QK_MODS && (code) <= QK_MODS_MAX)
#define IS_QK_MOD_TAP(code) ((code) >= QK_MOD_TAP && (code) <= QK_MOD_TAP_MAX)
#define IS_QK_LAYER_TAP(code) ((code) >= QK_LAYER_TAP && (code) <= QK_LAYER_TAP_MAX)
#define IS_QK_LAYER_MOD(code) ((code) >= QK_LAYER_MOD && (code) <= QK_LAYER_MOD_MAX)
#define IS_QK_TO(code) ((code) >= QK_TO && (code) <= QK_TO_MAX)
#define IS_QK_MOMENTARY(code) ((code) >= QK_MOMENTARY && (code) <= QK_MOMENTARY_MAX)
#define IS_QK_DEF_LAYER(code) ((code) >= QK_DEF_LAYER && (code) <= QK_DEF_LAYER_MAX)
#define IS_QK_TOGGLE_LAYER(code) ((code) >= QK_TOGGLE_LAYER && (code) <= QK_TOGGLE_LAYER_MAX)
#define IS_QK_ONE_SHOT_LAYER(code) ((code) >= QK_ONE_SHOT_LAYER && (code) <= QK_ONE_SHOT_LAYER_MAX)
#define IS_QK_ONE_SHOT_MOD(code) ((code) >= QK_ONE_SHOT_MOD && (code) <= QK_ONE_SHOT_MOD_MAX)
#define IS_QK_LAYER_TAP_TOGGLE(code) ((code) >= QK_LAYER_TAP_TOGGLE && (code) <= QK_LAYER_TAP_TOGGLE_MAX)
#define IS_QK_SWAP_HANDS(code) ((code) >= QK_SWAP_HANDS && (code) <= QK_SWAP_HANDS_MAX)
#define IS_QK_TAP_DANCE(code) ((code) >= QK_TAP_DANCE && (code) <= QK_TAP_DANCE_MAX)
#define IS_QK_MAGIC(code) ((code) >= QK_MAGIC && (code) <= QK_MAGIC_MAX)
#define IS_QK_MIDI(code) ((code) >= QK_MIDI && (code) <= QK_MIDI_MAX)
#define IS_QK_SEQUENCER(code) ((code) >= QK_SEQUENCER && (code) <= QK_SEQUENCER_MAX)
#define IS_KC_GAME_PAD(code) ((code) >= KC_GAME_PAD && (code) <= KC_GAME_PAD_MAX)
#define IS_QK_PROGRAMMABLE_BUTTON(code) ((code) >= QK_PROGRAMMABLE_BUTTON && (code) <= QK_PROGRAMMABLE_BUTTON_MAX)
#define IS_QK_AUDIO(code) ((code) >= QK_AUDIO && (code) <= QK_AUDIO_MAX)
#define IS_QK_STENO(code) ((code) >= QK_STENO && (code) <= QK_STENO_MAX)
#define IS_QK_MACRO(code) ((code) >= QK_MACRO && (code) <= QK_MACRO_MAX)
#define IS_QK_LIGHTING(code) ((code) >= QK_LIGHTING && (code) <= QK_LIGHTING_MAX)
#define IS_QK_QUANTUM(code) ((code) >= QK_QUANTUM && (code) <= QK_QUANTUM_MAX)
#define IS_QK_KB(code) ((code) >= QK_KB && (code) <= QK_KB_MAX)
#define IS_QK_USER(code) ((code) >= QK_USER && (code) <= QK_USER_MAX)
#define IS_QK_UNICODEMAP(code) ((code) >= QK_UNICODEMAP && (code) <= QK_UNICODEMAP_MAX)
#define IS_QK_UNICODE(code) ((code) >= QK_UNICODE && (code) <= QK_UNICODE_MAX)
#define IS_QK_UNICODEMAP_PAIR(code) ((code) >= QK_UNICODEMAP_PAIR && (code) <= QK_UNICODEMAP_PAIR_MAX)

// Group Helpers
#define IS_INTERNAL_KEYCODE(code) ((code) >= KC_NO && (code) <= KC_TRANSPARENT)
#define IS_BASIC_KEYCODE(code) ((code) >= KC_A && (code) <= KC_EXSEL)
#define IS_SYSTEM_KEYCODE(code) ((code) >= KC_SYSTEM_POWER && (code) <= KC_SYSTEM_WAKE)
#define IS_CONSUMER_KEYCODE(code) ((code) >= KC_AUDIO_MUTE && (code) <= KC_LAUNCHPAD)
#define IS_MOUSE_KEYCODE(code) ((code) >= KC_MS_UP && (code) <= KC_MS_ACCEL2)
#define IS_MODIFIER_KEYCODE(code) ((code) >= KC_LEFT_CTRL && (code) <= KC_RIGHT_GUI)
#define IS_SWAP_HANDS_KEYCODE(code) ((code) >= QK_SWAP_HANDS_TOGGLE && (code) <= QK_SWAP_HANDS_ONE_SHOT)
#define IS_MAGIC_KEYCODE(code) ((code) >= QK_MAGIC_SWAP_CONTROL_CAPS_LOCK && (code) <= QK_MAGIC_TOGGLE_ESCAPE_CAPS_LOCK)
#define IS_MIDI_KEYCODE(code) ((code) >= QK_MIDI_ON && (code) <= QK_MIDI_PITCH_BEND_UP)
#define IS_SEQUENCER_KEYCODE(code) ((code) >= QK_SEQUENCER_ON && (code) <= QK_SEQUENCER_STEPS_CLEAR)
#define IS_GAME_PAD_KEYCODE(code) ((code) >= KC_GAME_PAD_BUTTON_0 && (code) <= KC_GAME_PAD_BUTTON_31)
#define IS_PROGRAMMABLE_BUTTON_KEYCODE(code) ((code) >= QK_PROGRAMMABLE_DPAD_UP && (code) <= QK_PROGRAMMABLE_BUTTON_32)
#define IS_AUDIO_KEYCODE(code) ((code) >= QK_AUDIO_ON && (code) <= QK_AUDIO_VOICE_PREVIOUS)
#define IS_STENO_KEYCODE(code) ((code) >= QK_STENO_BOLT && (code) <= QK_STENO_COMB_MAX)
#define IS_MACRO_KEYCODE(code) ((code) >= QK_MACRO_0 && (code) <= QK_MACRO_31)
#define IS_BACKLIGHT_KEYCODE(code) ((code) >= QK_BACKLIGHT_ON && (code) <= QK_BACKLIGHT_TOGGLE_BREATHING)
#define IS_RGB_KEYCODE(code) ((code) >= RGB_TOG && (code) <= RGB_MODE_TWINKLE)
#define IS_QUANTUM_KEYCODE(code) ((code) >= QK_BOOTLOADER && (code) <= QK_ALT_REPEAT_KEY)
#define IS_KB_KEYCODE(code) ((code) >= QK_KB_0 && (code) <= QK_KB_31)
#define IS_USER_KEYCODE(code) ((code) >= QK_USER_0 && (code) <= QK_USER_31)

#endif /* KEYCODE_H */
