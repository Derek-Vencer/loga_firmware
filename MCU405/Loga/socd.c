/*******************
#include <stdint.h>
#include "socd.h"
#include "led/led_function.h"
#include "matrix.h"
#include "keymap_common.h" // for the keymaps[][][]



static keyevent_t last_left_event = {0}, last_right_event = {0};
static keyevent_t last_up_event = {0}, last_down_event = {0};

extern bool is_direction_key(uint16_t keycode);  // 你自己定義，例如 KC_LEFT, KC_RIGHT, KC_UP, KC_DOWN

extern const analogKey_Config_t matrixAnalogRelocate[MATRIX_ROWS][MATRIX_COLS];
extern adcsample_t samples2[ADC_BUF_DEPTH][ADC_NUM_CHANNELS], initPostionLevel[ADC_BUF_DEPTH][ADC_NUM_CHANNELS], travelPostionLevel[ADC_BUF_DEPTH][ADC_NUM_CHANNELS];

socd_keyrecord_t firstSnap = {
    .type = SOCD_PRIORITY_NEUTRAL,
    .event1 = {
        .key = { .row = 4, .col = 1 },
        .pressed = false,
        .time = 0
    },
    .event2 = {
        .key = { .row = 4, .col = 2 },
        .pressed = false,
        .time = 0
    }
};

// void socd_register_event(keyevent_t event) {
    // switch (event.key.col) {
    //     case LEFT_COL: last_left_event = event; break;
    //     case RIGHT_COL: last_right_event = event; break;
    //     case UP_COL: last_up_event = event; break;
    //     case DOWN_COL: last_down_event = event; break;
    // }
// }

bool socd_register_event(int8_t row, int8_t col) {
    // return (col == firstSnap.event1.key.col && row == firstSnap.event1.key.row)
    //        || (col == firstSnap.event2.key.col && row == firstSnap.event2.key.row);

    uint32_t now = get_system_time_ticks();
    bool hit = false;

    if (row == firstSnap.event1.key.row && col == firstSnap.event1.key.col) {
        firstSnap.event1.time = now;   // 記錄最近一次事件時間
        hit = true;
    }
    if (row == firstSnap.event2.key.row && col == firstSnap.event2.key.col) {
        firstSnap.event2.time = now;   // 記錄最近一次事件時間
        hit = true;
    }
    return hit;
}

void socd_direction(socd_keyrecord_t *report) {

    uint16_t keycode1 = keymaps[0][firstSnap.event1.key.row][firstSnap.event1.key.col];
    uint16_t keycode2 = keymaps[0][firstSnap.event2.key.row][firstSnap.event2.key.col];

    analogKey_Config_t info1 = matrixAnalogRelocate[firstSnap.event1.key.row][firstSnap.event1.key.col];
    analogKey_Config_t info2 = matrixAnalogRelocate[firstSnap.event2.key.row][firstSnap.event2.key.col];

    int adc1 = samples2[info1.colA][info1.rowA] - initPostionLevel[info1.colA][info1.rowA];
    int adc2 = samples2[info2.colA][info2.rowA] - initPostionLevel[info2.colA][info2.rowA];

    int travel1 = travelPostionLevel[info1.colA][info1.rowA] - initPostionLevel[info1.colA][info1.rowA];
    int travel2 = travelPostionLevel[info2.colA][info2.rowA] - initPostionLevel[info2.colA][info2.rowA];

    if (travel1 <= 0) travel1 = 1;
    if (travel2 <= 0) travel2 = 1;

    float ratio1 = (float)adc1 / travel1;
    float ratio2 = (float)adc2 / travel2;

    switch (report->type)
    {
    case SOCD_PRIORITY_NEUTRAL: // 互相抵消，輸出為「無方向」
        if(ratio1 < 0.4f && ratio2 < 0.4f){
            unregister_code(keycode1);
            unregister_code(keycode2);
        } else if (ratio1 > 0.4f && ratio2 > 0.4f){
            unregister_code(keycode1);
            unregister_code(keycode2);
        } else if (ratio1 > 0.4f && ratio2 < 0.4f){
            register_code(keycode1);
            unregister_code(keycode2);
        } else if (ratio1 < 0.4f && ratio2 > 0.4f){
            unregister_code(keycode1);
            register_code(keycode2);
        }
        break;
    case SOCD_PRIORITY_PRIMARY: // 永遠以 primary key 為主
        if(ratio1 < 0.4f && ratio2 < 0.4f){
            unregister_code(keycode1);
            unregister_code(keycode2);
        } else if (ratio1 > 0.4f && ratio2 > 0.4f){
            register_code(keycode1);
            unregister_code(keycode2);
        } else if (ratio1 > 0.4f && ratio2 < 0.4f){
            register_code(keycode1);
            unregister_code(keycode2);
        } else if (ratio1 < 0.4f && ratio2 > 0.4f){
            unregister_code(keycode1);
            register_code(keycode2);
        }
        break;
    case SOCD_PRIORITY_SECONDARY: // 永遠以 secondary key 為主
    if(ratio1 < 0.4f && ratio2 < 0.4f){
            unregister_code(keycode1);
            unregister_code(keycode2);
        } else if (ratio1 > 0.4f && ratio2 > 0.4f){
            unregister_code(keycode1);
            register_code(keycode2);
        } else if (ratio1 > 0.4f && ratio2 < 0.4f){
            register_code(keycode1);
            unregister_code(keycode2);
        } else if (ratio1 < 0.4f && ratio2 > 0.4f){
            unregister_code(keycode1);
            register_code(keycode2);
        }
        break;
    case SOCD_PRIORITY_LAST_INPUT: // 輸出最近按下的那一個方向
        if(ratio1 < 0.4f && ratio2 < 0.4f){
            unregister_code(keycode1);
            unregister_code(keycode2);
        } else if (ratio1 > 0.4f && ratio2 > 0.4f){
            if(firstSnap.event1.time > firstSnap.event2.time){
                register_code(keycode1);
                unregister_code(keycode2);
            }else if (firstSnap.event1.time < firstSnap.event2.time){
                unregister_code(keycode1);
                register_code(keycode2);
            }
        } else if (ratio1 > 0.4f && ratio2 < 0.4f){
            register_code(keycode1);
            unregister_code(keycode2);
        } else if (ratio1 < 0.4f && ratio2 > 0.4f){
            unregister_code(keycode1);
            register_code(keycode2);
        }
        break;
    
    default:
        break;
    }
    return;
}
	


// void socd_direction(socd_keyrecord_t *report) {
//     bool left = last_left_event.pressed;
//     bool right = last_right_event.pressed;

//     if (left && right) {
//         if (last_left_event.time > last_right_event.time)
//             report->x = -127; // left wins
//         else
//             report->x = 127;  // right wins
//     } else if (left) {
//         report->x = -127;
//     } else if (right) {
//         report->x = 127;
//     } else {
//         report->x = 0;
//     }

//     // 同理上下判斷
// }
****************/
// #include "socd.h"

