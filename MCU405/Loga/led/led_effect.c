
/*
    Copyright (C) 2023 iOne Electrionc
*/
# include "encoder.h"
# include "hal.h"
# include "main.h"
# include "led_function.h"
# include "led_effect.h"
# include "driver/led_driver.h"
#include "usb_main.h" // lyon fix reboot dynamic lighting 
#include "matrix.h" // all the define lyon
// wheel
#include <math.h>
#include <stdint.h>
#include <stdbool.h>

uint8_t wheel_base_hue[LED_MATRIX_ROWS][LED_MATRIX_COLS];
static bool wheel_inited = false;

// reactive
uint8_t reactive_level[MATRIX_ROWS][MATRIX_COLS] = {0};
//ripple thread
extern binary_semaphore_t ledUpdateSem;
//starlight
extern led_config_t LED_Datas[LedNumbers];

// fix led location
typedef struct {
    int16_t x;
    int16_t y;
    bool valid;
} led_pos_t;

static led_pos_t led_pos[LedNumbers];
static bool led_pos_inited = false;
static int16_t led_center_x = 0;
static int16_t led_center_y = 0;

static void calculate_mosart_breath_volume(uint8_t speed,uint16_t timebase,uint8_t *bright);

static void led_pos_init_from_remap(void)
{
    const int16_t pitch_x = 16;
    const int16_t pitch_y = 16;

    int16_t min_x = 32767, min_y = 32767;
    int16_t max_x = -32768, max_y = -32768;

    uint16_t i;
    uint8_t row, col;

    for (i = 0; i < LedNumbers; i++) {
        led_pos[i].x = 0;
        led_pos[i].y = 0;
        led_pos[i].valid = false;
    }

    for (row = 0; row < LED_MATRIX_ROWS; row++) {
        for (col = 0; col < LED_MATRIX_COLS; col++) {
            int8_t idx = ledRemap[row][col];
            if (idx >= 0 && idx < LedNumbers) {
                led_pos[idx].x = col * pitch_x;
                led_pos[idx].y = row * pitch_y;
                led_pos[idx].valid = true;

                if (led_pos[idx].x < min_x) min_x = led_pos[idx].x;
                if (led_pos[idx].x > max_x) max_x = led_pos[idx].x;
                if (led_pos[idx].y < min_y) min_y = led_pos[idx].y;
                if (led_pos[idx].y > max_y) max_y = led_pos[idx].y;
            }
        }
    }

    led_center_x = (min_x + max_x) / 2;
    led_center_y = (min_y + max_y) / 2;
    led_pos_inited = true;
}

typedef struct {
    bool enable;
    uint8_t offset;
} effect_wheel_t;

static effect_wheel_t wheel_fx;
// end
typedef struct {
    uint8_t active;
    uint8_t phase;      // 0~255
    uint8_t speed;      // 瘥活憓?憭?
    led_config_t color;
} starlight_led_t;

typedef struct {
    bool enable;
    uint8_t density;    // 瘥??啣????賊?
    uint8_t base_dim;   // ?敺桐漁嚗閮?0
} EffectStarlight_t;

static EffectStarlight_t starlight;
static starlight_led_t starlight_leds[LedNumbers];
static uint32_t starlight_seed = 0x12345678;

static uint32_t starlight_rand(void)
{
    starlight_seed = starlight_seed * 1664525UL + 1013904223UL;
    return starlight_seed;
}

static uint8_t starlight_rand8(void)
{
    return (uint8_t)(starlight_rand() >> 24);
}

static led_config_t starlight_pick_color(void)
{
    led_config_t c;
    uint8_t r = starlight_rand8() % 6;

    switch (r) {
        default:
        case 0: // ????
            c.red = 255; c.green = 210; c.blue = 120;
            break;
        case 1: // ???
            c.red = 255; c.green = 235; c.blue = 190;
            break;
        case 2: // 蝝??
            c.red = 255; c.green = 255; c.blue = 255;
            break;
        case 3: // ?瑞??
            c.red = 200; c.green = 230; c.blue = 255;
            break;
        case 4: // 瘛∟???
            c.red = 150; c.green = 200; c.blue = 255;
            //c.red = 120; c.green = 180; c.blue = 255;
            break;
        // case 0: // ?
        //     c.red = 255; c.green = 220; c.blue = 180;
        //     break;
        // case 1: // 銝剜�抒
        //     c.red = 255; c.green = 245; c.blue = 230;
        //     break;
        // case 2: // ?瑞
        //     c.red = 220; c.green = 235; c.blue = 255;
        //     break;
        // case 3: // 瘛∟???
        //     c.red = 190; c.green = 215; c.blue = 255;
        //     break;
        // case 4: // 敺格???
        //     c.red = 255; c.green = 235; c.blue = 210;
        //     break;
        // case 5: // ???
        //     c.red = 210; c.green = 225; c.blue = 255;
        //     break;
    }

    return c;
}

static uint8_t starlight_triangle_wave(uint8_t phase)
{
    if (phase < 128) {
        return phase << 1;
    } else {
        return (255 - phase) << 1;
    }
}

static led_config_t starlight_scale_color(led_config_t in, uint8_t bright)
{
    led_config_t out;
    out.red   = ((uint16_t)in.red   * bright) / 255;
    out.green = ((uint16_t)in.green * bright) / 255;
    out.blue  = ((uint16_t)in.blue  * bright) / 255;
    return out;
}

static void starlight_reset_all(void)
{
    for (uint16_t i = 0; i < LedNumbers; i++) {
        starlight_leds[i].active = 0;
        starlight_leds[i].phase = 0;
        starlight_leds[i].speed = 0;
        starlight_leds[i].color.red = 0;
        starlight_leds[i].color.green = 0;
        starlight_leds[i].color.blue = 0;
    }
}
// starlight_end
// ripple
#define RIPPLE_MAX_EVENTS 8
#define RIPPLE_RADIUS_STEP 12
#define RIPPLE_WIDTH 18
#define RIPPLE_FADE_STEP 10

// typedef struct { //centre
//     bool active;
//     uint8_t row;
//     uint8_t col;
//     uint16_t radius;
//     uint8_t life;
//     led_config_t color;
// } ripple_event_t;
typedef struct {
    bool active;
    uint8_t origin_led;
    uint16_t radius;
    uint8_t life;
    led_config_t color;
} ripple_event_t;

static ripple_event_t ripple_events[RIPPLE_MAX_EVENTS];

typedef struct {
    int16_t x;
    int16_t y;
} ripple_pos_t;

// static const uint8_t ripple_row_lengths[LED_MATRIX_ROWS] = {26, 16, 17, 17, 16, 15, 12};
// static ripple_pos_t ripple_pos[LED_MATRIX_ROWS][LED_MATRIX_COLS];
// static bool ripple_pos_inited = false;

// static void ripple_init_positions(void)
// {
//     const int16_t pitch_x = 16;
//     const int16_t pitch_y = 16;
//     uint8_t row, col;

//     for (row = 0; row < LED_MATRIX_ROWS; row++) {
//         int16_t row_offset = ((26 - ripple_row_lengths[row]) * pitch_x) / 2;

//         for (col = 0; col < LED_MATRIX_COLS; col++) {
//             ripple_pos[row][col].x = 0;
//             ripple_pos[row][col].y = 0;
//         }

//         for (col = 0; col < ripple_row_lengths[row]; col++) {
//             ripple_pos[row][col].x = row_offset + col * pitch_x;
//             ripple_pos[row][col].y = row * pitch_y;
//         }
//     }

//     ripple_pos_inited = true;
// }
// call when press
void ripple_add_event(uint8_t row, uint8_t col, led_config_t color)
{
    uint8_t i;
    int8_t origin_led;

    if (row >= MATRIX_ROWS || col >= MATRIX_COLS) {
        return;
    }

    origin_led = ledKeyMatrix[row][col];
    if (origin_led < 0 || origin_led >= LedNumbers) {
        return;
    }

    for (i = 0; i < RIPPLE_MAX_EVENTS; i++) {
        if (!ripple_events[i].active) {
            ripple_events[i].active = true;
            ripple_events[i].origin_led = (uint8_t)origin_led;
            ripple_events[i].radius = 0;
            ripple_events[i].life = 255;
            ripple_events[i].color = color;
            return;
        }
    }

    ripple_events[0].active = true;
    ripple_events[0].origin_led = (uint8_t)origin_led;
    ripple_events[0].radius = 0;
    ripple_events[0].life = 255;
    ripple_events[0].color = color;
}
// void ripple_add_event(uint8_t row, uint8_t col, led_config_t color)
// {
//     uint8_t i;

//     for (i = 0; i < RIPPLE_MAX_EVENTS; i++) {
//         if (!ripple_events[i].active) {
//             ripple_events[i].active = true;
//             ripple_events[i].row = row;
//             ripple_events[i].col = col;
//             ripple_events[i].radius = 0;
//             ripple_events[i].life = 255;
//             ripple_events[i].color = color;
//             return;
//         }
//     }

//     /* 皛蹂?撠梯????? 0 ??*/
//     ripple_events[0].active = true;
//     ripple_events[0].row = row;
//     ripple_events[0].col = col;
//     ripple_events[0].radius = 0;
//     ripple_events[0].life = 255;
//     ripple_events[0].color = color;
// }
// ripple process
extern led_config_t LED_Datas[LedNumbers];

/*
 * 刪除以下未使用函式：
 *
 * static uint8_t ripple_abs_u8(int16_t v)
 * {
 *     return (v < 0) ? (uint8_t)(-v) : (uint8_t)v;
 * }
 */

static uint16_t ripple_dist_approx(int16_t dx, int16_t dy)
{
    /* 銝 sqrt嚗?云??*/
    uint16_t ax = (dx < 0) ? -dx : dx;
    uint16_t ay = (dy < 0) ? -dy : dy;
    return ax + ay;   /* Manhattan distance嚗??其? */
}

