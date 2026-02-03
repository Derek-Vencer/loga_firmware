
#pragma once

#include "action.h"
#include "action_layer.h"

#ifndef DYNAMIC_MACRO_COUNT
#define DYNAMIC_MACRO_COUNT 2
#endif

#ifndef DYNAMIC_MACRO_SIZE
/* May be overridden with a custom value. Be aware that the effective
 * macro length is half of this value: each keypress is recorded twice
 * because of the down-event and up-event. This is not a bug, it's the
 * intended behavior.
 *
 * Usually it should be fine to set the macro size to at least 256 but
 * there have been reports of it being too much in some users' cases,
 * so 128 is considered a safe default.
 */
#define DYNAMIC_MACRO_SIZE 64
#endif

#ifndef DYNAMIC_MACRO_EEPROM_STORAGE
#define DYNAMIC_MACRO_EEPROM_STORAGE
#endif

/* DYNAMIC_MACRO_RANGE must be set as the last element of user's
 * "planck_keycodes" enum prior to including this header. This allows
 * us to 'extend' it.
 */
enum dynamic_macro_keycodes {
  DYN_MACRO_PROG = KC_MACRO_0,

  /* Requirement: DYN_MACRO_KEYs are in sequence in the enum. */
  DYN_MACRO_KEY1,
  DYN_MACRO_KEY2,
  DYN_MACRO_KEY3,
  DYN_MACRO_KEY4,
  DYN_MACRO_KEY5,
  DYN_MACRO_KEY6,
  DYN_MACRO_KEY7,
  DYN_MACRO_KEY8,
  DYN_MACRO_KEY9,
  DYN_MACRO_KEY10,
  DYN_MACRO_KEY11,
  DYN_MACRO_KEY12
};

enum dynamic_macro_recording_state { STATE_NOT_RECORDING, STATE_RECORD_KEY_PRESSED, STATE_CURRENTLY_RECORDING };

typedef struct {
  keyrecord_t events[DYNAMIC_MACRO_SIZE];
  uint8_t     length;
  uint16_t    checksum;
} dynamic_macro_t;

void     dynamic_macro_init(void);
void     dynamic_macro_led_blink(void);
void     dynamic_macro_record_start(uint8_t macro_id);
void     dynamic_macro_play(uint8_t macro_id,bool delayEnable);
void     dynamic_macro_record_key(uint8_t macro_id, keyrecord_t* record);
void     dynamic_macro_record_end(uint8_t macro_id,bool mode);
bool     process_record_dynamic_macro(keyrecord_t record);

#define DYNAMIC_MACRO_CRC_LENGTH (sizeof(dynamic_macro_t) - sizeof(uint16_t))

#ifdef DYNAMIC_MACRO_EEPROM_STORAGE
#define DYNAMIC_MACRO_EEPROM_MAGIC (uint16_t)0xDEAD

uint16_t dynamic_macro_calc_crc(dynamic_macro_t* macro);
void dynamic_macro_load_eeprom_all(void);
void dynamic_macro_load_eeprom(uint8_t macro_id);
void dynamic_macro_save_eeprom(uint8_t macro_id, bool mode);
bool dynamic_macro_header_correct(void);
void dynamic_macro_reset_eeprom(void);
#endif

