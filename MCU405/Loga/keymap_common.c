/*
Copyright 2012 Jun Wako <wakojun@gmail.com>

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

#include "keymap_common.h"
#include "keymap_in_eeprom.h"
#include "action_layer.h"



#ifdef KEYMAP_IN_EEPROM_ENABLE

//Origin Keymap
//const uint16_t keymaps[KEYMAPS_COUNT][MATRIX_ROWS][MATRIX_COLS] __attribute__ ((section (".keymap.keymaps"))) = {
//
//	KEYMAP(
//			ESC,   F1,   F2,   F3,   F4,   F5,   F6,   F7,   F8,   F9,   F10,   F11,   F12,   PSCR,\
//			GRV,    1,    2,    3,    4,    5,    6,    7,    8,    9,     0,  MINS,   EQL,   BSPC,    DEL,\
//			TAB,    Q,    W,    E,    R,    T,    Y,    U,    I,    O,     P,  LBRC,  RBRC,   BSLS,    PGUP,\
//			CAPS,   A,    S,    D,    F,    G,    H,    J,    K,    L,  SCLN,  QUOT,   ENT,     PGDN,\
//			LSFT,   Z,    X,    C,    V,    B,    N,    M, COMM,  DOT,  SLSH,   RSFT,   UP,\
//			LCTL, LGUI,LALT,  SPC, RALT, MO1, RCTL, LEFT, DOWN,  RGHT),
//
//	KEYMAP(
//			BTLD,   TRNS,  TRNS,  TRNS,  TRNS,  TRNS,  TRNS,  TRNS,  TRNS, MEDIA_STOP,  MEDIA_PREV_TRACK,  MEDIA_PLAY_PAUSE,  MEDIA_NEXT_TRACK,  TRNS,\
//			TRNS,  TRNS,  TRNS,  TRNS,  TRNS,  TRNS,  TRNS,  TRNS,  TRNS,  TRNS,  TRNS, RGB_SPD, RGB_SPI, TRNS,  TRNS,\
//			TRNS,  TRNS,  TRNS,  TRNS,  TRNS,  TRNS,  TRNS,  TRNS,  TRNS,  TRNS,  TRNS,  TRNS,  TRNS,  TRNS,  TRNS,\
//			TRNS,  TRNS,  TRNS,  TRNS,  TRNS,  TRNS,  TRNS,  TRNS,  TRNS,  TRNS,  TRNS,  TRNS,  TRNS,  TRNS, \
//			TRNS,  TRNS,  TRNS,  TRNS,  TRNS,  TRNS,  TRNS,  TRNS,  TRNS,  TRNS,  TRNS,  TRNS,  RGB_VAI,\
//			TRNS,  TRNS,  TRNS,  TRNS,  RAPID_TOG,  TRNS,  MO2,  TRNS,  RGB_VAD, TRNS),
//
//
//		KEYMAP(
//			KC_SYRST,   TRNS,  TRNS,  TRNS,  TRNS,  TRNS,  TRNS,  TRNS,  TRNS, MEDIA_STOP,  MEDIA_PREV_TRACK,  MEDIA_PLAY_PAUSE,  MEDIA_NEXT_TRACK,  TRNS,\
//			TRNS,  TRNS,  TRNS,  TRNS,  TRNS,  TRNS,  TRNS,  TRNS,  TRNS,  TRNS,  TRNS, RGB_SPD, RGB_SPI, TRNS,  TRNS,\
//			TRNS,  TRNS,  GAME_PAD_DPAD_UP,TRNS,TRNS,GAME_PAD_ANALOG_Y_PLUSE,TRNS,TRNS,GAME_PAD_ANALOG_Z_PLUSE,  TRNS,  TRNS,  TRNS,  TRNS,   GAME_PAD_BUTTON_8,  TRNS,\
//			TRNS,  GAME_PAD_DPAD_LEFT,GAME_PAD_DPAD_DOWN,GAME_PAD_DPAD_RIGHT,GAME_PAD_ANALOG_X_MINUS,GAME_PAD_ANALOG_Y_MINUS,GAME_PAD_ANALOG_X_PLUSE,GAME_PAD_ANALOG_RZ_MINUS,GAME_PAD_ANALOG_Z_MINUS,GAME_PAD_ANALOG_RZ_PLUSE,  TRNS,  TRNS, GAME_PAD_BUTTON_9,  TRNS,\
//			TRNS,  TRNS,  TRNS,  TRNS,  TRNS,  TRNS,  TRNS,  TRNS,  TRNS,  TRNS,  TRNS,  TRNS,  GAME_PAD_BUTTON_3,\
//			TRNS,  TRNS,  TRNS,  TRNS,  TRNS,  TRNS,  NO,    GAME_PAD_BUTTON_0,  GAME_PAD_BUTTON_1,  GAME_PAD_BUTTON_2),
//
//};

const uint16_t keymaps[KEYMAPS_COUNT][MATRIX_ROWS][MATRIX_COLS] __attribute__ ((section (".keymap.keymaps"))) = {//
//Profile 1
	KEYMAP(
			ESC,   F1,   F2,   F3,   F4,   F5,   F6,   F7,   F8,   F9,   F10,   F11,   F12,   PSCR,\
			GRV,    1,    2,    3,    4,    5,    6,    7,    8,    9,     0,  MINS,   EQL,   BSPC,    DEL,\
			TAB,    Q,    W,    E,    R,    T,    Y,    U,    I,    O,     P,  LBRC,  RBRC,   BSLS,    PGUP,\
			CAPS,   A,    S,    D,    F,    G,    H,    J,    K,    L,  SCLN,  QUOT,   ENT,     PGDN,\
			LSFT,   Z,    X,    C,    V,    B,    N,    M, COMM,  DOT,  SLSH,   RSFT,   UP,\
			LCTL, LGUI,LALT,  SPC, RALT, MO1, RCTL, LEFT, DOWN,  RGHT),// M1  DOWN RGHT FN15 FN16

	KEYMAP(
			BTLD,   MSTP,  VOLD,  VOLU,  MUTE,  MSTP,  MEDIA_PREV_TRACK,  MEDIA_PLAY_PAUSE,  MEDIA_NEXT_TRACK, MAIL,  WWW_HOME,  CALCULATOR,  WWW_SEARCH,  TRNS,\
			TRNS,  PROFILE0,  PROFILE1,  PROFILE2,  PROFILE3,  TRNS,  TRNS,  TRNS,  TRNS,  TRNS,  TRNS, RGB_SPD, RGB_SPI, TRNS,  SLEP,\
			TRNS,  0,  TRNS,  TRNS,  TRNS,  TRNS,  TRNS,  TRNS,  TRNS,  TRNS,  TRNS,  PSAT,  MSAT,  RAPID_TOG,  TRNS,\
			TRNS,  WINDOW, MAC,  TRNS,  TRNS,  TRNS,  TRNS,  TRNS,  TRNS,  TRNS,  TRNS,  TRNS,  TRNS,  TRNS, \
			TRNS,  TRNS,  TRNS,  TRNS,  TRNS,  TRNS,  TRNS,  TRNS,  TRNS,  TRNS,  TRNS,  SOCD_TOGG,  RGB_VAI,\
			TRNS,  WIN_LOCK,  TRNS,  TRNS,  TRNS,  TRNS,  MO2,  PHUE,  RGB_VAD, MHUE),//RGB_VAD

	KEYMAP(
			EECLR,   TRNS,  TRNS,  TRNS,  TRNS,  TRNS,  TRNS,  TRNS,  TRNS, MEDIA_STOP,  MEDIA_PREV_TRACK,  MEDIA_PLAY_PAUSE,  MEDIA_NEXT_TRACK,  TRNS,\
			TRNS,   LM1,   LM2,   LM3,   LM4,   LM5,   LM6,   LM7,   LM8,   LM9,  TRNS, RGB_SPD, RGB_SPI, TRNS,  TRNS,\
			TRNS,  TRNS,  TRNS,  TRNS,  TRNS,  TRNS,  TRNS,  TRNS,  TRNS,  TRNS,  TRNS,  TRNS,  TRNS,  TRNS,  TRNS,\
			TRNS,  TRNS,  TRNS,  TRNS,  TRNS,  TRNS,  TRNS,  TRNS,  TRNS,  TRNS,  TRNS,  TRNS,  TRNS,  TRNS, \
			TRNS,  TRNS,  TRNS,  TRNS,  TRNS,  TRNS,  TRNS,  TRNS,  TRNS,  TRNS,  TRNS,  TRNS,  RGB_VAI,\
			TRNS,  TRNS,  TRNS,  TRNS,  TRNS,  TRNS,  MO2,  TRNS,  RGB_VAD, TRNS),

//	KEYMAP(
//			KC_SYRST,   TRNS,  TRNS,  TRNS,  TRNS,  TRNS,  TRNS,  TRNS,  TRNS, MEDIA_STOP,  MEDIA_PREV_TRACK,  MEDIA_PLAY_PAUSE,  MEDIA_NEXT_TRACK,  TRNS,\
//			TRNS,  TRNS,  TRNS,  TRNS,  TRNS,  TRNS,  TRNS,  TRNS,  TRNS,  TRNS,  TRNS, RGB_SPD, RGB_SPI, TRNS,  TRNS,\
//			TRNS,  TRNS,  GAME_PAD_DPAD_UP,TRNS,TRNS,GAME_PAD_ANALOG_Y_PLUSE,TRNS,TRNS,GAME_PAD_ANALOG_Z_PLUSE,  TRNS,  TRNS,  TRNS,  TRNS,   GAME_PAD_BUTTON_8,  TRNS,\
//			TRNS,  GAME_PAD_DPAD_LEFT,GAME_PAD_DPAD_DOWN,GAME_PAD_DPAD_RIGHT,GAME_PAD_ANALOG_X_MINUS,GAME_PAD_ANALOG_Y_MINUS,GAME_PAD_ANALOG_X_PLUSE,GAME_PAD_ANALOG_RZ_MINUS,GAME_PAD_ANALOG_Z_MINUS,GAME_PAD_ANALOG_RZ_PLUSE,  TRNS,  TRNS, GAME_PAD_BUTTON_9,  TRNS,\
//			TRNS,  TRNS,  TRNS,  TRNS,  TRNS,  TRNS,  TRNS,  TRNS,  TRNS,  TRNS,  TRNS,  TRNS,  GAME_PAD_BUTTON_3,\
//			TRNS,  TRNS,  TRNS,  TRNS,  TRNS,  TRNS,  NO,    GAME_PAD_BUTTON_0,  GAME_PAD_BUTTON_1,  GAME_PAD_BUTTON_2),


//
////Profile 2
	KEYMAP(
			ESC,   F1,   F2,   F3,   F4,   F5,   F6,   F7,   F8,   F9,   F10,   F11,   F12,   PSCR,\
			GRV,    1,    2,    3,    4,    5,    6,    7,    8,    9,     0,  MINS,   EQL,   BSPC,    DEL,\
			TAB,    Q,    W,    E,    R,    T,    Y,    U,    I,    O,     P,  LBRC,  RBRC,   BSLS,    PGUP,\
			CAPS,   A,    S,    D,    F,    G,    H,    J,    K,    L,  SCLN,  QUOT,   ENT,     PGDN,\
			LSFT,   Z,    X,    C,    V,    B,    N,    M, COMM,  DOT,  SLSH,   RSFT,   UP,\
			LCTL, LGUI,LALT,  SPC, RALT, MO4, RCTL, LEFT, DOWN,  RGHT),

	KEYMAP(
			EECLR,   BRID,  BRIU,  WWW_HOME,  DESK_TOP,  RGB_VAD,  RGB_VAI,  MEDIA_PREV_TRACK,  MEDIA_NEXT_TRACK, MEDIA_PLAY_PAUSE,  MUTE,  VOLD,  VOLU,  RAPID_TOG,\
			TRNS,  PROFILE0,  PROFILE1,  PROFILE2,  PROFILE3,  TRNS,  TRNS,  TRNS,  TRNS,  TRNS,  TRNS, RGB_SPD, RGB_SPI, TRNS,  TRNS,\
			TRNS,  1,  TRNS,  TRNS,  TRNS,  TRNS,  TRNS,  TRNS,  TRNS,  TRNS,  TRNS,  TRNS,  TRNS,  TRNS,  TRNS,\
			TRNS,  TRNS,  TRNS,  TRNS,  TRNS,  TRNS,  TRNS,  TRNS,  TRNS,  TRNS,  TRNS,  TRNS,  TRNS,  TRNS, \
			TRNS,  TRNS,  TRNS,  TRNS,  TRNS,  TRNS,  TRNS,  TRNS,  TRNS,  TRNS,  TRNS,  TRNS,  RGB_VAI,\
			TRNS,  TRNS,  TRNS,  TRNS,  TRNS,  TRNS,  MO2,  TRNS,  RGB_VAD, TRNS),

	KEYMAP(
			BTLD,   TRNS,  TRNS,  TRNS,  TRNS,  TRNS,  TRNS,  TRNS,  TRNS, MEDIA_STOP,  MEDIA_PREV_TRACK,  MEDIA_PLAY_PAUSE,  MEDIA_NEXT_TRACK,  TRNS,\
			TRNS,  TRNS,  TRNS,  TRNS,  TRNS,  TRNS,  TRNS,  TRNS,  TRNS,  TRNS,  TRNS, RGB_SPD, RGB_SPI, TRNS,  TRNS,\
			TRNS,  TRNS,  TRNS,  TRNS,  TRNS,  TRNS,  TRNS,  TRNS,  TRNS,  TRNS,  TRNS,  TRNS,  TRNS,  TRNS,  TRNS,\
			TRNS,  TRNS,  TRNS,  TRNS,  TRNS,  TRNS,  TRNS,  TRNS,  TRNS,  TRNS,  TRNS,  TRNS,  TRNS,  TRNS, \
			TRNS,  TRNS,  TRNS,  TRNS,  TRNS,  TRNS,  TRNS,  TRNS,  TRNS,  TRNS,  TRNS,  TRNS,  RGB_VAI,\
			TRNS,  TRNS,  TRNS,  TRNS,  TRNS,  TRNS,  MO2,  TRNS,  RGB_VAD, TRNS),

//	KEYMAP(
//			KC_SYRST,   TRNS,  TRNS,  TRNS,  TRNS,  TRNS,  TRNS,  TRNS,  TRNS, MEDIA_STOP,  MEDIA_PREV_TRACK,  MEDIA_PLAY_PAUSE,  MEDIA_NEXT_TRACK,  TRNS,\
//			TRNS,  TRNS,  TRNS,  TRNS,  TRNS,  TRNS,  TRNS,  TRNS,  TRNS,  TRNS,  TRNS, RGB_SPD, RGB_SPI, TRNS,  TRNS,\
//			TRNS,  TRNS,  GAME_PAD_DPAD_UP,TRNS,TRNS,GAME_PAD_ANALOG_Y_PLUSE,TRNS,TRNS,GAME_PAD_ANALOG_Z_PLUSE,  TRNS,  TRNS,  TRNS,  TRNS,   GAME_PAD_BUTTON_8,  TRNS,\
//			TRNS,  GAME_PAD_DPAD_LEFT,GAME_PAD_DPAD_DOWN,GAME_PAD_DPAD_RIGHT,GAME_PAD_ANALOG_X_MINUS,GAME_PAD_ANALOG_Y_MINUS,GAME_PAD_ANALOG_X_PLUSE,GAME_PAD_ANALOG_RZ_MINUS,GAME_PAD_ANALOG_Z_MINUS,GAME_PAD_ANALOG_RZ_PLUSE,  TRNS,  TRNS, GAME_PAD_BUTTON_9,  TRNS,\
//			TRNS,  TRNS,  TRNS,  TRNS,  TRNS,  TRNS,  TRNS,  TRNS,  TRNS,  TRNS,  TRNS,  TRNS,  GAME_PAD_BUTTON_3,\
//			TRNS,  TRNS,  TRNS,  TRNS,  TRNS,  TRNS,  NO,    GAME_PAD_BUTTON_0,  GAME_PAD_BUTTON_1,  GAME_PAD_BUTTON_2),



//Profile 3
	KEYMAP(
			ESC,   F1,   F2,   F3,   F4,   F5,   F6,   F7,   F8,   F9,   F10,   F11,   F12,   PSCR,\
			GRV,    1,    2,    3,    4,    5,    6,    7,    8,    9,     0,  MINS,   EQL,   BSPC,    DEL,\
			TAB,    Q,    W,    E,    R,    T,    Y,    U,    I,    O,     P,  LBRC,  RBRC,   BSLS,    PGUP,\
			CAPS,   A,    S,    D,    F,    G,    H,    J,    K,    L,  SCLN,  QUOT,   ENT,     PGDN,\
			LSFT,   Z,    X,    C,    V,    B,    N,    M, COMM,  DOT,  SLSH,   RSFT,   UP,\
			LCTL, LGUI,LALT,  SPC, RALT, MO7, RCTL, LEFT, DOWN,  RGHT),

	KEYMAP(
			BTLD,   BRID,  BRIU,  WWW_HOME,  DESK_TOP,  RGB_VAD,  RGB_VAI,  MEDIA_PREV_TRACK,  MEDIA_NEXT_TRACK, MEDIA_PLAY_PAUSE,  MUTE,  VOLD,  VOLU,  RAPID_TOG,\
			TRNS,  PROFILE0,  PROFILE1,  PROFILE2,  PROFILE3,  TRNS,  TRNS,  TRNS,  TRNS,  TRNS,  TRNS, RGB_SPD, RGB_SPI, TRNS,  TRNS,\
			TRNS,  2,  TRNS,  TRNS,  TRNS,  TRNS,  TRNS,  TRNS,  TRNS,  TRNS,  TRNS,  TRNS,  TRNS,  TRNS,  TRNS,\
			TRNS,  TRNS,  TRNS,  TRNS,  TRNS,  TRNS,  TRNS,  TRNS,  TRNS,  TRNS,  TRNS,  TRNS,  TRNS,  TRNS, \
			TRNS,  TRNS,  TRNS,  TRNS,  TRNS,  TRNS,  TRNS,  TRNS,  TRNS,  TRNS,  TRNS,  TRNS,  RGB_VAI,\
			TRNS,  TRNS,  TRNS,  TRNS,  TRNS,  TRNS,  MO2,  TRNS,  RGB_VAD, TRNS),


	KEYMAP(
			BTLD,   TRNS,  TRNS,  TRNS,  TRNS,  TRNS,  TRNS,  TRNS,  TRNS, MEDIA_STOP,  MEDIA_PREV_TRACK,  MEDIA_PLAY_PAUSE,  MEDIA_NEXT_TRACK,  TRNS,\
			TRNS,  TRNS,  TRNS,  TRNS,  TRNS,  TRNS,  TRNS,  TRNS,  TRNS,  TRNS,  TRNS, RGB_SPD, RGB_SPI, TRNS,  TRNS,\
			TRNS,  TRNS,  TRNS,  TRNS,  TRNS,  TRNS,  TRNS,  TRNS,  TRNS,  TRNS,  TRNS,  TRNS,  TRNS,  TRNS,  TRNS,\
			TRNS,  TRNS,  TRNS,  TRNS,  TRNS,  TRNS,  TRNS,  TRNS,  TRNS,  TRNS,  TRNS,  TRNS,  TRNS,  TRNS, \
			TRNS,  TRNS,  TRNS,  TRNS,  TRNS,  TRNS,  TRNS,  TRNS,  TRNS,  TRNS,  TRNS,  TRNS,  RGB_VAI,\
			TRNS,  TRNS,  TRNS,  TRNS,  TRNS,  TRNS,  MO2,  TRNS,  RGB_VAD, TRNS),

//Profile 4
	KEYMAP(
			ESC,   F1,   F2,   F3,   F4,   F5,   F6,   F7,   F8,   F9,   F10,   F11,   F12,   PSCR,\
			GRV,    1,    2,    3,    4,    5,    6,    7,    8,    9,     0,  MINS,   EQL,   BSPC,    DEL,\
			TAB,    Q,    W,    E,    R,    T,    Y,    U,    I,    O,     P,  LBRC,  RBRC,   BSLS,    PGUP,\
			CAPS,   A,    S,    D,    F,    G,    H,    J,    K,    L,  SCLN,  QUOT,   ENT,     PGDN,\
			LSFT,   Z,    X,    C,    V,    B,    N,    M, COMM,  DOT,  SLSH,   RSFT,   UP,\
			LCTL, LGUI,LALT,  SPC, RALT, MO10, RCTL, LEFT, DOWN,  RGHT),

	KEYMAP(
			BTLD,   BRID,  BRIU,  WWW_HOME,  DESK_TOP,  RGB_VAD,  RGB_VAI,  MEDIA_PREV_TRACK,  MEDIA_NEXT_TRACK, MEDIA_PLAY_PAUSE,  MUTE,  VOLD,  VOLU,  RAPID_TOG,\
			TRNS,  PROFILE0,  PROFILE1,  PROFILE2,  PROFILE3,  TRNS,  TRNS,  TRNS,  TRNS,  TRNS,  TRNS, RGB_SPD, RGB_SPI, TRNS,  TRNS,\
			TRNS,  3,  TRNS,  TRNS,  TRNS,  TRNS,  TRNS,  TRNS,  TRNS,  TRNS,  TRNS,  TRNS,  TRNS,  TRNS,  TRNS,\
			TRNS,  TRNS,  TRNS,  TRNS,  TRNS,  TRNS,  TRNS,  TRNS,  TRNS,  TRNS,  TRNS,  TRNS,  TRNS,  TRNS, \
			TRNS,  TRNS,  TRNS,  TRNS,  TRNS,  TRNS,  TRNS,  TRNS,  TRNS,  TRNS,  TRNS,  TRNS,  RGB_VAI,\
			TRNS,  TRNS,  TRNS,  TRNS,  TRNS,  TRNS,  MO2,  TRNS,  RGB_VAD, TRNS),


	KEYMAP(
			BTLD,   TRNS,  TRNS,  TRNS,  TRNS,  TRNS,  TRNS,  TRNS,  TRNS, MEDIA_STOP,  MEDIA_PREV_TRACK,  MEDIA_PLAY_PAUSE,  MEDIA_NEXT_TRACK,  TRNS,\
			TRNS,  TRNS,  TRNS,  TRNS,  TRNS,  TRNS,  TRNS,  TRNS,  TRNS,  TRNS,  TRNS, RGB_SPD, RGB_SPI, TRNS,  TRNS,\
			TRNS,  TRNS,  TRNS,  TRNS,  TRNS,  TRNS,  TRNS,  TRNS,  TRNS,  TRNS,  TRNS,  TRNS,  TRNS,  TRNS,  TRNS,\
			TRNS,  TRNS,  TRNS,  TRNS,  TRNS,  TRNS,  TRNS,  TRNS,  TRNS,  TRNS,  TRNS,  TRNS,  TRNS,  TRNS, \
			TRNS,  TRNS,  TRNS,  TRNS,  TRNS,  TRNS,  TRNS,  TRNS,  TRNS,  TRNS,  TRNS,  TRNS,  RGB_VAI,\
			TRNS,  TRNS,  TRNS,  TRNS,  TRNS,  TRNS,  MO2,  TRNS,  RGB_VAD, TRNS),

};

#endif



#ifdef KEYMAP_SECTION_ENABLE
const uint16_t fn_actions[FN_ACTIONS_COUNT] __attribute__ ((section (".keymap.fn_actions"))) = {
#else
const uint16_t fn_actions[] PROGMEM = {
		[0] = ACTION_LAYER_MOMENTARY(0),
		[1] = ACTION_LAYER_MOMENTARY(1),
		[2] = ACTION_LAYER_MOMENTARY(2),
		[3] = ACTION_LAYER_MOMENTARY(3),
		[4] = ACTION_LAYER_MOMENTARY(4),
		[5] = ACTION_LAYER_MOMENTARY(5),
		[6] = ACTION_LAYER_MOMENTARY(6),
		[7] = ACTION_LAYER_MOMENTARY(7),
		[8] = ACTION_LAYER_MOMENTARY(8),
		[9] = ACTION_LAYER_MOMENTARY(9),
		[10] = ACTION_LAYER_MOMENTARY(10),
		[11] = ACTION_LAYER_MOMENTARY(11),
		[12] = ACTION_LAYER_MOMENTARY(12),
		[13] = ACTION_LAYER_MOMENTARY(13),
		[14] = ACTION_LAYER_MOMENTARY(14),
		[15] = ACTION_MODS_TAP_KEY(MOD_LCTL, KC_A), // Fn0：按住 Ctrl / 點按 A
    	[16] = ACTION_LAYER_TAP_KEY(1, KC_SPC),     // Fn1：按住進 Layer1 / 點按空白
#endif
};

// const action_t PROGMEM fn_actions[] = { // lyon for tap mod
//     [0] = ACTION_MODS_TAP_KEY(MOD_LCTL, KC_A), // Fn0：按住 Ctrl / 點按 A
//     [1] = ACTION_LAYER_TAP_KEY(1, KC_SPC),     // Fn1：按住進 Layer1 / 點按空白
// };

#ifdef KEYMAP_IN_EEPROM_ENABLE
uint16_t keys_count(void) {
	return sizeof(keymaps) / sizeof(keymaps[0]) * MATRIX_ROWS * MATRIX_COLS;
}

uint16_t fn_actions_count(void) {
    return sizeof(fn_actions) / sizeof(fn_actions[0]);
}
#endif

uint16_t keymap_key_to_keycode(uint8_t layer, keypos_t key)
{
#ifdef KEYMAP_IN_EEPROM_ENABLE
    //return eeconfig_read_keymap_key(layer, key.row, key.col);
    return eeconfig_read_keymapBuffer_key(layer, key.row, key.col);
#endif
}

/* translates Fn keycode to action */
action_t keymap_fn_to_action(uint16_t keycode)
{
    //printf ("Action Fn Key:%d find:%X\r\n",keycode-KC_FN0,eeconfig_read_keymap_fn_action(FN_INDEX(keycode)));
    return (action_t) {
#ifdef KEYMAP_IN_EEPROM_ENABLE
        .code = pgm_read_word(&fn_actions[FN_INDEX(keycode)])
#else
        .code = eeconfig_read_keymap_fn_action(FN_INDEX(keycode))
            
#endif
    };
}

#ifdef KEYMAP_IN_EEPROM_ENABLE
const uint16_t* keymaps_pointer(void) {
    return (const uint16_t*)keymaps;
}

const uint16_t* fn_actions_pointer(void) {
    return fn_actions;
}
#endif