static led_config_t ripple_scale_color(led_config_t in, uint8_t bright)
{
    led_config_t out;
    out.red   = ((uint16_t)in.red   * bright) / 255;
    out.green = ((uint16_t)in.green * bright) / 255;
    out.blue  = ((uint16_t)in.blue  * bright) / 255;
    return out;
}

static void ripple_clear_events(void)
{
    uint8_t i;
    for (i = 0; i < RIPPLE_MAX_EVENTS; i++) {
        ripple_events[i].active = false;
        ripple_events[i].radius = 0;
        ripple_events[i].life = 0;
    }
}

bool EffectTypeRippleProcess(void)
{
    uint16_t i;
    uint8_t e;

    if (!led_pos_inited) {
        led_pos_init_from_remap();
    }

    for (i = 0; i < LedNumbers; i++) {
        LED_Datas[i].red = 0;
        LED_Datas[i].green = 0;
        LED_Datas[i].blue = 0;
    }

    for (e = 0; e < RIPPLE_MAX_EVENTS; e++) {
        if (!ripple_events[e].active) {
            continue;
        }

        uint8_t origin = ripple_events[e].origin_led;
        int16_t cx = led_pos[origin].x;
        int16_t cy = led_pos[origin].y;

        for (i = 0; i < LedNumbers; i++) {
            uint16_t dist;
            uint16_t dr;
            uint8_t bright;
            led_config_t c;

            if (!led_pos[i].valid) {
                continue;
            }

            dist = ripple_dist_approx(
                led_pos[i].x - cx,
                led_pos[i].y - cy
            );

            dr = (dist > ripple_events[e].radius) ?
                 (dist - ripple_events[e].radius) :
                 (ripple_events[e].radius - dist);

            if (dr > RIPPLE_WIDTH) {
                continue;
            }

            bright = (uint8_t)(((uint32_t)(RIPPLE_WIDTH - dr) * ripple_events[e].life) / RIPPLE_WIDTH);
            c = ripple_scale_color(ripple_events[e].color, bright);

            {
                uint16_t r = LED_Datas[i].red + c.red;
                uint16_t g = LED_Datas[i].green + c.green;
                uint16_t b = LED_Datas[i].blue + c.blue;

                LED_Datas[i].red   = (r > 255) ? 255 : r;
                LED_Datas[i].green = (g > 255) ? 255 : g;
                LED_Datas[i].blue  = (b > 255) ? 255 : b;
            }
        }

        ripple_events[e].radius += RIPPLE_RADIUS_STEP;

        if (ripple_events[e].life > RIPPLE_FADE_STEP) {
            ripple_events[e].life -= RIPPLE_FADE_STEP;
        } else {
            ripple_events[e].active = false;
        }
    }

    chBSemSignal(&ledUpdateSem);
    return true;
}
// bool EffectTypeRippleProcess(void) //centre
// {
//     uint8_t row, col, i;

//     if (!ripple_pos_inited) {
//         ripple_init_positions();
//     }

//     for (i = 0; i < LedNumbers; i++) {
//         LED_Datas[i].red = 0;
//         LED_Datas[i].green = 0;
//         LED_Datas[i].blue = 0;
//     }

//     for (i = 0; i < RIPPLE_MAX_EVENTS; i++) {
//         if (!ripple_events[i].active) {
//             continue;
//         }

//         int16_t cx = ripple_pos[ripple_events[i].row][ripple_events[i].col].x;
//         int16_t cy = ripple_pos[ripple_events[i].row][ripple_events[i].col].y;

//         for (row = 0; row < LED_MATRIX_ROWS; row++) {
//             for (col = 0; col < ripple_row_lengths[row]; col++) {
//                 uint16_t dist;
//                 uint16_t dr;
//                 uint8_t bright;
//                 led_config_t c;
//                 int8_t led_idx;

//                 dist = ripple_dist_approx(
//                     ripple_pos[row][col].x - cx,
//                     ripple_pos[row][col].y - cy
//                 );

//                 dr = (dist > ripple_events[i].radius) ?
//                      (dist - ripple_events[i].radius) :
//                      (ripple_events[i].radius - dist);

//                 if (dr > RIPPLE_WIDTH) {
//                     continue;
//                 }

//                 bright = (uint8_t)(((uint32_t)(RIPPLE_WIDTH - dr) * ripple_events[i].life) / RIPPLE_WIDTH);
//                 c = ripple_scale_color(ripple_events[i].color, bright);

//                 led_idx = ledRemap[row][col];
//                 if (led_idx >= 0 && led_idx < LedNumbers) {
//                     uint16_t r = LED_Datas[led_idx].red + c.red;
//                     uint16_t g = LED_Datas[led_idx].green + c.green;
//                     uint16_t b = LED_Datas[led_idx].blue + c.blue;

//                     LED_Datas[led_idx].red   = (r > 255) ? 255 : r;
//                     LED_Datas[led_idx].green = (g > 255) ? 255 : g;
//                     LED_Datas[led_idx].blue  = (b > 255) ? 255 : b;
//                 }
//             }
//         }

//         ripple_events[i].radius += RIPPLE_RADIUS_STEP;

//         if (ripple_events[i].life > RIPPLE_FADE_STEP) {
//             ripple_events[i].life -= RIPPLE_FADE_STEP;
//         } else {
//             ripple_events[i].active = false;
//         }
//     }

//     chBSemSignal(&ledUpdateSem);
//     return true;
// }
// ripple_end
uint8_t ledUpdateInterval = 35;
static EffectrandonColor_t randColor;
static EffectProfile_t eProfile;
static led_runMode_t profilePreviusMode;
// static EffectMosArt_t mosArt;
EffectMosArt_t mosArt;
static EffectRanbowFlow_t ranbowFlow;
static uint16_t rainbowFlowPhaseFraction;
static effect_wheel_t wheel_fx;
led_runMode_t currentLED_RunMode,lastLED_RunMode;
EffectStatus_t effectStatus;
uint8_t LED_DemoModeIndex = 0,LED_DemoCurrentMode = 0;
uint8_t LED_PresetModeIndex = 0,LED_PresetCurrentMode;
EffectStruct_t	LED_DemoMode[LED_DemoModes], LED_PresetMode[LED_PresetModes];
led_config_t LED_NULL;
led_config_t c;

/*
 * 更新 Reactive 實際使用的顏色。
 */
void LED_SetReactiveColor(uint8_t red, uint8_t green, uint8_t blue)
{
    c.red   = red;
    c.green = green;
    c.blue  = blue;

    /*
     * mode 5 是 Reactive。
     * 同步更新 preset，避免重新進入燈效後恢復原色。
     */
    if (LED_PresetModeIndex >= 5U)
    {
        LED_PresetMode[5].color.red   = red;
        LED_PresetMode[5].color.green = green;
        LED_PresetMode[5].color.blue  = blue;
    }

    uprintf(
        "[LED] Reactive color R=%u G=%u B=%u\r\n",
        red,
        green,
        blue
    );
}

int8_t normalEffect;
uint8_t normalBrightness = 30, normalSpeed = 100;

binary_semaphore_t ledEffetSem;

extern virtual_timer_t randColor_vt;
extern binary_semaphore_t ledUpdateSem,ledRunmodeSem;
extern const analogKey_Config_t matrixAnalogRelocate[6][15];
extern uint16_t rainbowBase;

extern bool gameModeEnable;

// #ifdef reboot
// extern volatile bool g_usb_configured_flag;   // ??extern 摰??嚗?閬?= false嚗yon
// #endif

// wheel
#ifndef M_PI
#define M_PI 3.14159265358979323846f
#endif

// static void wheel_init_table(void)
// {
//     static const uint8_t wheel_row_lengths[LED_MATRIX_ROWS] = {26, 16, 17, 17, 16, 15, 12};

//     const float pitch_x = 16.0f;
//     const float pitch_y = 16.0f;

//     float row_offset[LED_MATRIX_ROWS];
//     float center_x;
//     float center_y;

//     uint8_t row, column;

//     for (row = 0; row < LED_MATRIX_ROWS; row++) {
//         row_offset[row] = ((26.0f - wheel_row_lengths[row]) * pitch_x) * 0.5f;
//     }

//     center_x = ((26.0f - 1.0f) * pitch_x) * 0.5f;
//     center_y = ((LED_MATRIX_ROWS - 1.0f) * pitch_y) * 0.5f;

//     /* ??冽????踹??剖??芯蝙?其?蝵格????*/
//     for (row = 0; row < LED_MATRIX_ROWS; row++) {
//         for (column = 0; column < LED_MATRIX_COLS; column++) {
//             wheel_base_hue[row][column] = 0;
//         }
//     }

//     for (row = 0; row < LED_MATRIX_ROWS; row++) {
//         for (column = 0; column < wheel_row_lengths[row]; column++) {
//             float x = row_offset[row] + (column * pitch_x);
//             float y = row * pitch_y;

//             float dx = x - center_x;
//             float dy = y - center_y;

//             float angle = atan2f(dy, dx);
//             float norm = (angle + (float)M_PI) / (2.0f * (float)M_PI);

//             wheel_base_hue[row][column] = (uint8_t)(norm * 255.0f);
//         }
//     }

//     wheel_inited = true;
// }
static void wheel_init_table(void)
{
    uint16_t i;

    if (!led_pos_inited) {
        led_pos_init_from_remap();
    }

    for (i = 0; i < LED_MATRIX_ROWS * LED_MATRIX_COLS; i++) {
        ((uint8_t*)wheel_base_hue)[i] = 0;
    }

    for (i = 0; i < LedNumbers; i++) {
        if (led_pos[i].valid) {
            float dx = (float)(led_pos[i].x - led_center_x);
            float dy = (float)(led_pos[i].y - led_center_y);

            float angle = atan2f(dy, dx);
            float norm = (angle + (float)M_PI) / (2.0f * (float)M_PI);
            uint8_t hue = (uint8_t)(norm * 255.0f);

            /* ???ledRemap 撠?雿蔭嚗???wheel_base_hue[row][col] */
            for (uint8_t row = 0; row < LED_MATRIX_ROWS; row++) {
                for (uint8_t col = 0; col < LED_MATRIX_COLS; col++) {
                    if (ledRemap[row][col] == i) {
                        wheel_base_hue[row][col] = hue;
                    }
                }
            }
        }
    }

    wheel_inited = true;
}
typedef struct {
    uint8_t red;
    uint8_t green;
    uint8_t blue;
} wheel_rgb_t;

