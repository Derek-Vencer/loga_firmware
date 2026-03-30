#include "socd.h"
#include <string.h>
#include "ch.h"
#include "hal.h"
#include "keycode.h"
#include "keymap_common.h"

// extern void register_code(uint16_t kc);
// extern void unregister_code(uint16_t kc);

/* ===== 時間 ===== */
static inline uint32_t socd_now_ms(void) {
    return TIME_I2MS(chVTGetSystemTimeX());
}

/* ===== RAM ===== */
socd_config_t socd_slots[SOCD_SLOT_COUNT];

typedef struct {
    bool down;
    uint32_t last_press;
} socd_key_rt_t;

typedef struct {
    socd_key_rt_t A;
    socd_key_rt_t B;
    uint16_t activeKC;
} socd_slot_rt_t;

static socd_slot_rt_t socd_rt[SOCD_SLOT_COUNT];

void socd_apply_config(uint8_t idx, const socd_config_t *c)
{
    socd_slots[idx] = *c;

    // ★ 關鍵：清 runtime 狀態
    memset(&socd_rt[idx], 0, sizeof(socd_rt[idx]));
}

void socd_load_all_from_eeprom(void)
{
    for (uint8_t i = 0; i < SOCD_SLOT_COUNT; i++) {
        // eeconfig_read_socd(i, &socd_slots[i]);
        socd_config_t tmp;
        eeconfig_read_socd(i, &tmp);

        // uprintf("EEPROM READ: slot%d ena=%d A=%04X B=%04X mode=%d\r\n",
        //         i,
        //         // socd_slots[i].enable,
        //         // socd_slots[i].kc_a,
        //         // socd_slots[i].kc_b,
        //         // socd_slots[i].mode
        //         tmp.enable,
        //         tmp.kc_a,
        //         tmp.kc_b,
        //         tmp.mode
        //     );

        /* 基本防呆 */
        // if (!socd_slots[i].enable) {
        //     socd_slots[i].mode = SOCD_DISABLE;
        // }
        if (!tmp.enable) {
            tmp.mode = SOCD_DISABLE;
        }

        socd_apply_config(i, &tmp);
    }
}

/* ===== Event queue ===== */
typedef struct {
    uint8_t  slot;
    uint16_t keycode;
    bool     pressed;
    uint32_t time_ms;
} socd_evt_t;

#define SOCD_QSIZE 32
static socd_evt_t q[SOCD_QSIZE];
static volatile uint8_t qh = 0, qt = 0;
static binary_semaphore_t semSOCD;

static inline bool q_empty(void){ return qh == qt; }
static inline void q_push(socd_evt_t e){
    uint8_t n = (qh + 1) % SOCD_QSIZE;
    if (n == qt) qt = (qt + 1) % SOCD_QSIZE;
    q[qh] = e; qh = n;
}
static inline bool q_pop(socd_evt_t* out){
    if (q_empty()) return false;
    *out = q[qt];
    qt = (qt + 1) % SOCD_QSIZE;
    return true;
}

/* ===== slot lookup ===== */
int8_t socd_find_slot_by_keycode(uint16_t kc) {
    for (uint8_t i = 0; i < SOCD_SLOT_COUNT; i++) {
        socd_config_t *c = &socd_slots[i];
        if (!c->enable) continue;
        if (kc == c->kc_a || kc == c->kc_b)
            return i;
    }
    return -1;
}

/* ===== output ===== */
static void apply_output(socd_slot_rt_t *rt, uint16_t want) {
    if (rt->activeKC == want) return;
    if (rt->activeKC) unregister_code(rt->activeKC);
    rt->activeKC = want;
    if (rt->activeKC) register_code(rt->activeKC);
}

