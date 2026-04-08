# include "ch.h"
# include "hal.h"
# include "eeprom.h"
# include "keymap_in_eeprom.h"
# include "keyboard.h"
# include "suspend.h"
# include "encoder.h"
# include "led/led_function.h"
# include "led/led_effect.h"
# include "print.h"
# include "debug.h"
# include "util.h"
# include "matrix.h"
# include "wait.h"
# include "keycode.h"
# include "report.h"
# include "math.h"
# include "action_layer.h"
# include "dynamic_macro.h"
# include "macro_engine.h"
# include "led/main.h"
# include "rappy_snappy.h"

// extern uint16_t keyMatrixDelta[MATRIX_ROWS][MATRIX_COLS];

# include "game_pad.h"
# include "loga_function.h" // lyon for layer_keypos_t
#ifdef socd
# include "socd.h" // socd function
extern socd_policy_t g_policy;
#endif
# include "keymap_common.h" // lyon customize keycode
#define draftShift	0 //
#include "feature.h"
#include "../../keyboard_core/protocol/chibios/usb_main.h"
#define TRAVEL_STEPS 350

//
extern led_runMode_t currentLED_RunMode, lastLED_RunMode;
extern binary_semaphore_t ledUpdateSem;
extern led_config_t LED_Datas[LedNumbers];
extern report_gamepad_report extra_game_report;
extern EffectMosArt_t mosArt; //lyon for changing color keycode
hsv_t hsv;
bool isKEYLOCK = false; // for toggle lock key
bool isCtrl = true;
static bool eeclr_pressed = false; //EECLR
static bool eeclr_fired = false;
static uint32_t eeclr_timer = 0;
// enum custom_keycodes{ //define in keymap_common.h
// 	KC_M1 = SAFE_RANGE,
// };

void send_string(const char *str) {
    while (*str) {
        tap_code16(*str++);
		// tap_code16((uint8_t)*str++);

    }
}

void send_A_to_Z(void) {
    for (uint16_t kc = KC_A; kc <= KC_Z; kc++) {
        tap_code(kc);       // 輸出單一按鍵
        // 如果需要節奏感或避免主機合併事件，可以加一點延遲：
        // chThdSleepMilliseconds(10);
    }
}

static void macro_engine_setup_test_copy_paste(void) {
    macro_config_t *cfg = macro_engine_get_config(0);

    if (cfg == NULL) {
        uprintf("[MACRO] test setup failed: slot 0 unavailable\r\n");
        return;
    }

    memset(cfg, 0, sizeof(*cfg));
    cfg->step_count = 8;

    cfg->step[0] = (macro_step_t){ .delay_type = MACRO_DELAY_FIXED, .delay_ms = 0,   .action = MACRO_ACTION_PRESS,   .keycode = KC_LCTL };
    cfg->step[1] = (macro_step_t){ .delay_type = MACRO_DELAY_FIXED, .delay_ms = 10,  .action = MACRO_ACTION_PRESS,   .keycode = KC_C };
    cfg->step[2] = (macro_step_t){ .delay_type = MACRO_DELAY_FIXED, .delay_ms = 20,  .action = MACRO_ACTION_RELEASE, .keycode = KC_C };
    cfg->step[3] = (macro_step_t){ .delay_type = MACRO_DELAY_FIXED, .delay_ms = 10,  .action = MACRO_ACTION_RELEASE, .keycode = KC_LCTL };
    cfg->step[4] = (macro_step_t){ .delay_type = MACRO_DELAY_FIXED, .delay_ms = 120, .action = MACRO_ACTION_PRESS,   .keycode = KC_LCTL };
    cfg->step[5] = (macro_step_t){ .delay_type = MACRO_DELAY_FIXED, .delay_ms = 10,  .action = MACRO_ACTION_PRESS,   .keycode = KC_V };
    cfg->step[6] = (macro_step_t){ .delay_type = MACRO_DELAY_FIXED, .delay_ms = 20,  .action = MACRO_ACTION_RELEASE, .keycode = KC_V };
    cfg->step[7] = (macro_step_t){ .delay_type = MACRO_DELAY_FIXED, .delay_ms = 10,  .action = MACRO_ACTION_RELEASE, .keycode = KC_LCTL };

    macro_save_to_eeprom(0);
    uprintf("[MACRO] test copy/paste macro loaded to slot 0\r\n");
}

static void macro_engine_dump_eeprom_slot(uint8_t index) {
    macro_config_t cfg = {0};

    if (index >= MACRO_GROUP_COUNT) {
        uprintf("[MACRO][EEPROM] invalid slot=%u\r\n", index);
        return;
    }

    eeprom_read_block(
        &cfg,
        (const void *)EEPROM_MACRO_SLOT_ADDR(index),
        sizeof(cfg)
    );

    uprintf("[MACRO][EEPROM] slot=%u step_count=%u\r\n", index, cfg.step_count);

    if (cfg.step_count > MACRO_MAX_STEPS) {
        uprintf("[MACRO][EEPROM] invalid step_count=%u\r\n", cfg.step_count);
        return;
    }

    for (uint8_t i = 0; i < cfg.step_count; i++) {
        uprintf(
            "[MACRO][EEPROM] step=%u kc=0x%04X delay_type=%u delay_ms=%u action=%u\r\n",
            i,
            cfg.step[i].keycode,
            cfg.step[i].delay_type,
            cfg.step[i].delay_ms,
            cfg.step[i].action
        );
    }
}

// void send_string(const char *str) {
//     while (*str) {
//         host_keyboard_send_char(*str++);
//     }
// }


#define ADC_v5
//#define ADC_Oversample

// #define rappySnappy // lyon move to matrix.h
// #define rappyTrigger
// #define eepromClear
// #define socd

#ifdef ADC_v5
static uint8_t adcStauts = adcSensorPowerOn;
#else
static uint8_t adcStauts = adcIdel;
#endif

binary_semaphore_t adcCompleteSem;
//mutex_t keymatrixProcessMut;
extern binary_semaphore_t gamepadAnalogSem;
extern uint16_t analogX,analogY,analogRx,analogRy,analogZ,analogRz;
static bool ADC_Check = false,ADC_Wait = false;

//test
bool ledOn_off_test = true;

//static uint8_t adcStauts = adcSensorPowerOn;
static uint8_t dly_times;
uint8_t draftValue = 20;

void irq_storm_gpt9_cb(GPTDriver* gptp);
static void muxSelect(uint8_t channel);
extern bool lampArrayDisable;
extern bool lampArrayEffect;
//uint16_t rainbowBase = 118;		//+2
////uint16_t rainbowBase = 145;	//+5
////uint16_t rainbowBase = 218;	//+3
////uint16_t rainbowBase = 290;

//uint16_t rainbowBase = 184;

uint16_t rainbowBase = 77;		//156

bool adcTestToggle = true;
HatSwitch_config_t gamePadHat;
AnalogStickStatus_t gameAnalogStickStatus;
analog_stickValue gameAnalogValue, lastGameAnalogValue;
bool eepromInitCheck = false;

uint8_t currentProfile = 0;
uint8_t defaultLayer = 0;
bool defaultLayerChanged = false;

extern uint16_t eepromTriggerBuffer[MATRIX_ROWS][MATRIX_COLS];

uint8_t knobState;// = 0; // lyon for encoder//
static uint32_t last_layer_state = 0; //keyboard_taskTMR (prooblem ctrl + mo(1))
#ifdef socd
typedef struct {
    bool a_pressed;
    bool b_pressed;
    uint32_t a_ts;
    uint32_t b_ts;
} socd_runtime_t;

static socd_runtime_t socd_rt[SOCD_SLOT_COUNT];

#endif // socd
#ifdef rappyTrigger
//rappy trigger lyon
int16_t adcPre_value[MATRIX_ROWS][MATRIX_COLS];

rappy_fixed_ratio_t rappy_q = {
    .enabled = true,
    .pressed = false,
    .row = 2,
    .col = 1,
    .keycode = KC_Q,
    .trigger_ratio = 0.40f,
    .release_ratio = 0.30f   // trigger - 0.10f
};

bool process_rappy_fixed_ratio(rappy_fixed_ratio_t *rappy);


#endif // rappyTrigger

#ifdef eepromClear
#define RESET_ROW 0
#define RESET_COL 0
#endif // eepromClear

#ifdef buttonLock
bool winLock, altTabLock, altF4Lock = false;
#endif // buttonLock

#ifdef modTap // need socd function

// #define KB_ROWS 6     // ← 改成你的實際 ROW
// #define KB_COLS 15    // ← 改成你的實際 COL

// // 目標鍵與觸發鍵的座標（請改成 A、P 的實際 row/col）
// #define A_ROW 3
// #define A_COL 3
// #define P_ROW 2
// #define P_COL 10

// ===== Mod-Tap minimal core (sparse map) =====
#include "timer.h"
#include "keycode.h"

// 你的矩陣大小（僅拿來做邊界檢查，不參與陣列維度）
#define KB_ROWS 6      // ← 改成你實際 ROW
#define KB_COLS 15     // ← 改成你實際 COL

// 目標鍵座標（請改成 A 與 P 的實際 row/col）
// #define A_ROW 3 // 好像完整版不需要
// #define A_COL 3
// #define P_ROW 2
// #define P_COL 10

// 同時支援的 Mod-Tap 鍵數 / 同時按住的 Mod-Tap 鍵數
#ifndef MT_CAP_MAP
#  define MT_CAP_MAP    8
#endif
#ifndef MT_CAP_ACTIVE
#  define MT_CAP_ACTIVE 4
#endif

// typedef struct { uint8_t L,R,C,hold,click; bool used; } mt_entry_t; // in matrix.h
// static mt_entry_t mt_map[MT_CAP_MAP];
mt_entry_t mt_map[MT_CAP_MAP] = {0}; 
static bool mt_prev[KB_ROWS][KB_COLS];   // 專給 Mod-Tap 邊緣偵測用

static int mt_find(uint8_t L,uint8_t R,uint8_t C){
    for (int i=0;i<MT_CAP_MAP;i++)
        if (mt_map[i].used && mt_map[i].L==L && mt_map[i].R==R && mt_map[i].C==C) return i;
    return -1;
}
static int mt_alloc(void){
    for (int i=0;i<MT_CAP_MAP;i++) if (!mt_map[i].used) return i;
    return 0; // 滿了就覆蓋第 0 筆（簡單策略）
}
static inline void mt_set(uint8_t L,uint8_t R,uint8_t C,uint8_t hold,uint8_t click){
    if (R>=KB_ROWS || C>=KB_COLS) {
		// uprintf("skip set\r\n");
		return;}        // 邊界保護
    if (!hold && !click){ 
		// uprintf("arrive set\r\n");
		int k=mt_find(L,R,C); if (k>=0) mt_map[k].used=false; return; }

	
    int k=mt_find(L,R,C); if (k<0) k=mt_alloc();
	// uprintf("pass set\r\n");
    mt_map[k]=(mt_entry_t){.L=L,.R=R,.C=C,.hold=hold,.click=click,.used=true};
	eeconfig_write_modtap(k, &mt_map[k]);
}
static inline bool mt_active(uint8_t L,uint8_t R,uint8_t C){ return mt_find(L,R,C) >= 0; }

// 目前被按住中的 Mod-Tap（動態狀態）
typedef struct { bool used,down,hold; uint8_t L,R,C; uint16_t t; } mt_state_t;
static mt_state_t mt_active_list[MT_CAP_ACTIVE];

static int mt_state_find(uint8_t L,uint8_t R,uint8_t C){
    for (int i=0;i<MT_CAP_ACTIVE;i++)
        if (mt_active_list[i].used && mt_active_list[i].L==L && mt_active_list[i].R==R && mt_active_list[i].C==C) return i;
    return -1;
}
static int mt_state_alloc(uint8_t L,uint8_t R,uint8_t C){
    int i=mt_state_find(L,R,C);
    if (i<0){ for (int j=0;j<MT_CAP_ACTIVE;j++) if (!mt_active_list[j].used){ i=j; break; } if (i<0) i=0; }
    mt_active_list[i]=(mt_state_t){ .used=true,.down=true,.hold=false,.L=L,.R=R,.C=C,.t=timer_read() };
    return i;
}
static inline void mt_state_clear_idx(int i){ mt_active_list[i].used=false; }

// 事件 API（不依賴 keycode / record；只吃 L/R/C 與 pressed 狀態）
static inline void mt_on_down(uint8_t L,uint8_t R,uint8_t C){
    if (mt_active(L,R,C)) (void)mt_state_alloc(L,R,C);
}
static inline void mt_on_up(uint8_t L,uint8_t R,uint8_t C){
    int s=mt_state_find(L,R,C); if (s<0) return;
    int e=mt_find(L,R,C); if (e<0){ mt_state_clear_idx(s); return; }
    uint8_t hold = mt_map[e].hold, click = mt_map[e].click;
    if (mt_active_list[s].hold) { if (hold) unregister_code(hold); }
    else if (click && timer_elapsed(mt_active_list[s].t) < TAPPING_TERM) tap_code(click);
    mt_state_clear_idx(s);
}

// 每輪掃描尾端呼叫（只處理“正在按住”的少量鍵）
static inline void mt_tick(uint8_t L){
    for (int i=0;i<MT_CAP_ACTIVE;i++){
        if (mt_active_list[i].used && mt_active_list[i].down && !mt_active_list[i].hold){
            int e=mt_find(mt_active_list[i].L, mt_active_list[i].R, mt_active_list[i].C);
            if (e<0){ mt_state_clear_idx(i); continue; }
            uint8_t hold = mt_map[e].hold;
            if (hold && timer_elapsed(mt_active_list[i].t) >= TAPPING_TERM){
                register_code(hold);
                mt_active_list[i].hold = true;
            }
        }
    }
}

// 排程修改：避免在事件回呼內直接改表（防重入/卡死）
static bool    mt_pending=false;
static uint8_t pL,pR,pC,pHold,pClick;
void mt_schedule_set(uint8_t L,uint8_t R,uint8_t C,uint8_t h,uint8_t k){ // 1
    pL=L; pR=R; pC=C; pHold=h; pClick=k; mt_pending=true;
	// uprintf("original %d %d %d %d %d %d\r\n",L, R, C, h, k, mt_pending);
	// uprintf("-- new --%d %d %d %d %d %d\r\n",pL, pR, pC, pHold, pClick, mt_pending);
}
static inline void mt_apply_pending(void){
	// uprintf("in pednding: %d\r\n", mt_pending);
    if (!mt_pending){
		// uprintf("no");
		return;
	} 
    // 只在目標鍵空閒時套用，避免改到正在處理的鍵
    if (mt_state_find(pL,pR,pC) < 0){
		// uprintf("in pednding: %d %d %d\r\n", pL,pR,pC);
        mt_set(pL,pR,pC,pHold,pClick);
        mt_pending=false;
        // 若要偵錯再開：uprintf("[MT] set L%u R%u C%u h=0x%02X k=0x%02X\r\n", pL,pR,pC,pHold,pClick);
    }
}

void eeconfig_write_modtap(uint8_t idx, const mt_entry_t* in)
{
    if (idx >= MT_CAP_MAP) return;

    eeprom_update_block(
        in,
        (void*)(EECONFIG_MODTAP_ADDR + idx * sizeof(mt_entry_t)),
        sizeof(mt_entry_t)
    );
}

void eeconfig_read_modtap(uint8_t idx, socd_config_t* out)
{
    if (idx >= MT_CAP_MAP) {
        memset(out, 0, sizeof(*out));
        return;
    }

    eeprom_read_block(
        out,
        (const void*)(EECONFIG_MODTAP_ADDR + idx * sizeof(mt_entry_t)),
        sizeof(mt_entry_t)
    );
}

void modtap_load_form_eeprom(void)
{
	for(uint8_t i = 0; MT_CAP_MAP > i; i++){
		mt_entry_t *m = &mt_map[i];
		eeconfig_read_modtap(i, m);

		// typedef struct {
		// 	uint8_t L, R, C;
		// 	uint8_t hold, click;
		// 	bool    used;
		// } mt_entry_t;

		// uprintf("EEPROM READ MODTAP: number= %d, layer=%d, row=%d, col=%d, hold=%d, click=%d, used=%d\r\n",
		// 		i,
		// 		m.L,
		// 		m.R,
		// 		m.C,
		// 		m.hold,
		// 		m.click,
		// 		m.used
		// 	);
	}
}

void eeconfig_modtap_clear(void){
	// mt_entry_t *m = &mt_map[i];
	mt_entry_t empty;
    memset(&empty, 0xff, sizeof(empty));

    for (uint8_t i = 0; i < MT_CAP_MAP; i++) {
        eeconfig_write_modtap(i, &empty);

        memset(&mt_map[i], 0xFF, sizeof(mt_map[i]));
        mt_map[i] = empty;
    }

}
#endif // modTap

extern const uint16_t keymaps[][MATRIX_ROWS][MATRIX_COLS]; //lyon for rappy snappy


//uint16_t analogX,analogY,analogRx,analogRy,analogZ,analogRz;

//const int8_t hatSwitchMap[] = {0,-1,1,8,9,1,5,4,6,-1,2,-1,-1,-1,-1,-1};
const int8_t hatSwitchMap[] = {0,1,5,0,7,8,6,0,3,2,4,0,0,0,0,0,0};

//#define nonOP
#ifdef toggleKey
/* matrix state(1:on, 0:off) */
matrix_row_t matrix[MATRIX_ROWS];
static matrix_row_t matrix_temp[MATRIX_ROWS];

// [layer][row][col]
static toggle_key_slot_t toggle_keys[KEYMAPS_COUNT][MATRIX_ROWS][MATRIX_COLS];

// 記錄實體按鍵上一次是否被視為「按下」，用來做 rising edge 判斷
static bool toggle_last_pressed[KEYMAPS_COUNT][MATRIX_ROWS][MATRIX_COLS];


// two api
void toggle_key_set(uint8_t layer, uint8_t row, uint8_t col, uint8_t keycode) {
    if (layer >= KEYMAPS_COUNT || row >= MATRIX_ROWS || col >= MATRIX_COLS) {
        return;
    }

    toggle_key_slot_t *tk = &toggle_keys[layer][row][col];

    if (keycode == 0) {
        tk->enabled = false;
        tk->state   = false;
        tk->keycode = 0;
        tk->layer   = layer;
        tk->row     = row;
        tk->col     = col;

        for (int i = 0; i < toggle_key_pair_count; i++) {
            toggle_key_slot_t tmp = {0};
            eeconfig_read_toggle_entry(i, &tmp);

            if (tmp.enabled == 0x01 &&
                tmp.layer == tk->layer &&
                tmp.row == tk->row &&
                tmp.col == tk->col) {
                eeconfig_clear_toggle_entry(i);
                uprintf("clear toggle eeprom idx=%d\r\n", i);
                break;
            }
        }
    } else {
        tk->enabled = true;
        tk->state   = false;
        tk->keycode = keycode;
        tk->layer   = layer;
        tk->row     = row;
        tk->col     = col;

		uint8_t data_start;
        bool isRepeat = false;

        for (int i = 0; i < toggle_key_pair_count; i++) {
            toggle_key_slot_t tmp = {0};
            eeconfig_read_toggle_entry(i, &tmp);

            if (tmp.enabled == 0x01 &&
                tmp.layer == tk->layer &&
                tmp.row == tk->row &&
                tmp.col == tk->col) {
                isRepeat = true;
                uprintf("isRepeat true idx=%d\r\n", i);

                // 如果你希望重設同位置時更新 keycode，這裡應直接覆寫
                eeconfig_write_toggle_entry(i, tk);
                uprintf("update toggle eeprom idx=%d\r\n", i);
                break;
            }
        }

        if (!isRepeat) {
            for (int i = 0; i < toggle_key_pair_count; i++) {
                toggle_key_slot_t tmp = {0};
                eeconfig_read_toggle_entry(i, &tmp);

                if (tmp.enabled != 0x01) {
                    eeconfig_write_toggle_entry(i, tk);
                    uprintf("write toggle eeprom idx=%d\r\n", i);
                    break;
                }
            }
        }
    }

    toggle_last_pressed[layer][row][col] = false;
}
// void toggle_key_set(uint8_t layer, uint8_t row, uint8_t col, uint8_t keycode) {
//     if (layer >= KEYMAPS_COUNT || row >= MATRIX_ROWS || col >= MATRIX_COLS) {
//         return;
//     }

//     toggle_key_slot_t *tk = &toggle_keys[layer][row][col];

//     if (keycode == 0) {
//         // keycode=0 當作「清除這個 toggle key」
//         tk->enabled = false;
//         tk->state   = false;
//         tk->keycode = 0;
// 		tk->layer   = layer;
// 		tk->row     = row;
// 		tk->col     = col;

// 		for(int i = 0; i < toggle_key_pair_count; i++){
// 			toggle_key_slot_t tmp;
// 			eeconfig_read_toggle_entry(i, &tmp);
// 			if(tmp.enabled == 0x01 && tmp.layer == tk->layer && tmp.row == tk->row && tmp.col == tk->col){
// 				// eeconfig_write_toggle_entry(i, &tk);
// 				eeconfig_clear_toggle_entry(i);
// 				uprintf("clear toggle eeprom\r\n");
// 			}
// 		}
//     } else {
//         tk->enabled = true;
//         tk->state   = false;     // 新設定預設 OFF
//         tk->keycode = keycode;
// 		tk->layer   = layer;
// 		tk->row     = row;
// 		tk->col     = col;

// 		bool isRepeat = false;
// 		for(int i = 0; i < toggle_key_pair_count; i++){
// 			toggle_key_slot_t tmp;
// 			eeconfig_read_toggle_entry(i, &tmp);
// 			if(tmp.layer == tk->layer && tmp.row == tk->row && tmp.col == tk->col){
// 				isRepeat == true;
// 				uprintf("isRepeat true\r\n");
// 			}
// 		}
// 		if(!isRepeat){
// 			for(int i = 0; i < toggle_key_pair_count; i++){
// 				toggle_key_slot_t tmp;
// 				eeconfig_read_toggle_entry(i, &tmp);
// 				if(tmp.enabled != 0x01){
// 					eeconfig_write_toggle_entry(i, &tk);
// 					uprintf("write toggle eeprom\r\n");
// 				}
// 			}
// 		}
		
//     }
// 	// eeconfig_write_toggle_entry(0,tk);

//     // 實體按鍵的狀態也清掉，避免殘留觸發一次
//     toggle_last_pressed[layer][row][col] = false;
// }

uint8_t toggle_key_get(uint8_t layer, uint8_t row, uint8_t col) {
    if (layer >= KEYMAPS_COUNT || row >= MATRIX_ROWS || col >= MATRIX_COLS) {
        return 0;
    }
    return toggle_keys[layer][row][col].keycode;
}

void eeconfig_write_toggle_entry(uint8_t index, const toggle_key_slot_t *entry)
{
    if (index >= toggle_key_pair_count) return;

    uint16_t addr = EECONFIG_TOGGLE_KEY_ADDR + index * sizeof(toggle_key_slot_t);
    eeprom_update_block((const void *)entry, (void *)addr, sizeof(toggle_key_slot_t));
}