// /* ---- 必要 include，避免 NULL/型別未定義 ---- */
// #include <stddef.h>
// #include <string.h>

// /* ---- ChibiOS ---- */
// #include "ch.h"
// #include "hal.h"

// // socd_config_t cfg;

// // socd_config_t socd_slots[SOCD_SLOT_COUNT]; 

// // typedef struct {
// //     bool a_pressed;
// //     bool b_pressed;
// //     uint32_t a_ts;
// //     uint32_t b_ts;
// // } socd_runtime_t;

// // static socd_runtime_t socd_rt[SOCD_SLOT_COUNT];


// /* ---- 你的 keycode 與輸出 API ---- */
// #include "keycode.h"        // KC_Z, KC_X ...（請確保可見）
// extern void register_code(uint16_t kc);
// extern void unregister_code(uint16_t kc);

// /* ---- 取得毫秒時間：若你已有 get_system_time_ticks()，用它；否則用 ChibiOS 換算 ---- */
// static inline uint32_t socd_now_ms(void) {
// #ifdef HAVE_GET_SYSTEM_TIME_TICKS
//     extern uint32_t get_system_time_ticks(void);
//     return get_system_time_ticks();
// #else
//     return TIME_I2MS(chVTGetSystemTimeX());
// #endif
// }

// /* ====================== 配對設定（先只管 Z(4,1) 與 X(4,2)） ====================== */
// // typedef struct {
// //     uint8_t  row, col;
// //     uint16_t kc;
// //     bool     down;
// //     uint32_t last_press;   /* 只在 pressed==true 時更新，用於 LAST_INPUT */
// // } member_t;

// // typedef struct {
// //     member_t A;            /* 例如 Z */
// //     member_t B;            /* 例如 X */
// //     uint16_t activeKC;     /* 目前實際輸出的 keycode（0 表示無） */
// // } pair_state_t;
// /*************
// typedef struct {
//     uint16_t kc;          // keycode（仍由 keymap 決定，Phase 1 先不動）
//     bool     down;
//     uint32_t last_press;
// } member_rt_t;

// typedef struct {
//     member_rt_t A;
//     member_rt_t B;
//     uint16_t activeKC;
// } pair_state_t;
// *************/
// typedef struct {
//     bool down;
//     uint32_t last_press;
// } socd_key_rt_t;

// typedef struct {
//     socd_key_rt_t A;
//     socd_key_rt_t B;
//     uint16_t activeKC;
// } socd_slot_rt_t;

// static socd_slot_rt_t socd_rt[SOCD_SLOT_COUNT];
// socd_config_t socd_slots[SOCD_SLOT_COUNT];  // RAM mirror


// /* 如果要加方向鍵，照這個格式再加一組即可 */
// // static pair_state_t g_pairs[] = {
// //     /* Z at (4,1), X at (4,2) */ //
// //     { .A = {4,1, KC_Z, false, 0}, .B = {4,2, KC_X, false, 0}, .activeKC = 0 }, // this work but to add the safety mode of the default disable mode
// //     /* 範例：LEFT/RIGHT、UP/DOWN 之後可加：
// //        { .A={rL,cL,KC_LEFT,false,0},  .B={rR,cR,KC_RIGHT,false,0}, .activeKC=0 },
// //        { .A={rU,cU,KC_UP,false,0},    .B={rD,cD,KC_DOWN,false,0},  .activeKC=0 }, */
// // };
// // static const uint8_t PAIR_COUNT = sizeof(g_pairs)/sizeof(g_pairs[0]);

// /* 全域策略（預設 LAST_INPUT） */
// // static socd_policy_t g_policy = SOCD_PRIORITY_LAST_INPUT;
// socd_policy_t g_policy = SOCD_PRIORITY_PRIMARY;//SOCD_PRIORITY_LAST_INPUT; //for via

// /* ====================== 事件佇列（lock-free ring） + semaphore ====================== */
// typedef struct {
//     uint8_t  slot, row, col;
//     bool     pressed;
//     uint32_t time_ms;
// } socd_evt_t;

// #define SOCD_QSIZE 32
// static socd_evt_t q[SOCD_QSIZE];
// static volatile uint8_t qh = 0, qt = 0;
// static binary_semaphore_t semSOCD;

// static inline bool q_full(void){ return (uint8_t)(qh+1)%SOCD_QSIZE == qt; }
// static inline bool q_empty(void){ return qh == qt; }
// static inline void q_push(socd_evt_t e){
//     uint8_t next = (uint8_t)(qh+1) % SOCD_QSIZE;
//     if (next == qt) {        /* 滿了：丟掉最舊一筆，保留最新事件 */
//         qt = (uint8_t)(qt+1) % SOCD_QSIZE;
//     }
//     q[qh] = e; qh = next;
// }
// static inline bool q_pop(socd_evt_t* out){
//     if (q_empty()) return false;
//     *out = q[qt]; qt = (uint8_t)(qt+1) % SOCD_QSIZE; return true;
// }

