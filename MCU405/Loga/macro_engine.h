#pragma once

#include "eeconfig.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#define MACRO_GROUP_COUNT 10U
#define MACRO_MAX_STEPS   180U
#define MACRO_MIN_STEPS   1U

#define MACRO_FIELD_KEYCODE      0x00U
#define MACRO_FIELD_DELAY_TYPE   0x01U
#define MACRO_FIELD_DELAY_MS     0x02U
#define MACRO_FIELD_ACTION       0x03U
#define MACRO_FIELD_STEP_COUNT   0x04U
#define MACRO_FIELD_CLEAR_STEP   0x05U

typedef enum
{
    MACRO_DELAY_FIXED  = 0,
    MACRO_DELAY_RANDOM = 1,
} macro_delay_type_t;

typedef enum
{
    MACRO_ACTION_NONE    = 0,
    MACRO_ACTION_PRESS   = 1,
    MACRO_ACTION_RELEASE = 2,
} macro_action_t;

typedef struct __attribute__((packed))
{
    uint8_t  delay_type;
    uint16_t delay_ms;
    uint8_t  action;
    uint16_t keycode;
} macro_step_t;

typedef struct __attribute__((packed))
{
    uint8_t      step_count;
    macro_step_t step[MACRO_MAX_STEPS];
} macro_config_t;

/*
 * EEPROM layout.
 *
 * One slot stores one complete macro_config_t.
 */
#define EEPROM_MACRO_SINGLE_SLOT_SIZE \
    ((size_t)sizeof(macro_config_t))

#define EEPROM_MACRO_REGION_SIZE \
    ((size_t)MACRO_GROUP_COUNT * EEPROM_MACRO_SINGLE_SLOT_SIZE)

#define EEPROM_MACRO_SLOT_ADDR(index) \
    ((uint8_t *)(uintptr_t)(EEPROM_MACRO_REGION_ADDR) + \
     ((size_t)(index) * EEPROM_MACRO_SINGLE_SLOT_SIZE))

#define EEPROM_MACRO_REGION_END \
    ((uint8_t *)(uintptr_t)(EEPROM_MACRO_REGION_ADDR) + \
     EEPROM_MACRO_REGION_SIZE)

void macro_engine_init(void);
void macro_engine_task(void);

bool macro_engine_start(uint8_t macro_index);
void macro_engine_stop(void);
bool macro_engine_is_running(void);

void macro_load_from_eeprom(uint8_t index);
void macro_save_to_eeprom(uint8_t index);
void macro_clear(uint8_t index);
void macro_delete(void);

macro_config_t *macro_engine_get_config(uint8_t macro_index);
uint8_t macro_engine_get_loaded_index(void);
bool macro_engine_is_loaded(uint8_t macro_index);

