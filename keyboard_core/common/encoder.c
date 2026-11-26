/*
 * Copyright 2018 Jack Humbert <jack.humb@gmail.com>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "encoder.h"
#include "wait.h"
// for memcpy
#include <string.h>




#if !defined(ENCODER_RESOLUTIONS) && !defined(ENCODER_RESOLUTION)
#    define ENCODER_RESOLUTION 4
#endif

#if !defined(ENCODERS_PAD_A) || !defined(ENCODERS_PAD_B)
#    error "No encoder pads defined by ENCODERS_PAD_A and ENCODERS_PAD_B"
#endif

#define NUMBER_OF_ENCODERS (sizeof(encoders_pad_a) / sizeof(pin_t))
static pin_t encoders_pad_a[] = ENCODERS_PAD_A;
static pin_t encoders_pad_b[] = ENCODERS_PAD_B;
static pin_t encoders_pad_btn[] = ENCODERS_PAD_BTN;
#ifdef ENCODER_RESOLUTIONS
static uint8_t encoder_resolutions[] = ENCODER_RESOLUTIONS;
#endif

static int8_t encoder_LUT[] = {0, -1, 1, 0, 1, 0, 0, -1, -1, 0, 0, 1, 0, 1, -1, 0};

static uint8_t encoder_state[NUMBER_OF_ENCODERS]  = {0};
static int8_t  encoder_pulses[NUMBER_OF_ENCODERS] = {0};
static bool    btn_status[NUMBER_OF_ENCODERS] = {false};

static uint8_t encoder_value[NUMBER_OF_ENCODERS] = {0};
static uint8_t btn_value[NUMBER_OF_ENCODERS] __attribute__((unused)) = {0};

__attribute__((weak)) void encoder_wait_pullup_charge(void) {
    wait_us(100);
}

__attribute__((weak))
bool encoder_update_user(uint8_t index, uint8_t clockwise)
{
    (void)index;
    (void)clockwise;

    return true;
}

__attribute__((weak)) bool encoder_update_kb(uint8_t index, uint8_t clockwise) {
    return encoder_update_user(index, clockwise);
}

void encoder_init(void) {

    for (uint8_t i = 0; i < NUMBER_OF_ENCODERS; i++) {
        setPinInputHigh(encoders_pad_a[i]);
        setPinInputHigh(encoders_pad_b[i]);
        setPinInputHigh(encoders_pad_btn[i]);
    }
    encoder_wait_pullup_charge();
    for (uint8_t i = 0; i < NUMBER_OF_ENCODERS; i++) {
        encoder_state[i] = (readPin(encoders_pad_a[i]) << 0) | (readPin(encoders_pad_b[i]) << 1);
        btn_status [i] = readPin(encoders_pad_btn[i]);
    }

}

static bool button_update (uint8_t index, bool state){
    if (!state){
        encoder_update_kb(index, ENCODER_BUTTON);
    }
		return true;
}

static bool encoder_update(uint8_t index, uint8_t state) {
    bool    changed = false;
    uint8_t i       = index;

#ifdef ENCODER_RESOLUTIONS
    uint8_t resolution = encoder_resolutions[i];
#else
    uint8_t resolution = ENCODER_RESOLUTION;
#endif

    encoder_pulses[i] += encoder_LUT[state & 0xF];
    if (encoder_pulses[i] >= resolution) {
        encoder_value[index]++;
        changed = true;
        encoder_update_kb(index, ENCODER_COUNTER_CLOCKWISE);
    }
    if (encoder_pulses[i] <= -resolution) { // direction is arbitrary here, but this clockwise
        encoder_value[index]--;
        changed = true;
        encoder_update_kb(index, ENCODER_CLOCKWISE);
    }
    encoder_pulses[i] %= resolution;
#ifdef ENCODER_DEFAULT_POS
    if ((state & 0x3) == ENCODER_DEFAULT_POS) {
        encoder_pulses[i] = 0;
    }
#endif
    return changed;
}

bool encoder_read(void) {
    // uprintf("read1\r\n"); //confirm is here
    bool changed = false;
    bool btnStatus = false;
    for (uint8_t i = 0; i < NUMBER_OF_ENCODERS; i++) {
        encoder_state[i] <<= 2;
        encoder_state[i] |= (readPin(encoders_pad_a[i]) << 0) | (readPin(encoders_pad_b[i]) << 1);
        changed |= encoder_update(i, encoder_state[i]);
        btnStatus = readPin(encoders_pad_btn[i]);
        if (btnStatus != btn_status[i] ){
            button_update(i, btnStatus);
            changed = true;
            btn_status[i] = btnStatus;
            // if (!btnStatus){
            //     palClearPad(GPIOF, 7);
            // } else {
            //     palSetPad(GPIOF, 7);
            // }            
        }
     }
    return changed;
}