void eeconfig_read_toggle_entry(uint8_t index, toggle_key_slot_t *entry)
{
    if (index >= toggle_key_pair_count) return;

    uint16_t addr = EECONFIG_TOGGLE_KEY_ADDR + index * sizeof(toggle_key_slot_t);
    eeprom_read_block((void *)entry, (const void *)addr, sizeof(toggle_key_slot_t));
}

void eeconfig_clear_toggle_entry(uint8_t index)
{
    toggle_key_slot_t empty = {0};
    eeconfig_write_toggle_entry(index, &empty);
}

void eeconfig_clear_all_toggle_entry(){
	for(int i = 0; i < toggle_key_pair_count; i++){
		eeconfig_clear_toggle_entry(i);
		uprintf("clear all toggle eeprom\r\n");
	}
}

void toggle_key_dump_eeprom(void)
{
    uprintf("\r\n===== TOGGLE EEPROM DUMP START =====\r\n");

    for (uint8_t i = 0; i < 10; i++) {   // 你現在用 5，也可以先印多一點
        toggle_key_slot_t tmp = {0};

        eeconfig_read_toggle_entry(i, &tmp);

        uprintf("[IDX %u] en=%u L=%u R=%u C=%u KC=0x%04X\r\n",
                i,
                tmp.enabled,
                tmp.layer,
                tmp.row,
                tmp.col,
                tmp.keycode);
    }

    uprintf("===== TOGGLE EEPROM DUMP END =====\r\n\r\n");
}

// int8_t toggle_key_find_empty_slot(void)
// {
//     toggle_key_slot_t temp;

//     for (uint8_t i = 0; i < MAX_TOGGLE_KEYS; i++) {
//         eeconfig_read_toggle_entry(i, &temp);
//         if (temp.valid == 0) {
//             return i;
//         }
//     }
//     return -1;
// }

// bool toggle_key_add(uint8_t layer, uint8_t row, uint8_t col,
//                     uint8_t binding_count, uint16_t *kc,
//                     uint8_t key_press, uint8_t key_release)
// {
//     int8_t slot = toggle_key_find_empty_slot();
//     if (slot < 0) return false;

//     toggle_key_entry_t entry = {0};
//     entry.valid = 1;
//     entry.layer = layer;
//     entry.row = row;
//     entry.col = col;
//     entry.binding_count = binding_count;
//     entry.key_press = key_press;
//     entry.key_release = key_release;

//     for (uint8_t i = 0; i < 4; i++) {
//         entry.keycode[i] = (i < binding_count) ? kc[i] : 0;
//     }

//     eeconfig_write_toggle_entry(slot, &entry);
//     return true;
// }

// int8_t toggle_key_find_by_pos(uint8_t layer, uint8_t row, uint8_t col)
// {
//     toggle_key_entry_t temp;

//     for (uint8_t i = 0; i < MAX_TOGGLE_KEYS; i++) {
//         eeconfig_read_toggle_entry(i, &temp);
//         if (temp.valid &&
//             temp.layer == layer &&
//             temp.row == row &&
//             temp.col == col) {
//             return i;
//         }
//     }
//     return -1;
// }

// int8_t toggle_key_find_by_pos(uint8_t layer, uint8_t row, uint8_t col)
// {
//     toggle_key_entry_t temp;

//     for (uint8_t i = 0; i < MAX_TOGGLE_KEYS; i++) {
//         eeconfig_read_toggle_entry(i, &temp);
//         if (temp.valid &&
//             temp.layer == layer &&
//             temp.row == row &&
//             temp.col == col) {
//             return i;
//         }
//     }
//     return -1;
// }

// bool toggle_key_remove(uint8_t layer, uint8_t row, uint8_t col)
// {
//     int8_t slot = toggle_key_find_by_pos(layer, row, col);
//     if (slot < 0) return false;

//     eeconfig_clear_toggle_entry(slot);
//     return true;
// }

#endif // toggleKey

#define ADC_Debug

#define ADCTestHispeed

//#define keycodeOutput

#define ADC_LevelOutput              //Check Level

//#define debug_matrix

//#define ADC_Monitor
//

#define Noise_Find

#define rapidShiftP 		15

#define rapidShiftM 		-15

extern void LED_init(void);
extern int uprintf(const char* fmt, ...);
//extern bool setRunMode(uint8_t runMode, int8_t mode, uint8_t brightness, uint8_t speed);
void send_lamp_array(report_lamp_array_t* report);
void adcOffsetGet(bool findMax);
extern void LED_SpeedChange(uint8_t brightness, uint8_t speed);
uint32_t testCB = 0,testADC = 0;


# ifndef DEBOUNCE
#define DEBOUNCE 5
#endif
//static uint8_t debouncing = DEBOUNCE;

/* matrix state(1:on, 0:off) */
matrix_row_t matrix[MATRIX_ROWS];
static matrix_row_t matrix_temp[MATRIX_ROWS];
static bool debouncing = false;
static uint16_t debouncing_time = 0;
uint16_t currentMatrix_CRC, lastMatrix_CRC;

rapidTriggerKey_config_t rapidTriggerKey[RAPID_TRIGGER_COUNT];

int16_t keyMatrixTriggerLevel = 50, keyMatrixReleaseLevel; // move to matrix.h for using in via.c //

uint8_t currentKeyMatrixMode = keyMatrixMode_Normal;
//uint8_t currentKeyMatrixMode = keyMatrixMode_TravelPostionCalc;

analogKey_Config_t keyLocation;

bool keycodeOutput = true;
bool rapidKeyEnable = false, lastRapidKeyEnable;
bool gameModeEnable = false;
bool keyScanEffect = false, lastKeyScanEffect;
bool globleTrigger = false;

extern bool ledUpdatingLock;
bool ledUpdateFilter = true;

bool lastkeycodeOutput;


const analogKey_Config_t analogKeyRelocate[7][16] = {
    { { 0, 0},{ 1, 0},{ 2, 0},{ 3, 0},{ 4, 0},{ 5, 0},{ 0, 1},{ 1, 1},{ 2, 1},{ 3, 1},{ 5, 1},{ 1, 2},{ 2, 2},{ 3, 2},{ 4, 1},{ 5, 2} },
	{ { 0, 2},{ 1, 3},{ 2, 3},{ 3, 3},{ 4, 2},{ 0, 3},{ 1, 4},{ 2, 4},{ 3, 4},{ 4, 3},{ 0, 4},{ 1, 5},{ 2, 5},{ 3, 5},{ 4, 4},{-1,-1} },
	{ { 0, 5},{ 1, 6},{ 2, 6},{ 3, 6},{ 4, 5},{ 5, 3},{ 0, 6},{ 1, 7},{ 2, 7},{ 3, 7},{ 4, 6},{ 0, 7},{ 1, 8},{ 2, 8},{ 3, 8},{ 4, 7} },
	{ { 0, 8},{ 1, 9},{ 2, 9},{ 3, 9},{ 4, 8},{ 0, 9},{ 1,10},{ 2,10},{ 3,10},{ 4, 9},{ 5, 4},{ 0,10},{ 1,11},{ 2,11},{ 4,10},{ 5, 5} },
	{ { 0,11},{ 1,12},{ 2,12},{ 3,11},{ 0,12},{ 1,13},{ 2,13},{ 3,12},{ 4,11},{ 5, 6},{ 0,13},{ 4,12},{ 5, 9},{ 5,10},{-1,-1},{-1,-1} },
	{ { 5, 7},{ 4,12},{ 5, 8},{ 5, 9},{ 1,14},{ 2,14},{ 3,13},{-1,-1},{-1,-1},{-1,-1},{-1,-1},{-1,-1},{-1,-1},{-1,-1},{-1,-1},{-1,-1} },
	{ {-1,-1},{-1,-1},{-1,-1},{-1,-1},{-1,-1},{-1,-1},{-1,-1},{-1,-1},{-1,-1},{-1,-1},{-1,-1},{-1,-1},{-1,-1},{-1,-1},{-1,-1},{-1,-1} },
};

const analogKey_Config_t matrixAnalogRelocate[MATRIX_ROWS][MATRIX_COLS] = {

		{ { 0, 0},{ 0, 6},{ 1, 0},{ 1, 5},{ 1,10},{ 2, 0},{ 2, 6},{ 2,11},{ 3, 0},{ 3, 5},{ 3,11},{ 4, 0},{ 4, 4},{ 4,10},{-1,-1},{-1,-1},{-1,-1}},						//16
		{ { 0, 1},{ 0, 7},{ 0,11},{ 1, 1},{ 1, 6},{ 1,11},{ 2, 1},{ 2, 7},{ 2,12},{ 3, 1},{ 3, 6},{ 3,12},{ 4, 1},{ 4, 5},{ 5, 4},{-1,-1},{-1,-1}},						//17
		{ { 0, 2},{ 0, 8},{ 0,12},{ 1, 2},{ 1, 7},{ 1,12},{ 2, 2},{ 2, 8},{ 2,13},{ 3, 2},{ 3, 7},{ 3,13},{ 4, 2},{ 4, 6},{ 6, 5},{-1,-1},{-1,-1}},						//17
		{ { 0, 3},{ 0, 9},{ 0,13},{ 1, 3},{ 1, 8},{ 1,13},{ 2, 3},{ 2, 9},{ 2,14},{ 3, 3},{ 3, 8},{ 4, 3},{ 4, 7},{ 6, 6},{-1,-1},{-1,-1},{-1,-1}},						//13
		{ { 0, 4},{ 0,14},{ 1, 4},{ 1, 9},{ 1,14},{ 2, 4},{ 2,10},{ 2,15},{ 3, 4},{ 3, 9},{ 3,14},{ 4, 8},{ 6, 1},{-1,-1},{-1,-1},{-1,-1},{-1,-1}},						//13
		{ { 0, 5},{ 0,10},{ 0,15},{ 2, 5},{ 3,10},{ 3,15},{ 4, 9},{ 5, 0},{ 5, 2},{ 5, 3},{-1,-1},{-1,-1},{-1,-1},{-1,-1},{-1,-1},{-1,-1},{-1,-1}},						//11
};


const uint8_t colMax[] = { 14, 15, 15, 14, 13, 10 };



inline uint8_t matrix_rows(void) {
    return MATRIX_ROWS;
}

inline uint8_t matrix_cols(void) {
    return MATRIX_COLS;
}

#define LED_ON()    do { palClearPad(GPIOD, 8) ;} while (0)
#define LED_OFF()   do { palSetPad(GPIOD, 8); } while (0)
#define LED_TGL()   do { palTogglePad(GPIOD, 8); } while (0)

/********** move to matrix.h
#define TriggerMax	1500
#define TriggerMin  3
#define TriggerInitial DEFAULT_TRIGGER_LEVEL

#define ReleaseMax	100
#define ReleaseMin	DEFAULT_RELEASE_LEVEL
#define ReleaseInitial 5
**********/
void led_ctrl(bool sw){
	if (!sw){
		palClearLine(SPI_LED_SDB_PIN);
		//uprintf ("led shutdown on\r\n");
	} else {
		palSetLine(SPI_LED_SDB_PIN);
		//uprintf ("led shutdown off\r\n");
	}
}

#ifdef socd
// extern socd_keyrecord_t firstSnap; // no need?
// extern SOCDPair_t; old dont need ?
bool isSOCD = true; // for toggle key



// extern socd_keyrecord_t firstSnap; //before error
//  /**** change becuase of add mutiple case
static inline void on_key_changed(uint8_t row, uint8_t col, bool pressed) {
	

    uint8_t layer = get_highest_layer(layer_state);

    uint16_t keycode = keymaps[layer][row][col];
	uprintf("KEY: kc=%04X pressed=%d\r\n", keycode, pressed);
    int8_t slot = socd_find_slot_by_keycode(keycode);
	uprintf("KEY: kc=%04X pressed=%d\r\n", keycode, pressed);

    if (slot >= 0) {
        socd_post_event(slot, keycode, pressed);
        return;   // ★ SOCD 接管，跳過原本流程
    }
}
// *********/
// 放在 matrix.c 檔案頂端「函式外」或其他合適區域
static inline bool calc_pressed_now(uint8_t row, uint8_t col, int32_t adcCalc) {

    // 先取「上一輪的狀態」當作中間帶的預設
    bool prev = ( (matrix_temp[row] >> col) & 0x01 ) != 0;

    if (globleTrigger) {
        if (adcCalc > (keyMatrixTriggerLevel + draftValue)) return true;   // 觸發
        if (adcCalc <= keyMatrixReleaseLevel)               return false;  // 釋放
        return prev;  // 落在中間帶，維持上次狀態
    } else {
        // extern int32_t eeconfig_read_Trigger(uint8_t r, uint8_t c);
        // extern int32_t eeconfig_read_Release(uint8_t r, uint8_t c);
        if (adcCalc > (eeconfig_read_Trigger(row, col) + draftValue)) return true;
        if (adcCalc <= eeconfig_read_Release(row, col))               return false;
        return prev;
    }
}
#endif // socd
// #define FORCE_HARDCODE_RT_PROFILE // dynamic keystroke

bool isMac = false;
static hsv_t current_hsv;
extern led_config_t c;

void hook_early_init(void)
{
    int16_t i;

    palSetPadMode(GPIOC, 13, PAL_MODE_OUTPUT_PUSHPULL);				//Analog_PWR
//    palSetPad(GPIOC, 13);
    palClearPad(GPIOC, 13);


    i2c_init();
    uart_init();
	osalThreadSleepMilliseconds(1000); // led delay add in here work
    uprintf("\033[2J\033[HiOne M18 Start .....\r\n");
    uprintf("KeyMap Size:%d %X END:%d %X\r\n",KEYMAP_SIZE,KEYMAP_SIZE,EECONFIG_END,EECONFIG_END);

    //Keyboard LEDs
    palSetPadMode(GPIOB, 3, PAL_MODE_OUTPUT_OPENDRAIN);				//Scroll Lock
    palSetPadMode(GPIOB, 4, PAL_MODE_OUTPUT_OPENDRAIN);				//Caps Lock



    palClearPad(GPIOB, 3);
    palClearPad(GPIOB, 4);

    //ADC Port GPIO Input mode
    setPinInputLow(C0);
    setPinInputLow(C1);
    setPinInputLow(C2);
    setPinInputLow(C3);

    setPinInputLow(A0);
    setPinInputLow(A1);
    setPinInputLow(A2);

    palClearPad(GPIOC, 0);
    palClearPad(GPIOC, 1);
    palClearPad(GPIOC, 2);
    palClearPad(GPIOC, 3);
    palClearPad(GPIOA, 0);
    palClearPad(GPIOA, 1);
    palClearPad(GPIOA, 2);


    uprintf ("check keymap status:%d\r\n",check_keymap_in_eeprom());
    eepromInitCheck = keymap_in_eeprom_init();

    dynamic_macro_load_eeprom_all();

	// Trigger level
	keyMatrixTriggerLevel = eeprom_read_word(EECONFIG_KEYMAP_TRIGGER_LEVEL);
	if (keyMatrixTriggerLevel>TriggerMax || keyMatrixTriggerLevel<TriggerMin){
		keyMatrixTriggerLevel = TriggerInitial;
		eeprom_write_word(EECONFIG_KEYMAP_TRIGGER_LEVEL, keyMatrixTriggerLevel);
	}

	//Release level
	keyMatrixReleaseLevel = eeprom_read_word(EECONFIG_KEYMAP_RELEASE_LEVEL);
	if (keyMatrixReleaseLevel>ReleaseMax || keyMatrixReleaseLevel<ReleaseMin || keyMatrixReleaseLevel>=keyMatrixTriggerLevel){
		keyMatrixReleaseLevel = ReleaseInitial;
		eeprom_write_word(EECONFIG_KEYMAP_RELEASE_LEVEL, keyMatrixReleaseLevel);
	}

	eeconfig_keyBuffer_init();

    //Encoder
    palSetPadMode(GPIOD, 2, PAL_MODE_INPUT_PULLUP);
    palSetPadMode(GPIOC, 12, PAL_MODE_INPUT_PULLUP);
	// palSetPadMode(GPIOB, 13, PAL_MODE_INPUT_PULLUP);
    palSetPadMode(GPIOA, 15, PAL_MODE_INPUT_PULLUP);

	// palSetPadMode(GPIOC, 12, PAL_MODE_OUTPUT_PUSHPULL);
	// palSetPadMode(GPIOB, 3, PAL_MODE_OUTPUT_PUSHPULL);
	// palClearPad(GPIOC, 12); // 預設 LOW

    memset(matrix, 0, MATRIX_ROWS);
//    ledAllOff(true);

	// sensorPowerCtrl(true);
	#ifdef socd
	// void socd_load_from_eeprom(void) {
	// 	for (uint8_t i = 0; i < SOCD_SLOT_COUNT; i++) {
	// 		eeconfig_read_socd(i, &socd_slots[i]);
	// 	}
	// }
	void socd_load_all_from_eeprom(void);

	#endif
}

void AdcPortChange(bool mode){
//	adcStop(&ADCD1);
	if (mode == true){
		setPinOutputOpenDrain(C0);
		setPinOutputOpenDrain(C1);
		setPinOutputOpenDrain(C2);
		setPinOutputOpenDrain(C3);
		setPinOutputOpenDrain(A0);
		setPinOutputOpenDrain(A1);
		setPinOutputOpenDrain(A2);
	} else {
		palSetPadMode(GPIOC, 0, PAL_MODE_INPUT_ANALOG);
		palSetPadMode(GPIOC, 1, PAL_MODE_INPUT_ANALOG);
		palSetPadMode(GPIOC, 2, PAL_MODE_INPUT_ANALOG);
		palSetPadMode(GPIOC, 3, PAL_MODE_INPUT_ANALOG);
		palSetPadMode(GPIOA, 0, PAL_MODE_INPUT_ANALOG);
		palSetPadMode(GPIOA, 1, PAL_MODE_INPUT_ANALOG);
		palSetPadMode(GPIOA, 2, PAL_MODE_INPUT_ANALOG);
	}
//	adcStart(&ADCD1, NULL);
}

void sensorPowerCtrl(bool enable){
	if (enable){
	    palSetPad(GPIOC, 13);
	} else
	{
	    palClearPad(GPIOC, 13);
	}
}

void hook_late_init()
{
    palSetPad(GPIOB, 3);
    palSetPad(GPIOB, 4);

    macro_engine_init();
    // macro_engine_setup_test_copy_paste();

	#ifdef toggleKey
	// test code
	// toggle_key_set(0, 2, 2, 0x04);   // 0x04 = KC_A
	// toggle_key_set(0, 2, 2, 0x00);   // 0x04 = KC_A
	#endif // toggleKey



	uint8_t FACTORY_INIT_MAGIC = 0xF2;
    uint8_t flag = eeprom_read_byte((uint8_t *)EEPROM_FACTORY_INIT_FLAG_ADDR);
	usb_set_polling_rate(eeprom_read_byte(EECONFIG_POLLING_RATE));

    // uprintf("=============factory init flag = %u\n", flag);

    if (flag != FACTORY_INIT_MAGIC) {
		eeprom_write_word(EEPROM_FACTORY_INIT_FLAG_ADDR, 0xF2);
		eeprom_write_word(EECONFIG_KEYMAP_TRIGGER_LEVEL, 0x00C8);
	    eeprom_write_word(EECONFIG_KEYMAP_RELEASE_LEVEL, 0x0000);
		// eeprom_write_word(EECONFIG_KEYMAP_TRIGGER_LEVEL, keyMatrixTriggerLevel);
	    // eeprom_write_word(EECONFIG_KEYMAP_RELEASE_LEVEL, keyMatrixReleaseLevel);
		rapidKeyEnable = eeprom_read_byte(RAPID_KEY_ENABLE_ADDR);
		eeprom_update_byte(KB_COLOR_MODE_ADDR, 8);
		eeprom_write_word(EECONFIG_POLLING_RATE, 0x00);
		eeprom_write_word(EECONFIG_MAC_MODE, 0x00);
		eeconfig_clear_all_toggle_entry();
		
	}
	load_all_rappy_snappy();
	modtap_load_form_eeprom();
	if(eeprom_read_word(IS_SOCD_ON_ADD) == 0x01){
		isSOCD = true;
	} else {
		isSOCD = false;
	}
	isMac = eeprom_read_word(EECONFIG_MAC_MODE);

	void toggle_key_load_from_eeprom(void)//toggle_key_pair_count
	{
		for (uint8_t i = 0; i < toggle_key_pair_count; i++) {
			toggle_key_slot_t tmp;
			eeconfig_read_toggle_entry(i, &tmp);

			// toggle_key_slot_t toggle_keys[MAX_TOGGLE_KEYS];
			if(tmp.enabled == 0x01 && tmp.layer <= LAYERS_PER_PROFILE && tmp.row <= MATRIX_ROWS && tmp.col <= MATRIX_COLS){
				toggle_key_slot_t *tka = &toggle_keys[tmp.layer][tmp.row][tmp.col];
				tka->enabled = true;
				tka->state   = false;     // 新設定預設 OFF
				tka->keycode = tmp.keycode;
				tka->layer   = tmp.layer;
				tka->row     = tmp.row;
				tka->col     = tmp.col;
				uprintf("load toggle eeprom\r\n");
			}
		}
	}
	toggle_key_load_from_eeprom();
}


/*===========================================================================*/
/* GPT driver related.                                                       */
/*===========================================================================*/

/*
 * GPT7 configuration. This timer is used as trigger for the ADC.
 */


#ifdef ADC_v5
const GPTConfig gpt9cfg1 = {
  //.frequency = 35000U,	//4.2v std
#ifndef ADC_Oversample
//  .frequency = 75000U,		//non over sample 4.2v std
	// .frequency = 60000U,		//non over sample 4.2v std
	.frequency = 140000U,//80000U,//140000U,        // lyon try increase to 8k
#else
  .frequency = 250000U,		//over sample 4.2v std
#endif
  .callback = irq_storm_gpt9_cb,
  .ctrl2 = 0x10,   /* MMS = 010 = TRGO on Update Event.    */
  .iden = 0U
};
#else
const GPTConfig gpt9cfg1 = {
  //.frequency = 35000U,	//4.2v std
#ifndef ADC_Oversample
//  .frequency = 75000U,		//non over sample 4.2v std
	// .frequency = 60000U,		//non over sample 4.2v std
	.frequency = 110000U,//80000U,//140000U,        // lyon try increase to 8k
#else
  .frequency = 250000U,		//over sample 4.2v std
#endif
  .callback = irq_storm_gpt9_cb,
  .ctrl2 = 0x10,   /* MMS = 010 = TRGO on Update Event.    */
  .iden = 0U
};
#endif

#ifdef ADC_v5
#define ADC_SAMPLE_CYCLE		ADC_SAMPLE_28P5			//ADC_SAMPLE_1P5 ADC_SAMPLE_7P5 ADC_SAMPLE_13P5 ADC_SAMPLE_28P5 ADC_SAMPLE_41P5 ADC_SAMPLE_55P5 ADC_SAMPLE_71P5 ADC_SAMPLE_239P5
#else
#define ADC_SAMPLE_CYCLE		ADC_SAMPLE_239P5			//ADC_SAMPLETIME_1_5  ADC_SAMPLETIME_7_5  ADC_SAMPLETIME_13_5 ADC_SAMPLETIME_28_5 ADC_SAMPLETIME//ADC_SAMPLETIME_1_5  ADC_SAMPLETIME_7_5  ADC_SAMPLETIME_13_5 ADC_SAMPLETIME_28_5 ADC_SAMPLETIME_41_5 ADC_SAMPLETIME_55_5 ADC_SAMPLETIME_71_5 ADC_SAMPLETIME_239_5_41_5 ADC_SAMPLETIME_55_5 ADC_SAMPLETIME_71_5 ADC_SAMPLETIME_239_5
#endif
//#define ADC_VM_HIGH				0x07F0
#define ADC_VM_HIGH				0xFFF0
#define ADC_VM_LOW				0x0000

