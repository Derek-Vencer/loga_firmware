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
#define TRAVEL_STEPS 100

//
extern led_runMode_t currentLED_RunMode, lastLED_RunMode;
extern binary_semaphore_t ledUpdateSem;
extern led_config_t LED_Datas[LedNumbers];
extern report_gamepad_report extra_game_report;
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

// void send_string(const char *str) {
//     while (*str) {
//         host_keyboard_send_char(*str++);
//     }
// }


#define ADC_v5
// #define ADC_v6
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

uint8_t knobState = 0; // lyon for encoder
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
#ifdef rappySnappy //


// Snappy 鍵位設定（可自行改）rappy snappy
// /*static*/ keypos_t snappy_key1 = {1, 3};  // 對應 KC_A // (COL, ROW)? WHY IS THIS HAPPEN?
// /*static*/ keypos_t snappy_key2 = {2, 3};  // 對應 KC_B
// static const uint16_t keycode1 = KC_1;
// static const uint16_t keycode2 = KC_2;

// layer_keypos_t snappy_key1 = {0, 3, 1};  // {0,10,10};//{0, 3, 1};  // 對應 KC_C // (COL, ROW)? WHY IS THIS HAPPEN?
// layer_keypos_t snappy_key2 = {0, 3, 2};  // {0,10,10};//{0, 3, 2};  // 對應 KC_V
// bool rappySnappyisOn = false;

// static int16_t adc_value[MATRIX_ROWS][MATRIX_COLS]; // put this in rappy_snappy.h
#endif // rappySnappy

#ifdef rappyTrigger
//rappy trigger lyon

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
/**********
typedef struct {
    bool enabled;              // 是否啟用此鍵的 Rappy 功能
    bool active;              // 當前按鍵狀態（是否已觸發）
    uint8_t row;               // 該鍵位於哪一行
    uint8_t col;               // 該鍵位於哪一列
    uint16_t keycode;          // 要註冊的 keycode

    float last_ratio;          // 上一次行程百分比
    uint8_t rise_count;        // 連續上升次數
    uint8_t fall_count;        // 連續下降次數

    uint8_t rise_threshold;    // 達到幾次連升才註冊
    uint8_t fall_threshold;    // 達到幾次連降才釋放
} rappy_ratio_trigger_t;

rappy_ratio_trigger_t rappy_q = {
    .enabled = true,
    .active = false,
    .row = 2,
    .col = 1,
    .keycode = KC_1,
    .last_ratio = 0.0f,
    .rise_count = 0,
    .fall_count = 0,
    .rise_threshold = 3,
    .fall_threshold = 3
};
bool process_rappy_ratio_trend(rappy_ratio_trigger_t *rappy);
***********/


#endif // rappyTrigger

#ifdef rappySnappy
// // 狀態記錄（避免重複輸出）
// static bool snappy1_active = false;
// static bool snappy2_active = false;

// // 判斷是否略過 Snappy 鍵（掃描時用）
// bool is_skipped_key(uint8_t row, uint8_t col) {
//     return (row == snappy_key1.row && col == snappy_key1.col) ||
//            (row == snappy_key2.row && col == snappy_key2.col);// ||
// 		   //(row == 2 && col == 1); // this is for rappy trigger KC_Q test
// }
#endif // rappySnappy

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
#  define MT_CAP_MAP    32
#endif
#ifndef MT_CAP_ACTIVE
#  define MT_CAP_ACTIVE 16
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
void mt_schedule_set(uint8_t L,uint8_t R,uint8_t C,uint8_t h,uint8_t k){
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

// 啟動時確保 P 不是 Mod-Tap（避免舊設定殘留）
// __attribute__((weak)) void mt_init_defaults(void){ 不用
//     mt_set(0, P_ROW, P_COL, 0, 0); // 清 P
//     // 也可在這裡先給 A 預設（通常不用）
//     // mt_set(0, A_ROW, A_COL, 0xE0, 0x04);
// }

static void mt_dump_map(void){ //測試用
    for (int i=0;i<MT_CAP_MAP;i++){
        if (mt_map[i].used)
            uprintf("[MT] MAP[%d]: L%u R%u C%u hold=0x%02X click=0x%02X\r\n",
                    i, mt_map[i].L, mt_map[i].R, mt_map[i].C, mt_map[i].hold, mt_map[i].click);
    }
}
#endif // modTap

extern const uint16_t keymaps[][MATRIX_ROWS][MATRIX_COLS]; //lyon for rappy snappy


#ifdef test8_thread
static binary_semaphore_t reportSem;
static THD_WORKING_AREA(waReportThread, 512);
// === Shared across threads ===
volatile bool rapidKeyActive = false;   // 取代跨函式想用的 rapidKeyStatus
uint32_t     lastMatrixUpdateTime = 0;  // 給 ReportThread 用
#endif // test8_thread
//uint16_t analogX,analogY,analogRx,analogRy,analogZ,analogRz;

//const int8_t hatSwitchMap[] = {0,-1,1,8,9,1,5,4,6,-1,2,-1,-1,-1,-1,-1};
const int8_t hatSwitchMap[] = {0,1,5,0,7,8,6,0,3,2,4,0,0,0,0,0,0};

//#define nonOP
#ifdef saocd_new
#define SOCD_MAX_PAIR 16

typedef enum {
    SOCD_MODE_NEUTRAL = 0,
    SOCD_MODE_LAST_WINS = 1,
    SOCD_MODE_FIRST_WINS = 2,
    SOCD_MODE_A_WINS = 3,
    SOCD_MODE_B_WINS = 4,
} socd_mode_t;

typedef struct {
    bool enable;

    uint8_t layer;   // 0xFF = all layers

    uint8_t a_row;
    uint8_t a_col;

    uint8_t b_row;
    uint8_t b_col;

    socd_mode_t mode;

    bool last_a;
    bool last_b;

	// 🔥 本輪 pressed 狀態（最關鍵）
    bool a_now;
    bool b_now;
} socd_pair_t;

static socd_pair_t socd_pairs[SOCD_MAX_PAIR];

///////////////////////
void socd_init(void) {
    memset(socd_pairs, 0, sizeof(socd_pairs));
}

void socd_set_pair(uint8_t index,
                   uint8_t layer,
                   uint8_t a_row, uint8_t a_col,
                   uint8_t b_row, uint8_t b_col,
                   socd_mode_t mode)
{
    socd_pair_t *p = &socd_pairs[index];

    p->enable = true;
    p->layer = layer;

    p->a_row = a_row;
    p->a_col = a_col;
    p->b_row = b_row;
    p->b_col = b_col;

    p->mode = mode;

    p->last_a = false;
    p->last_b = false;

    uprintf("[SOCD] SET pair %u: A(%u,%u) B(%u,%u) mode=%u\r\n",
        index, a_row, a_col, b_row, b_col, mode);
}
#endif // socd_new
#ifdef toggleKey
/* matrix state(1:on, 0:off) */
matrix_row_t matrix[MATRIX_ROWS];
static matrix_row_t matrix_temp[MATRIX_ROWS];

typedef struct {
    bool    enabled;   // 這個位置有沒有設定成 toggle key
    bool    state;     // 當前 ON(1) / OFF(0) 狀態
    uint8_t keycode;   // 要被「保持按住」的 keycode（1 byte）
} toggle_key_slot_t;

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
        // keycode=0 當作「清除這個 toggle key」
        tk->enabled = false;
        tk->state   = false;
        tk->keycode = 0;
    } else {
        tk->enabled = true;
        tk->state   = false;     // 新設定預設 OFF
        tk->keycode = keycode;
    }

    // 實體按鍵的狀態也清掉，避免殘留觸發一次
    toggle_last_pressed[layer][row][col] = false;
}

