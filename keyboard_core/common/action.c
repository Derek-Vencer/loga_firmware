/*
Copyright 2012,2013 Jun Wako <wakojun@gmail.com>

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 2 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/
#include "host.h"
#include "keycode.h"
#include "keyboard.h"
#include "mousekey.h"
#include "command.h"
#include "led.h"
#include "backlight.h"
#include "action_layer.h"
#include "action_tapping.h"
#include "action_macro.h"
#include "action_util.h"
#include "action.h"
#include "hook.h"
#include "wait.h"


#ifdef DEBUG_ACTION
#include "debug.h"
#else
#include "nodebug.h"
#endif


void action_exec(keyevent_t event)
{
    if (!IS_NOEVENT(event)) {
        dprintf("\n---- action_exec: start -----\n");
        dprintf("EVENT: "); debug_event(event); dprintln();
        //hook_matrix_change(event);
    }

    keyrecord_t record = { .event = event };

#ifndef NO_ACTION_TAPPING
    action_tapping_process(record);
#else
    process_action(&record);
    if (!IS_NOEVENT(record.event)) {
        dprintf("processed: "); debug_record(record); dprintln();
    }
#endif
}

#if !defined(NO_ACTION_LAYER) && !defined(STRICT_LAYER_RELEASE)
bool disable_action_cache = false;

void process_record_nocache(keyrecord_t *record, bool delayEnable) {
    disable_action_cache = true;
    process_record(record, delayEnable);
    disable_action_cache = false;
}
#else
void process_record_nocache(keyrecord_t *record, bool delayEnable) {
    process_record(record, delayEnable);
}
#endif

void process_action(keyrecord_t *record)
{
	//uint16_t code;
    keyevent_t event = record->event;
#ifndef NO_ACTION_TAPPING
    uint8_t tap_count = record->tap.count;
#endif

    if (IS_NOEVENT(event)) { return; }

    action_t action = layer_switch_get_action(event.key);
    //code = action.key.code;
    //dprint ("action keycode:%X modes:%X kid_id:%X\r\n",code,action.key.mods,action.kind.id);
    dprintf("ACTION: "); debug_action(action);
#ifndef NO_ACTION_LAYER
    dprintf(" layer_state: "); layer_debug();
    dprintf(" default_layer_state: "); default_layer_debug();
#endif
    dprintln();

    switch (action.kind.id) {
        /* Key and Mods */
        case ACT_LMODS:
        case ACT_RMODS:
            {
                uint8_t mods = (action.kind.id == ACT_LMODS) ?  action.key.mods :
                                                                action.key.mods<<4;
                if (event.pressed) {
                    if (mods) {
                        add_weak_mods(mods);
                        send_keyboard_report();
                    }
                    register_code(action.key.code);
                } else {
                    unregister_code(action.key.code);
                    if (mods) {
                        del_weak_mods(mods);
                        send_keyboard_report();
                    }
                }
            }
            break;