static adcsample_t samples1[ADC_NUM_CHANNELS];
adcsample_t samples2[ADC_BUF_DEPTH][ADC_NUM_CHANNELS], initPostionLevel[ADC_BUF_DEPTH][ADC_NUM_CHANNELS], travelPostionLevel[ADC_BUF_DEPTH][ADC_NUM_CHANNELS]; ;
adcsample_t keyDepth[MATRIX_ROWS][MATRIX_COLS];
// adcsample_t adc2mmLevel[ADC_BUF_DEPTH][ADC_NUM_CHANNELS];
static int32_t adcsampleIndex = 0, adcUpdateIndex = 0;
static void adccallback(ADCDriver* adcp);
static void adcerrorcallback(ADCDriver* adcp, adcerror_t err);
/*
 * ADC streaming callback.
 */


#ifdef ADC_v5
static const ADCConversionGroup adcgrpcfg2 = {
  FALSE,
  ADC_NUM_CHANNELS,
  adccallback,

  adcerrorcallback,
#ifdef ADC_Monitor
  ADC_VMONITOR_SINGLE_ORDINARY | ADC_CHANNEL_4,                        /* CR1 */
#else
  0,
#endif
//  	ADC_CTRL2_OCSWTRG,          /* CR2 */
  ADC_CTRL2_OCSWTRG | 7 << 17 | ADC_CTRL2_OCTEN,
//  	1<<30U,
	ADC_SPT1_CSPT13(ADC_SAMPLE_CYCLE) |
	ADC_SPT1_CSPT12(ADC_SAMPLE_CYCLE) | ADC_SPT1_CSPT11(ADC_SAMPLE_CYCLE) |
	ADC_SPT1_CSPT10(ADC_SAMPLE_CYCLE),
	ADC_SPT2_CSPT2(ADC_SAMPLE_CYCLE) | ADC_SPT2_CSPT1(ADC_SAMPLE_CYCLE) |
	ADC_SPT2_CSPT0(ADC_SAMPLE_CYCLE) ,                    /* SMPR2 */
	ADC_VM_HIGH,              /* HTR */
	ADC_VM_LOW,               /* LTR */
	0,                        /* SQR1 */
	ADC_OSQ2_OSN7_N(ADC_CHANNEL_IN2) ,
	ADC_OSQ3_OSN6_N(ADC_CHANNEL_IN1) | ADC_OSQ3_OSN5_N(ADC_CHANNEL_IN0) |
	ADC_OSQ3_OSN4_N(ADC_CHANNEL_IN13) | ADC_OSQ3_OSN3_N(ADC_CHANNEL_IN12) |
	ADC_OSQ3_OSN2_N(ADC_CHANNEL_IN11) | ADC_OSQ3_OSN1_N(ADC_CHANNEL_IN10)

};
#endif

//OK
#ifdef ADC_v1
void irq_storm_gpt9_cb(GPTDriver* gptp)
{
    //	msg_t msg;
    //
    //	(void)gptp;
    //	chSysLockFromISR();
    //#ifdef ADC_Debug
    //	palTogglePad(GPIOF, 7);														//ADC GPT Trigger LED
    //#endif
    //	chSysUnlockFromISR();

	sensorPowerCtrl(true);
	testCB++;
	adcsampleIndex = -1;
	adcStartConversion(&ADCD1, &adcgrpcfg2, samples1, 1);
//	adcStartConversion_Start(&ADCD1,ADC_CR2_SWSTART);
}
#endif

#ifdef ADC_v2
void irq_storm_gpt9_cb(GPTDriver* gptp)
{
	sensorPowerCtrl(true);
	testCB++;
//	adcsampleIndex = -1;
	adcStartConversion(&ADCD1, &adcgrpcfg2, samples1, 1);
//	adcStartConversion_Start(&ADCD1,ADC_CR2_SWSTART);
}
#endif

//#ifdef ADC_v4
//void irq_storm_gpt9_cb(GPTDriver* gptp)
//{
//	chSysLockFromISR();
//	switch(adcStauts){
//		case adcIdel:
//			if (dly_times-- == 0){
//				adcStauts = adcSensorPowerOn;
//			}
//
//		break;
//		case adcSensorPowerOn:
//			sensorPowerCtrl(true);
//			adcsampleIndex = 0;
//			muxSelect(adcsampleIndex);
//			adcStauts = adcConvertStart;
//#ifdef minTriggerCheck
//			dly_times = 4;
//#else
//			dly_times = 2;
//#endif
//		break;
////		case adcMuxSelect:
////			if (dly_times == 0){
////				muxSelect(adcsampleIndex);
////				adcStauts = adcConvertStart;
////			}
////		break;
//		case adcConvertStart:
//			if (!ledUpdatingLock && dly_times-- == 0){
//				if (adcsampleIndex<16){
//					adcStartConversion(&ADCD1, &adcgrpcfg2, samples1, 1);
//
//#ifdef minTriggerCheck
//					dly_times = 20;
//#else
//					dly_times = 4;
//#endif
//				} else {
//					sensorPowerCtrl(false);
//					chBSemSignalI(&adcCompleteSem);
//					adcsampleIndex = 0;
//					muxSelect(adcsampleIndex);
//					adcStauts = adcIdel;
//
//#ifdef minTriggerCheck
//					dly_times = 5;
//#else
//					dly_times = 5;
//#endif
//				}
//			}
//		break;
//		case adcSensorPowerOff:
//		break;
//	}
//	chSysUnlockFromISR();
//}
//#endif

#ifdef ADC_v4
void irq_storm_gpt9_cb(GPTDriver* gptp)
{
	switch(adcStauts){
		case adcIdel:
			if (dly_times-- == 0){
				adcStauts = adcSensorPowerOn;
			}

		break;
		case adcSensorPowerOn:
			sensorPowerCtrl(true);
			adcsampleIndex = 0;
			muxSelect(adcsampleIndex);
			adcStauts = adcConvertStart;
#ifdef minTriggerCheck
			dly_times = 3;
#else
			dly_times = 0;
#endif
		break;
//		case adcMuxSelect:
//			if (dly_times == 0){
//				muxSelect(adcsampleIndex);
//				adcStauts = adcConvertStart;
//			}
//		break;
		case adcConvertStart:
			if (!ledUpdatingLock && dly_times-- == 0){
				if (adcsampleIndex<16){
					adcStartConversion(&ADCD1, &adcgrpcfg2, &samples2[adcsampleIndex][0], 1);

#ifdef minTriggerCheck
					dly_times = 10;
#else
					dly_times = 1;
#endif
				} else {
					muxSelect(adcsampleIndex);
					sensorPowerCtrl(false);
					chBSemSignalI(&adcCompleteSem);
//					adcsampleIndex = 0;
					adcStauts = adcIdel;

#ifdef minTriggerCheck
					dly_times = 3;
#else
					dly_times = 5;
#endif
				}
			}
		break;
		case adcSensorPowerOff:
		break;
	}
}
#endif

#ifdef ADC_v5
void irq_storm_gpt9_cb(GPTDriver* gptp)
{
	switch(adcStauts){
		case adcSensorPowerOn:
			sensorPowerCtrl(true);
			adcsampleIndex = 0;
			muxSelect(adcsampleIndex);
			adcStauts = adcConvertStart;
#ifndef ADC_Oversample
			gptStartOneShot(&GPTD9, 2);
#else
			gptStartOneShot(&GPTD9, 3);
#endif
		break;
		case adcConvertStart:
//			adcStartConversion(&ADCD1, &adcgrpcfg2, &samples2[adcsampleIndex][0], 1);
			if (!ledUpdatingLock){
				if (adcsampleIndex<16){	
					// palSetPadMode(GPIOC, 12, PAL_MODE_INPUT_PULLUP);
					// palSetPad(GPIOC, 12);  
					AdcPortChange(false);
					// add control gpio at here
					// palSetPad(GPIOC, 12);    // HIGH
					// palClearPad(GPIOC, 12);  // LOW
					// palTogglePad(GPIOC, 12); // 反相
					// palSetPadMode(GPIOC, 12, PAL_MODE_INPUT_PULLDOWN);
					// chThdSleep(TIME_US2I(40));
					// chThdSleepMicroseconds(10); 
					static bool toggleGpio = 0;
					////////////////palSetPad(GPIOC, 12); 
					// if(toggleGpio){
					// 	palSetPad(GPIOC, 12); 
					// 	// palSetPadMode(GPIOC, 12, PAL_MODE_INPUT_PULLDOWN);
					// 	toggleGpio = 0;
					// } else {
					// 	palClearPad(GPIOC, 12);
					// 	// palSetPadMode(GPIOC, 12, PAL_MODE_INPUT_PULLUP);
					// 	toggleGpio = 1;
					// }
					adcStartConversion(&ADCD1, &adcgrpcfg2, &samples2[adcsampleIndex][0], 1);
					//adcStartConversion_Start(&ADCD1, ADC_CTRL2_OCSWTRG);

				return;
				} else {
					sensorPowerCtrl(false);
					adcsampleIndex = 0;
					muxSelect(adcsampleIndex);
					chBSemSignalI(&adcCompleteSem);
#ifndef ADC_Oversample
					gptStartOneShot(&GPTD9, 5);
#else
					gptStartOneShot(&GPTD9, 3);
#endif
					adcStauts = adcSensorPowerOn;

				}
			} else {
				sensorPowerCtrl(false);
#ifndef ADC_Oversample
					gptStartOneShot(&GPTD9, 2);  
#else
					gptStartOneShot(&GPTD9, 3);
#endif
					adcStauts = adcConvertWait;
			}

		break;
		
		case adcConvertWait:
			sensorPowerCtrl(true);
			gptStartOneShot(&GPTD9, 5);
			adcStauts = adcConvertStart;
			break;
	}
}
#endif
void adcOffsetGet(bool findMax)
{
    if (!findMax)
    {
        memset(initPostionLevel, 0, sizeof(initPostionLevel));
        //uprintf ("offset clear\r\n");
    }
    //uprintf ("offset get\r\n");
    for (uint8_t i = 0; i < ADC_NUM_CHANNELS; i++)
    {
        for (uint8_t j = 0; j < ADC_BUF_DEPTH; j++)
        {
            if (samples2[j][i] > initPostionLevel[j][i])
            {
                initPostionLevel[j][i] = samples2[j][i];
            }
        }
    }
}

void adcOffsetCalc(bool findMax, bool update, uint16_t samples)
{
    uint16_t k = 0;
    uint32_t SampleTimer;
    memset(initPostionLevel, 0, sizeof(initPostionLevel));

    adcUpdateIndex = 0;
    SampleTimer = timer_read();
    currentKeyMatrixMode = keyMatrixMode_InitialPostionCalc;
    lastKeyScanEffect = keyScanEffect;
    lastRapidKeyEnable = rapidKeyEnable;
    lastkeycodeOutput = keycodeOutput;
//    currentLED_RunMode.type = offMode;
//    setRunMode(currentLED_RunMode);
//    ledAllOff(true);
    keyScanEffect = true;
    rapidKeyEnable = false;
    keycodeOutput = false;
    while (timer_read() < (SampleTimer + samples*10))
    {
        for (uint8_t i = 0; i < ADC_NUM_CHANNELS; i++)
        {
            for (uint8_t j = 0; j < ADC_BUF_DEPTH; j++)
            {
#ifdef nonOP
                if (samples2[j][i] > initPostionLevel[j][i])
                {
                	if (k++<500){
                    initPostionLevel[j][i] 	= samples2[j][i]+3;
                	} else {
                		initPostionLevel[j][i] 	= samples2[j][i] + 5;
                	}
                }
#else
                if (samples2[j][i] > initPostionLevel[j][i])
                {
//                	if (k++<500){
//                    initPostionLevel[j][i] 	= samples2[j][i]+6;
//                	} else {
//                		initPostionLevel[j][i] 	= samples2[j][i] + 9;
//                	}
#ifdef minTriggerCheck
                	initPostionLevel[j][i] 	= samples2[j][i]+2;
#else
                	initPostionLevel[j][i] 	= samples2[j][i]+5;
#endif
                }
#endif
            }
        }
    }
    if (update)
    {
    	keyScanEffect = false;
        uprintf("offset calc \r\n");
        for (uint16_t i = 0; i < (ADC_NUM_CHANNELS * ADC_BUF_DEPTH); i++)
        {
            eeprom_write_word(EECONFIG_KEYMAP_OFFSET_CALIBRATION + i * 2, initPostionLevel[i % ADC_BUF_DEPTH][i / ADC_BUF_DEPTH]);
        }
    }
    currentKeyMatrixMode = keyMatrixMode_Normal;
    keyScanEffect = false;
    rapidKeyEnable = lastRapidKeyEnable;
    keycodeOutput = lastkeycodeOutput;
//    if (!globleTrigger){
//		currentLED_RunMode.type = normalMode;
//		setRunMode(currentLED_RunMode);
//    }
}

void adcTravelCalc(uint8_t mode)
{
    uint16_t i;
    uint32_t SampleTimer;
    switch (mode)
    {
        case 0:
            lastKeyScanEffect = keyScanEffect;
            lastRapidKeyEnable = rapidKeyEnable;
            lastkeycodeOutput = keycodeOutput;
            currentLED_RunMode.type = offMode;
            setRunMode(currentLED_RunMode);
//            chThdSleepMilliseconds(10);
            ledAllOff(true);
            keyScanEffect = true;
            rapidKeyEnable = false;
            keycodeOutput = false;
            memset(travelPostionLevel, 0, sizeof(travelPostionLevel));
            uprintf("enter travel postion calibration modes\r\n");
            currentKeyMatrixMode = keyMatrixMode_TravelPostionCalc;
            break;
        case 1:
        	currentKeyMatrixMode = keyMatrixMode_Normal;
        	keyScanEffect = false;
            uprintf("update travel postion levels\r\n");
            uprintf("Start:%X\r\n", EECONFIG_KEYMAP_TRAVEL_CALIBRATION);

            for (i = 0; i < (ADC_NUM_CHANNELS * ADC_BUF_DEPTH); i++)
            {
                eeprom_write_word(EECONFIG_KEYMAP_TRAVEL_CALIBRATION + i * 2, travelPostionLevel[i % ADC_BUF_DEPTH][i / ADC_BUF_DEPTH]);
                //uprintf ("write:%d data:%d\r\n",travelPostionLevel[i % ADC_BUF_DEPTH][i / ADC_BUF_DEPTH]);
            }
            uprintf("End:%X\r\n\r\n", EECONFIG_KEYMAP_TRAVEL_CALIBRATION + i * 2);

            keyScanEffect = lastKeyScanEffect;
            rapidKeyEnable = lastRapidKeyEnable;
            keycodeOutput =lastkeycodeOutput;
//            if (!globleTrigger){
//                currentLED_RunMode.type = normalMode;
//                setRunMode(currentLED_RunMode);
//            }
            break;
    }
}

// void adc2mmCapture(bool update)
// {
//     uint32_t SampleTimer;
//     uint16_t i;

//     uprintf("Start 2mm capture\r\n");

//     memset(adc2mmLevel, 0, sizeof(adc2mmLevel));   // 清空緩衝區
//     SampleTimer = timer_read();

//     // 暫時停掉不必要功能，避免干擾
//     lastKeyScanEffect  = keyScanEffect;
//     lastRapidKeyEnable = rapidKeyEnable;
//     lastkeycodeOutput  = keycodeOutput;

//     keyScanEffect  = true;
//     rapidKeyEnable = false;
//     keycodeOutput  = false;
//     currentKeyMatrixMode = keyMatrixMode_InitialPostionCalc;

//     // 給 20~50ms 穩定時間
//     while (timer_elapsed(SampleTimer) < 50) {
//         // 等待 ADC 有更新 samples2[][]，略延遲
//         chThdSleepMilliseconds(1);
//     }

//     // 🔹讀取當前 ADC 值當作「2mm 壓下」時的樣本
//     for (uint8_t i = 0; i < ADC_NUM_CHANNELS; i++) {
//         for (uint8_t j = 0; j < ADC_BUF_DEPTH; j++) {
//             adc2mmLevel[j][i] = samples2[j][i];
//			   uprintf ("write:%d data:%d\r\n",travelPostionLevel[i % ADC_BUF_DEPTH][i / ADC_BUF_DEPTH]);
//         }
//     }

//     uprintf("Captured 2mm ADC values\r\n");

//     // 🔹是否要更新 EEPROM
//     if (update) {
//         uprintf("Saving 2mm ADC values to EEPROM...\r\n");
//         for (i = 0; i < (ADC_NUM_CHANNELS * ADC_BUF_DEPTH); i++) {
//             eeprom_write_word(EECONFIG_KEYMAP_TRAVEL_2MM + i * 2,
//                 adc2mmLevel[i % ADC_BUF_DEPTH][i / ADC_BUF_DEPTH]);
//         }
//         uprintf("Save done\r\n");
//     }

//     // 恢復原狀態
//     currentKeyMatrixMode = keyMatrixMode_Normal;
//     keyScanEffect  = lastKeyScanEffect;
//     rapidKeyEnable = lastRapidKeyEnable;
//     keycodeOutput  = lastkeycodeOutput;
// }

static void muxSelect(uint8_t channel){
	// uprintf("mux idx=%d\r\n", channel);
	uint8_t data, data_pa, data_pf;
	data = 15-channel;
	data_pa = (((data & 0x01) | ((data >> 2)) & 0x1e)) << 3;
	data_pf = ((data >> 1) & 0x03) << 4;


	palWriteGroup(GPIOA, PAL_PORT_BIT(3) | PAL_PORT_BIT(4), 0, data_pa);
    palWriteGroup(GPIOC, PAL_PORT_BIT(6) | PAL_PORT_BIT(7) | PAL_PORT_BIT(8) | PAL_PORT_BIT(9), 0, (channel ) << 6);
    palWriteGroup(GPIOF, PAL_PORT_BIT(4) | PAL_PORT_BIT(5), 0, data_pf);
    //analogPowerSwitch(15-(channel));
}

#ifdef ADC_v1
static void adccallback(ADCDriver* adcp)
{
  	muxSelect(adcsampleIndex+1);
# ifdef ADC_Monitor
    if (adc_flag_get(adcp->adc, ADC_VMOR_FLAG) != RESET)
    {
        adc_flag_clear(ADC1, ADC_VMOR_FLAG);
        palTogglePad(GPIOD, 13);
    }
#endif


  	if (adcsampleIndex<16){
  		if (!ledUpdatingLock && adcsampleIndex>=0){
			for (uint8_t i = 0; i < ADC_NUM_CHANNELS; i++)
			  {
				  samples2[adcsampleIndex][i] = samples1[i];
			  }
  		}
    	adcStartConversion(&ADCD1, &adcgrpcfg2, samples1, 1);
//  		adcStartConversion(&ADCD1, &adcgrpcfg2, &samples2[adcsampleIndex][0], 1);
        adcsampleIndex++;
        testADC++;
  	} else {
  		sensorPowerCtrl(false);
  		//memset (samples1,0,sizeof(samples1));
  		//if (!ledUpdateFilter || !ledUpdatingLock){
  			chBSemSignalI(&adcCompleteSem);
  		//}
		adcsampleIndex = 0;
		muxSelect(adcsampleIndex);
  	}
}
#endif

#ifdef ADC_v2
static void adccallback(ADCDriver* adcp)
{
# ifdef ADC_Monitor
    if (adc_flag_get(adcp->adc, ADC_VMOR_FLAG) != RESET)
    {
        adc_flag_clear(ADC1, ADC_VMOR_FLAG);
        palTogglePad(GPIOD, 13);
    }
#endif
  	if (adcsampleIndex<16){
  		if (!ledUpdatingLock && adcsampleIndex>=0 && ADC_Check == true){
//  			if (ADC_Wait){
  			for (uint8_t i = 0; i < ADC_NUM_CHANNELS; i++)
			  {
				  samples2[adcsampleIndex][i] = samples1[i];
			  }
//  			}
  			if (!ADC_Wait){
			adcsampleIndex++;
  	    	} else {
  	    		ADC_Wait = true;
  	    	}
  		}
    	adcStartConversion(&ADCD1, &adcgrpcfg2, samples1, 1);
        testADC++;

  	} else {
//  		sensorPowerCtrl(false);
  		//memset (samples1,0,sizeof(samples1));
  		//if (!ledUpdateFilter || !ledUpdatingLock){
  			chBSemSignalI(&adcCompleteSem);
  		//}
		adcsampleIndex = 0;
		muxSelect(adcsampleIndex);
		ADC_Check = false;
		ADC_Wait = false;
  	}
  	if (ADC_Check){muxSelect(adcsampleIndex);}
  	ADC_Check = !ADC_Check;
}
#endif

//#ifdef ADC_v3
//static void adccallback(ADCDriver* adcp)
//{
//	chSysLockFromISR();
//	if (!ledUpdatingLock && adcsampleIndex>=0){
//		for (uint8_t i = 0; i < ADC_NUM_CHANNELS; i++)
//		  {
//				samples2[adcsampleIndex][i] = samples1[i];
//		  }
//	} else {
//		sensorPowerCtrl(false);
//		adcsampleIndex = 0;
//		muxSelect(adcsampleIndex);
//		adcStauts = adcIdel;
//#ifdef minTriggerCheck
//		dly_times = 5;
//#else
//		dly_times = 4;
//#endif
//		chSysUnlockFromISR();
//		return;
//	}
//	muxSelect(++adcsampleIndex);
//	chSysUnlockFromISR();
//}
//#endif


//#ifdef ADC_v4
//static void adccallback(ADCDriver* adcp)
//{
////	if (!ledUpdatingLock){
//		for (uint8_t i = 0; i < ADC_NUM_CHANNELS; i++)
//		  {
//				samples2[adcsampleIndex][i] = samples1[i];
//		  }
////	}
////	else{
////		sensorPowerCtrl(false);
////		adcsampleIndex = 0;
////		muxSelect(adcsampleIndex);
////		adcStauts = adcIdel;
////#ifdef minTriggerCheck
////		dly_times = 5;
////#else
////		dly_times = 2;
////#endif
////		return;
////	}
//	muxSelect(++adcsampleIndex);
//}
//#endif


#ifdef ADC_v4
static void adccallback(ADCDriver* adcp)
{
//	if (!ledUpdatingLock){
//		for (uint8_t i = 0; i < ADC_NUM_CHANNELS; i++)
//		  {
//				samples2[adcsampleIndex][i] = samples1[i];
//		  }
//	}
//	else{
//		sensorPowerCtrl(false);
//		adcsampleIndex = 0;
//		muxSelect(adcsampleIndex);
//		adcStauts = adcIdel;
//#ifdef minTriggerCheck
//		dly_times = 5;
//#else
//		dly_times = 2;
//#endif
//		return;
//	}
	muxSelect(++adcsampleIndex);
}
#endif