// /* ====================== 小工具 ====================== */
// // static inline bool is_member(uint8_t row, uint8_t col, const member_t* m) {
// //     return (row == m->row) && (col == m->col);
// // }

// static uint16_t decide_output(const pair_state_t* ps) {
//     const bool A = ps->A.down;
//     const bool B = ps->B.down;

//     switch (g_policy) {
//     case SOCD_PRIORITY_NEUTRAL:
//         if (A && B) return 0;
//         return A ? kcA : (B ? kcB : 0);

//     case SOCD_PRIORITY_PRIMARY:
//         if (A && B) return kcA;   // 同時按，A 優先
//         if (A)     return kcA;    // 只有 A
//         if (B)     return kcB;    // 只有 B
//         return 0;

//     case SOCD_PRIORITY_SECONDARY:
//         if (A && B) return kcB;   // 同時按，B 優先
//         if (B)     return kcB;    // 只有 B
//         if (A)     return kcA;    // 只有 A
//         return 0;

//     case SOCD_PRIORITY_LAST_INPUT:
//     default:
//         if (A && B) {
//             if      (ps->A.last_press > ps->B.last_press) return kcA;
//             else if (ps->B.last_press > ps->A.last_press) return kcB;
//             else    return ps->activeKC;   // 同刻：維持現況避免抖動
//         }
//         if (A) return kcA;
//         if (B) return kcB;
//         return 0;
//     case SOCD_DISABLE:
//         break;//return 0;
//     }
//     /*******
//     case SOCD_PRIORITY_NEUTRAL:
//         if (A && B) return 0;
//         return A ? ps->A.kc : (B ? ps->B.kc : 0);

//     case SOCD_PRIORITY_PRIMARY:
//         if (A && B) return ps->A.kc;   // 同時按，A 優先
//         if (A)     return ps->A.kc;    // 只有 A
//         if (B)     return ps->B.kc;    // 只有 B
//         return 0;

//     case SOCD_PRIORITY_SECONDARY:
//         if (A && B) return ps->B.kc;   // 同時按，B 優先
//         if (B)     return ps->B.kc;    // 只有 B
//         if (A)     return ps->A.kc;    // 只有 A
//         return 0;

//     case SOCD_PRIORITY_LAST_INPUT:
//     default:
//         if (A && B) {
//             if      (ps->A.last_press > ps->B.last_press) return ps->A.kc;
//             else if (ps->B.last_press > ps->A.last_press) return ps->B.kc;
//             else    return ps->activeKC;   // 同刻：維持現況避免抖動
//         }
//         if (A) return ps->A.kc;
//         if (B) return ps->B.kc;
//         return 0;
//     case SOCD_DISABLE:
//         break;//return 0;
//     }
//     *****/
// }

// static void apply_output(pair_state_t* ps, uint16_t wantKC) {
//     if (ps->activeKC == wantKC) return;      /* 無變化，不重送 */
//     if (ps->activeKC) unregister_code(ps->activeKC);
//     ps->activeKC = wantKC;
//     if (ps->activeKC) register_code(ps->activeKC);
// }

// /* ====================== SOCD Thread ====================== */
// static THD_WORKING_AREA(waSOCD, 256);
// static THD_FUNCTION(SOCDThread, arg) {
//     (void)arg;
//     chRegSetThreadName("SOCD");

//     while (true) {
//         /* 先吃完佇列；如果空就睡 */
//         socd_evt_t ev;
//         if (!q_pop(&ev)) {
//             chBSemWait(&semSOCD);
//             continue;
//         }

//         /* 事件派發 */
//         for (uint8_t i=0;i<PAIR_COUNT;i++){
//             pair_state_t* ps = &g_pairs[i];
//             // bool isA = is_member(ev.row, ev.col, &ps->A);
//             // bool isB = !isA && is_member(ev.row, ev.col, &ps->B);

//             bool isA =
//                 (ev.row == cfg.row_a) &&
//                 (ev.col == cfg.col_a);

//             bool isB =
//                 (ev.row == cfg.row_b) &&
//                 (ev.col == cfg.col_b);

//             if (!isA && !isB) continue;

//             if (isA) {
//                 if (ps->A.down != ev.pressed) {
//                     ps->A.down = ev.pressed;
//                     // ps->A.last_press = ev.time_ms;
//                     if (ev.pressed) ps->A.last_press = ev.time_ms; /* 按下才記時間 */
//                 }
//             } else { /* isB */
//                 if (ps->B.down != ev.pressed) {
//                     ps->B.down = ev.pressed;
//                     // ps->B.last_press = ev.time_ms;
//                     if (ev.pressed) ps->B.last_press = ev.time_ms;
//                 }
//             }

//             uint16_t kcA = keymap_get_keycode(
//                 cfg->layer_a, cfg->row_a, cfg->col_a
//             );
//             uint16_t kcB = keymap_get_keycode(
//                 cfg->layer_b, cfg->row_b, cfg->col_b
//             );


//             /* 計算並套用輸出 */
//             uint16_t want = decide_output(ps);
//             apply_output(ps, want);
//             break; /* 一個事件只會命中一個 pair */
//         }
//     }
// }

// static void socd_apply_config(const socd_config_t* in) {
//     // cfg 是你全域的 EEPROM 設定快取
//     cfg = *in;

//     if (!cfg.enable) {
//         g_policy = SOCD_DISABLE;
//         return;
//     }

//     /* Phase 1：暫時用固定 keycode */
//     g_pairs[0].A.kc = KC_Z;
//     g_pairs[0].B.kc = KC_X;


