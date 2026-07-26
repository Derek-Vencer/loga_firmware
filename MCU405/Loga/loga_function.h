#ifndef LOGA_FUNCTION_H
#define LOGA_FUNCTION_H

#include <stdint.h>

//lyon 
// #ifdef rappySnappy
typedef struct { // plan to add new struct to replace keypos_t because I need layer 
    uint8_t layer;
    uint8_t row;
    uint8_t col;
} layer_keypos_t;
// #endif

// rappyTrigger
typedef struct {
    bool enabled;
    bool pressed;
    uint8_t row;
    uint8_t col;
    uint16_t keycode;

    float trigger_ratio;
    float release_ratio;
} rappy_fixed_ratio_t;

void rapidTriggerProcess(bool mode, uint8_t key);

#endif // LOGA_FUNCTION_H
