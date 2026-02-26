
/*
    Copyright (C) 2023 iOne Electrionc
*/
# include "encoder.h"
# include "hal.h"
# include "main.h"
# include "led_function.h"
# include "led_effect.h"

#include "usb_main.h" // lyon fix reboot dynamic lighting 
#include "matrix.h" // all the define lyon

uint8_t ledUpdateInterval = 35;
static EffectrandonColor_t randColor;
static EffectProfile_t eProfile;
static led_runMode_t profilePreviusMode;
// static EffectMosArt_t mosArt;
EffectMosArt_t mosArt; // lyon
static EffectRanbowFlow_t ranbowFlow;
led_runMode_t currentLED_RunMode,lastLED_RunMode;
EffectStatus_t effectStatus;
uint8_t LED_DemoModeIndex = 0,LED_DemoCurrentMode = 0;
uint8_t LED_PresetModeIndex = 0,LED_PresetCurrentMode;
EffectStruct_t	LED_DemoMode[LED_DemoModes], LED_PresetMode[LED_PresetModes];
led_config_t LED_NULL;

int8_t normalEffect;
uint8_t normalBrightness = 3, normalSpeed = 2;

binary_semaphore_t ledEffetSem;

extern virtual_timer_t randColor_vt;
extern binary_semaphore_t ledUpdateSem,ledRunmodeSem;
extern const analogKey_Config_t matrixAnalogRelocate[6][15];
extern uint16_t rainbowBase;

extern bool gameModeEnable;

// #ifdef reboot
// extern volatile bool g_usb_configured_flag;   // ★ extern 宣告（不要 = false）lyon
// #endif

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

void LED_SpeedChange(int8_t brightness, uint8_t speed){
	if (currentLED_RunMode.brightness > 5){
		currentLED_RunMode.brightness = 5;
	}
	if (currentLED_RunMode.brightness < 0){
		currentLED_RunMode.brightness = 0;
	}
	if (currentLED_RunMode.speed < 1){
		currentLED_RunMode.speed = 1;
	}
	if (currentLED_RunMode.speed > 4){
		currentLED_RunMode.speed = 4;
	}
	LED_CurrentAdjuest(currentLED_RunMode.brightness);
	switch (effectStatus.runMode)
	{
		case normalMode:
			switch (LED_PresetMode[normalEffect].effect){
				case EffectTypeRandColor:
//					effectStatus.speed = speed * ledUpdateInterval;
//					randColor.brightness = brightness;
					break;
		        case EffectTypeMosArt:
//					mosArt.Brightness = brightness;
//					effectStatus.speed = 33;
		        	break;
		        case EffectTypeRainbowFlow:
//	                ranbowFlow.Brightness = brightness;
//	                effectStatus.speed = speed * (ledUpdateInterval);
		        	break;
		        case EffectTypeKeytest:
		        	break;
		        case EffectTypeColorBrith:
		        	break;
		        case EffectTypeChangeProfile:
		        	break;
		        case EffectTypeOff:
		        	break;
			}
            effectStatus.speed = speed * (ledUpdateInterval);
			break;
		case demoMode:
			break;
	}
}