//     g_policy = (socd_policy_t)cfg.mode;
// }

// // static void socd_apply_config(const socd_config_t* cfg) {
// //     /* enable */
// //     if (!cfg->enable) {
// //         g_policy = SOCD_DISABLE;
// //         return;
// //     }

// //     /* 目前只用 pair[0] */
// //     g_pairs[0].A.row = cfg->row_a;
// //     g_pairs[0].A.col = cfg->col_a;
// //     g_pairs[0].B.row = cfg->row_b;
// //     g_pairs[0].B.col = cfg->col_b;

// //     g_policy = (socd_policy_t)cfg->mode;
// // }

// void socd_load_from_eeprom(void) {
//     eeconfig_read_socd(0, &cfg);
//     socd_apply_config(&cfg);
// }

// void eeconfig_socd_clear(void) {
//     socd_config_t empty;
//     memset(&empty, 0, sizeof(empty));

//     for (uint8_t i = 0; i < SOCD_SLOT_COUNT; i++) {
//         eeprom_update_block(
//             &empty,
//             (void*)(EECONFIG_SOCD_ADDR + i * sizeof(socd_config_t)),
//             sizeof(socd_config_t)
//         );
//     }
// }

// void socd_test_eeprom(void){
//     // eeconfig_write_socd(0, &cfg);
//     eeconfig_socd_clear();

//     eeconfig_write_socd(0, &empty);
//     cfg = empty;                 // ★ RAM 同步
//     socd_apply_config(&cfg);     // ★ 立即生效
// }

// /* ====================== Public API ====================== */
// void socd_thread_init(void) {
//     /* 狀態清空與輸出清乾淨 */
//     // for (uint8_t i=0;i<PAIR_COUNT;i++){
//     //     g_pairs[i].A.down = g_pairs[i].B.down = false;
//     //     g_pairs[i].A.last_press = g_pairs[i].B.last_press = 0;
//     //     if (g_pairs[i].activeKC) {
//     //         unregister_code(g_pairs[i].activeKC);
//     //         g_pairs[i].activeKC = 0;
//     //     }
//     // }

//     socd_evt_t ev = pop_event();

//     uint8_t s = ev.slot;
//     socd_slot_rt_t *rt = &socd_rt[s];
//     socd_config_t  *cfg = &socd_slots[s];


//     g_policy = SOCD_DISABLE;

//     chBSemObjectInit(&semSOCD, true);
//     chThdCreateStatic(waSOCD, sizeof(waSOCD), NORMALPRIO-1, SOCDThread, NULL);
//     socd_load_from_eeprom();
// }

// void socd_set_policy(socd_policy_t p) { g_policy = p; }

// // bool socd_claims_pos(uint8_t row, uint8_t col) {
// //     for (uint8_t i=0;i<PAIR_COUNT;i++){
// //         if (is_member(row,col,&g_pairs[i].A) || is_member(row,col,&g_pairs[i].B))
// //             return true;
// //     }
// //     return false;
// // }

// bool socd_claims_pos(uint8_t row, uint8_t col) {
//     if (!cfg.enable) return false;

//     bool isA = (row == cfg.row_a) && (col == cfg.col_a);
//     bool isB = (row == cfg.row_b) && (col == cfg.col_b);

//     return isA || isB;
// }


// void socd_post_event(uint8_t slot, uint8_t row, uint8_t col, bool pressed) {
//     socd_evt_t e = {
//         .slot = slot,
//         .row = row,
//         .col = col,
//         .pressed = pressed,
//         .time_ms = socd_now_ms(),
//     };

//     chSysLock();
//     q_push(e);
//     chBSemSignalI(&semSOCD);
//     chSysUnlock();

//     // socd_evt_t e = { .row=row, .col=col, .pressed=pressed, .time_ms=socd_now_ms() };
//     // chSysLock();
//     // q_push(e);
//     // chBSemSignalI(&semSOCD);
//     // chSysUnlock();
// }

// //get api for ione via
// uint8_t socd_fill_report(uint8_t *buf) {
//     uint8_t *p = buf;
//     *p;// = 0xaa; // index
//     p++;
//     *p;// = 0xa1; // primary layer
//     p++;
//     *p = cfg.row_a; // primary row
//     p++;
//     *p = cfg.col_a; // primary col
//     p++;
//     *p;// = 0xa2; // secondary layer
//     p++;
//     *p = cfg.row_b; // secondary row
//     p++;
//     *p = cfg.col_b; // secondary col
//     p++;
//     *p = cfg.mode; // type of SOCD
//     p++;
//     *p;// = 0xa3;

//     return (uint8_t)(p - buf);
// }
// /*********************
// uint8_t socd_fill_report(uint8_t *buf) {
//     uint8_t *p = buf;

//     // *p++ = 0xaa;              // index
//     // *p++ = 0xa1; *p++ = g_pairs[0].A.row; *p++ = g_pairs[0].A.col;
//     // *p++ = 0xa2; *p++ = g_pairs[0].B.row; *p++ = g_pairs[0].B.col;
//     // *p++ = 0xa3; *p++ = (uint8_t)g_policy;

//     *p;// = 0xaa; // index
//     p++;
//     *p;// = 0xa1; // primary layer
//     p++;
//     *p = g_pairs[0].A.row; // primary row
//     p++;
//     *p = g_pairs[0].A.col; // primary col
//     p++;
//     *p;// = 0xa2; // secondary layer
//     p++;
//     *p = g_pairs[0].B.row; // secondary row
//     p++;
//     *p = g_pairs[0].B.col; // secondary col
//     p++;
//     *p = g_policy; // type of SOCD
//     p++;
//     *p;// = 0xa3;

//     return (uint8_t)(p - buf); // 回傳長度
// }
// ****************/

