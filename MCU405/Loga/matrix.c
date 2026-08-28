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
# include "timer.h"
# include "matrix.h"
# include "wait.h"
# include "keycode.h"
# include "report.h"
# include "math.h"
# include "action_layer.h"
# include "action.h"
# include "dynamic_macro.h"
# include "macro_engine.h"
# include "led/main.h"
# include "rappy_snappy.h"
# include <math.h>
# include <stdint.h>
# include <string.h>
# include "rappy_snappy.h"
# include "via/via.h"
# include "host.h"
#include <stdbool.h>
#include "action.h"
#include "led/led_effect.h"
#include "i2c_master.h"

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

/*
 * Hall sensor output is approximately a convex (about x^1.8) function of
 * key travel. The table applies its inverse curve (about x^0.55) to the
 * calibrated 0..TRAVEL_STEPS ADC range with integer-only interpolation.
 */
#define ADC_LINEAR_CURVE_SEGMENTS 16U
static const uint16_t adc_linear_curve[ADC_LINEAR_CURVE_SEGMENTS + 1U] = {
    0U, 76U, 112U, 139U, 163U, 185U, 204U, 222U, 239U,
    255U, 270U, 285U, 299U, 312U, 325U, 338U, 350U
};

static int32_t adc_linearize_travel(int32_t adc_delta, int32_t adc_range)
{
    uint32_t raw_steps;
    uint32_t scaled_index;
    uint32_t index;
    uint32_t fraction;
    int32_t low;
    int32_t high;

    if (adc_delta <= 0 || adc_range <= 0) {
        return 0;
    }

    if (adc_delta >= adc_range) {
        return TRAVEL_STEPS;
    }

    raw_steps = ((uint32_t)adc_delta * TRAVEL_STEPS) /
                (uint32_t)adc_range;
    scaled_index = raw_steps * ADC_LINEAR_CURVE_SEGMENTS;
    index = scaled_index / TRAVEL_STEPS;
    fraction = scaled_index % TRAVEL_STEPS;

    if (index >= ADC_LINEAR_CURVE_SEGMENTS) {
        return TRAVEL_STEPS;
    }

    low = adc_linear_curve[index];
    high = adc_linear_curve[index + 1U];

    return low + ((high - low) * (int32_t)fraction +
                  (TRAVEL_STEPS / 2)) / TRAVEL_STEPS;
}

/*
 * Missing declarations used by matrix.c.
 */
//uint8_t get_highest_layer(layer_state_t state);

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
// Test timeout for LED idle-off. Change back to 30 * 60 * 1000UL after validation.
#define LED_IDLE_OFF_TIMEOUT_MS (30UL * 60UL * 1000UL)
static uint32_t idle_led_last_activity_ms = 0;
static bool idle_led_timeout_reported = false;
static bool idle_led_is_sleeping = false;
static led_runMode_t idle_led_restore_mode;

static led_runMode_t idle_led_build_mode(led_runMode_t base, uint8_t type, uint8_t mode) {
    led_runMode_t run_mode = base;

    run_mode.type = type;
    run_mode.mode = mode;

    if (run_mode.brightness < 1 || run_mode.brightness > 100) {
        run_mode.brightness = 30;
    }

    if (run_mode.speed > 100) {
        run_mode.speed = 100;
    }

    return run_mode;
}

static void idle_led_note_activity(void) {
    idle_led_last_activity_ms = timer_read32();
    idle_led_timeout_reported = false;

    if (idle_led_is_sleeping) {
        led_runMode_t wake_mode = idle_led_build_mode(lastLED_RunMode, normalMode, eeprom_read_byte(KB_COLOR_MODE_ADDR));
        idle_led_is_sleeping = false;
        idle_led_restore_mode = wake_mode;
        currentLED_RunMode = wake_mode;
        setRunMode(wake_mode);
    }

    // uprintf("[IDLE] activity reset\r\n");
    // uprintf("1\r\n");
}

static void idle_led_maybe_report_timeout(void) {
    if (idle_led_timeout_reported) {
        return;
    }

    if (timer_elapsed32(idle_led_last_activity_ms) < LED_IDLE_OFF_TIMEOUT_MS) {
        return;
    }

    idle_led_timeout_reported = true;
    idle_led_is_sleeping = true;
    idle_led_restore_mode = idle_led_build_mode(lastLED_RunMode, normalMode, eeprom_read_byte(KB_COLOR_MODE_ADDR));
    currentLED_RunMode = idle_led_build_mode(lastLED_RunMode, normalMode, 0x00);
    setRunMode(currentLED_RunMode);
    // uprintf("[IDLE] timeout\r\n");
    // uprintf("0\r\n");
}

void hook_keyboard_loop(void)
{
    idle_led_maybe_report_timeout();
}

// sleeping mode

void send_string(const char *str) {
    while (*str) {
        tap_code16(*str++);
		// tap_code16((uint8_t)*str++);

    }
}

void send_A_to_Z(void) {
    for (uint16_t kc = KC_A; kc <= KC_Z; kc++) {
        tap_code(kc);       // ??????
        // ????????????????,???????:
        // chThdSleepMilliseconds(10);
    }
}

/*
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
*/
/*
static bool macro_engine_set_step_like_api(
    uint8_t macro_index,
    uint8_t step_index,
    uint16_t keycode,
    uint8_t delay_type,
    uint16_t delay_ms,
    uint8_t action
) {
    macro_config_t *cfg = macro_engine_get_config(macro_index);

    if (cfg == NULL || macro_index >= MACRO_GROUP_COUNT || step_index >= MACRO_MAX_STEPS) {
        uprintf("[MACRO][API-SIM] set step fail idx=%u step=%u\r\n", macro_index, step_index);
        return false;
    }

    cfg->step[step_index].keycode    = keycode;
    cfg->step[step_index].delay_type = delay_type;
    cfg->step[step_index].delay_ms   = delay_ms;
    cfg->step[step_index].action     = action;

    if (cfg->step_count <= step_index) {
        cfg->step_count = (uint8_t)(step_index + 1);
    }

    macro_save_to_eeprom(macro_index);
    return true;
}
*/
/*
static void macro_engine_write_test_copy_paste_api_style(uint8_t macro_index) {
    macro_clear(macro_index);

    if (!macro_engine_set_step_like_api(macro_index, 0, KC_LCTL, MACRO_DELAY_FIXED, 0,   MACRO_ACTION_PRESS))   return;
    if (!macro_engine_set_step_like_api(macro_index, 1, KC_C,    MACRO_DELAY_FIXED, 10,  MACRO_ACTION_PRESS))   return;
    if (!macro_engine_set_step_like_api(macro_index, 2, KC_C,    MACRO_DELAY_FIXED, 20,  MACRO_ACTION_RELEASE)) return;
    if (!macro_engine_set_step_like_api(macro_index, 3, KC_LCTL, MACRO_DELAY_FIXED, 10,  MACRO_ACTION_RELEASE)) return;
    if (!macro_engine_set_step_like_api(macro_index, 4, KC_LCTL, MACRO_DELAY_FIXED, 120, MACRO_ACTION_PRESS))   return;
    if (!macro_engine_set_step_like_api(macro_index, 5, KC_V,    MACRO_DELAY_FIXED, 10,  MACRO_ACTION_PRESS))   return;
    if (!macro_engine_set_step_like_api(macro_index, 6, KC_V,    MACRO_DELAY_FIXED, 20,  MACRO_ACTION_RELEASE)) return;
    if (!macro_engine_set_step_like_api(macro_index, 7, KC_LCTL, MACRO_DELAY_FIXED, 10,  MACRO_ACTION_RELEASE)) return;

    uprintf("[MACRO][API-SIM] full macro written idx=%u step_count=8\r\n", macro_index);
}
*/
/*
static void macro_engine_write_full_slot_api_style(uint8_t macro_index) {
    macro_clear(macro_index);

    for (uint8_t step_index = 0; step_index < MACRO_MAX_STEPS; step_index++) {
        uint8_t pair_index = (uint8_t)(step_index / 2);
        uint16_t keycode = (uint16_t)(KC_A + (pair_index % 26));
        uint16_t delay_ms = (step_index % 2 == 0) ? 0 : 5;
        uint8_t action = (step_index % 2 == 0) ? MACRO_ACTION_PRESS : MACRO_ACTION_RELEASE;

        if (!macro_engine_set_step_like_api(
                macro_index,
                step_index,
                keycode,
                MACRO_DELAY_FIXED,
                delay_ms,
                action)) {
            uprintf("[MACRO][API-SIM] full-slot write stopped at step=%u\r\n", step_index);
            return;
        }
    }

    uprintf("[MACRO][API-SIM] full-slot macro written idx=%u step_count=%u\r\n", macro_index, MACRO_MAX_STEPS);
}
*/
/*
static void macro_engine_dump_eeprom_slot(uint8_t index) {
    uint8_t *base_addr;
    uint8_t step_count;
    macro_step_t step = {0};

    if (index >= MACRO_GROUP_COUNT) {
        uprintf("[MACRO][EEPROM] invalid slot=%u\r\n", index);
        return;
    }

    base_addr = EEPROM_MACRO_SLOT_ADDR(index);
    step_count = eeprom_read_byte(base_addr);

    uprintf("[MACRO][EEPROM] slot=%u step_count=%u\r\n", index, step_count);

    if (step_count > MACRO_MAX_STEPS) {
        uprintf("[MACRO][EEPROM] invalid step_count=%u\r\n", step_count);
        return;
    }

    for (uint8_t i = 0; i < step_count; i++) {
        eeprom_read_block(
            &step,
            (const void *)(base_addr + 1U + ((uint16_t)i * sizeof(macro_step_t))),
            sizeof(step)
        );

        uprintf(
            "[MACRO][EEPROM] step=%u kc=0x%04X delay_type=%u delay_ms=%u action=%u\r\n",
            i,
            step.keycode,
            step.delay_type,
            step.delay_ms,
            step.action
        );
    }
}
*/
/*
 void send_string(const char *str) {
     while (*str) {
         host_keyboard_send_char(*str++);
     }
 }
*/

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
bool ADC_Check = false,ADC_Wait = false;

//test
bool ledOn_off_test = true;

//static uint8_t adcStauts = adcSensorPowerOn;
uint8_t dly_times;
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

// #define KB_ROWS 6     // ? ?????? ROW
// #define KB_COLS 15    // ? ?????? COL

// // ??????????(??? A?P ??? row/col)
// #define A_ROW 3
// #define A_COL 3
// #define P_ROW 2
// #define P_COL 10

// ===== Mod-Tap minimal core (sparse map) =====
#include "timer.h"
#include "keycode.h"

// ??????(????????,???????)
#define KB_ROWS 6      // ? ????? ROW
#define KB_COLS 15     // ? ????? COL

// ?????(??? A ? P ??? row/col)
// #define A_ROW 3 // ????????
// #define A_COL 3
// #define P_ROW 2
// #define P_COL 10

// ????? Mod-Tap ?? / ????? Mod-Tap ??
#ifndef MT_CAP_MAP
#  define MT_CAP_MAP    8
#endif
#ifndef MT_CAP_ACTIVE
#  define MT_CAP_ACTIVE 4
#endif

// typedef struct { uint8_t L,R,C,hold,click; bool used; } mt_entry_t; // in matrix.h
// static mt_entry_t mt_map[MT_CAP_MAP];
mt_entry_t mt_map[MT_CAP_MAP] = {0}; 
static bool mt_prev[KB_ROWS][KB_COLS];   // ?? Mod-Tap ?????

static int mt_find(uint8_t L,uint8_t R,uint8_t C){
    for (int i=0;i<MT_CAP_MAP;i++)
        if (mt_map[i].used && mt_map[i].L==L && mt_map[i].R==R && mt_map[i].C==C) return i;
    return -1;
}
static int mt_alloc(void){
    for (int i=0;i<MT_CAP_MAP;i++) if (!mt_map[i].used) return i;
    return 0; // ?????? 0 ?(????)
}
static inline void mt_set(uint8_t L,uint8_t R,uint8_t C,uint8_t hold,uint8_t click){
    if (R>=KB_ROWS || C>=KB_COLS) {
		// uprintf("skip set\r\n");
		return;}        // ????
    if (!hold && !click){ 
		// uprintf("arrive set\r\n");
		int k=mt_find(L,R,C); if (k>=0) mt_map[k].used=false; return; }

	
    int k=mt_find(L,R,C); if (k<0) k=mt_alloc();
	// uprintf("pass set\r\n");
    mt_map[k]=(mt_entry_t){.L=L,.R=R,.C=C,.hold=hold,.click=click,.used=true};
	eeconfig_write_modtap(k, &mt_map[k]);
}
static inline bool mt_active(uint8_t L,uint8_t R,uint8_t C){ return mt_find(L,R,C) >= 0; }