/* 0~255 hue -> RGB */
static led_config_t wheel_hue_to_rgb(uint8_t hue)
{
    led_config_t c;
    uint8_t region = hue / 43;                 // 256 / 6 ~= 43
    uint8_t remainder = (hue - (region * 43)) * 6;

    uint8_t p = 0;
    uint8_t q = 255 - remainder;
    uint8_t t = remainder;

    switch (region) {
        default:
        case 0:
            c.red = 255; c.green = t;   c.blue = p;
            break;
        case 1:
            c.red = q;   c.green = 255; c.blue = p;
            break;
        case 2:
            c.red = p;   c.green = 255; c.blue = t;
            break;
        case 3:
            c.red = p;   c.green = q;   c.blue = 255;
            break;
        case 4:
            c.red = t;   c.green = p;   c.blue = 255;
            break;
        case 5:
            c.red = 255; c.green = p;   c.blue = q;
            break;
    }
    return c;
}

/* 鈭桀漲蝮格嚗rightness 雿輻 0~100 ?曉?瘥?*/
static uint8_t led_clamp_percent(uint8_t value)
{
    return (value > 100u) ? 100u : value;
}

static uint8_t led_speed_percent_to_interval(uint8_t speed)
{
    speed = led_clamp_percent(speed);
    return (uint8_t)(175u - (((uint16_t)speed * 140u) / 100u));
}

static uint32_t led_speed_scale_timebase(uint32_t timebase, uint8_t speed)
{
    uint16_t scale = 16u + (((uint16_t)led_clamp_percent(speed) * 112u) / 100u);
    return (timebase * scale) / 16u;
}

static uint8_t mosart_effective_speed(uint8_t mode, uint8_t speed)
{
    speed = led_clamp_percent(speed);

    if (mode == mosArtBreath || mode == mosArtWave) {
        speed = (uint8_t)(((uint16_t)speed * 50u) / 100u);
        if (speed == 0u) {
            speed = 1u;
        }
    }

    return speed;
}

static uint8_t mosart_update_interval(uint8_t mode)
{
    (void)mode;

    /* All MosArt animations refresh at 100 Hz. */
    return 10U;
}

/*
static led_config_t wheel_apply_brightness(led_config_t in, uint8_t brightness)
{
    led_config_t out;
    brightness = led_clamp_percent(brightness);

    out.red   = ((uint16_t)in.red   * brightness) / 100u;
    out.green = ((uint16_t)in.green * brightness) / 100u;
    out.blue  = ((uint16_t)in.blue  * brightness) / 100u;
    return out;
}
*/
// end wheel

static THD_WORKING_AREA(waLED_EffectThread, 256);
static THD_FUNCTION(LED_EffectThread, arg)
{
    (void)arg;
    uint8_t enable;
    chRegSetThreadName("LED Modes");
    chBSemObjectInit(&ledEffetSem, true);
    while (1){
    	chBSemWait(&ledEffetSem);
    	//uprintf ("effect mode:%d profile:%d\r\n",effectStatus.effect,EffectTypeChangeProfile);
        switch (effectStatus.effect)
        {
        case EffectTypeRandColor:
         enable = effectRandowColorProcess();
        break;
    	case EffectTypeMosArt:
        enable = effectMosArtProcess();
        break;
    	case EffectTypeRainbowFlow:
        enable = EffectTypeRainbowFlowProcess();
        break;
    	case EffectTypeColorBrith:
        enable = EffectTypeRainbowFlowProcess();
        break;
    	case EffectTypeKeytest:
    	enable = EffectTypeKeytestProcess();
    	break;
    	case EffectTypeChangeProfile:
    	enable = EffectTypeChangeProfileProcess();
    	break;
        case EffectTypeWheel:
        enable = EffectTypeWheelProcess();
        break;
        case EffectTypeReactive:
        enable = EffectTypeReactiveProcess();
        break;
        case EffectTypeStarlight:
        enable = EffectTypeStarlightProcess();
        break;
        case EffectTypeRipple:
        enable = EffectTypeRippleProcess();
        break;
    	case EffectTypeOff:
        break;
    	case EffectIdle:
        break;
        }

        if (enable)
        {
        	 chVTSetI(&randColor_vt, TIME_MS2I(effectStatus.speed), (vtfunc_t)led_cb, NULL);
        }
    }

}

void LED_SpeedChange(int8_t brightness, uint8_t speed)
{
    currentLED_RunMode.brightness =
        led_clamp_percent((uint8_t)brightness);

    currentLED_RunMode.speed =
        led_clamp_percent(speed);

    normalBrightness =
        currentLED_RunMode.brightness;

    normalSpeed =
        currentLED_RunMode.speed;

    /*
     * Save both brightness and speed.
     */
    eeprom_update_byte(
        KB_COLOR_BRIGHTNESS_ADDR,
        currentLED_RunMode.brightness
    );

    eeprom_update_byte(
        KB_COLOR_BRIGHTNESS_MAGIC_ADDR,
        KB_COLOR_BRIGHTNESS_MAGIC
    );

    eeprom_update_byte(
        KB_COLOR_SPEED_ADDR,
        currentLED_RunMode.speed
    );

    LED_CurrentAdjuest(
        currentLED_RunMode.brightness
    );

    switch (effectStatus.runMode)
    {
        case normalMode:
            switch (LED_PresetMode[normalEffect].effect)
            {
                case EffectTypeRandColor:
                    break;

                case EffectTypeMosArt:
                    mosArt.Brightness =
                        currentLED_RunMode.brightness;
                    break;

                case EffectTypeRainbowFlow:
                    ranbowFlow.Brightness =
                        currentLED_RunMode.brightness;
                    break;

                case EffectTypeKeytest:
                case EffectTypeColorBrith:
                case EffectTypeChangeProfile:
                case EffectTypeOff:
                    break;

                case EffectTypeWheel:
                case EffectTypeReactive:
                case EffectTypeStarlight:
                case EffectTypeRipple:
                    currentLED_RunMode.brightness =
                        led_clamp_percent(
                            (uint8_t)brightness
                        );

                    currentLED_RunMode.speed =
                        led_clamp_percent(speed);
                    break;

                default:
                    break;
            }

						if (LED_PresetMode[normalEffect].effect == EffectTypeRainbowFlow)
						{
								effectStatus.speed = 10U;
						}
						else
						{
								effectStatus.speed = led_speed_percent_to_interval(currentLED_RunMode.speed);
						}
            break;

        case demoMode:
        default:
            break;
    }

    uprintf(
        "[LED SPEED] brightness=%u speed=%u "
        "interval=%u effect=%u mode=%u\r\n",
        (unsigned int)currentLED_RunMode.brightness,
        (unsigned int)currentLED_RunMode.speed,
        (unsigned int)effectStatus.speed,
        (unsigned int)LED_PresetMode[normalEffect].effect,
        (unsigned int)effectStatus.runMode
    );
}