// //set api for ione via

// void socd_apply_report(const uint8_t *buf) {
//     // if (!buf) return;

//     // const uint8_t *p = buf;

//     // uprintf("buf: %02X %02X %02X %02X %02X %02X %02X %02X\r\n",
//     //         buf[0], buf[1], buf[2], buf[3], buf[4], buf[5], buf[6], buf[7]);

//     // 目前先讀掉 layer，但不使用；未來要啟用 LAYER 時，直接寫回 g_pairs[0].A.layer/B.layer 即可 
//     // uint8_t pri_layer = *p;      // 先保留，暫不使用
//     // p++;
//     // uint8_t pri_row   = *p;
//     // p++;
//     // uint8_t pri_col   = *p;
//     // p++;
//     // uint8_t sec_layer = *p;      // 先保留，暫不使用
//     // p++;
//     // uint8_t sec_row   = *p;
//     // p++;
//     // uint8_t sec_col   = *p;
//     // p++;
//     // uint8_t policy_u8 = *p;

//     // 寫回你現在用的 pair(0) 的位置
//     // g_pairs[0].A.row = pri_row;
//     // g_pairs[0].A.col = pri_col;
//     // g_pairs[0].B.row = sec_row;
//     // g_pairs[0].B.col = sec_col;

//     if (!buf) return;

//     socd_config_t new_cfg = cfg;   // cfg 是全域

//     const uint8_t *p = buf;

//     // uint8_t pri_layer = *p;      /* 先保留，暫不使用 */
//     p++;
//     new_cfg.row_a   = *p;
//     p++;
//     new_cfg.col_a   = *p;
//     p++;
//     uint8_t sec_layer = *p;      /* 先保留，暫不使用 */
//     p++;
//     new_cfg.row_b   = *p;
//     p++;
//     new_cfg.col_b   = *p;
//     p++;
//     new_cfg.mode = *p;
//     new_cfg.enable = 1;

//     // 寫進 EEPROM
//     eeconfig_write_socd(0, &new_cfg);

//     // 更新 RAM
//     cfg = new_cfg;
//     socd_apply_config(&cfg);

//     uprintf("SOCD SAVE: ena=%d A=(%d,%d) B=(%d,%d) mode=%d\r\n",
//         cfg.enable, cfg.row_a, cfg.col_a,
//         cfg.row_b, cfg.col_b, cfg.mode);


//     // uprintf ("pri_row: %d, pri_col: %d, sec_row: %d, sec_col: %d\r\n",pri_row,pri_col,sec_row,sec_col);

//     /* 未來要用 layer 時，這兩行打開即可：
//        g_pairs[0].A.layer = pri_layer;
//        g_pairs[0].B.layer = sec_layer;
//     */

//     /* 更新策略（夾在合法範圍） */
//     // if (policy_u8 <= (uint8_t)SOCD_PRIORITY_NEUTRAL) {
//     //     g_policy = (socd_policy_t)policy_u8;
//     // }
//     if (new_cfg.mode <= (uint8_t)SOCD_PRIORITY_NEUTRAL) {
//         g_policy = (socd_policy_t)new_cfg.mode;
//     }
// }

// void eeconfig_socd_init(void) {
//     socd_config_t empty = {0};

//     for (uint8_t i = 0; i < SOCD_SLOT_COUNT; i++) {
//         eeprom_update_block(
//             &empty,
//             (void*)(EECONFIG_SOCD_ADDR + i * sizeof(socd_config_t)),
//             sizeof(socd_config_t)
//         );
//     }
// }

// void eeconfig_read_socd(uint8_t idx, socd_config_t* out) {
//     eeprom_read_block(
//         out,
//         (void*)(EECONFIG_SOCD_ADDR + idx * sizeof(socd_config_t)),
//         sizeof(socd_config_t)
//     );
// }

// void eeconfig_write_socd(uint8_t idx, const socd_config_t* in) {
//     eeprom_update_block(
//         in,
//         (void*)(EECONFIG_SOCD_ADDR + idx * sizeof(socd_config_t)),
//         sizeof(socd_config_t)
//     );
// }

// int8_t socd_find_slot(uint8_t layer, uint8_t row, uint8_t col) {
//     for (uint8_t i = 0; i < SOCD_SLOT_COUNT; i++) {
//         socd_config_t* c = &socd_slots[i];
//         if (!c->enable) continue;

//         if ((layer == c->layer_a && row == c->row_a && col == c->col_a) ||
//             (layer == c->layer_b && row == c->row_b && col == c->col_b)) {
//             return i;
//         }
//     }
//     return -1;
// }

// void socd_load_all_from_eeprom(void) {
//     for (uint8_t i = 0; i < SOCD_SLOT_COUNT; i++) {
//         eeconfig_read_socd(i, &socd_slots[i]);

//         // 安全防呆
//         if (!socd_slots[i].enable) {
//             socd_slots[i].mode = SOCD_DISABLE;
//         }
//     }
// }

/******************new but I will delete before release code ***********/
// #include "socd.h"
// #include <string.h>
// #include "ch.h"
// #include "hal.h"
// # include "keymap_common.h" // lyon keymap

// /* 你的 keycode 與輸出 API */
// #include "keycode.h"
// extern void register_code(uint16_t kc);
// extern void unregister_code(uint16_t kc);

// /* EEPROM range 你已經有：EECONFIG_SOCD_ADDR + sizeof(socd_config_t)*SOCD_SLOT_COUNT */
// // extern void *EECONFIG_SOCD_ADDR; // 若你不是這種寫法，請改成你現有的 define
// // extern void eeprom_read_block(void *dst, const void *addr, uint32_t n);
// // extern void eeprom_update_block(const void *src, void *addr, uint32_t n);