uint8_t toggle_key_get(uint8_t layer, uint8_t row, uint8_t col) {
    if (layer >= KEYMAPS_COUNT || row >= MATRIX_ROWS || col >= MATRIX_COLS) {
        return 0;
    }
    return toggle_keys[layer][row][col].keycode;
}

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

int16_t keyMatrixTriggerLevel = 1500, keyMatrixReleaseLevel; // move to matrix.h for using in via.c //

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

    // if (socd_claims_pos(row, col)) {
    //     socd_post_event(row, col, pressed);
    //     return;
    // }
	// uint8_t layer = get_highest_layer(layer_state);

	// int8_t slot = socd_find_slot(layer, row, col);
	// if (slot >= 0) {
	// 	socd_post_event(slot, row, col, pressed);
	// 	// continue;
	// 	return;
	// }
	// return;

    // if (socd_claims_pos(row, col)) {
    //     socd_post_event(row, col, pressed);
    //     return;
    // }
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
/************** 
bool now = calc_pressed_now(row, col, adcCalc);
bool prev = (matrix_temp[row] >> col) & 1;

if (now != prev) {
    matrix_temp[row] ^= (1 << col);

    uint8_t current_layer = get_highest_layer(layer_state);
    on_key_changed(current_layer, row, col, now);
}

static inline void on_key_changed(uint8_t layer, uint8_t row, uint8_t col, bool pressed)
{
    for (uint8_t i = 0; i < g_feature_count; i++) {

        switch (g_features[i].type) {

            case FEATURE_SOCD:
                socd_process(
                    (socd_cfg_t*)g_features[i].config,
                    layer, row, col, pressed
                );
                break;

            // 之後我們在 Step3 再加：
            // case FEATURE_MODTAP:
            // case FEATURE_RAPIDTRIGGER:
            // case FEATURE_RAPPYSNAPPY:
        }
    }
}
************/
#endif // socd
#ifdef saocd_new
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
#endif //socd_new
#define FORCE_HARDCODE_RT_PROFILE // lyon dynamic keystroke

