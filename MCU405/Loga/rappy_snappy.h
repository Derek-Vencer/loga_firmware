// ===== rappy_snappy.h =====

#pragma once
#include <stdint.h>
#include <stdbool.h>

#define RAPPY_SNAPPY_PAIR_COUNT 8   // 可自行調整

static int16_t adc_value[MATRIX_ROWS][MATRIX_COLS];

typedef enum {
    RS2_IDLE = 0,
    RS2_A_ACTIVE,
    RS2_B_ACTIVE
} rs2_state_t;

typedef struct {
    uint8_t enable;

    // Key A
    uint8_t row_a;
    uint8_t col_a;
    uint16_t kc_a;

    // Key B
    uint8_t row_b;
    uint8_t col_b;
    uint16_t kc_b;

    int16_t hysteresis;   // 深度差門檻
    rs2_state_t state;
} rappy_snappy_pair_t;

extern rappy_snappy_pair_t g_rappy_pairs[RAPPY_SNAPPY_PAIR_COUNT];

bool rappy_snappy_is_key(uint8_t row, uint8_t col);
void rappy_snappy_process_all(void);
void rappy_snappy_debug_init(void);