#ifndef NO_ACTION_TAPPING
        case ACT_LMODS_TAP:
        case ACT_RMODS_TAP:
            {
                uint8_t mods = (action.kind.id == ACT_LMODS_TAP) ?  action.key.mods :
                                                                    action.key.mods<<4;
                switch (action.layer_tap.code) {
    #ifndef NO_ACTION_ONESHOT
                    case MODS_ONESHOT:
                        // Oneshot modifier
                        if (event.pressed) {
                            if (tap_count == 0) {
                                register_mods(mods);
                            }
                            else if (tap_count == 1) {
                                dprintf("MODS_TAP: Oneshot: start\n");
                                set_oneshot_mods(mods);
                            }
                            else {
                                register_mods(mods);
                            }
                        } else {
                            if (tap_count == 0) {
                                clear_oneshot_mods();
                                unregister_mods(mods);
                            }
                            else if (tap_count == 1) {
                                // Retain Oneshot mods
                            }
                            else {
                                clear_oneshot_mods();
                                unregister_mods(mods);
                            }
                        }
                        break;
    #endif
                    case MODS_TAP_TOGGLE:
                        if (event.pressed) {
                            if (tap_count <= TAPPING_TOGGLE) {
                                if (mods & get_mods()) {
                                    dprintf("MODS_TAP_TOGGLE: toggle mods off\n");
                                    unregister_mods(mods);
                                } else {
                                    dprintf("MODS_TAP_TOGGLE: toggle mods on\n");
                                    register_mods(mods);
                                }
                            }
                        } else {
                            if (tap_count < TAPPING_TOGGLE) {
                                dprintf("MODS_TAP_TOGGLE: release : unregister_mods\n");
                                unregister_mods(mods);
                            }
                        }
                        break;
                    default:
                        if (event.pressed) {
                            if (tap_count > 0) {
                                if (record->tap.interrupted) {
                                    dprintf("MODS_TAP: Tap: Cancel: add_mods\n");
                                    // ad hoc: set 0 to cancel tap
                                    record->tap.count = 0;
                                    register_mods(mods);
                                } else {
                                    dprintf("MODS_TAP: Tap: register_code\n");
                                    register_code(action.key.code);
                                }
                            } else {
                                dprintf("MODS_TAP: No tap: add_mods\n");
                                register_mods(mods);
                            }
                        } else {
                            if (tap_count > 0) {
                                dprintf("MODS_TAP: Tap: unregister_code\n");
                                unregister_code(action.key.code);
                            } else {
                                dprintf("MODS_TAP: No tap: add_mods\n");
                                unregister_mods(mods);
                            }
                        }
                        break;
                }
            }
            break;
#endif
#ifdef EXTRAKEY_ENABLE
        /* other HID usage */

        case ACT_USAGE:

            switch (action.usage.page) {
                case PAGE_SYSTEM:
                    if (event.pressed) {
                        host_system_send(action.usage.code);
                    } else {
                        host_system_send(0);
                    }
                    break;
                case PAGE_CONSUMER:
                    if (event.pressed) {
                        host_consumer_send(action.usage.code);
                    } else {
                        host_consumer_send(0);
                    }
                    break;
            }
            break;
#endif
#ifdef MOUSEKEY_ENABLE
        /* Mouse key */
        case ACT_MOUSEKEY:
            if (event.pressed) {
                mousekey_on(action.key.code);
                mousekey_send();
            } else {
                mousekey_off(action.key.code);
                mousekey_send();
            }
            break;