static uint16_t decide_output(const socd_config_t *cfg,
                              const socd_slot_rt_t *rt)
{
    bool A = rt->A.down;
    bool B = rt->B.down;

    switch ((socd_policy_t)cfg->mode) {
    case SOCD_DISABLE:
        return 0;

    case SOCD_PRIORITY_NEUTRAL:
        if (A && B) return 0;
        return A ? cfg->kc_a : (B ? cfg->kc_b : 0);

    case SOCD_PRIORITY_PRIMARY:
        if (A && B) return cfg->kc_a;
        return A ? cfg->kc_a : (B ? cfg->kc_b : 0);

    case SOCD_PRIORITY_SECONDARY:
        if (A && B) return cfg->kc_b;
        return B ? cfg->kc_b : (A ? cfg->kc_a : 0);

    case SOCD_PRIORITY_LAST_INPUT:
    default:
        if (A && B) {
            if      (rt->A.last_press > rt->B.last_press) return cfg->kc_a;
            else if (rt->B.last_press > rt->A.last_press) return cfg->kc_b;
            else    return rt->activeKC;
        }
        return A ? cfg->kc_a : (B ? cfg->kc_b : 0);
    }
}

/* ===== Thread ===== */
static THD_WORKING_AREA(waSOCD, 512);
static THD_FUNCTION(SOCDThread, arg) {
    (void)arg;
    while (true) {
        uprintf("SOCD THREAD START\r\n");

        socd_evt_t ev;
        if (!q_pop(&ev)) { chBSemWait(&semSOCD); continue; }
        if (ev.slot >= SOCD_SLOT_COUNT) continue;

        socd_config_t  *cfg = &socd_slots[ev.slot];
        socd_slot_rt_t *rt  = &socd_rt[ev.slot];
        if (!cfg->enable) continue;

        if (ev.keycode == cfg->kc_a) {
            if (rt->A.down != ev.pressed) {
                rt->A.down = ev.pressed;
                if (ev.pressed) rt->A.last_press = ev.time_ms;
            }
        } else if (ev.keycode == cfg->kc_b) {
            if (rt->B.down != ev.pressed) {
                rt->B.down = ev.pressed;
                if (ev.pressed) rt->B.last_press = ev.time_ms;
            }
        } else continue;

        apply_output(rt, decide_output(cfg, rt));
    }
}

/* ===== Public ===== */
void socd_post_event(uint8_t slot, uint16_t keycode, bool pressed) {
    socd_evt_t e = { slot, keycode, pressed, socd_now_ms() };
    chSysLock();
    q_push(e);
    chBSemSignalI(&semSOCD);
    chSysUnlock();
}

void socd_thread_init(void) {
    uprintf("SOCD INIT CALLED...\r\n");
    memset(socd_rt, 0, sizeof(socd_rt));
    chBSemObjectInit(&semSOCD, true);
    chThdCreateStatic(waSOCD, sizeof(waSOCD), NORMALPRIO-1, SOCDThread, NULL);
    socd_load_all_from_eeprom();

    // memset(socd_rt, 0, sizeof(socd_rt));
    // chBSemObjectInit(&semSOCD, true);

    // thread_t *tp = chThdCreateStatic(
    //     waSOCD,
    //     sizeof(waSOCD),
    //     NORMALPRIO + 2,     // ★ 明確給高一點
    //     SOCDThread,
    //     NULL
    // );

    // if (tp == NULL) {
    //     uprintf("SOCD THREAD CREATE FAIL\r\n");
    // }
}

uint8_t socd_fill_report(uint8_t *buf) {
    uint8_t idx = buf[0];
    if (idx >= SOCD_SLOT_COUNT) return 0;

    socd_config_t *c = &socd_slots[idx];
    buf[0] = c->a_layer;
    buf[1] = c->a_row;
    buf[2] = c->a_col;
    buf[3] = c->b_layer;
    buf[4] = c->b_row;
    buf[5] = c->b_col;
    buf[6] = c->mode;
    buf[7] = idx;

    // buf[0] = idx;
    // buf[1] = c->kc_a & 0xFF;
    // buf[2] = c->kc_a >> 8;
    // buf[3] = c->kc_b & 0xFF;
    // buf[4] = c->kc_b >> 8;
    // buf[5] = c->mode;
    return 6;
}

