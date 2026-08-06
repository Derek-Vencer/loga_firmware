#include "macro_engine.h"
// #include "cushion96.h"
#include "eeprom.h" // this might work
// #include "bt_protocol.h"
// #include "quantum.h"
#include "uart.h"
#include "print.h"
#include "keycode.h"
#include "wait.h"
// #include "sleep_manager.h"

#include "action_util.h"
#include "timer.h"
#include <string.h>
#include "action.h"

static uint16_t clamp_delay_by_mode(uint16_t delay_ms);

// -----------------------------
// Random delay helper
// -----------------------------

static uint16_t pseudo_random_u16(void) {
    uint32_t t = timer_read32();
    t ^= (t << 7);
    t ^= (t >> 9);
    t ^= (t << 8);
    return (uint16_t)(t & 0xFFFF);
}

static uint16_t get_effective_delay_ms(const macro_step_t *step) {
    uint16_t delay_ms = step->delay_ms;

    if (step->delay_type == MACRO_DELAY_RANDOM) {
        if (delay_ms > 0) {
            delay_ms = (uint16_t)(pseudo_random_u16() % (delay_ms + 1));
        }
    }

    return clamp_delay_by_mode(delay_ms);
}

// -----------------------------
// Wireless state machine
// -----------------------------
extern uint8_t current_wireless_mode;

// -----------------------------
// Wireless HID report bridge
// -----------------------------
#if 0
typedef struct {
    uint8_t mods;
    uint8_t keys[6];
} whid_report_t;

static whid_report_t g_whid = {0};
static whid_report_t g_whid_last_sent = {0};

// static bool whid_is_wireless_mode(void) {
//     return (current_wireless_mode == BT_MODE_BLE) || (current_wireless_mode == BT_MODE_RF);
// }

static uint32_t whid_get_interval_us(void) {
    uint32_t delay_us = 1000;

    // if (current_wireless_mode == BT_MODE_BLE) {
    //     uint16_t ble_hz = ble_get_polling_rate_hz();
    //     if (ble_hz > 0) {
    //         delay_us = 1000000UL / ble_hz;
    //     }
    // } else if (current_wireless_mode == BT_MODE_RF) {
    //     uint16_t rf_hz = rf_get_polling_rate_hz();
    //     if (rf_hz > 0) {
    //         delay_us = 500000UL / rf_hz;
    //     }
    // }

    if (delay_us < 500) {
        delay_us = 500;
    }

    return delay_us;
}

static void whid_wait_one_interval(void) {
    uint32_t us = whid_get_interval_us();
    if (us < 500) us = 500;
    wait_us(us);
}

static bool whid_report_equal(const whid_report_t *a, const whid_report_t *b) {
    if (a->mods != b->mods) return false;
    return memcmp(a->keys, b->keys, sizeof(a->keys)) == 0;
}

static void whid_send_if_changed(void) {
    // if (!whid_is_wireless_mode()) return;

    if (whid_report_equal(&g_whid, &g_whid_last_sent)) {
        return;
    }

    g_whid_last_sent = g_whid;
    whid_wait_one_interval();
}

// -----------------------------
// Keycode -> HID helpers
// -----------------------------

static bool whid_is_basic_modifier(uint16_t kc) {
    return (kc == KC_LCTL) || (kc == KC_LSFT) || (kc == KC_LALT) || (kc == KC_LGUI) ||
           (kc == KC_RCTL) || (kc == KC_RSFT) || (kc == KC_RALT) || (kc == KC_RGUI);
}

static uint8_t whid_modifier_bit(uint16_t kc) {
    switch (kc) {
        case KC_LCTL: return (1u << 0);
        case KC_LSFT: return (1u << 1);
        case KC_LALT: return (1u << 2);
        case KC_LGUI: return (1u << 3);
        case KC_RCTL: return (1u << 4);
        case KC_RSFT: return (1u << 5);
        case KC_RALT: return (1u << 6);
        case KC_RGUI: return (1u << 7);
        default:      return 0;
    }
}

__attribute__((weak)) uint8_t keycode_to_hid(uint16_t keycode);

static uint8_t whid_keycode_to_hid_usage(uint16_t kc) {
    if (keycode_to_hid) {
        return keycode_to_hid(kc);
    }

    if (kc >= KC_A && kc <= KC_Z) return (uint8_t)(0x04 + (kc - KC_A));
    if (kc >= KC_1 && kc <= KC_0) return (uint8_t)(0x1E + (kc - KC_1));
    if (kc == KC_ENTER) return 0x28;
    if (kc == KC_ESC)   return 0x29;
    if (kc == KC_BSPC)  return 0x2A;
    if (kc == KC_TAB)   return 0x2B;
    if (kc == KC_SPC)   return 0x2C;

    return 0;
}

