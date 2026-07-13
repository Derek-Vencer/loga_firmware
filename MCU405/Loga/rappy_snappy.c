// ===== rappy_snappy.c =====

#include "rappy_snappy.h"
#include "matrix.h"
#include <string.h>
#include "eeprom.h"
#include "action.h"
// 雿�����
// extern int adcCalc;  // 瘜冽�����������
extern int adcPre; 
// extern int adc_value(uint8_t row, uint8_t col); 
// ��� 憒����� function嚗������獐����


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

// ===== �蝯� pair ��� state machine =====
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

// ===== 憭���� =====
void rappy_snappy_process_all(void)
{
    for (int i = 0; i < RAPPY_SNAPPY_PAIR_COUNT; i++) {

        rappy_snappy_pair_t *p = &g_rappy_pairs[i];
        if (!p->enable) continue;

        // ��� �ㄐ���嚗�獐���� depth
        // �雿��� adc_value(row,col)嚗��������
        int16_t da = adcPre_value[p->row_a][p->col_a];
        int16_t db = adcPre_value[p->row_b][p->col_b];


        if (da < 0) da = 0;
        if (db < 0) db = 0;
        // uprintf("da value is %d, db value is %d\r\n", da, db);

        rappy_snappy_process_pair(p, da, db);
    }
}
void rappy_snappy_apply_config(uint8_t index, const rappy_snappy_pair_t *in){
    g_rappy_pairs[index] = *in;
    // uprintf("g_rappy_pairs a layer %d row %d col %d, b layer %d, row %d col %d\r\n",
    //     g_rappy_pairs[index].lay_a,
    //     g_rappy_pairs[index].row_a,
    //     g_rappy_pairs[index].col_a,
    //     g_rappy_pairs[index].lay_b,
    //     g_rappy_pairs[index].row_b,
    //     g_rappy_pairs[index].col_b);

    // 皜� runtime �����
    // memset(&g_rappy_pairs[index], 0, sizeof(g_rappy_pairs[index]));
}

void eeconfig_read_rappy_snappy(uint8_t idx, rappy_snappy_pair_t* out)
{
    if (idx >= RAPPY_SNAPPY_PAIR_COUNT) {
        memset(out, 0, sizeof(*out));
        return;
    }

    eeprom_read_block(
        out,
        (const void*)(EECONFIG_RAPPYSNAPPY_ADDR + idx * sizeof(rappy_snappy_pair_t)),
        sizeof(rappy_snappy_pair_t)
    );
}

void eeconfig_write_rappy_snappy(uint8_t idx, const rappy_snappy_pair_t* in)
{
    if (idx >= RAPPY_SNAPPY_PAIR_COUNT) return;

    eeprom_update_block(
        in,
        (void*)(EECONFIG_RAPPYSNAPPY_ADDR + idx * sizeof(rappy_snappy_pair_t)),
        sizeof(rappy_snappy_pair_t)
    );
    rappy_snappy_apply_config(idx, in);
}

void eeconfig_rappy_snappy_clear(void) {
    rappy_snappy_pair_t empty;
    memset(&empty, 0xff, sizeof(empty));

    for (uint8_t i = 0; i < RAPPY_SNAPPY_PAIR_COUNT; i++) {
        
        eeconfig_write_rappy_snappy(i, &empty);
        // RAM ��郊 + 閫�����雿�撓�
        // if (socd_rt[i].activeKC) {
        //     unregister_code(socd_rt[i].activeKC);
        // }
        // memset(&socd_rt[i], 0, sizeof(socd_rt[i]));
        // socd_slots[i] = empty;
    }
}

void load_all_rappy_snappy(void){
    for (int i = 0; i < RAPPY_SNAPPY_PAIR_COUNT; i++) {
        rappy_snappy_pair_t *p = &g_rappy_pairs[i];
        eeconfig_read_rappy_snappy(i, p);
        rappy_snappy_apply_config(i, p);

        // if (!g_rappy_pairs[i].enable) continue;
        // if ((g_rappy_pairs[i].row_a == row &&
        //      g_rappy_pairs[i].col_a == col) ||
        //     (g_rappy_pairs[i].row_b == row &&
        //      g_rappy_pairs[i].col_b == col))
        //     return true;
    }
    // return true;
}
