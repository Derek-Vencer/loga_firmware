
/*
    Copyright (C) 2023 iOne Electrionc
*/

# include "hal.h"
# include "main.h"
# include "led_function.h"
# include "led_effect.h"

uint8_t ledUpdateInterval = 25;
static EffectrandonColor_t randColor;
static EffectMosArt_t mosArt;
static EffectRanbowFlow_t ranbowFlow;
EffectStatus_t effectStatus;
uint8_t LED_DemoModeIndex = 0,LED_DemoCurrentMode;
uint8_t LED_PresetModeIndex = 0,LED_PresetCurrentMode;
EffectStruct_t	LED_DemoMode[LED_DemoModes], LED_PresetMode[LED_PresetModes];
led_config_t LED_NULL;

int8_t normalEffect;
uint8_t normalBrightness = 3, normalSpeed = 2;


extern virtual_timer_t randColor_vt;
extern binary_semaphore_t ledUpdateSem,ledRunmodeSem;



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
                randColor.brightness = brightness;
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
                if (speed > 5 || (brightness < 1 || brightness > 10) || mode > mosArtFlash || colorMode > ranbow)
                {
                    return false;
                }
                mosArt.Mode = mode;
                mosArt.Rainbow = colorMode;
                mosArt.Speed = --speed;
                mosArt.Brightness = brightness;
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
                effectStatus.speed = speed * ledUpdateInterval;
                ranbowFlow.timebase = 700;
                ranbowFlow.enable = true;
                ranbowFlow.direction = direction;
                chVTSet(&randColor_vt, TIME_MS2I(effectStatus.speed), (vtfunc_t)led_cb, NULL);
            }
            else
            {
                ranbowFlow.enable = false;
            }
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
            ledAllOff();
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


bool effectRandowColorProcess(void)
{
    float brightTemp;
    led_config_t ledTemp;
    brightTemp = (float)(randColor.bright * randColor.brightness) / 1000;
    switch (randColor.colorMode)
    {
        case single:
            ledTemp.red = randColor.LED.red * brightTemp;
            ledTemp.green = randColor.LED.green * brightTemp;
            ledTemp.blue = randColor.LED.blue * brightTemp;
            break;
        case ranbow:
            ledTemp.red = rainbowColor[randColor.ranbowIndex].red * brightTemp;
            ledTemp.green = rainbowColor[randColor.ranbowIndex].green * brightTemp;
            ledTemp.blue = rainbowColor[randColor.ranbowIndex].blue * brightTemp;
            break;
    }
    led_updateFrameBufferSingle(0, LedNumbers, &ledTemp);

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
                    ledAllOff();
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
                ledAllOff();
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
            ledAllOff();
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
        }
        tempmBright = ((float)(mosArt.Brightness)) * bright / 2550;
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
        ledAllOff();
        effectStatus.effect = EffectIdle;
    }
    return false;
}