void hook_early_init(void)
{
    int16_t i;

    palSetPadMode(GPIOC, 13, PAL_MODE_OUTPUT_PUSHPULL);				//Analog_PWR
//    palSetPad(GPIOC, 13);
    palClearPad(GPIOC, 13);


    i2c_init();
    uart_init();
	#ifdef socd
	// socd_init(); // lyon test for socd thread
	#endif // socd
	osalThreadSleepMilliseconds(1000); // lyon led delay add in here work
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

	#ifdef toggleKey
	// test code
	// toggle_key_set(0, 2, 2, 0x04);   // 0x04 = KC_A
	// toggle_key_set(0, 2, 2, 0x00);   // 0x04 = KC_A
	#endif // toggleKey

	// palSetPad(GPIOB, 3);
    // palSetPad(GPIOB, 4);

    // // 例如：Layer 0, row 4 col 1 / col 2，對應 Z / X
    // uint8_t idx = socd_add_pair(
    //     0, 4, 1, KC_Z,
    //     0, 4, 2, KC_X,
    //     SOCD_MODE_LAST_INPUT
    // );

    // uprintf("SOCD feature index=%d, total features=%d\r\n", idx, g_feature_count);
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

#ifdef ADC_v6
static void adccallback_v6(ADCDriver* adcp);
#endif
#ifdef ADC_v7
static void adccallback_v7(ADCDriver* adcp);
#endif

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
					#ifdef test8_thread
					chBSemSignalI(&reportSem);
					#endif
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
//=====V6R START
#ifdef ADC_v6E
// 可調參數 (以 GPT9 .frequency = 80000U → tick = 12.5 µs 為基準)
#define ADC_SETTLE_TICKS  3   // MUX 切換後等待 ~37.5 µs
#define ADC_SAMPLE_TICKS  4   // 轉換完成到下一步等 ~50 µs

static const ADCConversionGroup adcgrpcfg2 = {
    FALSE,                // circular
    6,                    // 一次轉換 6 個通道
    adccallback_v6,       // ★ 一定要用 v6 callback
    adcerrorcallback,     // 錯誤回呼

    0, // CR1
	// for one row?
  




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


// v6 的 adccallback：只處理 ADC 完成事件，排下一步
static void adccallback_v6(ADCDriver* adcp)
{
	// chBSemSignalI(&adcCompleteSem);
	// #ifdef test8_thread
	// chBSemSignalI(&reportSem); 
	// #endif
	++adcsampleIndex;
	adcStauts = adcConvertStart;
	gptStartOneShot(&GPTD9, ADC_SETTLE_TICKS );
}

// v6 的主狀態機
void irq_storm_gpt9_cb(GPTDriver* gptp)
{
    (void)gptp;
    switch (adcStauts) {
    case adcSensorPowerOn:
        sensorPowerCtrl(true);
		adcStauts = adcConvertStart;
        adcsampleIndex = 0; //original 0
        // muxSelect(adcsampleIndex);
        

        gptStartOneShot(&GPTD9, ADC_SETTLE_TICKS);
        break;

    case adcConvertStart:
        if (!ledUpdatingLock) {
            if (adcsampleIndex < 16) {
				muxSelect(adcsampleIndex);
				adcStauts = adcConvertWait;
				gptStartOneShot(&GPTD9, ADC_SETTLE_TICKS + 8);
                // AdcPortChange(false);
				// adcStartConversion(&ADCD1, &adcgrpcfg2,
                //                    &samples2[adcsampleIndex][0], 1); // for test

            } else {
                // 一輪完成：喚醒 matrixAnalogScan()
                sensorPowerCtrl(false);
                adcsampleIndex = 0;
                muxSelect(adcsampleIndex);
                chBSemSignalI(&adcCompleteSem);
				#ifdef test8_thread
				chBSemSignalI(&reportSem); 
				#endif
				adcStauts = adcSensorPowerOn;
                gptStartOneShot(&GPTD9, ADC_SETTLE_TICKS);
            }
        } else {
            sensorPowerCtrl(false);
            adcStauts = adcSensorPowerOn;
            gptStartOneShot(&GPTD9, ADC_SETTLE_TICKS);
        }
        break;

    case adcConvertWait:
		AdcPortChange(false);
		adcStartConversion(&ADCD1, &adcgrpcfg2,
							&samples2[adcsampleIndex][0], 1); // for test
        // sensorPowerCtrl(true);
        // if (++adcsampleIndex < 16) {
        //     muxSelect(adcsampleIndex);
        //     adcStauts = adcConvertStart;
        //     gptStartOneShot(&GPTD9, ADC_SETTLE_TICKS);
        // } else {
        //     // 保險：一輪完成

        //     sensorPowerCtrl(false);
        //     adcsampleIndex = 0;
        //     muxSelect(adcsampleIndex);
        //     chBSemSignalI(&adcCompleteSem);
		// 	#ifdef test8_thread
		// 	chBSemSignalI(&reportSem); 
		// 	#endif
        //     adcStauts = adcSensorPowerOn;
        //     gptStartOneShot(&GPTD9,
        //                     ADC_SETTLE_TICKS + ADC_SAMPLE_TICKS);
        // }
        break;
    }
}
// {
//     (void)gptp;

//     switch (adcStauts) {
//     case adcSensorPowerOn:
// 		adcStauts = adcConvertStart;
// 		adcsampleIndex = 0;
// 		gptStartOneShot(&GPTD9, ADC_SETTLE_TICKS);
// 		break;
//     case adcConvertStart:
// 	if (!ledUpdatingLock) {
// 		if (adcsampleIndex < 16) {
// 			AdcPortChange(true);
// 			sensorPowerCtrl(false);
// 			// osalSysPolledDelayX(US2RTC(AT32_SYSCLK, 5)); // add delay
// 			muxSelect(adcsampleIndex);
// 			// gptStartOneShot(&GPTD9, ADC_SETTLE_TICKS); // -2 will reach to 8k， (+４ is 5k but stable)
// 			sensorPowerCtrl(true);
// 			adcStauts = adcConvertWait;
// 			gptStartOneShot(&GPTD9, ADC_SETTLE_TICKS+ 8);
// 			// adcStartConversion(&ADCD1, &adcgrpcfg2,
// 			// 				   &samples2[adcsampleIndex][0], 1); // for test
// 		} else {
// 			sensorPowerCtrl(false);
// 			// chBSemSignalI(&adcCompleteSem);
// 			// #ifdef test8_thread
// 			// chBSemSignalI(&reportSem); 
// 			// #endif
// 			adcsampleIndex = 0;
// 			// adcStauts = adcConvertStart;
// 			gptStartOneShot(&GPTD9, ADC_SETTLE_TICKS);
// 		}
// 	} else {
// 		sensorPowerCtrl(false);
// 		adcStauts = adcSensorPowerOn;
// 		gptStartOneShot(&GPTD9, ADC_SETTLE_TICKS);
// 	}
// 	case adcConvertWait:
// 		AdcPortChange(false);
// 		adcStartConversion(&ADCD1, &adcgrpcfg2,
// 						   &samples2[adcsampleIndex][0], 1); // for test
// 	}
// }
#endif
// ===== /ADC_v6E =================================

#ifdef ADC_v6
// 可調參數 (以 GPT9 .frequency = 80000U → tick = 12.5 µs 為基準)
#define ADC_SETTLE_TICKS  3   // MUX 切換後等待 ~37.5 µs
#define ADC_SAMPLE_TICKS  4   // 轉換完成到下一步等 ~50 µs
// 掃描 6 路 ADC (PF4, PF5, PA4, PA5, PA6, PA7)
// 注意：ADC_NUM_CHANNELS 要定義成 6
static const ADCConversionGroup adcgrpcfg2 = {
    FALSE,                // circular
    6,                    // 一次轉換 6 個通道
    adccallback_v6,       // ★ 一定要用 v6 callback
    adcerrorcallback,     // 錯誤回呼

    0, // CR1
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

#ifdef ADC_v6
static void adccallback_v6(ADCDriver* adcp)
{
	chBSemSignalI(&adcCompleteSem);
	#ifdef test8_thread
	chBSemSignalI(&reportSem);
	#endif
	AdcPortChange(true);
	sensorPowerCtrl(false);
	muxSelect(++adcsampleIndex);

	gptStartOneShot(&GPTD9, ADC_SETTLE_TICKS + ADC_SAMPLE_TICKS + 5 + 1 + 1); // lyon 更改這個數值至3看看adc數值是否正常//original is 7 // 9問題改8測試8可以，為什麼13可以，改10吧(為了8k)會有問題
	// maybe change to higher value give it more time to process ex: 01-9, 02-8,02會有點狀況改成刪除狀態?no
	// 7 / 60000 秒 ＝ 約 116.7 µs
	// 8 / 140000sec = 57usec, 7/140000=50

	sensorPowerCtrl(true);
}

void irq_storm_gpt9_cb(GPTDriver* gptp)
{
	switch (adcStauts) {
		case adcSensorPowerOn:
			sensorPowerCtrl(true);
			adcsampleIndex = 0;
			muxSelect(adcsampleIndex);
			adcStauts = adcConvertStart;
			gptStartOneShot(&GPTD9, ADC_SETTLE_TICKS);

		break;
		case adcConvertStart:
			if (!ledUpdatingLock){
				if (adcsampleIndex<16){	
					AdcPortChange(false);
					adcStartConversion(&ADCD1, &adcgrpcfg2, &samples2[adcsampleIndex][0], 1);
				return;
				} else {
					sensorPowerCtrl(false);
					adcsampleIndex = 0;
					muxSelect(adcsampleIndex);
					chBSemSignalI(&adcCompleteSem);
					#ifdef test8_thread
					chBSemSignalI(&reportSem);
					#endif
					gptStartOneShot(&GPTD9, ADC_SETTLE_TICKS + ADC_SAMPLE_TICKS);
					adcStauts = adcSensorPowerOn;

				}
			} else {
				sensorPowerCtrl(false);
				gptStartOneShot(&GPTD9, ADC_SETTLE_TICKS);  
				adcStauts = adcConvertWait;
			}

		break;
		
		case adcConvertWait:
			sensorPowerCtrl(true);
			gptStartOneShot(&GPTD9, ADC_SETTLE_TICKS);
			adcStauts = adcConvertStart;
			break;
	}
}
#endif
// ===== /ADC_v6 =====
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
	#ifdef test8_thread
	chBSemSignalI(&reportSem);
	#endif
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
        perform_factory_reset();  // 執行你原本的清除邏輯
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
			if (adcPre >= 100){
				adcPre = 100;
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
					adcCalc -= 150;//DEFAULT_TRIGGER_LEVEL;
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

			#ifdef rappySnappy
			// ✅ 若此鍵是 Snappy 指定的，跳過掃描觸發處理 //lyon
			// if (is_skipped_key(keyLocation.rowA, keyLocation.colA) && rappySnappyisOn == true) {continue;}

			// adcCalc 已經算好
			adc_value[keyLocation.rowA][keyLocation.colA] = (int16_t)adcCalc;

			/**********
			#define TEST_ROW  2
			#define TEST_COL  1

			if (keyLocation.rowA == TEST_ROW && keyLocation.colA == TEST_COL) {
				static int16_t last = -1;
				if (adcCalc != last) {
					uprintf("[ADC] r%d c%d adcCalc=%d cache=%d\r\n",
							TEST_ROW, TEST_COL,
							adcCalc,
							adc_value[TEST_ROW][TEST_COL]);
					last = adcCalc;
				}
			***********/


			if (rappy_snappy_is_key(keyLocation.rowA, keyLocation.colA)) {
				continue;
			}

			// void rappy_process_key(uint8_t row, uint8_t col, int16_t depth)
			// {
			// 	for (uint8_t i = 0; i < RAPPY_COUNT; i++)
			// 	{
			// 		rappy_key_t *rk = &g_rappy[i];

			// 		if (!rk->enable)
			// 			continue;

			// 		if (rk->row == row && rk->col == col)
			// 		{
			// 			if (depth < 0)
			// 				depth = 0;

			// 			rappy_process_one(rk, depth);
			// 			return;
			// 		}
			// 	}
			// }

			// if (rappy_is_bound(keyLocation.rowA, keyLocation.colA))
			// {
			// 	uprintf("isBound!!!\n");
			// 	rappy_process_key(keyLocation.rowA, keyLocation.colA, (int16_t)adcCalc);
			// }
			#endif // rappySnappy

			#ifdef rappyTrigger
			if(keyLocation.rowA == -1 && keyLocation.colA == -1){continue;}
			#endif // rappyTrigger

			#ifdef socd
			/********* old one 
			bool pressed_now = calc_pressed_now(keyLocation.rowA, keyLocation.colA, adcCalc);

			if (socd_claims_pos(keyLocation.rowA, keyLocation.colA) && g_policy != 4) {
				socd_post_event(keyLocation.rowA, keyLocation.colA, pressed_now);
				continue;   // ★ 跳過原本邏輯
			}
			************/

			bool pressed_now = calc_pressed_now(keyLocation.rowA, keyLocation.colA, adcCalc);

			/* ① 先通知 SOCD */
			void on_key_changed(uint8_t row, uint8_t col, bool pressed);

			// uint8_t layer = 0;//get_highest_layer(layer_state);
			// int8_t slot = socd_find_slot(layer, keyLocation.rowA, keyLocation.colA);
			// if (slot >= 0) {
			// 	socd_post_event((uint8_t)slot, keyLocation.rowA, keyLocation.colA, pressed_now);
			// 	continue; // SOCD 接管，跳過原本 register/unregister
			// }

			/* ② 如果這顆鍵是 SOCD 管的，就不要自己送 keycode */
			// if (socd_claims_pos(keyLocation.rowA, keyLocation.colA)) {
			// 	continue;//return;
			// }

			#endif // socd
			#ifdef modTap
			uint8_t L = 0;
			// uint8_t R = keyLocation.rowA;
			// uint8_t C = keyLocation.colA;
			// bool prev = mt_prev[R][C];
			bool prev = mt_prev[keyLocation.rowA][keyLocation.colA];

			// A) P：按下邊緣 → toggle A 的 Mod-Tap；不論按/放，都要更新 prev 並 continue
			// if (R == P_ROW && C == P_COL) {
			// 	if (pressed_now && !prev) {                   // 只在「按下瞬間」切換
			// 		if (mt_active(0, A_ROW, A_COL)) {
			// 			mt_schedule_set(0, A_ROW, A_COL, 0, 0);                  // 關
			// 			// uprintf("[MT] toggle -> OFF\n");
			// 		} else {
			// 			mt_schedule_set(0, A_ROW, A_COL, 0xE0, 0x04);            // 開：長壓 LCtrl、點按 A
			// 			// uprintf("[MT] toggle -> ON\n");
			// 		}
			// 	}
			// 	mt_prev[R][C] = pressed_now;                 // ★ 不論 press/release 都更新
			// 	continue;                                    // ★ 吃掉 P 的事件，避免落回原路徑
			// }

			// B) A 或任何已被設定為 Mod-Tap 的鍵：用邊緣觸發 mt_on_down/up
			// if (mt_active(L, R, C)) {
			// 	if (pressed_now && !prev) mt_on_down(L, R, C);   // 上一輪未按、這輪按下 → down
			// 	if (!pressed_now && prev) mt_on_up  (L, R, C);   // 上一輪按下、這輪放開 → up
			// 	mt_prev[R][C] = pressed_now;                     // ★ 自行維護 prev
			// 	continue;                                        // ★ 跳過原本 matrix_temp 的設/清位
			// }
			if (mt_active(L, keyLocation.rowA, keyLocation.colA)) {
				if (pressed_now && !prev) mt_on_down(L, keyLocation.rowA, keyLocation.colA);   // 上一輪未按、這輪按下 → down
				if (!pressed_now && prev) mt_on_up  (L, keyLocation.rowA, keyLocation.colA);   // 上一輪按下、這輪放開 → up
				mt_prev[keyLocation.rowA][keyLocation.colA] = pressed_now;                     // ★ 自行維護 prev
				continue;                                        // ★ 跳過原本 matrix_temp 的設/清位
			}
			#endif // modTap

			#ifdef saocd_new
			bool socd_process(uint8_t layer,
							uint8_t row,
							uint8_t col,
							bool *pressed_io)
			{
				for (uint8_t i = 0; i < SOCD_MAX_PAIR; i++) {

					socd_pair_t *p = &socd_pairs[i];
					if (!p->enable) continue;
					if (p->layer != 0xFF && p->layer != layer) continue;

					bool is_a = (p->a_row == row && p->a_col == col);
					bool is_b = (p->b_row == row && p->b_col == col);
					if (!is_a && !is_b) continue;

					// 🔥 使用「本輪」的 A/B 狀態
					bool A_state = p->a_now;
					bool B_state = p->b_now;

					// update last pressed
					if (is_a) p->last_a = *pressed_io;
					if (is_b) p->last_b = *pressed_io;

					switch (p->mode)
					{
						case SOCD_MODE_NEUTRAL:
							if (A_state && B_state) {
								*pressed_io = false;
								return true;
							}
							return false;

						case SOCD_MODE_A_WINS:
							if (A_state && B_state) {
								if (is_b) *pressed_io = false;
								return true;
							}
							return false;

						case SOCD_MODE_B_WINS:
							if (A_state && B_state) {
								if (is_a) *pressed_io = false;
								return true;
							}
							return false;

						case SOCD_MODE_LAST_WINS:
							if (A_state && B_state) {
								if (p->last_a && !p->last_b && is_b) *pressed_io = false;
								if (p->last_b && !p->last_a && is_a) *pressed_io = false;
								return true;
							}
							return false;

						case SOCD_MODE_FIRST_WINS:
							if (A_state && B_state) {
								if (p->last_a && !p->last_b && is_a) *pressed_io = false;
								if (p->last_b && !p->last_a && is_b) *pressed_io = false;
								return true;
							}
							return false;
					}
				}

				return false;
			}
			#endif // socd_new
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
			#ifdef saocd_new
			// uint8_t act_layer = biton32(layer_state);
			bool pressed = calc_pressed_now(keyLocation.rowA,
											keyLocation.colA,
											adcCalc);

			// socd_process(act_layer, keyLocation.rowA, keyLocation.colA, &pressed);

			// // 再把 pressed 寫進 matrix_temp
			// if (pressed)
			// 	matrix_temp[keyLocation.rowA] |= (1u << keyLocation.colA);
			// else
			// 	matrix_temp[keyLocation.rowA] &= ~(1u << keyLocation.colA);


			// 偵測是否為 SOCD 的 A/B key
			for (uint8_t i = 0; i < SOCD_MAX_PAIR; i++) {
				if (!socd_pairs[i].enable) continue;

				if (keyLocation.rowA == socd_pairs[i].a_row && keyLocation.colA == socd_pairs[i].a_col)
					socd_pairs[i].a_now = pressed;

				if (keyLocation.rowA == socd_pairs[i].b_row && keyLocation.colA == socd_pairs[i].b_col)
					socd_pairs[i].b_now = pressed;
			}

			#endif //socd_new


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
	// process_snappy_pair(0, 3, 1, 0, 3, 2); // lyon rappy snappy function
	// process_snappy_pair(0, snappy_key1.row, snappy_key1.col, 0, snappy_key2.row, snappy_key2.col);

	rappy_snappy_process_all();

	// void rappy_process_all(void) {
	// 	for (int i=0;i<RAPPY_COUNT;i++){
	// 		rappy_key_t *rk = &g_rappy[i];
	// 		if (!rk->enable) continue;
	// 		int16_t depth = keyMatrixDelta[rk->row][rk->col];
	// 		rappy_process_one(rk, depth);
	// 	}
	// }
	// void rappy_process_all(uint8_t row, uint8_t col, int16_t depth)
	// {
	// 	for (uint8_t i = 0; i < RAPPY_COUNT; i++)
	// 	{
	// 		rappy_key_t *rk = &g_rappy[i];

	// 		if (!rk->enable)
	// 			continue;

	// 		if (rk->row == row && rk->col == col)
	// 		{
	// 			if (depth < 0)
	// 				depth = 0;

	// 			rappy_process_one(rk, depth);
	// 			return;
	// 		}
	// 	}
	// }
	// rappy_process_all(); 

	#endif // rappySnappy

	#ifdef rappyTrigger
	// process_rappy_ratio_trend(&rappy_q);
	rappy_q.enabled = false;
	if(rappy_q.enabled == true){
		process_rappy_fixed_ratio(&rappy_q);
	}
	#endif // rappyTrigger

	#ifdef socd
	// socd_direction(&firstSnap); // before error
	// static inline void after_matrix_scan(void){
	// 	socd_tick();
	// }
	#endif //socd

	#ifdef modTap
	mt_tick(0);         // 處理長按超時 → 送/收修飾鍵
	mt_apply_pending(); //
	#endif // modTap


#ifdef test8_thread
	rapidKeyActive = rapidKeyStatus;
#endif
#ifndef test8_thread
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
#endif // test8_thread
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
	#ifdef test8_thread
	chBSemObjectInit(&reportSem, true);
	#endif
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
//	LED_init();
	led_ctrl(true);
//    currentLED_RunMode.type = offMode;
#ifdef socd
	socd_thread_init();  // lyon socd.c file
	socd_load_all_from_eeprom();
	// 測試：pair 0 綁 row=4,col=1 和 row=4,col=2
    // socd_config_pair(
    //     4,
    //     true,       // enable
    //     4, 3,       // A
    //     4, 4,       // B
    //     SOCD_PRIORITY_PRIMARY
    // );

    // uprintf("SOCD test pair loaded\r\n");
#endif

#ifdef saocd_new

    socd_init();   // ✅ 初始化所有 SOCD pair

    // ✅ 在 pre_init 設定 Pair0：
    //   Layer 0
    //   A = R2C1
    //   B = R3C1
    //   Mode = Neutral
    socd_set_pair(
        0,               // index = Pair0
        0,               // layer = 0
        2, 1,            // A = row=2, col=1
        3, 1,            // B = row=3, col=1
        SOCD_MODE_NEUTRAL
    );

#endif //socd_new


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


	// 在 LED 相關 thread 啟動之後 // lyon add
	if(!lampArrayDisable){
		// uprintf(">>>>>>>>>>8\r\n");
		chThdSleepMilliseconds(300);
		buildNormalModes();
		// currentLED_RunMode.type = normalMode;
		currentLED_RunMode.mode = 8;//0;// RED
		currentLED_RunMode.brightness = 5;
		currentLED_RunMode.speed = 1;
		setRunMode(currentLED_RunMode);
	}
	

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
#ifdef test8_thread
// static THD_WORKING_AREA(waReportThread, 512);
static THD_FUNCTION(ReportThread, arg) {
    (void)arg;
    chRegSetThreadName("ReportThread");

    while (1) {
		uprintf("a\r\n");
		palSetPad(GPIOC, 12);
		if(currentLED_RunMode.type != dynamticLightingMode){
			// chThdSleep(TIME_US2I(200)); //
			chBSemWait(&reportSem);   // 等 ISR 通知
			palClearPad(GPIOC, 12);
			// uprintf("a1\r\n");
			memcpy(matrix, matrix_temp, sizeof(matrix));
			keyboard_taskTMR();
			if (rapidKeyActive == false){
				// chThdSleep(TIME_US2I(300));
				memcpy(matrix, matrix_temp, sizeof(matrix));
				keyboard_taskTMR();

			} else if (timer_elapsed32(lastMatrixUpdateTime) > 500){
				memcpy(matrix, matrix_temp, sizeof(matrix)); // lyon try 
				keyboard_taskTMR();
				lastMatrixUpdateTime = timer_read32();
			}
			if (gameAnalogStickStatus.raw !=0){
				chBSemSignal(&gamepadAnalogSem);
			}
			
		} else {

		}
		// palClearPad(GPIOC, 12);  //
    }
}

#endif // test8_thread
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

void matrix_init(void)
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



//    if (rapidTriggerKeyBuilder(7, 1, 10, 10, keycodes) !=0){
//    	uprintf ("build Fail #9\r\n");
//    }


    //if (1){
    //eepromInitCheck = false;
	//write_keymap_to_eeprom();
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
    	uprintf("Factory Reset end\r\n");
//    	NVIC_SystemReset();
    }
    chThdCreateStatic(waADC_ProcessThread, sizeof(waADC_ProcessThread), NORMALPRIO - 4, ADC_ProcessThread, NULL);
    LED_init();
	#ifdef test8_thread
	chThdCreateStatic(waReportThread, sizeof(waReportThread), NORMALPRIO - 3, ReportThread, NULL);
	#endif

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
//	uprintf ("Layer:%d keycode:%04x\r\n", biton32(layer_state), keycode);

//    action_t action = layer_switch_get_action(record.event.key);
//    uint8_t code = action.key.code;
//    uprintf ("action keycode:%X modes:%X kid_id:%X\r\n",code,action.key.mods,action.kind.id);

	#ifdef socd
	int8_t slot = socd_find_slot_by_keycode(keycode);
    if (slot >= 0) {
        socd_post_event(slot, keycode, record.event.pressed);
        return false;   // SOCD 接管
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
				case KC_M1:
				// 	return false; // 我已處理
				// }
				// return true;
					// send_string("abcdefghijklmnopqrstuvwxyz\n"); /**/
					send_A_to_Z();
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
					gamePadIndicate();
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

// void setKnobState(status, knobState){ // THIS FUNCTION CAN CHANGE THE STATE OF FIRST ENCODER
// 	switch(knobState){
// 		case 0: // CHANGE VOLUMN
// 			switch(status){
// 				case ENCODER_CLOCKWISE:
// 					tap_code_delay(KC_VOLU, 10);
// 				break;
// 				case ENCODER_COUNTER_CLOCKWISE:
// 					tap_code_delay(KC_VOLD, 10);
// 				break;
// 				case ENCODER_BUTTON:
// 					keycodeOutput = !keycodeOutput;
//                 	tap_code(KC_MUTE);
// 				break;
// 			}
// 		break;
// 		case 1: // CHANGE RGB EFFECT
// 			switch(status){ 
// 				case ENCODER_CLOCKWISE:
// 					currentLED_RunMode.mode++;
//                 	setRunMode(currentLED_RunMode);
// 				break;
// 				case ENCODER_COUNTER_CLOCKWISE:
// 					currentLED_RunMode.mode--;
//                 	setRunMode(currentLED_RunMode);
// 				break;
// 				case ENCODER_BUTTON:
// 					// keycodeOutput = !keycodeOutput;
//                 	// tap_code(KC_MUTE);
// 				break;
// 			}
// 		break;
// 		case 2: // CHANGE RGB COLOR(TDB) 
// 			switch(status){
// 				case ENCODER_CLOCKWISE:
// 					// tap_code_delay(KC_BRIU, 10);
// 				break;
// 				case ENCODER_COUNTER_CLOCKWISE:
// 					// tap_code_delay(KC_BRID, 10);
// 				break;
// 				case ENCODER_BUTTON:
// 					// keycodeOutput = !keycodeOutput;
//                 	// tap_code(KC_MUTE);
// 				break;
// 			}
// 		break;
// 		case 3: // MULTIMEDIA
// 			switch(status){
// 				case ENCODER_CLOCKWISE:
// 					tap_code_delay(KC_MNXT, 10);
// 				break;
// 				case ENCODER_COUNTER_CLOCKWISE:
// 					tap_code_delay(KC_MPRV, 10);
// 				break;
// 				case ENCODER_BUTTON:
// 					keycodeOutput = !keycodeOutput;
//                 	tap_code(KC_MPLY);
// 				break;
// 			}
// 		break;
// 	}
// }

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
						// keycodeOutput = !keycodeOutput; //lyon this is testing code(keycode output will toggle)
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
        // case 1: // try not use this lyon
        //     if (status)
        //     {           /* Second encoder */
        //         tap_code(KC_VOLU);
        //     }
        //     else
        //     {
        //         tap_code(KC_VOLD);
        //     }
        //     break;

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
// void process_snappy_pair(uint8_t layer1, uint8_t row1, uint8_t col1,
//                          uint8_t layer2, uint8_t row2, uint8_t col2) {

//     // === 取得對應的 Keycode ===
//     uint16_t keycode1 = keymaps[layer1][row1][col1];//keymaps[0][1][3];//keymaps[layer1][row1][col1];
//     uint16_t keycode2 = keymaps[layer2][row2][col2];//keymaps[0][2][3];//keymaps[layer2][row2][col2];

// 	// snappy_key1 = {1, 3};  // 對應 KC_A
// 	// snappy_key2 = {2, 3};  // 對應 KC_B
// 	// snappy_key1.col = 1;
// 	// snappy_key1.row = 3;
// 	// snappy_key2.col = 2;
// 	// snappy_key2.row = 3;

// 	// snappy_key1.col = col1;
// 	// snappy_key1.row = row1;
// 	// snappy_key2.col = col2;
// 	// snappy_key2.row = row2;

//     // === 取得 ADC 原始資料
//     analogKey_Config_t info1 = matrixAnalogRelocate[row1][col1];
//     analogKey_Config_t info2 = matrixAnalogRelocate[row2][col2];

//     int adc1 = samples2[info1.colA][info1.rowA] - initPostionLevel[info1.colA][info1.rowA];
//     int adc2 = samples2[info2.colA][info2.rowA] - initPostionLevel[info2.colA][info2.rowA];

//     int travel1 = travelPostionLevel[info1.colA][info1.rowA] - initPostionLevel[info1.colA][info1.rowA];
//     int travel2 = travelPostionLevel[info2.colA][info2.rowA] - initPostionLevel[info2.colA][info2.rowA];

//     if (travel1 <= 0) travel1 = 1;
//     if (travel2 <= 0) travel2 = 1;

//     float ratio1 = (float)adc1 / travel1;
//     float ratio2 = (float)adc2 / travel2;

// 	// uprintf("snappy r1=%.2f r2=%.2f pressed1=%d pressed2=%d\r\n", ratio1, ratio2, travel1, travel2);

//     const float trigger_threshold = 0.10f;
//     const float full_threshold = 0.80f;

//     bool pressed1 = ratio1 > trigger_threshold;
//     bool pressed2 = ratio2 > trigger_threshold;
//     bool full1 = ratio1 > full_threshold;
//     bool full2 = ratio2 > full_threshold;

//     if (!pressed1 && !pressed2) {
//         if (snappy1_active) { unregister_code(keycode1); snappy1_active = false; }
//         if (snappy2_active) { unregister_code(keycode2); snappy2_active = false; }
//     }
//     else if (full1 && full2) {
//         if (!snappy1_active) { register_code(keycode1); snappy1_active = true; }
//         if (!snappy2_active) { register_code(keycode2); snappy2_active = true; }
//     }
//     else if (pressed1 && (!pressed2 || ratio1 >= ratio2)) {
//         if (!snappy1_active) { register_code(keycode1); snappy1_active = true; }
//         if (snappy2_active)  { unregister_code(keycode2); snappy2_active = false; }
//     }
//     else if (pressed2 && (!pressed1 || ratio2 > ratio1)) {
//         if (!snappy2_active) { register_code(keycode2); snappy2_active = true; }
//         if (snappy1_active)  { unregister_code(keycode1); snappy1_active = false; }
//     }

	
// }
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

    // if (!rappy->pressed && ratio >= rappy->trigger_ratio) {
    //     register_code(rappy->keycode);
    //     rappy->pressed = true;
    //     uprintf(">>> Rappy PRESS: %.2f\r\n", ratio);
    // } else if (rappy->pressed && ratio <= rappy->release_ratio) {
    //     unregister_code(rappy->keycode);
    //     rappy->pressed = false;
    //     // uprintf("<<< Rappy RELEASE: %.2f\r\n", ratio);
    // }
	
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

/*****
bool process_rappy_ratio_trend(rappy_ratio_trigger_t *rappy) {
	analogKey_Config_t info1 = matrixAnalogRelocate[rappy->row][rappy->col];

    int adc1 = samples2[info1.colA][info1.rowA] - initPostionLevel[info1.colA][info1.rowA];

    int travel1 = travelPostionLevel[info1.colA][info1.rowA] - initPostionLevel[info1.colA][info1.rowA];

    if (travel1 <= 0) travel1 = 1;

    float ratio1 = (float)adc1 / travel1;

    const float trigger_threshold = 0.10f;
    const float full_threshold = 0.80f;

    bool pressed1 = ratio1 > trigger_threshold;
    bool full1 = ratio1 > full_threshold;

	static float oriState = 0.00f;
	static float newState = 0.00f;

	if(!pressed1){
		unregister_code(rappy->keycode); rappy->active = false;
	}else if(full1){
		register_code(rappy->keycode); rappy->active = true;
	}else if(pressed1){ //between 0.8-0.1
		newState = ratio1;
		if(oriState < newState){
			if(rappy->active == true){
				oriState = newState;
			}else if(newState - oriState > 0.30f){
				register_code(rappy->keycode);
				oriState = newState;
				rappy->active = true;
			}
		} else if(oriState > newState){
			if(rappy->active == false){
				oriState = newState;
			}else if(oriState - newState > 0.30f){
				unregister_code(rappy->keycode);
				oriState = newState;
				rappy->active = false;
			}
		}
		if(newState > full_threshold){newState = full_threshold; }
		if(oriState > full_threshold){oriState = full_threshold; }
	}
}
*****/
/************
#define RISE_TREND_COUNT 3
#define FALL_TREND_COUNT 2

static uint8_t rise_count = 0;
static uint8_t fall_count = 0;
static float last_ratio = 0.0f;
static bool rappy_active = false;

float ratio = (float)(adc - init) / travel;  // 你的行程比率算法

if (!rappy_active) {
    if (ratio > last_ratio) {
        rise_count++;
        fall_count = 0;
        if (rise_count >= RISE_TREND_COUNT) {
            register_code(KC_Q);
            rappy_active = true;
            uprintf(">>> Rappy PRESS (%.2f)\n", ratio);
            rise_count = 0;
        }
    } else {
        rise_count = 0;
        if (ratio < last_ratio) fall_count++;
    }
} else {
    if (ratio < last_ratio) {
        fall_count++;
        rise_count = 0;
        if (fall_count >= FALL_TREND_COUNT) {
            unregister_code(KC_Q);
            rappy_active = false;
            uprintf("<<< Rappy RELEASE (%.2f)\n", ratio);
            fall_count = 0;
        }
    } else {
        fall_count = 0;
        if (ratio > last_ratio) rise_count++;
    }
}

last_ratio = ratio;
***********/
#endif // rappyTrigger
