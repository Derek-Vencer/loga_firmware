/*
Copyright 2013,2014 Kai Ryu <kai1103@gmail.com>

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

//#include <avr/pgmspace.h>
//#include <avr/eeprom.h>
#include "ch.h"
#include "eeconfig.h"
#include "keymap_in_eeprom.h"
#include "matrix.h"
#include "debug.h"
#include "eeprom.h"

uint16_t eepromKeyBuffer[KEYMAPS_COUNT][MATRIX_ROWS][MATRIX_COLS];
uint16_t eepromTriggerBuffer[MATRIX_ROWS][MATRIX_COLS];
uint16_t eepromReleaseBuffer[MATRIX_ROWS][MATRIX_COLS];
extern int16_t keyMatrixTriggerLevel, keyMatrixReleaseLevel;


#ifdef KEYMAP_IN_EEPROM_ENABLE

bool keymap_in_eeprom_init(void) {
    bool ret;
    ret = check_keymap_in_eeprom();
    if (!ret)
    {
        write_keymap_to_eeprom();
    }
    return ret;
}

void keymap_in_eeprom_disable(void) {
    eeprom_write_word(EECONFIG_KEYMAP_CHECKSUM, eeprom_read_word(EECONFIG_KEYMAP_CHECKSUM) + 1);
}

bool check_keymap_in_eeprom(void) {
    uint16_t checksum_in_eeprom = eeprom_read_word(EECONFIG_KEYMAP_CHECKSUM);
    uint16_t checksum = EECONFIG_MAGIC_NUMBER;
//     for (uint16_t i = 0; i < KEYMAP_SIZE; i++) {
//         uint8_t byte = eeprom_read_byte((uint8_t *)EECONFIG_KEYMAP_FN_ACTIONS + i);
//         uint16_t word = (i & 1) ? byte << 8 : byte;
//         checksum += word;
//     }
// // #ifdef DEBUG
//     eeprom_write_word(EECONFIG_KEYMAP_DEBUG, checksum);
// // #endif
    return (checksum_in_eeprom == checksum);
}

void write_keymap_to_eeprom(void) {
    uint16_t checksum = EECONFIG_MAGIC_NUMBER;
    const uint16_t *fn_actions = fn_actions_pointer();
    const uint16_t *keymaps = keymaps_pointer();
    // write fn_actions
    if (fn_actions != NULL) {
        uint16_t fn_actions_count_in_flash = fn_actions_count();
        //printf ("fn inFlash:%d FN_COUNT:%d\r\n",fn_actions_count_in_flash,FN_ACTIONS_COUNT);
        for (uint16_t i = 0; i < FN_ACTIONS_COUNT; i++) {
            uint16_t fn_action = 0;
            if (i < fn_actions_count_in_flash) {
                //fn_action = pgm_read_word(fn_actions + i);
                fn_action = *(fn_actions+i);
            }
            eeconfig_write_keymap_fn_action(i, fn_action);
            checksum += fn_action;
        }
    }
    // write keymaps
    if (keymaps != NULL) {
        uint16_t keys_count_in_flash = keys_count();
        for (uint16_t i = 0; i < KEYS_COUNT; i++) {
            uint16_t keymap = 0;
            if (i < keys_count_in_flash) {
                //keymap = pgm_read_byte(keymaps + i);
                keymap = *(keymaps+i);
            }
            //printf ("i:%d k:%2X\r\n",i,keymap);
            eeconfig_write_keymap_key_by_index(i, keymap);
            uint16_t keymap_word = keymap;
            if (i & 1) {
                keymap_word = keymap << 8;
            }
            checksum += keymap_word;
        }
    }
    // write checksum
    eeprom_write_word(EECONFIG_KEYMAP_CHECKSUM, EECONFIG_MAGIC_NUMBER);
    eeconfig_keyBuffer_init();
}

void write_trigger_to_eeprom(void){
    for (uint8_t layer=0;layer<KEYMAPS_COUNT;layer++){
        for (uint8_t row=0;row<MATRIX_ROWS;row++){
            for (uint8_t col=0;col<MATRIX_COLS;col++){
                eeprom_write_word(EECONFIG_TRIGGER_LEVEL + TRIGGER_OFFSET(row,col), keyMatrixTriggerLevel);
                eepromTriggerBuffer[row][col] = keyMatrixTriggerLevel;
            }
        }
    }
}

void write_Release_to_eeprom(void){
    for (uint8_t layer=0;layer<KEYMAPS_COUNT;layer++){
        for (uint8_t row=0;row<MATRIX_ROWS;row++){
            for (uint8_t col=0;col<MATRIX_COLS;col++){
                eeprom_write_word(EECONFIG_RELEASE_LEVEL + TRIGGER_OFFSET(row,col), keyMatrixReleaseLevel);
                eepromReleaseBuffer[row][col] = keyMatrixReleaseLevel;
            }
        }
    }
}


uint16_t eeconfig_read_keymap_fn_action(uint8_t index) {
    return eeprom_read_word(EECONFIG_KEYMAP_FN_ACTIONS + index);
}

void eeconfig_write_keymap_fn_action(uint8_t index, uint16_t fn_action) {
    return eeprom_write_word(EECONFIG_KEYMAP_FN_ACTIONS + index, fn_action);
}

void eeconfig_keyBuffer_init(void){
    //uprintf ("key buffer init\r\n");
    for (uint8_t layer=0;layer<KEYMAPS_COUNT;layer++){
        for (uint8_t row=0;row<MATRIX_ROWS;row++){
            for (uint8_t col=0;col<MATRIX_COLS;col++){
                eepromKeyBuffer[layer][row][col] = eeconfig_read_keymap_key(layer,row,col);
                eepromTriggerBuffer[row][col] = eeprom_read_word(EECONFIG_TRIGGER_LEVEL + TRIGGER_OFFSET(row,col));
                eepromReleaseBuffer[row][col] = eeprom_read_word(EECONFIG_RELEASE_LEVEL + TRIGGER_OFFSET(row,col));
            }
        }
    }
}

void eeconfig_rapidTrigger_init(void){
    rapidTrigger_config_t rapidTrigger ={0};
    for (uint8_t i=0;i<RAPID_TRIGGER_COUNT;i++){
        eeconfig_write_rapidTrigger(i, rapidTrigger);
    }
}

uint16_t eeconfig_read_keymap_key( uint8_t layer, uint8_t row, uint8_t col) { 
		const uint32_t byte_offset = (uint32_t)KEY_OFFSET(layer, row, col) * sizeof(uint16_t); 
		const uint16_t *address = (const uint16_t *)(const void *) (EECONFIG_KEYMAP_KEYMAPS + byte_offset); 
		return eeprom_read_word(address); }

uint16_t eeconfig_read_keymapBuffer_key(uint8_t layer, uint8_t row, uint8_t col) {
    return eepromKeyBuffer[layer][row][col];
}

uint16_t eeconfig_read_Trigger( uint8_t row, uint8_t col){
    return eepromTriggerBuffer[row][col];
}

rapidTrigger_config_t eeconfig_read_rapidTrigger(uint8_t key){
    rapidTrigger_config_t rapidTrigger;
    eeprom_read_block(&rapidTrigger, EECONFIG_RAPID_TRIGGER + sizeof(rapidTrigger) * key, sizeof(rapidTrigger));
    return rapidTrigger;
}

uint16_t eeconfig_read_Release( uint8_t row, uint8_t col){
    return eepromReleaseBuffer[row][col];
}

void eeconfig_write_Trigger( uint8_t row, uint8_t col, uint16_t triggerLevel) { 
		eepromTriggerBuffer[row][col] = triggerLevel; 
		/* * ???????? void, * no return eepromTriggerBuffer[row][col]; */ 
}