#ifdef ADC_v5
static void adccallback(ADCDriver* adcp)
{
	/////////////palClearPad(GPIOC, 12);
	chBSemSignalI(&adcCompleteSem);
	AdcPortChange(true);
	sensorPowerCtrl(false);
	muxSelect(++adcsampleIndex);
#ifndef ADC_Oversample
	gptStartOneShot(&GPTD9, 14); // lyon 更改這個數值至3看看adc數值是否正常//original is 7 // 9問題改8測試8可以，為什麼13可以，改10吧(為了8k)會有問題
	// maybe change to higher value give it more time to process ex: 01-9, 02-8,02會有點狀況改成刪除狀態?no
	// 7 / 60000 秒 ＝ 約 116.7 µs
	// 8 / 140000sec = 57usec, 7/140000=50
	#else
	gptStartOneShot(&GPTD9, 3);
#endif
	sensorPowerCtrl(true);
}
#endif


static void adcerrorcallback(ADCDriver* adcp, adcerror_t err)
{
	(void)adcp;
	(void)err;
}

void analogPowerSwitch(uint8_t data)
{
	uint8_t data_pa, data_pf;
	data_pa = (((data & 0x01) | ((data >> 2)) & 0x1e)) << 3;
	data_pf = ((data >> 1) & 0x03) << 4;
	//uprintf ("pa:%X pf:%X\r\n",data_pa,data_pf);
	palWriteGroup(GPIOA, PAL_PORT_BIT(3) | PAL_PORT_BIT(4), 0, data_pa);
	palWriteGroup(GPIOF, PAL_PORT_BIT(4) | PAL_PORT_BIT(5), 0, data_pf);
    }
//# ifdef ADCTestHispeed
//void matrixDebugTest(uint8_t mode)
//{
//	int adcCalc;
//	for (uint8_t group = 0; group < ADC_NUM_CHANNELS; group++)
//	{
//		for (uint8_t location = 0; location < ADC_BUF_DEPTH; location++)
//		{
//			switch (mode)
//			{
//				case 0:
//					adcCalc = samples2[location][group] - initPostionLevel[location][group];
//					if (adcCalc < 3)
//					{
//						adcCalc = 0;
//					}
//					else
//					{
//						adcCalc -= 3;
//					}
//					break;
//				case 1:
//					adcCalc = travelPostionLevel[location][group] - initPostionLevel[location][group];
//					if (adcCalc < 32)
//					{
//						adcCalc = 0;
//					}
//					break;
//			}
//
//			//analogBtnFill(group, location, LED_NULL, adcCalc);
//
//
//				led_config_t EffectranbowColor;
//				analogKey_Config_t keyLocation = analogKeyRelocate[group][location];;
//				uint8_t currentStep = 0;
//				if (location > colMax[group]) { break; }
//				for (uint8_t i=0;i<RAPID_TRIGGER_COUNT;i++){
//						if (rapidTriggerKeys[i].row == keyLocation.rowA && rapidTriggerKeys[i].col == keyLocation.colA){
//							if (rapidTriggerKeys[i].enable && rapidTriggerKeys[i].counts != 0){
//							if (adcCalc < rapidTriggerKeys[i].onLevel[0]){
//								EffectranbowColor = LED_NULL;
//
//							} else {
//								for (uint8_t j=0;j<rapidTriggerKeys[i].counts;j++){
//									if (adcCalc> rapidTriggerKeys[i].onLevel[j] && adcCalc< rapidTriggerKeys[i].onLevel[j+1]){
//										currentStep ++;
//										break;
//									}else {
//										if (adcCalc>rapidTriggerKeys[i].onLevel[j+1]){currentStep ++;}
//									}
//								}
//								EffectranbowColor = rainbowColor[currentStep-1];
//							}
//							if (currentStep && currentStep != rapidTriggerKeys[i].lastStep){
//								if (rapidTriggerKeys[i].lastKeycode != 0xffff){
//									unregister_rapidTrigger_code(rapidTriggerKeys[i].lastKeycode);
//									register_rapidTrigger_code(rapidTriggerKeys[i].keycode[currentStep-1]);
//
//								} else {
//									register_rapidTrigger_code(rapidTriggerKeys[i].keycode[currentStep-1]);
//								}
//								rapidTriggerKeys[i].lastKeycode = rapidTriggerKeys[i].keycode[currentStep-1];
//							}
//							if (currentStep == 0 && rapidTriggerKeys[i].lastStep != 0){
//								unregister_rapidTrigger_code(rapidTriggerKeys[i].lastKeycode);
//								rapidTriggerKeys[i].lastKeycode = 0xffff;
//								rapidTriggerKeys[i].lastStep = 0;
//							}
//							rapidTriggerKeys[i].lastStep = currentStep;
//							setKeyMatrixLED(keyLocation.rowA, keyLocation.colA, EffectranbowColor);
//							//break;
//						}
//
//					}
//				}
//
//				float mult;
//				if (keyLocation.rowA == -1 || keyLocation.colA == -1) { return; }
//				EffectranbowColor = rainbowColor[group];
//				if (adcCalc>35){ adcCalc-=35;}
//				if (adcCalc>600){
//					mult = 1;
//				} else {
//					mult = (float)adcCalc / 600;         //OP*2	nonOP*10
//				}
//				EffectranbowColor.red *= mult;
//				EffectranbowColor.green *= mult;
//				EffectranbowColor.blue *= mult;
//				setKeyMatrixLED(keyLocation.rowA, keyLocation.colA, EffectranbowColor);
//				//break;
//
//		}
//	}
//	chBSemSignal(&ledUpdateSem);
//    }
//
//
//#else
//    uint8_t matrixDebugTest(uint8_t mode)
//    {
//        int adcCalc;
//        uprintf("\033[2J\033[H");
//        uprintf("Timer:%6d   00   01   02   03   04   05   06   07   08   09   10   11   12   13   14   15\r\n", adcUpdateIndex);
//
//        for (uint8_t i = 0; i < ADC_NUM_CHANNELS; i++)
//        {
//            uprintf("Key group %d: ", i);
//            for (uint8_t j = 0; j < ADC_BUF_DEPTH; j++)
//            {
//                adcCalc = samples2[j][i] - initPostionLevel[j][i];
//                if (adcCalc < 3)
//                {
//                    adcCalc = 0;
//                    uprintf("   0 ");
//                }
//                else
//                {
//                    adcCalc -= 3;
//                    uprintf("%4d ", adcCalc);
//                }
//                analogBtnFill(i, j, LED_NULL, adcCalc);
//
//            }
//            uprintf("\r\n");
//        }
//        chBSemSignal(&ledUpdateSem);
//    }
//#endif
//

//Optimize version
//bool matrixAnalogScan(void)
//{
//	uint8_t rapidKeyStatus;
//	int adcCalc;
//	bool rapidStartCheck, rapidEndCheck;
//	led_config_t EffectranbowColor;
//	uint8_t currentStep = 0;
//    chSysLock();
//	for (uint8_t group = 0; group < ADC_NUM_CHANNELS; group++)
//	{
//		for (uint8_t channel = 0; channel < (ADC_BUF_DEPTH); channel++)
//		{
//			switch (currentKeyMatrixMode)
//			{
//				case keyMatrixMode_Normal:
//				case keyMatrixMode_InitialPostionCalc:
//					adcCalc = samples2[channel][group] - initPostionLevel[channel][group];
//					break;
//				case keyMatrixMode_TravelPostionCalc:
//					if (travelPostionLevel[channel][group] < samples2[channel][group])
//					{
//						travelPostionLevel[channel][group] = samples2[channel][group];
//					}
//					adcCalc = travelPostionLevel[channel][group] - initPostionLevel[channel][group];
//					if (adcCalc < 32)
//					{
//						adcCalc = 0;
//					}
//					break;
//			}
//
//			if (adcCalc < 0)
//			{
//				adcCalc = 0;
//			}
//			rapidKeyStatus = false;
//			keyLocation = analogKeyRelocate[group][channel];
//			for (uint8_t i = 0; i < RAPID_TRIGGER_COUNT && !rapidKeyStatus && rapidKeyEnable; i++)
//			{
//				if (rapidTriggerKeys[i].row == keyLocation.rowA && rapidTriggerKeys[i].col == keyLocation.colA)
//				{
//					if (rapidTriggerKeys[i].enable && rapidTriggerKeys[i].counts != 0)
//					{
//
//						if (adcCalc < rapidTriggerKeys[i].onLevel[0])																//no press
//						{
//							EffectranbowColor = LED_NULL;
//						}
//						else
//						{
//							for (uint8_t j = 0; j < rapidTriggerKeys[i].counts; j++)
//							{
//								rapidEndCheck = false;
//								rapidStartCheck = false;
//								if (adcCalc > rapidTriggerKeys[i].onLevel[j] && adcCalc < rapidTriggerKeys[i].onLevel[j + 1])
//								{
//									currentStep++;
//									break;
//								}
//								else
//								{
//									if (adcCalc > rapidTriggerKeys[i].onLevel[j + 1]) { currentStep++; }
//								}
//							}
//							EffectranbowColor = rainbowColor[currentStep - 1];
//						}
//						if (currentStep && currentStep != rapidTriggerKeys[i].lastStep)
//						{
//							if (keycodeOutput){
//								if (rapidTriggerKeys[i].lastKeycode != 0xffff)
//								{
//									unregister_rapidTrigger_code(rapidTriggerKeys[i].lastKeycode);
//									uprintf ("last keycode:%X\r\n",rapidTriggerKeys[i].lastKeycode);
//									uprintf ("break last key Step:%d\r\n",currentStep,rapidTriggerKeys[i].lastKeycode);
//									register_rapidTrigger_code(rapidTriggerKeys[i].keycode[currentStep - 1]);
//									uprintf ("make last key Step:%d code:%X\r\n",currentStep,rapidTriggerKeys[i].keycode[currentStep - 1]);
//								}
//								else
//								{
//									if (!rapidStartCheck){
//									register_rapidTrigger_code(rapidTriggerKeys[i].keycode[currentStep - 1]);
//									uprintf ("\033[2J\033[HMake start current step:%d rapidStartCheck:%d \r\n",currentStep,rapidStartCheck);
//									rapidStartCheck = true;
//									}
//								}
//							}
//							rapidTriggerKeys[i].lastKeycode = rapidTriggerKeys[i].keycode[currentStep - 1];
//						}
//						if (currentStep == 0 && rapidTriggerKeys[i].lastStep != 0)
//						{
//							if (keycodeOutput && rapidTriggerKeys[i].lastKeycode != 0xffff && !rapidEndCheck){
//							unregister_rapidTrigger_code(rapidTriggerKeys[i].lastKeycode);
//							uprintf ("End break\r\n");
//							//tap_code(KC_ENT);
//							rapidEndCheck = true;
//							}
//							rapidTriggerKeys[i].lastKeycode = 0xffff;
//							rapidTriggerKeys[i].lastStep = 0;
//						}
//						rapidTriggerKeys[i].lastStep = currentStep;
//						if (keyScanEffect)LED_Datas[ledKeyMatrix[keyLocation.rowA][keyLocation.colA]] = EffectranbowColor;
//						rapidKeyStatus = true;
//						break;
//					}
//				}
//			}
//			if (rapidKeyStatus == false)
//			{
//				if (adcCalc > keyMatrixTriggerLevel)
//				{
//					matrix_temp[keyLocation.rowA] |= (0x01 << keyLocation.colA);
//				}
//				if (adcCalc <= keyMatrixReleaseLevel)
//				{
//					matrix_temp[keyLocation.rowA] &= ~(0x01 << keyLocation.colA);
//				}
//				if (globleTrigger){
//					if (keyScanEffect)
//					{
//						float mult;
//						//if (keyLocation.rowA == -1 || keyLocation.colA == -1) { return; }
//
//						if (adcCalc>1){
//							adcCalc -= 1;
//							EffectranbowColor = rainbowColor[group];
//							if (adcCalc > 600)
//							{
//								mult = 1;
//							}
//							else
//							{
//								mult = (float)adcCalc / 600;         //OP*2	nonOP*10
//							}
//							EffectranbowColor.red *= mult;
//							EffectranbowColor.green *= mult;
//							EffectranbowColor.blue *= mult;
//
//							LED_Datas[ledKeyMatrix[keyLocation.rowA][keyLocation.colA]] = EffectranbowColor;
//
//						} else {
//							EffectranbowColor = LED_NULL;
//						}
//						//LED_Datas[ledKeyMatrix[keyLocation.rowA][keyLocation.colA]] = EffectranbowColor;
//					}
//				}
//				else {
//					if (keyScanEffect)
//					{
//						float mult;
//						//if (keyLocation.rowA == -1 || keyLocation.colA == -1) { return; }
//
//						if (adcCalc>15){
//							adcCalc -= 15;
//							EffectranbowColor = rainbowColor[group];
//							if (adcCalc > 600)
//							{
//								mult = 1;
//							}
//							else
//							{
//								mult = (float)adcCalc / 600;         //OP*2	nonOP*10
//							}
//							EffectranbowColor.red *= mult;
//							EffectranbowColor.green *= mult;
//							EffectranbowColor.blue *= mult;
//						} else {
//							EffectranbowColor = LED_NULL;
//						}
//						LED_Datas[ledKeyMatrix[keyLocation.rowA][keyLocation.colA]] = EffectranbowColor;
//					}
//				}
//			}
//		}
//	}
//		crcReset(&CRCD1);
//		currentMatrix_CRC = crcCalc(&CRCD1, sizeof(matrix_temp), &matrix_temp);
//		if (lastMatrix_CRC != currentMatrix_CRC)
//		{
//			memcpy(matrix, matrix_temp, sizeof(matrix));
//			keyboard_taskTMR();
//			lastMatrix_CRC = currentMatrix_CRC;
//		}
//
//	if (keyScanEffect)chBSemSignal(&ledUpdateSem);
//    chSysUnlock();
//}

uint16_t current_keycode = KC_NO;
uint8_t level = 0;