#endif
#ifndef NO_ACTION_LAYER
        case ACT_LAYER:
            if (action.layer_bitop.on == 0) {
                /* Default Layer Bitwise Operation */
                if (!event.pressed) {
                    uint8_t shift = action.layer_bitop.part*4;
                    uint32_t bits = ((uint32_t)action.layer_bitop.bits)<<shift;
                    uint32_t mask = (action.layer_bitop.xbit) ? ~(((uint32_t)0xf)<<shift) : 0;
                    switch (action.layer_bitop.op) {
                        case OP_BIT_AND: default_layer_and(bits | mask); break;
                        case OP_BIT_OR:  default_layer_or(bits | mask);  break;
                        case OP_BIT_XOR: default_layer_xor(bits | mask); break;
                        case OP_BIT_SET: default_layer_and(mask); default_layer_or(bits); break;
                    }
                }
            } else {
                /* Layer Bitwise Operation */
                if (event.pressed ? (action.layer_bitop.on & ON_PRESS) :
                                    (action.layer_bitop.on & ON_RELEASE)) {
                    uint8_t shift = action.layer_bitop.part*4;
                    uint32_t bits = ((uint32_t)action.layer_bitop.bits)<<shift;
                    uint32_t mask = (action.layer_bitop.xbit) ? ~(((uint32_t)0xf)<<shift) : 0;
                    switch (action.layer_bitop.op) {
                        case OP_BIT_AND: layer_and(bits | mask); break;
                        case OP_BIT_OR:  layer_or(bits | mask);  break;
                        case OP_BIT_XOR: layer_xor(bits | mask); break;
                        case OP_BIT_SET: layer_and(mask); layer_or(bits); break;
                    }
                }
            }
            break;
    #ifndef NO_ACTION_TAPPING
        case ACT_LAYER_TAP:
        case ACT_LAYER_TAP_EXT:
            switch (action.layer_tap.code) {
                case 0xe0 ... 0xef:
                    /* layer On/Off with modifiers(left only) */
                    if (event.pressed) {
                        layer_on(action.layer_tap.val);
                        register_mods(action.layer_tap.code & 0x0f);
                    } else {
                        layer_off(action.layer_tap.val);
                        unregister_mods(action.layer_tap.code & 0x0f);
                    }
                    break;
                case OP_TAP_TOGGLE:
                    /* tap toggle */
                    if (event.pressed) {
                        if (tap_count < TAPPING_TOGGLE) {
                            layer_invert(action.layer_tap.val);
                        }
                    } else {
                        if (tap_count <= TAPPING_TOGGLE) {
                            layer_invert(action.layer_tap.val);
                        }
                    }
                    break;
                case OP_ON_OFF:
                    event.pressed ? layer_on(action.layer_tap.val) :
                                    layer_off(action.layer_tap.val);
                    break;
                case OP_OFF_ON:
                    event.pressed ? layer_off(action.layer_tap.val) :
                                    layer_on(action.layer_tap.val);
                    break;
                case OP_SET_CLEAR:
                    event.pressed ? layer_move(action.layer_tap.val) :
                                    layer_clear();
                    break;
                default:
                    /* tap key */
                    if (event.pressed) {
                        if (tap_count > 0) {
                            dprintf("KEYMAP_TAP_KEY: Tap: register_code\n");
                            register_code(action.layer_tap.code);
                        } else {
                            dprintf("KEYMAP_TAP_KEY: No tap: On on press\n");
                            layer_on(action.layer_tap.val);
                        }
                    } else {
                        if (tap_count > 0) {
                            dprintf("KEYMAP_TAP_KEY: Tap: unregister_code\n");
                            unregister_code(action.layer_tap.code);
                        } else {
                            dprintf("KEYMAP_TAP_KEY: No tap: Off on release\n");
                            layer_off(action.layer_tap.val);
                        }
                    }
                    break;
            }
            break;
    #endif
#endif
        /* Extentions */
#ifndef NO_ACTION_MACRO
        case ACT_MACRO:
            action_macro_play(action_get_macro(record, action.func.id, action.func.opt));
            break;
#endif
#ifdef BACKLIGHT_ENABLE
        case ACT_BACKLIGHT:
            if (!event.pressed) {
                switch (action.backlight.opt) {
                    case BACKLIGHT_INCREASE:
                        backlight_increase();
                        break;
                    case BACKLIGHT_DECREASE:
                        backlight_decrease();
                        break;
                    case BACKLIGHT_TOGGLE:
                        backlight_toggle();
                        break;
                    case BACKLIGHT_STEP:
                        backlight_step();
                        break;
                    case BACKLIGHT_LEVEL:
                        backlight_level(action.backlight.level);
                        break;
                }
            }
            break;
#endif
        case ACT_COMMAND:
            break;
#ifndef NO_ACTION_FUNCTION
        case ACT_FUNCTION:
            action_function(record, action.func.id, action.func.opt);
            break;
#endif
        default:
            break;
    }
}




/*
 * Utilities for actions.
 */
