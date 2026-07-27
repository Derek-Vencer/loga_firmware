#include <stdint.h>
#include "keyboard.h"
#include "matrix.h"
#include "keymap.h"
#include "host.h"
#include "led.h"
#include "keycode.h"
#include "timer.h"
#include "print.h"
#include "debug.h"
#include "command.h"
#include "util.h"
#include "sendchar.h"
#include "bootmagic.h"
#include "eeconfig.h"
#include "backlight.h"
#include "hook.h"
#ifdef MOUSEKEY_ENABLE
#   include "mousekey.h"
#endif
#ifdef PS2_MOUSE_ENABLE
#   include "ps2_mouse.h"
#endif
#ifdef SERIAL_MOUSE_ENABLE
#include "serial_mouse.h"
#endif
#ifdef ADB_MOUSE_ENABLE
#include "adb.h"
#endif
#ifdef ENCODER_ENABLE
#    include "encoder.h"
#endif

#ifdef MATRIX_HAS_GHOST
static bool has_ghost_in_row(uint8_t row)
{
    matrix_row_t matrix_row = matrix_get_row(row);
    // No ghost exists when less than 2 keys are down on the row
    if (((matrix_row - 1) & matrix_row) == 0)
        return false;

    // Ghost occurs when the row shares column line with other row
    for (uint8_t i=0; i < MATRIX_ROWS; i++) {
        if (i != row && (matrix_get_row(i) & matrix_row))
            return true;
    }
    return false;
}
#endif


static uint32_t last_input_modification_time = 0;
uint32_t        last_input_activity_time(void) {
    return last_input_modification_time;
}
uint32_t last_input_activity_elapsed(void) {
    return timer_elapsed32(last_input_modification_time);
}

static uint32_t last_encoder_modification_time = 0;
uint32_t        last_encoder_activity_time(void) {
    return last_encoder_modification_time;
}
uint32_t last_encoder_activity_elapsed(void) {
    return timer_elapsed32(last_encoder_modification_time);
}
void last_encoder_activity_trigger(void) {
    last_encoder_modification_time = last_input_modification_time = timer_read32();
}

__attribute__ ((weak)) void matrix_setup(void) {}
void keyboard_setup(void)
{
#ifdef EEPROM_DRIVER
    eeprom_driver_init();
#endif    
    matrix_setup();
}

void keyboard_init(void)
{
    timer_init();
    matrix_init();
#ifdef PS2_MOUSE_ENABLE
    ps2_mouse_init();
#endif
#ifdef SERIAL_MOUSE_ENABLE
    serial_mouse_init();
#endif
#ifdef ADB_MOUSE_ENABLE
    adb_mouse_init();
#endif


#ifdef BOOTMAGIC_ENABLE
    bootmagic();
#endif

#ifdef BACKLIGHT_ENABLE
    backlight_init();
#endif
}

/*
 * Do keyboard routine jobs: scan mantrix, light LEDs, ...
 * This is repeatedly called as fast as possible.
 */
void keyboard_task(void)
{
    static uint8_t led_status = 0;
	//chThdSleepMilliseconds(1);
#ifdef ENCODER_ENABLE
    // bool encoders_changed = encoder_read(); // combine into taskTMR to prevent encoder failure when dynamic lighting turns on
    // if (encoders_changed) last_encoder_activity_trigger();
#endif

#ifdef MOUSEKEY_ENABLE
    // mousekey repeat & acceleration
    mousekey_task();
#endif

#ifdef PS2_MOUSE_ENABLE
    ps2_mouse_task();
#endif

#ifdef SERIAL_MOUSE_ENABLE
        serial_mouse_task();
#endif

#ifdef ADB_MOUSE_ENABLE
        adb_mouse_task();
#endif

    // update LED
    if (led_status != host_keyboard_leds()) {
        led_status = host_keyboard_leds();
        if (debug_keyboard) dprintf("LED: %02X\n", led_status);
        hook_keyboard_leds_change(led_status);
    }
    chThdSleepMilliseconds(1);
}