bool LED_Effect(uint8_t effect, bool enable, uint32_t interval, uint8_t colorMode, uint8_t mode, led_config_t color, uint8_t brightness, uint8_t speed, bool direction)
{
    switch (effect)
    {
        case EffectTypeRandColor:
            if (enable)
            {
                if (speed <= 0 || speed > 5 || (brightness < 1 || brightness > 10))
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
                effectStatus.speed = speed * ledUpdateInterval;
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
                if (speed > 5 || (brightness < 1 || brightness > 10) || mode > mosArtWave || colorMode > ranbow)
                {
                    return false;
                }
                mosArt.Mode = mode;
                mosArt.Rainbow = colorMode;
                mosArt.Speed = --speed;
//                mosArt.Brightness = brightness;
                effectStatus.speed = 33;
                mosArt.TimerBase = 0;
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
                if (speed > 5 || (brightness < 1 || brightness > 10))
                {
                    return false;
                }
                ranbowFlow.Brightness = brightness;
                effectStatus.speed = speed * (ledUpdateInterval);
                ranbowFlow.timebase = rainbowBase;
                ranbowFlow.enable = true;
                ranbowFlow.direction = direction;
                chVTSet(&randColor_vt, TIME_MS2I(effectStatus.speed), (vtfunc_t)led_cb, NULL);
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
        	effectStatus.speed = speed * 1000;
        	chVTSet(&randColor_vt, TIME_MS2I(effectStatus.speed), (vtfunc_t)led_cb, NULL);
        	break;
        case EffectTypeColorBrith:
            break;
        case EffectTypeOff:
            if (randColor.enable)
            {
                randColor.enable = false;
                randColor.ranbowIndex++;
                chVTReset(&randColor_vt);
            }
            ledAllOff(true);
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


bool EffectTypeChangeProfileProcess (void){
static uint16_t row,column,color;
static status = false;
	chVTReset(&randColor_vt);
	if (status){
		setKeymapLED(eProfile.row,eProfile.col, eProfile.color);
	} else {
		setKeymapLED(eProfile.row,eProfile.col, LED_NULL);
	}
	status = ~status;
	chBSemSignal(&ledUpdateSem);
	return true;
}


bool effectRandowColorProcess(void)
{
    float brightTemp;
    led_config_t ledTemp;
//    brightTemp = (float)(randColor.bright * randColor.brightness) / 1000;
    switch (randColor.colorMode)
    {
        case single:
//            ledTemp.red = randColor.LED.red * brightTemp;
//            ledTemp.green = randColor.LED.green * brightTemp;
//            ledTemp.blue = randColor.LED.blue * brightTemp;
            led_updateFrameBufferSingle(0, LedNumbers, &randColor.LED);
            break;
        case ranbow://
//            ledTemp.red = rainbowColor[randColor.ranbowIndex].red * brightTemp;
//            ledTemp.green = rainbowColor[randColor.ranbowIndex].green * brightTemp;
//            ledTemp.blue = rainbowColor[randColor.ranbowIndex].blue * brightTemp;
            led_updateFrameBufferSingle(0, LedNumbers, &rainbowColor[randColor.ranbowIndex]);
            break;
    }
//    led_updateFrameBufferSingle(0, LedNumbers, &ledTemp);

    if (randColor.direction)
    {
        if (randColor.bright >= 100)
        {
            randColor.direction = false;
        }
        else
        {
            randColor.bright += 3;
        }
    }
    else
    {
        if (randColor.bright <= 0)
        {
            if (randColor.colorMode)
            {
                randColor.ranbowIndex++;
            }
            if (randColor.enable)
            {
                randColor.direction = true;
                if (effectStatus.interval!=0 && (get_system_time_ticks()-effectStatus.timeStamp>effectStatus.interval)){
                    chVTReset(&randColor_vt);
                    randColor.enable = false;
                    ledAllOff(false);
                    effectStatus.effect = EffectIdle;
                    if (effectStatus.runMode == demoMode){
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
            randColor.bright -= 3;
        }
    }
    if (randColor.bright > 100) { randColor.bright = 100; }
    if (randColor.ranbowIndex > (uint8_t)(sizeof(sizeof(rainbowColor) / 3)+2)){
    randColor.ranbowIndex = 0;
}
return true;
}

bool effectMosArtProcess(void)
{
    float tempmBright;
    uint8_t bright;
    led_config_t ledTemp;
    if (mosArt.enable)
    {
    	if (effectStatus.interval!=0 && (get_system_time_ticks()-effectStatus.timeStamp>effectStatus.interval)){
            chVTReset(&randColor_vt);
            mosArt.enable = false;
//            LED_FadoutEffect(LedNumbers,100);
//            chThdSleepMilliseconds(150);
            ledAllOff(false);
            effectStatus.effect = EffectIdle;
            if (effectStatus.runMode == demoMode){
            	chBSemSignal(&ledRunmodeSem);
            }
            return false;
        }
        switch (mosArt.Mode)
        {
            case 0:
                bright = 255;
                break;
            case 1:
                calculate_breath_volume(mosArt.Speed, mosArt.TimerBase, &bright);
                break;

            case 2:
                Calculate_Flashing_Volume(mosArt.Speed, mosArt.TimerBase, &bright);
                break;
            case 3:{
                // === 參數 ===
                const uint8_t  WAVE_WAVELENGTH_COLS = 12;   // 幾欄一個亮暗週期
                const uint8_t  WAVE_WAVELENGTH_ROWS = 0;    // 0=不做列向；>0 可做斜向
                const uint16_t TIMEBASE_STEP        = 10;   // 與你原本 Mos Art 同步
                uint16_t phase_step_col = WAVE_WAVELENGTH_COLS ? (256u / WAVE_WAVELENGTH_COLS) : 0;
                uint16_t phase_step_row = WAVE_WAVELENGTH_ROWS ? (256u / WAVE_WAVELENGTH_ROWS) : 0;

                // 單色：用 mosArt.LED（把 mosArt.Rainbow 設 0 即單色）
                led_config_t base = mosArt.LED;

                uint16_t t0 = mosArt.TimerBase;  // 本幀時間基準
                for (uint8_t col = 0; col < LED_MATRIX_COLS; col++) {
                    for (uint8_t row = 0; row < LED_MATRIX_ROWS; row++) {
                        uint16_t local_tb = t0;
                        if (phase_step_col) local_tb += (uint16_t)col * phase_step_col;
                        if (phase_step_row) local_tb += (uint16_t)row * phase_step_row;

                        uint8_t b;                                 // 0..255 三角波亮度
                        calculate_breath_volume(mosArt.Speed, local_tb, &b);  //:contentReference[oaicite:3]{index=3}

                        // 依 b 套到單色
                        led_config_t px;
                        px.red   = (uint8_t)(((uint16_t)base.red   * b) / 255);
                        px.green = (uint8_t)(((uint16_t)base.green * b) / 255);
                        px.blue  = (uint8_t)(((uint16_t)base.blue  * b) / 255);

                        setRemapLED(row, col, px);                // per-pixel 寫入:contentReference[oaicite:4]{index=4}
                    }
                }

                chBSemSignal(&ledUpdateSem);                      // 送畫面:contentReference[oaicite:5]{index=5}
                mosArt.TimerBase = t0 + TIMEBASE_STEP;            // 時間推進
                return true;                                      // <== 提早結束，避免被共用結尾覆蓋
                // Calculate_Waving_Volumn(mosArt.Speed, mosArt.TimerBase, &bright); // lyon this is old one
                // break;
            }
        }
        tempmBright = (float)bright / 255;
        if (mosArt.Rainbow != 0)
        {
            Calculate_Colorful_Color(mosArt.Speed, mosArt.TimerBase, &mosArt.LED);
        }

        ledTemp.red = mosArt.LED.red * tempmBright;
        ledTemp.green = mosArt.LED.green * tempmBright;
        ledTemp.blue = mosArt.LED.blue * tempmBright;
        led_updateFrameBufferSingle(0, LedNumbers, &ledTemp);
        mosArt.TimerBase += 10;
        return true;
    }
    else
    {
        chVTReset(&randColor_vt);
        mosArt.enable = false;
        ledAllOff(false);
        effectStatus.effect = EffectIdle;
    }
    return false;
}

bool EffectTypeRainbowFlowProcess(void)
{
    uint8_t row, column;
//    float tempmBright;
    if (ranbowFlow.enable)
    {
    	if (effectStatus.interval!=0 && (get_system_time_ticks()-effectStatus.timeStamp>effectStatus.interval)){
            chVTReset(&randColor_vt);
            ranbowFlow.enable = false;
            ledAllOff(false);
            effectStatus.effect = EffectIdle;
            if (effectStatus.runMode == demoMode){
            	chBSemSignal(&ledRunmodeSem);
            }
            return false;
        }
//        tempmBright = (float)(ranbowFlow.Brightness) / 10;
//        for (column = 0; column < 16; column++)
//        {
//            Calculate_Colorful_Color(0, ranbowFlow.timebase, &ranbowFlow.LED);
//            if (ranbowFlow.direction)
//            {
//                ranbowFlow.timebase += 99;
//            }
//            else
//            {
//                ranbowFlow.timebase += 96;
//            }
//            ranbowFlow.LED.red *= tempmBright;
//            ranbowFlow.LED.green *= tempmBright;
//            ranbowFlow.LED.blue *= tempmBright;
//            for (row = 0; row < 6; row++)
//            {
//                setRemapLED(row, column, ranbowFlow.LED);
//            }
//        }

        for (column = 0; column < LED_MATRIX_COLS; column++)
        {
            Calculate_Colorful_Color(0, ranbowFlow.timebase, &ranbowFlow.LED);
            if (ranbowFlow.direction)
            {
                ranbowFlow.timebase += rainbowBase+4;
            }
            else
            {
                ranbowFlow.timebase += rainbowBase;
            }
//            ranbowFlow.LED.red *= tempmBright;
//            ranbowFlow.LED.green *= tempmBright;
//            ranbowFlow.LED.blue *= tempmBright;
            for (row = 0; row < LED_MATRIX_ROWS; row++)
            {
                setRemapLED(row, column, ranbowFlow.LED);
            }
        }
        chBSemSignal(&ledUpdateSem);
        return true;
    }
    else
    {
        chVTReset(&randColor_vt);
        ranbowFlow.enable = false;
        ledAllOff(false);
        effectStatus.effect = EffectIdle;
    }
    return false;
}


//Mos Art effect
void Calculate_Colorful_Color(uint8_t speed, uint32_t timebase, led_config_t* LED)
{
    uint8_t i;
    uint8_t color, colorInv;
    timebase >>= 1;
    for (i = 0; i < speed; i++)
    {
        timebase >>= 1;
    }
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

void calculate_breath_volume(uint8_t speed, uint16_t timebase, uint8_t* bright)
{
    uint8_t i;
    uint16_t wtemp;
    timebase >>= 1;
    for (i = 0; i < speed; i++)
    {
        timebase >>= 1;
    }
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
    uint16_t blinkOnBit;
    timebase >>= 1;
    blinkOnBit = 0x0040;
    do
    {
        blinkOnBit <<= 1;
    } while (speed--);
    if (!(timebase & blinkOnBit))
    {
        *bright = 255;
    }
    else
    {
        *bright = 0;
    }
}

void Calculate_Waving_Volumn(uint8_t speed, uint16_t timebase, uint8_t* bright){
    // case mosArtWave: 
    // === 可調參數 ===
    // 幾「欄」形成一個完整亮暗週期（越小越密）
    const uint8_t  WAVE_WAVELENGTH_COLS = 12;
    // 幾「列」形成一個週期；0=不沿列做，>0 可做斜向/棋盤式起伏
    const uint8_t  WAVE_WAVELENGTH_ROWS = 0;    // 例如想斜向可設 8
    // 每幀時間推進量（維持與你其他 Mos Art 效果一致）
    const uint16_t TIMEBASE_STEP        = 10;

    uint16_t phase_step_col = WAVE_WAVELENGTH_COLS ? (256u / WAVE_WAVELENGTH_COLS) : 0;
    uint16_t phase_step_row = WAVE_WAVELENGTH_ROWS ? (256u / WAVE_WAVELENGTH_ROWS) : 0;

    // 單色或彩虹：跟原架構一致
    led_config_t baseColor = mosArt.LED;
    if (mosArt.Rainbow != 0) {
        // 若要保留彩虹底色可開這行（但你要單色，mosArt.Rainbow=0 即可）
        Calculate_Colorful_Color(0, mosArt.TimerBase, &baseColor);  // 現成函式
    }

    for (uint8_t col = 0; col < LED_MATRIX_COLS; col++) {
        for (uint8_t row = 0; row < LED_MATRIX_ROWS; row++) {
            // 空間相位：以欄/列位置偏移 timebase，形成波面
            uint16_t local_tb = mosArt.TimerBase;
            if (phase_step_col) local_tb += (uint16_t)col * phase_step_col;
            if (phase_step_row) local_tb += (uint16_t)row * phase_step_row;

            // 亮度三角波（含死區/縮放），沿用你現成的函式
            uint8_t b;
            calculate_breath_volume(mosArt.Speed, local_tb, &b);        // 0..255
            float m = (float)b / 255.0f;

            // 套用亮度到顏色
            led_config_t px;
            px.red   = (uint8_t)((float)baseColor.red   * m);
            px.green = (uint8_t)((float)baseColor.green * m);
            px.blue  = (uint8_t)((float)baseColor.blue  * m);

            setRemapLED(row, col, px); // 寫入 frame buffer（逐點） :contentReference[oaicite:1]{index=1}
        }
    }

    // 這種逐點寫法「不會」自動送出，需要手動喚醒更新緒
    chBSemSignal(&ledUpdateSem);  // 送畫面到驅動執行緒傳輸 :contentReference[oaicite:2]{index=2}

    mosArt.TimerBase += TIMEBASE_STEP; // 與原本 Mos Art 同樣的逐幀推進
    // return true;

}

bool LED_ModesBuilder(uint8_t action,bool start,uint8_t effect, bool enable, uint32_t interval, uint8_t colorMode, uint8_t mode, led_config_t color, uint8_t brightness, uint8_t speed, bool direction){
	switch (action){
		case	newDemo:
		case	addDemoEffect:
			switch (action){
				case newDemo:
				LED_DemoModeIndex = 0;
				break;
			case	addDemoEffect:
				if (LED_DemoModeIndex++>LED_DemoModes){
					return false;
				}
				break;
			}
			LED_DemoMode[LED_DemoModeIndex].effect = effect;
			LED_DemoMode[LED_DemoModeIndex].enable = enable;
			LED_DemoMode[LED_DemoModeIndex].interval = interval;
			LED_DemoMode[LED_DemoModeIndex].colorMode = colorMode;
			LED_DemoMode[LED_DemoModeIndex].mode = mode;
			LED_DemoMode[LED_DemoModeIndex].color = color;
			LED_DemoMode[LED_DemoModeIndex].brightness = brightness;
			LED_DemoMode[LED_DemoModeIndex].speed = speed;
			LED_DemoMode[LED_DemoModeIndex].direction = direction;
			if (start){
				chVTReset(&randColor_vt);
				ledAllOff(false);
				//chThdSleepMilliseconds(150);
				effectStatus.runMode = demoMode;
				LED_DemoCurrentMode = 0;
				chBSemSignal(&ledRunmodeSem);
			}
			break;
		case	newPreset:
		case	addPresetEffect:
			switch (action){
				case	newPreset:
				LED_PresetModeIndex = 0;
				break;
			case	addPresetEffect:
				if (LED_PresetModeIndex++>LED_PresetModeIndex){
					return false;
				}
			break;
			}
			LED_PresetMode[LED_PresetModeIndex].effect = effect;
			LED_PresetMode[LED_PresetModeIndex].enable = enable;
			LED_PresetMode[LED_PresetModeIndex].interval = 0;
			LED_PresetMode[LED_PresetModeIndex].colorMode = colorMode;
			LED_PresetMode[LED_PresetModeIndex].mode = mode;
			LED_PresetMode[LED_PresetModeIndex].color = color;
			LED_PresetMode[LED_PresetModeIndex].brightness = normalBrightness;
			LED_PresetMode[LED_PresetModeIndex].speed = normalSpeed;
			LED_PresetMode[LED_PresetModeIndex].direction = direction;
	}

	return false;
}


void buildDemoModes(void){
uint8_t currentBrightness = 5;
uint16_t currentSpeed = 4;
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
    LED_ModesBuilder(newPreset , false,EffectTypeMosArt, true, 5000, single, mosArtStatic,  rainbowColor[red], normalBrightness, normalSpeed, 0); // lyon change
    // LED_ModesBuilder(newPreset , false,EffectTypeMosArt, true, 5000, single, mosArtStatic,  {0,0,0}, normalBrightness, normalSpeed, 0); // lyon change

	// LED_ModesBuilder(addPresetEffect , false,EffectTypeMosArt, true, 5000, ranbow, mosArtBreath,  rainbowColor[green], normalBrightness, normalSpeed, 0);
    LED_ModesBuilder(addPresetEffect , false,EffectTypeMosArt, true, 5000, single, mosArtWave,  rainbowColor[offreal], normalBrightness, normalSpeed, 0);

	// LED_ModesBuilder(addPresetEffect , false,EffectTypeMosArt, true, 5000, single, mosArtFlash,  rainbowColor[blue], normalBrightness, normalSpeed, 0);
    LED_ModesBuilder(addPresetEffect , false,EffectTypeMosArt, true, 5000, single, mosArtWave,  rainbowColor[blue], normalBrightness, normalSpeed, true);

	LED_ModesBuilder(addPresetEffect , false,EffectTypeMosArt, true, 5000, ranbow, mosArtFlash,  rainbowColor[yellow], normalBrightness, normalSpeed, 0);
	LED_ModesBuilder(addPresetEffect , false,EffectTypeMosArt, true, 5000, single, mosArtStatic,  rainbowColor[white], normalBrightness, normalSpeed, 0);
	LED_ModesBuilder(addPresetEffect , false,EffectTypeMosArt, true, 5000, single, mosArtStatic,  rainbowColor[orange], normalBrightness, normalSpeed, 0);
	LED_ModesBuilder(addPresetEffect , false,EffectTypeMosArt, true, 5000, single, mosArtStatic,  rainbowColor[indigo], normalBrightness, normalSpeed, 0);
	LED_ModesBuilder(addPresetEffect , false,EffectTypeMosArt, true, 5000, single, mosArtStatic,  rainbowColor[violet], normalBrightness, normalSpeed, 0);
	LED_ModesBuilder(addPresetEffect , false,EffectTypeRainbowFlow, true, 5000, 0, 0,  LED_NULL, normalBrightness, normalSpeed, false);
	LED_ModesBuilder(addPresetEffect , false,EffectTypeRainbowFlow, true, 5000, 0, 0,  LED_NULL, normalBrightness, normalSpeed, true);
	LED_ModesBuilder(addPresetEffect, false, EffectTypeKeytest, true, 5000, 0, 0,  LED_NULL, normalBrightness, normalSpeed, true);
	LED_ModesBuilder(addPresetEffect , false,EffectTypeChangeProfile, true, 5000, 0, 0,  LED_NULL, normalBrightness, normalSpeed, true);



}

//bool setRunMode(uint8_t runMode, int8_t mode, uint8_t brightness, uint8_t speed){
bool setRunMode(led_runMode_t runMode){
	if (gameModeEnable){return false;}
	else {
		// uprintf ("set runmode:%d\r\n",runMode.type);
        // uprintf ("%d\r\n",runMode.type); //
	}
	chVTReset(&randColor_vt);

	switch (runMode.type){
		case	normalMode:
			ledAllOff(false);
			//chThdSleepMilliseconds(150);
			LED_PresetCurrentMode = runMode.mode;
			effectStatus.runMode = normalMode;
			if (runMode.mode>LED_PresetModeIndex){
				runMode.mode = 0;
			}
			if (runMode.mode<0){
				runMode.mode = LED_PresetModeIndex;
			}
			if (runMode.brightness>10){
				runMode.brightness = 10;
			}
            // if (runMode.brightness>5){ //lyon cut in half
			// 	runMode.brightness = 5;
			// }
			if (runMode.brightness < 1){
				runMode.brightness = 1;
			}
			if (runMode.speed>5){
				runMode.speed = 5;
			}
			if (runMode.speed < 1){
				runMode.speed = 1;
			}
			normalEffect = runMode.mode;
			normalBrightness = runMode.brightness;
			normalSpeed = runMode.speed;
			chBSemSignal(&ledRunmodeSem);
			break;
		case	demoMode:
			ledAllOff(false);
			LED_DemoCurrentMode = 0;
			effectStatus.runMode = demoMode;
			chBSemSignal(&ledRunmodeSem);
			break;
		case testMode:
			break;
		case dynamticLightingMode:
            // if(!g_usb_configured_flag){ // add this to bypass reboot auto setting
                effectStatus.effect = EffectIdle; //lyon eventually these two line cause reboot turn into dynamic mode and led_function
			    ledAllOff(false);
            // }
			// effectStatus.effect = EffectIdle;
			// ledAllOff(false);
			break;
		case offMode:
			effectStatus.effect = EffectIdle;
			ledAllOff(true);
			break;
		case powerOff:
			ledAllOff(true);
			break;
	}
	lastLED_RunMode = runMode;

	if (runMode.type==powerOff){
		palClearLine(SPI_LED_SDB_PIN);
		// uprintf ("led shutdown on\r\n");
	} else {
		palSetLine(SPI_LED_SDB_PIN);
		// uprintf ("led shutdown off\r\n");
	}
}

void LED_init(void){
	led_creatTask();

	buildDemoModes();
	buildNormalModes();
	chThdCreateStatic(waLED_EffectThread, sizeof(waLED_EffectThread), NORMALPRIO - 3, LED_EffectThread, NULL); //original -3 test +７
    currentLED_RunMode.mode = 8;
	currentLED_RunMode.type = normalMode;
    setRunMode(currentLED_RunMode);
	//leRemapLocationTest(500);
	//ledKeymatrixLocationTest(1);

	//ledAnalogMatrixzlocationTest();

	//setRunMode(demoMode, 0, 0, 0);
//	setRunMode(normalMode, 4, 5, 2);
//	chThdSleepMilliseconds(2000);
//	setRunMode(normalMode, ++normalEffect, normalBrightness+1, normalSpeed+1);
//	chThdSleepMilliseconds(1000);
////	setRunMode(offMode, 0, 0, 0);
////	chThdSleepMilliseconds(3000);
////	setRunMode(normalMode, 8, 10, 1);
////	chThdSleepMilliseconds(3000);
////	setRunMode(normalMode, ++normalEffect, --normalBrightness, ++normalBrightness);
////	chThdSleepMilliseconds(3000);
//	setRunMode(normalMode, ++normalEffect, normalBrightness, normalBrightness);
//	chThdSleepMilliseconds(3000);
//	setRunMode(normalMode, --normalEffect, normalBrightness, normalBrightness);
//	chThdSleepMilliseconds(3000);
//	*((uint32_t *)(0x2000FFF0)) = 0xDEADBEEF;
//	NVIC_SystemReset();


//	setRunMode(demoMode, 0, 0, 0);
////	chThdSleepMilliseconds(15000);
////	//setRunMode(normalMode, 8, 4, 1);
////
////	*((uint32_t *)(0x2000FFF0)) = 0xDEADBEEF;
////	NVIC_SystemReset();

	//setRunMode(demoMode, 0, 0, 0);



//	currentLED_RunMode.type = demoMode;
//	currentLED_RunMode.mode = 0;
//	currentLED_RunMode.brightness = 0;
//	currentLED_RunMode.speed = 0;
//	setRunMode(currentLED_RunMode);

//	currentLED_RunMode.type = offMode;
//	currentLED_RunMode.mode = 8;
//	currentLED_RunMode.brightness = 5;
//	currentLED_RunMode.speed = 1;
//	setRunMode(currentLED_RunMode);



//	chThdSleepMilliseconds(3000);
//	setRunMode(offMode, 0, 0, 0);
//	*((uint32_t *)(0x2000FFF0)) = 0xDEADBEEF;
//	NVIC_SystemReset();

}