// /* ===== 時間 ===== */
// static inline uint32_t socd_now_ms(void) {
// #ifdef HAVE_GET_SYSTEM_TIME_TICKS
//     extern uint32_t get_system_time_ticks(void);
//     return get_system_time_ticks();
// #else
//     return TIME_I2MS(chVTGetSystemTimeX());
// #endif
// }

// /* ===== RAM mirror of config + runtime ===== */
// socd_config_t socd_slots[SOCD_SLOT_COUNT];

// typedef struct { bool down; uint32_t last_press; } socd_key_rt_t;
// typedef struct { socd_key_rt_t A, B; uint16_t activeKC; } socd_slot_rt_t;
// static socd_slot_rt_t socd_rt[SOCD_SLOT_COUNT];

// /* ===== Event queue ===== */
// typedef struct {
//     uint8_t slot, row, col;
//     bool pressed;
//     uint32_t time_ms;
// } socd_evt_t;

// #define SOCD_QSIZE 32
// static socd_evt_t q[SOCD_QSIZE];
// static volatile uint8_t qh = 0, qt = 0;
// static binary_semaphore_t semSOCD;

// static inline bool q_empty(void){ return qh == qt; }
// static inline void q_push(socd_evt_t e){
//     uint8_t next = (uint8_t)(qh+1) % SOCD_QSIZE;
//     if (next == qt) { qt = (uint8_t)(qt+1) % SOCD_QSIZE; } // drop oldest
//     q[qh] = e; qh = next;
// }
// static inline bool q_pop(socd_evt_t* out){
//     if (q_empty()) return false;
//     *out = q[qt];
//     qt = (uint8_t)(qt+1) % SOCD_QSIZE;
//     return true;
// }

// /* ===== Apply output ===== */
// static void apply_output_slot(socd_slot_rt_t* rt, uint16_t wantKC) {
//     if (rt->activeKC == wantKC) return;
//     if (rt->activeKC) unregister_code(rt->activeKC);
//     rt->activeKC = wantKC;
//     if (rt->activeKC) register_code(rt->activeKC);
// }

// /* ===== Decide output (per-slot policy) ===== */
// static uint16_t decide_output_slot(const socd_config_t* cfg,
//                                    const socd_slot_rt_t* rt,
//                                    uint16_t kcA,
//                                    uint16_t kcB)
// {
//     bool A = rt->A.down;
//     bool B = rt->B.down;

//     switch ((socd_policy_t)cfg->mode) {
//     case SOCD_DISABLE:
//         return 0;

//     case SOCD_PRIORITY_NEUTRAL:
//         if (A && B) return 0;
//         return A ? kcA : (B ? kcB : 0);

//     case SOCD_PRIORITY_PRIMARY:
//         if (A && B) return kcA;
//         return A ? kcA : (B ? kcB : 0);

//     case SOCD_PRIORITY_SECONDARY:
//         if (A && B) return kcB;
//         return B ? kcB : (A ? kcA : 0);

//     case SOCD_PRIORITY_LAST_INPUT:
//     default:
//         if (A && B) {
//             if      (rt->A.last_press > rt->B.last_press) return kcA;
//             else if (rt->B.last_press > rt->A.last_press) return kcB;
//             else    return rt->activeKC; // tie: keep stable
//         }
//         return A ? kcA : (B ? kcB : 0);
//     }
// }

// /* ===== Thread ===== */
// static THD_WORKING_AREA(waSOCD, 256);
// static THD_FUNCTION(SOCDThread, arg) {
//     (void)arg;
//     chRegSetThreadName("SOCD");

//     while (true) {
//         socd_evt_t ev;
//         if (!q_pop(&ev)) { chBSemWait(&semSOCD); continue; }

//         if (ev.slot >= SOCD_SLOT_COUNT) continue;

//         socd_config_t*  cfg = &socd_slots[ev.slot];
//         socd_slot_rt_t* rt  = &socd_rt[ev.slot];

//         if (!cfg->enable || (socd_policy_t)cfg->mode == SOCD_DISABLE) continue;

//         bool isA = (ev.row == cfg->row_a && ev.col == cfg->col_a);
//         bool isB = (ev.row == cfg->row_b && ev.col == cfg->col_b);
//         if (!isA && !isB) continue;

//         if (isA) {
//             if (rt->A.down != ev.pressed) {
//                 rt->A.down = ev.pressed;
//                 if (ev.pressed) rt->A.last_press = ev.time_ms;
//             }
//         } else {
//             if (rt->B.down != ev.pressed) {
//                 rt->B.down = ev.pressed;
//                 if (ev.pressed) rt->B.last_press = ev.time_ms;
//             }
//         }

//         uint16_t kcA = keymaps[cfg->layer_a][cfg->row_a][cfg->col_a];
//         //keymap_get_keycode(cfg->layer_a, cfg->row_a, cfg->col_a);
//         uint16_t kcB = keymaps[cfg->layer_b][cfg->row_b][cfg->col_b];
//         //keymap_get_keycode(cfg->layer_b, cfg->row_b, cfg->col_b);

//         uint16_t want = decide_output_slot(cfg, rt, kcA, kcB);
//         apply_output_slot(rt, want);
//     }
// }

// /* ===== Public API ===== */
// void socd_thread_init(void) {
//     memset(socd_rt, 0, sizeof(socd_rt));
//     chBSemObjectInit(&semSOCD, true);
//     chThdCreateStatic(waSOCD, sizeof(waSOCD), NORMALPRIO-1, SOCDThread, NULL);
//     socd_load_all_from_eeprom();
// }

// int8_t socd_find_slot(uint8_t layer, uint8_t row, uint8_t col) {
//     for (uint8_t i = 0; i < SOCD_SLOT_COUNT; i++) {
//         socd_config_t* c = &socd_slots[i];
//         if (!c->enable || (socd_policy_t)c->mode == SOCD_DISABLE) continue;