// -----------------------------
// 6KRO slot management
// -----------------------------

static bool whid_has_key(uint8_t usage) {
    for (uint8_t i = 0; i < 6; i++) {
        if (g_whid.keys[i] == usage) return true;
    }
    return false;
}

static bool whid_add_key(uint8_t usage) {
    if (usage == 0) return false;
    if (whid_has_key(usage)) return true;

    for (uint8_t i = 0; i < 6; i++) {
        if (g_whid.keys[i] == 0) {
            g_whid.keys[i] = usage;
            return true;
        }
    }

    uprintf("[WHID] 6KRO overflow, drop usage=0x%02X\n", usage);
    return false;
}

static void whid_del_key(uint8_t usage) {
    for (uint8_t i = 0; i < 6; i++) {
        if (g_whid.keys[i] == usage) {
            g_whid.keys[i] = 0;
        }
    }
}

static void whid_press(uint16_t kc) {
    if (whid_is_basic_modifier(kc)) {
        g_whid.mods |= whid_modifier_bit(kc);
        whid_send_if_changed();
        return;
    }

    uint8_t usage = whid_keycode_to_hid_usage(kc);
    if (usage) {
        whid_add_key(usage);
        whid_send_if_changed();
    }
}

static void whid_release(uint16_t kc) {
    if (whid_is_basic_modifier(kc)) {
        g_whid.mods &= (uint8_t)~whid_modifier_bit(kc);
        whid_send_if_changed();
        return;
    }

    uint8_t usage = whid_keycode_to_hid_usage(kc);
    if (usage) {
        whid_del_key(usage);
        whid_send_if_changed();
    }
}

static void whid_release_all(void) {
    g_whid.mods = 0;
    memset(g_whid.keys, 0, sizeof(g_whid.keys));
    whid_send_if_changed();
}

#endif
// -----------------------------
// Internal state machine
// -----------------------------

typedef enum {
    ME_IDLE = 0,
    ME_WAIT_DELAY,
    ME_EXECUTE_STEP,
    ME_DONE,
} me_state_t;

/*
 * RAM optimization:
 * Keep only one macro config in RAM.
 * EEPROM still stores all macro groups.
 */
static macro_config_t g_macro_cfg;
static uint8_t g_loaded_macro_index = 0xFF;

static struct {
    bool       running;
    uint8_t    macro_index;
    uint8_t    step_index;
    me_state_t state;
    uint32_t   deadline_ms;
} g_me;

// -----------------------------
// Delay helpers
// -----------------------------

static uint16_t clamp_delay_by_mode(uint16_t delay_ms) {
    // if (current_wireless_mode == BT_MODE_BLE) {
    //     if (delay_ms < 8 && delay_ms != 0) {
    //         return 8;
    //     }
    // } else if (current_wireless_mode == BT_MODE_RF) {
    //     if (delay_ms < 2 && delay_ms != 0) {
    //         return 2;
    //     }
    // }
    return delay_ms;
}

static void set_deadline_from_now(uint16_t delay_ms) {
    g_me.deadline_ms = timer_read32() + (uint32_t)delay_ms;
}

// -----------------------------
// Key press/release helpers
// -----------------------------

static void press_key(uint16_t keycode) {
    uint8_t usage = (uint8_t)(keycode & 0xFF);

    // if (current_wireless_mode == BT_MODE_BLE ||
    //     current_wireless_mode == BT_MODE_RF) {
    //     whid_press(keycode);
    // } else {
        if (usage) {
            register_code(usage);
            send_keyboard_report();
        }
    // }
}

static void release_key(uint16_t keycode) {
    uint8_t usage = (uint8_t)(keycode & 0xFF);

    // if (current_wireless_mode == BT_MODE_BLE ||
    //     current_wireless_mode == BT_MODE_RF) {
    //     whid_release(keycode);
    // } else {
        if (usage) {
            unregister_code(usage);
            send_keyboard_report();
        }
    // }
}

static void release_all_keys(void) {
    // if (whid_is_wireless_mode()) {
    //     whid_release_all();
    // } else {
        clear_keyboard();
        send_keyboard_report();
    // }
}

