#pragma once

#include <stdint.h>
#include <stdbool.h>

#ifndef SOCD_SLOT_COUNT
#define SOCD_SLOT_COUNT 10
#endif

typedef enum {
    SOCD_PRIORITY_LAST_INPUT = 0,
    SOCD_PRIORITY_PRIMARY,
    SOCD_PRIORITY_SECONDARY,
    SOCD_PRIORITY_NEUTRAL,
    SOCD_DISABLE,
} socd_policy_t;

/*
 * EEPROM 中每一組 SOCD 的資料。
 *
 * 注意：
 * 不要任意調整欄位順序，否則 sizeof(socd_config_t)
 * 以及 EEPROM layout 都會改變。
 */
typedef struct {
    uint8_t  enable;

    uint16_t kc_a;
    uint8_t  a_layer;
    uint8_t  a_row;
    uint8_t  a_col;

    uint16_t kc_b;
    uint8_t  b_layer;
    uint8_t  b_row;
    uint8_t  b_col;

    uint8_t  mode;
} socd_config_t;

/* ===== Public API ===== */

void socd_thread_init(void);

/* 使用 keycode 尋找所屬 slot */
int8_t socd_find_slot_by_keycode(uint16_t kc);

/* Matrix/hook 將按鍵事件送入 SOCD thread */
void socd_post_event(
    uint8_t slot,
    uint16_t keycode,
    bool pressed
);

/* EEPROM */
void socd_load_all_from_eeprom(void);
void eeconfig_socd_clear(void);
void eeconfig_socd_clear_slot(uint8_t idx);

void eeconfig_read_socd(
    uint8_t idx,
    socd_config_t *out
);

void eeconfig_write_socd(
    uint8_t idx,
    const socd_config_t *in
);

/*
 * VIA / Raw HID：
 *
 * GET/SET buffer 格式：
 * buf[0] = primary layer
 * buf[1] = primary row
 * buf[2] = primary col
 * buf[3] = secondary layer
 * buf[4] = secondary row
 * buf[5] = secondary col
 * buf[6] = SOCD mode
 * buf[7] = slot index
 */
uint8_t socd_fill_report(uint8_t *buf);
bool socd_apply_report(const uint8_t *buf);

/* 測試用 */
void socd_test_write_eeprom_once(void);

extern socd_config_t socd_slots[SOCD_SLOT_COUNT];