//         if ((layer == c->layer_a && row == c->row_a && col == c->col_a) ||
//             (layer == c->layer_b && row == c->row_b && col == c->col_b)) {
//             return (int8_t)i;
//         }
//     }
//     return -1;
// }

// void socd_post_event(uint8_t slot, uint8_t row, uint8_t col, bool pressed) {
//     socd_evt_t e = {
//         .slot = slot,
//         .row = row,
//         .col = col,
//         .pressed = pressed,
//         .time_ms = socd_now_ms(),
//     };

//     chSysLock();
//     q_push(e);
//     chBSemSignalI(&semSOCD);
//     chSysUnlock();
// }

// /* ===== EEPROM ===== */
// void socd_load_all_from_eeprom(void) {
//     for (uint8_t i = 0; i < SOCD_SLOT_COUNT; i++) {
//         eeconfig_read_socd(i, &socd_slots[i]);

//         // 防呆：非法 mode 直接 disable（避免 0xFF/垃圾值）
//         if (socd_slots[i].mode > SOCD_PRIORITY_LAST_INPUT) {
//             socd_slots[i].enable = 0;
//             socd_slots[i].mode = SOCD_DISABLE;
//         }
//     }
// }

// void eeconfig_socd_clear(void) {
//     socd_config_t empty;
//     memset(&empty, 0, sizeof(empty));

//     for (uint8_t i = 0; i < SOCD_SLOT_COUNT; i++) {
//         eeconfig_write_socd(i, &empty);
//         // RAM 同步 + 解除可能卡住的輸出
//         if (socd_rt[i].activeKC) {
//             unregister_code(socd_rt[i].activeKC);
//         }
//         memset(&socd_rt[i], 0, sizeof(socd_rt[i]));
//         socd_slots[i] = empty;
//     }
// }

// /* 你已有 driver 就用你自己的；這裡給通用版本（若你已有同名函式就刪掉這段） */
// void eeconfig_read_socd(uint8_t idx, socd_config_t* out) {
//     eeprom_read_block(out,
//         (void*)((uintptr_t)EECONFIG_SOCD_ADDR + idx * sizeof(socd_config_t)),
//         sizeof(socd_config_t)
//     );
// }
// void eeconfig_write_socd(uint8_t idx, const socd_config_t* in) {
//     eeprom_update_block(in,
//         (void*)((uintptr_t)EECONFIG_SOCD_ADDR + idx * sizeof(socd_config_t)),
//         sizeof(socd_config_t)
//     );
// }

// /* ===== VIA GET/SET ===== */
// uint8_t socd_fill_report(uint8_t *buf) {
//     uint8_t idx = buf[0];
//     if (idx >= SOCD_SLOT_COUNT) return 0;

//     socd_config_t *c = &socd_slots[idx];
//     uint8_t *p = buf;

//     *p++ = idx;
//     *p++ = c->layer_a;
//     *p++ = c->row_a;
//     *p++ = c->col_a;
//     *p++ = c->layer_b;
//     *p++ = c->row_b;
//     *p++ = c->col_b;
//     *p++ = c->mode;

//     return (uint8_t)(p - buf);
// }

// void socd_apply_report(const uint8_t *buf) {
//     if (!buf) return;

//     uint8_t idx = buf[0];
//     if (idx >= SOCD_SLOT_COUNT) return;

//     socd_config_t new_cfg = socd_slots[idx];

//     new_cfg.enable  = 1;
//     new_cfg.layer_a = buf[1];
//     new_cfg.row_a   = buf[2];
//     new_cfg.col_a   = buf[3];
//     new_cfg.layer_b = buf[4];
//     new_cfg.row_b   = buf[5];
//     new_cfg.col_b   = buf[6];
//     new_cfg.mode    = buf[7];

//     if (new_cfg.mode > SOCD_PRIORITY_LAST_INPUT) {
//         // 非法 policy：直接 disable
//         new_cfg.enable = 0;
//         new_cfg.mode = SOCD_DISABLE;
//     }

//     eeconfig_write_socd(idx, &new_cfg);
//     socd_slots[idx] = new_cfg;

//     // 如果改了 slot，建議把該 slot runtime 清掉避免殘狀態
//     if (socd_rt[idx].activeKC) unregister_code(socd_rt[idx].activeKC);
//     memset(&socd_rt[idx], 0, sizeof(socd_rt[idx]));
// }

#include "socd.h"
#include <string.h>
#include "ch.h"
#include "hal.h"
#include "keycode.h"
#include "keymap_common.h"

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

void socd_apply_config(uint8_t idx, const socd_config_t *c)
{
    socd_slots[idx] = *c;

    // ★ 關鍵：清 runtime 狀態
    memset(&socd_rt[idx], 0, sizeof(socd_rt[idx]));
}

