/******************

#pragma once
#include <stdint.h>
#include <stdbool.h>
#include "keyboard.h" //keypos_t

// SOCD 策略（預設用 LAST_INPUT）
typedef enum {
    SOCD_PRIORITY_LAST_INPUT = 0,  // 0
    SOCD_PRIORITY_PRIMARY    = 1,  // 1
    SOCD_PRIORITY_SECONDARY  = 2,  // 2
    SOCD_PRIORITY_NEUTRAL    = 3,   // 3
    SOCD_DISABLE             = 4
} socd_policy_t;

// 啟動 SOCD 執行緒（只需呼叫一次）
void socd_thread_init(void);

// 可動態切換策略（可不呼叫，預設 LAST_INPUT）
void socd_set_policy(socd_policy_t p);

// 回報 (row,col) 是否由 SOCD 接管（若 true，掃描層就不要再走原本送鍵路徑）
bool socd_claims_pos(uint8_t row, uint8_t col);

// 掃描層在「邊緣事件（press/release）」丟給 SOCD（非阻塞）
void socd_post_event(uint8_t slot, uint8_t row, uint8_t col, bool pressed);

// VIA 呼叫用：把目前的 SOCD 設定寫入 buffer，回傳長度
uint8_t socd_fill_report(uint8_t *buf);

// setter：從 VIA 傳進來的 buffer 更新設定
void socd_apply_report(const uint8_t *buf);

typedef struct {
    uint8_t enable;

    // Primary key
    uint8_t layer_a;
    uint8_t row_a;
    uint8_t col_a;

    // Secondary key
    uint8_t layer_b;
    uint8_t row_b;
    uint8_t col_b;

    uint8_t mode;      // socd_mode_t
    uint8_t alt_full;  // alternative fully pressed behaviour
    uint8_t reserved;
} socd_config_t;

#define SOCD_SLOT_COUNT   2   // 8 組SOCD

extern socd_config_t socd_slots[SOCD_SLOT_COUNT];

void socd_load_all_from_eeprom(void)
******************/
// #pragma once
// #include <stdint.h>
// #include <stdbool.h>

// #ifndef SOCD_SLOT_COUNT
// #define SOCD_SLOT_COUNT 2
// #endif

// typedef enum {
//     SOCD_DISABLE = 0,
//     SOCD_PRIORITY_NEUTRAL = 1,
//     SOCD_PRIORITY_PRIMARY = 2,
//     SOCD_PRIORITY_SECONDARY = 3,
//     SOCD_PRIORITY_LAST_INPUT = 4,
// } socd_policy_t;

// /* EEPROM/RAM 設定：每個 slot 一份 */
// typedef struct {
//     uint8_t enable;

//     uint8_t layer_a;
//     uint8_t row_a;
//     uint8_t col_a;

//     uint8_t layer_b;
//     uint8_t row_b;
//     uint8_t col_b;

//     uint8_t mode;      // socd_policy_t
//     uint8_t reserved;  // alignment/future
// } socd_config_t;

// /* ===== 你需要提供的 keymap 介面（TODO） =====
//  * 回傳指定 layer,row,col 的 keycode。
//  */
// extern uint16_t keymap_get_keycode(uint8_t layer, uint8_t row, uint8_t col);

// /* ===== SOCD public API ===== */
// void socd_thread_init(void);

// /* matrix.c 用：用 layer,row,col 找到屬於哪個 slot (找不到回 -1) */
// int8_t socd_find_slot(uint8_t layer, uint8_t row, uint8_t col);

// /* matrix.c 用：把事件丟給 SOCD（slot 已算好） */
// void socd_post_event(uint8_t slot, uint8_t row, uint8_t col, bool pressed);

// /* EEPROM */
// void socd_load_all_from_eeprom(void);
// void eeconfig_socd_clear(void);

// /* VIA / RawHID 封包：
//  * buf[0]=index, buf[1..7] = layerA,rowA,colA,layerB,rowB,colB,policy
//  */
// uint8_t socd_fill_report(uint8_t *buf);          // GET
// void    socd_apply_report(const uint8_t *buf);   // SET

// /* 你的 EEPROM driver（你專案裡應該已經有） */
// void eeconfig_read_socd(uint8_t idx, socd_config_t* out);
// void eeconfig_write_socd(uint8_t idx, const socd_config_t* in);

#pragma once
#include <stdint.h>
#include <stdbool.h>

#ifndef SOCD_SLOT_COUNT
#define SOCD_SLOT_COUNT 2
#endif

typedef enum {
    // SOCD_DISABLE = 0,
    SOCD_PRIORITY_LAST_INPUT = 0,
    SOCD_PRIORITY_PRIMARY,
    SOCD_PRIORITY_SECONDARY,
    SOCD_PRIORITY_NEUTRAL,
    SOCD_DISABLE,
} socd_policy_t;

/* ===== 每一組 SOCD 的設定（只存 keycode） ===== */
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
    uint8_t  mode;   // socd_policy_t
} socd_config_t;

/* ===== Public API ===== */
void socd_thread_init(void);

/* 用 keycode 找 slot */
int8_t socd_find_slot_by_keycode(uint16_t kc);

/* matrix / hook 丟事件進來 */
void socd_post_event(uint8_t slot, uint16_t keycode, bool pressed);

/* EEPROM */
void socd_load_all_from_eeprom(void);
void eeconfig_socd_clear(void);

/* VIA / RawHID
 * buf = [idx, kc_a_L, kc_a_H, kc_b_L, kc_b_H, mode]
 */
uint8_t socd_fill_report(uint8_t *buf);
void socd_apply_report(const uint8_t *buf);

/* EEPROM driver（你專案本來就有） */
void eeconfig_read_socd(uint8_t idx, socd_config_t* out);
void eeconfig_write_socd(uint8_t idx, const socd_config_t* in);

void socd_test_write_eeprom_once(void);