// -----------------------------
// EEPROM init
// -----------------------------
/*
static void macro_eeprom_init_if_needed(void) {
    uint16_t v = (uint16_t)eeprom_read_dword((void *)EEPROM_MACRO_VERSION_ADDR);

    if (v != MACRO_EEPROM_VERSION) {
        eeprom_update_dword((void *)EEPROM_MACRO_VERSION_ADDR, MACRO_EEPROM_VERSION);

        macro_config_t empty = {0};
        for (uint8_t i = 0; i < MACRO_GROUP_COUNT; i++) {
            eeprom_update_block(&empty, (void *)EEPROM_MACRO_SLOT_ADDR(i), sizeof(empty));
        }
    }
}
*/

// -----------------------------
// Public API
// -----------------------------

uint8_t macro_engine_get_loaded_index(void) {
    return g_loaded_macro_index;
}

bool macro_engine_is_loaded(uint8_t macro_index) {
    return (g_loaded_macro_index == macro_index);
}

macro_config_t *macro_engine_get_config(uint8_t macro_index) {
    if (macro_index >= MACRO_GROUP_COUNT) return NULL;

    if (g_loaded_macro_index != macro_index) {
        macro_load_from_eeprom(macro_index);
    }

    return &g_macro_cfg;
}

bool macro_engine_is_running(void) {
    return g_me.running;
}

void macro_engine_stop(void) {
    release_all_keys();

    g_me.running = false;
    g_me.state = ME_IDLE;
    g_me.step_index = 0;
    g_me.macro_index = 0xFF;
    g_me.deadline_ms = 0;
}

bool macro_engine_start(uint8_t macro_index)
{
    macro_config_t *cfg;
    bool has_executable_step = false;

    if (macro_index >= MACRO_GROUP_COUNT) {
        uprintf(
            "[MACRO] start fail: invalid index=%u\r\n",
            (unsigned int)macro_index
        );
        return false;
    }

    if (g_loaded_macro_index != macro_index) {
        macro_load_from_eeprom(macro_index);
    }

    cfg = &g_macro_cfg;

    if (cfg->step_count < MACRO_MIN_STEPS ||
        cfg->step_count > MACRO_MAX_STEPS) {
        uprintf(
            "[MACRO] start fail: invalid step_count=%u\r\n",
            (unsigned int)cfg->step_count
        );
        return false;
    }

    for (uint8_t i = 0U; i < cfg->step_count; i++) {
        if ((cfg->step[i].action == MACRO_ACTION_PRESS ||
             cfg->step[i].action == MACRO_ACTION_RELEASE) &&
            cfg->step[i].keycode != KC_NO) {
            has_executable_step = true;
            break;
        }
    }

    if (!has_executable_step) {
        uprintf(
            "[MACRO] start fail: no executable step idx=%u\r\n",
            (unsigned int)macro_index
        );
        return false;
    }

    if (g_me.running) {
        uprintf("[MACRO] already running, stop first\r\n");
        macro_engine_stop();
    }

    g_me.running = true;
    g_me.macro_index = macro_index;
    g_me.step_index = 0U;
    g_me.deadline_ms = 0U;

    /*
     * The delay belongs to the current step, matching the previous
     * synchronous implementation. The task executes the step after
     * this delay expires.
     */
    {
        uint16_t delay_ms = get_effective_delay_ms(&cfg->step[0]);

        if (delay_ms > 0U) {
            set_deadline_from_now(delay_ms);
            g_me.state = ME_WAIT_DELAY;
        } else {
            g_me.state = ME_EXECUTE_STEP;
        }
    }

    uprintf(
        "[MACRO] queued idx=%u step_count=%u\r\n",
        (unsigned int)macro_index,
        (unsigned int)cfg->step_count
    );

    return true;
}

void macro_engine_init(void) {
    uintptr_t start =
        (uintptr_t)EEPROM_MACRO_REGION_ADDR;

    uintptr_t end =
        (uintptr_t)EEPROM_MACRO_REGION_END;

    uprintf(
        "[MACRO] EEPROM start=0x%04lX end=0x%04lX size=%lu\r\n",
        (unsigned long)start,
        (unsigned long)end,
        (unsigned long)(end - start)
    );

    if (end > 0x10000UL || end <= start) {
        uprintf("[MACRO] ERROR: EEPROM range invalid\r\n");
        return;
    }

    // macro_eeprom_init_if_needed();
}