void register_code(uint8_t code)
{
    if (code == KC_NO) {
        return;
    }

#ifdef LOCKING_SUPPORT_ENABLE
    else if (KC_LOCKING_CAPS == code) {
#ifdef LOCKING_RESYNC_ENABLE
        // Resync: ignore if caps lock already is on
        if (host_keyboard_leds() & (1<<USB_LED_CAPS_LOCK)) return;
#endif
        add_key(KC_CAPSLOCK);
        send_keyboard_report();
        del_key(KC_CAPSLOCK);
        send_keyboard_report();
    }

    else if (KC_LOCKING_NUM == code) {
#ifdef LOCKING_RESYNC_ENABLE
        if (host_keyboard_leds() & (1<<USB_LED_NUM_LOCK)) return;
#endif
        add_key(KC_NUMLOCK);
        send_keyboard_report();
        del_key(KC_NUMLOCK);
        send_keyboard_report();
    }

    else if (KC_LOCKING_SCROLL == code) {
#ifdef LOCKING_RESYNC_ENABLE
        if (host_keyboard_leds() & (1<<USB_LED_SCROLL_LOCK)) return;
#endif
        add_key(KC_SCROLLLOCK);
        send_keyboard_report();
        del_key(KC_SCROLLLOCK);
        send_keyboard_report();
    }
#endif

    else if IS_KEY(code) {
        // TODO: should push command_proc out of this block?
        if (command_proc(code)) return;

#ifndef NO_ACTION_ONESHOT
/* TODO: remove
        if (oneshot_state.mods && !oneshot_state.disabled) {
            uint8_t tmp_mods = get_mods();
            add_mods(oneshot_state.mods);

            add_key(code);
            send_keyboard_report();

            set_mods(tmp_mods);
            send_keyboard_report();
            oneshot_cancel();
        } else 
*/
#endif
        {
            add_key(code);
            send_keyboard_report();
        }
    }
    else if IS_MOD(code) {
        add_mods(MOD_BIT(code));
        send_keyboard_report();
    }
    else if IS_SYSTEM(code) {
        host_system_send(KEYCODE2SYSTEM(code));
    }
    else if IS_CONSUMER(code) {
        host_consumer_send(KEYCODE2CONSUMER(code));
    }
    //else if IS_GAMEPAD(code){
        //host_gamepad_send();
    //    uprintf ("reg Gamepad keycode:04X\r\n", code);
    //}
}

void unregister_code(uint8_t code)
{
    if (code == KC_NO) {
        return;
    }

#ifdef LOCKING_SUPPORT_ENABLE
    else if (KC_LOCKING_CAPS == code) {
#ifdef LOCKING_RESYNC_ENABLE
        // Resync: ignore if caps lock already is off
        if (!(host_keyboard_leds() & (1<<USB_LED_CAPS_LOCK))) return;
#endif
        add_key(KC_CAPSLOCK);
        send_keyboard_report();
        del_key(KC_CAPSLOCK);
        send_keyboard_report();
    }

    else if (KC_LOCKING_NUM == code) {
#ifdef LOCKING_RESYNC_ENABLE
        if (!(host_keyboard_leds() & (1<<USB_LED_NUM_LOCK))) return;
#endif
        add_key(KC_NUMLOCK);
        send_keyboard_report();
        del_key(KC_NUMLOCK);
        send_keyboard_report();
    }

    else if (KC_LOCKING_SCROLL == code) {
#ifdef LOCKING_RESYNC_ENABLE
        if (!(host_keyboard_leds() & (1<<USB_LED_SCROLL_LOCK))) return;
#endif
        add_key(KC_SCROLLLOCK);
        send_keyboard_report();
        del_key(KC_SCROLLLOCK);
        send_keyboard_report();
    }
#endif

    else if IS_KEY(code) {
        del_key(code);
        send_keyboard_report();
    }
    else if IS_MOD(code) {
        del_mods(MOD_BIT(code));
        send_keyboard_report();
    }
    else if IS_SYSTEM(code) {
        host_system_send(0);
    }
    else if IS_CONSUMER(code) {
        host_consumer_send(0);
    }
    //else if IS_GAMEPAD(code){
    //    uprintf ("unreg Gamepad keycode:04X\r\n", code);
    //}    
}

