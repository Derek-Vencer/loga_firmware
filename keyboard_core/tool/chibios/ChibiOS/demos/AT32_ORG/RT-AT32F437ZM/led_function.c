
/*
    Copyright (C) 2023 iOne Electrionc
*/

# include "hal.h"
# include "main.h"
# include "led_function.h"


#define LED_SPI_Port					SPID1


led_config_t LED_Datas[LedNumbers]; //


virtual_timer_t randColor_vt;

extern EffectStatus_t effectStatus;
extern uint8_t LED_DemoCurrentMode;

binary_semaphore_t ledUpdateSem,ledRunmodeSem;

int32_t lastLED_Update = -1;

//uint32_t spi_timer,spi_timer_last;
uint32_t LED_UpdateCount = 0;

extern uint32_t get_system_time_ticks();

extern EffectStruct_t	LED_DemoMode[LED_DemoModes], LED_PresetMode[LED_PresetModes];

extern int8_t normalEffect;
extern uint8_t normalBrightness, normalSpeed;

bool brightAdj = true;


bool setRemapLED(uint8_t row, uint8_t column, led_config_t color);
bool LED_Effect(uint8_t effect, bool enable, uint32_t interval, uint8_t colorMode, uint8_t mode, led_config_t color, uint8_t brightness, uint8_t speed, bool direction);
uint32_t get_system_time_ticks(void);


bool LED_FadoutEffect(uint8_t End, uint8_t speed){
uint8_t maxValue,decValue;
uint8_t *pointer;
	End = LedNumbers;
	pointer = (uint8_t *)LED_Datas;
	//Find Max value;
	uprintf ("Find Max value:%d\r\n",pointer);

	for (uint16_t i=0;i<End*3;i++){
		if (*pointer>maxValue){
			maxValue = *pointer;
		}
		*pointer = 0x20;
		pointer++;
	}
	//uprintf ("MaxValue:%d\r\n",maxValue);
	if (maxValue == 0){
		return false;
	}
	decValue = maxValue / 20;
	if (!decValue){decValue++;}

	//uprintf ("decValue:%d\r\n",decValue);
	//brightAdj = false;
	for (uint16_t i=0;i<speed;i++){
		for (uint16_t j=0;j<End*3;j++){
			if (pointer[j]>decValue){
				pointer[j] -= decValue;
			} else {
				pointer[j] = 0;
			}
			pointer[j] = 0x30;
		}

		//chBSemSignal(&ledUpdateSem);
		//return true;
		//chThdSleepMilliseconds(25);
	}
	//brightAdj = true;
	return true;
}


bool PattenConvet(uint8_t End, uint8_t* data, uint8_t* buffer)
{
    uint8_t i, j;
    uint32_t singleLED;
    uint16_t brightTemp;
    static int32_t LED_Update;
    LED_Update = 0;
    if (End > LedNumbers)
    {
        End = LedNumbers;
    }
    for (i = 0; i < End; i++)
    {
        for (j = 0; j < 3; j++)
        {
            singleLED <<= 8;
            if (brightAdj){
            brightTemp = *data++ * defaultBright / 100;
            } else {
            	brightTemp = *data++;
            }
            singleLED |= brightTemp;
            LED_Update += (brightTemp << j);
        }

        for (j = 0; j < 12; j++)
        {
            if (singleLED & 0x0800000)
            {
                *buffer = 0xe0;
            }
            else
            {
                *buffer = 0x80;
            }
            singleLED <<= 1;
            if (singleLED & 0x0800000)
            {
                *buffer |= 0x0e;
            }
            else
            {
                *buffer |= 0x08;
            }
            singleLED <<= 1;
            buffer++;
        }
    }
    if (LED_Update == lastLED_Update)
    {
        lastLED_Update = LED_Update;
        //uprintf ("pass\t\tupdate:%d last:%d\r\n",LED_Update,lastLED_Update);
        return false;
    }
    lastLED_Update = LED_Update;
    //uprintf ("OK\tupdate:%d last:%d\r\n",LED_Update,lastLED_Update);
    return true;
}