bool LED_Effect(uint8_t effect, bool enable, uint32_t interval, uint8_t colorMode, uint8_t mode, led_config_t color, uint8_t brightness, uint8_t speed, bool direction)
{
    switch (effect)
    {
        case EffectTypeRandColor:
            if (enable)
            {
                if (speed > 100 || brightness > 100)
                {
                    return false;
                }
                randColor.colorMode = colorMode;
                switch (colorMode)
                {
                    case single:
                        randColor.LED = color;
                        break;
                    case ranbow:
                        //randColor.ranbowIndex = 0;
                        break;
                }
                randColor.bright = 0;
                randColor.direction = true;
//                randColor.brightness = brightness;
                effectStatus.speed = led_speed_percent_to_interval(speed);
                randColor.enable = true;
                chVTSet(&randColor_vt, TIME_MS2I(effectStatus.speed), (vtfunc_t)led_cb, NULL);
            }
            else
            {
                randColor.enable = false;
            }
            break;
        case EffectTypeMosArt:
            if (enable)
            {
                if (speed > 100 || brightness > 100 || mode > mosArtWave || colorMode > ranbow)
                {
                    return false;
                }
                mosArt.Mode = mode;
                mosArt.Rainbow = colorMode;
                mosArt.Speed = speed; //problem
                // mosArt.Speed = --speed;
//                mosArt.Brightness = brightness;
                effectStatus.speed = mosart_update_interval(mode);
								mosArt.TimerBase = 0U;
								mosArt.enable = true;
                switch (colorMode)
                {
                    case single:
                        mosArt.LED = color;
                        break;
                    case ranbow:
                        break;
                }
                chVTSet(&randColor_vt, TIME_MS2I(effectStatus.speed), (vtfunc_t)led_cb, NULL);
            }
            else
            {
                mosArt.enable = false;
            }
            break;

					case EffectTypeRainbowFlow:
							if (enable)
							{
									if (speed > 100U || brightness > 100U)
									{
											return false;
									}

									ranbowFlow.Brightness = brightness;
									effectStatus.speed = 10U;
									ranbowFlow.timebase = rainbowBase;
									rainbowFlowPhaseFraction = 0U;
									ranbowFlow.enable = true;
									ranbowFlow.direction = direction;

									chVTSet(
											&randColor_vt,
											TIME_MS2I(effectStatus.speed),
											(vtfunc_t)led_cb,
											NULL
									);
							}
							else
							{
									ranbowFlow.enable = false;
							}
							break;
        case EffectTypeKeytest:
        	chVTSet(&randColor_vt, TIME_MS2I(effectStatus.speed), (vtfunc_t)led_cb, NULL);
            break;

        case EffectTypeChangeProfile:
        	effectStatus.speed = led_speed_percent_to_interval(speed);
        	chVTSet(&randColor_vt, TIME_MS2I(effectStatus.speed), (vtfunc_t)led_cb, NULL);
        	break;
        case EffectTypeColorBrith:
            break;
        case EffectTypeWheel:
            if (enable) {
                if (speed > 100 || brightness > 100) {
                    return false;
                }
                wheel_fx.enable = true;
                wheel_fx.offset = 0;

                effectStatus.speed = led_speed_percent_to_interval(speed);

                chBSemSignal(&ledEffetSem);

                chVTSet(&randColor_vt, TIME_MS2I(effectStatus.speed), (vtfunc_t)led_cb, NULL);
            }
            break;

						case EffectTypeReactive:
								if (enable)
								{
										if (speed > 100 || brightness > 100)
										{
												return false;
										}

										/*
										 * Reactive 執行函式實際使用全域 c。
										 */
										c = color;

										effectStatus.speed =
												led_speed_percent_to_interval(speed);

										chBSemSignal(&ledEffetSem);

										chVTSet(
												&randColor_vt,
												TIME_MS2I(effectStatus.speed),
												(vtfunc_t)led_cb,
												NULL
										);
								}
								break;
        case EffectTypeStarlight:
            if (enable)
            {
                if (speed > 100 || brightness > 100) {
                    return false;
                }

                starlight.enable = true;

                starlight.density = 2u + (((uint16_t)speed * 18u) / 100u);
                starlight.base_dim = 0;

                starlight_reset_all();

                effectStatus.speed = led_speed_percent_to_interval(speed);
                chVTSet(&randColor_vt, TIME_MS2I(effectStatus.speed), (vtfunc_t)led_cb, NULL);
            }
            else
            {
                starlight.enable = false;
            }
            break;
        case EffectTypeRipple:
            if (enable)
            {
                if (speed > 100 || brightness > 100) {
                    return false;
                }

                ripple_clear_events();

                effectStatus.speed = led_speed_percent_to_interval(speed);
                chVTSet(&randColor_vt, TIME_MS2I(effectStatus.speed), (vtfunc_t)led_cb, NULL);
            }
            break;
        case EffectTypeOff:
            if (randColor.enable)
            {
                randColor.enable = false;
                randColor.ranbowIndex++;
                chVTReset(&randColor_vt);
            }
            // ledAllOff(true);
            led_config_t bg;
            bg.red = 0;
            bg.green = 0;
            bg.blue = 2;
            led_updateFrameBufferSingle(0, LedNumbers, &bg);
            effectStatus.effect = EffectIdle;
            return true;
            break;
        case EffectIdle:
            break;
    }

    effectStatus.effect = effect;
    effectStatus.enable = enable;
    effectStatus.colorMode = colorMode;
    effectStatus.mode = mode;
    effectStatus.brightness = brightness;
    effectStatus.direction = direction;
    effectStatus.timeStamp = get_system_time_ticks();
    effectStatus.interval = interval;
    return true;
}

bool EffectTypeKeytestProcess (void){
	chVTReset(&randColor_vt);
	//getKeysStatus(true);
	chBSemSignal(&ledUpdateSem);
	return true;
}

void setChangeProfileEffect(bool enable, uint8_t row, uint8_t column, led_config_t color){
	eProfile.enable = enable;
	if (enable){
		if (currentLED_RunMode.mode == 11){return;}
		eProfile.row = row;
		eProfile.col = column;
		eProfile.color = color;

		profilePreviusMode = currentLED_RunMode;
		currentLED_RunMode.type = normalMode;
		currentLED_RunMode.mode = 11;
		setRunMode(currentLED_RunMode);
	} else {
		currentLED_RunMode = profilePreviusMode;
	}
	setRunMode(currentLED_RunMode);

}

bool EffectTypeChangeProfileProcess(void)
{
    static bool status = false;

    chVTReset(&randColor_vt);

    if (status) {
        setKeymapLED(
            eProfile.row,
            eProfile.col,
            eProfile.color
        );
    } else {
        setKeymapLED(
            eProfile.row,
            eProfile.col,
            LED_NULL
        );
    }

    status = !status;

    chBSemSignal(&ledUpdateSem);

    return true;
}

bool effectRandowColorProcess(void)
{
    led_config_t ledTemp;
    const led_config_t *sourceColor;

    /*
     * 先確保彩虹顏色索引合法，避免讀取陣列範圍之外。
     */
    if (randColor.ranbowIndex >=
        (uint8_t)(sizeof(rainbowColor) / sizeof(rainbowColor[0])))
    {
        randColor.ranbowIndex = 0U;
    }

    /*
     * 選擇目前要顯示的原始顏色。
     */
    switch (randColor.colorMode)
    {
        case single:
            sourceColor = &randColor.LED;
            break;

        case ranbow:
            sourceColor = &rainbowColor[randColor.ranbowIndex];
            break;

        default:
            sourceColor = &randColor.LED;
            break;
    }

    /*
     * 恢復真正的呼吸亮度效果。
     *
     * 原始設計：
     * brightTemp = bright * brightness / 1000
     * output RGB = source RGB * brightTemp
     *
     * 使用 uint32_t 避免乘法溢位，並避免 float 運算。
     */
    ledTemp.red = (uint8_t)(
        ((uint32_t)sourceColor->red *
         (uint32_t)randColor.bright *
         (uint32_t)randColor.brightness) / 1000U
    );

    ledTemp.green = (uint8_t)(
        ((uint32_t)sourceColor->green *
         (uint32_t)randColor.bright *
         (uint32_t)randColor.brightness) / 1000U
    );

    ledTemp.blue = (uint8_t)(
        ((uint32_t)sourceColor->blue *
         (uint32_t)randColor.bright *
         (uint32_t)randColor.brightness) / 1000U
    );

    /*
     * 將計算後的呼吸亮度顏色更新到全部 LED。
     */
    led_updateFrameBufferSingle(
        0,
        LedNumbers,
        &ledTemp
    );

    /*
     * 呼吸變亮階段。
     */
    if (randColor.direction)
    {
        if (randColor.bright >= 100)
        {
            randColor.bright = 100;
            randColor.direction = false;
        }
        else
        {
            if (randColor.bright <= 97)
            {
                randColor.bright += 3;
            }
            else
            {
                randColor.bright = 100;
            }
        }
    }
    /*
     * 呼吸變暗階段。
     */
    else
    {
        if (randColor.bright == 0)
        {
            /*
             * 彩虹模式每完成一次呼吸後，
             * 切換到下一個顏色。
             */
            if (randColor.colorMode == ranbow)
            {
                randColor.ranbowIndex++;

                if (randColor.ranbowIndex >=
                    (uint8_t)(sizeof(rainbowColor) /
                              sizeof(rainbowColor[0])))
                {
                    randColor.ranbowIndex = 0;
                }
            }

            if (randColor.enable)
            {
                randColor.direction = true;

                /*
                 * 效果執行時間到期。
                 */
                if ((effectStatus.interval != 0U) &&
                    ((get_system_time_ticks() -
                      effectStatus.timeStamp) >
                     effectStatus.interval))
                {
                    chVTReset(&randColor_vt);

                    randColor.enable = false;

                    ledAllOff(false);

                    effectStatus.effect = EffectIdle;

                    if (effectStatus.runMode == demoMode)
                    {
                        chBSemSignal(&ledRunmodeSem);
                    }

                    return false;
                }
            }
            else
            {
                chVTReset(&randColor_vt);

                randColor.enable = false;

                ledAllOff(false);

                effectStatus.effect = EffectIdle;

                return false;
            }
        }
        else
        {
            /*
             * 避免 uint8_t 在 1 或 2 時減 3 發生下溢，
             * 例如 1 - 3 變成 254。
             */
            if (randColor.bright >= 3)
            {
                randColor.bright -= 3;
            }
            else
            {
                randColor.bright = 0;
            }
        }
    }

    return true;
}

#define MOSART_FRAME_DEBUG 1

#if MOSART_FRAME_DEBUG
static void mosart_debug_frame(uint8_t mode,
                               uint16_t timer_base,
                               uint8_t brightness)
{
    static uint32_t last_tick;
    static uint32_t window_start;
    static uint16_t frame_count;
    static uint16_t min_delta = 0xFFFFU;
    static uint16_t max_delta;
    uint32_t now;
    uint16_t delta;

    now = get_system_time_ticks();

    if (last_tick == 0U)
    {
        last_tick = now;
        window_start = now;
        frame_count = 0U;
        min_delta = 0xFFFFU;
        max_delta = 0U;
        return;
    }

    delta = (uint16_t)(now - last_tick);
    last_tick = now;

    if (delta < min_delta)
    {
        min_delta = delta;
    }

    if (delta > max_delta)
    {
        max_delta = delta;
    }

    frame_count++;

    if (frame_count >= 100U)
    {
        uprintf(
            "[MOSART FRAME] mode=%u frames=100 elapsed=%lu "
            "avg=%lu ms min=%u ms max=%u ms base=%u bright=%u\r\n",
            (unsigned int)mode,
            (unsigned long)(now - window_start),
            (unsigned long)((now - window_start) / frame_count),
            (unsigned int)min_delta,
            (unsigned int)max_delta,
            (unsigned int)timer_base,
            (unsigned int)brightness
        );

        window_start = now;
        frame_count = 0U;
        min_delta = 0xFFFFU;
        max_delta = 0U;
    }
}
#endif

