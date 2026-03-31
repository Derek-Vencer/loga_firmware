#include "macro_engine.h"
// #include "cushion96.h"
#include "eeprom.h" // this might work
// #include "bt_protocol.h"
// #include "quantum.h"
#include "uart.h"
#include "print.h"
// #include "sleep_manager.h"
#include <string.h>

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

    bt_protocol_send_keycode(g_whid.mods, g_whid.keys);
    g_whid_last_sent = g_whid;
    whid_wait_one_interval();
}

// -----------------------------
// Keycode -> HID helpers
// -----------------------------

// static bool whid_is_basic_modifier(uint16_t kc) {
//     return (kc == KC_LCTL) || (kc == KC_LSFT) || (kc == KC_LALT) || (kc == KC_LGUI) ||
//            (kc == KC_RCTL) || (kc == KC_RSFT) || (kc == KC_RALT) || (kc == KC_RGUI);
// }

// static uint8_t whid_modifier_bit(uint16_t kc) {
//     switch (kc) {
//         case KC_LCTL: return (1u << 0);
//         case KC_LSFT: return (1u << 1);
//         case KC_LALT: return (1u << 2);
//         case KC_LGUI: return (1u << 3);
//         case KC_RCTL: return (1u << 4);
//         case KC_RSFT: return (1u << 5);
//         case KC_RALT: return (1u << 6);
//         case KC_RGUI: return (1u << 7);
//         default:      return 0;
//     }
// }

__attribute__((weak)) uint8_t keycode_to_hid(uint16_t keycode);

// static uint8_t whid_keycode_to_hid_usage(uint16_t kc) {
//     if (keycode_to_hid) {
//         return keycode_to_hid(kc);
//     }

//     if (kc >= KC_A && kc <= KC_Z) return (uint8_t)(0x04 + (kc - KC_A));
//     if (kc >= KC_1 && kc <= KC_0) return (uint8_t)(0x1E + (kc - KC_1));
//     if (kc == KC_ENTER) return 0x28;
//     if (kc == KC_ESC)   return 0x29;
//     if (kc == KC_BSPC)  return 0x2A;
//     if (kc == KC_TAB)   return 0x2B;
//     if (kc == KC_SPC)   return 0x2C;

//     return 0;
// }

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

bool macro_engine_start(uint8_t macro_index) {
    if (macro_index >= MACRO_GROUP_COUNT) {
        uprintf("[MACRO] start fail: invalid index=%u\n", macro_index);
        return false;
    }

    if (g_loaded_macro_index != macro_index) {
        macro_load_from_eeprom(macro_index);
    }

    macro_config_t *cfg = &g_macro_cfg;

    uprintf("[MACRO] start idx=%u step_count=%u\n", macro_index, cfg->step_count);

    if (cfg->step_count < MACRO_MIN_STEPS || cfg->step_count > MACRO_MAX_STEPS) {
        uprintf("[MACRO] start fail: invalid step_count=%u\n", cfg->step_count);
        return false;
    }

    if (g_me.running) {
        uprintf("[MACRO] already running, stop first\n");
        macro_engine_stop();
    }

    g_me.running = true;
    g_me.macro_index = macro_index;
    g_me.step_index = 0;
    g_me.state = ME_WAIT_DELAY;
    g_me.deadline_ms = 0;

    if (cfg->step_count > 0) {
        uint16_t d = get_effective_delay_ms(&cfg->step[0]);
        set_deadline_from_now(d);
        uprintf("[MACRO] start ok idx=%u first_delay=%u\n", macro_index, d);
    } else {
        uprintf("[MACRO] start fail: step_count=0\n");
        return false;
    }

    return true;
}

void macro_engine_init(void) {
    memset(&g_me, 0, sizeof(g_me));
    g_me.macro_index = 0xFF;

    memset(&g_macro_cfg, 0, sizeof(g_macro_cfg));
    g_loaded_macro_index = 0xFF;

    //macro_eeprom_init_if_needed();
}

void macro_engine_task(void) {
    if (!g_me.running) return;

    macro_config_t *cfg = &g_macro_cfg;

    if (cfg->step_count < MACRO_MIN_STEPS || cfg->step_count > MACRO_MAX_STEPS) {
        uprintf("[MACRO] invalid step_count=%u\n", cfg->step_count);
        macro_engine_stop();
        return;
    }

    if (g_me.step_index >= cfg->step_count) {
        g_me.state = ME_DONE;
    }

    switch (g_me.state) {
        case ME_WAIT_DELAY: {
            uint32_t now = timer_read32();
            int32_t diff = (int32_t)(now - g_me.deadline_ms);

            if (diff < 0) {
                break;
            }

            g_me.state = ME_EXECUTE_STEP;
        } break;

        case ME_EXECUTE_STEP: {
            macro_step_t *st = &cfg->step[g_me.step_index];

            switch (st->action) {
                case MACRO_ACTION_PRESS:
                    // if (st->keycode != KC_NO) {
                    //     press_key(st->keycode);
                    // }
                    break;

                case MACRO_ACTION_RELEASE:
                    // if (st->keycode != KC_NO) {
                    //     release_key(st->keycode);
                    // }
                    break;

                case MACRO_ACTION_NONE:
                default:
                    uprintf("[MACRO] UNKNOWN action=%u\n", st->action);
                    break;
            }

            g_me.step_index++;

            if (g_me.step_index >= cfg->step_count) {
                uprintf("[MACRO] DONE\n");
                g_me.state = ME_DONE;
            } else {
                uint16_t d = get_effective_delay_ms(&cfg->step[g_me.step_index]);
                set_deadline_from_now(d);
                g_me.state = ME_WAIT_DELAY;
            }
        } break;

        case ME_DONE:
            uprintf("[MACRO] reset state\n");
            g_me.running = false;
            g_me.state = ME_IDLE;
            g_me.step_index = 0;
            g_me.macro_index = 0xFF;
            g_me.deadline_ms = 0;
            break;

        default:
            uprintf("[MACRO] invalid state=%u -> stop\n", g_me.state);
            macro_engine_stop();
            break;
    }
}

void macro_load_from_eeprom(uint8_t index) {
    if (index >= MACRO_GROUP_COUNT) return;

    eeprom_read_block(
        &g_macro_cfg,
        (void *)EEPROM_MACRO_SLOT_ADDR(index),
        sizeof(macro_config_t)
    );

    g_loaded_macro_index = index;
}

void macro_save_to_eeprom(uint8_t index) {
    if (index >= MACRO_GROUP_COUNT) return;

    /*
     * Save current RAM cache to the specified EEPROM slot.
     * Caller should ensure the cache already belongs to this macro index.
     */
    if (g_loaded_macro_index != index) {
        g_loaded_macro_index = index;
    }

    eeprom_update_block(
        &g_macro_cfg,
        (void *)EEPROM_MACRO_SLOT_ADDR(index),
        sizeof(macro_config_t)
    );
}

void macro_delete(void) {
    for (uint8_t i = 0; i < MACRO_GROUP_COUNT; i++) {
        macro_config_t *cfg = macro_engine_get_config(i);
        if (cfg == NULL) {
            uprintf("[MACRO][DEL][ERR] cfg is NULL, index=%u\n", i);
            continue;
        }

        // Clear RAM cache
        memset(cfg, 0, sizeof(macro_config_t));
        cfg->step_count = 0;

        // Save cleared data to EEPROM
        macro_save_to_eeprom(i);

        wait_ms(2);

        uprintf("[MACRO][DEL] macro %u cleared\n", i + 1);
    }
}