void spi_end(void){
	LED_UpdateCount++;
}


void led_init(void)
{
    const SPIConfig hs_spicfg = {
	  .circular         = false,
	  .slave            = false,
	  .data_cb          = spi_end,
	  .error_cb         = NULL,
	  .ssport           = GPIOB,
	  .sspad            = 5U,
	  .cr1              = 36U,
	  .cr2              = 0U
    };
    spiStart(&LED_SPI_Port, &hs_spicfg); /* Setup transfer parameters.       */
    palSetLineMode(PAL_LINE(GPIOB, 5U), PAL_MODE_OUTPUT_PUSHPULL);
    palSetLineMode(PAL_LINE(GPIOA, 8U), PAL_MODE_OUTPUT_PUSHPULL);
    palSetLine(PAL_LINE(GPIOA, 8U));
    palClearLine(PAL_LINE(GPIOB, 5U));
	palSetPadMode(GPIOB, 5, PAL_MODE_ALTERNATE(5)); /* SPI1_MOSI */
}

static THD_WORKING_AREA(waLED_ModesThread, 128);
static THD_FUNCTION(LED_ModesThread, arg)
{
    (void)arg;
    chRegSetThreadName("LED Modes");
    chBSemObjectInit(&ledRunmodeSem, true);
    while (true)
    {
    	chBSemWait(&ledRunmodeSem);
    	//normalMode
    	switch (effectStatus.runMode){
    	case normalMode:
        	LED_Effect(LED_PresetMode[normalEffect].effect,
        			LED_PresetMode[normalEffect].enable,
					0,
					LED_PresetMode[normalEffect].colorMode,
					LED_PresetMode[normalEffect].mode,
					LED_PresetMode[normalEffect].color,
					normalBrightness,
					normalSpeed,
					LED_PresetMode[normalEffect].direction);
    		break;
    	case 	demoMode:
        	chThdSleepMilliseconds(150);
        	LED_Effect(LED_DemoMode[LED_DemoCurrentMode].effect,
        			LED_DemoMode[LED_DemoCurrentMode].enable,
    				LED_DemoMode[LED_DemoCurrentMode].interval,
    				LED_DemoMode[LED_DemoCurrentMode].colorMode,
    				LED_DemoMode[LED_DemoCurrentMode].mode,
    				LED_DemoMode[LED_DemoCurrentMode].color,
    				LED_DemoMode[LED_DemoCurrentMode].brightness,
    				LED_DemoMode[LED_DemoCurrentMode].speed,
    				LED_DemoMode[LED_DemoCurrentMode].direction);
        	if (LED_DemoMode[LED_DemoCurrentMode+1].effect){
        		LED_DemoCurrentMode++;
        	} else {
        		LED_DemoCurrentMode = 0;
        	}
    		break;
    	case testMode:
    		break;
    	case offMode:
    		break;
    	}
    }
}

static THD_WORKING_AREA(waLEDUpdateThread, 256);
static THD_FUNCTION(LEDUpdateThread, arg)
{
    (void)arg;
    chRegSetThreadName("LED Update");
    static CC_ALIGN_DATA(32) uint8_t patternBuffer[1296+120];
	chBSemObjectInit(&ledUpdateSem, true);
	chVTObjectInit(&randColor_vt);
	memset(patternBuffer, 0x00, sizeof(patternBuffer));
	led_init();
	while (true)
	{
		chBSemWait(&ledUpdateSem);
		if (PattenConvet(LedNumbers - 1, (uint8_t*)LED_Datas, patternBuffer+120))
		{
			//spiAcquireBus(&LED_SPI_Port);        /* Acquire ownership of the bus.    */
			//spi_timer = get_system_time_ticks();
			spiStartSend(&LED_SPI_Port, sizeof(patternBuffer), patternBuffer);
			//spi_timer_last = get_system_time_ticks()-spi_timer;
			//spiReleaseBus(&LED_SPI_Port);
		}
	}
}

