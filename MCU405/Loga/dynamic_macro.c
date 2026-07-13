//#include QMK_KEYBOARD_H
#include <string.h>
#include "dynamic_macro.h"
#include "wait.h"
#include "eeconfig.h"
#include "debug.h"
#include "timer.h"



dynamic_macro_t dynamic_macros[DYNAMIC_MACRO_COUNT];//
uint32_t macro_timeStamp = 0;
uint8_t macro_layer = 0;

void dynamic_macro_init(void) {
  /* zero out macro blocks  */
  memset(&dynamic_macros, 0, DYNAMIC_MACRO_COUNT * sizeof(dynamic_macro_t));
}

/* Blink the LEDs to notify the user about some event. */
void dynamic_macro_led_blink(void) {
#ifdef BACKLIGHT_ENABLE
  backlight_toggle();
  chThdSleepMilliseconds(100);
  backlight_toggle();
#else
  led_set(host_keyboard_leds() ^ 0xFF);
  chThdSleepMilliseconds(100);
  led_set(host_keyboard_leds());
#endif
}

/**
 * Start recording of the dynamic macro.
 *
 * @param macro_id[in]     The id of macro to be recorded
 */
void dynamic_macro_record_start(uint8_t macro_id) {
  //uprintf("dynamic macro recording: started for slot %d\r\n", macro_id);

  dynamic_macro_led_blink();

  clear_keyboard();
  layer_clear();

  dynamic_macros[macro_id].length = 0;
}

/**
 * Play the dynamic macro.
 *
 * @param macro_id[in]     The id of macro to be played
 */
void dynamic_macro_play(uint8_t macro_id, bool delayEnable) {
  //uprintf("dynamic macro: slot %d playback, length %d\r\n", macro_id, dynamic_macros[macro_id].length);

  uint32_t saved_layer_state = layer_state;
  // memset (&dynamic_macros[macro_id], 0x00, sizeof(dynamic_macros[macro_id]));
  // dynamic_macro_load_eeprom(macro_id);
  clear_keyboard();
  layer_clear();

 for (uint8_t i = 0; i < dynamic_macros[macro_id].length; ++i) {
   process_record(&dynamic_macros[macro_id].events[i], delayEnable);
 }

  clear_keyboard();
  layer_state = saved_layer_state;
}

/**
 * Record a single key in a dynamic macro.
 *
 * @param macro_id[in] The start of the used macro buffer.
 * @param record[in]     The current keypress.
 */
void dynamic_macro_record_key(uint8_t macro_id, keyrecord_t* record) {
  dynamic_macro_t* macro  = &dynamic_macros[macro_id];
  uint8_t          length = macro->length;
  ////uprintf ("recording record->row:%d col:%d time:%d\r\n", record->event.key.row, record->event.key.col,record->event.time);
  /* If we've just started recording, ignore all the key releases. */
  if (!record->event.pressed && length == 0) {
    //dprintln("dynamic macro: ignoring a leading key-up event");
    return;
  }

  record->event.time = (timer_read() - macro_timeStamp) & 0xffff;
  macro_timeStamp = timer_read();
  ////uprintf ("==>reccorded time:%d\r\n",record->event.time);
  if (length < DYNAMIC_MACRO_SIZE) {
    macro->events[length] = *record;
    macro->length         = ++length;
  } else {
    dynamic_macro_led_blink();
  }

  //uprintf("dynamic macro: slot %d length: %d/%d\r\n", macro_id, length, DYNAMIC_MACRO_SIZE);
}

/**
 * End recording of the dynamic macro. Essentially just update the
 * pointer to the end of the macro.
 */
void dynamic_macro_record_end(uint8_t macro_id,bool mode) {
  dynamic_macro_led_blink();

  dynamic_macro_t* macro  = &dynamic_macros[macro_id];
  uint8_t          length = macro->length;

  keyrecord_t* events_begin   = &(macro->events[0]);
  keyrecord_t* events_pointer = &(macro->events[length - 1]);

  //uprintf("dynamic_macro: macro length before trimming: %d\r\n", macro->length);
  while (events_pointer != events_begin && (events_pointer)->event.pressed) {
    //dprintln("dynamic macro: trimming a trailing key-down event");
    --(macro->length);
    --events_pointer;
  }

#ifdef DYNAMIC_MACRO_EEPROM_STORAGE
 macro->checksum = dynamic_macro_calc_crc(macro);
 //uprintf ("macro crc:%X\r\n",macro->checksum);
 dynamic_macro_save_eeprom(macro_id, mode);
#endif

  //uprintf("dynamic macro: slot %d saved, length: %d\r\n", macro_id, length);
}


