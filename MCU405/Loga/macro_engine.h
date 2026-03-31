#pragma once

// #include "quantum.h"
#include "eeconfig.h"
#include <stdint.h>
#include <stdbool.h>
// #include "cushion96.h"

#define MACRO_GROUP_COUNT 10
#define MACRO_MAX_STEPS   180
#define MACRO_MIN_STEPS   1

#define MACRO_EEPROM_VERSION 0x0006

#define EEPROM_MACRO_BASE         (EECONFIG_TOGGLE_KEY_ADDR + EECONFIG_TOGGLE_KEY_SIZE)
#define EEPROM_MACRO_VERSION_ADDR (EEPROM_MACRO_BASE)
#define EEPROM_MACRO_SLOT_BASE    (EEPROM_MACRO_BASE + 4)
#define EEPROM_MACRO_SLOT_SIZE    (sizeof(macro_config_t))
#define EEPROM_MACRO_SLOT_ADDR(i) \
    (EEPROM_MACRO_SLOT_BASE + ((i) * EEPROM_MACRO_SLOT_SIZE))

#define MACRO_FIELD_KEYCODE      0x00
#define MACRO_FIELD_DELAY_TYPE   0x01
#define MACRO_FIELD_DELAY_MS     0x02
#define MACRO_FIELD_ACTION       0x03
#define MACRO_FIELD_STEP_COUNT   0x04
#define MACRO_FIELD_CLEAR_STEP   0x05

typedef enum {
    MACRO_DELAY_FIXED  = 0,
    MACRO_DELAY_RANDOM = 1,
} macro_delay_type_t;

typedef enum {
    MACRO_ACTION_NONE    = 0,
    MACRO_ACTION_PRESS   = 1,
    MACRO_ACTION_RELEASE = 2,
} macro_action_t;

typedef struct __attribute__((packed)) {
    uint8_t  delay_type;
    uint16_t delay_ms;
    uint8_t  action;
    uint16_t keycode;
} macro_step_t;

typedef struct __attribute__((packed)) {
    uint8_t      step_count;
    macro_step_t step[MACRO_MAX_STEPS];
} macro_config_t;

void macro_engine_init(void);
void macro_engine_task(void);
bool macro_engine_start(uint8_t macro_index);
void macro_engine_stop(void);
bool macro_engine_is_running(void);

void macro_load_from_eeprom(uint8_t index);
void macro_save_to_eeprom(uint8_t index);

macro_config_t *macro_engine_get_config(uint8_t macro_index);
uint8_t macro_engine_get_loaded_index(void);
bool macro_engine_is_loaded(uint8_t macro_index);
void macro_delete(void);
