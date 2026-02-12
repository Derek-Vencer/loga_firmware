// ===== rappy_snappy.c =====

#include "rappy_snappy.h"
#include "matrix.h"

// 你專案既有
// extern int adcCalc;  // 注意：這是「目前掃描到的鍵」用
extern int adcPre; 
// extern int adc_value(uint8_t row, uint8_t col); 
// ↑ 如果你沒有 function，下面會教你怎麼處理

extern void register_code(uint16_t kc);
extern void unregister_code(uint16_t kc);

rappy_snappy_pair_t g_rappy_pairs[RAPPY_SNAPPY_PAIR_COUNT];

bool rappy_snappy_is_key(uint8_t row, uint8_t col)
{
    for (int i = 0; i < RAPPY_SNAPPY_PAIR_COUNT; i++) {
        if (!g_rappy_pairs[i].enable) continue;

        if ((g_rappy_pairs[i].row_a == row &&
             g_rappy_pairs[i].col_a == col) ||
            (g_rappy_pairs[i].row_b == row &&
             g_rappy_pairs[i].col_b == col))
            return true;
    }
    return false;
}

// ===== 單組 pair 的 state machine =====
static void rappy_snappy_process_pair(rappy_snappy_pair_t *p,
                                      int16_t da, int16_t db)
{
    switch (p->state) {

    case RS2_IDLE:
        if (da > db + p->hysteresis) {
            register_code(p->kc_a);
            p->state = RS2_A_ACTIVE;
        } else if (db > da + p->hysteresis) {
            register_code(p->kc_b);
            p->state = RS2_B_ACTIVE;
        }
        break;

    case RS2_A_ACTIVE:
        if (db > da + p->hysteresis) {
            unregister_code(p->kc_a);
            register_code(p->kc_b);
            p->state = RS2_B_ACTIVE;
        } else if (da <= 0 && db <= 0) {
            unregister_code(p->kc_a);
            p->state = RS2_IDLE;
        }
        break;

    case RS2_B_ACTIVE:
        if (da > db + p->hysteresis) {
            unregister_code(p->kc_b);
            register_code(p->kc_a);
            p->state = RS2_A_ACTIVE;
        } else if (da <= 0 && db <= 0) {
            unregister_code(p->kc_b);
            p->state = RS2_IDLE;
        }
        break;
    }
}

// ===== 多組處理 =====
void rappy_snappy_process_all(void)
{
    for (int i = 0; i < RAPPY_SNAPPY_PAIR_COUNT; i++) {

        rappy_snappy_pair_t *p = &g_rappy_pairs[i];
        if (!p->enable) continue;

        // ★ 這裡是關鍵：怎麼取得 depth
        // 若你沒有 adc_value(row,col)，請看下面「補充」
        int16_t da = adcPre_value[p->row_a][p->col_a];
        int16_t db = adcPre_value[p->row_b][p->col_b];


        if (da < 0) da = 0;
        if (db < 0) db = 0;

        rappy_snappy_process_pair(p, da, db);
    }
}
