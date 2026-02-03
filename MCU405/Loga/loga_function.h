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

    float trigger_ratio;   // 使用者設定觸發百分比 (0.0 ~ 1.0)
    float release_ratio;   // 自動計算（trigger - 0.10）
} rappy_fixed_ratio_t;


#endif // LOGA_FUNCTION_H