void macro_engine_task(void)
{
    macro_config_t *cfg;

    if (!g_me.running) {
        return;
    }

    cfg = &g_macro_cfg;

    if (g_loaded_macro_index != g_me.macro_index ||
        cfg->step_count < MACRO_MIN_STEPS ||
        cfg->step_count > MACRO_MAX_STEPS) {
        uprintf("[MACRO] runtime config invalid\r\n");
        macro_engine_stop();
        return;
    }

    if (g_me.step_index >= cfg->step_count) {
        g_me.state = ME_DONE;
    }

    switch (g_me.state) {
        case ME_WAIT_DELAY:
        {
            uint32_t now = timer_read32();

            if ((int32_t)(now - g_me.deadline_ms) < 0) {
                return;
            }

            g_me.state = ME_EXECUTE_STEP;
        }
        break;

        case ME_EXECUTE_STEP:
        {
            macro_step_t *step;

            if (g_me.step_index >= cfg->step_count) {
                g_me.state = ME_DONE;
                break;
            }

            step = &cfg->step[g_me.step_index];

            switch (step->action) {
                case MACRO_ACTION_PRESS:
                    if (step->keycode != KC_NO) {
                        press_key(step->keycode);
                    }
                    break;

                case MACRO_ACTION_RELEASE:
                    if (step->keycode != KC_NO) {
                        release_key(step->keycode);
                    }
                    break;

                case MACRO_ACTION_NONE:
                    break;

                default:
                    uprintf(
                        "[MACRO] skip invalid action=%u step=%u\r\n",
                        (unsigned int)step->action,
                        (unsigned int)g_me.step_index
                    );
                    break;
            }

            g_me.step_index++;

            if (g_me.step_index >= cfg->step_count) {
                g_me.state = ME_DONE;
            } else {
                uint16_t delay_ms =
                    get_effective_delay_ms(&cfg->step[g_me.step_index]);

                if (delay_ms > 0U) {
                    set_deadline_from_now(delay_ms);
                    g_me.state = ME_WAIT_DELAY;
                } else {
                    g_me.state = ME_EXECUTE_STEP;
                }
            }
        }
        break;

        case ME_DONE:
            uprintf(
                "[MACRO] done idx=%u executed=%u\r\n",
                (unsigned int)g_me.macro_index,
                (unsigned int)g_me.step_index
            );
            macro_engine_stop();
            break;

        case ME_IDLE:
        default:
            uprintf(
                "[MACRO] invalid state=%u -> stop\r\n",
                (unsigned int)g_me.state
            );
            macro_engine_stop();
            break;
    }
}

void macro_load_from_eeprom(uint8_t index)
{
    const uint8_t *slot_addr;

    if (index >= MACRO_GROUP_COUNT) {
        return;
    }

    slot_addr = EEPROM_MACRO_SLOT_ADDR(index);

    memset(&g_macro_cfg, 0, sizeof(g_macro_cfg));

    eeprom_read_block(
        &g_macro_cfg,
        slot_addr,
        sizeof(g_macro_cfg)
    );

    uprintf(
        "[MACRO] Load index=%u addr=%p step_count=%u\r\n",
        (unsigned int)index,
        slot_addr,
        (unsigned int)g_macro_cfg.step_count
    );

    /*
     * Dump every step before sanitize.
     */
    for (uint8_t i = 0;
         i < MACRO_MAX_STEPS;
         i++) {

        macro_step_t *step = &g_macro_cfg.step[i];

        uprintf(
            "[MACRO] RAW step=%u "
            "type=%u delay=%u action=%u key=0x%04X\r\n",
            (unsigned int)i,
            (unsigned int)step->delay_type,
            (unsigned int)step->delay_ms,
            (unsigned int)step->action,
            (unsigned int)step->keycode
        );
    }

    /*
     * Erased EEPROM.
     */
    if (g_macro_cfg.step_count == 0xFFU ||
        g_macro_cfg.step_count > MACRO_MAX_STEPS) {

        uprintf(
            "[MACRO] invalid EEPROM step_count=%u idx=%u\r\n",
            (unsigned int)g_macro_cfg.step_count,
            (unsigned int)index
        );

        memset(&g_macro_cfg, 0, sizeof(g_macro_cfg));
        g_loaded_macro_index = index;
        return;
    }

    for (uint8_t i = 0;
         i < g_macro_cfg.step_count;
         i++) {

        macro_step_t *step = &g_macro_cfg.step[i];

        if (step->delay_type != MACRO_DELAY_FIXED &&
            step->delay_type != MACRO_DELAY_RANDOM) {

            uprintf(
                "[MACRO] Fix delay_type step=%u (%u->0)\r\n",
                (unsigned int)i,
                (unsigned int)step->delay_type
            );

            step->delay_type = MACRO_DELAY_FIXED;
            step->delay_ms   = 0;
        }

        if (step->action > MACRO_ACTION_RELEASE) {

            uprintf(
                "[MACRO] Fix action step=%u (%u->NONE)\r\n",
                (unsigned int)i,
                (unsigned int)step->action
            );

            step->action  = MACRO_ACTION_NONE;
            step->delay_ms = 0;
            step->keycode = KC_NO;
        }

        if ((step->action == MACRO_ACTION_PRESS ||
             step->action == MACRO_ACTION_RELEASE) &&
            step->keycode == 0xFFFFU) {

            uprintf(
                "[MACRO] Fix keycode step=%u\r\n",
                (unsigned int)i
            );

            step->keycode = KC_NO;
        }
    }

    uprintf("[MACRO] Load finish\r\n");

    g_loaded_macro_index = index;
}

