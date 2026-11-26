#pragma once
#include <stdint.h>
#include <stdbool.h>

#define FEATURE_MAX 16  // 你可改成 8

typedef enum {
    FEATURE_NONE = 0,
    FEATURE_SOCD,
    FEATURE_MODTAP,
    FEATURE_RAPPYSNAPPY,
    FEATURE_RAPIDTRIGGER,
    FEATURE_TOGGLEKEY,
    FEATURE_DYNKEY,
    FEATURE_BUTTONLOCK,
} feature_type_t;


typedef struct {
    feature_type_t type;
    void *config;      // 指向專屬的設定資料結構
} feature_slot_t;


extern feature_slot_t g_features[FEATURE_MAX];
extern uint8_t g_feature_count;

// API
uint8_t feature_add(feature_type_t type, void *cfg);
bool feature_remove(uint8_t index);