void eeconfig_write_rapidTrigger( uint8_t key, rapidTrigger_config_t rapidTrigger){
    //eepromTriggerBuffer[row][col] = triggerLevel;
    return eeprom_update_block(&rapidTrigger, EECONFIG_RAPID_TRIGGER + sizeof(rapidTrigger) * key, sizeof(rapidTrigger_config_t));
}

void eeconfig_write_Release( uint8_t row, uint8_t col, uint16_t releaseLevel) { 
		eepromReleaseBuffer[row][col] = releaseLevel; 
		/* * ???????? void, * ?? return eepromReleaseBuffer[row][col]; */ 
}

void eeconfig_write_keymap_key(uint8_t layer,uint8_t row,uint8_t col,uint16_t key)
{
    uint32_t byte_offset;
    uint16_t *address;

    if (layer >= KEYMAPS_COUNT || row >= MATRIX_ROWS || col >= MATRIX_COLS) {
        uprintf("[KEYMAP] Write fail: invalid position L=%u R=%u C=%u\r\n",(unsigned int)layer,(unsigned int)row,(unsigned int)col);
        return;
    }

    byte_offset = (uint32_t)KEY_OFFSET(layer, row, col) * sizeof(uint16_t);
    address = (uint16_t *)(void *)(EECONFIG_KEYMAP_KEYMAPS + byte_offset);
    eeprom_write_word(address,key);
    eepromKeyBuffer[layer][row][col] = key;
    uprintf(
        "[KEYMAP] Write OK "
        "L=%u R=%u C=%u "
        "Key=0x%04X Addr=0x%08lX\r\n",
        (unsigned int)layer,
        (unsigned int)row,
        (unsigned int)col,
        (unsigned int)key,
        (unsigned long)(uintptr_t)address
    );
}

uint16_t eeconfig_read_keymap_key_by_index(uint16_t index) { 
		const uint32_t byte_offset = (uint32_t)index * sizeof(uint16_t); 
		const uint16_t *address = (const uint16_t *)(const void *) (EECONFIG_KEYMAP_KEYMAPS + byte_offset); 
		return eeprom_read_word(address); 
}

void eeconfig_write_keymap_key_by_index( uint16_t index, uint16_t key) { 
		const uint32_t byte_offset = (uint32_t)index * sizeof(uint16_t); 
		uint16_t *address = (uint16_t *)(void *) (EECONFIG_KEYMAP_KEYMAPS + byte_offset); 
		eeprom_write_word(address, key); 
}
#endif