void macro_save_to_eeprom(uint8_t index)
{
    uint8_t *slot_addr;
    uint8_t step_count;
    size_t used_size;

    if (index >= MACRO_GROUP_COUNT) {
        uprintf(
            "[MACRO] Save fail: invalid index=%u\r\n",
            (unsigned int)index
        );
        return;
    }

    step_count = g_macro_cfg.step_count;

    if (step_count > MACRO_MAX_STEPS) {
        step_count = MACRO_MAX_STEPS;
        g_macro_cfg.step_count = step_count;
    }

    slot_addr = EEPROM_MACRO_SLOT_ADDR(index);

    /*
     * Store step_count followed by all active macro steps.
     */
    used_size =
        sizeof(g_macro_cfg.step_count) +
        ((size_t)step_count * sizeof(macro_step_t));

    if (used_size > EEPROM_MACRO_SINGLE_SLOT_SIZE) {
        uprintf(
            "[MACRO] Save fail: size overflow "
            "idx=%u size=%lu max=%lu\r\n",
            (unsigned int)index,
            (unsigned long)used_size,
            (unsigned long)EEPROM_MACRO_SINGLE_SLOT_SIZE
        );
        return;
    }

    eeprom_update_block(
        &g_macro_cfg,
        slot_addr,
        used_size
    );

    g_loaded_macro_index = index;

    uprintf(
        "[MACRO] Save OK idx=%u steps=%u "
        "size=%lu addr=0x%04lX\r\n",
        (unsigned int)index,
        (unsigned int)step_count,
        (unsigned long)used_size,
        (unsigned long)(uintptr_t)slot_addr
    );
}

void macro_clear(uint8_t index)
{
    uint8_t *slot_addr;
    static const uint8_t zero_chunk[32] = {0};

    if (index >= MACRO_GROUP_COUNT)
    {
        return;
    }

    slot_addr = EEPROM_MACRO_SLOT_ADDR(index);

    /*
     * 建議清除整個 macro slot，而不是只根據 EEPROM 中的
     * step_count 清除部分內容。
     *
     * 原因：
     * 1. EEPROM 內容可能損壞。
     * 2. step_count 可能不正確。
     * 3. 舊 macro 尾端資料可能殘留。
     */
    for (size_t offset = 0;
         offset < EEPROM_MACRO_SINGLE_SLOT_SIZE;
         offset += sizeof(zero_chunk))
    {
        size_t chunk_size =
            EEPROM_MACRO_SINGLE_SLOT_SIZE - offset;

        if (chunk_size > sizeof(zero_chunk))
        {
            chunk_size = sizeof(zero_chunk);
        }

        eeprom_update_block(
            zero_chunk,
            slot_addr + offset,
            chunk_size
        );
    }

    if (g_loaded_macro_index == index)
    {
        memset(&g_macro_cfg, 0, sizeof(g_macro_cfg));

        /*
         * 如果有無效 index 定義，建議改成該值。
         * 例如：
         *
         * g_loaded_macro_index = MACRO_INVALID_INDEX;
         */
    }
}

void macro_delete(void)
{
    for (uint8_t i = 0U; i < MACRO_GROUP_COUNT; i++)
    {
        macro_clear(i);

        wait_ms(2);

        uprintf(
            "[MACRO][DEL] macro %u cleared\r\n",
            (unsigned int)(i + 1U)
        );
    }
}