bool effectMosArtProcess(void)
{
    float tempmBright;
    uint8_t bright;
    uint8_t effect_speed;
    led_config_t ledTemp;

    if (mosArt.enable)
    {
        effect_speed = mosart_effective_speed(mosArt.Mode,mosArt.Speed);

        if ((effectStatus.interval != 0U) && ((get_system_time_ticks() - effectStatus.timeStamp) > effectStatus.interval))
        {
            chVTReset(&randColor_vt);
            mosArt.enable = false;

            ledAllOff(false);

            effectStatus.effect = EffectIdle;

            if (effectStatus.runMode == demoMode)
            {
                chBSemSignal(&ledRunmodeSem);
            }

            return false;
        }

        switch (mosArt.Mode)
        {
            case mosArtStatic:
                bright = 255U;
                break;

						case mosArtBreath:
								calculate_mosart_breath_volume(effect_speed,mosArt.TimerBase,&bright);

#if MOSART_FRAME_DEBUG
								mosart_debug_frame(mosArt.Mode,mosArt.TimerBase,bright);
#endif
								break;

            case mosArtFlash:
                Calculate_Flashing_Volume(mosArt.Speed,mosArt.TimerBase,&bright);
                break;

            case mosArtWave:
            {
                const uint8_t WAVE_WAVELENGTH_COLS = 12U;
                const uint16_t phase_step_col = (uint16_t)(256U / WAVE_WAVELENGTH_COLS);
                const uint16_t phase_step_row = 0U;
                const uint16_t frameTimebase = mosArt.TimerBase;
                led_config_t baseColor = mosArt.LED;

                if (mosArt.Rainbow != 0U)
                {
                    Calculate_Colorful_Color(0U,frameTimebase,&baseColor);
                }

                for (uint8_t col = 0U;col < LED_MATRIX_COLS;col++)
                {
                    for (uint8_t row = 0U;row < LED_MATRIX_ROWS;row++)
                    {
                        uint16_t localTimebase = frameTimebase;

                        uint8_t breathBrightness = 0U;
                        led_config_t pixelColor;
                        localTimebase += (uint16_t)col * phase_step_col;
                        localTimebase += (uint16_t)row * phase_step_row;
                        calculate_mosart_breath_volume(effect_speed,localTimebase,&breathBrightness);
                        pixelColor.red = (uint8_t)(((uint32_t)baseColor.red * breathBrightness) / 255U);
                        pixelColor.green = (uint8_t)(((uint32_t)baseColor.green * breathBrightness) / 255U);
                        pixelColor.blue = (uint8_t)(((uint32_t)baseColor.blue * breathBrightness) / 255U);
                        setRemapLED(row,col,pixelColor);
                    }
                }

                chBSemSignal(&ledUpdateSem);

                /*
                 * 10 ms frame rate:
                 * move one unit every frame.
                 */
                mosArt.TimerBase = (uint16_t)(frameTimebase + 1U);
                return true;
            }

            default:
                bright = 0U;
                break;
        }

        tempmBright = (float)bright / 255.0f;

        if (mosArt.Rainbow != 0U)
        {
            Calculate_Colorful_Color(effect_speed,mosArt.TimerBase,&mosArt.LED);
        }

        ledTemp.red = (uint8_t)(mosArt.LED.red * tempmBright);
        ledTemp.green = (uint8_t)(mosArt.LED.green * tempmBright);
        ledTemp.blue = (uint8_t)(mosArt.LED.blue * tempmBright);
        led_updateFrameBufferSingle(0U,LedNumbers,&ledTemp);
				
#if MOSART_FRAME_DEBUG
        mosart_debug_frame(mosArt.Mode,mosArt.TimerBase,bright);
#endif

        /*
         * Breath: 1 unit / 10 ms.
         * Flash/static: 3 units / 10 ms.
         */
        mosArt.TimerBase += (mosArt.Mode == mosArtBreath || mosArt.Mode == mosArtWave) ? 1U : 3U;

        return true;
    }

    chVTReset(&randColor_vt);
    mosArt.enable = false;
    ledAllOff(false);
    effectStatus.effect = EffectIdle;

    return false;
}

#define RAINBOW_FRAME_DEBUG 1

#if RAINBOW_FRAME_DEBUG
static void rainbow_debug_frame(uint16_t timebase)
{
    static uint32_t last_tick;
    static uint32_t window_start;
    static uint16_t frame_count;
    static uint16_t min_delta = 0xFFFFU;
    static uint16_t max_delta;
    uint32_t now;
    uint16_t delta;

    now = get_system_time_ticks();

    if (last_tick == 0U)
    {
        last_tick = now;
        window_start = now;
        frame_count = 0U;
        min_delta = 0xFFFFU;
        max_delta = 0U;
        return;
    }

    delta = (uint16_t)(now - last_tick);
    last_tick = now;

    if (delta < min_delta)
    {
        min_delta = delta;
    }

    if (delta > max_delta)
    {
        max_delta = delta;
    }

    frame_count++;

    /* 彩虹流動在 speed=20 約 147 ms/frame，10 frame 約 1.5 秒。 */
    if (frame_count >= 10U)
    {
        uprintf(
            "[RAINBOW FRAME] frames=10 elapsed=%lu "
            "avg=%lu ms min=%u ms max=%u ms base=%u\r\n",
            (unsigned long)(now - window_start),
            (unsigned long)((now - window_start) / frame_count),
            (unsigned int)min_delta,
            (unsigned int)max_delta,
            (unsigned int)timebase
        );

        window_start = now;
        frame_count = 0U;
        min_delta = 0xFFFFU;
        max_delta = 0U;
    }
}
#endif

bool EffectTypeWheelProcess(void)
{
    uint8_t row;
    uint8_t col;

    if (!wheel_fx.enable)
    {
        chVTReset(&randColor_vt);
        ledAllOff(false);
        effectStatus.effect = EffectIdle;
        return false;
    }

    if (!led_pos_inited)
    {
        led_pos_init_from_remap();
    }

    if ((effectStatus.interval != 0U) &&
        ((get_system_time_ticks() -
          effectStatus.timeStamp) >
         effectStatus.interval))
    {
        chVTReset(&randColor_vt);
        wheel_fx.enable = false;
        ledAllOff(false);
        effectStatus.effect = EffectIdle;

        if (effectStatus.runMode == demoMode)
        {
            chBSemSignal(&ledRunmodeSem);
        }

        return false;
    }

    for (row = 0U;
         row < LED_MATRIX_ROWS;
         row++)
    {
        for (col = 0U;
             col < LED_MATRIX_COLS;
             col++)
        {
            int8_t idx = ledRemap[row][col];

            if ((idx < 0) ||
                (idx >= LedNumbers))
            {
                continue;
            }

            {
                uint8_t hue =
                    (uint8_t)(
                        wheel_base_hue[row][col] +
                        wheel_fx.offset
                    );

                led_config_t color =
                    wheel_hue_to_rgb(hue);

                LED_Datas[idx] = color;
            }
        }
    }

    wheel_fx.offset +=
        (uint8_t)(
            1U +
            (((uint16_t)
              led_clamp_percent(
                  currentLED_RunMode.speed
              ) * 7U) / 100U)
        );

    chBSemSignal(&ledUpdateSem);
    return true;
}

bool EffectTypeRainbowFlowProcess(void)
{
    uint8_t row;
    uint8_t column;
    uint8_t old_interval_ms;
    uint16_t column_phase_step;
    uint16_t frame_phase_step;
    uint16_t frame_timebase;

    if (!ranbowFlow.enable)
    {
        chVTReset(&randColor_vt);
        ledAllOff(false);
        effectStatus.effect = EffectIdle;
        return false;
    }

    if ((effectStatus.interval != 0U) &&
        ((get_system_time_ticks() -
          effectStatus.timeStamp) >
         effectStatus.interval))
    {
        chVTReset(&randColor_vt);
        ranbowFlow.enable = false;
        ledAllOff(false);
        effectStatus.effect = EffectIdle;

        if (effectStatus.runMode == demoMode)
        {
            chBSemSignal(&ledRunmodeSem);
        }

        return false;
    }

    /*
     * RainbowFlow frame rate remains fixed at 10 ms.
     * Use the old speed interval only to calculate phase movement.
     */
    old_interval_ms =
        led_speed_percent_to_interval(
            currentLED_RunMode.speed
        );

    column_phase_step =
        (uint16_t)rainbowBase;

    if (ranbowFlow.direction)
    {
        column_phase_step += 4U;
    }

    frame_phase_step =
        (uint16_t)(
            (
                ((uint32_t)column_phase_step *
                 LED_MATRIX_COLS *
                 10U) +
                ((uint32_t)old_interval_ms / 2U)
            ) /
            old_interval_ms
        );

    /*
     * First reduce the 100 Hz equivalent step.
     */
    frame_phase_step /= 4U;

    /*
     * Reduce movement by another 60%.
     * Keep 40% of the current RainbowFlow speed.
     */
    frame_phase_step =
        (uint16_t)(
            (((uint32_t)frame_phase_step * 10U) +
             50U) / 100U
        );

    if (frame_phase_step == 0U)
    {
        frame_phase_step = 1U;
    }

    frame_timebase = ranbowFlow.timebase;

    for (column = 0U;
         column < LED_MATRIX_COLS;
         column++)
    {
        uint16_t column_timebase;

        column_timebase =
            (uint16_t)(
                frame_timebase +
                ((uint16_t)column *
                 column_phase_step)
            );

        Calculate_Colorful_Color(
            0U,
            column_timebase,
            &ranbowFlow.LED
        );

        for (row = 0U;
             row < LED_MATRIX_ROWS;
             row++)
        {
            setRemapLED(
                row,
                column,
                ranbowFlow.LED
            );
        }
    }

    ranbowFlow.timebase =
        (uint16_t)(
            frame_timebase +
            frame_phase_step
        );

#if RAINBOW_FRAME_DEBUG
    rainbow_debug_frame(ranbowFlow.timebase);
#endif

    chBSemSignal(&ledUpdateSem);
    return true;
}

bool EffectTypeReactiveProcess(void)
{
    uint8_t row, col;
    led_config_t px;

    for (row = 0; row < MATRIX_ROWS; row++) {
        for (col = 0; col < MATRIX_COLS; col++) {
            uint8_t level = reactive_level[row][col];

            if (level) {
                px.red   = ((uint16_t)c.red   * level) / 255;
                px.green = ((uint16_t)c.green * level) / 255;
                px.blue  = ((uint16_t)c.blue  * level) / 255;

                setKeymapLED(row, col, px);

                if (level > 20) {
                    reactive_level[row][col] -= 20;
                } else {
                    reactive_level[row][col] = 0;
                    setKeymapLED(row, col, LED_NULL);   // ?
                }
            } else {
                setKeymapLED(row, col, LED_NULL);       // 靽嚗Ⅱ靽?????
            }
        }
    }

    chBSemSignal(&ledUpdateSem);
    return true;
}