bool matrixAnalogScan(void)
{
	bool rapidKeyStatus;
	int adcCalc, adcPre;
	bool rapidStartCheck, rapidEndCheck;
	led_config_t EffectranbowColor;
	float mult;
	int8_t currentStep = 0;
//	uint8_t filterValue;
//    chSysLock();

#ifdef eepromClear
// 在 matrixAnalogScan() 函式中（僅需加入一次）
void perform_factory_reset(void) {
	currentLED_RunMode.mode = 0;
	currentLED_RunMode.type = normalMode;
	setRunMode(currentLED_RunMode);
	eeconfig_rapidTrigger_init();
	keyMatrixTriggerLevel = DEFAULT_TRIGGER_LEVEL;
	keyMatrixReleaseLevel = DEFAULT_RELEASE_LEVEL;
	eeprom_write_word(EECONFIG_KEYMAP_TRIGGER_LEVEL, keyMatrixTriggerLevel);
	eeprom_write_word(EECONFIG_KEYMAP_RELEASE_LEVEL, keyMatrixReleaseLevel);
	write_trigger_to_eeprom();
	write_Release_to_eeprom();
	eeconfig_rapidTrigger_init();
	write_keymap_to_eeprom();
	dynamic_macro_reset_eeprom();
	chThdSleepMilliseconds(100);
	NVIC_SystemReset();
}

static bool reset_checked = false;

if (!reset_checked) {
    reset_checked = true;

    analogKey_Config_t loc = matrixAnalogRelocate[0][0];
	int adc_val = samples2[loc.rowA][loc.colA] - initPostionLevel[loc.rowA][loc.colA];

    if (adc_val > keyMatrixTriggerLevel) {
        perform_factory_reset();
    }
}
#endif // eepromClear

#ifdef minTriggerCheck

    if (rapidKeyEnable){
    	draftValue = 0;
    } else {
    	draftValue = 15;
    }
#else
    draftValue = 0;
#endif
	for (uint8_t group = 0; group < ADC_NUM_CHANNELS; group++)
	{
		for (uint8_t channel = 0; channel < (ADC_BUF_DEPTH); channel++)
		{

			switch (currentKeyMatrixMode)
			{
				case keyMatrixMode_Normal:
				case keyMatrixMode_InitialPostionCalc:
					adcCalc = samples2[channel][group] - initPostionLevel[channel][group];
					break;
				case keyMatrixMode_TravelPostionCalc:
					if (travelPostionLevel[channel][group] < samples2[channel][group])
					{
						travelPostionLevel[channel][group] = samples2[channel][group];
					}
					adcCalc = travelPostionLevel[channel][group] - initPostionLevel[channel][group];
					if (adcCalc < 32)
					{
						adcCalc = 0;
					}
					break;
			}

			adcsample_t travel = travelPostionLevel[channel][group];
			adcsample_t init   = initPostionLevel[channel][group];
			adcsample_t now    = samples2[channel][group];

			adcsample_t range  = travel - init;
			adcPre = adcCalc * TRAVEL_STEPS / range;

			if (adcPre < 0){
				adcPre = 0;
			}
			if (adcPre >= TRAVEL_STEPS){
				adcPre = TRAVEL_STEPS;
			}

			if (adcCalc < 0)
			{
				adcCalc = 0;
			}
			keyLocation = analogKeyRelocate[group][channel];

			// Dynamic Keystroke - lyon
			rapidKeyStatus = false;
			if (rapidKeyEnable){
				for (uint8_t i = 0; i < RAPID_TRIGGER_COUNT && !rapidKeyStatus; i++){
					if (rapidTriggerKey[i].key.enable && rapidTriggerKey[i].key.counts != 0){
						if (rapidTriggerKey[i].key.row == keyLocation.rowA && rapidTriggerKey[i].key.col == keyLocation.colA){
							currentStep = 0;
							rapidKeyStatus = true;
							if (adcCalc < rapidTriggerKey[i].onLevel[0])																//no press
							{

								if (rapidTriggerKey[i].lastStep != -1 && adcCalc<=4){
									//uprintf ("<==end break:%d \r\n",rapidTriggerKey[i].lastStep);
									if (keycodeOutput){
										unregister_rapidTrigger_code(rapidTriggerKey[i].key.keycode[rapidTriggerKey[i].lastStep]);

										tap_code(KC_ENT);
									}
									rapidTriggerKey[i].lastStep = -1;
									rapidTriggerKey[i].triggeredLevel = 0;
									if (keyScanEffect && ledKeyMatrix[keyLocation.rowA][keyLocation.colA] != -1)LED_Datas[ledKeyMatrix[keyLocation.rowA][keyLocation.colA]] = LED_NULL;
									//chThdSleepMilliseconds(3);
									//rapidKeyStatus = true;
									continue;
								}

							}else {
								for (uint8_t j = 1; j < rapidTriggerKey[i].key.counts; j++)
								{
									//rapidEndCheck = false;
									//rapidStartCheck = false;
									if (adcCalc >= rapidTriggerKey[i].onLevel[j] && adcCalc < rapidTriggerKey[i].onLevel[j + 1])
									//if (adcCalc >= rapidTriggerKey[i].onLevel[j] && adcCalc < rapidTriggerKey[i].onLevel[j] + 50)
									{
										currentStep++;
										break;
									}
									else
									{
										if (adcCalc > rapidTriggerKey[i].onLevel[j + 1]) { currentStep++; }
									}
								}
								EffectranbowColor = rainbowColor[currentStep];
								EffectranbowColor = rainbowColor[currentStep - 1];
#ifdef nonOP
								if (rapidTriggerKey[i].lastStep != currentStep && (rapidTriggerKey[i].triggeredLevel == 0 || (abs(rapidTriggerKey[i].triggeredLevel - adcCalc)>8)) && ((adcCalc < rapidTriggerKey[i].onLevel[rapidTriggerKey[i].key.counts]) || rapidTriggerKey[i].key.counts == RAPID_TRIGGER_LEVELS)){
#else
								if (rapidTriggerKey[i].lastStep != currentStep && (rapidTriggerKey[i].triggeredLevel == 0 || (abs(rapidTriggerKey[i].triggeredLevel - adcCalc)>30)) && ((adcCalc < rapidTriggerKey[i].onLevel[rapidTriggerKey[i].key.counts]) || rapidTriggerKey[i].key.counts == RAPID_TRIGGER_LEVELS)){
#endif
									if (currentStep == 0 && rapidTriggerKey[i].lastStep ==-1){
										//uprintf ("\033[2J\033[H==>start make step:%d\r\n",currentStep);
										if (keycodeOutput){
											register_rapidTrigger_code(rapidTriggerKey[i].key.keycode[currentStep]);
										}
										if (keyScanEffect && ledKeyMatrix[keyLocation.rowA][keyLocation.colA] != -1)LED_Datas[ledKeyMatrix[keyLocation.rowA][keyLocation.colA]] = rainbowColor[0];
										//chThdSleepMilliseconds(3);
									}else {
										//uprintf ("break last step:%d\r\n",rapidTriggerKey[i].lastStep);
										//if (rapidTriggerKey[i].triggeredLevel!=0 || ((abs(rapidTriggerKey[i].triggeredLevel - adcCalc)>50) && currentStep == rapidTriggerKey[i].lastStep)){
										//if (rapidTriggerKey[i].triggeredLevel!=0){
											rapidTriggerKey[i].triggeredLevel = adcCalc;
											if (keycodeOutput){
												//chThdSleepMilliseconds(3);
												unregister_rapidTrigger_code(rapidTriggerKey[i].key.keycode[rapidTriggerKey[i].lastStep]);
												//uprintf ("make step:%d\r\n",currentStep);
												register_rapidTrigger_code(rapidTriggerKey[i].key.keycode[currentStep]);
											}
											if (keyScanEffect && ledKeyMatrix[keyLocation.rowA][keyLocation.colA] != -1)LED_Datas[ledKeyMatrix[keyLocation.rowA][keyLocation.colA]] = rainbowColor[currentStep];
										//}
									}
									//rapidKeyStatus = true;
									rapidTriggerKey[i].lastStep = currentStep;
									continue;
								}
							}
						}
					}
				}

			}

			if (keyScanEffect)
			{

				//if (keyLocation.rowA == -1 || keyLocation.colA == -1) { return; }

				if (adcCalc>150){//DEFAULT_TRIGGER_LEVEL){ // lyon change DEFAULT_TRIGGER_LEVEL to original daytona value 150
					adcCalc -= 150;
					EffectranbowColor = rainbowColor[group];
					if (adcCalc > 600)
					{
						mult = 1;
					}
					else
					{
						mult = (float)adcCalc / 600;         //OP*2	nonOP*10
					}
					EffectranbowColor.red *= mult;
					EffectranbowColor.green *= mult;
					EffectranbowColor.blue *= mult;


				} else {
					EffectranbowColor = LED_NULL;
				}
			if (ledKeyMatrix[keyLocation.rowA][keyLocation.colA] != -1){
					LED_Datas[ledKeyMatrix[keyLocation.rowA][keyLocation.colA]] = EffectranbowColor;
					chBSemSignal(&ledUpdateSem);
				}
			}
				// uprintf("KEY row=%d col=%d layer=%d kc=%04X pressed=%d\r\n",
				// 	keyLocation.rowA, keyLocation.colA, get_highest_layer(layer_state), keycode, pressed);

			#ifdef rappySnappy
			// adcCalc 已經算好
			adcPre_value[keyLocation.rowA][keyLocation.colA] = (int16_t)adcPre;



			if (rappy_snappy_is_key(keyLocation.rowA, keyLocation.colA)) {
				continue;
			}
			#endif // rappySnappy

			#ifdef rappyTrigger
			if(keyLocation.rowA == -1 && keyLocation.colA == -1){continue;}
			#endif // rappyTrigger

			#ifdef socd

			bool pressed_now = calc_pressed_now(keyLocation.rowA, keyLocation.colA, adcCalc);

			/* ① 先通知 SOCD */
			if(isSOCD){
				void on_key_changed(uint8_t row, uint8_t col, bool pressed);
			}

			#endif // socd
			#ifdef modTap
			uint8_t L = 0;
			// uint8_t R = keyLocation.rowA;
			// uint8_t C = keyLocation.colA;
			// bool prev = mt_prev[R][C];
			bool prev = mt_prev[keyLocation.rowA][keyLocation.colA];

			if (mt_active(L, keyLocation.rowA, keyLocation.colA)) {
				if (pressed_now && !prev) mt_on_down(L, keyLocation.rowA, keyLocation.colA);   // 上一輪未按、這輪按下 → down
				if (!pressed_now && prev) mt_on_up  (L, keyLocation.rowA, keyLocation.colA);   // 上一輪按下、這輪放開 → up
				mt_prev[keyLocation.rowA][keyLocation.colA] = pressed_now;                     // ★ 自行維護 prev
				continue;                                        // ★ 跳過原本 matrix_temp 的設/清位
			}
			#endif // modTap
			#ifdef toggleKey
			// 取得目前 active layer（假設你有 action_layer.h / layer_state）
			uint8_t act_layer = biton32(layer_state);

			// 確保是合法位置
			if (keyLocation.rowA >= 0 && keyLocation.colA >= 0 &&
				keyLocation.rowA < MATRIX_ROWS && keyLocation.colA < MATRIX_COLS &&
				act_layer < KEYMAPS_COUNT) {

				toggle_key_slot_t *tk = &toggle_keys[act_layer][keyLocation.rowA][keyLocation.colA];

				if (tk->enabled) {
					// === 1. 用同樣門檻邏輯算「現在視為按下？」 ===
					bool now_pressed = false;

					if (globleTrigger) {
						if (adcCalc > (keyMatrixTriggerLevel + draftValue)) {
							now_pressed = true;
						}
						if (adcCalc <= keyMatrixReleaseLevel) {
							now_pressed = false;
						}
					} else {
						if (adcCalc > (eeconfig_read_Trigger(keyLocation.rowA, keyLocation.colA) + draftValue)) {
							now_pressed = true;
						}
						if (adcCalc <= eeconfig_read_Release(keyLocation.rowA, keyLocation.colA)) {
							now_pressed = false;
						}
					}

					bool was_pressed = toggle_last_pressed[act_layer][keyLocation.rowA][keyLocation.colA];
					toggle_last_pressed[act_layer][keyLocation.rowA][keyLocation.colA] = now_pressed;

					// === 2. Rising edge：從「沒按」變成「有按」才翻轉狀態 ===
					if (!was_pressed && now_pressed) {
						tk->state = !tk->state;

						if (tk->state) {
							// 切到 ON：按下 keycode
							if (tk->keycode) {
								register_code(tk->keycode);
								// uprintf("Toggle ON L%u R%u C%u KC=0x%02X\r\n",
								//         act_layer, keyLocation.rowA, keyLocation.colA, tk->keycode);
							}
						} else {
							// 切回 OFF：放開 keycode
							if (tk->keycode) {
								unregister_code(tk->keycode);
								// uprintf("Toggle OFF L%u R%u C%u KC=0x%02X\r\n", ...);
							}
						}
					}

					// === 3. 這顆 key 當成純「控制開關」，不參與一般 matrix 掃描 ===
					matrix_temp[keyLocation.rowA] &= ~(1 << keyLocation.colA);

					// 如果你不想讓它亮掃描燈效，也可以在這裡直接 continue
					// 例如：
					// if (keyScanEffect) {
					//     LED_Datas[ ledKeyMatrix[keyLocation.rowA][keyLocation.colA] ] = LED_NULL;
					// }
					continue;
				}
			}

			#endif // toggleKey


			// if (rapidKeyStatus == false) // lyon 
			{
//				if (ledUpdateFilter){
//#ifdef nonOP
//					filterValue = 15;
//#else
//					filterValue = 45;
//#endif
//				} else {
//					filterValue = 0;
//				}
//				filterValue = draftShift;
				// uprintf("%s\r\n", globleTrigger ? "true" : "false"); //false
				if (globleTrigger){ //
					if (adcPre > (keyMatrixTriggerLevel+draftValue))
					{
						matrix_temp[keyLocation.rowA] |= (0x01 << keyLocation.colA);
					}
				} else {
					if (adcPre > (eeconfig_read_Trigger(keyLocation.rowA,keyLocation.colA)+draftValue))
					{
						matrix_temp[keyLocation.rowA] |= (0x01 << keyLocation.colA);
						// uprintf ("group:%d channel:%d row:%d col:%d\r\n",group,channel,keyLocation.rowA,keyLocation.colA);
						// uprintf ("eeconfig_read_Trigger:%d\r\n",eeconfig_read_Trigger(keyLocation.rowA,keyLocation.colA)+draftValue); //
					}
				}
				if (globleTrigger){
					if (adcPre <= keyMatrixReleaseLevel)
					{
						matrix_temp[keyLocation.rowA] &= ~(0x01 << keyLocation.colA);
					}
				}
				else {
					if (adcPre <= eeconfig_read_Release(keyLocation.rowA,keyLocation.colA))
					{
						matrix_temp[keyLocation.rowA] &= ~(0x01 << keyLocation.colA);
					}
				}
				// if (globleTrigger){ //
				// 	if (adcCalc > (keyMatrixTriggerLevel+draftValue))
				// 	{
				// 		// matrix_temp[keyLocation.rowA] |= (0x01 << keyLocation.colA);
				// 	}
				// } else {
				// 	if (adcCalc > (eeconfig_read_Trigger(keyLocation.rowA,keyLocation.colA)+draftValue))
				// 	{
				// 		// matrix_temp[keyLocation.rowA] |= (0x01 << keyLocation.colA);
				// 		// uprintf("eeconfig_read_Trigger(keyLocation.rowA,keyLocation.colA) is %d",eeconfig_read_Trigger(keyLocation.rowA,keyLocation.colA));
				// 		// uprintf ("group:%d channel:%d row:%d col:%d\r\n",group,channel,keyLocation.rowA,keyLocation.colA);
				// 		// uprintf ("eeconfig_read_Trigger:%d\r\n",eeconfig_read_Trigger(keyLocation.rowA,keyLocation.colA)+draftValue); //
				// 	}
				// }
				// if (globleTrigger){
				// 	if (adcCalc <= keyMatrixReleaseLevel)
				// 	{
				// 		// matrix_temp[keyLocation.rowA] &= ~(0x01 << keyLocation.colA);
				// 	}
				// }
				// else {
				// 	if (adcCalc <= eeconfig_read_Release(keyLocation.rowA,keyLocation.colA))
				// 	{
				// 		// matrix_temp[keyLocation.rowA] &= ~(0x01 << keyLocation.colA);
				// 	}
				// }
				//if (globleTrigger){
#ifdef nonOP
					if (keyScanEffect)
					{
						float mult;
						//if (keyLocation.rowA == -1 || keyLocation.colA == -1) { return; }

						if (adcCalc>5){
							adcCalc -= 5;
							EffectranbowColor = rainbowColor[group];
							if (adcCalc > 100)
							{
								mult = 1;
							}
							else
							{
								mult = (float)adcCalc / 100;         //OP*2	nonOP*10
							}
							EffectranbowColor.red *= mult;
							EffectranbowColor.green *= mult;
							EffectranbowColor.blue *= mult;

							//LED_Datas[ledKeyMatrix[keyLocation.rowA][keyLocation.colA]] = EffectranbowColor;

						} else {
							EffectranbowColor = LED_NULL;
						}
						LED_Datas[ledKeyMatrix[keyLocation.rowA][keyLocation.colA]] = EffectranbowColor;
					}
#else
					// if (keyScanEffect)
					// {

					// 	//if (keyLocation.rowA == -1 || keyLocation.colA == -1) { return; }

					// 	if (adcCalc>150){//DEFAULT_TRIGGER_LEVEL){ // lyon change DEFAULT_TRIGGER_LEVEL to original daytona value 150
					// 		adcCalc -= 150;//DEFAULT_TRIGGER_LEVEL;
					// 		EffectranbowColor = rainbowColor[group];
					// 		if (adcCalc > 600)
					// 		{
					// 			mult = 1;
					// 		}
					// 		else
					// 		{
					// 			mult = (float)adcCalc / 600;         //OP*2	nonOP*10
					// 		}
					// 		EffectranbowColor.red *= mult;
					// 		EffectranbowColor.green *= mult;
					// 		EffectranbowColor.blue *= mult;


					// 	} else {
					// 		EffectranbowColor = LED_NULL;
					// 	}
					// if (ledKeyMatrix[keyLocation.rowA][keyLocation.colA] != -1){
					// 		LED_Datas[ledKeyMatrix[keyLocation.rowA][keyLocation.colA]] = EffectranbowColor;
					// 		chBSemSignal(&ledUpdateSem);
					// 	}
					// }
#endif
				//}
//				else {
//					if (keyScanEffect)
//					{
//						float mult;
//						//if (keyLocation.rowA == -1 || keyLocation.colA == -1) { return; }
//
//						if (adcCalc>15){
//							adcCalc -= 15;
//							EffectranbowColor = rainbowColor[group];
//							if (adcCalc > 600)
//							{
//								mult = 1;
//							}
//							else
//							{
//								mult = (float)adcCalc / 600;         //OP*2	nonOP*10
//							}
//							EffectranbowColor.red *= mult;
//							EffectranbowColor.green *= mult;
//							EffectranbowColor.blue *= mult;
//						} else {
//							EffectranbowColor = LED_NULL;
//						}
//						LED_Datas[ledKeyMatrix[keyLocation.rowA][keyLocation.colA]] = EffectranbowColor;
//					}
//				}
			}
		}
	}
	static uint32_t lastMatrixUpdateTime = 0;

	// if (keyScanEffect) // lyon put it outside of the loop
	// {

	// 	//if (keyLocation.rowA == -1 || keyLocation.colA == -1) { return; }

	// 	if (adcCalc>150){//DEFAULT_TRIGGER_LEVEL){ // lyon change DEFAULT_TRIGGER_LEVEL to original daytona value 150
	// 		adcCalc -= 150;//DEFAULT_TRIGGER_LEVEL;
	// 		EffectranbowColor = rainbowColor[group];
	// 		if (adcCalc > 600)
	// 		{
	// 			mult = 1;
	// 		}
	// 		else
	// 		{
	// 			mult = (float)adcCalc / 600;         //OP*2	nonOP*10
	// 		}
	// 		EffectranbowColor.red *= mult;
	// 		EffectranbowColor.green *= mult;
	// 		EffectranbowColor.blue *= mult;


	// 	} else {
	// 		EffectranbowColor = LED_NULL;
	// 	}
	// if (ledKeyMatrix[keyLocation.rowA][keyLocation.colA] != -1){
	// 		LED_Datas[ledKeyMatrix[keyLocation.rowA][keyLocation.colA]] = EffectranbowColor;
	// 		chBSemSignal(&ledUpdateSem);
	// 	}
	// }



	#ifdef rappySnappy
	rappy_snappy_process_all();
	#endif // rappySnappy

	#ifdef rappyTrigger
	// process_rappy_ratio_trend(&rappy_q);
	rappy_q.enabled = false;
	if(rappy_q.enabled == true){
		process_rappy_fixed_ratio(&rappy_q);
	}
	#endif // rappyTrigger

	#ifdef modTap
	mt_tick(0);         // 處理長按超時 → 送/收修飾鍵
	mt_apply_pending(); //
	#endif // modTap


// uprintf("rapidKeyStatus is %d\r\n", rapidKeyStatus);
	if (rapidKeyStatus == false){
		// crcReset(&CRCD1);
		// currentMatrix_CRC = crcCalc(&CRCD1, sizeof(matrix_temp), &matrix_temp);
		// if (lastMatrix_CRC != currentMatrix_CRC)
		if (true)
		{
				uint32_t layer = layer_state;   // 目前 layer 狀態

				/* 偵測 MO(1) 被按下（0 → 1） */
				bool mo1_pressed =
					!(last_layer_state & (1UL << 1)) &&
					(layer & (1UL << 1));

				if (mo1_pressed) {
					clear_mods();      // 清 modifier
					layer_clear();     // 清殘留 Fn / MO
					layer_on(1);       // 確保 MO(1) 仍然有效
				}

				last_layer_state = layer;
			memcpy(matrix, matrix_temp, sizeof(matrix));
			keyboard_taskTMR(); // 
			//lastMatrix_CRC = currentMatrix_CRC;
		}
	} else if (timer_elapsed32(lastMatrixUpdateTime) > 500){
		keyboard_taskTMR();
		lastMatrixUpdateTime = timer_read32();
	}
	if (gameAnalogStickStatus.raw !=0){
		chBSemSignal(&gamepadAnalogSem);
	}
	// if(currentLED_RunMode.type == dynamticLightingMode && !lampArrayDisable){
	// 	if (rapidKeyStatus == false){
	// 		// crcReset(&CRCD1);
	// 		// currentMatrix_CRC = crcCalc(&CRCD1, sizeof(matrix_temp), &matrix_temp);
	// 		// if (lastMatrix_CRC != currentMatrix_CRC)
	// 		if (true)
	// 		{
	// 			memcpy(matrix, matrix_temp, sizeof(matrix));
	// 			keyboard_taskTMR();
	// 			//lastMatrix_CRC = currentMatrix_CRC;
	// 		}
	// 	} else if (timer_elapsed32(lastMatrixUpdateTime) > 500){
	// 		keyboard_taskTMR();
	// 		lastMatrixUpdateTime = timer_read32();
	// 	}
	// 	if (gameAnalogStickStatus.raw !=0){
	// 		chBSemSignal(&gamepadAnalogSem);
	// 	}
	// }
//    chSysUnlock();

	/*if (rapidKeyStatus == false) {
		crcReset(&CRCD1);
		currentMatrix_CRC = crcCalc(&CRCD1, sizeof(matrix_temp), &matrix_temp);
		if (lastMatrix_CRC != currentMatrix_CRC) {
			memcpy(matrix, matrix_temp, sizeof(matrix));
			keyboard_taskTMR();
			lastMatrix_CRC = currentMatrix_CRC;  // ← 必須有這行
		}
	}*/

	// EECLR
	void eeclr_task(void)
	{
		if (!eeclr_pressed || eeclr_fired) {
			return;
		}

		if (timer_elapsed32(eeclr_timer) >= 3000) {
			eeclr_fired = true;
			eeclr_pressed = false;

			uprintf("[EECLR] hold 3s -> factory reset\r\n");
			// keyboard_factory_reset_all();
			knobState = 0;
			eeprom_update_byte(KB_ENCODER_MODE_ADDR, knobState);
			eeconfig_socd_clear();
			eeconfig_rappy_snappy_clear();
			eeconfig_modtap_clear();
			currentLED_RunMode.mode = 8;
			currentLED_RunMode.type = offMode;
			setRunMode(currentLED_RunMode);
			eeprom_update_byte(KB_COLOR_MODE_ADDR, currentLED_RunMode.mode);
			eeprom_write_word(EECONFIG_POLLING_RATE, 0x00);
			uprintf (".");
			eeconfig_rapidTrigger_init();
			keyMatrixTriggerLevel = DEFAULT_TRIGGER_LEVEL;
			keyMatrixReleaseLevel = DEFAULT_RELEASE_LEVEL;
			eeprom_write_word(EECONFIG_KEYMAP_TRIGGER_LEVEL, keyMatrixTriggerLevel);
			uprintf (".");
			eeprom_write_word(EECONFIG_KEYMAP_RELEASE_LEVEL, keyMatrixReleaseLevel);
			uprintf (".");
			write_trigger_to_eeprom();
			uprintf (".");
			write_Release_to_eeprom();
			uprintf (".");
			eeconfig_rapidTrigger_init();
			uprintf (".");
			write_keymap_to_eeprom();
			uprintf (".");
			dynamic_macro_reset_eeprom();
			uprintf (".");
			uprintf (".");
			eeprom_write_word(EEPROM_FACTORY_INIT_FLAG_ADDR, 0xFA);
			uprintf("\r\nEnd\r\n");
			chThdSleepMilliseconds(500);
			NVIC_SystemReset();
		}
	}
	eeclr_task();

}


//
//
//
//			rapidKeyStatus = false;
//			keyLocation = analogKeyRelocate[group][channel];
//			for (uint8_t i = 0; i < RAPID_TRIGGER_COUNT && !rapidKeyStatus && rapidKeyEnable; i++)
//			{
//				if (rapidTriggerKeys[i].row == keyLocation.rowA && rapidTriggerKeys[i].col == keyLocation.colA)
//				{
//					if (rapidTriggerKeys[i].enable && rapidTriggerKeys[i].counts != 0)
//					{
//
//						if (adcCalc < rapidTriggerKeys[i].onLevel[0])																//no press
//						{
//							EffectranbowColor = LED_NULL;
//						}
//						else
//						{
//							for (uint8_t j = 0; j < rapidTriggerKeys[i].counts; j++)
//							{
//								rapidEndCheck = false;
//								rapidStartCheck = false;
//								if (adcCalc > rapidTriggerKeys[i].onLevel[j] && adcCalc < rapidTriggerKeys[i].onLevel[j + 1])
//								{
//									currentStep++;
//									break;
//								}
//								else
//								{
//									if (adcCalc > rapidTriggerKeys[i].onLevel[j + 1]) { currentStep++; }
//								}
//							}
//							EffectranbowColor = rainbowColor[currentStep - 1];
//						}
//						if (currentStep && currentStep != rapidTriggerKeys[i].lastStep)
//						{
//							if (keycodeOutput){
//								if (rapidTriggerKeys[i].lastKeycode != 0xffff)
//								{
//									unregister_rapidTrigger_code(rapidTriggerKeys[i].lastKeycode);
//									uprintf ("last keycode:%X\r\n",rapidTriggerKeys[i].lastKeycode);
//									uprintf ("break last key Step:%d\r\n",currentStep,rapidTriggerKeys[i].lastKeycode);
//									register_rapidTrigger_code(rapidTriggerKeys[i].keycode[currentStep - 1]);
//									uprintf ("make last key Step:%d code:%X\r\n",currentStep,rapidTriggerKeys[i].keycode[currentStep - 1]);
//								}
//								else
//								{
//									if (!rapidStartCheck){
//									register_rapidTrigger_code(rapidTriggerKeys[i].keycode[currentStep - 1]);
//									uprintf ("\033[2J\033[HMake start current step:%d rapidStartCheck:%d \r\n",currentStep,rapidStartCheck);
//									rapidStartCheck = true;
//									}
//								}
//							}
//							rapidTriggerKeys[i].lastKeycode = rapidTriggerKeys[i].keycode[currentStep - 1];
//						}
//						if (currentStep == 0 && rapidTriggerKeys[i].lastStep != 0)
//						{
//							if (keycodeOutput && rapidTriggerKeys[i].lastKeycode != 0xffff && !rapidEndCheck){
//							unregister_rapidTrigger_code(rapidTriggerKeys[i].lastKeycode);
//							uprintf ("End break\r\n");
//							//tap_code(KC_ENT);
//							rapidEndCheck = true;
//							}
//							rapidTriggerKeys[i].lastKeycode = 0xffff;
//							rapidTriggerKeys[i].lastStep = 0;
//						}
//						rapidTriggerKeys[i].lastStep = currentStep;
//						if (keyScanEffect)LED_Datas[ledKeyMatrix[keyLocation.rowA][keyLocation.colA]] = EffectranbowColor;
//						rapidKeyStatus = true;
//						break;
//					}
//				}
//			}
//			if (rapidKeyStatus == false)
//			{
//				if (adcCalc > keyMatrixTriggerLevel)
//				{
//					matrix_temp[keyLocation.rowA] |= (0x01 << keyLocation.colA);
//				}
//				if (adcCalc <= keyMatrixReleaseLevel)
//				{
//					matrix_temp[keyLocation.rowA] &= ~(0x01 << keyLocation.colA);
//				}
//				if (globleTrigger){
//					if (keyScanEffect)
//					{
//						float mult;
//						//if (keyLocation.rowA == -1 || keyLocation.colA == -1) { return; }
//
//						if (adcCalc>1){
//							adcCalc -= 1;
//							EffectranbowColor = rainbowColor[group];
//							if (adcCalc > 600)
//							{
//								mult = 1;
//							}
//							else
//							{
//								mult = (float)adcCalc / 600;         //OP*2	nonOP*10
//							}
//							EffectranbowColor.red *= mult;
//							EffectranbowColor.green *= mult;
//							EffectranbowColor.blue *= mult;
//
//							LED_Datas[ledKeyMatrix[keyLocation.rowA][keyLocation.colA]] = EffectranbowColor;
//
//						} else {
//							EffectranbowColor = LED_NULL;
//						}
//						//LED_Datas[ledKeyMatrix[keyLocation.rowA][keyLocation.colA]] = EffectranbowColor;
//					}
//				}
//				else {
//					if (keyScanEffect)
//					{
//						float mult;
//						//if (keyLocation.rowA == -1 || keyLocation.colA == -1) { return; }
//
//						if (adcCalc>15){
//							adcCalc -= 15;
//							EffectranbowColor = rainbowColor[group];
//							if (adcCalc > 600)
//							{
//								mult = 1;
//							}
//							else
//							{
//								mult = (float)adcCalc / 600;         //OP*2	nonOP*10
//							}
//							EffectranbowColor.red *= mult;
//							EffectranbowColor.green *= mult;
//							EffectranbowColor.blue *= mult;
//						} else {
//							EffectranbowColor = LED_NULL;
//						}
//						LED_Datas[ledKeyMatrix[keyLocation.rowA][keyLocation.colA]] = EffectranbowColor;
//					}
//				}
//			}
//		}
//	}
//		crcReset(&CRCD1);
//		currentMatrix_CRC = crcCalc(&CRCD1, sizeof(matrix_temp), &matrix_temp);
//		if (lastMatrix_CRC != currentMatrix_CRC)
//		{
//			memcpy(matrix, matrix_temp, sizeof(matrix));
//			keyboard_taskTMR();
//			lastMatrix_CRC = currentMatrix_CRC;
//		}
//
//	if (keyScanEffect)chBSemSignal(&ledUpdateSem);
//    chSysUnlock();
//}

//BF Marco Org version
//bool matrixAnalogScan(void)
//{
//uint8_t rapidKeyStatus;
//int adcCalc;
//led_config_t EffectranbowColor;
//uint8_t currentStep = 0;
//for (uint8_t group = 0; group < ADC_NUM_CHANNELS; group++)
//{
//	for (uint8_t channel = 0; channel < (ADC_BUF_DEPTH); channel++)
//	{
//		switch (currentKeyMatrixMode)
//		{
//			case keyMatrixMode_Normal:
//			case keyMatrixMode_InitialPostionCalc:
//				adcCalc = samples2[channel][group] - initPostionLevel[channel][group];
//				break;
//			case keyMatrixMode_TravelPostionCalc:
//				if (travelPostionLevel[channel][group] < samples2[channel][group])
//				{
//					travelPostionLevel[channel][group] = samples2[channel][group];
//				}
//				adcCalc = travelPostionLevel[channel][group] - initPostionLevel[channel][group];
//				if (adcCalc < 32)
//				{
//					adcCalc = 0;
//				}
//				break;
//		}
//
//		if (adcCalc < 0)
//		{
//			adcCalc = 0;
//		}
//		if (channel <= colMax[group])
//		{
//			keyLocation = analogKeyRelocate[group][channel];
//			rapidKeyStatus = false;
//			for (uint8_t i = 0; i < RAPID_TRIGGER_COUNT && !rapidKeyStatus && rapidKeyEnable; i++)
//			{
//				if (rapidTriggerKeys[i].row == keyLocation.rowA && rapidTriggerKeys[i].col == keyLocation.colA)
//				{
//					if (rapidTriggerKeys[i].enable && rapidTriggerKeys[i].counts != 0)
//					{
//						rapidKeyStatus = TRUE;
//						if (adcCalc < rapidTriggerKeys[i].onLevel[0])
//						{
//							EffectranbowColor = LED_NULL;
//
//						}
//						else
//						{
//							for (uint8_t j = 0; j < rapidTriggerKeys[i].counts; j++)
//							{
//								if (adcCalc > rapidTriggerKeys[i].onLevel[j] && adcCalc < rapidTriggerKeys[i].onLevel[j + 1])
//								{
//									currentStep++;
//									break;
//								}
//								else
//								{
//									if (adcCalc > rapidTriggerKeys[i].onLevel[j + 1]) { currentStep++; }
//								}
//							}
//							EffectranbowColor = rainbowColor[currentStep - 1];
//						}
//						if (currentStep && currentStep != rapidTriggerKeys[i].lastStep)
//						{
//							if (keycodeOutput){
//								if (rapidTriggerKeys[i].lastKeycode != 0xffff)
//								{
//										unregister_code(rapidTriggerKeys[i].lastKeycode);
//										register_code(rapidTriggerKeys[i].keycode[currentStep - 1]);
//								}
//								else
//								{
//									register_code(rapidTriggerKeys[i].keycode[currentStep - 1]);
//								}
//							}
//							rapidTriggerKeys[i].lastKeycode = rapidTriggerKeys[i].keycode[currentStep - 1];
//						}
//						if (currentStep == 0 && rapidTriggerKeys[i].lastStep != 0)
//						{
//							if (keycodeOutput){
//							unregister_code(rapidTriggerKeys[i].lastKeycode);
//							}
//							rapidTriggerKeys[i].lastKeycode = 0xffff;
//							rapidTriggerKeys[i].lastStep = 0;
//						}
//						rapidTriggerKeys[i].lastStep = currentStep;
//						if (keyScanEffect)LED_Datas[ledKeyMatrix[keyLocation.rowA][keyLocation.colA]] = EffectranbowColor;
//						//chBSemSignal(&ledUpdateSem);
//						rapidKeyStatus = true;
//					}
//
//				}
//			}
//			if (rapidKeyStatus == false)
//			{
//				if (adcCalc > keyMatrixTriggerLevel)
//				{
//					matrix_temp[keyLocation.rowA] |= (0x01 << keyLocation.colA);
//				}
//				else
//				{
//					matrix_temp[keyLocation.rowA] &= ~(0x01 << keyLocation.colA);
//				}
//				if (keyScanEffect)
//				{
//					float mult;
//					//if (keyLocation.rowA == -1 || keyLocation.colA == -1) { return; }
//					EffectranbowColor = rainbowColor[group];
//					if (adcCalc > 600)
//					{
//						mult = 1;
//					}
//					else
//					{
//						mult = (float)adcCalc / 600;         //OP*2	nonOP*10
//					}
//					EffectranbowColor.red *= mult;
//					EffectranbowColor.green *= mult;
//					EffectranbowColor.blue *= mult;
//					LED_Datas[ledKeyMatrix[keyLocation.rowA][keyLocation.colA]] = EffectranbowColor;
//				}
//			}
//		}
//	}
//	//if (keycodeOutput){
//		crcReset(&CRCD1);
//		currentMatrix_CRC = crcCalc(&CRCD1, sizeof(matrix_temp), &matrix_temp);
////			if (lastMatrix_CRC != currentMatrix_CRC)
//		{
//			memcpy(matrix, matrix_temp, sizeof(matrix));
//			keyboard_taskTMR();
//			lastMatrix_CRC = currentMatrix_CRC;
//		}
//
//	//}
////# ifdef keycodeOutput
////		switch (currentKeyMatrixMode)
////		{
////			case keyMatrixMode_Normal:
////				crcReset(&CRCD1);
////				currentMatrix_CRC = crcCalc(&CRCD1, sizeof(matrix_temp), &matrix_temp);
////				if (lastMatrix_CRC != currentMatrix_CRC)
////				{
////					memcpy(matrix, matrix_temp, sizeof(matrix));
////					keyboard_taskTMR();
////				}
////				lastMatrix_CRC = currentMatrix_CRC;
////
////			case keyMatrixMode_InitialPostionCalc:
////			case keyMatrixMode_TravelPostionCalc:
////				break;
////		}
////
////#endif
//
//}
//if (keyScanEffect)chBSemSignal(&ledUpdateSem);
//}

    //bool matrixAnalogFind(void){
    //int triggerLevel = 15;
    //int releaseLevel = 1;
    //int adcCalc;
    //	for (uint8_t i = 0; i < ADC_NUM_CHANNELS; i++) {
    //		for (uint8_t j = 0; j < (ADC_BUF_DEPTH); j++) {
    //			adcCalc = samples2[j][i] - initPostionLevel[j][i];
    //			keyLocation = analogKeyRelocate[i][j];
    //			//uprintf ("Groupt:%d Channel:%d Row:%d Col:%d\r\n",i,j,keyLocation.rowA,keyLocation.colA);
    //			uprintf ("%d\t%d\t%d\t%d\t\r\n",i,j,keyLocation.rowA,keyLocation.colA);
    //		}
    //	}
    //}

    bool matrixAnalogFind(void)
    {
        int triggerLevel = 20;
        int releaseLevel = 5;
        int adcCalc;

        uprintf("\033[2J\033[H");
        uprintf("         00   01   02   03   04   05   06   07   08   09   10   11   12   13   14   15   16   17   18   19   20\r\n");
        for (uint8_t i = 0; i < 6; i++)
        {
            uprintf("Row %d: ", i);
            for (uint8_t j = 0; j < 15; j++)
            {
                keyLocation = matrixAnalogRelocate[i][j];
                adcCalc = initPostionLevel[keyLocation.colA][keyLocation.rowA];
                if (j < colMax[i])
                {
                    uprintf("%04X ", adcCalc);
                }
            }
            uprintf("\r\n");
        }
    }


static THD_WORKING_AREA(waADC_ProcessThread, 512);
static THD_FUNCTION(ADC_ProcessThread, arg)
{
    uprintf("ADC Task start...\r\n");

    //Init ADC control GPIO
    chBSemObjectInit(&adcCompleteSem, true);
    //osalMutexObjectInit(&keymatrixProcessMut);
    palSetGroupMode(GPIOC, PAL_PORT_BIT(0) | PAL_PORT_BIT(1) | PAL_PORT_BIT(2) | PAL_PORT_BIT(3), 0, PAL_MODE_INPUT_ANALOG);
    palSetGroupMode(GPIOA, PAL_PORT_BIT(0) | PAL_PORT_BIT(1) | PAL_PORT_BIT(2), 0, PAL_MODE_INPUT_ANALOG);



    //ADC Mux select			S0+:PC6	S1+:PC7	S2+:PC8	S3+:PC9
    //Selector IO
    palSetGroupMode(GPIOC, PAL_PORT_BIT(6) | PAL_PORT_BIT(7) | PAL_PORT_BIT(8) | PAL_PORT_BIT(9), 0, PAL_MODE_OUTPUT_PUSHPULL);
    palWriteGroup(GPIOC, PAL_PORT_BIT(6) | PAL_PORT_BIT(7) | PAL_PORT_BIT(8) | PAL_PORT_BIT(9), 0, 0xffffffff);

    //Analog Power				S0-:PA3	S1-:PF4	S2-:PF5	S3-:PA4

//    palSetGroupMode(GPIOA, PAL_PORT_BIT(3) | PAL_PORT_BIT(4) | PAL_PORT_BIT(5) | PAL_PORT_BIT(6) | PAL_PORT_BIT(7), 0, PAL_MODE_OUTPUT_PUSHPULL);
//    palSetGroupMode(GPIOF, PAL_PORT_BIT(4) | PAL_PORT_BIT(5), 0, PAL_MODE_OUTPUT_PUSHPULL);
//    analogPowerSwitch(0);

    palSetGroupMode(GPIOA, PAL_PORT_BIT(3) | PAL_PORT_BIT(4) , 0, PAL_MODE_OUTPUT_PUSHPULL);
    palSetGroupMode(GPIOF, PAL_PORT_BIT(4) | PAL_PORT_BIT(5), 0, PAL_MODE_OUTPUT_PUSHPULL);
    analogPowerSwitch(0);



    //Mux Enable IO
    palSetGroupMode(GPIOC, PAL_PORT_BIT(4) | PAL_PORT_BIT(5), 0, PAL_MODE_OUTPUT_PUSHPULL);
    palWriteGroup(GPIOC, PAL_PORT_BIT(4) | PAL_PORT_BIT(5), 0, 0x0000);

    palSetGroupMode(GPIOB, PAL_PORT_BIT(0) | PAL_PORT_BIT(1) | PAL_PORT_BIT(2) | PAL_PORT_BIT(10) | PAL_PORT_BIT(12), 0, PAL_MODE_OUTPUT_PUSHPULL);
    palWriteGroup(GPIOB, PAL_PORT_BIT(0) | PAL_PORT_BIT(1) | PAL_PORT_BIT(2) | PAL_PORT_BIT(10) | PAL_PORT_BIT(12), 0, 0x0000);


//    //Analog Power
//    palSetGroupMode(GPIOA, PAL_PORT_BIT(3) | PAL_PORT_BIT(4) | PAL_PORT_BIT(5) | PAL_PORT_BIT(6) | PAL_PORT_BIT(7), 0, PAL_MODE_OUTPUT_PUSHPULL);
//    palSetGroupMode(GPIOF, PAL_PORT_BIT(4) | PAL_PORT_BIT(5), 0, PAL_MODE_OUTPUT_PUSHPULL);
//    analogPowerSwitch(0);

    //enable all analog power
    //analogPowerSwitch(PAL_PORT_BIT(0) | PAL_PORT_BIT(1) | PAL_PORT_BIT(2) | PAL_PORT_BIT(3) | PAL_PORT_BIT(4) | PAL_PORT_BIT(5) | PAL_PORT_BIT(6));


    //Starting GPT7 driver, it is used for triggering the ADC.
//    gptStart(&GPTD9, &gpt9cfg1);
//    adcStart(&ADCD1, NULL);
//    adcStartConversion(&ADCD1, &adcgrpcfg2, samples1, 1);
//    adcStartConversion_Start(&ADCD1, ADC_CR2_SWSTART);
//    //gptStartContinuous(&GPTD9, 3);
//    gptStartContinuous(&GPTD9, 10);


    adcStart(&ADCD1, NULL);
#ifdef ADC_Oversample
    adc_ordinary_oversample_enable(ADC1,true);
    adc_oversample_ratio_shift_set(ADC1, ADC_OVERSAMPLE_RATIO_2, ADC_OVERSAMPLE_SHIFT_0);
#endif
    gptStart(&GPTD9, &gpt9cfg1);


    adcUpdateIndex = 0;
	//gptStartContinuous(&GPTD9, 2);			//check gpt min 9
	adcStauts = adcSensorPowerOn;
	// adcsampleIndex = 1;
	gptStartOneShot(&GPTD9, 100);			//check gpt min 9
	via_init();
	// LED_init(); //
	led_ctrl(true);
    // current_hsv = rgb_to_hsv(c);
//    currentLED_RunMode.type = offMode;
#ifdef socd
	socd_thread_init();  // lyon socd.c file
	socd_load_all_from_eeprom();
#endif



//	setRunMode(currentLED_RunMode);
	chThdSleepMilliseconds(500);

	for (uint16_t i = 0; i < (ADC_NUM_CHANNELS * ADC_BUF_DEPTH); i++)
    {
        initPostionLevel[i % ADC_BUF_DEPTH][i / ADC_BUF_DEPTH] = eeprom_read_word(EECONFIG_KEYMAP_OFFSET_CALIBRATION + i * 2);
        travelPostionLevel[i % ADC_BUF_DEPTH][i / ADC_BUF_DEPTH] = eeprom_read_word(EECONFIG_KEYMAP_TRAVEL_CALIBRATION + i * 2);
    }

	uprintf ("Trigger Level:%d output:%d btn:%d\r\n",keyMatrixTriggerLevel, keycodeOutput, readPin(A15));

	rapidTriggerProcess(true,0);
    gamePad_init();

	#ifdef FORCE_HARDCODE_RT_PROFILE
	rapidTriggerKey[0].key.enable = 1;
	rapidTriggerKey[0].key.row    = 3;
	rapidTriggerKey[0].key.col    = 1;
	rapidTriggerKey[0].key.counts = 5;

	rapidTriggerKey[0].key.keycode[0] = KC_1;
	rapidTriggerKey[0].key.keycode[1] = KC_2;
	rapidTriggerKey[0].key.keycode[2] = KC_3;
	rapidTriggerKey[0].key.keycode[3] = KC_4;
	rapidTriggerKey[0].key.keycode[4] = KC_5;
	rapidTriggerKey[0].key.keycode[5] = KC_NO;

	// 2) 寫入 EEPROM 的某個 slot（這裡以 slot 0 為例）
    eeconfig_write_rapidTrigger(0, rapidTriggerKey[0].key);

    // 3) 重新從 EEPROM 載入/重算 onLevel 門檻並套用到 RAM
    rapidTriggerProcess(true, 0);

	#endif
	/* Watchdog 設定 */
    // static const WDGConfig wdgcfg = {
    //     .pr  = STM32_IWDG_PR_256,   // 分頻 256
    //     .rlr = 156 * 2              // Reload 值 -> 約 2 秒 timeout
    // };
    // wdgStart(&WDGD1, &wdgcfg);

    while (1)
    {
		// chThdSleepMilliseconds(7);// >2 work just fine for example 2 or 5 later will try 1 lyon
		// chThdSleepMicroseconds(1500); 
		// chThdSleepMicroseconds(10); 

		chBSemWait(&adcCompleteSem); 
		// wdgReset(&WDGD1); //lyon WD

		// chThdSleepMilliseconds(4);
		// chThdSleepMicroseconds(1500); 

        matrixAnalogScan();
        //matrixAnalogFind();
		// uprintf("b\r\n");

    }
}
bool rapidTriggerKeyBuilder(uint8_t key){

analogKey_Config_t keyLocation;
uint16_t initPosValue, travelPosValue;
keyevent_t event;
uint16_t shiftValue;
	rapidTriggerKey[key].key = eeconfig_read_rapidTrigger(key);
	if (rapidTriggerKey[key].key.counts>= RAPID_TRIGGER_LEVELS || rapidTriggerKey[key].key.row>MATRIX_ROWS || rapidTriggerKey[key].key.col>MATRIX_COLS){return false;}

	keyLocation = matrixAnalogRelocate[rapidTriggerKey[key].key.row][rapidTriggerKey[key].key.col];
	initPosValue = initPostionLevel[keyLocation.colA][keyLocation.rowA];
	travelPosValue = travelPostionLevel[keyLocation.colA][keyLocation.rowA];
    for (uint8_t i=0; i< rapidTriggerKey[key].key.counts; i++){
    	rapidTriggerKey[key].onLevel[rapidTriggerKey[key].key.counts-i] = keyMatrixTriggerLevel + (1-sqrt((float)i/(float)rapidTriggerKey[key].key.counts)) * (travelPosValue - initPosValue - keyMatrixTriggerLevel);
    	rapidTriggerKey[key].onLevel[rapidTriggerKey[key].key.counts-i] *= 1.015;
    }
    rapidTriggerKey[key].onLevel[0] = keyMatrixTriggerLevel;

    for (uint8_t i=0; i< rapidTriggerKey[key].key.counts-1; i++){
    	rapidTriggerKey[key].offLevel[i] = rapidTriggerKey[key].onLevel[i+1] - ((rapidTriggerKey[key].onLevel[i+1] - rapidTriggerKey[key].onLevel[i])/8);
    	//uprintf ("Off: %4d %4d\r\n",rapidTriggerKeys[key].onLevel[i+1],rapidTriggerKeys[key].onLevel[i]);

    }


    rapidTriggerKey[key].lastStep = -1;
    rapidTriggerKey[key].triggeredLevel = 0;
	//memcpy (rapidTriggerKeys[key].keycode,keycode,counts*2);
	event.key.row = rapidTriggerKey[key].key.row;
	event.key.col = rapidTriggerKey[key].key.col;
	//uprintf ("rapidTriggerKey row:%d col:%d counts:%d keycode:%04X\r\n",rapidTriggerKey[key].key.row, rapidTriggerKey[key].key.col, rapidTriggerKey[key].key.counts,keymap_key_to_keycode(biton32(layer_state),event.key));
	for (uint8_t i=0;i<rapidTriggerKey[key].key.counts;i++){
		//uprintf ("onLevel  %d:%0d\t",i , rapidTriggerKey[key].onLevel[i]);
	}
	//uprintf ("\r\n");
	for (uint8_t i=0;i<rapidTriggerKey[key].key.counts-1;i++){
		//uprintf ("offLevel %d:%0d\t",i , rapidTriggerKey[key].offLevel[i]);
	}
	//uprintf ("\r\n");
	for (uint8_t i=0;i<rapidTriggerKey[key].key.counts;i++){
		//uprintf ("Keycode %d:%04x\t",i , rapidTriggerKey[key].key.keycode[i]);
	}
	//uprintf ("\r\n");
	return 0;
}

void rapidTriggerProcess(bool mode, uint8_t key){
	if (mode){
		for (uint8_t i=0;i<RAPID_TRIGGER_COUNT;i++){
			rapidTriggerKeyBuilder(i);
		}
	} else {
		rapidTriggerKeyBuilder(key);
	}
}

//typedef struct {
//	//uint8_t layer;
//	uint8_t row;
//	uint8_t col;
//    uint8_t counts;
//    uint16_t level[rapidTrigger_Levels];
//    uint16_t keycode[rapidTrigger_Levels];
//    uint8_t lastStep;
//    uint16_t lastKeycode;
//} rapidTrigger_config_t;

void matrix_init(void)//
{
	//uprintf ("rapidTrigger_config_t size:%d\r\n",sizeof(rapidTrigger_config_t));

//	rapidTrigger_config_t rapidTrigger;
//
////	eeconfig_rapidTrigger_init();
////
//	rapidTrigger.enable = true;
//	rapidTrigger.row = 1;
//	rapidTrigger.col = 2;
//	rapidTrigger.counts = 5;
//	rapidTrigger.keycode[0] =0x000a;
//	rapidTrigger.keycode[1] =0x001a;
//	rapidTrigger.keycode[2] =0x002a;
//	rapidTrigger.keycode[3] =0x003a;
//	rapidTrigger.keycode[4] =0x004a;
//	rapidTrigger.keycode[5] =0x005a;
//
//
//
//
//	eeconfig_write_rapidTrigger(0,rapidTrigger);
////	eeconfig_write_rapidTrigger(3,rapidTrigger);
////	eeconfig_write_rapidTrigger(9,rapidTrigger);
////	eeconfig_write_rapidTrigger(16,rapidTrigger);
//
//
//
//	for (uint8_t i=0;i<RAPID_TRIGGER_COUNT;i++){ // print dynamic keystroke example - lyon
//		rapidTrigger = eeconfig_read_rapidTrigger(i);
//		uprintf ("enable:%d row:%d col:%d counts:%d code[0]:%x code[1]:%x code[2]:%x code[3]:%x code[4]:%x code[5]:%x\r\n",
//				rapidTrigger.enable,rapidTrigger.row,rapidTrigger.col,rapidTrigger.counts
//				,rapidTrigger.keycode[0],rapidTrigger.keycode[1],rapidTrigger.keycode[2],rapidTrigger.keycode[3],rapidTrigger.keycode[4],rapidTrigger.keycode[5]);
//	}
//




	if (globleTrigger){
//		keycodeOutput = false;
//		keyScanEffect = true;
//		rapidKeyEnable = false;
	} else {
//		keycodeOutput = true;
//		keyScanEffect = false;
	}
//#ifdef nonOP
//	keycodeOutput = false;
//#else
//	keycodeOutput = true;
//#endif
	keycodeOutput = true;	//===> startup stop keycode output

	if (!readPin(A15)){ //lyon dont know what is this ask tony later
		keycodeOutput = false;
	}

    //Reade Iniit/Travel postion calibration data

	#ifdef saocd_void
	void keyboard_post_init_user(void){
		socd_init();
		socd_set_priority(SOCD_PRIORITY_LAST_INPUT);
	}
	#endif


    if (!eepromInitCheck){
    	uprintf("Factory Reset start\r\n");
    	write_keymap_to_eeprom();
    	eeconfig_rapidTrigger_init();
    	dynamic_macro_reset_eeprom();
    	eeprom_write_word(EECONFIG_KEYMAP_TRIGGER_LEVEL, DEFAULT_TRIGGER_LEVEL);
    	eeprom_write_word(EECONFIG_KEYMAP_RELEASE_LEVEL, DEFAULT_RELEASE_LEVEL);
    	write_trigger_to_eeprom();
    	write_Release_to_eeprom();
    	eeconfig_rapidTrigger_init();
    	adcOffsetCalc(true, true, 1500);
		eeconfig_socd_clear();
    	uprintf("Factory Reset end\r\n");
//    	NVIC_SystemReset();
    }
    chThdCreateStatic(waADC_ProcessThread, sizeof(waADC_ProcessThread), NORMALPRIO - 4, ADC_ProcessThread, NULL);
    LED_init();
	knobState = eeprom_read_byte(KB_ENCODER_MODE_ADDR);

	// currentLED_RunMode.mode = 8; // original 8 lyon
    // currentLED_RunMode.brightness = 3;//1;	//max 8 original 3
    // currentLED_RunMode.speed = 1;		//slow 1 fast 6
    // currentLED_RunMode.type = offMode;//normalMode;//offMode;
	// setRunMode(currentLED_RunMode);
	
	// currentLED_RunMode.mode = 0;          // 指定 static 效果 preset 模式 0
	// currentLED_RunMode.brightness = 3;    // 可自行調整 1~10
	// currentLED_RunMode.speed = 2;         // Static 不用 speed，但需填參數
	// currentLED_RunMode.type = normalMode; // 開啟正常動畫
	// setRunMode(currentLED_RunMode);

	// LED_ModesBuilder(newPreset, true, EffectTypeMosArt, true, 0, single, mosArtStatic, rainbowColor[red], 5, 1, false);


	

	// currentLED_RunMode.type = normalMode;
	// currentLED_RunMode.mode = 0;
	// currentLED_RunMode.brightness = 5;
	// currentLED_RunMode.speed = 1;
	// setRunMode(currentLED_RunMode);
}



uint8_t matrix_scan(void)
{
static counts = 0;
	//chThdSleepMilliseconds(1);
//	chBSemWait(&matrixScanSem);
	uprintf ("Matrix scan\r\n");
	keyboard_taskTMR();
	macro_engine_task();
//	if (counts++<5000){ss
//		chThdSleepMilliseconds(1);
//	}
//	for (uint32_t i=0;i>10000000;i++);
    return 1;
}

bool matrix_is_on(uint8_t row, uint8_t col)
{
    return (matrix[row] & (1 << col));
}

matrix_row_t matrix_get_row(uint8_t row)
{
    return matrix[row];
}

//matrix_row_t matrix_get_row(uint8_t row) {
//static bool speedTest = false;
//static uint16_t key;
////	if (speedTest){
////		matrix[3] = 0x08;
////	} else {
////		matrix[3] = 0x00;
////	}
////	speedTest = !speedTest;
////	if (key ==0){key = 1;}
////	matrix[3] = key;
////	key<<=1;
//	return matrix[row];
//}

void rapidKeyIndicate(void){
	//LED_Datas[ledKeyMatrix[keyLocation.rowA][keyLocation.colA]]
	ledAllOff(true);
	for (uint8_t i = 0; i < RAPID_TRIGGER_COUNT; i++){
		if (rapidTriggerKey[i].key.counts != 0){
			if (rapidTriggerKey[i].key.enable){
				LED_Datas[ledKeyMatrix[rapidTriggerKey[i].key.row][rapidTriggerKey[i].key.col]] = rainbowColor[rapidTriggerKey[i].key.counts-1];
			} else {
				LED_Datas[ledKeyMatrix[rapidTriggerKey[i].key.row][rapidTriggerKey[i].key.col]] = LED_NULL;
			}
		}
	}

	chBSemSignal(&ledUpdateSem);
//	chThdSleepMilliseconds(500);
//	ledAllOff(true);
}

void gamePadIndicate(void)
{
	gameModeEnable = true;


	for (uint8_t i = 0; i < MATRIX_ROWS; i++)
	{
		for (uint8_t j = 0; j < MATRIX_COLS; j++)
		{
			//			uprintf ("%04X ", eeconfig_read_keymapBuffer_key(2,i,j));
			switch (eeconfig_read_keymapBuffer_key(2, i, j))
			{

				//			case KC_GAME_PAD_DPAD_UP ... KC_GAME_PAD_BUTTON_31:
				//				LED_Datas[ledKeyMatrix[i][j]] = rainbowColor[j%6];
				//			break;

			case KC_GAME_PAD_DPAD_UP ... KC_GAME_PAD_DPAD_RIGHT:
				LED_Datas[ledKeyMatrix[i][j]] = rainbowColor[5];
				break;

			case KC_GAME_PAD_ANALOG_X_PLUSE:
			case KC_GAME_PAD_ANALOG_X_MINUS:
				LED_Datas[ledKeyMatrix[i][j]] = rainbowColor[violet];
				eepromTriggerBuffer[i][j] = 15;
				break;

			case KC_GAME_PAD_ANALOG_Y_PLUSE:
			case KC_GAME_PAD_ANALOG_Y_MINUS:
				LED_Datas[ledKeyMatrix[i][j]] = rainbowColor[violet];
				eepromTriggerBuffer[i][j] = 15;
				break;

			case KC_GAME_PAD_ANALOG_Z_PLUSE:
			case KC_GAME_PAD_ANALOG_Z_MINUS:
				LED_Datas[ledKeyMatrix[i][j]] = rainbowColor[white];
				eepromTriggerBuffer[i][j] = 15;
				break;

			case KC_GAME_PAD_ANALOG_RZ_PLUSE:
			case KC_GAME_PAD_ANALOG_RZ_MINUS:
				LED_Datas[ledKeyMatrix[i][j]] = rainbowColor[white];
				eepromTriggerBuffer[i][j] = 15;
				break;

			case KC_GAME_PAD_BUTTON_0:
				LED_Datas[ledKeyMatrix[i][j]] = rainbowColor[blue];
				break;

			case KC_GAME_PAD_BUTTON_1:
				LED_Datas[ledKeyMatrix[i][j]] = rainbowColor[green];
				break;

			case KC_GAME_PAD_BUTTON_2:
				LED_Datas[ledKeyMatrix[i][j]] = rainbowColor[red];
				break;

			case KC_GAME_PAD_BUTTON_3:
				LED_Datas[ledKeyMatrix[i][j]] = rainbowColor[yellow];
				break;

			case KC_GAME_PAD_BUTTON_4:
			case KC_GAME_PAD_BUTTON_5:
				LED_Datas[ledKeyMatrix[i][j]] = rainbowColor[orange];
				break;

			case KC_GAME_PAD_BUTTON_6:
			case KC_GAME_PAD_BUTTON_7:
				LED_Datas[ledKeyMatrix[i][j]] = rainbowColor[orange];
				break;

			case KC_GAME_PAD_BUTTON_8...KC_GAME_PAD_BUTTON_30:
				LED_Datas[ledKeyMatrix[i][j]] = rainbowColor[white];
				break;

			case QK_SYSTEMRESET:
				LED_Datas[ledKeyMatrix[i][j]] = rainbowColor[red];
				break;
			}
		}
	}

	chBSemSignal(&ledUpdateSem);
}

bool process_gamepad(keyrecord_t record, uint16_t keycode){
uint8_t * gamepad_report_Index = &extra_game_report;
static hatSwTest = 0;

	switch (keycode){
		case KC_GAME_PAD_DPAD_UP ... KC_GAME_PAD_BUTTON_31:
			switch (keycode){

			case KC_GAME_PAD_DPAD_UP:
				gamePadHat.up = record.event.pressed;
				break;
			case KC_GAME_PAD_DPAD_DOWN:
				gamePadHat.down = record.event.pressed;
				break;
			case KC_GAME_PAD_DPAD_LEFT:
				gamePadHat.left = record.event.pressed;
				break;
			case KC_GAME_PAD_DPAD_RIGHT:
				gamePadHat.right = record.event.pressed;
				break;
			case KC_GAME_PAD_ANALOG_X_PLUSE ... KC_GAME_PAD_ANALOG_RY_MINUS:
				if (record.event.pressed){
					switch (keycode){
						case KC_GAME_PAD_ANALOG_X_PLUSE:
						case KC_GAME_PAD_ANALOG_X_MINUS:
							switch (keycode){
							case KC_GAME_PAD_ANALOG_X_PLUSE:
								if (!gameAnalogStickStatus.x_minus){
									gameAnalogStickStatus.x_plus = true;
									gameAnalogValue.x_plus.event = record.event;
								}
								break;
							case KC_GAME_PAD_ANALOG_X_MINUS:
								if (!gameAnalogStickStatus.x_plus){
									gameAnalogStickStatus.x_minus = true;
									gameAnalogValue.x_minus.event = record.event;
								}
								break;
							}
							break;
						case KC_GAME_PAD_ANALOG_Y_PLUSE:
						case KC_GAME_PAD_ANALOG_Y_MINUS:
							switch (keycode){
							case KC_GAME_PAD_ANALOG_Y_PLUSE:
								if (!gameAnalogStickStatus.y_minus){
									gameAnalogStickStatus.y_plus = true;
									gameAnalogValue.y_plus.event = record.event;
								}
								break;
							case KC_GAME_PAD_ANALOG_Y_MINUS:
								if (!gameAnalogStickStatus.y_plus){
									gameAnalogStickStatus.y_minus = true;
									gameAnalogValue.y_minus.event = record.event;
								}
								break;
							}
							break;

						case KC_GAME_PAD_ANALOG_Z_PLUSE:
						case KC_GAME_PAD_ANALOG_Z_MINUS:
							switch (keycode){
							case KC_GAME_PAD_ANALOG_Z_PLUSE:
								if (!gameAnalogStickStatus.z_minus){
									gameAnalogStickStatus.z_plus = true;
									gameAnalogValue.z_plus.event = record.event;
								}
								break;
							case KC_GAME_PAD_ANALOG_Z_MINUS:
								if (!gameAnalogStickStatus.z_plus){
									gameAnalogStickStatus.z_minus = true;
									gameAnalogValue.z_minus.event = record.event;
								}
								break;
							}
							break;
						case KC_GAME_PAD_ANALOG_RZ_PLUSE:
						case KC_GAME_PAD_ANALOG_RZ_MINUS:
							switch (keycode){
							case KC_GAME_PAD_ANALOG_RZ_PLUSE:
								if (!gameAnalogStickStatus.Rz_minus){
									gameAnalogStickStatus.Rz_plus = true;
									gameAnalogValue.Rz_plus.event = record.event;
								}
								break;
							case KC_GAME_PAD_ANALOG_RZ_MINUS:
								if (!gameAnalogStickStatus.Rz_plus){
									gameAnalogStickStatus.Rz_minus = true;
									gameAnalogValue.Rz_minus.event = record.event;
								}
								break;
							}
							break;
						}

					}
					else {
						switch (keycode){
							case KC_GAME_PAD_ANALOG_X_PLUSE:
							case KC_GAME_PAD_ANALOG_X_MINUS:
								switch (keycode){
								case KC_GAME_PAD_ANALOG_X_PLUSE:
									gameAnalogStickStatus.x_plus = false;
									gameAnalogValue.x_plus.event = record.event;
									break;
								case KC_GAME_PAD_ANALOG_X_MINUS:
									gameAnalogStickStatus.x_minus = false;
									gameAnalogValue.x_minus.event = record.event;
									break;
								}
								break;
							case KC_GAME_PAD_ANALOG_Y_PLUSE:
							case KC_GAME_PAD_ANALOG_Y_MINUS:
								switch (keycode){
								case KC_GAME_PAD_ANALOG_Y_PLUSE:
									gameAnalogStickStatus.y_plus = false;
									gameAnalogValue.y_plus.event = record.event;
									break;
								case KC_GAME_PAD_ANALOG_Y_MINUS:
									gameAnalogStickStatus.y_minus = false;
									gameAnalogValue.y_minus.event = record.event;
									break;
								}
								break;
							case KC_GAME_PAD_ANALOG_Z_PLUSE:
							case KC_GAME_PAD_ANALOG_Z_MINUS:
								switch (keycode){
								case KC_GAME_PAD_ANALOG_Z_PLUSE:
									gameAnalogStickStatus.z_plus = false;
									gameAnalogValue.z_plus.event = record.event;
									break;
								case KC_GAME_PAD_ANALOG_Z_MINUS:
									gameAnalogStickStatus.z_minus = false;
									gameAnalogValue.z_minus.event = record.event;
									break;
								}
								break;
							case KC_GAME_PAD_ANALOG_RZ_PLUSE:
							case KC_GAME_PAD_ANALOG_RZ_MINUS:
								switch (keycode){
								case KC_GAME_PAD_ANALOG_RZ_PLUSE:
									gameAnalogStickStatus.Rz_plus = false;
									gameAnalogValue.Rz_plus.event = record.event;
									break;
								case KC_GAME_PAD_ANALOG_RZ_MINUS:
									gameAnalogStickStatus.Rz_minus = false;
									gameAnalogValue.Rz_minus.event = record.event;
									break;
								}
								break;
							}
					}
				return;
			break;

			case KC_GAME_PAD_BUTTON_0:
					extra_game_report.BTN_GamePadButton1 = record.event.pressed;
				break;
				case KC_GAME_PAD_BUTTON_1:
					extra_game_report.BTN_GamePadButton2 = record.event.pressed;
				break;
				case KC_GAME_PAD_BUTTON_2:
					extra_game_report.BTN_GamePadButton3 = record.event.pressed;
				break;
				case KC_GAME_PAD_BUTTON_3:
					extra_game_report.BTN_GamePadButton4 = record.event.pressed;
				break;
				case KC_GAME_PAD_BUTTON_4:
					extra_game_report.BTN_GamePadButton5 = record.event.pressed;
				break;
				case KC_GAME_PAD_BUTTON_5:
					extra_game_report.BTN_GamePadButton6 = record.event.pressed;
				break;
				case KC_GAME_PAD_BUTTON_6:
					extra_game_report.BTN_GamePadButton7 = record.event.pressed;
				break;
				case KC_GAME_PAD_BUTTON_7:
					extra_game_report.BTN_GamePadButton8 = record.event.pressed;
				break;
				case KC_GAME_PAD_BUTTON_8:
					extra_game_report.BTN_GamePadButton9 = record.event.pressed;
				break;
				case KC_GAME_PAD_BUTTON_9:
					extra_game_report.BTN_GamePadButton10 = record.event.pressed;
				break;
				case KC_GAME_PAD_BUTTON_10:
					extra_game_report.GD_GamePadSystemControlSystemMainMenu = record.event.pressed;
			}
//			X += 1000;
//			Y -= 1000;
//			Rx += 1000;
//			Ry -= 1000;
//			Z += 100;
//			Rz -= 100;
			extra_game_report.GD_GamePadHatSwitch = hatSwitchMap[gamePadHat.raw];
			////////uprintf ("raw:%d up:%d dn:%d left:%d right:%d hat sw:%d\r\n",gamePadHat.raw, gamePadHat.up, gamePadHat.down, gamePadHat.left, gamePadHat.right, extra_game_report.GD_GamePadHatSwitch);
			//extra_game_report.GD_GamePadHatSwitch = hatSwTest % 9;
			//uprintf ("hatSwTest:%d\r\n",hatSwTest % 9);

//			//debug out
//			if (record.event.pressed){
//				hatSwTest++;
//				uprintf ("game pad code:%X pressed\r\n",keycode);
//			}
//			else {
//				uprintf ("game pad code:%X released\r\n",keycode);
//			}
//			for (uint8_t i=0; i<sizeof(extra_game_report);i++){
//				uprintf ("%02X ",*gamepad_report_Index++);
//			}
//			uprintf ("\r\n");
			send_gamepad(extra_game_report);
		break;
	}
	return true;
}

bool hook_matrix_change(keyrecord_t record)
{
	uint16_t keycode = keymap_key_to_keycode(biton32(layer_state),record.event.key);
	static uint16_t counts = 0;
	// uprintf("KEY row=%d col=%d kc=%04X pressed=%d\r\n",
	// 				record.event.key.row, record.event.key.col, keycode, record.event.pressed);
//	uprintf ("Layer:%d keycode:%04x\r\n", biton32(layer_state), keycode);

//    action_t action = layer_switch_get_action(record.event.key);
//    uint8_t code = action.key.code;
//    uprintf ("action keycode:%X modes:%X kid_id:%X\r\n",code,action.key.mods,action.kind.id);

    if (record.event.pressed && (currentLED_RunMode.mode == 5 )) { //|| currentLED_RunMode.mode == 7
        reactive_level[record.event.key.row][record.event.key.col] = REACTIVE_MAX;
    }

	if (record.event.pressed && currentLED_RunMode.mode == 7) {
		// led_config_t ripple_color;
		// ripple_color.red = 180;
		// ripple_color.green = 220;
		// ripple_color.blue = 255;
		// ripple_add_event(record.event.key.row, record.event.key.col, ripple_color);
		ripple_add_event(record.event.key.row, record.event.key.col, c);
	}
	#ifdef socd
	if(isSOCD){
		int8_t slot = socd_find_slot_by_keycode(keycode);
		if (slot >= 0) {
			socd_post_event(slot, keycode, record.event.pressed);
			return false;   // SOCD 接管
		}
	}
	#endif

	bool resault;
	resault = true;
    if (!process_record_dynamic_macro(record)) {
      return false;
    }

    if (IS_GAMEPAD(keycode)){
    	process_gamepad(record, keycode);
    	return false;
    }
//    if (keycode!=0x0047){
//    	uprintf ("Count:%5d Keycode:%04X state:%d\r\n",counts++,keycode,record.event.pressed);
//    } else {
//    	return;
//    }


	/////!!!! uprintf ("Layer:%d row:%d col:%d state:%d keycode:%04X\r\n",biton32(layer_state), record.event.key.row, record.event.key.col, record.event.pressed, keycode);
	//return true;

	//uprintf ("Layer state:%d keycode:%04X\r\n", biton32(layer_state),keycode);

	void dump_eeprom(void)
	{
		uint8_t buffer[32];

		uprintf("\r\n=== EEPROM DUMP START ===\r\n");

		for (uint16_t addr = EEPROM_MACRO_SLOT_ADDR(0); addr < EEPROM_MACRO_SLOT_ADDR(1)/*EEPROM_SIZE*/; addr += 32)
		{
			eeprom_read_block(buffer, (const void*)addr, 32);

			uprintf("%04X: ", addr);

			for (uint8_t i = 0; i < 32; i++)
			{
				uprintf("%02X ", buffer[i]);
			}

			uprintf("\r\n");
			chThdSleepMilliseconds(1);  // 避免卡死
		}

		uprintf("=== EEPROM DUMP END ===\r\n");
	}
	void eeprom_write8_verify(uint16_t addr, uint8_t value)
	{
		uint8_t before = eeprom_read_byte((const uint8_t*)addr);

		eeprom_update_byte((uint8_t*)addr, value);

		uint8_t after  = eeprom_read_byte((const uint8_t*)addr);

		uprintf("[EEP] W8  addr=0x%04X  before=0x%02X  write=0x%02X  after=0x%02X  %s\r\n",
				addr, before, value, after, (after == value) ? "OK" : "FAIL");
	}
	// static inline uint8_t clamp_u8_i16(int16_t x)
	// {
	// 	if (x < 0)   return 0;
	// 	if (x > 255) return 255;
	// 	return (uint8_t)x;
	// }
	void plusHue(led_config_t in){
		hsv = rgb_to_hsv(in);
		current_hsv.h = (current_hsv.h + 15) % 360;
    	mosArt.LED = hsv_to_rgb(current_hsv);
	}
	void minusHue(led_config_t in){
		hsv = rgb_to_hsv(in);
		current_hsv.h = (current_hsv.h + 345) % 360;
    	mosArt.LED = hsv_to_rgb(current_hsv);
	}
	void plusSat(led_config_t in){
		current_hsv.s = (current_hsv.s > 239) ? 255 : current_hsv.s + 16;
    	mosArt.LED = hsv_to_rgb(current_hsv);
	}
	void minusSat(led_config_t in){
		current_hsv.s = (current_hsv.s < 16) ? 0 : current_hsv.s - 16;
    	mosArt.LED = hsv_to_rgb(current_hsv);
	}
	void plusBri(led_config_t in){
		current_hsv.v = (current_hsv.v > 239) ? 255 : current_hsv.v + 16;
    	mosArt.LED = hsv_to_rgb(current_hsv);
	}
	void minusBri(led_config_t in){
		current_hsv.v = (current_hsv.v < 16) ? 0 : current_hsv.v - 16;
    	mosArt.LED = hsv_to_rgb(current_hsv);
	}

		if (record.event.pressed){
			//uprintf ("keymatrix change pressed code:%04X\r\n",keycode);
			switch(keycode){
#ifdef buttonLock
				case KC_LGUI://KC_LALT:
					if(winLock){
						uprintf ("try to skip win");
						return false;
					}
				break;
				case KC_TAB:{
					uint8_t mods = get_mods();  // 目前按著的修飾鍵

					if(altTabLock){
						if (mods & (MOD_BIT(KC_LALT) | MOD_BIT(KC_RALT))) {
							uprintf("skip alt+tab\r\n");
							return false;
						}
					}
					
				}
				break;
				case KC_F4:{
					uint8_t mods = get_mods();  // 目前按著的修飾鍵

					if(altF4Lock){//altF4Lock){
						if (mods & (MOD_BIT(KC_LALT) | MOD_BIT(KC_RALT))) {
							uprintf("skip alt+F4\r\n");
							return false;
						}
					}
				}
				break;
#endif // buttonLock
				case KC_L:
					// uprintf("TEST: KC_L pressed\r\n");
					// socd_test_write_eeprom_once();
					// void socd_load_all_from_eeprom(void);
					// mt_dump_map();
					// uprintf ("Trigger Level:%d output:%d btn:%d release Level:%d\r\n",keyMatrixTriggerLevel, keycodeOutput, readPin(A15),keyMatrixReleaseLevel);
				break;
				case KC_MBRI:
				// 	return false; // 我已處理
				// }
				// return true;
					// send_string("abcdefghijklmnopqrstuvwxyz\n"); /**/
					// send_A_to_Z();
					// dump_eeprom();
					minusBri(mosArt.LED);
					uprintf("load_all_rappy_snappy\r\n");
					void load_all_rappy_snappy(void);
				break;
				case KC_PBRI:
					// eeprom_write8_verify
					// eeprom_write8_verify(EECONFIG_SOCD_ADDR_END, 0xFA);
					// uprintf("EECONFIG_SOCD_ADDR_END address is %d", EECONFIG_SOCD_ADDR_END);
					plusBri(mosArt.LED);
					// hsv = rgb_to_hsv(mosArt.LED);
					// hsv.h += 2;   // 每幀轉動色相
					// mosArt.LED = hsv_to_rgb(hsv);
				break;
				case KC_MSAT:
					minusSat(mosArt.LED);
				break;
				case KC_PSAT:
					plusSat(mosArt.LED);
				break;
				case KC_MHUE:
					minusHue(mosArt.LED);
				break;
				case KC_PHUE:
					plusHue(mosArt.LED);
				break;
				case KC_LM1:
					if(isCtrl){
						currentLED_RunMode.type = normalMode;
						currentLED_RunMode.mode = 0x00;
						setRunMode(currentLED_RunMode);
						eeprom_update_byte(KB_COLOR_MODE_ADDR, 0);
					}
				break;
				case KC_LM2:
					if(isCtrl){
						currentLED_RunMode.type = normalMode;
						currentLED_RunMode.mode = 0x01;
						setRunMode(currentLED_RunMode);
						eeprom_update_byte(KB_COLOR_MODE_ADDR, 1);
					}
				break;
				case KC_LM3:
					if(isCtrl){
						currentLED_RunMode.type = normalMode;
						currentLED_RunMode.mode = 0x02;
						setRunMode(currentLED_RunMode);
						eeprom_update_byte(KB_COLOR_MODE_ADDR, 2);
					}
				break;
				case KC_LM4:
					if(isCtrl){
						currentLED_RunMode.type = normalMode;
						currentLED_RunMode.mode = 0x03;
						setRunMode(currentLED_RunMode);
						eeprom_update_byte(KB_COLOR_MODE_ADDR, 3);
					}
				break;
				case KC_LM5:
					if(isCtrl){
						currentLED_RunMode.type = normalMode;
						currentLED_RunMode.mode = 0x04;
						setRunMode(currentLED_RunMode);
						eeprom_update_byte(KB_COLOR_MODE_ADDR, 4);
					}
				break;
				case KC_LM6:
					if(isCtrl){
						currentLED_RunMode.type = normalMode;
						currentLED_RunMode.mode = 0x05;
						setRunMode(currentLED_RunMode);
						eeprom_update_byte(KB_COLOR_MODE_ADDR, 5);
					}
				break;
				case KC_LM7:
					if(isCtrl){
						currentLED_RunMode.type = normalMode;
						currentLED_RunMode.mode = 0x06;
						setRunMode(currentLED_RunMode);
						eeprom_update_byte(KB_COLOR_MODE_ADDR, 6);
					}
				break;
				case KC_LM8:
					if(isCtrl){
						currentLED_RunMode.type = normalMode;
						currentLED_RunMode.mode = 0x07;
						setRunMode(currentLED_RunMode);
						eeprom_update_byte(KB_COLOR_MODE_ADDR, 7);
					}
				break;
				case KC_LM9:
					if(isCtrl){
						currentLED_RunMode.type = normalMode;
						currentLED_RunMode.mode = 0x08;
						setRunMode(currentLED_RunMode);
						eeprom_update_byte(KB_COLOR_MODE_ADDR, 8);
					}
				break;
				case KC_M1:
					// macro_engine_setup_test_copy_paste();
					macro_engine_start(0);
					// macro_engine_dump_eeprom_slot(0);
					uprintf("KC_M1 TRIGGER \r\n");
					// void dump_eeprom_range(uint32_t start, uint32_t end) {
					// 	uprintf("---- EEPROM DUMP (0x%04lX ~ 0x%04lX) ----\n",
					// 			(unsigned long)start, (unsigned long)end);

					// 	for (uint32_t addr = start; addr <= end; addr++) {
					// 		if ((addr % 16) == 0) {
					// 			uprintf("\n0x%04lX: ", (unsigned long)addr);
					// 		}

					// 		//uint8_t val = eeprom_read_byte((const void *)(uintptr_t)addr);
					// 		//uprintf("%02X ", val);
					// 	}

					// 	uprintf("\n\n-------------------------------\n");
					// }
					// dump_eeprom_range(EEPROM_MACRO_SLOT_ADDR(0),EEPROM_MACRO_SLOT_ADDR(1));
					// dump_eeprom();
					return false;
				break;
				case KC_M2:
					// macro_save_to_eeprom(0);
					uprintf("KC_M2 TRIGGER \r\n");
					// uprintf("matrix (%d)_(%d)_(%d),(%d)_(%d)_(%d)\r\n",
					// 	g_rappy_pairs[0].lay_a, g_rappy_pairs[0].row_a, g_rappy_pairs[0].col_a,
					// 	g_rappy_pairs[0].lay_b, g_rappy_pairs[0].row_b, g_rappy_pairs[0].col_b);
				break;
				case KC_M3:
					// eeconfig_rappy_snappy_clear();
					toggle_key_dump_eeprom();
				break;
				case KC_MACRO_C1:
					macro_engine_start(0);
				break;
				case KC_MACRO_C2:
					macro_engine_start(1);
				break;
				case KC_MACRO_C3:
					macro_engine_start(2);
				break;
				case KC_MACRO_C4:
					macro_engine_start(3);
				break;
				case KC_MACRO_C5:
					macro_engine_start(4);
				break;
				case KC_MACRO_C6:
					macro_engine_start(5);
				break;
				case KC_MACRO_C7:
					macro_engine_start(6);
				break;
				case KC_MACRO_C8:
					macro_engine_start(7);
				break;
				case KC_MACRO_C9:
					macro_engine_start(8);
				break;
				case KC_MACRO_C10:
					macro_engine_start(9);
				break;
				case KC_SOCD_TOGG:
					if(isSOCD){//IS_SOCD_ON_ADD == 0x01){
						isSOCD = false;
					} else {
						isSOCD = true;
					}
				break;
				case KC_EECLR:
					if (record.event.pressed) {
						eeclr_pressed = true;
						eeclr_fired = false;
						eeclr_timer = timer_read32();
					} else {
						eeclr_pressed = false;
						eeclr_fired = false;
					}
					return false; // 不往下送出一般按鍵
				break;
				case KC_WIN_LOCK:
				//eeprom keylock
				uprintf("before, KEYLOCK is %d\r\n", isKEYLOCK);
					if (isKEYLOCK) {      
						uint8_t mask = 0x00;
						winLock    = (mask & 0x01u) != 0;
						altTabLock = (mask & 0x02u) != 0;
						altF4Lock  = (mask & 0x04u) != 0;
						isKEYLOCK = false;
						// write to eeprom?
					} else {
						uint8_t mask = 0x01;
						winLock    = (mask & 0x01u) != 0;
						altTabLock = (mask & 0x02u) != 0;
						altF4Lock  = (mask & 0x04u) != 0;
						isKEYLOCK = true;
					}
				uprintf("after, KEYLOCK is %d\r\n", isKEYLOCK);
				break;
			// case first key: //F1
			// 	if (isMac) {
			// 		if (record->event.pressed) {
			// 			register_code(KC_BRID);
			// 		} else {
			// 			unregister_code(KC_BRID);
			// 		}
			// 		return false;
			// 	}
			// break;
			case KC_VOLD: //F2
				if (isMac) {
					if (record.event.pressed) {
						register_code(KC_BRIU);
					} else {
						unregister_code(KC_BRIU);
					}
					return false;
				}
			break;
			case KC_VOLU: //F3 Open Control Center
				if (isMac) {
					if (record.event.pressed) {
						register_code(KC_LCTL);
						tap_code(KC_UP);
						unregister_code(KC_LCTL);
					} 
					return false;
				}
			break;
			case KC_MUTE: //F4 Open/Close Launchpad
				if (isMac) {
					if (record.event.pressed) {
						register_code(KC_LCTL);
						tap_code(KC_F4);
						unregister_code(KC_LCTL);
					} 
					return false;
				}
			break;
			case KC_MSTP: //F5
				if (isMac) {
					if (record.event.pressed) {
						register_code(KC_RGB_VAD);
					} else {
						unregister_code(KC_RGB_VAD);
					}
					return false;
				}
			break;
			case KC_MEDIA_PREV_TRACK:  //F6
				if (isMac) {
					if (record.event.pressed) {
						register_code(KC_RGB_VAI);
					} else {
						unregister_code(KC_RGB_VAI);
					}
					return false;
				}
			break;
			case KC_MEDIA_PLAY_PAUSE: //F7
				if (isMac) {
					if (record.event.pressed) {
						register_code(KC_MEDIA_NEXT_TRACK);
					} else {
						unregister_code(KC_MEDIA_NEXT_TRACK);
					}
					return false;
				}
			break;
			case KC_MEDIA_NEXT_TRACK: //F8
				if (isMac) {
					if (record.event.pressed) {
						register_code(KC_MEDIA_PLAY_PAUSE);
					} else {
						unregister_code(KC_MEDIA_PLAY_PAUSE);
					}
					return false;
				}
			break;
			case KC_MAIL: //F9
				if (isMac) {
					if (record.event.pressed) {
						register_code(KC_MEDIA_PREV_TRACK);
					} else {
						unregister_code(KC_MEDIA_PREV_TRACK);
					}
					return false;
				}
			break;
			case KC_WWW_HOME: //F10
				if (isMac) {
					if (record.event.pressed) {
						register_code(KC_MUTE);
					} else {
						unregister_code(KC_MUTE);
					}
					return false;
				}
			break;
			case KC_CALCULATOR: //F11
				if (isMac) {
					if (record.event.pressed) {
						register_code(KC_VOLD);
					} else {
						unregister_code(KC_VOLD);
					}
					return false;
				}
			break;
			case KC_WWW_SEARCH: //F12
				if (isMac) {
					if (record.event.pressed) {
						register_code(KC_VOLU);
					} else {
						unregister_code(KC_VOLU);
					}
					return false;
				}
			break;
			case KC_MAC:
				isMac = 1;
				eeprom_write_word(EECONFIG_MAC_MODE, isMac);
			break;
			case KC_WINDOW:
				isMac = 0;
				eeprom_write_word(EECONFIG_MAC_MODE, isMac);
			break;
//			case KC_BRIU:
//				send_consumer(0x00BD);
//				break;
//				case KC_RGB_TOG:
//				//case	KC_X:
//                    if (!keyScanEffect)
//                    {
//                        currentLED_RunMode.type = offMode;
//                        keyScanEffect = true;
//                    }
//                    else
//                    {
//                    	keyScanEffect = false;
//                    	currentLED_RunMode.type = normalMode;
//                    }
//                    setRunMode(currentLED_RunMode);
//                    //keycodeOutput = !keycodeOutput;
//                    resault = false;
//					break;

//				case KC_1:
//					for (uint16_t i=0;i<50;i++){
//					tap_code16(0x0014);
//					}
//					return false;
//					break;

//				case KC_UP:
//					rainbowBase++;
//					break;
//				case KC_DOWN:
//					rainbowBase--;
//					break;

//				case KC_UP:
//					rainbowBase++;
//					uprintf ("%3d\r\n",rainbowBase);
//					break;
//				case KC_DOWN:
//					rainbowBase--;
//					uprintf ("%3d\r\n",rainbowBase);
//					break;



				case KC_RGB_TOG:
//					palSetPad(GPIOF, 7);
					rapidKeyEnable = false;
					if (currentLED_RunMode.type == offMode){
						ledUpdateFilter = true;
						currentLED_RunMode.type = normalMode;
						setRunMode(currentLED_RunMode);
						resault = false;
						break;
					}
					if (currentLED_RunMode.type == normalMode){
						//////ledUpdateFilter = false;
						currentLED_RunMode.type = offMode;
						setRunMode(currentLED_RunMode);
//						chThdSleepMilliseconds(50);
						ledAllOff(true);
						resault = false;
						break;
					}
					resault = false;
					break;
				case  KC_RGB_ON:
					if (currentLED_RunMode.type == offMode){
						currentLED_RunMode.type = normalMode;
						setRunMode(currentLED_RunMode);
					}
					resault = false;
				break;
				case KC_RGB_OFF:
					//keyScanEffect = false;
					rapidKeyEnable = false;
					if (currentLED_RunMode.type == normalMode){
						currentLED_RunMode.type = offMode;
						setRunMode(currentLED_RunMode);
						ledAllOff(true);
					}
					resault = false;
					break;
				case KC_RGB_MODE_RAINBOWFLOW ... KC_RGB_MODE_RGBTEST:
					if (currentLED_RunMode.type != dynamticLightingMode){
					    currentLED_RunMode.brightness = 8;
					    currentLED_RunMode.speed = 1;
					    switch (keycode){
							case KC_RGB_MODE_RAINBOWFLOW:
								if (currentLED_RunMode.type != dynamticLightingMode && lastLED_RunMode.mode!= 8 ){
									currentLED_RunMode.type = normalMode;
									currentLED_RunMode.mode = 8;
								}
								break;
							case KC_RGB_MODE_COLORSHIFT:
								if (currentLED_RunMode.type != dynamticLightingMode && lastLED_RunMode.mode!= 0 ){
									currentLED_RunMode.type = normalMode;
									currentLED_RunMode.mode = 0;
								}
								break;
							case KC_RGB_MODE_COLORBREATH:
								if (currentLED_RunMode.type != dynamticLightingMode && lastLED_RunMode.mode!= 1 ){
									currentLED_RunMode.type = normalMode;
									currentLED_RunMode.mode = 1;
								}
								break;
							case KC_RGB_MODE_FLASH:
								if (currentLED_RunMode.type != dynamticLightingMode && lastLED_RunMode.mode!= 2 ){
									currentLED_RunMode.type = normalMode;
									currentLED_RunMode.mode = 2;
								}
								break;
							case KC_RGB_MODE_SWIRL:
								break;
							case KC_RGB_MODE_KNIGHT:
								break;
							case KC_RGB_MODE_XMAS:
								break;
							case KC_RGB_MODE_GRADIENT:
								break;
							case KC_RGB_MODE_RGBTEST:
								if (currentLED_RunMode.type != dynamticLightingMode && lastLED_RunMode.mode!= 10 ){
									currentLED_RunMode.type = normalMode;
									currentLED_RunMode.mode = 10;
								}
								break;
					    }

					    setRunMode(currentLED_RunMode);

					}

				break;

		case KC_RAPID_TOG:
					if (!rapidKeyEnable){
#ifdef minTriggerCheck
//						palClearPad(GPIOA, 8);
#endif

						ledUpdateFilter = true;
//						ACTION_LAYER_OFF_ON(1);
//						tap_code16(KC_TO2);
						uprintf ("Rapid set layer to 1\r\n");
//						palClearPad(GPIOF, 7);
						if (currentLED_RunMode.type == normalMode){
							currentLED_RunMode.type = offMode;
							setRunMode(currentLED_RunMode);
						}
//						chThdSleepMilliseconds(25);
						lampArrayDisable = true;
						rapidKeyIndicate();
						rapidKeyEnable = true;
//						chThdSleepMilliseconds(175);
						//////ledUpdateFilter = false;
					} else {
						lampArrayDisable = false;
//						palSetPad(GPIOF, 7);
						ledUpdateFilter = true;
						if (lampArrayEffect){
							currentLED_RunMode.type = dynamticLightingMode;
						} else {
							currentLED_RunMode.type = normalMode;
						}

						setRunMode(currentLED_RunMode);
//						tap_code16(KC_TO0);
						uprintf ("Rapid set layer to 0\r\n");
#ifdef minTriggerCheck
//						palSetPad(GPIOA, 8);
#endif
						rapidKeyEnable = false;
					}
					resault = false;
					break;
				case KC_RAPID_ON:
					currentLED_RunMode.type = offMode;
					setRunMode(currentLED_RunMode);
//					chThdSleepMilliseconds(50);
					rapidKeyEnable = true;
					resault = false;
					break;
				case KC_RAPID_OFF:
					currentLED_RunMode.type = normalMode;
					setRunMode(currentLED_RunMode);
					rapidKeyEnable = false;
					resault = false;
					break;

				case KC_MO2:
					// currentLED_RunMode.type = offMode; // lyon close these three lines
					// setRunMode(currentLED_RunMode);
//					chThdSleepMilliseconds(50);
					// lampArrayDisable = true;
					// gamePadIndicate(); // disable
#ifdef minTriggerCheck
					rapidKeyEnable = true;
#endif
					break;

				case KC_DESK_TOP:
					register_code(KC_LGUI);
					tap_code_delay(KC_D, 10);
					unregister_code(KC_LGUI);
//				      clear_keyboard();
//				      register_code(KC_LGUI);
//				      chThdSleepMilliseconds(10);
//				      tap_code(KC_D);
//				      unregister_code(KC_LGUI);
					break;

				case KC_RGB_VAI ... KC_RGB_SPD:
			//if (!rapidKeyEnable && currentLED_RunMode.type == normalMode){
				if (!rapidKeyEnable){
					switch (keycode){
						case KC_RGB_VAI:
							++currentLED_RunMode.brightness;
							uprintf ("RGB Bright ++:%d\r\n",currentLED_RunMode.brightness);
							break;
						case KC_RGB_VAD:
							--currentLED_RunMode.brightness;
							uprintf ("RGB Bright --:%d\r\n",currentLED_RunMode.brightness);
							break;
						case KC_RGB_SPI:
						case KC_RAPID_LVI:
							--currentLED_RunMode.speed;
							break;
						case KC_RGB_SPD:
						case KC_RAPID_LVD:
							++currentLED_RunMode.speed;
							break;
					}
						if (currentLED_RunMode.brightness > 9){
							currentLED_RunMode.brightness = 9;
						}
						if (currentLED_RunMode.brightness < 1){
							currentLED_RunMode.brightness = 1;
						}
						if (currentLED_RunMode.speed < 1){
							currentLED_RunMode.speed = 1;
						}
						if (currentLED_RunMode.speed > 4){
							currentLED_RunMode.speed = 4;
						}
						LED_SpeedChange(currentLED_RunMode.brightness, currentLED_RunMode.speed);
						//setRunMode(currentLED_RunMode);
						resault = false;
						break;
					}
					else{

					}
				break;

				case QK_BOOTLOADER:
			        currentLED_RunMode.type = offMode;
			        setRunMode(currentLED_RunMode);
		            chThdSleepMilliseconds(50);
		            ledAllOff(true);
					break;

				case QK_SYSTEMRESET:
			        currentLED_RunMode.type = offMode;
			        setRunMode(currentLED_RunMode);
//		            chThdSleepMilliseconds(50);
                	NVIC_SystemReset();
					break;

			}
		} else {
			switch(keycode){
			case KC_PROFILE0 ... KC_PROFILE4:
				{
					uint8_t index = keycode - KC_PROFILE0;
					uint8_t profileLayer;
					//uprintf ("KC Profile:%d pressed\r\n",keycode - KC_PROFILE1+1);
					currentProfile = index;
					//defaultLayer = eeconfig_read_default_layer();
					profileLayer = index * LAYERS_PER_PROFILE;
					//if (defaultLayer != profileLayer){
						//eeconfig_update_default_layer(profileLayer);
						//uprintf("Update default:%d\r\n",profileLayer);
						defaultLayer = profileLayer;
						defaultLayerChanged = true;
					//}
					//uprintf("Profile:%d default Layer:%d\r\n", currentProfile, profileLayer);
					setChangeProfileEffect(true,record.event.key.row,record.event.key.col,rainbowColor[blue]);
				}
				return false;
			break;

			//change profile
			case KC_MO0 ... KC_MO15:
			case KC_ROLL_OVER:
				if (defaultLayerChanged && layer_switch_get_action(record.event.key).kind.id == 0x0A){
					defaultLayerChanged = false;
					layer_move(defaultLayer);
					uprintf ("Set Profile:%d \r\n",currentProfile);
					clear_keyboard();
					setChangeProfileEffect(false,record.event.key.row,record.event.key.col,LED_NULL);
					return false;
				}
			break;
			}
		}

		if (!keycodeOutput){ resault = false;}

	return resault;
}

void matrix_print(void)
{
    uprintf("\033[2J\033[H");   //VT100 Clr Screen
    uprintf("\r\nr/c 0123456789ABCDE\r\n");
    for (uint8_t row = 0; row < MATRIX_ROWS; row++)
    {
        uprintf("%02X: ", row);
        matrix_row_t data = matrix_get_row(row);
        for (int col = 0; col < MATRIX_COLS; col++)
        {
            if (data & (1 << col))
                uprintf("1");
            else
                uprintf("0");
            //				host_consumer_send(0x00e9);
            //				host_consumer_send(0x0000);
        }
        uprintf("\r\n");
    }
}

void analogKeyMatrixConvert(uint8_t group, uint8_t location, uint8_t level)
{
    //analogKey_Config_t keyLocation;
    uprintf("Row:%2d  Col:%2d  ", keyLocation.rowA, keyLocation.colA);
    if (location > colMax[group])
    {
        uprintf("Skip\r\n");
        return;
    }
    keyLocation = analogKeyRelocate[group][location];
    if (level > keyMatrixTriggerLevel)
    {
        matrix[keyLocation.rowA] |= (0x01 << keyLocation.colA);
        uprintf("On\r\n");
    }
    else
    {
        matrix[keyLocation.rowA] &= ~(0x01 << keyLocation.colA);
        uprintf("Off\r\n");
    }
    //matrix_print();
}


bool encoder_update_kb(uint8_t index, uint8_t status)
{
    if (!encoder_update_user(index, status))
    {
        return false; /* Don't process further events if user function exists and returns false */
    }
    switch (index)
    {
        case 0:                         /* First encoder */
			// setKnobState(status, knobState);
            switch (status)
            {
                case ENCODER_CLOCKWISE:
					if(knobState == 0){
						tap_code_delay(KC_VOLU, 10);
					}else if(knobState == 1){
						currentLED_RunMode.mode++;
                		setRunMode(currentLED_RunMode);
					}else if(knobState == 2){

					}else if(knobState == 3){
						tap_code_delay(KC_MNXT, 10);
					}
                	//if (keycodeOutput)
                    //tap_code(KC_VOLU);
                    // tap_code_delay(KC_VOLU, 10);
                    //uprintf ("ENCODER_CLOCKWISE\r\n");
                    break;
                case ENCODER_COUNTER_CLOCKWISE:
					if(knobState == 0){
						tap_code_delay(KC_VOLD, 10);
					}else if(knobState == 1){
						currentLED_RunMode.mode--;
                		setRunMode(currentLED_RunMode);
					}else if(knobState == 2){

					}else if(knobState == 3){
						tap_code_delay(KC_MPRV, 10);
					}
                	//if (keycodeOutput)
                	//uprintf ("ENCODER_COUNTER_CLOCKWISEr\n");
                	// tap_code_delay(KC_VOLD, 10);
                    //tap_code(KC_VOLD);
                    break;

                case ENCODER_BUTTON:
					if(knobState == 0){
                		tap_code(KC_MUTE);
					}else if(knobState == 1){
						// currentLED_RunMode.mode++;
                		// setRunMode(currentLED_RunMode);
					}else if(knobState == 2){

					}else if(knobState == 3){

					}
                	// keycodeOutput = !keycodeOutput;
                	// tap_code(KC_MUTE);
//                	ledOn_off_test = !ledOn_off_test;
//                	led_ctrl(ledOn_off_test);
//					currentLED_RunMode.type = offMode;
//					setRunMode(currentLED_RunMode);
//                	gamePadIndicate();
                    break;


            }
            //chThdSleepMilliseconds(25);
            break;
    }

    return true;
}

void hook_usb_wakeup(void)// try this lyon
{
    currentLED_RunMode.type = normalMode;
    setRunMode(currentLED_RunMode);
}

void hook_usb_suspend_entry(void)
{
    if (timer_read() > 1500)
    {
        currentLED_RunMode.type = powerOff;//offMode;   // 原本是 powerOff; // lyon try to fix reboot problem
        setRunMode(currentLED_RunMode);
    }
}

#ifdef rappySnappy
#endif // rappySnappy


#ifdef rappyTrigger
bool process_rappy_fixed_ratio(rappy_fixed_ratio_t *rappy) {
    if (!rappy->enabled) return false;

    analogKey_Config_t info = matrixAnalogRelocate[rappy->row][rappy->col];
    int adc = samples2[info.colA][info.rowA];
    int init = initPostionLevel[info.colA][info.rowA];
    int travel = travelPostionLevel[info.colA][info.rowA] - init;
    if (travel <= 0) travel = 1;

    float ratio = (float)(adc - init) / travel;
    if (ratio < 0.0f) ratio = 0.0f;
    if (ratio > 1.0f) ratio = 1.0f;
	
	if (!rappy->pressed) {
		if (ratio >= rappy->trigger_ratio) {
			rappy->pressed = true;
			register_code(rappy->keycode);
			uprintf(">>> press %.2f\r\n", ratio);
		}
	} else {
		// 已經是按下狀態 → 要到明顯低於 release_ratio 才放開
		if (ratio <= rappy->release_ratio) {
			rappy->pressed = false;
			unregister_code(rappy->keycode);
			uprintf("<<< release %.2f\r\n", ratio);
		}
	}


    return true;
}

#endif // rappyTrigger