bool process_record_dynamic_macro(keyrecord_t record) {
  /* 0 to DYNAMIC_MACRO_COUNT -1 - macro macro_id is being recorded */
  static uint8_t macro_id        = 255;
  static uint8_t recording_state = STATE_NOT_RECORDING;
  uint16_t keycode;
  //uprintf ("macro:%d keyrecord:%d keyevent:%d keyevent.press:%d keypos:%d \r\n",sizeof(dynamic_macro_t),sizeof(keyrecord_t),sizeof(keyevent_t),sizeof(bool),sizeof(keypos_t));
  keycode = keymap_key_to_keycode(biton32(layer_state),record.event.key);
  ////uprintf ("current keycode:%04X\r\n",keycode);
  if (recording_state == STATE_CURRENTLY_RECORDING && (keycode >= KC_MO0 && keycode <=KC_MO5)){
    if (record.event.pressed){
      macro_layer = keycode & 0x000f;
    } else {
      macro_layer = 0;
    }
  }
  if (macro_layer!=0){
 	  keycode = keymap_key_to_keycode(macro_layer,record.event.key);
    ////uprintf ("recording layer:%d keycode:%d",macro_layer,keycode);   
  }
  ////uprintf ("keycode:%04X event:%d \r\n",keycode, record.event.pressed);
  if (STATE_NOT_RECORDING == recording_state) {
    /* Program key pressed to request programming mode */
    if (keycode == DYN_MACRO_PROG && record.event.pressed) {
      dynamic_macro_led_blink();

      recording_state = STATE_RECORD_KEY_PRESSED;
      //uprintf("dynamic macro: programming key pressed, waiting for macro slot selection. %d\r\n", recording_state);

      return false;
    }
    /* Macro key pressed to request macro playback */
    if (keycode >= DYN_MACRO_KEY1 && keycode <= DYN_MACRO_KEY12 && record.event.pressed) {
      dynamic_macro_play(keycode - DYN_MACRO_KEY1 , true);

      return false;
    }

    /* Non-dynamic macro key, process it elsewhere. */
    return true;
  } else if (STATE_RECORD_KEY_PRESSED == recording_state) {
    /* Program key pressed again before a macro selector key, cancel macro recording.
       Blink leds to indicate cancelation. */
    if (keycode == DYN_MACRO_PROG && record.event.pressed) {
      dynamic_macro_led_blink();

      recording_state = STATE_NOT_RECORDING;
      //uprintf("dynamic macro: programming key pressed, programming mode canceled. %d\r\n", recording_state);

      return false;
    } else if (keycode >= DYN_MACRO_KEY1 && keycode <= DYN_MACRO_KEY12 && record.event.pressed) {
      macro_id = keycode - DYN_MACRO_KEY1;

      /* Macro slot selected, enter recording state. */
      recording_state = STATE_CURRENTLY_RECORDING;
      dynamic_macro_record_start(macro_id);

      return false;
    }
    /* Ignore any non-macro key press while in RECORD_KEY_PRESSED state. */
    return false;
  } else if (STATE_CURRENTLY_RECORDING == recording_state) {
    /* Program key pressed to request end of macro recording. */
    if (keycode == DYN_MACRO_PROG && record.event.pressed) {
      dynamic_macro_record_end(macro_id, false);
      recording_state = STATE_NOT_RECORDING;

      return false;
    }
    /* Don't record other macro key presses. */
    else if (keycode >= DYN_MACRO_KEY1 && keycode <= DYN_MACRO_KEY12 && record.event.pressed) {
      //dprintln("dynamic macro: playback key ignored in programming mode.");
      return false;
    }
    /* Non-macro keypress that should be recorded  */
    else {
      if (macro_layer == 0){
        dynamic_macro_record_key(macro_id, &record);
      }

      /* Don't output recorded keypress. */
      return false;
    }
  }

  return true;
}