bool EffectTypeStarlightProcess(void)
{
    uint16_t i;
    uint8_t spawn_count;

    if (!starlight.enable) {
        chVTReset(&randColor_vt);
        ledAllOff(false);
        effectStatus.effect = EffectIdle;
        return false;
    }

    if (effectStatus.interval != 0 &&
        (get_system_time_ticks() - effectStatus.timeStamp > effectStatus.interval)) {
        chVTReset(&randColor_vt);
        starlight.enable = false;
        ledAllOff(false);
        effectStatus.effect = EffectIdle;

        if (effectStatus.runMode == demoMode) {
            chBSemSignal(&ledRunmodeSem);
        }
        return false;
    }

    /* ??蝛箸擃??*/
    ledAllOff(false);

    /* 瘥????游???嚗?摨阡?銝�暺?*/
    spawn_count = starlight.density;

    for (i = 0; i < spawn_count; i++) {
        uint16_t idx = starlight_rand() % LedNumbers;

        if (!starlight_leds[idx].active) {
            starlight_leds[idx].active = 1;
            starlight_leds[idx].phase = starlight_rand8();
            starlight_leds[idx].speed = 8 + (starlight_rand8() % 18);  // 8~25
            starlight_leds[idx].color = starlight_pick_color();
        }
    }

    /* ?湔?�????*/
    for (i = 0; i < LedNumbers; i++) {
        if (starlight_leds[i].active) {
            uint8_t bright;
            led_config_t c;

            bright = starlight_triangle_wave(starlight_leds[i].phase);

            /* 霈??擃?閬云?箇嚗?隞?憿?*/
            bright = (uint8_t)(((uint16_t)bright * led_clamp_percent(effectStatus.brightness)) / 100u);

            c = starlight_scale_color(starlight_leds[i].color, bright);
            LED_Datas[i] = c;

            starlight_leds[i].phase += starlight_leds[i].speed;

            /* ??撠望?憭梧?銋???冽??? */
            if (starlight_leds[i].phase >= 250 || starlight_leds[i].phase <= starlight_leds[i].speed) {
                if ((starlight_rand8() & 0x03) == 0) {
                    starlight_leds[i].active = 0;
                    LED_Datas[i].red = 0;
                    LED_Datas[i].green = 0;
                    LED_Datas[i].blue = 0;
                }
            }
        }
    }

    chBSemSignal(&ledUpdateSem);
    return true;
}
//Mos Art effect
void Calculate_Colorful_Color(uint8_t speed, uint32_t timebase, led_config_t* LED)
{
    uint8_t color, colorInv;
    timebase = led_speed_scale_timebase(timebase, speed);
    color = (uint8_t)(timebase & 0xFF);
    colorInv = ~color;
    switch (((uint8_t)((timebase >> 8) & 0xFF)) & 0x03)
    {
        default:
            LED->red = color;
            LED->green = 0;
            LED->blue = colorInv;
            break;
        case 1:
            LED->red = 255;
            LED->green = color;
            LED->blue = 0;
            break;
        case 2:
            LED->red = colorInv;
            LED->green = 255;
            LED->blue = 0;
            break;
        case 3:
            LED->red = 0;
            LED->green = colorInv;
            LED->blue = color;
            break;
    }
}

static void calculate_mosart_breath_volume(uint8_t speed,uint16_t timebase,uint8_t *bright)
{
    uint32_t scaled_timebase;
    uint16_t scale_per_thousand;
    uint16_t phase;
    uint16_t wtemp;

    /*
     * New frame rate:
     *   1 unit / 10 ms = 100 units/s
     *
     * Old breath rate:
     *   3 units / 50 ms = 60 units/s
     *
     * Therefore apply 0.6x of the old speed scale,
     * so the visible breathing speed remains unchanged.
     */
    speed = led_clamp_percent(speed);

    scale_per_thousand =
        (uint16_t)(600U + ((uint16_t)speed * 42U));

    scaled_timebase =
        ((uint32_t)timebase *
         (uint32_t)scale_per_thousand) / 1000U;

    phase = (uint16_t)scaled_timebase;

    if ((((phase >> 8) & 0x01U) == 0U))
    {
        *bright = (uint8_t)(phase & 0xFFU);
    }
    else
    {
        *bright = (uint8_t)(
            255U - (phase & 0xFFU)
        );
    }

    if (*bright <= 15U)
    {
        *bright = 0U;
    }
    else if (*bright >= 240U)
    {
        *bright = 255U;
    }
    else
    {
        wtemp = (uint16_t)(*bright - 15U);
        wtemp = (wtemp * 255U) / 225U;
        *bright = (uint8_t)wtemp;
    }
}

void calculate_breath_volume(uint8_t speed, uint16_t timebase, uint8_t* bright)
{
    uint16_t wtemp;
    timebase = (uint16_t)led_speed_scale_timebase(timebase, speed);
    if (((uint8_t)((timebase >> 8) & 0xff) & 0x01) == 0x00)
    {
        *bright = (uint8_t)(timebase & 0xff);
    }
    else
    {
        *bright = 255 - (uint8_t)(timebase & 0xff);
    }
    if (*bright <= 15)
    {
        *bright = 0;
    }
    else if (*bright >= 240)
    {
        *bright = 255;
    }
    else
    {
        wtemp = (uint16_t) * bright - 15;
        wtemp = (wtemp * 255) / 225;
        *bright = (uint8_t)wtemp;
    }
}


void Calculate_Flashing_Volume(uint8_t speed, uint16_t timebase, uint8_t* bright)
{
    timebase = (uint16_t)led_speed_scale_timebase(timebase, speed);
    if (((timebase >> 8) & 0x01u) == 0u)
    {
        *bright = 255;
    }
    else
    {
        *bright = 0;
    }
}

void Calculate_Waving_Volumn(uint8_t speed,uint16_t timebase,uint8_t *bright)
{
    /*
     * 橫向波長：每 12 column 完成一個波形週期。
     * 不使用 row 相位差，所以 phase_step_row 固定為 0。
     */
    const uint8_t  WAVE_WAVELENGTH_COLS = 12U;
    const uint16_t TIMEBASE_STEP        = 10U;

    const uint16_t phase_step_col =
        (uint16_t)(256U / WAVE_WAVELENGTH_COLS);

    const uint16_t phase_step_row = 0U;

    led_config_t baseColor = mosArt.LED;

    /*
     * 彩虹模式下，依傳入的 timebase 計算基礎顏色。
     */
    if (mosArt.Rainbow != 0U) {
        Calculate_Colorful_Color(
            0U,
            timebase,
            &baseColor
        );
    }

    uint8_t lastBrightness = 0U;

    for (uint8_t col = 0U; col < LED_MATRIX_COLS; col++) {
        for (uint8_t row = 0U; row < LED_MATRIX_ROWS; row++) {
            uint16_t localTimebase = timebase;

            localTimebase +=
                (uint16_t)col * phase_step_col;

            localTimebase +=
                (uint16_t)row * phase_step_row;

            calculate_breath_volume(
                speed,
                localTimebase,
                &lastBrightness
            );

            led_config_t pixelColor;

            pixelColor.red = (uint8_t)(
                ((uint32_t)baseColor.red *
                 lastBrightness) / 255U
            );

            pixelColor.green = (uint8_t)(
                ((uint32_t)baseColor.green *
                 lastBrightness) / 255U
            );

            pixelColor.blue = (uint8_t)(
                ((uint32_t)baseColor.blue *
                 lastBrightness) / 255U
            );

            setRemapLED(row, col, pixelColor);
        }
    }

    /*
     * 保留舊介面的輸出參數。
     * 回傳最後一個 pixel 計算出的呼吸亮度。
     */
    if (bright != NULL) {
        *bright = lastBrightness;
    }
		
#if MOSART_FRAME_DEBUG
		mosart_debug_frame(mosArt.Mode,mosArt.TimerBase,*bright);
#endif

    chBSemSignal(&ledUpdateSem);

    mosArt.TimerBase =
        (uint16_t)(timebase + TIMEBASE_STEP);
}