// ??????? Mod-Tap(????)
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

// ?? API(??? keycode / record;?? L/R/C ? pressed ??)
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

static inline void mt_tick(uint8_t L)
{
    (void)L;
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

// ????:????????????(???/??)
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
    // ??????????,??????????
    if (mt_state_find(pL,pR,pC) < 0){
		// uprintf("in pednding: %d %d %d\r\n", pL,pR,pC);
        mt_set(pL,pR,pC,pHold,pClick);
        mt_pending=false;
        // ??????:uprintf("[MT] set L%u R%u C%u h=0x%02X k=0x%02X\r\n", pL,pR,pC,pHold,pClick);
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

void eeconfig_read_modtap(uint8_t idx, mt_entry_t *out)
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

// ??????????????????,??? rising edge ??
static bool toggle_last_pressed[KEYMAPS_COUNT][MATRIX_ROWS][MATRIX_COLS];

/*
 * Toggle-key EEPROM function prototypes.
 *
 * These declarations must appear before toggle_key_set() and
 * toggle_key_load_from_eeprom(), because those functions call the
 * EEPROM access functions before their actual definitions below.
 */
void eeconfig_write_toggle_entry(
    uint8_t index,
    const toggle_key_slot_t *entry
);

void eeconfig_read_toggle_entry(
    uint8_t index,
    toggle_key_slot_t *entry
);

void eeconfig_clear_toggle_entry(
    uint8_t index
);

void eeconfig_clear_all_toggle_entry(void);

// two api
bool toggle_key_set(uint8_t layer,uint8_t row,uint8_t col,uint8_t keycode)
{
    toggle_key_slot_t new_config;

    if (layer >= KEYMAPS_COUNT ||
        row >= MATRIX_ROWS ||
        col >= MATRIX_COLS) {

        uprintf(
            "[TOGGLE] Invalid position L=%u R=%u C=%u\r\n",
            (unsigned int)layer,
            (unsigned int)row,
            (unsigned int)col
        );

        return false;
    }

    memset(&new_config, 0, sizeof(new_config));

    new_config.layer   = layer;
    new_config.row     = row;
    new_config.col     = col;
    new_config.keycode = keycode;
    new_config.state   = false;

    /*
     * keycode == 0 disables the toggle key and clears
     * the corresponding EEPROM entry.
     */
    if (keycode == 0U) {
        bool entry_found = false;

        for (uint8_t i = 0U;
             i < toggle_key_pair_count;
             i++) {

            toggle_key_slot_t stored_config;

            memset(
                &stored_config,
                0,
                sizeof(stored_config)
            );

            eeconfig_read_toggle_entry(
                i,
                &stored_config
            );

            if (stored_config.enabled == 0x01U &&
                stored_config.layer == layer &&
                stored_config.row == row &&
                stored_config.col == col) {

                eeconfig_clear_toggle_entry(i);
                entry_found = true;

                uprintf(
                    "[TOGGLE] Cleared EEPROM index=%u "
                    "L=%u R=%u C=%u\r\n",
                    (unsigned int)i,
                    (unsigned int)layer,
                    (unsigned int)row,
                    (unsigned int)col
                );

                break;
            }
        }

        /*
         * The runtime configuration must also be cleared,
         * even when no EEPROM entry exists.
         */
        toggle_keys[layer][row][col].enabled = false;
        toggle_keys[layer][row][col].state   = false;
        toggle_keys[layer][row][col].keycode = 0U;
        toggle_keys[layer][row][col].layer   = layer;
        toggle_keys[layer][row][col].row     = row;
        toggle_keys[layer][row][col].col     = col;

        toggle_last_pressed[layer][row][col] = false;

        if (!entry_found) {
            uprintf(
                "[TOGGLE] No EEPROM entry to clear "
                "L=%u R=%u C=%u\r\n",
                (unsigned int)layer,
                (unsigned int)row,
                (unsigned int)col
            );
        }

        return true;
    }

    new_config.enabled = true;

    /*
     * First, search for an existing EEPROM entry with
     * the same layer, row and column.
     */
    for (uint8_t i = 0U;
         i < toggle_key_pair_count;
         i++) {

        toggle_key_slot_t stored_config;

        memset(
            &stored_config,
            0,
            sizeof(stored_config)
        );

        eeconfig_read_toggle_entry(
            i,
            &stored_config
        );

        if (stored_config.enabled == 0x01U &&
            stored_config.layer == layer &&
            stored_config.row == row &&
            stored_config.col == col) {

            eeconfig_write_toggle_entry(
                i,
                &new_config
            );

            /*
             * Update runtime RAM only after the EEPROM
             * update operation has been issued.
             */
            toggle_keys[layer][row][col] =
                new_config;

            toggle_last_pressed[layer][row][col] =
                false;

            uprintf(
                "[TOGGLE] Updated EEPROM index=%u "
                "L=%u R=%u C=%u KC=0x%02X\r\n",
                (unsigned int)i,
                (unsigned int)layer,
                (unsigned int)row,
                (unsigned int)col,
                (unsigned int)keycode
            );

            return true;
        }
    }

     /*
     * No existing entry was found. Search for the first
     * available EEPROM slot.
     */
    for (uint8_t i = 0U;
         i < toggle_key_pair_count;
         i++) {

        toggle_key_slot_t stored_config;

        memset(
            &stored_config,
            0,
            sizeof(stored_config)
        );

        eeconfig_read_toggle_entry(
            i,
            &stored_config
        );

        uprintf(
            "[TOGGLE] Scan idx=%u "
            "EN=%02X "
            "L=%u R=%u C=%u "
            "KC=%02X\r\n",
            (unsigned int)i,
            (unsigned int)stored_config.enabled,
            (unsigned int)stored_config.layer,
            (unsigned int)stored_config.row,
            (unsigned int)stored_config.col,
            (unsigned int)stored_config.keycode
        );

        if (stored_config.enabled != 0x01U) {

            uprintf(
                "[TOGGLE] Use free slot %u\r\n",
                (unsigned int)i
            );

            eeconfig_write_toggle_entry(
                i,
                &new_config
            );

            toggle_keys[layer][row][col] =
                new_config;

            toggle_last_pressed[layer][row][col] =
                false;

            uprintf(
                "[TOGGLE] Added EEPROM index=%u "
                "L=%u R=%u C=%u KC=0x%02X\r\n",
                (unsigned int)i,
                (unsigned int)layer,
                (unsigned int)row,
                (unsigned int)col,
                (unsigned int)keycode
            );

            return true;
        }
    }

    /*
     * EEPROM is full. Dump all entries.
     */
    uprintf("[TOGGLE] EEPROM FULL Dump\r\n");

    for (uint8_t i = 0U;
         i < toggle_key_pair_count;
         i++) {

        toggle_key_slot_t stored_config;

        memset(
            &stored_config,
            0,
            sizeof(stored_config)
        );

        eeconfig_read_toggle_entry(
            i,
            &stored_config
        );

        uprintf(
            " idx=%u "
            "EN=%02X "
            "L=%u R=%u C=%u "
            "KC=%02X\r\n",
            (unsigned int)i,
            (unsigned int)stored_config.enabled,
            (unsigned int)stored_config.layer,
            (unsigned int)stored_config.row,
            (unsigned int)stored_config.col,
            (unsigned int)stored_config.keycode
        );
    }

    uprintf(
        "[TOGGLE] EEPROM full, "
        "L=%u R=%u C=%u KC=0x%02X\r\n",
        (unsigned int)layer,
        (unsigned int)row,
        (unsigned int)col,
        (unsigned int)keycode
    );

    return false;
}

void toggle_key_load_from_eeprom(void)
{
    /*
     * Clear the runtime table before loading saved entries.
     * This prevents old RAM values from remaining after reload.
     */
    memset(toggle_keys, 0, sizeof(toggle_keys));
    memset(toggle_last_pressed, 0, sizeof(toggle_last_pressed));

    for (uint8_t i = 0; i < toggle_key_pair_count; i++)
    {
        toggle_key_slot_t tmp = {0};

        eeconfig_read_toggle_entry(i, &tmp);

        if (tmp.enabled == 0x01 &&
            tmp.layer < KEYMAPS_COUNT &&
            tmp.row < MATRIX_ROWS &&
            tmp.col < MATRIX_COLS)
        {
            toggle_key_slot_t *tk =
                &toggle_keys[tmp.layer][tmp.row][tmp.col];

            tk->enabled = true;
            tk->state   = false;
            tk->keycode = tmp.keycode;
            tk->layer   = tmp.layer;
            tk->row     = tmp.row;
            tk->col     = tmp.col;
        }
    }

    uprintf("[TOGGLE] load done\r\n");
}

uint8_t toggle_key_get(uint8_t layer,uint8_t row,uint8_t col)
{
    if (layer >= KEYMAPS_COUNT ||
        row >= MATRIX_ROWS ||
        col >= MATRIX_COLS)
    {
        return 0;
    }

    if (!toggle_keys[layer][row][col].enabled)
    {
        return 0;
    }

    return toggle_keys[layer][row][col].keycode;
}

void eeconfig_write_toggle_entry(uint8_t index,const toggle_key_slot_t *entry)
{
    if (index >= toggle_key_pair_count || entry == NULL)
    {
        return;
    }

    uint8_t *addr =
        EECONFIG_TOGGLE_KEY_ADDR +
        ((uint32_t)index * sizeof(toggle_key_slot_t));

    eeprom_update_block(
        (const void *)entry,
        (void *)addr,
        sizeof(toggle_key_slot_t)
    );
}

void eeconfig_read_toggle_entry(uint8_t index,toggle_key_slot_t *entry)
{
    if (entry == NULL)
    {
        return;
    }

    /*
     * Always initialize the destination. If index is invalid,
     * the caller receives a disabled empty entry.
     */
    memset(entry, 0, sizeof(toggle_key_slot_t));

    if (index >= toggle_key_pair_count)
    {
        return;
    }

    const uint8_t *addr =
        EECONFIG_TOGGLE_KEY_ADDR +
        ((uint32_t)index * sizeof(toggle_key_slot_t));

    eeprom_read_block(
        (void *)entry,
        (const void *)addr,
        sizeof(toggle_key_slot_t)
    );
}

void eeconfig_clear_toggle_entry(uint8_t index)
{
    if (index >= toggle_key_pair_count)
    {
        return;
    }

    toggle_key_slot_t empty = {0};

    eeconfig_write_toggle_entry(index, &empty);
}
void eeconfig_clear_all_toggle_entry(void)
{
    for (uint8_t i = 0; i < toggle_key_pair_count; i++)
    {
        eeconfig_clear_toggle_entry(i);

        uprintf(
            "clear toggle eeprom idx=%u\r\n",
            i
        );
    }

    memset(toggle_keys, 0, sizeof(toggle_keys));
    memset(toggle_last_pressed, 0, sizeof(toggle_last_pressed));

    uprintf("[TOGGLE] clear all done\r\n");
}


void toggle_key_dump_eeprom(void)
{
    uprintf(
        "\r\n===== TOGGLE EEPROM DUMP START =====\r\n"
    );

    for (uint8_t i = 0; i < toggle_key_pair_count; i++)
    {
        toggle_key_slot_t tmp = {0};

        eeconfig_read_toggle_entry(i, &tmp);

        uprintf(
            "[IDX %u] en=%u L=%u R=%u C=%u KC=0x%04X\r\n",
            i,
            tmp.enabled,
            tmp.layer,
            tmp.row,
            tmp.col,
            tmp.keycode
        );
    }

    uprintf(
        "===== TOGGLE EEPROM DUMP END =====\r\n\r\n"
    );
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

//extern bool setRunMode(uint8_t runMode, int8_t mode, uint8_t brightness, uint8_t speed);
void send_lamp_array(report_lamp_array_t* report);
void adcOffsetGet(bool findMax);

uint32_t testCB = 0,testADC = 0;


# ifndef DEBOUNCE
#define DEBOUNCE 5
#endif
//static uint8_t debouncing = DEBOUNCE;

/* matrix state(1:on, 0:off) */
matrix_row_t matrix[MATRIX_ROWS];
static matrix_row_t matrix_temp[MATRIX_ROWS];
//static bool debouncing = false;
//static uint16_t debouncing_time = 0;
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


const analogKey_Config_t analogKeyRelocate[ADC_NUM_CHANNELS][ADC_BUF_DEPTH] = {
    { { 0, 0},{ 1, 0},{ 2, 0},{ 3, 0},{ 4, 0},{ 5, 0},{ 0, 1},{ 1, 1},{ 2, 1},{ 3, 1},{ 5, 1},{ 1, 2},{ 2, 2},{ 3, 2},{ 4, 1},{ 5, 2} },
    { { 0, 2},{ 1, 3},{ 2, 3},{ 3, 3},{ 4, 2},{ 0, 3},{ 1, 4},{ 2, 4},{ 3, 4},{ 4, 3},{ 0, 4},{ 1, 5},{ 2, 5},{ 3, 5},{ 4, 4},{-1,-1} },
    { { 0, 5},{ 1, 6},{ 2, 6},{ 3, 6},{ 4, 5},{ 5, 3},{ 0, 6},{ 1, 7},{ 2, 7},{ 3, 7},{ 4, 6},{ 0, 7},{ 1, 8},{ 2, 8},{ 3, 8},{ 4, 7} },
    { { 0, 8},{ 1, 9},{ 2, 9},{ 3, 9},{ 4, 8},{ 0, 9},{ 1,10},{ 2,10},{ 3,10},{ 4, 9},{ 5, 4},{ 0,10},{ 1,11},{ 2,11},{ 4,10},{ 5, 5} },
    { { 0,11},{ 1,12},{ 2,12},{ 3,11},{ 0,12},{ 1,13},{ 2,13},{ 3,12},{ 4,11},{ 5, 6},{ 0,13},{-1,-1},{-1,-1},{-1,-1},{-1,-1},{-1,-1} },
    { { 5, 7},{-1,-1},{ 5, 8},{ 5, 9},{ 1,14},{-1,-1},{-1,-1},{-1,-1},{-1,-1},{-1,-1},{-1,-1},{-1,-1},{-1,-1},{-1,-1},{-1,-1},{-1,-1} },
    { {-1,-1},{ 4,12},{-1,-1},{-1,-1},{-1,-1},{ 2,14},{ 3,13},{-1,-1},{-1,-1},{-1,-1},{-1,-1},{-1,-1},{-1,-1},{-1,-1},{-1,-1},{-1,-1} },
};

const analogKey_Config_t matrixAnalogRelocate[MATRIX_ROWS][MATRIX_COLS] = {
    { { 0, 0},{ 0, 6},{ 1, 0},{ 1, 5},{ 1,10},{ 2, 0},{ 2, 6},{ 2,11},{ 3, 0},{ 3, 5},{ 3,11},{ 4, 0},{ 4, 4},{ 4,10},{-1,-1} },
    { { 0, 1},{ 0, 7},{ 0,11},{ 1, 1},{ 1, 6},{ 1,11},{ 2, 1},{ 2, 7},{ 2,12},{ 3, 1},{ 3, 6},{ 3,12},{ 4, 1},{ 4, 5},{ 5, 4} },
    { { 0, 2},{ 0, 8},{ 0,12},{ 1, 2},{ 1, 7},{ 1,12},{ 2, 2},{ 2, 8},{ 2,13},{ 3, 2},{ 3, 7},{ 3,13},{ 4, 2},{ 4, 6},{ 5, 5} },
    { { 0, 3},{ 0, 9},{ 0,13},{ 1, 3},{ 1, 8},{ 1,13},{ 2, 3},{ 2, 9},{ 2,14},{ 3, 3},{ 3, 8},{ 4, 3},{ 4, 7},{ 5, 6},{-1,-1} },
    { { 0, 4},{ 0,14},{ 1, 4},{ 1, 9},{ 1,14},{ 2, 4},{ 2,10},{ 2,15},{ 3, 4},{ 3, 9},{ 3,14},{ 4, 8},{ 5, 1},{-1,-1},{-1,-1} },
    { { 0, 5},{ 0,10},{ 0,15},{ 2, 5},{ 3,10},{ 3,15},{ 4, 9},{ 5, 0},{ 5, 2},{ 5, 3},{-1,-1},{-1,-1},{-1,-1},{-1,-1},{-1,-1} },
};


const uint8_t colMax[] = { 14, 15, 15, 14, 13, 10 };

static inline bool matrix_key_location_valid(analogKey_Config_t loc) {
    return loc.rowA >= 0 && loc.colA >= 0 &&
           loc.rowA < MATRIX_ROWS && loc.colA < MATRIX_COLS;
}



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
static bool isSOCD = true; // for toggle key

static inline void socd_setting_write(bool enabled) {
	eeprom_update_byte((uint8_t *)IS_SOCD_ON_ADD, enabled ? 0x01 : 0x00);
	eeprom_update_byte((uint8_t *)SOCD_SETTING_MAGIC_ADDR, SOCD_SETTING_MAGIC);
}

static bool socd_setting_read(void) {
	uint8_t magic = eeprom_read_byte((uint8_t *)SOCD_SETTING_MAGIC_ADDR);
	uint8_t value = eeprom_read_byte((uint8_t *)IS_SOCD_ON_ADD);

	if (magic != SOCD_SETTING_MAGIC) {
		socd_setting_write(true);
		return true;
	}

	return value == 0x01;
}

static inline void on_key_changed(uint8_t row, uint8_t col, bool pressed)
{
    uint8_t layer = biton32(layer_state);
    uint16_t keycode = keymaps[layer][row][col];

    //uprintf("KEY: kc=%04X pressed=%d\r\n", keycode, pressed);

    int8_t slot = socd_find_slot_by_keycode(keycode);

    if (slot >= 0) {
        socd_post_event(slot, keycode, pressed);
        return;
    }
}

static inline bool calc_pressed_now(uint8_t row, uint8_t col, int32_t adcCalc) {

    // ??????????????????
    bool prev = ( (matrix_temp[row] >> col) & 0x01 ) != 0;

    if (globleTrigger) {
        if (adcCalc > (keyMatrixTriggerLevel + draftValue)) return true;   // ??
        if (adcCalc <= keyMatrixReleaseLevel)               return false;  // ??
        return prev;  // ?????,??????
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

/*
static void external_eeprom_clear_all(void)
{
    uint8_t buffer[128];

    memset(buffer, 0xFF, sizeof(buffer));

    uprintf("[EEPROM] clear start\r\n");

    for (uint32_t addr = 0; addr < 65536U; addr += sizeof(buffer))
    {
        eeprom_write_block(
            buffer,
            (void *)(uintptr_t)addr,
            sizeof(buffer)
        );

        if ((addr & 0x0FFFU) == 0U)
        {
            uprintf(
                "[EEPROM] clear addr=0x%04lX\r\n",
                (unsigned long)addr
            );
        }
    }

    uprintf("[EEPROM] clear complete\r\n");
}
*/

#define EXT_EEPROM_CLEAR_MAGIC_ADDR   ((uint8_t *)0x007A)
#define EXT_EEPROM_CLEAR_MAGIC_VALUE  0x5A

bool isMac = false;
static hsv_t current_hsv;
extern led_config_t c;

void hook_early_init(void)
{
    uint8_t clear_magic;

    palSetPadMode(GPIOC, 13, PAL_MODE_OUTPUT_PUSHPULL);				//Analog_PWR
    palClearPad(GPIOC, 13);

    i2c_init();
    uart_init();
		osalThreadSleepMilliseconds(1000); // led delay add in here work
	

    /*
     * MCU reset EEPROM
     */
    clear_magic =
        eeprom_read_byte(EXT_EEPROM_CLEAR_MAGIC_ADDR);

    if (clear_magic != EXT_EEPROM_CLEAR_MAGIC_VALUE) {

        //external_eeprom_clear_all();

        /*
         * external EEPROM ?
         * 
         */
        eeprom_update_byte(
            EXT_EEPROM_CLEAR_MAGIC_ADDR,
            EXT_EEPROM_CLEAR_MAGIC_VALUE
        );
    }

    uart_init();
		
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
    palSetPadMode(GPIOA, 15, PAL_MODE_INPUT_PULLUP);
    memset(matrix, 0, sizeof(matrix));

	// sensorPowerCtrl(true);
	#ifdef socd
	void socd_load_all_from_eeprom(void);

	#endif
}

void AdcPortChange(bool mode)
{
    /*
     * ADC pins are always kept in analog input mode.
     */
    (void)mode;

    palSetPadMode(GPIOC, 0, PAL_MODE_INPUT_ANALOG);
    palSetPadMode(GPIOC, 1, PAL_MODE_INPUT_ANALOG);
    palSetPadMode(GPIOC, 2, PAL_MODE_INPUT_ANALOG);
    palSetPadMode(GPIOC, 3, PAL_MODE_INPUT_ANALOG);

    palSetPadMode(GPIOA, 0, PAL_MODE_INPUT_ANALOG);
    palSetPadMode(GPIOA, 1, PAL_MODE_INPUT_ANALOG);
    palSetPadMode(GPIOA, 2, PAL_MODE_INPUT_ANALOG);
}

void sensorPowerCtrl(bool enable){
	if (enable){
	    palSetPad(GPIOC, 13);
	} else
	{
	    palClearPad(GPIOC, 13);
	}
}

void hook_late_init(void)
{
    palSetPad(GPIOB, 3);
    palSetPad(GPIOB, 4);

    idle_led_last_activity_ms = timer_read32();
    idle_led_timeout_reported = false;
    idle_led_is_sleeping = false;

    idle_led_restore_mode = idle_led_build_mode(
        currentLED_RunMode,
        normalMode,
        eeprom_read_byte(KB_COLOR_MODE_ADDR)
    );

    macro_engine_init();

    usb_set_polling_rate(
        eeprom_read_byte((uint8_t *)EECONFIG_POLLING_RATE)
    );

    load_all_rappy_snappy();
    modtap_load_form_eeprom();

    isSOCD = socd_setting_read();
    isMac = eeprom_read_byte((uint8_t *)EECONFIG_MAC_MODE);

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

adcsample_t samples1[ADC_NUM_CHANNELS];
adcsample_t samples2[ADC_BUF_DEPTH][ADC_NUM_CHANNELS], initPostionLevel[ADC_BUF_DEPTH][ADC_NUM_CHANNELS], travelPostionLevel[ADC_BUF_DEPTH][ADC_NUM_CHANNELS]; ;
// volatile adcsample_t keyRawDepth[MATRIX_ROWS][MATRIX_COLS];
// volatile adcsample_t keyDepth[MATRIX_ROWS][MATRIX_COLS];
adcsample_t keyDepth[MATRIX_ROWS][MATRIX_COLS]; // CHANGE BACK
// adcsample_t adc2mmLevel[ADC_BUF_DEPTH][ADC_NUM_CHANNELS];
static int32_t adcsampleIndex = 0, adcUpdateIndex = 0;
static void adccallback(ADCDriver* adcp);
static void adcerrorcallback(ADCDriver* adcp, adcerror_t err);

/*
 * Filtered ADC values used by the keyboard processing logic.
 */
static adcsample_t adcFiltered[ADC_BUF_DEPTH][ADC_NUM_CHANNELS];
static bool adcFilterReady[ADC_BUF_DEPTH];

#define HIGH_MODE_1

#ifdef  HIGH_MODE_1
#define ADC_FILTER_DEADBAND       7
#define ADC_FILTER_MEDIUM_DELTA  16
#define ADC_FILTER_FAST_DELTA    80
#endif

#ifdef  HIGH_MODE
#define ADC_FILTER_DEADBAND       5
#define ADC_FILTER_MEDIUM_DELTA  16
#define ADC_FILTER_FAST_DELTA    64
#endif

#ifdef  MID_MODE
#define ADC_FILTER_DEADBAND       3
#define ADC_FILTER_MEDIUM_DELTA  12
#define ADC_FILTER_FAST_DELTA    48
#endif

#ifdef  LOW_MODE
#define ADC_FILTER_DEADBAND       2
#define ADC_FILTER_MEDIUM_DELTA  	8
#define ADC_FILTER_FAST_DELTA    32
#endif

#define ADC_GAIN_NUMERATOR       33U
#define ADC_GAIN_DENOMINATOR     10U
#define ADC_MAX_VALUE ((int32_t)4200)

static inline adcsample_t adc_apply_digital_gain(adcsample_t value)
{
    uint32_t amplified;

    amplified = ((uint32_t)value * ADC_GAIN_NUMERATOR) / ADC_GAIN_DENOMINATOR;

    if (amplified > ADC_MAX_VALUE) {amplified = ADC_MAX_VALUE;}

    return (adcsample_t)amplified;
}

static void adc_filter_update_channel(uint8_t channel)
{
    if (channel >= ADC_BUF_DEPTH) {
        return;
    }

    if (!adcFilterReady[channel])
    {
        for (uint8_t group = 0U;
             group < ADC_NUM_CHANNELS;
             group++)
        {
					adcFiltered[channel][group] = adc_apply_digital_gain(samples2[channel][group]);
        }

        adcFilterReady[channel] = true;
        return;
    }

    for (uint8_t group = 0U;
         group < ADC_NUM_CHANNELS;
         group++)
    {
        int32_t raw;
        int32_t filtered;
        int32_t difference;
        int32_t absolute_difference;
        int32_t correction;

				raw = (int32_t)adc_apply_digital_gain(samples2[channel][group]);

        filtered =
            (int32_t)adcFiltered[channel][group];

        difference =
            raw - filtered;

        absolute_difference =
            difference >= 0 ?
            difference :
            -difference;

        /*
         * Reject very small ADC noise.
         */
        if (absolute_difference <=
            ADC_FILTER_DEADBAND)
        {
            continue;
        }

        /*
         * Use an adaptive filter:
         *
         * Large movement : 1/2 correction
         * Medium movement: 1/4 correction
         * Small movement : 1/8 correction
         */
        if (absolute_difference >=
            ADC_FILTER_FAST_DELTA)
        {
            correction = difference / 2;
        }
        else if (absolute_difference >=
                 ADC_FILTER_MEDIUM_DELTA)
        {
            correction = difference / 4;
        }
        else
        {
            correction = difference / 8;

            /*
             * Ensure the filter can still move by one count.
             */
            if (correction == 0)
            {
                correction =
                    difference > 0 ? 1 : -1;
            }
        }

        filtered += correction;

        if (filtered < 0) {
            filtered = 0;
        }

        if (filtered > ADC_MAX_VALUE) {
            filtered = ADC_MAX_VALUE;
        }

        /* Store the calculated filtered value. */
        adcFiltered[channel][group] = (adcsample_t)filtered;
    }
}

uint16_t matrix_get_filtered_adc(uint8_t channel,uint8_t group)
{
    if (channel >= ADC_BUF_DEPTH || group >= ADC_NUM_CHANNELS)
    {
        return 0U;
    }

    return (uint16_t)adcFiltered[channel][group];
}

/*
 * Return an ADC-scale value whose calibrated travel is linearized.
 *
 * Do not clamp values below init to init.  initPostionLevel is a trigger
 * calibration reference, not the first valid display sample.  Clamping here
 * hid real ADC movement at the beginning of travel as a flat segment in the
 * PC curve viewer.  The key-scan trigger path keeps its existing reference
 * and dead-band behaviour.
 */
uint16_t matrix_get_linearized_adc(uint8_t channel, uint8_t group)
{
    int32_t raw;
    int32_t init;
    int32_t travel;
    int32_t range;
    int32_t linear_steps;
    int32_t linear_adc;

    if (channel >= ADC_BUF_DEPTH || group >= ADC_NUM_CHANNELS) {
        return 0U;
    }

    raw = (int32_t)adcFiltered[channel][group];
    init = (int32_t)initPostionLevel[channel][group];
    travel = (int32_t)travelPostionLevel[channel][group];
    range = travel - init;

    if (range <= 0) {
        return (uint16_t)raw;
    }

    if (raw <= init) {
        return (uint16_t)raw;
    }

    linear_steps = adc_linearize_travel(raw - init, range);
    linear_adc = init +
                 (linear_steps * range + (TRAVEL_STEPS / 2)) /
                 TRAVEL_STEPS;

    if (linear_adc < 0) {
        linear_adc = 0;
    } else if (linear_adc > ADC_MAX_VALUE) {
        linear_adc = ADC_MAX_VALUE;
    }

    return (uint16_t)linear_adc;
}

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
void irq_storm_gpt9_cb(GPTDriver *gptp)
{
    (void)gptp;

	sensorPowerCtrl(true);
	testCB++;
	adcsampleIndex = -1;
	adcStartConversion(&ADCD1, &adcgrpcfg2, samples1, 1);
//	adcStartConversion_Start(&ADCD1,ADC_CR2_SWSTART);
}
#endif

#ifdef ADC_v2
void irq_storm_gpt9_cb(GPTDriver *gptp)
{
    (void)gptp;
	sensorPowerCtrl(true);
	testCB++;
//	adcsampleIndex = -1;
	adcStartConversion(&ADCD1, &adcgrpcfg2, samples1, 1);
//	adcStartConversion_Start(&ADCD1,ADC_CR2_SWSTART);
}
#endif

#ifdef ADC_v4
void irq_storm_gpt9_cb(GPTDriver *gptp)
{
    (void)gptp;
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
void irq_storm_gpt9_cb(GPTDriver *gptp)
{
    (void)gptp;
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
            if (adcFiltered[j][i] > initPostionLevel[j][i])
            {
                initPostionLevel[j][i] = adcFiltered[j][i];
            }
        }
    }
}

void adcOffsetCalc(bool findMax, bool update, uint16_t samples)
{
#ifdef nonOP
    uint16_t k = 0;
#endif
    uint32_t SampleTimer;

    (void)findMax;

    memset(initPostionLevel, 0, sizeof(initPostionLevel));

    adcUpdateIndex = 0;
    SampleTimer = timer_read();
    currentKeyMatrixMode = keyMatrixMode_InitialPostionCalc;

    lastKeyScanEffect = keyScanEffect;
    lastRapidKeyEnable = rapidKeyEnable;
    lastkeycodeOutput = keycodeOutput;

    keyScanEffect = true;
    rapidKeyEnable = false;
    keycodeOutput = false;

    while (timer_read() < (SampleTimer + samples * 10U))
    {
        for (uint8_t i = 0; i < ADC_NUM_CHANNELS; i++)
        {
            for (uint8_t j = 0; j < ADC_BUF_DEPTH; j++)
            {
#ifdef nonOP
                if (adcFiltered[j][i] > initPostionLevel[j][i])
                {
                    if (k++ < 500U) {
                        initPostionLevel[j][i] = adcFiltered[j][i] + 3U;
                    } else {
                        initPostionLevel[j][i] = adcFiltered[j][i] + 5U;
                    }
                }
#else
                if (adcFiltered[j][i] > initPostionLevel[j][i])
                {
#ifdef minTriggerCheck
                    initPostionLevel[j][i] = adcFiltered[j][i] + 2U;
#else
                    initPostionLevel[j][i] = adcFiltered[j][i] + 5U;
#endif
                }
#endif
            }
        }
    }

    if (update)
    {
        keyScanEffect = false;
        uprintf("offset calc\r\n");

        for (uint16_t i = 0;
             i < (ADC_NUM_CHANNELS * ADC_BUF_DEPTH);
             i++)
        {
            eeprom_write_word(
                EECONFIG_KEYMAP_OFFSET_CALIBRATION + i * 2U,
                initPostionLevel[i % ADC_BUF_DEPTH][i / ADC_BUF_DEPTH]
            );
        }
    }

    currentKeyMatrixMode = keyMatrixMode_Normal;
    keyScanEffect = false;
    rapidKeyEnable = lastRapidKeyEnable;
    keycodeOutput = lastkeycodeOutput;
}

void adcTravelCalc(uint8_t mode)
{
    uint16_t i;

    switch (mode)
    {
        case 0:
            lastKeyScanEffect = keyScanEffect;
            lastRapidKeyEnable = rapidKeyEnable;
            lastkeycodeOutput = keycodeOutput;

            currentLED_RunMode.type = offMode;
            setRunMode(currentLED_RunMode);
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
            uprintf("Start:%X\r\n",
                    EECONFIG_KEYMAP_TRAVEL_CALIBRATION);

            for (i = 0;
                 i < (ADC_NUM_CHANNELS * ADC_BUF_DEPTH);
                 i++)
            {
                eeprom_write_word(
                    EECONFIG_KEYMAP_TRAVEL_CALIBRATION + i * 2U,
                    travelPostionLevel[
                        i % ADC_BUF_DEPTH
                    ][
                        i / ADC_BUF_DEPTH
                    ]
                );
            }

            uprintf(
                "End:%X\r\n\r\n",
                EECONFIG_KEYMAP_TRAVEL_CALIBRATION + i * 2U
            );

            keyScanEffect = lastKeyScanEffect;
            rapidKeyEnable = lastRapidKeyEnable;
            keycodeOutput = lastkeycodeOutput;
            break;

        default:
            break;
    }
}

static void muxSelect(uint8_t channel){
	// uprintf("mux idx=%d\r\n", channel);
	uint8_t data, data_pa, data_pf;
	data = 15-channel;
	data_pa = ((data & 0x01) | ((data >> 2) & 0x1E)) << 3;
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

#ifdef ADC_v4
static void adccallback(ADCDriver* adcp)
{
    (void)adcp;    // 消除 unused parameter 警告

	muxSelect(++adcsampleIndex);
}
#endif

#ifdef ADC_v5
static void adccallback(ADCDriver *adcp)
{
    uint8_t completed_channel;

    (void)adcp;

    completed_channel =
        (uint8_t)adcsampleIndex;

    if (completed_channel < ADC_BUF_DEPTH)
    {
        adc_filter_update_channel(completed_channel);
    }

    chBSemSignalI(&adcCompleteSem);

    sensorPowerCtrl(false);
    muxSelect(++adcsampleIndex);

#ifndef ADC_Oversample
    gptStartOneShot(&GPTD9, 14);
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
	data_pa = ((data & 0x01) | ((data >> 2) & 0x1E)) << 3;
	data_pf = ((data >> 1) & 0x03) << 4;
	//uprintf ("pa:%X pf:%X\r\n",data_pa,data_pf);
	palWriteGroup(GPIOA, PAL_PORT_BIT(3) | PAL_PORT_BIT(4), 0, data_pa);
	palWriteGroup(GPIOF, PAL_PORT_BIT(4) | PAL_PORT_BIT(5), 0, data_pf);
}

uint16_t current_keycode = KC_NO;
uint8_t level = 0;

static void eeclr_task(void)
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
		eeprom_update_byte((uint8_t *)KB_COLOR_MODE_ADDR, currentLED_RunMode.mode);
		eeprom_update_byte((uint8_t *)EECONFIG_POLLING_RATE, 0x00);
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
		eeprom_update_byte((uint8_t *)EEPROM_FACTORY_INIT_FLAG_ADDR, 0xFA);
		uprintf("\r\nEnd\r\n");
		chThdSleepMilliseconds(500);
		NVIC_SystemReset();
	}
}

#ifdef eepromClear
static void perform_factory_reset(void)
{
    currentLED_RunMode.mode = 0;
    currentLED_RunMode.type = normalMode;
    setRunMode(currentLED_RunMode);

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
#endif // eepromClear

bool matrixAnalogScan(void)
{
    int adcCalc, adcPre;
    led_config_t EffectranbowColor;
    float mult;
    int8_t currentStep = 0;

#ifdef eepromClear

static bool reset_checked = false;
static uint16_t reset_check_delay = 0U;
static uint8_t reset_pressed_count = 0U;

if (!reset_checked)
{
    analogKey_Config_t loc;
    int raw_adc;
    int init_adc;
    int adc_val;

    /*
     * Wait for ADC conversion and calibration data loading.
     * Adjust this value according to the actual ADC scan rate.
     */
    if (reset_check_delay < 100U)
    {
        reset_check_delay++;

        if (reset_check_delay == 1U) {
            uprintf(
                "[FACTORY RESET CHECK] Waiting for ADC and "
                "calibration data\r\n"
            );
        }
    }
    else
    {
        loc = matrixAnalogRelocate[0][0];

        uprintf(
            "[FACTORY RESET CHECK] Location rowA=%d colA=%d\r\n",
            loc.rowA,
            loc.colA
        );

        /*
         * Validate ADC array indexes before accessing the arrays.
         */
        if (loc.rowA < 0 ||
            loc.rowA >= ADC_BUF_DEPTH ||
            loc.colA < 0 ||
            loc.colA >= ADC_NUM_CHANNELS)
        {
            uprintf(
                "[FACTORY RESET CHECK] ERROR invalid location "
                "rowA=%d colA=%d\r\n",
                loc.rowA,
                loc.colA
            );

            reset_checked = true;
        }
        else
        {
            raw_adc =
                samples2[loc.rowA][loc.colA];

            init_adc =
                initPostionLevel[loc.rowA][loc.colA];

            /*
             * An erased or unloaded calibration value must never be used
             * for factory-reset detection.
             */
            if (init_adc == 0 ||
                init_adc == 0xFFFF)
            {
                uprintf(
                    "[FACTORY RESET CHECK] SKIP invalid calibration "
                    "raw=%d init=%d\r\n",
                    raw_adc,
                    init_adc
                );

                reset_checked = true;
                reset_pressed_count = 0U;
            }
            else
            {
                adc_val = raw_adc - init_adc;

                uprintf(
                    "[FACTORY RESET CHECK] raw=%d init=%d "
                    "delta=%d trigger=%d count=%u\r\n",
                    raw_adc,
                    init_adc,
                    adc_val,
                    keyMatrixTriggerLevel,
                    (unsigned int)reset_pressed_count
                );

                /*
                 * Require several consecutive pressed samples to avoid
                 * reset caused by ADC startup noise.
                 */
                if (adc_val > keyMatrixTriggerLevel)
                {
                    if (reset_pressed_count < 3U) {
                        reset_pressed_count++;
                    }

                    if (reset_pressed_count >= 3U)
                    {
                        uprintf(
                            "[FACTORY RESET CHECK] CONFIRMED "
                            "CALL perform_factory_reset()\r\n"
                        );

                        /*
                         * Allow the USB debug message to be transmitted.
                         */
                        chThdSleepMilliseconds(100);

                        reset_checked = true;
                        perform_factory_reset();
                    }
                }
                else
                {
                    uprintf(
                        "[FACTORY RESET CHECK] NO RESET\r\n"
                    );

                    reset_pressed_count = 0U;
                    reset_checked = true;
                }
            }
        }
    }
}

#endif // eepromClear

#ifdef minTriggerCheck
    if (rapidKeyEnable) {
        draftValue = 0;
    } else {
        draftValue = 15;
    }
#else
    draftValue = 0;
#endif

    for (uint8_t group = 0; group < ADC_NUM_CHANNELS; group++)
    {
        for (uint8_t channel = 0; channel < ADC_BUF_DEPTH; channel++)
        {
            bool dksKeyMatched = false;

            keyLocation = analogKeyRelocate[group][channel];

            if (!matrix_key_location_valid(keyLocation)) {
                continue;
            }

            switch (currentKeyMatrixMode)
            {
                case keyMatrixMode_Normal:
                case keyMatrixMode_InitialPostionCalc:
										adcCalc =
												(int32_t)adcFiltered[channel][group] -
												(int32_t)initPostionLevel[channel][group];
                    break;

                case keyMatrixMode_TravelPostionCalc:
											if (travelPostionLevel[channel][group] <
													adcFiltered[channel][group])
											{
													travelPostionLevel[channel][group] =
															adcFiltered[channel][group];
											}

                    adcCalc =
                        travelPostionLevel[channel][group] -
                        initPostionLevel[channel][group];

                    if (adcCalc < 32) {
                        adcCalc = 0;
                    }
                    break;
            }

            adcsample_t travel =
                travelPostionLevel[channel][group];

            adcsample_t init =
                initPostionLevel[channel][group];

            int32_t range =
                (int32_t)travel -
                (int32_t)init;

            adcPre = adc_linearize_travel(adcCalc, range);

            if (adcPre < 0) {
                adcPre = 0;
            }

            if (adcPre >= TRAVEL_STEPS) {
                adcPre = TRAVEL_STEPS;
            }

            if (adcCalc < 0) {
                adcCalc = 0;
            }

            /*
             * Dynamic Keystroke
             *
             * DKS row/col use logical keyboard matrix coordinates.
             * keyLocation.rowA/colA are the logical matrix coordinates
             * converted from the current ADC scan position.
             */

            if (rapidKeyEnable)
            {
                for (uint8_t i = 0U;
                     i < RAPID_TRIGGER_COUNT;
                     i++)
                {
                    if (rapidTriggerKey[i].key.enable != 0x01U ||
                        rapidTriggerKey[i].key.counts == 0U)
                    {
                        continue;
                    }

                    if (rapidTriggerKey[i].key.row != keyLocation.rowA ||
                        rapidTriggerKey[i].key.col != keyLocation.colA)
                    {
                        continue;
                    }

                    /*
                     * This physical key belongs to DKS.
                     */
                    dksKeyMatched = true;
                    currentStep = 0;

                    /*
                     * Prevent the original physical key from being
                     * reported through the normal matrix path.
                     */
                    if (keyLocation.rowA >= 0 &&
                        keyLocation.rowA < MATRIX_ROWS &&
                        keyLocation.colA >= 0 &&
                        keyLocation.colA < MATRIX_COLS)
                    {
                        matrix_temp[keyLocation.rowA] &=
                            ~((matrix_row_t)1U <<
                              keyLocation.colA);
                    }

                    /*
                     * Fully released.
                     */
                    if (adcCalc <
                        rapidTriggerKey[i].onLevel[0])
                    {
                        if (rapidTriggerKey[i].lastStep >= 0 &&
                            rapidTriggerKey[i].lastStep <
                                rapidTriggerKey[i].key.counts &&
                            adcCalc <= 4)
                        {
                            uint8_t last =
                                (uint8_t)
                                rapidTriggerKey[i].lastStep;

                            uprintf(
                                "[DKS RELEASE] IDX=%u STEP=%u "
                                "KC=%02X ADC=%d\r\n",
                                (unsigned int)i,
                                (unsigned int)last,
                                (unsigned int)
                                    rapidTriggerKey[i]
                                        .key.keycode[last],
                                adcCalc
                            );

                            if (keycodeOutput) {
                                unregister_rapidTrigger_code(
                                    rapidTriggerKey[i]
                                        .key.keycode[last]
                                );
                            }

                            rapidTriggerKey[i].lastStep = -1;
                            rapidTriggerKey[i].triggeredLevel = 0;

                            if (keyScanEffect &&
                                ledKeyMatrix[
                                    keyLocation.rowA
                                ][
                                    keyLocation.colA
                                ] != -1)
                            {
                                LED_Datas[
                                    ledKeyMatrix[
                                        keyLocation.rowA
                                    ][
                                        keyLocation.colA
                                    ]
                                ] = LED_NULL;
                            }
                        }

                        break;
                    }

                    /*
                     * Calculate DKS step.
                     */
                    currentStep = 0;

                    for (uint8_t j = 1U;
                         j < rapidTriggerKey[i].key.counts;
                         j++)
                    {
                        if (adcCalc >=
                                rapidTriggerKey[i].onLevel[j] &&
                            adcCalc <
                                rapidTriggerKey[i]
                                    .onLevel[j + 1U])
                        {
                            currentStep++;
                            break;
                        }

                        if (adcCalc >
                            rapidTriggerKey[i]
                                .onLevel[j + 1U])
                        {
                            currentStep++;
                        }
                    }

                    /*
                     * Protect keycode[] from an invalid step.
                     */
                    if (currentStep < 0) {
                        currentStep = 0;
                    }

                    if (currentStep >=
                        rapidTriggerKey[i].key.counts)
                    {
                        currentStep =
                            rapidTriggerKey[i]
                                .key.counts - 1;
                    }

                    /*
                     * Never use rainbowColor[currentStep - 1].
                     */
                    EffectranbowColor =
                        rainbowColor[
                            (uint8_t)currentStep
                        ];

                    /*
                     * Print only when step changes.
                     */
                    if (rapidTriggerKey[i].lastStep !=
                        currentStep)
                    {
                        uprintf(
                            "[DKS STEP] IDX=%u R=%u C=%u "
                            "ADC=%d STEP=%d LAST=%d "
                            "KC=%02X\r\n",
                            (unsigned int)i,
                            (unsigned int)
                                keyLocation.rowA,
                            (unsigned int)
                                keyLocation.colA,
                            adcCalc,
                            (int)currentStep,
                            (int)
                                rapidTriggerKey[i]
                                    .lastStep,
                            (unsigned int)
                                rapidTriggerKey[i]
                                    .key.keycode[
                                        (uint8_t)currentStep
                                    ]
                        );
                    }

#ifdef nonOP
                    if (
                        rapidTriggerKey[i].lastStep !=
                            currentStep &&
                        (
                            rapidTriggerKey[i]
                                .triggeredLevel == 0 ||
                            abs(
                                rapidTriggerKey[i]
                                    .triggeredLevel -
                                adcCalc
                            ) > 8
                        ) &&
                        (
                            adcCalc <
                                rapidTriggerKey[i]
                                    .onLevel[
                                        rapidTriggerKey[i]
                                            .key.counts
                                    ] ||
                            rapidTriggerKey[i]
                                .key.counts ==
                                RAPID_TRIGGER_LEVELS
                        )
                    )
#else
                    if (
                        rapidTriggerKey[i].lastStep !=
                            currentStep &&
                        (
                            rapidTriggerKey[i]
                                .triggeredLevel == 0 ||
                            abs(
                                rapidTriggerKey[i]
                                    .triggeredLevel -
                                adcCalc
                            ) > 30
                        ) &&
                        (
                            adcCalc <
                                rapidTriggerKey[i]
                                    .onLevel[
                                        rapidTriggerKey[i]
                                            .key.counts
                                    ] ||
                            rapidTriggerKey[i]
                                .key.counts ==
                                RAPID_TRIGGER_LEVELS
                        )
                    )
#endif
                    {
                        /*
                         * Release previous DKS step only when
                         * lastStep is valid.
                         */
                        if (rapidTriggerKey[i].lastStep >= 0 &&
                            rapidTriggerKey[i].lastStep <
                                rapidTriggerKey[i].key.counts)
                        {
                            uint8_t last =
                                (uint8_t)
                                rapidTriggerKey[i].lastStep;

                            uprintf(
                                "[DKS RELEASE] IDX=%u STEP=%u "
                                "KC=%02X\r\n",
                                (unsigned int)i,
                                (unsigned int)last,
                                (unsigned int)
                                    rapidTriggerKey[i]
                                        .key.keycode[last]
                            );

                            if (keycodeOutput) {
                                unregister_rapidTrigger_code(
                                    rapidTriggerKey[i]
                                        .key.keycode[last]
                                );
                            }
                        }

                        /*
                         * Press current DKS step.
                         */
                        uprintf(
                            "[DKS PRESS] IDX=%u STEP=%d "
                            "KC=%02X ADC=%d\r\n",
                            (unsigned int)i,
                            (int)currentStep,
                            (unsigned int)
                                rapidTriggerKey[i]
                                    .key.keycode[
                                        (uint8_t)currentStep
                                    ],
                            adcCalc
                        );

                        if (keycodeOutput) {
                            register_rapidTrigger_code(
                                rapidTriggerKey[i]
                                    .key.keycode[
                                        (uint8_t)currentStep
                                    ]
                            );
                        }

                        rapidTriggerKey[i].triggeredLevel =
                            adcCalc;

                        rapidTriggerKey[i].lastStep =
                            currentStep;

                        if (keyScanEffect &&
                            ledKeyMatrix[
                                keyLocation.rowA
                            ][
                                keyLocation.colA
                            ] != -1)
                        {
                            LED_Datas[
                                ledKeyMatrix[
                                    keyLocation.rowA
                                ][
                                    keyLocation.colA
                                ]
                            ] =
                                rainbowColor[
                                    (uint8_t)currentStep
                                ];
                        }
                    }

                    /*
                     * Only one DKS slot can own this matrix key.
                     */
                    break;
                }
            }

            /*
             * A DKS key must not continue into normal keyboard,
             * Mod Tap, Toggle Key, SOCD, or matrix_temp handling.
             *
             * Normal keys continue through the original path.
             */
            if (dksKeyMatched) {
                continue;
            }

            /*
             * Original LED scan effect.
             */
            if (keyScanEffect)
            {
                if (adcCalc > 150)
                {
                    adcCalc -= 150;

                    EffectranbowColor =
                        rainbowColor[group];

                    if (adcCalc > 600) {
                        mult = 1;
                    } else {
                        mult =
                            (float)adcCalc /
                            600;
                    }

                    EffectranbowColor.red *= mult;
                    EffectranbowColor.green *= mult;
                    EffectranbowColor.blue *= mult;
                }
                else
                {
                    EffectranbowColor =
                        LED_NULL;
                }

                if (ledKeyMatrix[
                        keyLocation.rowA
                    ][
                        keyLocation.colA
                    ] != -1)
                {
                    LED_Datas[
                        ledKeyMatrix[
                            keyLocation.rowA
                        ][
                            keyLocation.colA
                        ]
                    ] = EffectranbowColor;

                    chBSemSignal(
                        &ledUpdateSem
                    );
                }
            }

#ifdef rappySnappy
            adcPre_value[
                keyLocation.rowA
            ][
                keyLocation.colA
            ] = (int16_t)adcPre;

            if (rappy_snappy_is_key(
                    keyLocation.rowA,
                    keyLocation.colA))
            {
                continue;
            }
#endif // rappySnappy

#ifdef rappyTrigger
            if (keyLocation.rowA == -1 &&
                keyLocation.colA == -1)
            {
                continue;
            }
#endif // rappyTrigger

#ifdef socd
            {
                static bool socd_last_pressed
                    [MATRIX_ROWS][MATRIX_COLS];

                bool socd_pressed_now =
                    calc_pressed_now(
                        keyLocation.rowA,
                        keyLocation.colA,
                        adcPre
                    );

                bool socd_previous =
                    socd_last_pressed[
                        keyLocation.rowA
                    ][
                        keyLocation.colA
                    ];

                if (isSOCD &&
                    socd_previous !=
                        socd_pressed_now)
                {
                    socd_last_pressed[
                        keyLocation.rowA
                    ][
                        keyLocation.colA
                    ] = socd_pressed_now;

                    on_key_changed(
                        keyLocation.rowA,
                        keyLocation.colA,
                        socd_pressed_now
                    );
                }
            }
#endif // socd

#ifdef modTap
            {
                uint8_t L = 0U;

                bool modtap_pressed_now =
                    calc_pressed_now(
                        keyLocation.rowA,
                        keyLocation.colA,
                        adcPre
                    );

                bool modtap_previous =
                    mt_prev[
                        keyLocation.rowA
                    ][
                        keyLocation.colA
                    ];

                if (mt_active(
                        L,
                        keyLocation.rowA,
                        keyLocation.colA))
                {
                    if (modtap_pressed_now &&
                        !modtap_previous)
                    {
                        mt_on_down(
                            L,
                            keyLocation.rowA,
                            keyLocation.colA
                        );
                    }

                    if (!modtap_pressed_now &&
                        modtap_previous)
                    {
                        mt_on_up(
                            L,
                            keyLocation.rowA,
                            keyLocation.colA
                        );
                    }

                    mt_prev[
                        keyLocation.rowA
                    ][
                        keyLocation.colA
                    ] = modtap_pressed_now;

                    continue;
                }
            }
#endif // modTap

#ifdef toggleKey
            {
                uint8_t act_layer =
                    biton32(layer_state);

                if (keyLocation.rowA >= 0 &&
                    keyLocation.colA >= 0 &&
                    keyLocation.rowA < MATRIX_ROWS &&
                    keyLocation.colA < MATRIX_COLS &&
                    act_layer < KEYMAPS_COUNT)
                {
                    toggle_key_slot_t *tk =
                        &toggle_keys[
                            act_layer
                        ][
                            keyLocation.rowA
                        ][
                            keyLocation.colA
                        ];

                    if (tk->enabled)
                    {
                        bool was_pressed =
                            toggle_last_pressed[
                                act_layer
                            ][
                                keyLocation.rowA
                            ][
                                keyLocation.colA
                            ];

                        bool now_pressed =
                            was_pressed;

                        int32_t toggle_trigger_level;
                        int32_t toggle_release_level;

                        if (globleTrigger)
                        {
                            toggle_trigger_level =
                                (int32_t)
                                    keyMatrixTriggerLevel +
                                (int32_t)
                                    draftValue;

                            toggle_release_level =
                                (int32_t)
                                    keyMatrixReleaseLevel;
                        }
                        else
                        {
                            toggle_trigger_level =
                                (int32_t)
                                    eeconfig_read_Trigger(
                                        keyLocation.rowA,
                                        keyLocation.colA
                                    ) +
                                (int32_t)
                                    draftValue;

                            toggle_release_level =
                                (int32_t)
                                    eeconfig_read_Release(
                                        keyLocation.rowA,
                                        keyLocation.colA
                                    );
                        }

                        if (!was_pressed &&
                            adcPre >
                                toggle_trigger_level)
                        {
                            now_pressed = true;
                        }
                        else if (
                            was_pressed &&
                            adcPre <=
                                toggle_release_level)
                        {
                            now_pressed = false;
                        }

                        toggle_last_pressed[
                            act_layer
                        ][
                            keyLocation.rowA
                        ][
                            keyLocation.colA
                        ] = now_pressed;

                        if (!was_pressed &&
                            now_pressed)
                        {
                            tk->state =
                                !tk->state;

                            if (tk->state)
                            {
                                if (tk->keycode) {
                                    register_code(
                                        tk->keycode
                                    );
                                }
                            }
                            else
                            {
                                if (tk->keycode) {
                                    unregister_code(
                                        tk->keycode
                                    );
                                }
                            }
                        }

                        matrix_temp[
                            keyLocation.rowA
                        ] &=
                            ~((matrix_row_t)1U <<
                              keyLocation.colA);

                        continue;
                    }
                }
            }
#endif // toggleKey

            /*
             * Original normal analog key path.
             */
            {
                const uint8_t row =
                    (uint8_t)
                        keyLocation.rowA;

                const uint8_t col =
                    (uint8_t)
                        keyLocation.colA;

                const matrix_row_t key_mask =
                    ((matrix_row_t)1U << col);

                const bool was_pressed =
                    (matrix_temp[row] &
                     key_mask) != 0U;

                int32_t trigger_level;
                int32_t release_level;

                if (globleTrigger)
                {
                    trigger_level =
                        (int32_t)
                            keyMatrixTriggerLevel +
                        (int32_t)
                            draftValue;

                    release_level =
                        (int32_t)
                            keyMatrixReleaseLevel;
                }
                else
                {
                    trigger_level =
                        (int32_t)
                            eeconfig_read_Trigger(
                                row,
                                col
                            ) +
                        (int32_t)
                            draftValue;

                    release_level =
                        (int32_t)
                            eeconfig_read_Release(
                                row,
                                col
                            );
                }

                if (!was_pressed)
                {
                    if (adcPre >
                        trigger_level)
                    {
                        matrix_temp[row] |=
                            key_mask;
                    }
                }
                else if (
                    adcPre <= release_level)
                {
                    matrix_temp[row] &=
                        ~key_mask;
                }
#ifndef NO_PRINT
                {
                    const bool now_pressed =
                        (matrix_temp[row] &
                         key_mask) != 0U;

                    if (was_pressed !=
                        now_pressed)
                    {
                        uint8_t active_layer =
                            biton32(
                                layer_state
                            );

                        uint16_t keycode =
                            KC_NO;

                        if (active_layer <
                            KEYMAPS_COUNT)
                        {
                            keycode =
                                keymaps[
                                    active_layer
                                ][
                                    row
                                ][
                                    col
                                ];
                        }

                        uprintf(
                            "[ANALOG KEY] %s "
                            "KC=%04X L=%u R=%u C=%u "
                            "ADC_RAW=%ld TRAVEL=%ld "
                            "TRIG=%ld REL=%ld "
                            "GLOBAL=%u DRAFT=%u\r\n",
                            now_pressed ?
                                "PRESS" :
                                "RELEASE",
                            (unsigned int)
                                keycode,
                            (unsigned int)
                                active_layer,
                            (unsigned int)row,
                            (unsigned int)col,
                            (long)adcCalc,
                            (long)adcPre,
                            (long)trigger_level,
                            (long)release_level,
                            (unsigned int)
                                globleTrigger,
                            (unsigned int)
                                draftValue
                        );
                    }
                }
#endif
#ifdef nonOP
                if (keyScanEffect)
                {
                    float effect_mult;

                    if (adcCalc > 5)
                    {
                        adcCalc -= 5;

                        EffectranbowColor =
                            rainbowColor[group];

                        if (adcCalc > 100) {
                            effect_mult = 1;
                        } else {
                            effect_mult =
                                (float)adcCalc /
                                100;
                        }

                        EffectranbowColor.red *=
                            effect_mult;

                        EffectranbowColor.green *=
                            effect_mult;

                        EffectranbowColor.blue *=
                            effect_mult;
                    }
                    else
                    {
                        EffectranbowColor =
                            LED_NULL;
                    }

                    LED_Datas[
                        ledKeyMatrix[
                            keyLocation.rowA
                        ][
                            keyLocation.colA
                        ]
                    ] = EffectranbowColor;
                }
#else

#endif
            }
        }
    }

#ifdef rappySnappy
    rappy_snappy_process_all();
#endif // rappySnappy

#ifdef rappyTrigger
    rappy_q.enabled = false;

    if (rappy_q.enabled == true) {
        process_rappy_fixed_ratio(
            &rappy_q
        );
    }
#endif // rappyTrigger

#ifdef modTap
    mt_tick(0);
    mt_apply_pending();
#endif // modTap

    /*
     * Always update the normal QMK matrix.
     *
     * DKS physical keys have already been removed from
     * matrix_temp inside the scan loop, therefore updating
     * matrix[] here does not send the original Q key.
     *
     * Normal keys remain unaffected.
     */
    {
        uint32_t current_layer_state =
            layer_state;

        bool mo1_pressed =
            ((last_layer_state &
              (1UL << 1)) == 0U) &&
            ((current_layer_state &
              (1UL << 1)) != 0U);

        if (mo1_pressed)
        {
            clear_mods();
            layer_clear();
            layer_on(1);
        }

        last_layer_state =
            current_layer_state;

        memcpy(
            matrix,
            matrix_temp,
            sizeof(matrix)
        );

        keyboard_taskTMR();
    }

    if (gameAnalogStickStatus.raw != 0) {
        chBSemSignal(
            &gamepadAnalogSem
        );
    }

    chSysUnlock();

    eeclr_task();

    return true;
}

bool matrixAnalogFind(void)
{
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

        return true;
}

static THD_WORKING_AREA(waADC_ProcessThread, 512);
static THD_FUNCTION(ADC_ProcessThread, arg)
{
		(void)arg;
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
    palSetGroupMode(GPIOA, PAL_PORT_BIT(3) | PAL_PORT_BIT(4) , 0, PAL_MODE_OUTPUT_PUSHPULL);
    palSetGroupMode(GPIOF, PAL_PORT_BIT(4) | PAL_PORT_BIT(5), 0, PAL_MODE_OUTPUT_PUSHPULL);
    analogPowerSwitch(0);



    //Mux Enable IO
    palSetGroupMode(GPIOC, PAL_PORT_BIT(4) | PAL_PORT_BIT(5), 0, PAL_MODE_OUTPUT_PUSHPULL);
    palWriteGroup(GPIOC, PAL_PORT_BIT(4) | PAL_PORT_BIT(5), 0, 0x0000);

    palSetGroupMode(GPIOB, PAL_PORT_BIT(0) | PAL_PORT_BIT(1) | PAL_PORT_BIT(2) | PAL_PORT_BIT(10) | PAL_PORT_BIT(12), 0, PAL_MODE_OUTPUT_PUSHPULL);
    palWriteGroup(GPIOB, PAL_PORT_BIT(0) | PAL_PORT_BIT(1) | PAL_PORT_BIT(2) | PAL_PORT_BIT(10) | PAL_PORT_BIT(12), 0, 0x0000);

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
		/*
     * socd_thread_init() already includes EEPROM loading internally;
     * do not call socd_load_all_from_eeprom() again.
     */
    socd_thread_init();
#endif



//	setRunMode(currentLED_RunMode);
	chThdSleepMilliseconds(500);
	
	bool offset_calibration_valid = false;

	uprintf("[ADC CAL] Load calibration data\r\n");

	for (uint16_t i = 0U;
			 i < (ADC_NUM_CHANNELS * ADC_BUF_DEPTH);
			 i++)
	{
			uint8_t channel =
					i % ADC_BUF_DEPTH;

			uint8_t group =
					i / ADC_BUF_DEPTH;

			initPostionLevel[channel][group] =
					eeprom_read_word(
							EECONFIG_KEYMAP_OFFSET_CALIBRATION +
							i * 2U
					);

			travelPostionLevel[channel][group] =
					eeprom_read_word(
							EECONFIG_KEYMAP_TRAVEL_CALIBRATION +
							i * 2U
					);

			if (initPostionLevel[channel][group] != 0U &&
					initPostionLevel[channel][group] != 0xFFFFU)
			{
					offset_calibration_valid = true;
			}
	}

	if (!offset_calibration_valid)
	{
			uprintf(
					"[ADC CAL] Invalid offset data, start calibration\r\n"
			);

			uprintf(
					"[ADC CAL] Keep all keys released\r\n"
			);

			adcOffsetCalc(true, true, 1500);

			uprintf(
					"[ADC CAL] Calibration completed "
					"raw[0][0]=%u init[0][0]=%u\r\n",
					(unsigned int)samples2[0][0],
					(unsigned int)initPostionLevel[0][0]
			);
	}
	else
	{
			uprintf(
					"[ADC CAL] Calibration loaded "
					"raw[0][0]=%u init[0][0]=%u\r\n",
					(unsigned int)samples2[0][0],
					(unsigned int)initPostionLevel[0][0]
			);
	}

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

    eeconfig_write_rapidTrigger(0, rapidTriggerKey[0].key);

    rapidTriggerProcess(true, 0);
		

	#endif
	uprintf("ADC Task compli...\r\n");
    while (1)
    {
				chBSemWait(&adcCompleteSem); 
				matrixAnalogScan();
    }
}

bool rapidTriggerKeyBuilder(uint8_t key)
{
    analogKey_Config_t keyLocation;
    uint16_t initPosValue;
    uint16_t travelPosValue;

    if (key >= RAPID_TRIGGER_COUNT) {
        return false;
    }

    rapidTriggerKey[key].key = eeconfig_read_rapidTrigger(key);

    /* EEPROM entry is not enabled. */
    if (rapidTriggerKey[key].key.enable != 1) {
        return false;
    }

    if (rapidTriggerKey[key].key.counts == 0 ||
        rapidTriggerKey[key].key.counts >= RAPID_TRIGGER_LEVELS) {
        return false;
    }

    if (rapidTriggerKey[key].key.row >= MATRIX_ROWS ||
        rapidTriggerKey[key].key.col >= MATRIX_COLS) {
        return false;
    }

    keyLocation =
        matrixAnalogRelocate
            [rapidTriggerKey[key].key.row]
            [rapidTriggerKey[key].key.col];

    /* Validate matrixAnalogRelocate result. */
    if (keyLocation.colA >= ADC_BUF_DEPTH ||
        keyLocation.rowA >= ADC_NUM_CHANNELS) {
        return false;
    }

    initPosValue =
        initPostionLevel[keyLocation.colA][keyLocation.rowA];

    travelPosValue =
        travelPostionLevel[keyLocation.colA][keyLocation.rowA];

    /* Prevent unsigned underflow and invalid travel range. */
    if (travelPosValue <= initPosValue ||
        (travelPosValue - initPosValue) <= keyMatrixTriggerLevel) {
        return false;
    }

    for (uint8_t i = 0;
         i < rapidTriggerKey[key].key.counts;
         i++) {

        uint8_t index =
            rapidTriggerKey[key].key.counts - i;

        if (index >= RAPID_TRIGGER_LEVELS) {
            return false;
        }

        rapidTriggerKey[key].onLevel[index] =
            keyMatrixTriggerLevel +
            (1.0f -
             sqrtf(
                 (float)i /
                 (float)rapidTriggerKey[key].key.counts
             )) *
            (
                travelPosValue -
                initPosValue -
                keyMatrixTriggerLevel
            );

        rapidTriggerKey[key].onLevel[index] =
            (uint16_t)(
                rapidTriggerKey[key].onLevel[index] *
                1.015f
            );
    }

    rapidTriggerKey[key].onLevel[0] =
        keyMatrixTriggerLevel;

    for (uint8_t i = 0;
         i + 1 < rapidTriggerKey[key].key.counts;
         i++) {

        rapidTriggerKey[key].offLevel[i] =
            rapidTriggerKey[key].onLevel[i + 1] -
            (
                (
                    rapidTriggerKey[key].onLevel[i + 1] -
                    rapidTriggerKey[key].onLevel[i]
                ) / 8
            );
    }

    rapidTriggerKey[key].lastStep = -1;
    rapidTriggerKey[key].triggeredLevel = 0;

    return true;
}

void rapidTriggerProcess(bool mode, uint8_t key)
{
    if (mode) {
        for (uint8_t i = 0; i < RAPID_TRIGGER_COUNT; i++) {
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

/*
static bool eeprom_write_self_test(void)
{
    const uint8_t test_value = 0x5A;
    const uint16_t test_addr = 0xFFF0;

    uprintf("[EEPROM-TEST] begin\r\n");

    palSetPadMode(GPIOB, 7, PAL_MODE_OUTPUT_PUSHPULL);
    palClearPad(GPIOB, 7);
    chThdSleepMilliseconds(10);

    uprintf(
        "[EEPROM-TEST] WP=%d\r\n",
        palReadPad(GPIOB, 7)
    );

    uprintf(
        "[EEPROM-TEST] before=0x%02X\r\n",
        eeprom_read_byte((uint8_t *)(uintptr_t)test_addr)
    );

    uprintf("[EEPROM-TEST] write begin\r\n");

    eeprom_update_byte(
        (uint8_t *)(uintptr_t)test_addr,
        test_value
    );

    uprintf("[EEPROM-TEST] write returned\r\n");

    chThdSleepMilliseconds(6);

    uint8_t readback =
        eeprom_read_byte((uint8_t *)(uintptr_t)test_addr);

    uprintf(
        "[EEPROM-TEST] readback=0x%02X expected=0x%02X\r\n",
        readback,
        test_value
    );

    return readback == test_value;
}
*/
void matrix_init(void)//
{
	if (globleTrigger){

	} else {

	}
	keycodeOutput = true;	//===> startup stop keycode output

	if (!readPin(A15)){ //lyon dont know what is this ask tony later
		keycodeOutput = false;
	}

if (!eepromInitCheck) {
    write_keymap_to_eeprom();
    eeconfig_rapidTrigger_init();
    dynamic_macro_reset_eeprom();
    keyMatrixTriggerLevel = DEFAULT_TRIGGER_LEVEL;
    keyMatrixReleaseLevel = DEFAULT_RELEASE_LEVEL;

    eeprom_write_word(
        EECONFIG_KEYMAP_TRIGGER_LEVEL,
        keyMatrixTriggerLevel
    );

    eeprom_write_word(
        EECONFIG_KEYMAP_RELEASE_LEVEL,
        keyMatrixReleaseLevel
    );

    write_trigger_to_eeprom();
    write_Release_to_eeprom();
		if (!eepromInitCheck)
		{
				uprintf("Factory Reset start\r\n");

				write_keymap_to_eeprom();
				eeconfig_rapidTrigger_init();
				dynamic_macro_reset_eeprom();

				eeprom_write_word(
						EECONFIG_KEYMAP_TRIGGER_LEVEL,
						DEFAULT_TRIGGER_LEVEL
				);

				eeprom_write_word(
						EECONFIG_KEYMAP_RELEASE_LEVEL,
						DEFAULT_RELEASE_LEVEL
				);

				write_trigger_to_eeprom();
				write_Release_to_eeprom();
				eeconfig_rapidTrigger_init();
				eeconfig_socd_clear();

				/*
				 * ADC calibration must not run here because the ADC thread
				 * has not been started yet.
				 */

				uprintf(
						"Factory Reset configuration completed\r\n"
				);
		}

		chThdCreateStatic(
				waADC_ProcessThread,
				sizeof(waADC_ProcessThread),
				NORMALPRIO - 4,
				ADC_ProcessThread,
				NULL
		);

    eeconfig_socd_clear();

    eeprom_update_byte(
        (uint8_t *)KB_COLOR_MODE_ADDR,
        8
    );

    eeprom_update_byte(
        (uint8_t *)EECONFIG_POLLING_RATE,
        0
    );

    eeprom_update_byte(
        (uint8_t *)EECONFIG_MAC_MODE,
        0
    );

    eeprom_update_byte(
        KB_ENCODER_MODE_ADDR,
        0
    );

    chThdSleepMilliseconds(100);
    NVIC_SystemReset();

    while (true) {
        chThdSleepMilliseconds(1000);
    }
}
		chThdCreateStatic(waADC_ProcessThread, sizeof(waADC_ProcessThread), NORMALPRIO - 4, ADC_ProcessThread, NULL);
		LED_init();
		factory_reset_init();
		reset_all_key_init();
		factory_system_reset_init();
		knobState = eeprom_read_byte(KB_ENCODER_MODE_ADDR);
}



uint8_t matrix_scan(void)
{
	//chThdSleepMilliseconds(1);
//	chBSemWait(&matrixScanSem);
	keyboard_taskTMR();
	macro_engine_task();
	idle_led_maybe_report_timeout();
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

// uint16_t matrix_get_key_raw_adc(uint8_t row, uint8_t col)
// {
//     if (row >= MATRIX_ROWS || col >= MATRIX_COLS) {
//         return 0;
//     }

//     return keyRawDepth[row][col];
// }

// uint16_t matrix_get_key_depth(uint8_t row, uint8_t col)
// {
//     if (row >= MATRIX_ROWS || col >= MATRIX_COLS) {
//         return 0;
//     }

//     return keyDepth[row][col];
// }

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

bool process_gamepad(keyrecord_t record, uint16_t keycode)
{

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
				return false;
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

void dump_eeprom(void)
{
    uint8_t buffer[32];

    uint8_t *start_addr =
        EEPROM_MACRO_SLOT_ADDR(0);

    uint8_t *end_addr =
        EEPROM_MACRO_REGION_END;

    for (uint8_t *addr = start_addr;
         addr < end_addr;
         addr += sizeof(buffer)) {

        size_t read_size =
            (size_t)(end_addr - addr);

        if (read_size > sizeof(buffer)) {
            read_size = sizeof(buffer);
        }

        memset(buffer, 0, sizeof(buffer));

        eeprom_read_block(
            buffer,
            (const void *)addr,
            read_size
        );

        uprintf(
            "EEPROM %08lX: ",
            (unsigned long)(uintptr_t)addr
        );

        for (size_t i = 0;
             i < read_size;
             i++) {
            uprintf("%02X ", buffer[i]);
        }

        uprintf("\r\n");
    }
}
/*
static void eeprom_write8_verify(uint8_t *addr, uint8_t value)
{
    if (addr == NULL) {
        return;
    }

    uint8_t before =
        eeprom_read_byte((const uint8_t *)addr);

    eeprom_update_byte(addr, value);

    uint8_t after =
        eeprom_read_byte((const uint8_t *)addr);

    uprintf(
        "EEPROM addr=%08lX before=%02X write=%02X after=%02X %s\r\n",
        (unsigned long)(uintptr_t)addr,
        before,
        value,
        after,
        after == value ? "OK" : "FAIL"
    );
}
*/

static void save_mosart_color_to_eeprom(void)
{
    /*
     * Keep the global startup color synchronized
     * with the adjusted MosArt color.
     */
    c = mosArt.LED;
    eeprom_update_byte(KB_COLOR_RED_ADDR,mosArt.LED.red);
    eeprom_update_byte(KB_COLOR_GREEN_ADDR,mosArt.LED.green);
    eeprom_update_byte(KB_COLOR_BLUE_ADDR,mosArt.LED.blue);
    uprintf("[RGB SAVE] R=%u G=%u B=%u\r\n",(unsigned int)mosArt.LED.red,(unsigned int)mosArt.LED.green,(unsigned int)mosArt.LED.blue);
}

static void plusHue(led_config_t in)
{
    current_hsv = rgb_to_hsv(in);
    current_hsv.h = (current_hsv.h + 15U) % 360U;
    mosArt.LED = hsv_to_rgb(current_hsv);
}

static void minusHue(led_config_t in)
{
    current_hsv = rgb_to_hsv(in);
    current_hsv.h = (current_hsv.h + 345U) % 360U;
    mosArt.LED = hsv_to_rgb(current_hsv);
}

static void plusSat(led_config_t in)
{
    current_hsv = rgb_to_hsv(in);
    current_hsv.s = (current_hsv.s > 239U) ? 255U : (uint8_t)(current_hsv.s + 16U);
    mosArt.LED = hsv_to_rgb(current_hsv);
}

static void minusSat(led_config_t in)
{
    current_hsv = rgb_to_hsv(in);
    current_hsv.s = (current_hsv.s < 16U) ? 0U : (uint8_t)(current_hsv.s - 16U);
    mosArt.LED = hsv_to_rgb(current_hsv);
}

static void plusBri(led_config_t in)
{
    current_hsv = rgb_to_hsv(in);
    current_hsv.v = (current_hsv.v > 239U) ? 255U : (uint8_t)(current_hsv.v + 16U);
    mosArt.LED = hsv_to_rgb(current_hsv);
}

static void minusBri(led_config_t in)
{
    current_hsv = rgb_to_hsv(in);
    current_hsv.v = (current_hsv.v < 16U) ? 0U : (uint8_t)(current_hsv.v - 16U);
    mosArt.LED = hsv_to_rgb(current_hsv);
}

bool hook_matrix_change(keyrecord_t record)
{
	uint16_t keycode = keymap_key_to_keycode(biton32(layer_state),record.event.key);
	// uprintf("KEY row=%d col=%d kc=%04X pressed=%d\r\n",
	// 				record.event.key.row, record.event.key.col, keycode, record.event.pressed);
//	uprintf ("Layer:%d keycode:%04x\r\n", biton32(layer_state), keycode);

//    action_t action = layer_switch_get_action(record.event.key);
//    uint8_t code = action.key.code;
//    uprintf ("action keycode:%X modes:%X kid_id:%X\r\n",code,action.key.mods,action.kind.id);
	
		switch (keycode)
    {
        case KC_PHUE:
            if (record.event.pressed)
            {
                plusHue(mosArt.LED);
                save_mosart_color_to_eeprom();

                uprintf(
                    "[RGB KEY] PHUE press\r\n"
                );
            }
            else
            {
                uprintf(
                    "[RGB KEY] PHUE release\r\n"
                );
            }

            return false;

        case KC_MHUE:
            if (record.event.pressed)
            {
                minusHue(mosArt.LED);
                save_mosart_color_to_eeprom();

                uprintf(
                    "[RGB KEY] MHUE press\r\n"
                );
            }
            else
            {
                uprintf(
                    "[RGB KEY] MHUE release\r\n"
                );
            }

            return false;

        case KC_PSAT:
            if (record.event.pressed)
            {
                plusSat(mosArt.LED);
                save_mosart_color_to_eeprom();

                uprintf(
                    "[RGB KEY] PSAT press\r\n"
                );
            }
            else
            {
                uprintf(
                    "[RGB KEY] PSAT release\r\n"
                );
            }

            return false;

        case KC_MSAT:
            if (record.event.pressed)
            {
                minusSat(mosArt.LED);
                save_mosart_color_to_eeprom();

                uprintf(
                    "[RGB KEY] MSAT press\r\n"
                );
            }
            else
            {
                uprintf(
                    "[RGB KEY] MSAT release\r\n"
                );
            }

            return false;

        default:
            break;
    }

		/*
		 * Mac mode:
		 *
		 * Physical Left Win -> Left Alt / Option
		 * Physical Left Alt -> Left GUI / Command
		 *
		 * Windows mode:
		 * Keep the original key mapping.
		 */
		if (isMac)
		{
				switch (keycode)
				{
						case KC_LGUI:
								if (record.event.pressed)
								{
										register_code(KC_LALT);

										uprintf(
												"[MAC MOD] Physical LGUI -> LALT press\r\n"
										);
								}
								else
								{
										unregister_code(KC_LALT);

										uprintf(
												"[MAC MOD] Physical LGUI -> LALT release\r\n"
										);
								}

								return false;

						case KC_LALT:
								if (record.event.pressed)
								{
										register_code(KC_LGUI);

										uprintf(
												"[MAC MOD] Physical LALT -> LGUI press\r\n"
										);
								}
								else
								{
										unregister_code(KC_LGUI);

										uprintf(
												"[MAC MOD] Physical LALT -> LGUI release\r\n"
										);
								}

								return false;

						default:
								break;
				}
		}

    if (record.event.pressed && (currentLED_RunMode.mode == 5 )) { //|| currentLED_RunMode.mode == 7
        reactive_level[record.event.key.row][record.event.key.col] = REACTIVE_MAX;
    }

    if (record.event.pressed) {
        // uprintf("[IDLE] pressed row=%u col=%u kc=%04X\r\n",
        //         record.event.key.row,
        //         record.event.key.col,
        //         keycode);
        idle_led_note_activity();
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
			return false;   // SOCD ??
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
					uint8_t mods = get_mods();  // ????????

					if(altTabLock){
						if (mods & (MOD_BIT(KC_LALT) | MOD_BIT(KC_RALT))) {
							uprintf("skip alt+tab\r\n");
							return false;
						}
					}
					
				}
				break;
				case KC_F4:{
					uint8_t mods = get_mods();  // ????????

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
				// 	return false; // ????
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
					// hsv.h += 2;   // ??????
					// mosArt.LED = hsv_to_rgb(hsv);
				break;
				case KC_LM1:
					if(isCtrl){
						currentLED_RunMode.type = normalMode;
						currentLED_RunMode.mode = 0x00;
						setRunMode(currentLED_RunMode);
						eeprom_update_byte((uint8_t *)KB_COLOR_MODE_ADDR, 0);
					}
				break;
				case KC_LM2:
					if(isCtrl){
						currentLED_RunMode.type = normalMode;
						currentLED_RunMode.mode = 0x01;
						setRunMode(currentLED_RunMode);
						eeprom_update_byte((uint8_t *)KB_COLOR_MODE_ADDR, 1);
					}
				break;
				case KC_LM3:
					if(isCtrl){
						currentLED_RunMode.type = normalMode;
						currentLED_RunMode.mode = 0x02;
						setRunMode(currentLED_RunMode);
						eeprom_update_byte((uint8_t *)KB_COLOR_MODE_ADDR, 2);
					}
				break;
				case KC_LM4:
					if(isCtrl){
						currentLED_RunMode.type = normalMode;
						currentLED_RunMode.mode = 0x03;
						setRunMode(currentLED_RunMode);
						eeprom_update_byte((uint8_t *)KB_COLOR_MODE_ADDR, 3);
					}
				break;
				case KC_LM5:
					if(isCtrl){
						currentLED_RunMode.type = normalMode;
						currentLED_RunMode.mode = 0x04;
						setRunMode(currentLED_RunMode);
						eeprom_update_byte((uint8_t *)KB_COLOR_MODE_ADDR, 4);
					}
				break;
				case KC_LM6:
					if(isCtrl){
						currentLED_RunMode.type = normalMode;
						currentLED_RunMode.mode = 0x05;
						setRunMode(currentLED_RunMode);
						eeprom_update_byte((uint8_t *)KB_COLOR_MODE_ADDR, 5);
					}
				break;
				case KC_LM7:
					if(isCtrl){
						currentLED_RunMode.type = normalMode;
						currentLED_RunMode.mode = 0x06;
						setRunMode(currentLED_RunMode);
						eeprom_update_byte((uint8_t *)KB_COLOR_MODE_ADDR, 6);
					}
				break;
				case KC_LM8:
					if(isCtrl){
						currentLED_RunMode.type = normalMode;
						currentLED_RunMode.mode = 0x07;
						setRunMode(currentLED_RunMode);
						eeprom_update_byte((uint8_t *)KB_COLOR_MODE_ADDR, 7);
					}
				break;
				case KC_LM9:
					if(isCtrl){
						currentLED_RunMode.type = normalMode;
						currentLED_RunMode.mode = 0x08;
						setRunMode(currentLED_RunMode);
						eeprom_update_byte((uint8_t *)KB_COLOR_MODE_ADDR, 8);
					}
				break;
				case KC_M1:
					// macro_engine_setup_test_copy_paste();
					// macro_engine_start(0);
					// dump_eeprom();
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
					// macro_engine_write_full_slot_api_style(0);
					// macro_engine_write_full_slot_api_style(1);
					// macro_engine_write_full_slot_api_style(2);
					// macro_engine_write_full_slot_api_style(3);
					// macro_engine_write_full_slot_api_style(4);
					// macro_engine_write_full_slot_api_style(5);
					// macro_engine_write_full_slot_api_style(6);
					// macro_engine_write_full_slot_api_style(7);
					// macro_engine_write_full_slot_api_style(8);
					// macro_engine_write_full_slot_api_style(9);
					// macro_engine_dump_eeprom_slot(0);
					// macro_engine_dump_eeprom_slot(1);
					// macro_engine_dump_eeprom_slot(8);
					// macro_engine_dump_eeprom_slot(9);

					uprintf("KC_M2 API-SIM FULL WRITE \r\n");
					// dump_eeprom();
					return false;
					// uprintf("matrix (%d)_(%d)_(%d),(%d)_(%d)_(%d)\r\n",
					// 	g_rappy_pairs[0].lay_a, g_rappy_pairs[0].row_a, g_rappy_pairs[0].col_a,
					// 	g_rappy_pairs[0].lay_b, g_rappy_pairs[0].row_b, g_rappy_pairs[0].col_b);
				break;
				case KC_M3:
					// eeconfig_rappy_snappy_clear();
					// toggle_key_dump_eeprom();
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
					socd_setting_write(isSOCD);
					// if (record.event.pressed) {
					// 	if(isSOCD){//IS_SOCD_ON_ADD == 0x01){
					// 		uprintf("socd close\r\n");
					// 		isSOCD = false;
					// 	} else {
					// 		uprintf("socd on\r\n");
					// 		isSOCD = true;
					// 	}
					// }
					
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
					return false; // ?????????
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
						register_code16(KC_RGB_VAD);
					} else {
						unregister_code16(KC_RGB_VAD);
					}
					return false;
				}
			break;
			case KC_MEDIA_PREV_TRACK:  //F6
				if (isMac) {
					if (record.event.pressed) {
						register_code16(KC_RGB_VAI);
					} else {
						unregister_code16(KC_RGB_VAI);
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
					/*
					 * Release any active modifiers before changing mode.
					 */
					clear_keyboard();

					isMac = 1;

					eeprom_update_byte(
							(uint8_t *)EECONFIG_MAC_MODE,
							isMac
					);

					uprintf(
							"[OS MODE] MAC enabled, "
							"LGUI/LALT swapped\r\n"
					);

					return false;

			case KC_WINDOW:
					/*
					 * Release any remapped modifier before restoring
					 * the normal Windows layout.
					 */
					clear_keyboard();

					isMac = 0;

					eeprom_update_byte(
							(uint8_t *)EECONFIG_MAC_MODE,
							isMac
					);

					uprintf(
							"[OS MODE] WINDOWS enabled, "
							"LGUI/LALT normal\r\n"
					);

					return false;

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
					    currentLED_RunMode.brightness = 80;
					    currentLED_RunMode.speed = 100;
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
							if (currentLED_RunMode.brightness < 100) {
								++currentLED_RunMode.brightness;
							}
							uprintf ("RGB Bright ++:%d\r\n",currentLED_RunMode.brightness);
							break;
						case KC_RGB_VAD:
							if (currentLED_RunMode.brightness > 0) {
								--currentLED_RunMode.brightness;
							}
							uprintf ("RGB Bright --:%d\r\n",currentLED_RunMode.brightness);
							break;
						case KC_RGB_SPI:
						case KC_RAPID_LVI:
							if (currentLED_RunMode.speed > 0) {
								--currentLED_RunMode.speed;
							}
							uprintf ("RGB Speed --:%d\r\n",currentLED_RunMode.speed);
							break;
						case KC_RGB_SPD:
						case KC_RAPID_LVD:
							if (currentLED_RunMode.speed < 100) {
								++currentLED_RunMode.speed;
							}
							uprintf ("RGB Speed ++:%d\r\n",currentLED_RunMode.speed);
							break;
					}
						if (currentLED_RunMode.brightness > 100){
							currentLED_RunMode.brightness = 100;
						}
						if (currentLED_RunMode.speed > 100){
							currentLED_RunMode.speed = 100;
						}
						/*
						 * Keep keyboard brightness behavior consistent
						 * with API command 0x020C.
						 */
						if (currentLED_RunMode.brightness == 0U)
						{
								uprintf(
										"[RGB KEY] brightness=0 -> offMode\r\n"
								);

								currentLED_RunMode.type = offMode;
						}
						else if (currentLED_RunMode.type == offMode)
						{
								uprintf(
										"[RGB KEY] brightness=%u -> normalMode\r\n",
										(unsigned int)
												currentLED_RunMode.brightness
								);

								currentLED_RunMode.type = normalMode;
						}

						LED_SpeedChange(
								currentLED_RunMode.brightness,
								currentLED_RunMode.speed
						);

						setRunMode(currentLED_RunMode);
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
        currentLED_RunMode.type = powerOff;//offMode;   // ??? powerOff; // lyon try to fix reboot problem
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
		// ??????? ? ?????? release_ratio ???
		if (ratio <= rappy->release_ratio) {
			rappy->pressed = false;
			unregister_code(rappy->keycode);
			uprintf("<<< release %.2f\r\n", ratio);
		}
	}


    return true;
}

#endif // rappyTrigger
