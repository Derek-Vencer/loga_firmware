#include "socd.h"
#include <string.h>
#include "ch.h"
#include "hal.h"
#include "keycode.h"
#include "keymap_common.h"
#include "keymap_in_eeprom.h"

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
/*
 * 防止 socd_thread_init() 被重複呼叫。
 */
static bool socd_initialized = false;

static inline bool socd_slot_enabled(const socd_config_t *cfg)
{
    return (cfg != NULL) && (cfg->enable == 1u);
}

static bool socd_config_is_valid(const socd_config_t *cfg)
{
    if (cfg == NULL) {
        return false;
    }

    if (cfg->enable != 1u) {
        return false;
    }

    if (cfg->a_layer >= KEYMAPS_COUNT ||
        cfg->b_layer >= KEYMAPS_COUNT) {
        return false;
    }

    if (cfg->a_row >= MATRIX_ROWS ||
        cfg->b_row >= MATRIX_ROWS) {
        return false;
    }

    if (cfg->a_col >= MATRIX_COLS ||
        cfg->b_col >= MATRIX_COLS) {
        return false;
    }

    if (cfg->mode >= SOCD_DISABLE) {
        return false;
    }

    return true;
}

static uint16_t socd_resolve_keycode(uint8_t layer, uint8_t row, uint8_t col)
{
    if (layer >= KEYMAPS_COUNT || row >= MATRIX_ROWS || col >= MATRIX_COLS) {
        return 0;
    }

#ifdef KEYMAP_IN_EEPROM_ENABLE
    return eeconfig_read_keymapBuffer_key(layer, row, col);
#else
    return keymaps[layer][row][col];
#endif
}

void socd_apply_config(uint8_t idx, const socd_config_t *c)
{
    if (idx >= SOCD_SLOT_COUNT) {
        return;
    }

    if (socd_rt[idx].activeKC) {
        unregister_code(socd_rt[idx].activeKC);
    }

    socd_slots[idx] = *c;

    // ★ 關鍵：清 runtime 狀態
    memset(&socd_rt[idx], 0, sizeof(socd_rt[idx]));
}