bool LED_ModesBuilder(
    uint8_t action,
    bool start,
    uint8_t effect,
    bool enable,
    uint32_t interval,
    uint8_t colorMode,
    uint8_t mode,
    led_config_t color,
    uint8_t brightness,
    uint8_t speed,
    bool direction
)
{
    const uint8_t demoModeCount =
        (uint8_t)(sizeof(LED_DemoMode) /
                  sizeof(LED_DemoMode[0]));

    const uint8_t presetModeCount =
        (uint8_t)(sizeof(LED_PresetMode) /
                  sizeof(LED_PresetMode[0]));

    switch (action)
    {
        case newDemo:
        {
            /*
             * 建立新的 Demo 清單，從第 0 筆開始。
             */
            LED_DemoModeIndex = 0U;

            LED_DemoMode[LED_DemoModeIndex].effect     = effect;
            LED_DemoMode[LED_DemoModeIndex].enable     = enable;
            LED_DemoMode[LED_DemoModeIndex].interval   = interval;
            LED_DemoMode[LED_DemoModeIndex].colorMode  = colorMode;
            LED_DemoMode[LED_DemoModeIndex].mode       = mode;
            LED_DemoMode[LED_DemoModeIndex].color      = color;
            LED_DemoMode[LED_DemoModeIndex].brightness = brightness;
            LED_DemoMode[LED_DemoModeIndex].speed      = speed;
            LED_DemoMode[LED_DemoModeIndex].direction  = direction;

            if (start)
            {
                chVTReset(&randColor_vt);
                ledAllOff(false);

                effectStatus.runMode = demoMode;
                LED_DemoCurrentMode = 0U;

                chBSemSignal(&ledRunmodeSem);
            }

            return true;
        }

        case addDemoEffect:
        {
            /*
             * 先檢查下一個 index 是否超過陣列範圍，
             * 確認安全後才遞增。
             */
            if (((uint16_t)LED_DemoModeIndex + 1U) >=
                demoModeCount)
            {
                return false;
            }

            LED_DemoModeIndex++;

            LED_DemoMode[LED_DemoModeIndex].effect     = effect;
            LED_DemoMode[LED_DemoModeIndex].enable     = enable;
            LED_DemoMode[LED_DemoModeIndex].interval   = interval;
            LED_DemoMode[LED_DemoModeIndex].colorMode  = colorMode;
            LED_DemoMode[LED_DemoModeIndex].mode       = mode;
            LED_DemoMode[LED_DemoModeIndex].color      = color;
            LED_DemoMode[LED_DemoModeIndex].brightness = brightness;
            LED_DemoMode[LED_DemoModeIndex].speed      = speed;
            LED_DemoMode[LED_DemoModeIndex].direction  = direction;

            if (start)
            {
                chVTReset(&randColor_vt);
                ledAllOff(false);

                effectStatus.runMode = demoMode;
                LED_DemoCurrentMode = 0U;

                chBSemSignal(&ledRunmodeSem);
            }

            return true;
        }

        case newPreset:
        {
            /*
             * 建立新的 Preset 清單，從第 0 筆開始。
             */
            LED_PresetModeIndex = 0U;

            LED_PresetMode[LED_PresetModeIndex].effect     = effect;
            LED_PresetMode[LED_PresetModeIndex].enable     = enable;
            LED_PresetMode[LED_PresetModeIndex].interval   = 0U;
            LED_PresetMode[LED_PresetModeIndex].colorMode  = colorMode;
            LED_PresetMode[LED_PresetModeIndex].mode       = mode;
            LED_PresetMode[LED_PresetModeIndex].color      = color;
            LED_PresetMode[LED_PresetModeIndex].brightness =
                normalBrightness;
            LED_PresetMode[LED_PresetModeIndex].speed =
                normalSpeed;
            LED_PresetMode[LED_PresetModeIndex].direction =
                direction;

            return true;
        }

        case addPresetEffect:
        {
            /*
             * 檢查下一筆是否仍在 LED_PresetMode 陣列範圍內。
             */
            if (((uint16_t)LED_PresetModeIndex + 1U) >=
                presetModeCount)
            {
                return false;
            }

            LED_PresetModeIndex++;

            LED_PresetMode[LED_PresetModeIndex].effect     = effect;
            LED_PresetMode[LED_PresetModeIndex].enable     = enable;
            LED_PresetMode[LED_PresetModeIndex].interval   = 0U;
            LED_PresetMode[LED_PresetModeIndex].colorMode  = colorMode;
            LED_PresetMode[LED_PresetModeIndex].mode       = mode;
            LED_PresetMode[LED_PresetModeIndex].color      = color;
            LED_PresetMode[LED_PresetModeIndex].brightness =
                normalBrightness;
            LED_PresetMode[LED_PresetModeIndex].speed =
                normalSpeed;
            LED_PresetMode[LED_PresetModeIndex].direction =
                direction;

            return true;
        }

        default:
            return false;
    }
}


void buildDemoModes(void){
uint8_t currentBrightness = 50;
uint16_t currentSpeed = 100;
	    LED_ModesBuilder(newDemo , false,EffectTypeRainbowFlow, true, 5000, 0, 0,  LED_NULL, currentBrightness, currentSpeed, false);
	    LED_ModesBuilder(addDemoEffect , false,EffectTypeRainbowFlow, true, 5000, 0, 0,  LED_NULL, currentBrightness, currentSpeed, true);
	    LED_ModesBuilder(addDemoEffect , false,EffectTypeMosArt, true, 5000, ranbow, mosArtStatic,  rainbowColor[green], currentBrightness, currentSpeed, 0);
	    LED_ModesBuilder(addDemoEffect , false,EffectTypeMosArt, true, 5000, single, mosArtStatic,  rainbowColor[white], currentBrightness, currentSpeed, 0);
	    LED_ModesBuilder(addDemoEffect , false,EffectTypeMosArt, true, 5000, single, mosArtBreath,  rainbowColor[violet], currentBrightness, currentSpeed, 0);


		//    LED_ModesBuilder(newDemo , false,EffectTypeMosArt, true, 1500, single, mosArtStatic,  rainbowColor[red], currentBrightness, currentSpeed, 0);
		//    LED_ModesBuilder(addDemoEffect , false,EffectTypeMosArt, true, 1500, single, mosArtStatic,  rainbowColor[green], currentBrightness, currentSpeed, 0);
		//    LED_ModesBuilder(addDemoEffect , false,EffectTypeMosArt, true, 1500, single, mosArtStatic,  rainbowColor[blue], currentBrightness, currentSpeed, 0);
		//    LED_ModesBuilder(addDemoEffect , false,EffectTypeMosArt, true, 1500, single, mosArtStatic,  rainbowColor[yellow], currentBrightness, currentSpeed, 0);
		//    LED_ModesBuilder(addDemoEffect , false,EffectTypeMosArt, true, 1500, single, mosArtStatic,  rainbowColor[white], currentBrightness, currentSpeed, 0);
		//    LED_ModesBuilder(addDemoEffect , false,EffectTypeMosArt, true, 1500, single, mosArtStatic,  rainbowColor[orange], currentBrightness, currentSpeed, 0);
		//    LED_ModesBuilder(addDemoEffect , false,EffectTypeMosArt, true, 1500, single, mosArtStatic,  rainbowColor[indigo], currentBrightness, currentSpeed, 0);
		//    LED_ModesBuilder(addDemoEffect , true,EffectTypeMosArt, true, 1500, single, mosArtStatic,  rainbowColor[violet], currentBrightness, currentSpeed, 0);
}

void buildNormalModes(void){
	//	LED_ModesBuilder(newPreset , false,EffectTypeRainbowFlow, true, 0, 0, 0,  LED_NULL, normalBrightness, normalSpeed, true);
	//	LED_ModesBuilder(addPresetEffect , false,EffectTypeRainbowFlow, true, 0, 0, 0,  LED_NULL, normalBrightness, normalSpeed, false);
	//	LED_ModesBuilder(addPresetEffect , false,EffectTypeMosArt, true, 0, ranbow, mosArtStatic,  rainbowColor[green], normalBrightness, normalSpeed, 0);
	//	LED_ModesBuilder(addPresetEffect , false,EffectTypeMosArt, true, 0, single, mosArtStatic,  rainbowColor[green], normalBrightness, normalSpeed, 0);
	//	LED_ModesBuilder(addPresetEffect , true,EffectTypeRandColor, true, 0, ranbow, 0,  rainbowColor[green], normalBrightness, normalSpeed, 0);

	// LED_ModesBuilder(newPreset , false,EffectTypeMosArt, true, 5000, ranbow, mosArtStatic,  rainbowColor[red], normalBrightness, normalSpeed, 0);
    LED_ModesBuilder(newPreset , false,EffectTypeMosArt, true, 5000, single, mosArtStatic,  LED_NULL, normalBrightness, normalSpeed, 0); // lyon change
    // LED_ModesBuilder(newPreset , false,EffectTypeMosArt, true, 5000, single, mosArtStatic,  {0,0,0}, normalBrightness, normalSpeed, 0); // lyon change

	// LED_ModesBuilder(addPresetEffect , false,EffectTypeMosArt, true, 5000, ranbow, mosArtBreath,  rainbowColor[green], normalBrightness, normalSpeed, 0);
    LED_ModesBuilder(addPresetEffect , false,EffectTypeMosArt, true, 5000, single, mosArtStatic,  c, normalBrightness, normalSpeed, 0);

	// LED_ModesBuilder(addPresetEffect , false,EffectTypeMosArt, true, 5000, single, mosArtFlash,  rainbowColor[blue], normalBrightness, normalSpeed, 0);
    LED_ModesBuilder(addPresetEffect , false,EffectTypeMosArt, true, 5000, single, mosArtBreath,  c, normalBrightness, normalSpeed, true);

	// LED_ModesBuilder(addPresetEffect , false,EffectTypeMosArt, true, 5000, ranbow, mosArtFlash,  rainbowColor[yellow], normalBrightness, normalSpeed, 0);
    // LED_ModesBuilder(addPresetEffect, false, EffectTypeWheel, true, 0, 0, 0, LED_NULL, normalBrightness, normalSpeed, true /* direction */);
    LED_ModesBuilder(addPresetEffect , false,EffectTypeRainbowFlow, true, 5000, 0, 0,  LED_NULL, normalBrightness, normalSpeed, false);

	LED_ModesBuilder(addPresetEffect , false,EffectTypeMosArt, true, 5000, single, mosArtWave,  c, normalBrightness, normalSpeed, 0);
	
	LED_ModesBuilder(addPresetEffect, false,EffectTypeReactive, true, 5000, single, mosArtStatic, c, normalBrightness, normalSpeed, false); // 5 Reactive，使用 EEPROM 讀出的顏色
    
		LED_ModesBuilder(addPresetEffect , false, EffectTypeStarlight, true, 5000, single, mosArtStatic, rainbowColor[orange], normalBrightness, normalSpeed, 0); //6
	LED_ModesBuilder(addPresetEffect , false, EffectTypeRipple, true, 5000, single, mosArtStatic,  c, normalBrightness, normalSpeed, 0);
    LED_ModesBuilder(addPresetEffect , false, EffectTypeWheel, true, 5000, single, mosArtStatic, rainbowColor[orange], normalBrightness, normalSpeed, 0); //8
	LED_ModesBuilder(addPresetEffect , false,EffectTypeRainbowFlow, true, 5000, 0, 0,  LED_NULL, normalBrightness, normalSpeed, false);
	LED_ModesBuilder(addPresetEffect , false,EffectTypeRainbowFlow, true, 5000, 0, 0,  LED_NULL, normalBrightness, normalSpeed, true);
	// LED_ModesBuilder(addPresetEffect , false, EffectTypeKeytest, true, 5000, 0, 0,  LED_NULL, normalBrightness, normalSpeed, true);
	LED_ModesBuilder(addPresetEffect , false,EffectTypeChangeProfile, true, 5000, 0, 0,  LED_NULL, normalBrightness, normalSpeed, true);



}

