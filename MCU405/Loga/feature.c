#include "feature.h"
#include <string.h>

feature_slot_t g_features[FEATURE_MAX];
uint8_t g_feature_count = 0;

// 新增功能（自動帶 index）
uint8_t feature_add(feature_type_t type, void *cfg)
{
    if (g_feature_count >= FEATURE_MAX)
        return 0xFF; // No space

    g_features[g_feature_count].type = type;
    g_features[g_feature_count].config = cfg;

    return g_feature_count++;      // 回傳 index
}


// 刪除功能，後面往前移動
bool feature_remove(uint8_t index)
{
    if (index >= g_feature_count){
        return false;
    }

    // 先 free 掉這個 feature 的配置
    if (g_features[index].config){
        free(g_features[index].config);
    }
    
    // 後面全部往前補
    for (uint8_t i = index; i < g_feature_count - 1; i++){
        g_features[i] = g_features[i + 1];
    }

    g_feature_count--;
    return true;
}