void socd_apply_report(const uint8_t *buf) {
    // uint8_t idx = buf[0];
    // if (idx >= SOCD_SLOT_COUNT) return;

    // socd_config_t c = {
    //     .enable = 1,
    //     .kc_a = (uint16_t)buf[1] | ((uint16_t)buf[2] << 8),
    //     .kc_b = (uint16_t)buf[3] | ((uint16_t)buf[4] << 8),
    //     .mode = buf[5],
    // };
    // eeconfig_write_socd(idx, &c);
    // socd_slots[idx] = c;
    // memset(&socd_rt[idx], 0, sizeof(socd_rt[idx]));
    uint8_t idx = buf[7];
    if (idx >= SOCD_SLOT_COUNT) return;

    uint8_t pri_layer = buf[0];
    uint8_t pri_row   = buf[1];
    uint8_t pri_col   = buf[2];
    uint8_t sec_layer = buf[3];
    uint8_t sec_row   = buf[4];
    uint8_t sec_col   = buf[5];
    uint8_t mode      = buf[6];

    uint16_t kc_a = keymaps[pri_layer][pri_row][pri_col];//keymap_get_keycode(pri_layer, pri_row, pri_col);
    uint16_t kc_b = keymaps[sec_layer][sec_row][sec_col];//keymap_get_keycode(sec_layer, sec_row, sec_col);

    socd_config_t c = {
        .enable = 1,
        .kc_a   = kc_a,
        .kc_b   = kc_b,
        .a_layer = pri_layer,
        .a_row  = pri_row,
        .a_col  = pri_col,
        .b_layer = sec_layer,
        .b_row  = sec_row,
        .b_col  = sec_col,
        .mode   = mode,
    };

    eeconfig_write_socd(idx, &c);
    socd_apply_config(idx, &c);
}

void eeconfig_read_socd(uint8_t idx, socd_config_t* out)
{
    if (idx >= SOCD_SLOT_COUNT) {
        memset(out, 0, sizeof(*out));
        return;
    }

    eeprom_read_block(
        out,
        (const void*)(EECONFIG_SOCD_ADDR + idx * sizeof(socd_config_t)),
        sizeof(socd_config_t)
    );
}

void eeconfig_write_socd(uint8_t idx, const socd_config_t* in)
{
    if (idx >= SOCD_SLOT_COUNT) return;

    eeprom_update_block(
        in,
        (void*)(EECONFIG_SOCD_ADDR + idx * sizeof(socd_config_t)),
        sizeof(socd_config_t)
    );
}

void socd_test_write_eeprom_once(void)
{
    socd_config_t test = {
        .enable = 1,
        .kc_a   = KC_E,
        .kc_b   = KC_R,
        .mode   = SOCD_PRIORITY_PRIMARY,
    };

    eeconfig_write_socd(0, &test);

    uprintf("EEPROM WRITE: slot0 A=%04X B=%04X mode=%d\r\n",
            test.kc_a, test.kc_b, test.mode);
}

void eeconfig_socd_clear(void) {
    socd_config_t empty;
    memset(&empty, 0xFF, sizeof(empty));

    for (uint8_t i = 0; i < SOCD_SLOT_COUNT; i++) {
        eeconfig_write_socd(i, &empty);
        // RAM 同步 + 解除可能卡住的輸出
        if (socd_rt[i].activeKC) {
            unregister_code(socd_rt[i].activeKC);
        }
        memset(&socd_rt[i], 0xFF, sizeof(socd_rt[i]));
        socd_slots[i] = empty;
    }
}

// void socd_thread_init(void)
// {
//     uprintf("SOCD INIT CALLED\r\n");

//     /* 清 runtime 狀態 */
//     memset(socd_rt, 0, sizeof(socd_rt));

//     /* 初始化 semaphore */
//     chBSemObjectInit(&semSOCD, true);

//     /* 建立 thread */
//     chThdCreateStatic(
//         waSOCD,
//         sizeof(waSOCD),
//         NORMALPRIO + 1,   // 測試階段給高一點
//         SOCDThread,
//         NULL
//     );
// }
