#ifndef KEYMAP_IN_EEPROM_H
#define KEYMAP_IN_EEPROM_H

#ifdef KEYMAP_IN_EEPROM_ENABLE

#include <stdint.h>
#include <stdbool.h>
#include "matrix.h"
//#include "ledmap_in_eeprom.h"

#ifndef EECONFIG_KEYMAP_IN_EEPROM
#   ifdef EECONFIG_LEDMAP_IN_EEPROM_END
#       define EECONFIG_KEYMAP_IN_EEPROM    EECONFIG_LEDMAP_IN_EEPROM_END
#   else
#       define EECONFIG_KEYMAP_IN_EEPROM    EECONFIG_END
#   endif
#endif

#ifdef KEYMAP_IN_EEPROM_ENABLE
#   define EECONFIG_KEYMAP_IN_EEPROM_END
#else
#   define EECONFIG_KEYMAP_IN_EEPROM_END
#endif

#ifdef DEBUG
#   ifndef VALUE_TO_STRING
#       define VALUE_TO_STRING(x) #x
#       define VALUE(x) VALUE_TO_STRING(x)
#       define VAR_NAME_VALUE(var) #var "="  VALUE(var)
#   endif
#   ifndef MESSAGE_EECONFIG_KEYMAP_IN_EEPROM
#       define MESSAGE_EECONFIG_KEYMAP_IN_EEPROM
#       pragma message(VAR_NAME_VALUE(EECONFIG_KEYMAP_IN_EEPROM))
#   endif
#endif

#ifndef FN_ACTIONS_COUNT
#define FN_ACTIONS_COUNT 32
#endif
#ifndef KEYMAPS_COUNT
#define KEYMAPS_COUNT 1
#endif
#ifndef MATRIX_SIZE
#define MATRIX_SIZE (matrix_rows() * matrix_cols())
#endif
#define KEYS_COUNT (KEYMAPS_COUNT * MATRIX_SIZE)

typedef struct {
    uint16_t checksum;
    uint16_t fn_actions[FN_ACTIONS_COUNT];
    uint16_t keymaps[KEYMAPS_COUNT][MATRIX_ROWS][MATRIX_COLS];
} keymap_in_eeprom_t;

#define EECONFIG_KEYMAP_CHECKSUM   (uint16_t *)(EECONFIG_KEYMAP_IN_EEPROM)
#define EECONFIG_KEYMAP_FN_ACTIONS (uint16_t *)(EECONFIG_KEYMAP_CHECKSUM + 1)
#define EECONFIG_KEYMAP_KEYMAPS    (uint8_t *)(EECONFIG_KEYMAP_FN_ACTIONS + FN_ACTIONS_COUNT)
#define EECONFIG_KEYMAP_DEBUG      (uint16_t *)(EECONFIG_KEYMAP_CHECKSUM - 1)

#define KEYMAP_SIZE (sizeof(uint16_t) * FN_ACTIONS_COUNT + sizeof(uint16_t) * KEYS_COUNT)
#define KEYMAP_WORD_SIZE ((KEYMAP_SIZE + 1) / 2)
#define KEY_OFFSET(layer, row, col) (layer * MATRIX_SIZE + row * matrix_cols() + col)
#define TRIGGER_OFFSET(row,col)(row * matrix_cols() + col)

bool keymap_in_eeprom_init(void);
void keymap_in_eeprom_disable(void);
bool check_keymap_in_eeprom(void);
void write_keymap_to_eeprom(void);
uint16_t eeconfig_read_keymap_key(uint8_t layer, uint8_t row, uint8_t col);
uint16_t eeconfig_read_keymapBuffer_key(uint8_t layer, uint8_t row, uint8_t col);

void write_trigger_to_eeprom(void);
uint16_t eeconfig_read_Trigge(uint8_t row, uint8_t col);
void eeconfig_write_Trigger( uint8_t row, uint8_t col, uint16_t triggerLevel);
uint16_t eeconfig_read_Release(uint8_t row, uint8_t col);
void eeconfig_write_Release( uint8_t row, uint8_t col, uint16_t triggerLevel);
void eeconfig_write_keymap_key(uint8_t layer, uint8_t row, uint8_t col, uint16_t key);
uint16_t eeconfig_read_keymap_key_by_index(uint16_t index);
void eeconfig_write_keymap_key_by_index(uint16_t index, uint16_t key);
uint16_t eeconfig_read_keymap_fn_action(uint8_t index);
void eeconfig_write_keymap_fn_action(uint8_t index, uint16_t fn_action);

void eeconfig_rapidTrigger_init(void);
rapidTrigger_config_t eeconfig_read_rapidTrigger(uint8_t key);
void eeconfig_write_rapidTrigger( uint8_t key, rapidTrigger_config_t rapidTrigger);

const uint16_t* keymaps_pointer(void);
const uint16_t* fn_actions_pointer(void);
uint16_t keys_count(void);
uint16_t fn_actions_count(void);
#endif

#endif