void ledAllOff(void)
{
    memset(LED_Datas, 0x00, sizeof(LED_Datas));
    chBSemSignal(&ledUpdateSem);
}

void led_updateFrameBufferSingle(uint8_t start, uint8_t end, led_config_t* led)
{
    for (uint8_t i = start; i < end; i++)
    {
    	LED_Datas[i] = *led;
    }
    chBSemSignal(&ledUpdateSem);
}

void led_cb(void)
{
    bool enable;
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
        case EffectTypeOff:
            break;
        case EffectIdle:
            break;
        default:
            return;
    }

    if (enable)
    {
        chSysLockFromISR();
        chVTSetI(&randColor_vt, TIME_MS2I(effectStatus.speed), (vtfunc_t)led_cb, NULL);
        chSysUnlockFromISR();
    }
    //	else {
    //		chVTReset(&randColor_vt);
    //	}
}




bool setKeyMatrixLED(uint8_t row, uint8_t column, led_config_t color)
{
    int8_t number;
    const uint8_t LED_Remap[2][4]={ { 55,60,62,63},{ 56,61,64,65} };
    number = ledKeyMatrix[row][column];
    if (number > LedNumbers) { return false; }
    if (number != -1)
    {
        for (uint8_t i = 0; i < 4; i++)
        {
            if (LED_Remap[0][i] == number)
            {
                LED_Datas[LED_Remap[1][i]] = color;
            }
        }
        LED_Datas[number] = color;
        return true;

    }
    return false;
}

bool setRemapLED(uint8_t row, uint8_t column, led_config_t color)
{
    int8_t number;
    const uint8_t LED_Remap[2][6]={ { 3, 55, 60, 62, 63, 95},{ 4, 56, 61, 64, 65, 96} };
    number = ledRemap[row][column];
    if (number > LedNumbers) { return false; }
    if (number != -1)
    {
        for (uint8_t i = 0; i < 6; i++)
        {
            if (LED_Remap[0][i] == number)
            {
                LED_Datas[LED_Remap[1][i]] = color;
            }
        }
        LED_Datas[number] = color;
        return true;

    }
    return false;
}



void ledKeymatrixLocationTest(uint8_t count)
{
    uint8_t row, column;
    while (count--)
    {
        for (row = 0; row < 10; row++)
        {
            for (column = 0; column < 16; column++)
            {
                if (setKeyMatrixLED(row, column, rainbowColor[row % 7]))
                {
                    chBSemSignal(&ledUpdateSem);
                    chThdSleepMilliseconds(150);
                    memset(LED_Datas, 0x00, sizeof(LED_Datas));
                }
            }
        }
    }
}

void leRemapLocationTest(uint16_t count)
{
    uint16_t colorCount = 0;
    while (count--)
    {
        memset(LED_Datas, 0x00, sizeof(LED_Datas));
        for (uint8_t column = 0; column < 21; column++)
        {
            for (uint8_t row = 0; row < 6; row++)
            {
                setRemapLED(row, column, rainbowColor[(colorCount + column) % 7]);
            }
        }
        colorCount--;
        chBSemSignal(&ledUpdateSem);
        chThdSleepMilliseconds(200);
    }
    ledAllOff();
    chThdSleepMilliseconds(500);

}

uint32_t get_system_time_ticks(void) {
	return (uint32_t)TIME_I2MS((uint32_t)chVTGetSystemTimeX());
}

void led_creatTask(void){
    chThdCreateStatic(waLED_ModesThread, sizeof(waLED_ModesThread), NORMALPRIO + 1, LED_ModesThread, NULL);
    chThdCreateStatic(waLEDUpdateThread, sizeof(waLEDUpdateThread), NORMALPRIO + 2, LEDUpdateThread, NULL);
}

