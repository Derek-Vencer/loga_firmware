#ifndef LED_EFFECT_H_
#define LED_EFFECT_H_

#include "led_function.h"
#include "report.h"

bool LED_Effect(uint8_t effect, bool enable, uint32_t interval, uint8_t colorMode, uint8_t mode, led_config_t color, uint8_t brightness, uint8_t speed, bool direction);
bool effectRandowColorProcess(void);
bool effectMosArtProcess(void);
bool EffectTypeRainbowFlowProcess(void);
bool EffectTypeKeytestProcess(void);
bool EffectTypeChangeProfileProcess (void);
bool EffectTypeWheelProcess (void);
bool EffectTypeReactiveProcess (void);
bool EffectTypeStarlightProcess(void);
bool EffectTypeRippleProcess(void);
void ripple_add_event(uint8_t row, uint8_t col, led_config_t color);
void Calculate_Colorful_Color(uint8_t speed, uint32_t timebase, led_config_t* LED);
void calculate_breath_volume(uint8_t speed, uint16_t timebase, uint8_t* bright);
void Calculate_Flashing_Volume(uint8_t speed, uint16_t timebase, uint8_t* bright);

bool LED_DemoModesBuilder(uint8_t action,bool start,uint8_t effect, bool enable, uint32_t interval, uint8_t colorMode, uint8_t mode, led_config_t color, uint8_t brightness, uint8_t speed, bool direction);
bool setRunMode(led_runMode_t runMode);
void buildDemoModes(void);
void buildNormalModes(void);
void LED_init(void);
void setChangeProfileEffect(bool enable, uint8_t row, uint8_t column, led_config_t color);
void LED_SetReactiveColor(uint8_t red, uint8_t green, uint8_t blue);

hsv_t rgb_to_hsv(led_config_t rgb);
led_config_t hsv_to_rgb(hsv_t hsv);

extern led_config_t LED_NULL;

#define REACTIVE_MAX 255
extern uint8_t reactive_level[MATRIX_ROWS][MATRIX_COLS];
#endif /* LED_EFFECT_H_ */