uint16_t  crc16_update(uint16_t crc, uint8_t a)
{
    crc ^= a;
    for (uint8_t i = 0; i < 8; ++i)
    {
        if (crc & 1)
        crc = (crc >> 1) ^ 0xA001;
        else
        crc = (crc >> 1);
    }

    return crc;
}

uint16_t dynamic_macro_calc_crc(dynamic_macro_t* macro) {
  uint16_t crc  = 0;
  uint8_t* data = (uint8_t*)macro;

  for (uint16_t i = 0; i < DYNAMIC_MACRO_CRC_LENGTH; ++i) {
    crc = crc16_update(crc, *(data++));
  }
  return crc;
}

uint16_t dynamic_macro_eeprom_macro_addr(uint8_t macro_id) {
    return DYNAMIC_MACRO_EEPROM_BLOCK0_ADDR + sizeof(dynamic_macro_t) * macro_id;
}

bool dynamic_macro_header_correct(void) {
    return eeprom_read_word(DYNAMIC_MACRO_EEPROM_MAGIC_ADDR) == DYNAMIC_MACRO_EEPROM_MAGIC;
}

void dynamic_macro_load_eeprom_all(void) {
  if (!dynamic_macro_header_correct()) {
    //uprintf("dynamic_macro: eeprom header not valid, not restoring macros.\r\n");
    return;
  }

  for (uint8_t i = 0; i < DYNAMIC_MACRO_COUNT; ++i) {
    dynamic_macro_load_eeprom(i);
  }
}

void dynamic_macro_load_eeprom(uint8_t macro_id) {
  dynamic_macro_t* dst = &dynamic_macros[macro_id];
  uint8_t *dstAdd = &dynamic_macros[macro_id]; 
  //eeprom_read_block(dst, dynamic_macro_eeprom_macro_addr(macro_id), sizeof(dynamic_macro_t));
  //uprintf ("id:%d\r\n",macro_id);
  for (uint16_t i=0;i<sizeof(dynamic_macro_t);i++){
    *dstAdd = eeprom_read_byte(dynamic_macro_eeprom_macro_addr(macro_id)+i);
    //uprintf ("%02X ",*dstAdd);
    dstAdd++;
  }  
  //uprintf ("\r\n");

  /* Validate checksum, ifchecksum is NOT valid for macro, set its length to 0 to prevent its use. */
  if (dynamic_macro_calc_crc(dst) != dst->checksum) {
    //uprintf("dynamic macro: slot %d not loaded, checksum mismatch\r\n", macro_id);
    dst->length = 0;

    return;
  }

  //uprintf("dynamic macro: slot %d loaded from eeprom, checksum okay\r\n", macro_id);
}

void dynamic_macro_reset_eeprom(void){
  eeprom_write_word(DYNAMIC_MACRO_EEPROM_MAGIC_ADDR, 0x0000);
}

void dynamic_macro_save_eeprom(uint8_t macro_id,bool mode) {
  if (!dynamic_macro_header_correct()) {
    eeprom_write_word(DYNAMIC_MACRO_EEPROM_MAGIC_ADDR, DYNAMIC_MACRO_EEPROM_MAGIC);
    uprintf("dynamic macro: writing magic eeprom header\r\n");
  }
if (!mode){
  for (uint16_t i=0;i<dynamic_macros[macro_id].length;i++){
    dynamic_macros[macro_id].events[i].event.time = dynamic_macros[macro_id].events[i+1].event.time*10;
  }
  dynamic_macros[macro_id].events[dynamic_macros[macro_id].length-1].event.time = 0;
}

  dynamic_macro_t* src = &dynamic_macros[macro_id];
  uint8_t *srcAdd =  &dynamic_macros[macro_id];
  uprintf ("start macro save address:%X size:%d\r\n",dynamic_macro_eeprom_macro_addr(macro_id),sizeof(dynamic_macro_t));

  for (uint16_t i=0;i<sizeof(dynamic_macro_t);i++){
    eeprom_update_byte(dynamic_macro_eeprom_macro_addr(macro_id)+i, *srcAdd);
    srcAdd++;
  }
  uprintf("dynamic macro: slot %d saved to eeprom\r\n", macro_id);
}
