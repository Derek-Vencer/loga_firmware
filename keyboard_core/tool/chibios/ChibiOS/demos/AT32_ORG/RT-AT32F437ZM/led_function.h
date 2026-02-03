#ifndef LED_FUNCTION_H_
#define LED_FUNCTION_H_


void led_cb(void);
void led_creatTask(void);



#define LedNumbers 108+1
//#define LedNumbers 33+1
//#define LedNumbers 2+1
#define defaultBright 25

#define LED_DemoModes	15

#define LED_PresetModes	15

//typedef union {
//  uint32_t raw;
//  struct {
//    uint8_t red;
//    uint8_t green;
//    uint8_t blue;
//    uint8_t btight;
//  };
//} led_config_t;

typedef struct {
	uint8_t red;
	uint8_t green;
	uint8_t blue;
} led_config_t;


typedef struct {
	bool enable;
	uint8_t colorMode;
	led_config_t LED;
	uint8_t brightness;
	//uint8_t speed;
	int16_t ranbowIndex;
	int8_t bright;
	bool direction;
	float lastBrightTemp;
} EffectrandonColor_t;

typedef struct {
	bool enable;
	uint8_t Mode;
	uint8_t Rainbow;
	uint8_t Brightness;
	uint8_t bright;
	uint8_t Speed;
	led_config_t LED;
	uint8_t bbreathvolume;
	uint8_t bFlashingVolume;
	uint16_t TimerBase;
} EffectMosArt_t;


typedef struct {
	bool enable;
	uint8_t Brightness;
	uint8_t bright;
	//uint8_t speed;
	led_config_t LED;
	bool direction;
	uint16_t timebase;
} EffectRanbowFlow_t;


typedef struct {
	uint8_t runMode;
	uint8_t effect;
	bool enable;
	uint8_t colorMode;
	uint8_t mode;
	uint8_t brightness;
	uint8_t speed;
	bool direction;
	uint32_t timeStamp;
	uint32_t interval;
} EffectStatus_t;


typedef struct {
	uint8_t effect;
	bool enable;
	uint32_t interval;
	uint8_t colorMode;
	uint8_t mode;
	led_config_t color;
	uint8_t brightness;
	uint8_t speed;
	bool direction;
} EffectStruct_t;



enum effectTypes{
	EffectIdle,
	EffectTypeRandColor,
	EffectTypeMosArt,
	EffectTypeRainbowFlow,
	EffectTypeColorBrith,
	EffectTypeOff,
};

enum mosArtModes{
	mosArtStatic,
	mosArtBreath,
	mosArtFlash,
};

enum ranbowColors{
	red,
	orange,
	yellow,
	green,
	blue,
	indigo,
	violet,
	white,
	dummy,
	off
};

enum colorModes{
	single,
	ranbow,
};

enum ledRunModes{
	normalMode,
	demoMode,
	testMode,
	offMode,
};

enum ledDemoModeBuilderMode{
	newDemo,
	addDemoEffect,
	newPreset,
	addPresetEffect,
};


static const led_config_t rainbowColor[] = {
    {255, 0, 0},
    {192, 96, 0},
    {255, 255, 0},
    { 0, 255, 0},
    { 0, 0, 255},
    { 0, 255, 255},
    {255, 0, 255},
    {170, 170, 170},
    { 0,  0,  0},
    { 0,  0,  0},
};

static const int8_t ledKeyMatrix[10][16]={
    //	  0   1   2   3   4   5   6   7   8   9  10  11  12  13  14  15
    { 15, 14, 13, 12, 11, 10,  9,  8,  7,  6,  5,  4,  3, -1, 33, 35},
		{ 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 40, 38},
		{ 58, 57, 55, 54, 53, 52, 51, 50, 49, 48, 47, 46, 45, 44, 75, 77},
		{ 59, 60, 62, 63, 66, 67, 68, 69, 70, 71, 72, 73, -1, 74, 81, 79},
		{ 94, -1, 93, 92, 91, 90, 89, 88, 87, 86, 85, 84, -1, 83,106,107},
		{ 95, 96, 97, -1, -1, -1, 98, -1, -1, 99,100,101,102, 82, 80, 78},
		{ -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,  1,  0, 76, 37},
		{ -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 31, 32,105, 36},
		{ -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 42, 41,104, 34},
		{ -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,103, 39},
};

static const int8_t ledRemap[6][21] = {
    { 15,  -1,  -1,  14,  13,  12,  11,  10,    9,  8,   7,    6,  5,   3,   2,   1,   0,   0,  -1,  -1,  -1},
	{ 16,  17,  18,  19,  20,  21,  22,  23,  24,  25,  26,  27,  28,  29,  30,  31,  32,  33,  34,  35,  36},
	{ 58,  57,  55,  54,  53,  52,  51,  50,  49,  48,  47,  46,  45,  44,  43,  42,  41,  40,  39,  38,  37},
	{ 59,  60,  62,  63,  66,  67,  68,  69,  70,  71,  72,  73,  -1,  74,  -1,  -1,  -1,  75,  76,  77,  -1},
	{ 94,  93,  92,  91,  90,  89,  88,  87,  86,  85,  84,  -1,  83,  -1,  -1,  82,  -1,  81,  80,  79,  78},
	{ 95,  97,  -1,  -1,  -1,  98,  -1,  -1,  -1,  99, 100, 101,  -1, 102, 103, 104, 105,  -1, 106, 107,  -1},
};


#endif /* LED_FUNCTION_H_ */