uint8_t extract_mod_bits(uint16_t code) {
    switch (code) {
        case QK_MODS ... QK_MODS_MAX:
            break;
        default:
            return 0;
    }

    uint8_t mods_to_send = 0;

    if (code & QK_RMODS_MIN) { // Right mod flag is set
        if (code & QK_LCTL) mods_to_send |= MOD_BIT(KC_RCTRL);
        if (code & QK_LSFT) mods_to_send |= MOD_BIT(KC_RSHIFT);
        if (code & QK_LALT) mods_to_send |= MOD_BIT(KC_RALT);
        if (code & QK_LGUI) mods_to_send |= MOD_BIT(KC_RGUI);
    } else {
        if (code & QK_LCTL) mods_to_send |= MOD_BIT(KC_LCTRL);
        if (code & QK_LSFT) mods_to_send |= MOD_BIT(KC_LSHIFT);
        if (code & QK_LALT) mods_to_send |= MOD_BIT(KC_LALT);
        if (code & QK_LGUI) mods_to_send |= MOD_BIT(KC_LGUI);
    }

    return mods_to_send;
}

void do_code16(uint16_t code, void (*f)(uint8_t)) {
    f(extract_mod_bits(code));
}

__attribute__((weak)) void register_code16(uint16_t code) {
    if (IS_MOD(code) || code == KC_NO) {
        do_code16(code, register_mods);
    } else {
        do_code16(code, register_weak_mods);
    }
    register_code(code);
}

__attribute__((weak)) void unregister_code16(uint16_t code) {
    unregister_code(code);
    if (IS_MOD(code) || code == KC_NO) {
        do_code16(code, unregister_mods);
    } else {
        do_code16(code, unregister_weak_mods);
    }
}

__attribute__((weak)) void tap_code16(uint16_t code) {
    register_code16(code);
    if (code == KC_CAPSLOCK) {
        chThdSleepMilliseconds(TAP_HOLD_CAPS_DELAY);
    } else if (TAP_CODE_DELAY > 0) {
        //wait_ms();
        chThdSleepMilliseconds(TAP_CODE_DELAY);
    }
    //code &= 0xff;
    unregister_code16(code);
}

void register_rapidTrigger_code(uint8_t code)
{
    if (code == KC_NO) {
        return;
    }
    if IS_KEY(code) {
        add_rapidTrigger_key(code);
        send_keyboard_report();
    }    
}

void unregister_rapidTrigger_code(uint8_t code)
{
    if (code == KC_NO) {
        return;
    }
    if IS_KEY(code) {
        del_rapidTrigger_key(code);
        send_keyboard_report();
    }    
}

/** \brief Tap a keycode with a delay.
 *
 * \param code The basic keycode to tap.
 * \param delay The amount of time in milliseconds to leave the keycode registered, before unregistering it.
 */
__attribute__((weak)) void tap_code_delay(uint8_t code, uint16_t delay) {
    register_code(code);
    for (uint16_t i = delay; i > 0; i--) {
        chThdSleepMilliseconds(1);
    }
    unregister_code(code);
}

/** \brief Tap a keycode with the default delay.
 *
 * \param code The basic keycode to tap. If `code` is `KC_CAPS_LOCK`, the delay will be `TAP_HOLD_CAPS_DELAY`, otherwise `TAP_CODE_DELAY`, if defined.
 */
__attribute__((weak)) void tap_code(uint8_t code) {
    tap_code_delay(code, code == KC_CAPSLOCK ? TAP_HOLD_CAPS_DELAY : TAP_CODE_DELAY);
}

void register_mods(uint8_t mods)
{
    if (mods) {
        add_mods(mods);
        send_keyboard_report();
    }
}

void unregister_mods(uint8_t mods)
{
    if (mods) {
        del_mods(mods);
        send_keyboard_report();
    }
}

__attribute__((weak)) void register_weak_mods(uint8_t mods) {
    if (mods) {
        add_weak_mods(mods);
        send_keyboard_report();
    }
}

__attribute__((weak)) void unregister_weak_mods(uint8_t mods) {
    if (mods) {
        del_weak_mods(mods);
        send_keyboard_report();
    }
}

void clear_keyboard(void)
{
    clear_mods();
    clear_keyboard_but_mods();
}

void clear_keyboard_but_mods(void)
{
    clear_weak_mods();
    clear_keys();
    send_keyboard_report();
#ifdef MOUSEKEY_ENABLE
    mousekey_clear();
    mousekey_send();
#endif
#ifdef EXTRAKEY_ENABLE
    host_system_send(0);
    host_consumer_send(0);
#endif
}