bool EffectTypeRainbowFlowProcess(void)
{
    uint8_t row, column;
    float tempmBright;
    if (ranbowFlow.enable)
    {
    	if (effectStatus.interval!=0 && (get_system_time_ticks()-effectStatus.timeStamp>effectStatus.interval)){
            chVTReset(&randColor_vt);
            ranbowFlow.enable = false;
            ledAllOff();
            effectStatus.effect = EffectIdle;
            if (effectStatus.runMode == demoMode){
            	chBSemSignal(&ledRunmodeSem);
            }
            return false;
        }
        tempmBright = (float)(ranbowFlow.Brightness) / 10;
        for (column = 0; column < 21; column++)
        {
            Calculate_Colorful_Color(0, ranbowFlow.timebase, &ranbowFlow.LED);
            if (ranbowFlow.direction)
            {
                ranbowFlow.timebase += 99;
            }
            else
            {
                ranbowFlow.timebase += 96;
            }
            ranbowFlow.LED.red *= tempmBright;
            ranbowFlow.LED.green *= tempmBright;
            ranbowFlow.LED.blue *= tempmBright;
            for (row = 0; row < 6; row++)
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
        ledAllOff();
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
				ledAllOff();
				chThdSleepMilliseconds(150);
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
uint16_t currentSpeed = 2;
	    LED_ModesBuilder(newDemo , false,EffectTypeRainbowFlow, true, 5000, 0, 0,  LED_NULL, currentBrightness, currentSpeed, false);
	    LED_ModesBuilder(addDemoEffect , false,EffectTypeRainbowFlow, true, 5000, 0, 0,  LED_NULL, currentBrightness, currentSpeed, true);
	    LED_ModesBuilder(addDemoEffect , false,EffectTypeMosArt, true, 5000, ranbow, mosArtStatic,  rainbowColor[green], currentBrightness, currentSpeed, 0);
	    LED_ModesBuilder(addDemoEffect , false,EffectTypeMosArt, true, 5000, single, mosArtStatic,  rainbowColor[green], currentBrightness, currentSpeed, 0);
	    LED_ModesBuilder(addDemoEffect , false,EffectTypeRandColor, true, 5000, ranbow, 0,  rainbowColor[green], currentBrightness, currentSpeed, 0);

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

	LED_ModesBuilder(newPreset , false,EffectTypeMosArt, true, 1500, single, mosArtStatic,  rainbowColor[red], normalBrightness, normalSpeed, 0);
	LED_ModesBuilder(addPresetEffect , false,EffectTypeMosArt, true, 0, single, mosArtStatic,  rainbowColor[green], normalBrightness, normalSpeed, 0);
	LED_ModesBuilder(addPresetEffect , false,EffectTypeMosArt, true, 0, single, mosArtStatic,  rainbowColor[blue], normalBrightness, normalSpeed, 0);
	LED_ModesBuilder(addPresetEffect , false,EffectTypeMosArt, true, 0, single, mosArtStatic,  rainbowColor[yellow], normalBrightness, normalSpeed, 0);
	LED_ModesBuilder(addPresetEffect , false,EffectTypeMosArt, true, 0, single, mosArtStatic,  rainbowColor[white], normalBrightness, normalSpeed, 0);
	LED_ModesBuilder(addPresetEffect , false,EffectTypeMosArt, true, 0, single, mosArtStatic,  rainbowColor[orange], normalBrightness, normalSpeed, 0);
	LED_ModesBuilder(addPresetEffect , false,EffectTypeMosArt, true, 0, single, mosArtStatic,  rainbowColor[indigo], normalBrightness, normalSpeed, 0);
	LED_ModesBuilder(addPresetEffect , false,EffectTypeMosArt, true, 0, single, mosArtStatic,  rainbowColor[violet], normalBrightness, normalSpeed, 0);
	LED_ModesBuilder(addPresetEffect , false,EffectTypeRainbowFlow, true, 5000, 0, 0,  LED_NULL, normalBrightness, normalSpeed, false);
	LED_ModesBuilder(addPresetEffect , false,EffectTypeRainbowFlow, true, 5000, 0, 0,  LED_NULL, normalBrightness, normalSpeed, true);


}

bool setRunMode(uint8_t runMode, int8_t mode, uint8_t brightness, uint8_t speed){
	switch (runMode){
		case	normalMode:
			chVTReset(&randColor_vt);
			ledAllOff();
			chThdSleepMilliseconds(150);
			LED_PresetCurrentMode = mode;
			effectStatus.runMode = normalMode;
			if (mode>LED_PresetModeIndex){
				mode = 0;
			}
			if (mode<0){
				mode = LED_PresetModeIndex;
			}
			if (brightness>10){
				brightness = 10;
			}
			if (brightness < 1){
				brightness = 1;
			}
			if (speed>5){
				speed = 5;
			}
			if (speed < 1){
				speed = 1;
			}
			normalEffect = mode;
			normalBrightness = brightness;
			normalSpeed = speed;
			chBSemSignal(&ledRunmodeSem);
			break;
		case	demoMode:
			chVTReset(&randColor_vt);
			ledAllOff();
			chThdSleepMilliseconds(150);
			LED_DemoCurrentMode = 0;
			effectStatus.runMode = demoMode;
			chBSemSignal(&ledRunmodeSem);
			break;
		case testMode:
			break;
		case offMode:
			chVTReset(&randColor_vt);
			effectStatus.effect = EffectIdle;
			ledAllOff();

			break;
	}
}

void LED_init(void)
{
    c.red   = eeprom_read_byte(KB_COLOR_RED_ADDR);
    c.green = eeprom_read_byte(KB_COLOR_GREEN_ADDR);
    c.blue  = eeprom_read_byte(KB_COLOR_BLUE_ADDR);

    uprintf(
        "[LED INIT] saved color R=%u G=%u B=%u\r\n",
        c.red,
        c.green,
        c.blue
    );

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

    currentLED_RunMode.mode =
        eeprom_read_byte(KB_COLOR_MODE_ADDR);

    currentLED_RunMode.brightness = normalBrightness;
    currentLED_RunMode.speed = normalSpeed;
    currentLED_RunMode.type = normalMode;

    setRunMode(currentLED_RunMode);
}