void keyboard_taskTMR(void)
{
    static matrix_row_t matrix_prev[MATRIX_ROWS];
#ifdef MATRIX_HAS_GHOST
    static matrix_row_t matrix_ghost[MATRIX_ROWS];
#endif
    //static uint8_t led_status = 0;
    matrix_row_t matrix_row = 0;
    matrix_row_t matrix_change = 0;

    for (uint8_t r = 0; r < MATRIX_ROWS; r++) {
        matrix_row = matrix_get_row(r);
        matrix_change = matrix_row ^ matrix_prev[r];
        
        if (matrix_change) {
#ifdef MATRIX_HAS_GHOST //
            if (has_ghost_in_row(r)) {
                /* Keep track of whether ghosted status has changed for
                 * debugging. But don't update matrix_prev until un-ghosted, or
                 * the last key would be lost.
                 */
                if (debug_matrix && matrix_ghost[r] != matrix_row) {
                    matrix_print();
                }
                matrix_ghost[r] = matrix_row;
                continue;
            }
            matrix_ghost[r] = matrix_row;
#endif
            if (debug_matrix) matrix_print();
            for (uint8_t c = 0; c < MATRIX_COLS; c++) {
                if (matrix_change & ((matrix_row_t)1<<c)) {
                    keyevent_t e = (keyevent_t){
                        .key = (keypos_t){ .row = r, .col = c },
                        .pressed = (matrix_row & ((matrix_row_t)1<<c)),
                        .time = (timer_read() | 1) /* time should not be 0 */
                    };
                    keyrecord_t record = { .event = e };
                    if (hook_matrix_change(record)){
                    action_exec(e);
                    }
//                    action_exec(e);
//                    hook_matrix_change(e);
                    // record a processed key
                    matrix_prev[r] ^= ((matrix_row_t)1<<c);
                    // process a key per task call
                    goto MATRIX_LOOP_END;
                }
            }
        }
    }
    encoder_read();   // ← 即使沒有按鍵變化，也持續掃描 encoder lyon 2025/10/30
    // call with pseudo tick event when no real key event.
    action_exec(TICK);

MATRIX_LOOP_END:

    hook_keyboard_loop();
}

//void keyboard_task(void)
//{
//    static matrix_row_t matrix_prev[MATRIX_ROWS];
//#ifdef MATRIX_HAS_GHOST
//    static matrix_row_t matrix_ghost[MATRIX_ROWS];
//#endif
//    static uint8_t led_status = 0;
//    matrix_row_t matrix_row = 0;
//    matrix_row_t matrix_change = 0;
//
//    matrix_scan();
//    for (uint8_t r = 0; r < MATRIX_ROWS; r++) {
//        matrix_row = matrix_get_row(r);
//        matrix_change = matrix_row ^ matrix_prev[r];
//
//        if (matrix_change) {
//#ifdef MATRIX_HAS_GHOST
//            if (has_ghost_in_row(r)) {
//                /* Keep track of whether ghosted status has changed for
//                 * debugging. But don't update matrix_prev until un-ghosted, or
//                 * the last key would be lost.
//                 */
//                if (debug_matrix && matrix_ghost[r] != matrix_row) {
//                    matrix_print();
//                }
//                matrix_ghost[r] = matrix_row;
//                continue;
//            }
//            matrix_ghost[r] = matrix_row;
//#endif
//            if (debug_matrix) matrix_print();
//            for (uint8_t c = 0; c < MATRIX_COLS; c++) {
//                if (matrix_change & ((matrix_row_t)1<<c)) {
//                    keyevent_t e = (keyevent_t){
//                        .key = (keypos_t){ .row = r, .col = c },
//                        .pressed = (matrix_row & ((matrix_row_t)1<<c)),
//                        .time = (timer_read() | 1) /* time should not be 0 */
//                    };
//                    action_exec(e);
//                    hook_matrix_change(e);
//                    // record a processed key
//                    matrix_prev[r] ^= ((matrix_row_t)1<<c);
//                    // process a key per task call
//                    goto MATRIX_LOOP_END;
//                }
//            }
//        }
//    }
//    // call with pseudo tick event when no real key event.
//    action_exec(TICK);
//
//MATRIX_LOOP_END:
//
//    hook_keyboard_loop();
//
//#ifdef ENCODER_ENABLE
//    bool encoders_changed = encoder_read();
//    if (encoders_changed) last_encoder_activity_trigger();
//#endif
//
//#ifdef MOUSEKEY_ENABLE
//    // mousekey repeat & acceleration
//    mousekey_task();
//#endif
//
//#ifdef PS2_MOUSE_ENABLE
//    ps2_mouse_task();
//#endif
//
//#ifdef SERIAL_MOUSE_ENABLE
//        serial_mouse_task();
//#endif
//
//#ifdef ADB_MOUSE_ENABLE
//        adb_mouse_task();
//#endif
//
//    // update LED
//    if (led_status != host_keyboard_leds()) {
//        led_status = host_keyboard_leds();
//        if (debug_keyboard) dprintf("LED: %02X\n", led_status);
//        hook_keyboard_leds_change(led_status);
//    }
//}

void keyboard_set_leds(uint8_t leds)
{
    led_set(leds);
}