static inline uint8_t u8_max3(uint8_t a, uint8_t b, uint8_t c) {
    uint8_t m = (a > b) ? a : b;
    return (m > c) ? m : c;
}
static inline uint8_t u8_min3(uint8_t a, uint8_t b, uint8_t c) {
    uint8_t m = (a < b) ? a : b;
    return (m < c) ? m : c;
}

hsv_t rgb_to_hsv(led_config_t rgb)
{
    hsv_t hsv;
    uint8_t r = rgb.red, g = rgb.green, b = rgb.blue;

    uint8_t max = u8_max3(r, g, b);
    uint8_t min = u8_min3(r, g, b);
    uint8_t delta = (uint8_t)(max - min);

    hsv.v = max;

    // S
    if (max == 0) {
        hsv.s = 0;
        hsv.h = 0;
        return hsv;
    }
    hsv.s = (uint8_t)((uint16_t)delta * 255 / max);

    // H
    if (delta == 0) {
        hsv.h = 0;   // ?∪蔗?莎?h ?⊥?蝢抬?蝯?0
        return hsv;
    }

    int16_t h; // ?怠??迂鞎
    if (max == r) {
        // h = 60 * (g-b)/delta  (mod 360)
        h = (int16_t)(60 * (int16_t)(g - b) / delta);
    } else if (max == g) {
        // h = 120 + 60*(b-r)/delta
        h = (int16_t)(120 + (60 * (int16_t)(b - r) / delta));
    } else {
        // max == b
        // h = 240 + 60*(r-g)/delta
        h = (int16_t)(240 + (60 * (int16_t)(r - g) / delta));
    }

    if (h < 0) h += 360;
    if (h >= 360) h -= 360;

    hsv.h = (uint16_t)h;
    return hsv;
}

led_config_t hsv_to_rgb(hsv_t hsv)
{
    led_config_t rgb;

    uint8_t s = hsv.s;
    uint8_t v = hsv.v;

    if (s == 0) {
        // ?圈?
        rgb.red = v;
        rgb.green = v;
        rgb.blue = v;
        return rgb;
    }

    // Hue 0..359
    uint16_t h = hsv.h % 360;

    // region: 0..5
    uint8_t region = h / 60;
    uint16_t f_deg = h - (uint16_t)region * 60; // 0..59

    // ??0..255 ??fraction 餈撮
    uint16_t f = (f_deg * 255) / 60; // 0..250(撌血)

    uint8_t p = (uint16_t)v * (255 - s) / 255;
    uint8_t q = (uint16_t)v * (255 - ((uint16_t)s * f / 255)) / 255;
    uint8_t t = (uint16_t)v * (255 - ((uint16_t)s * (255 - f) / 255)) / 255;

    switch (region) {
        case 0: rgb.red = v; rgb.green = t; rgb.blue = p; break;
        case 1: rgb.red = q; rgb.green = v; rgb.blue = p; break;
        case 2: rgb.red = p; rgb.green = v; rgb.blue = t; break;
        case 3: rgb.red = p; rgb.green = q; rgb.blue = v; break;
        case 4: rgb.red = t; rgb.green = p; rgb.blue = v; break;
        default: rgb.red = v; rgb.green = p; rgb.blue = q; break; // region 5
    }
    return rgb;
}

bool setRunMode(led_runMode_t runMode)
{
    if (gameModeEnable)
    {
        return false;
    }

    chVTReset(&randColor_vt);

    switch (runMode.type)
    {
        case normalMode:
            ledAllOff(false);

            /*
             * uint8_t 從 0 往下減時會下溢成 255，
             * 此時切換至最後一個燈效。
             */
            if (runMode.mode == UINT8_MAX)
            {
                runMode.mode = LED_PresetModeIndex;
            }
            else if (runMode.mode > LED_PresetModeIndex)
            {
                runMode.mode = 0;
            }

            runMode.brightness =
                led_clamp_percent(runMode.brightness);

            if (runMode.brightness < 1U)
            {
                runMode.brightness = 1U;
            }

            runMode.speed =
                led_clamp_percent(runMode.speed);

            LED_PresetCurrentMode = runMode.mode;
            effectStatus.runMode = normalMode;

            normalEffect = runMode.mode;
            normalBrightness = runMode.brightness;
            normalSpeed = runMode.speed;

            chBSemSignal(&ledRunmodeSem);
            break;

        case demoMode:
            ledAllOff(false);
            LED_DemoCurrentMode = 0;
            effectStatus.runMode = demoMode;
            chBSemSignal(&ledRunmodeSem);
            break;

        case testMode:
            break;

        case dynamticLightingMode:
            effectStatus.effect = EffectIdle;
            ledAllOff(false);
            break;

        case offMode:
            effectStatus.effect = EffectIdle;
            ledAllOff(true);
            break;

        case powerOff:
            ledAllOff(true);
            break;

        default:
            return false;
    }

    lastLED_RunMode = runMode;

    if (runMode.type == powerOff)
    {
        palClearLine(SPI_LED_SDB_PIN);
    }
    else
    {
        palSetLine(SPI_LED_SDB_PIN);
    }

    return true;
}

void LED_init(void)
{
    uint8_t ee_mode;
    uint8_t ee_brightness;
    uint8_t ee_speed;

    c.red   = eeprom_read_byte(KB_COLOR_RED_ADDR);
    c.green = eeprom_read_byte(KB_COLOR_GREEN_ADDR);
    c.blue  = eeprom_read_byte(KB_COLOR_BLUE_ADDR);

    ee_mode = eeprom_read_byte(KB_COLOR_MODE_ADDR);
    ee_brightness = eeprom_read_byte(KB_COLOR_BRIGHTNESS_ADDR);
    ee_speed = eeprom_read_byte(KB_COLOR_SPEED_ADDR);

    if (ee_brightness <= 100U)
    {
        normalBrightness = ee_brightness;
    }
    else
    {
        normalBrightness = 30U;
    }

    if (ee_speed <= 100U)
    {
        normalSpeed = ee_speed;
    }
    else
    {
        normalSpeed = 100U;
    }

    uprintf("\r\n");
    uprintf("========== LED EEPROM ==========\r\n");
    uprintf("Mode  : %3u (0x%02X)\r\n", ee_mode, ee_mode);
    uprintf("Bright: %3u%%\r\n", normalBrightness);
    uprintf("Speed : %3u%%\r\n", normalSpeed);

    uprintf(
        "Color : R=%3u (0x%02X) "
        "G=%3u (0x%02X) "
        "B=%3u (0x%02X)\r\n",
        c.red,
        c.red,
        c.green,
        c.green,
        c.blue,
        c.blue
    );

    uprintf("================================\r\n");

    led_creatTask();

    buildDemoModes();
    buildNormalModes();

    led_pos_init_from_remap();
    wheel_init_table();
    wheel_inited = true;

    chThdCreateStatic(
        waLED_EffectThread,
        sizeof(waLED_EffectThread),
        NORMALPRIO - 3,
        LED_EffectThread,
        NULL
    );

    /*
     * EEPROM blank: use preset 3.
     */
    if (ee_mode == 0xFFU)
    {
        ee_mode = 3U;
    }

    currentLED_RunMode.mode = ee_mode;
    currentLED_RunMode.brightness = normalBrightness;
    currentLED_RunMode.speed = normalSpeed;
    currentLED_RunMode.type = normalMode;

    uprintf(
        "[LED INIT] RunMode mode=%u brightness=%u "
        "speed=%u type=%u\r\n",
        (unsigned int)currentLED_RunMode.mode,
        (unsigned int)currentLED_RunMode.brightness,
        (unsigned int)currentLED_RunMode.speed,
        (unsigned int)currentLED_RunMode.type
    );

    setRunMode(currentLED_RunMode);
}

void led_set_effect_color(uint8_t red, uint8_t green, uint8_t blue)
{
    uint8_t index;
    led_config_t new_color;

    new_color.red   = red;
    new_color.green = green;
    new_color.blue  = blue;

    /*
     * 更新共用燈效顏色。
     */
    c = new_color;

    /*
     * 同步 MosArt 顏色。
     */
    mosArt.LED = new_color;

    /*
     * setRunMode() starts the currently selected effect from
     * LED_PresetMode[normalEffect].color.  Sync this entry as well, so a
     * brightness change cannot re-apply the colour from before the last
     * Hue/Saturation adjustment.
     */
    if ((currentLED_RunMode.type == normalMode) &&
        (normalEffect >= 0) &&
        ((uint8_t)normalEffect <= LED_PresetModeIndex) &&
        (LED_PresetMode[(uint8_t)normalEffect].colorMode == single))
    {
        LED_PresetMode[(uint8_t)normalEffect].color = new_color;

        uprintf(
            "[LED] Active preset index=%u color=%u/%u/%u\r\n",
            (unsigned int)normalEffect,
            new_color.red,
            new_color.green,
            new_color.blue
        );
    }

    /*
     * 更新所有 Reactive preset 的顏色。
     */
    for (index = 0U; index <= LED_PresetModeIndex; index++)
    {
        if (LED_PresetMode[index].effect == EffectTypeReactive)
        {
            LED_PresetMode[index].color = new_color;

            uprintf(
                "[LED] Reactive preset index=%u color=%u/%u/%u\r\n",
                index,
                new_color.red,
                new_color.green,
                new_color.blue
            );
        }
    }

    uprintf(
        "[LED COLOR RAM] R=%u G=%u B=%u\r\n",
        c.red,
        c.green,
        c.blue
    );
}