void socd_load_all_from_eeprom(void)
{
    uprintf(
        "[SOCD] EEPROM load start, addr=0x%08lX, "
        "slot_size=%u, slots=%u, total=%lu\r\n",
        (unsigned long)(uintptr_t)EECONFIG_SOCD_ADDR,
        (unsigned int)sizeof(socd_config_t),
        (unsigned int)SOCD_SLOT_COUNT,
        (unsigned long)EECONFIG_SOCD_SIZE
    );

    for (uint8_t i = 0; i < SOCD_SLOT_COUNT; i++) {
        socd_config_t tmp;
        memset(&tmp, 0, sizeof(tmp));

        eeconfig_read_socd(i, &tmp);

        uprintf(
            "[SOCD] read slot=%u ena=%u "
            "A=%u/%u/%u kc=%04X "
            "B=%u/%u/%u kc=%04X mode=%u\r\n",
            i,
            tmp.enable,
            tmp.a_layer,
            tmp.a_row,
            tmp.a_col,
            tmp.kc_a,
            tmp.b_layer,
            tmp.b_row,
            tmp.b_col,
            tmp.kc_b,
            tmp.mode
        );

        /*
         * EEPROM 為空、舊資料或資料越界時，
         * 一律轉為 disabled，禁止拿錯誤 layer/row/col 查 keymap。
         */
        if (!socd_config_is_valid(&tmp)) {
            memset(&tmp, 0, sizeof(tmp));
            tmp.mode = SOCD_DISABLE;

            socd_apply_config(i, &tmp);
            continue;
        }

        /*
         * keycode 不完全信任 EEPROM 中保存的值，
         * 每次開機根據目前 keymap 重新解析。
         */
        tmp.kc_a = socd_resolve_keycode(
            tmp.a_layer,
            tmp.a_row,
            tmp.a_col
        );

        tmp.kc_b = socd_resolve_keycode(
            tmp.b_layer,
            tmp.b_row,
            tmp.b_col
        );

        /*
         * 無效 keycode 或兩邊相同時，停用此 slot。
         */
        if (tmp.kc_a == KC_NO ||
            tmp.kc_b == KC_NO ||
            tmp.kc_a == tmp.kc_b) {
            uprintf(
                "[SOCD] disable invalid slot=%u, A=%04X B=%04X\r\n",
                i,
                tmp.kc_a,
                tmp.kc_b
            );

            memset(&tmp, 0, sizeof(tmp));
            tmp.mode = SOCD_DISABLE;
        }

        socd_apply_config(i, &tmp);
    }

    uprintf("[SOCD] EEPROM load done\r\n");
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
        if (!socd_slot_enabled(c)) continue;
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

static THD_FUNCTION(SOCDThread, arg)
{
    (void)arg;

    uprintf("[SOCD] THREAD START\r\n");

    while (true) {
        socd_evt_t ev;

        if (!q_pop(&ev)) {
            chBSemWait(&semSOCD);
            continue;
        }

        if (ev.slot >= SOCD_SLOT_COUNT) {
            continue;
        }

        socd_config_t *cfg = &socd_slots[ev.slot];
        socd_slot_rt_t *rt = &socd_rt[ev.slot];

        if (!socd_slot_enabled(cfg)) {
            continue;
        }

        if (ev.keycode == cfg->kc_a) {
            if (rt->A.down != ev.pressed) {
                rt->A.down = ev.pressed;

                if (ev.pressed) {
                    rt->A.last_press = ev.time_ms;
                }
            }
        } else if (ev.keycode == cfg->kc_b) {
            if (rt->B.down != ev.pressed) {
                rt->B.down = ev.pressed;

                if (ev.pressed) {
                    rt->B.last_press = ev.time_ms;
                }
            }
        } else {
            continue;
        }

        apply_output(
            rt,
            decide_output(cfg, rt)
        );
    }
}

/* ===== Public ===== */
void socd_post_event(uint8_t slot,uint16_t keycode,bool pressed)
{
    if (!socd_initialized) {
        return;
    }

    if (slot >= SOCD_SLOT_COUNT) {
        return;
    }

    socd_evt_t e = {
        .slot = slot,
        .keycode = keycode,
        .pressed = pressed,
        .time_ms = socd_now_ms(),
    };

    chSysLock();

    q_push(e);
    chBSemSignalI(&semSOCD);

    chSysUnlock();
}

void socd_thread_init(void)
{
    if (socd_initialized) {
        uprintf("[SOCD] already initialized\r\n");
        return;
    }

    uprintf("[SOCD] INIT START\r\n");

    /*
     * 清除 RAM runtime。
     */
    memset(socd_slots, 0, sizeof(socd_slots));
    memset(socd_rt, 0, sizeof(socd_rt));
    memset(q, 0, sizeof(q));

    qh = 0;
    qt = 0;

    /*
     * 初始化 semaphore。
     * true 代表初始為 taken，thread 會等待第一個事件 signal。
     */
    chBSemObjectInit(&semSOCD, true);

    /*
     * EEPROM layout magic 由 0xA5 更新為 0xA6。
     *
     * 第一次使用新版韌體時，清除舊版錯誤位址所留下的
     * SOCD 資料，避免開機解析損壞內容。
     */
    uint8_t stored_magic =
        eeprom_read_byte(SOCD_SETTING_MAGIC_ADDR);

    if (stored_magic != SOCD_SETTING_MAGIC) {
        uprintf(
            "[SOCD] EEPROM layout changed: old=%02X new=%02X\r\n",
            stored_magic,
            SOCD_SETTING_MAGIC
        );

        eeconfig_socd_clear();

        eeprom_update_byte(
            SOCD_SETTING_MAGIC_ADDR,
            SOCD_SETTING_MAGIC
        );

        uprintf("[SOCD] EEPROM region cleared\r\n");
    }

    /*
     * 先載入並驗證 EEPROM。
     * 不可先建立 thread，避免 thread 在資料尚未準備好時執行。
     */
    socd_load_all_from_eeprom();

    thread_t *tp = chThdCreateStatic(
        waSOCD,
        sizeof(waSOCD),
        NORMALPRIO - 1,
        SOCDThread,
        NULL
    );

    if (tp == NULL) {
        uprintf("[SOCD] THREAD CREATE FAIL\r\n");
        return;
    }

    socd_initialized = true;

    uprintf("[SOCD] INIT DONE\r\n");
}

uint8_t socd_fill_report(uint8_t *buf)
{
    uint8_t idx;
    socd_config_t *c;

    if (buf == NULL) {
        return 0U;
    }

    /*
     * The caller places the requested slot in buf[0].
     */
    idx = buf[0];

    if (idx >= SOCD_SLOT_COUNT) {
        return 0U;
    }

    c = &socd_slots[idx];

    /*
     * Get report format:
     * buf[0] = index
     * buf[1] = primary layer
     * buf[2] = primary row
     * buf[3] = primary column
     * buf[4] = secondary layer
     * buf[5] = secondary row
     * buf[6] = secondary column
     * buf[7] = priority
     * buf[8] = slot index
     */
    buf[0] = idx;
    buf[1] = c->a_layer;
    buf[2] = c->a_row;
    buf[3] = c->a_col;
    buf[4] = c->b_layer;
    buf[5] = c->b_row;
    buf[6] = c->b_col;
    buf[7] = c->mode;
    buf[8] = idx;

    return 9U;
}

bool socd_apply_report(const uint8_t *buf)
{
    uint8_t idx;
    uint8_t pri_layer;
    uint8_t pri_row;
    uint8_t pri_col;
    uint8_t sec_layer;
    uint8_t sec_row;
    uint8_t sec_col;
    uint8_t mode;
    uint16_t kc_a;
    uint16_t kc_b;
    socd_config_t c;

    if (buf == NULL) {
        uprintf("[SOCD] SET null report\r\n");
        return false;
    }

    pri_layer = buf[0];
    pri_row   = buf[1];
    pri_col   = buf[2];

    sec_layer = buf[3];
    sec_row   = buf[4];
    sec_col   = buf[5];

    mode = buf[6];
    idx  = buf[7];

    uprintf(
        "[SOCD] RX slot=%u "
        "A=%u/%u/%u B=%u/%u/%u mode=%u\r\n",
        idx,
        pri_layer,
        pri_row,
        pri_col,
        sec_layer,
        sec_row,
        sec_col,
        mode
    );

    if (idx >= SOCD_SLOT_COUNT) {
        uprintf("[SOCD] SET invalid slot=%u\r\n", idx);
        return false;
    }

    /*
     * SOCD_DISABLE is an internal value and is not defined
     * in the current Set API payload specification.
     */
    if (mode > SOCD_PRIORITY_NEUTRAL) {
        uprintf(
            "[SOCD] SET invalid mode=%u slot=%u\r\n",
            mode,
            idx
        );
        return false;
    }

    if (pri_layer >= KEYMAPS_COUNT ||
        sec_layer >= KEYMAPS_COUNT ||
        pri_row >= MATRIX_ROWS ||
        sec_row >= MATRIX_ROWS ||
        pri_col >= MATRIX_COLS ||
        sec_col >= MATRIX_COLS) {

        uprintf(
            "[SOCD] SET range error slot=%u "
            "A=%u/%u/%u B=%u/%u/%u\r\n",
            idx,
            pri_layer,
            pri_row,
            pri_col,
            sec_layer,
            sec_row,
            sec_col
        );

        return false;
    }

    kc_a = socd_resolve_keycode(
        pri_layer,
        pri_row,
        pri_col
    );

    kc_b = socd_resolve_keycode(
        sec_layer,
        sec_row,
        sec_col
    );

    uprintf(
        "[SOCD] Resolved slot=%u A=%04X B=%04X\r\n",
        idx,
        kc_a,
        kc_b
    );

    if (kc_a == KC_NO ||
        kc_b == KC_NO ||
        kc_a == kc_b) {

        uprintf(
            "[SOCD] SET keycode error slot=%u "
            "A=%04X B=%04X\r\n",
            idx,
            kc_a,
            kc_b
        );

        return false;
    }

    memset(&c, 0, sizeof(c));

    c.enable = 1U;

    c.kc_a = kc_a;
    c.a_layer = pri_layer;
    c.a_row = pri_row;
    c.a_col = pri_col;

    c.kc_b = kc_b;
    c.b_layer = sec_layer;
    c.b_row = sec_row;
    c.b_col = sec_col;

    c.mode = mode;

    eeconfig_write_socd(idx, &c);
    socd_apply_config(idx, &c);

    uprintf(
        "[SOCD] SET success slot=%u "
        "A=%04X B=%04X mode=%u\r\n",
        idx,
        kc_a,
        kc_b,
        mode
    );

    return true;
}

void eeconfig_read_socd(uint8_t idx,socd_config_t *out)
{
    if (out == NULL) {
        return;
    }

    memset(out, 0, sizeof(*out));

    if (idx >= SOCD_SLOT_COUNT) {
        out->mode = SOCD_DISABLE;
        return;
    }

    uint8_t *addr = EECONFIG_SOCD_ADDR + ((uint32_t)idx * sizeof(socd_config_t));
    eeprom_read_block(out,(const void *)addr,sizeof(*out));
}

void eeconfig_write_socd(uint8_t idx,const socd_config_t *in)
{
    if (in == NULL) {
        return;
    }

    if (idx >= SOCD_SLOT_COUNT) {
        return;
    }

    uint8_t *addr = EECONFIG_SOCD_ADDR + ((uint32_t)idx * sizeof(socd_config_t));
    eeprom_update_block(in,(void *)addr,sizeof(*in));
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

void eeconfig_socd_clear_slot(uint8_t idx)
{
    socd_config_t empty;

    if (idx >= SOCD_SLOT_COUNT) {
        return;
    }

    memset(&empty, 0, sizeof(empty));
    empty.mode = SOCD_DISABLE;

    eeconfig_write_socd(idx, &empty);

    if (socd_rt[idx].activeKC) {
        unregister_code(socd_rt[idx].activeKC);
    }

    memset(&socd_rt[idx], 0, sizeof(socd_rt[idx]));
    socd_slots[idx] = empty;
}

void eeconfig_socd_clear(void) {
    socd_config_t empty;
    memset(&empty, 0, sizeof(empty));
    empty.mode = SOCD_DISABLE;

    for (uint8_t i = 0; i < SOCD_SLOT_COUNT; i++) {
        eeconfig_write_socd(i, &empty);
        // RAM 同步 + 解除可能卡住的輸出
        if (socd_rt[i].activeKC) {
            unregister_code(socd_rt[i].activeKC);
        }
        memset(&socd_rt[i], 0, sizeof(socd_rt[i]));
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