void socd_load_all_from_eeprom(void)
{
    for (uint8_t i = 0; i < SOCD_SLOT_COUNT; i++) {
        // eeconfig_read_socd(i, &socd_slots[i]);
        socd_config_t tmp;
        eeconfig_read_socd(i, &tmp);

        uprintf("EEPROM READ: slot%d ena=%d A=%04X B=%04X mode=%d\r\n",
                i,
                socd_slots[i].enable,
                socd_slots[i].kc_a,
                socd_slots[i].kc_b,
                socd_slots[i].mode);

        /* 基本防呆 */
        // if (!socd_slots[i].enable) {
        //     socd_slots[i].mode = SOCD_DISABLE;
        // }
        if (!tmp.enable) {
            tmp.mode = SOCD_DISABLE;
        }

        socd_apply_config(i, &tmp);
    }

    // ===== TEMP TEST ONLY =====
    // socd_slots[0].enable = 1;
    // socd_slots[0].kc_a   = KC_Z;
    // socd_slots[0].kc_b   = KC_X;
    // socd_slots[0].mode   = SOCD_PRIORITY_LAST_INPUT;

    // socd_slots[1].enable = 1;
    // socd_slots[1].kc_a   = KC_C;
    // socd_slots[1].kc_b   = KC_V;
    // socd_slots[1].mode   = 1;

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
        if (!c->enable) continue;
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
static THD_FUNCTION(SOCDThread, arg) {
    (void)arg;
    while (true) {
        uprintf("SOCD THREAD START\r\n");

        socd_evt_t ev;
        if (!q_pop(&ev)) { chBSemWait(&semSOCD); continue; }
        if (ev.slot >= SOCD_SLOT_COUNT) continue;

        socd_config_t  *cfg = &socd_slots[ev.slot];
        socd_slot_rt_t *rt  = &socd_rt[ev.slot];
        if (!cfg->enable) continue;

        if (ev.keycode == cfg->kc_a) {
            if (rt->A.down != ev.pressed) {
                rt->A.down = ev.pressed;
                if (ev.pressed) rt->A.last_press = ev.time_ms;
            }
        } else if (ev.keycode == cfg->kc_b) {
            if (rt->B.down != ev.pressed) {
                rt->B.down = ev.pressed;
                if (ev.pressed) rt->B.last_press = ev.time_ms;
            }
        } else continue;

        apply_output(rt, decide_output(cfg, rt));
    }
}

/* ===== Public ===== */
void socd_post_event(uint8_t slot, uint16_t keycode, bool pressed) {
    socd_evt_t e = { slot, keycode, pressed, socd_now_ms() };
    chSysLock();
    q_push(e);
    chBSemSignalI(&semSOCD);
    chSysUnlock();
}

void socd_thread_init(void) {
    uprintf("SOCD INIT CALLED...\r\n");
    memset(socd_rt, 0, sizeof(socd_rt));
    chBSemObjectInit(&semSOCD, true);
    chThdCreateStatic(waSOCD, sizeof(waSOCD), NORMALPRIO-1, SOCDThread, NULL);
    socd_load_all_from_eeprom();

    // memset(socd_rt, 0, sizeof(socd_rt));
    // chBSemObjectInit(&semSOCD, true);

    // thread_t *tp = chThdCreateStatic(
    //     waSOCD,
    //     sizeof(waSOCD),
    //     NORMALPRIO + 2,     // ★ 明確給高一點
    //     SOCDThread,
    //     NULL
    // );

    // if (tp == NULL) {
    //     uprintf("SOCD THREAD CREATE FAIL\r\n");
    // }
}

uint8_t socd_fill_report(uint8_t *buf) {
    uint8_t idx = buf[0];
    if (idx >= SOCD_SLOT_COUNT) return 0;

    socd_config_t *c = &socd_slots[idx];
    buf[0] = c->a_layer;
    buf[1] = c->a_row;
    buf[2] = c->a_col;
    buf[3] = c->b_layer;
    buf[4] = c->b_row;
    buf[5] = c->b_col;
    buf[6] = c->mode;
    buf[7] = idx;

    // buf[0] = idx;
    // buf[1] = c->kc_a & 0xFF;
    // buf[2] = c->kc_a >> 8;
    // buf[3] = c->kc_b & 0xFF;
    // buf[4] = c->kc_b >> 8;
    // buf[5] = c->mode;
    return 6;
}

void socd_apply_report(const uint8_t *buf) {
    // uint8_t idx = buf[0];
    // if (idx >= SOCD_SLOT_COUNT) return;

    // socd_config_t c = {
    //     .enable = 1,
    //     .kc_a = (uint16_t)buf[1] | ((uint16_t)buf[2] << 8),
    //     .kc_b = (uint16_t)buf[3] | ((uint16_t)buf[4] << 8),
    //     .mode = buf[5],
    // };
    // eeconfig_write_socd(idx, &c);
    // socd_slots[idx] = c;
    // memset(&socd_rt[idx], 0, sizeof(socd_rt[idx]));
    uint8_t idx = buf[7];
    if (idx >= SOCD_SLOT_COUNT) return;

    uint8_t pri_layer = buf[0];
    uint8_t pri_row   = buf[1];
    uint8_t pri_col   = buf[2];
    uint8_t sec_layer = buf[3];
    uint8_t sec_row   = buf[4];
    uint8_t sec_col   = buf[5];
    uint8_t mode      = buf[6];

    uint16_t kc_a = keymaps[pri_layer][pri_row][pri_col];//keymap_get_keycode(pri_layer, pri_row, pri_col);
    uint16_t kc_b = keymaps[sec_layer][sec_row][sec_col];//keymap_get_keycode(sec_layer, sec_row, sec_col);

    socd_config_t c = {
        .enable = 1,
        .kc_a   = kc_a,
        .kc_b   = kc_b,
        .a_layer = pri_layer,
        .a_row  = pri_row,
        .a_col  = pri_col,
        .b_layer = sec_layer,
        .b_row  = sec_row,
        .b_col  = sec_col,
        .mode   = mode,
    };

    eeconfig_write_socd(idx, &c);
    socd_apply_config(idx, &c);
}

void eeconfig_read_socd(uint8_t idx, socd_config_t* out)
{
    if (idx >= SOCD_SLOT_COUNT) {
        memset(out, 0, sizeof(*out));
        return;
    }

    eeprom_read_block(
        out,
        (const void*)(EECONFIG_SOCD_ADDR + idx * sizeof(socd_config_t)),
        sizeof(socd_config_t)
    );
}

void eeconfig_write_socd(uint8_t idx, const socd_config_t* in)
{
    if (idx >= SOCD_SLOT_COUNT) return;

    eeprom_update_block(
        in,
        (void*)(EECONFIG_SOCD_ADDR + idx * sizeof(socd_config_t)),
        sizeof(socd_config_t)
    );
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