bool is_tap_key(keypos_t key)
{
    action_t action = layer_switch_get_action(key);

    switch (action.kind.id) {
        case ACT_LMODS_TAP:
        case ACT_RMODS_TAP:
        case ACT_LAYER_TAP:
        case ACT_LAYER_TAP_EXT:
            switch (action.layer_tap.code) {
                case 0x00 ... 0xdf:
                case OP_TAP_TOGGLE:
                    return true;
            }
            return false;
        case ACT_MACRO:
        case ACT_FUNCTION:
            if (action.func.opt & FUNC_TAP) { return true; }
            return false;
    }
    return false;
}


/*
 * debug print
 */
void debug_event(keyevent_t event)
{
		(void)event;
    dprintf("%04X%c(%u)", (event.key.row<<8 | event.key.col), (event.pressed ? 'd' : 'u'), event.time);
}

void debug_record(keyrecord_t record)
{
    debug_event(record.event);
#ifndef NO_ACTION_TAPPING
    dprintf(":%u%c", record.tap.count, (record.tap.interrupted ? '-' : ' '));
#endif
}

void debug_action(action_t action)
{
    switch (action.kind.id) {
        case ACT_LMODS:             dprintf("ACT_LMODS");             break;
        case ACT_RMODS:             dprintf("ACT_RMODS");             break;
        case ACT_LMODS_TAP:         dprintf("ACT_LMODS_TAP");         break;
        case ACT_RMODS_TAP:         dprintf("ACT_RMODS_TAP");         break;
        case ACT_USAGE:             dprintf("ACT_USAGE");             break;
        case ACT_MOUSEKEY:          dprintf("ACT_MOUSEKEY");          break;
        case ACT_LAYER:             dprintf("ACT_LAYER");             break;
        case ACT_LAYER_TAP:         dprintf("ACT_LAYER_TAP");         break;
        case ACT_LAYER_TAP_EXT:     dprintf("ACT_LAYER_TAP_EXT");     break;
        case ACT_MACRO:             dprintf("ACT_MACRO");             break;
        case ACT_COMMAND:           dprintf("ACT_COMMAND");           break;
        case ACT_FUNCTION:          dprintf("ACT_FUNCTION");          break;
        default:                    dprintf("UNKNOWN");               break;
    }
    dprintf("[%X:%02X]", action.kind.param>>8, action.kind.param&0xff);
}

void process_record(keyrecord_t *record, bool delayEnable) {
    if (IS_NOEVENT(record->event)) {
        return;
    }

//     if (!process_record_quantum(record)) {
// #ifndef NO_ACTION_ONESHOT
//         //if (is_oneshot_layer_active() && record->event.pressed && !keymap_config.oneshot_disable) {
//         if (is_oneshot_layer_active() && record->event.pressed) {
//             clear_oneshot_layer_state(ONESHOT_OTHER_KEY_PRESSED);
//         }
// #endif
//         return;
//     }

    process_record_handler(record);
    if (delayEnable){
        //uprintf ("play delay time:%d\r\n",record->event.time);
        //wait_ms(record->event.time);
        if (record->event.time>10){
            wait_us(record->event.time*100);
        } else {
            switch (record->event.time){
                case 0:
                case 1:
                wait_us(1);
                break;
                default:
                wait_us(record->event.time*100);
            }
        }
        //chThdSleepMilliseconds(record->event.time);    
    }
    //post_process_record_quantum(record);
}

void process_record_handler(keyrecord_t *record) {
#ifdef COMBO_ENABLE
    action_t action;
    if (record->keycode) {
        action = action_for_keycode(record->keycode);
    } else {
        action = store_or_get_action(record->event.pressed, record->event.key);
    }
#else
    action_t action = store_or_get_action(record->event.pressed, record->event.key);
#endif
    dprint("ACTION: ");
    debug_action(action);
#ifndef NO_ACTION_LAYER
    dprint(" layer_state: ");
    layer_debug();
    dprint(" default_layer_state: ");
    default_layer_debug();
#endif
    dprintln();
    process_action(record);
}