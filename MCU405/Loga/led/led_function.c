
/*
    Copyright (C) 2023 iOne Electrionc
*/

//#define LED_ARGB
#define LED_Driver

#include "hal.h"
#include "main.h"
#include "keymap_common.h"
#include "led_function.h"
#include "report.h"
#include "matrix.h"
#include "dynamic_macro.h"
#include "usb_main.h"
#include "../via/via.h"
#include "pin_defs.h"
#include "driver/LED_Driver.h"
#include <string.h>
#include "timer.h"
#include "keymap_in_eeprom.h"
#include "led_effect.h"
#include "loga_function.h" 
#include "chprintf.h"
#include "config.h"


//#include "driver/led_RGBTable.h"

#define NO_PRINT
//#define multiChannelARGB

#define LED_SPI_Port1					SPID1
#define LED_SPI_Port2					SPID2
#define LED_SPI_Port3					SPID3

#define ledArrayCounts					7

typedef void (*spicallback_t)(SPIDriver *spip);

//const uint8_t ledRows[] = {16, 17, 17, 13, 13, 11};
const uint8_t ledRows[] = {26, 16, 17, 17, 16, 15, 12};


//#define patternCheck

led_config_t LED_Datas[LedNumbers];


virtual_timer_t randColor_vt;
//analogKey_Config_t keyLocation;

extern EffectStatus_t effectStatus;
extern uint8_t LED_DemoCurrentMode;
extern report_lamp_array_t lamp_array_data_report;
extern led_runMode_t currentLED_RunMode,lastLED_RunMode;
extern const analogKey_Config_t analogKeyRelocate[7][16];
extern adcsample_t initPostionLevel[ADC_BUF_DEPTH][ADC_NUM_CHANNELS],samples2[ADC_BUF_DEPTH][ADC_NUM_CHANNELS], travelPostionLevel[ADC_BUF_DEPTH][ADC_NUM_CHANNELS];
extern dynamic_macro_t dynamic_macros[DYNAMIC_MACRO_COUNT];

extern const uint8_t tabLED_CTLRed[18];
extern const uint8_t tabLED_CTLGreen[18];
extern const uint8_t tabLED_CTLBlue[18];
extern const uint8_t tabLED_PWMCTLRed[40];
extern const uint8_t tabLED_PWMCTLGreen[40];
extern const uint8_t tabLED_PWMCTLBlue[40];
extern uint16_t keyMatrixDelta[MATRIX_ROWS][MATRIX_COLS];

extern uint8_t raw_report[32];
extern bool gameModeEnable;

rapidTrigger_config_t eeconfig_read_rapidTrigger(uint8_t key);
bool lampArrayDisable = false;
bool lampArrayEffect = false;

extern void rapidKeyIndicate(void);

binary_semaphore_t ledUpdateSem, ledUpdateingSem, ledRunmodeSem, ledDynamaticLightingSem,rawSendSem, ledUpdateingSem; // lyon add last one for fixing scan rate
extern binary_semaphore_t ledEffetSem;
mutex_t usbRecivedProcess;
extern led_config_t LED_NULL;
extern bool adcTestToggle;
//int8_t MultiUpdatelastLED = -1;
bool ledUpdatingLock = false;
int32_t lastLED_Update = -1;
extern uint16_t keyMatrixTriggerLevel, keyMatrixReleaseLevel;

extern const analogKey_Config_t matrixAnalogRelocate[6][15];

//uint32_t spi_timer,spi_timer_last;

extern void adcOffsetCalc(bool findMax,bool update,uint16_t samples);
extern void adcTravelCalc(uint8_t mode);
extern uint8_t dynamaticReciveBuff[LAMP_ARRAY_EPSIZE];

extern EffectStruct_t LED_DemoMode[LED_DemoModes], LED_PresetMode[LED_PresetModes];

extern matrix_row_t matrix[];
extern rapidTrigger_config_t rapidTriggerKeys[RAPID_TRIGGER_COUNT];

extern int8_t normalEffect;
extern uint8_t normalBrightness, normalSpeed;
extern uint8_t currentKeyMatrixMode;

extern bool keycodeOutput, rapidKeyEnable, keyScanEffect, globleTrigger, ledUpdateFilter;

//static CC_ALIGN_DATA(32) uint8_t patternBuffer[(LedNumbers - 1) * 12];
static CC_ALIGN_DATA(32) uint8_t ledDriverData[LED_DriverCount][PWM_FRAME_LENGTH+2];

static void spi_end1(SPIDriver *spip)
{
    (void)spip;
}

static void spi_end2(SPIDriver *spip)
{
    (void)spip;
}

static void spi_end3(SPIDriver *spip)
{
    (void)spip;
}

bool autonomousMode = true;

//const ledsPerRow[] = {LedNumbers, 21, 21, 16, 17, 13};

#ifdef multiChannelARGB

		static const uint8_t ledsPerRow[] = {
				14, 15, 15, 14, 13, 10, 28
		};

		static const uint8_t ARGB_Port1[] = {
				0, 4, 6
		};

		static const uint8_t ARGB_Port2[] = {
				2
		};

		static const uint8_t ARGB_Port3[] = {
				1, 3, 5
		};

		static const uint8_t ARGB_PortStart[] = {
				ARGB_Port1[0],
				ARGB_Port2[0],
				ARGB_Port3[0]
		};

		static uint8_t port1_Index = 0;
		static uint8_t port2_Index = 0;
		static uint8_t port3_Index = 0;

#endif
	const uint8_t ARGB_Port1[] = {0, 4, 6};
	const uint8_t ARGB_Port2[] = {2};
	const uint8_t ARGB_Port3[] = {1, 3, 5};
	const uint8_t ARGB_PortStart[] = {ARGB_Port1[0], ARGB_Port2[0], ARGB_Port3[0]};
	uint8_t port1_Index = 0, port2_Index = 0, port3_Index = 0;

//const uint8_t keyID_Remap[] = {
//			 0, 1, 2, 3, 4, 5, 6, 7, 8, 9,10,11,12,13,14,15,							//16 0-15
//			16,17,18,19,20,21,22,23,24,25,26,27,28,29,30,31,32,  						//17 16-32
//			33,34,35,36,37,38,39,40,41,42,43,44,45,46,47,48,49,							//17 33-49
//			50,51,52,53,54,55,56,57,58,59,60,61,62,										//13 50-62
//			63,64,65,66,67,68,69,70,71,72,73,74,75,										//13 63-75
//			76,77,78,79,80,81,82,83,84,85,86											//11 76-86
//};


const uint8_t keyID_Remap[] = {
		 0, 1, 2, 3, 4, 5, 6, 7, 8, 9,10,11,12,13,14,15,16,17,18,19,20,21,22,23,24,25,	//26 0-25
		26,27,28,29,30,31,32,33,34,35,36,37,38,39,40,41,   								//16 26,41
		42,43,44,45,46,47,48,49,50,51,52,53,54,55,56,57,58,								//17 42-58
		59,60,61,62,63,64,65,66,67,68,69,70,71,72,73,74,75,								//17 59,75
		76,77,78,79,80,81,82,83,84,85,86,87,88,89,90,91,								//16 76-91
		92,93,94,95,96,97,98,99,100,101,102,103,104,105,106,							//15 92-106
		107,108,109,110,111,112,113,114,115,116,117,118									//12 107-118
};




const uint8_t driverLocationRemap[] = {

};

/*
 * CRC32 configuration
 */
/*
static const CRCConfig crc32_config = {
  .poly_size         = 32,
  .poly              = 0x04C11DB7,
  .initial_val       = 0xFFFFFFFF,
  .final_val         = 0xFFFFFFFF,
  .reflect_data      = 1,
  .reflect_remainder = 1
};
*/
/*
 * CRC16 configuration
 */
static const CRCConfig crc16_config = {
  .poly_size         = 16,
  .poly              = 0x8005,
  .initial_val       = 0x0,
  .final_val         = 0x0,
  .reflect_data      = 1,
  .reflect_remainder = 1
};
extern const uint8_t locationMax[7];


typedef struct
 {
     SPIDriver *driver;
     ioportid_t port;
     uint8_t pad;
     uint8_t mux;
     uint8_t cr1;
     spicallback_t end_cb;

 }
 ledSPI_Channels_t;



 const ledSPI_Channels_t ledSPI_Channels [] = {
		 {
				 .driver = &LED_SPI_Port1,
				 .port = GPIOB,
				 .pad = 5,
				 .mux = 5,
				 .cr1 = 41U,
				 .end_cb = spi_end1
		 },
		 {
				 .driver = &LED_SPI_Port3,
				 .port = GPIOC,
				 .pad = 12,
				 .mux = 6,
				 .cr1 = 35U,
				 .end_cb = spi_end3
		 },
		 {
				 .driver = &LED_SPI_Port2,
				 .port = GPIOA,
				 .pad = 10,
				 .mux = 5,
				 .cr1 = 35U,
				 .end_cb = spi_end2
		 },
		 {
				 .driver = &LED_SPI_Port3,
				 .port = GPIOB,
				 .pad = 2,
				 .mux = 6,
				 .cr1 = 35U,
				 .end_cb = spi_end3
		 },
		 {
				 .driver = &LED_SPI_Port1,
				 .port = GPIOB,
				 .pad = 1,
				 .mux = 5,
				 .cr1 = 41U,
				 .end_cb = spi_end1
		 },
		 {
				 .driver = &LED_SPI_Port3,
				 .port = GPIOB,
				 .pad = 0,
				 .mux = 6,
				 .cr1 = 35U,
				 .end_cb = spi_end3
		 },
		 {
				 .driver = &LED_SPI_Port1,
				 .port = GPIOA,
				 .pad = 7,
				 .mux = 5,
				 .cr1 = 41U,
				 .end_cb = spi_end1
		 },

 };

 uint16_t spiCurrentLedCounts = 0;

uint64_t ledArrayChannel = 0;



 SPIConfig hs_spicfg[ledArrayCounts];


bool LED_Effect(uint8_t effect, bool enable, uint32_t interval, uint8_t colorMode, uint8_t mode, led_config_t color, uint8_t brightness, uint8_t speed, bool direction);

bool LED_FadoutEffect(uint8_t End, uint8_t speed)
{
    uint8_t maxValue, decValue;
    uint8_t* pointer;
    End = LedNumbers;
    pointer = (uint8_t*)LED_Datas;
    //Find Max value;
    //uprintf ("Find Max value:%d\r\n",pointer);

    for (uint16_t i = 0; i < End * 3; i++)
    {
        if (*pointer > maxValue)
        {
            maxValue = *pointer;
        }
        *pointer = 0x20;
        pointer++;
    }
    //uprintf ("MaxValue:%d\r\n",maxValue);
    if (maxValue == 0)
    {
        return false;
    }
    decValue = maxValue / 20;
    if (!decValue) { decValue++; }

    //uprintf ("decValue:%d\r\n",decValue);

    for (uint16_t i = 0; i < speed; i++)
    {
        for (uint16_t j = 0; j < End * 3; j++)
        {
            if (pointer[j] > decValue)
            {
                pointer[j] -= decValue;
            }
            else
            {
                pointer[j] = 0;
            }
            pointer[j] = 0x30;
        }
    }
    //brightAdj = true;
    return true;
}

//float PattenLimitCaculate(uint8_t End, uint8_t* data){
//	float adj = 1.0f;
//	uint32_t maxBright = End*3*255;
//	uint32_t ledBrightCount = 0;
//    if (ledBrightCount!=1.0f){
//		for (uint16_t x=0;x<(End*3);x++){
//
//			if (x%1 == 0){
//				ledBrightCount+= (float)(data[x])*1.25;
//			} else {
//				ledBrightCount+= data[x];
//			}
//		}
//
//
//		if (ledBrightCount>((float)maxBright*defaultBright)){
//			adj = defaultBright/((float)ledBrightCount / maxBright);
//		} else {
//			adj = 1.0f;
//		}
//    }
//    return adj;
//
//}

#ifdef LED_ARGB
void PattenConvert(uint8_t* data, uint8_t* buffer)
{
float adj = 1.0f;
uint8_t End = LedNumbers - 1;
uint32_t maxBright = End*3*255;
uint32_t ledBrightCount = 0;
uint32_t singleLED;

    if (End > LedNumbers)
    {
        End = LedNumbers;
    }
//    float adj = PattenLimitCaculate(End, data);
    if (ledBrightCount!=1.0f){
		for (uint16_t x=0;x<(End*3);x++){

			if (x%1 == 0){
				ledBrightCount+= (float)(*(data+x))*1.25;
			} else {
				ledBrightCount+= *(data+x);
			}
		}


		if (ledBrightCount>((float)maxBright*defaultBright)){
			adj = defaultBright/((float)ledBrightCount / maxBright);
		} else {
			adj = 1.0f;
		}
    }

    for (uint8_t i = 0; i < End; i++)
    {
        singleLED = (uint8_t)(adj * *data++);
        singleLED <<= 8;
        singleLED |= (uint8_t)(adj * *data++);
        singleLED <<= 8;
        singleLED |= (uint8_t)(adj * *data++);

        for (uint8_t j = 0; j < 12; j++)
        {
            if (singleLED)
            {
                if (singleLED & 0x0800000)
                {
                    *buffer = 0xe0;
                }
                else
                {
                    *buffer = 0x80;
                }
                if (singleLED & 0x0400000)
                {
                    *buffer++ |= 0x0e;
                }
                else
                {
                    *buffer++ |= 0x08;
                }
                singleLED <<= 2;
            }
            else
            {
                *buffer++ = 0x88;
            }
        }
    }
}

void spi_end1(void)
{
uint8_t start,length;
	chSysLockFromISR();
	palSetPadMode(PAL_PORT(ledSPI_Channels[ARGB_Port1[port1_Index]].port), ledSPI_Channels[ARGB_Port1[port1_Index]].pad, PAL_MODE_ALTERNATE(0)); /* SPI1_MOSI MODE:5 */
	port1_Index++;
	if (port1_Index < sizeof(ARGB_Port1)){
		start = 0;
		for (uint8_t j=0;j<ARGB_Port1[port1_Index];j++)
		{
			start += ledsPerRow[j];
		}
		palSetPadMode(PAL_PORT(ledSPI_Channels[ARGB_Port1[port1_Index]].port), ledSPI_Channels[ARGB_Port1[port1_Index]].pad, PAL_MODE_ALTERNATE(ledSPI_Channels[ARGB_Port1[port1_Index]].mux)); /* SPI1_MOSI MODE:5 */
		spiStart(ledSPI_Channels[ARGB_Port1[port1_Index]].driver, &hs_spicfg[ARGB_Port1[port1_Index]]); /* Setup transfer parameters.       */
		length = ledsPerRow[ARGB_Port1[port1_Index]];
		spiStartSend(ledSPI_Channels[ARGB_Port1[port1_Index]].driver, length * 12 , &patternBuffer[start * 12]);


	}else {
		if (port1_Index>=sizeof(ARGB_Port1) && port2_Index>=sizeof(ARGB_Port2) && port3_Index>=sizeof(ARGB_Port3))
		{
			chBSemSignalI(&ledUpdateingSem);
		}
	}
	chSysUnlockFromISR();
}


void spi_end2(void)
{
uint8_t start,length;
	chSysLockFromISR();
	palSetPadMode(PAL_PORT(ledSPI_Channels[ARGB_Port2[port2_Index]].port), ledSPI_Channels[ARGB_Port2[port2_Index]].pad, PAL_MODE_ALTERNATE(0)); /* SPI1_MOSI MODE:5 */
	port2_Index++;
	if (port2_Index < sizeof(ARGB_Port2)){
		start = 0;
		for (uint8_t j=0;j<ARGB_Port2[port2_Index];j++)
		{
			start += ledsPerRow[j];
		}
		palSetPadMode(PAL_PORT(ledSPI_Channels[ARGB_Port2[port2_Index]].port), ledSPI_Channels[ARGB_Port2[port2_Index]].pad, PAL_MODE_ALTERNATE(ledSPI_Channels[ARGB_Port2[port2_Index]].mux)); /* SPI1_MOSI MODE:5 */
		spiStart(ledSPI_Channels[ARGB_Port2[port2_Index]].driver, &hs_spicfg[ARGB_Port2[port2_Index]]); /* Setup transfer parameters.       */
		length = ledsPerRow[ARGB_Port2[port2_Index]];
		spiStartSend(ledSPI_Channels[ARGB_Port2[port2_Index]].driver, length * 12 , &patternBuffer[start * 12]);


	}else {
		if (port1_Index>=sizeof(ARGB_Port1) && port2_Index>=sizeof(ARGB_Port2) && port3_Index>=sizeof(ARGB_Port3))
		{
			chBSemSignalI(&ledUpdateingSem);
//			uprintf("end2\r\n");
		}
	}
	chSysUnlockFromISR();
}


void spi_end3(void)
{
uint8_t start,length;
	chSysLockFromISR();
	palSetPadMode(PAL_PORT(ledSPI_Channels[ARGB_Port3[port3_Index]].port), ledSPI_Channels[ARGB_Port3[port3_Index]].pad, PAL_MODE_ALTERNATE(0)); /* SPI1_MOSI MODE:5 */
	port3_Index++;
	if (port3_Index < sizeof(ARGB_Port3)){
		start = 0;
		for (uint8_t j=0;j<ARGB_Port3[port3_Index];j++)
		{
			start += ledsPerRow[j];
		}
		palSetPadMode(PAL_PORT(ledSPI_Channels[ARGB_Port3[port3_Index]].port), ledSPI_Channels[ARGB_Port3[port3_Index]].pad, PAL_MODE_ALTERNATE(ledSPI_Channels[ARGB_Port3[port3_Index]].mux)); /* SPI1_MOSI MODE:5 */
		spiStart(ledSPI_Channels[ARGB_Port3[port3_Index]].driver, &hs_spicfg[ARGB_Port3[port3_Index]]); /* Setup transfer parameters.       */
		length = ledsPerRow[ARGB_Port3[port3_Index]];
		spiStartSend(ledSPI_Channels[ARGB_Port3[port3_Index]].driver, length * 12 , &patternBuffer[start * 12]);


	}else {
		if (port1_Index>=sizeof(ARGB_Port1) && port2_Index>=sizeof(ARGB_Port2) && port3_Index>=sizeof(ARGB_Port3))
		{
			chBSemSignalI(&ledUpdateingSem);
//			uprintf("end3\r\n");
		}
	}
	chSysUnlockFromISR();
}





void led_ChannelSet(uint8_t channel){
	ledArrayChannel = channel;
    spiStart(ledSPI_Channels[ledArrayChannel].driver, &hs_spicfg[ledArrayChannel]); /* Setup transfer parameters.       */
}

#endif

#ifdef LED_Driver

void ledDriver_PatternConvert(uint8_t* data)
{
uint8_t postion = 0;
uint16_t dataPostion = 0;
//uint8_t brightness_limit = 120;

	for (uint8_t i=0; i< sizeof(ledRows); i++){
		for (uint8_t j=0; j<ledRows[i]; j++){
			#if 0 // lyon try to change the brightness
			uint8_t r = data[dataPostion++];
            uint8_t g = data[dataPostion++];
            uint8_t b = data[dataPostion++];

			// ��鈭桀漲
            if (r > brightness_limit) r = brightness_limit;
            if (g > brightness_limit) g = brightness_limit;
            if (b > brightness_limit) b = brightness_limit;
			
			ledDriverData[ledDriver_Remap[postion].chipNo][tabLED_PWMCTLRed[ledDriver_Remap[postion].postion]] = r;
			ledDriverData[ledDriver_Remap[postion].chipNo][tabLED_PWMCTLGreen[ledDriver_Remap[postion].postion]] = g;
			ledDriverData[ledDriver_Remap[postion].chipNo][tabLED_PWMCTLBlue[ledDriver_Remap[postion].postion]] = b;
			
			#endif
			//ledDriverData[ledDriver_Remap[postion].chipNo][tabLED_PWMCTLRed[ledDriver_Remap[postion].postion]] = (float)data[dataPostion++] *.7;		//white balance adj
			// ledDriverData[ledDriver_Remap[postion].chipNo][tabLED_PWMCTLRed[ledDriver_Remap[postion].postion]] = data[dataPostion++] * 0.45 ;//*1.1;
			// ledDriverData[ledDriver_Remap[postion].chipNo][tabLED_PWMCTLGreen[ledDriver_Remap[postion].postion]] = data[dataPostion++] ;//*2;
			// ledDriverData[ledDriver_Remap[postion].chipNo][tabLED_PWMCTLBlue[ledDriver_Remap[postion].postion]] = data[dataPostion++] * 0.45 ;//*1.1;

			ledDriverData[ledDriver_Remap[postion].chipNo][tabLED_PWMCTLRed[ledDriver_Remap[postion].postion]] = data[dataPostion++] * 0.45;
			ledDriverData[ledDriver_Remap[postion].chipNo][tabLED_PWMCTLGreen[ledDriver_Remap[postion].postion]] = data[dataPostion++];
			ledDriverData[ledDriver_Remap[postion].chipNo][tabLED_PWMCTLBlue[ledDriver_Remap[postion].postion]] = data[dataPostion++] * 0.45;
			
			    
			postion++;
		}
	}
	for (uint8_t i=0 ;i < LED_DriverCount; i++){
		SPI_UPDATE_NBYTE(i,&ledDriverData[i][0]);
	}
}


#endif


void led_init(uint8_t mode)
{
	(void)mode;
#ifdef LED_ARGB
	uprintf ("LED INIT Type:ARGB\r\n");
	ledArrayChannel = mode;
	for (uint8_t i=0;i<ledArrayCounts;i++){
		hs_spicfg[i].circular = false;
		hs_spicfg[i].slave = false;
		hs_spicfg[i].data_cb = ledSPI_Channels[i].end_cb;
		hs_spicfg[i].error_cb = NULL;
		hs_spicfg[i].ssport = PAL_PORT(ledSPI_Channels[i].port);
		hs_spicfg[i].sspad = ledSPI_Channels[i].pad;
		hs_spicfg[i].ctrl1 = ledSPI_Channels[i].cr1;			//41U
		hs_spicfg[i].ctrl2 = 0U;
	}
#endif



#ifdef LED_Driver
	uprintf ("LED INIT Type:Driver IC\r\n");
	ledDriver_init();
	for (uint8_t i = 0; i<LED_DriverCount; i++){
		ledDriverData[i][0] = 0x20 | SPI_FRAME_ONE_PAGE;
		ledDriverData[i][1] = PWM_FRAME_FIRST_ADDR;
	}


#endif

    crcAcquireUnit(&CRCD1);             /* Acquire ownership of the bus.    */
    crcStart(&CRCD1, &crc16_config);           /* Activate CRC driver              */
    crcReset(&CRCD1);
//    palSetPadMode(GPIOA, 8, PAL_MODE_OUTPUT_PUSHPULL);				//LED_PWR
//    palClearPad(GPIOA, 8);
}


static THD_WORKING_AREA(waLED_ModesThread, 1024);
static THD_FUNCTION(LED_ModesThread, arg)
{
    (void)arg;
    chRegSetThreadName("LED Modes");
    chBSemObjectInit(&ledRunmodeSem, true);
    while (true)
    {
        chBSemWait(&ledRunmodeSem);
		switch (effectStatus.runMode)
		{
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
			case demoMode:
				LED_Effect(LED_DemoMode[LED_DemoCurrentMode].effect,
						LED_DemoMode[LED_DemoCurrentMode].enable,
						LED_DemoMode[LED_DemoCurrentMode].interval,
						LED_DemoMode[LED_DemoCurrentMode].colorMode,
						LED_DemoMode[LED_DemoCurrentMode].mode,
						LED_DemoMode[LED_DemoCurrentMode].color,
						LED_DemoMode[LED_DemoCurrentMode].brightness,
						LED_DemoMode[LED_DemoCurrentMode].speed,
						LED_DemoMode[LED_DemoCurrentMode].direction);
				if (LED_DemoMode[LED_DemoCurrentMode + 1].effect)
				{
					LED_DemoCurrentMode++;
				}
				else
				{
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
chBSemObjectInit(&ledUpdateSem, true);
chBSemObjectInit(&ledUpdateingSem, true);
chBSemObjectInit(&rawSendSem, true);
	
chVTObjectInit(&randColor_vt);
#ifdef LED_ARGB
memset(patternBuffer, 0x00, sizeof(patternBuffer));
#endif

#ifdef LED_Driver
memset(ledDriverData, 0x00, sizeof(ledDriverData));
#endif
//chThdSleepMilliseconds(1000);
led_init(0);

#ifdef LED_ARGB
	uprintf ("LED Task Type:ARGB\r\n");
	while (true)
	{
		chBSemWait(&ledUpdateSem);
		PattenConvert((uint8_t*)LED_Datas, patternBuffer);
		port1_Index = 0;
		port2_Index = 0;
		port3_Index = 0;
		ledUpdatingLock = true;

		for (uint8_t i=0; i<sizeof(ARGB_PortStart);i++){
			start = 0;
			for (uint8_t j=0;j<ARGB_PortStart[i];j++)
			{
				start += ledsPerRow[j];
			}
			palSetPadMode(PAL_PORT(ledSPI_Channels[ARGB_PortStart[i]].port), ledSPI_Channels[ARGB_PortStart[i]].pad, PAL_MODE_ALTERNATE(ledSPI_Channels[ARGB_PortStart[i]].mux)); /* SPI1_MOSI MODE:5 */
			spiStart(ledSPI_Channels[ARGB_PortStart[i]].driver, &hs_spicfg[ARGB_PortStart[i]]); /* Setup transfer parameters.       */
			length = ledsPerRow[ARGB_PortStart[i]];
			spiStartSend(ledSPI_Channels[ARGB_PortStart[i]].driver, length * 12 , &patternBuffer[start * 12]);

		}
		chBSemWait(&ledUpdateingSem);
//		uprintf ("#1:%d %2:%d %3:%d\r\n",port1_Index,port2_Index,port3_Index);

	}
#endif

#ifdef LED_Driver
	uprintf ("LED Task Type:Driver IC\r\n");
	while (true)
	{
		while (1){
//			SPI_LevelShift_Test();
//
//			chBSemWait(&ledUpdateSem);
//
//			chBSemWait(&ledUpdateingSem);
			chBSemWait(&ledUpdateSem);
			ledDriver_PatternConvert((uint8_t*)LED_Datas);
		}

#endif
		if (lamp_array_data_report.updated == true){
			lamp_array_data_report.updated = false;
		}
		ledUpdatingLock = false;
	}

}


//static void adxl355SPIReadRegister(ADXL355Driver *devp, uint8_t reg, size_t n,
//                                   uint8_t* b) {
//  unsigned i;
//  devp->commtxp[0] = (reg << 1) | ADXL355_RW;
//  cacheBufferFlush(&devp->commtxp[0], sizeof devp->commtxp);
//  spiSelect(devp->config->spip);
//  spiSend(devp->config->spip, 1, devp->commtxp);
//  spiReceive(devp->config->spip, n, devp->commrxp);
//  spiUnselect(devp->config->spip);
//  cacheBufferInvalidate(&devp->commrxp[0], sizeof devp->commrxp);
//  for(i = 0; i < n; i++, b++) {
//    *b = devp->commrxp[i];
//  }
//}


void setLightRunMode(uint8_t mode)
{
    if (gameModeEnable)
    {
        return;
    }

    switch (mode)
    {
        case 0U:
            autonomousMode = false;
            ledUpdateFilter = true;
            currentLED_RunMode.type = dynamticLightingMode;
            lampArrayDisable = false;
            lampArrayEffect = true;
            (void)setRunMode(currentLED_RunMode);
            ledAllOff(true);
            break;

        case 1U:
            if ((timer_read() > 3000U) && !keyScanEffect)
            {
                autonomousMode = true;
                currentLED_RunMode.type = normalMode;
                ledUpdateFilter = true;
                (void)setRunMode(currentLED_RunMode);
                lampArrayEffect = false;
            }
            break;

        case 2U:
            autonomousMode = false;
            currentLED_RunMode.type = dynamticLightingMode;
            (void)setRunMode(currentLED_RunMode);
            ledAllOff(true);
            break;

        default:
            break;
    }
}


//void via_controlSet(uint8_t *data){
//	switch (data[0]){
//		case id_keyControl:
//			keycodeOutput = data[1];
//			break;
//		case id_rapidEnable:
//			rapidKeyEnable = data[1];
//			if (rapidKeyEnable){
//				currentLED_RunMode.type = offMode;
//				setRunMode(currentLED_RunMode);
//				rapidKeyIndicate();
//			} else {
//				currentLED_RunMode.type = normalMode;
//				setRunMode(currentLED_RunMode);
//			}
//
//			break;
//		case id_keyScanEffect:
//			keyScanEffect = data[1];
//			break;
//		case id_GloableTriggle:
//			globleTrigger = data[1];
//			IQS9320_enableIndividualThresholds(!globleTrigger);
//			uprintf ("trigger enable:%d\r\n",globleTrigger);
//			break;
//		case id_LED_Filter:
//			ledUpdateFilter = data[1];
//			break;
//		}
//		data[1] = keycodeOutput;
//		data[2] = rapidKeyEnable;
//		data[3] = keyScanEffect;
//		data[4] = globleTrigger;
//		data[5] = ledUpdateFilter;
//}

void via_controlSet(uint8_t *data){
	switch (data[0]){
		case id_keyControl:
			keycodeOutput = data[1];
			break;
		case id_rapidEnable:
			rapidKeyEnable = data[1];
			if (rapidKeyEnable){
				currentLED_RunMode.type = offMode;
				setRunMode(currentLED_RunMode);
				rapidKeyIndicate();
			} else {
				currentLED_RunMode.type = normalMode;
				setRunMode(currentLED_RunMode);
			}

			break;
		case id_keyScanEffect:
			keyScanEffect = data[1];
			break;
		case id_GloableTriggle:
			globleTrigger = data[1];
			break;
		case id_LED_Filter:
			ledUpdateFilter = data[1];
			break;
		}
		data[1] = keycodeOutput;
		data[2] = rapidKeyEnable;
		data[3] = keyScanEffect;
		data[4] = globleTrigger;
		data[5] = ledUpdateFilter;
}

void via_rapidValueSet(uint8_t *data){
	rapidTrigger_config_t rapidTriggerKey;
	//uint8_t *rapidKeyTemp;
	for (uint8_t i=0;i<15;i++){
		uprintf ("%02X ",data[i]);
	}
	uprintf ("\r\n");
	rapidTriggerKey.enable = data[1];
	rapidTriggerKey.row = data[2];
	rapidTriggerKey.col = data[3];
	rapidTriggerKey.counts = data[4];

	rapidTriggerKey.keycode[0] = data[5] | data[6]<<8;
	rapidTriggerKey.keycode[1] = data[7] | data[8]<<8;
	rapidTriggerKey.keycode[2] = data[9] | data[10]<<8;
	rapidTriggerKey.keycode[3] = data[11] | data[12]<<8;
	rapidTriggerKey.keycode[4] = data[13] | data[14]<<8;
	rapidTriggerKey.keycode[5] = data[15] | data[16]<<8;
	eeconfig_write_rapidTrigger(data[0],rapidTriggerKey);
	rapidTriggerProcess(true,0);
	if (rapidKeyEnable){rapidKeyIndicate();}
}

//USB Recived
static THD_WORKING_AREA(waLED_DynamaticLighting, 512);
static THD_FUNCTION(LED_DynamaticLighting, arg)
{
    (void)arg;
    //static bool ledStatus;
    //uint16_t mode, data;
		uint16_t data;
    keypos_t key;
    uint16_t keycode = 0,triggerLevel,releaseLevel;
    chRegSetThreadName("LED Dynamatic Lighting");
    chBSemObjectInit(&ledDynamaticLightingSem, true);
    osalMutexObjectInit(&usbRecivedProcess);
    rapidTrigger_config_t rapidTriggerKey;
    //uint8_t *rapidKeyTemp;
    int16_t i;

    while (true)
    {
		// chThdSleepMilliseconds(1);
		// chThdSleep(TIME_US2I(300));
		// uprintf("DLS\r\n");
        chBSemWait(&ledDynamaticLightingSem);
		// uprintf("reportID is %d, lampArrayDisable is %d, currentLED_RunMode.type is %d\r\n",lamp_array_data_report.reportID, lampArrayDisable, currentLED_RunMode.type);
        // uprintf("report ID: %d\r\n",lamp_array_data_report.reportID);
		switch (lamp_array_data_report.reportID)
        {
            case 1:
                break;
            case 2:
                break;
            case 3:
                break;
            case 4:
            	if (currentLED_RunMode.type != dynamticLightingMode && timer_read() > 1500){
					currentLED_RunMode.type = dynamticLightingMode;
					uprintf ("case4");
					setRunMode(currentLED_RunMode);
            	}
            	if (!lampArrayDisable){
					// currentLED_RunMode.type = dynamticLightingMode;
            		dynamaticLightingMultiUpdate(lamp_array_data_report.data);
            	}
                break;
            case 5:
				if (currentLED_RunMode.type != dynamticLightingMode && timer_read() > 1500){
					// currentLED_RunMode.type = dynamticLightingMode; //lyon eventually these two line cause reboot turn into dynamic mode and led_function
					currentLED_RunMode.type = normalMode; // last add
					setRunMode(currentLED_RunMode);
					// uprintf ("if 1\r\n");
				}
				if (!lampArrayDisable){
					// currentLED_RunMode.type = dynamticLightingMode; // last add
					dynamaticLightingRangeUpdate(lamp_array_data_report.data);
					// setLightRunMode(0x0);
					// uprintf ("if 22222222\r\n");
				}
			/*
                if (currentLED_RunMode.type != dynamticLightingMode && timer_read() > 1500 && lampArrayDisable){
					currentLED_RunMode.type = dynamticLightingMode; //lyon eventually these two line cause reboot turn into dynamic mode and led_function
					setRunMode(currentLED_RunMode);
					// setLightRunMode(lamp_array_data_report.data[0]);
            	}
				if (!lampArrayDisable){
					currentLED_RunMode.type = dynamticLightingMode;
					setRunMode(currentLED_RunMode);
            		dynamaticLightingRangeUpdate(lamp_array_data_report.data);
					uprintf("!lampArrayDisable\r\n");
            	}
				*/
				// if (lampArrayDisable){
				// 	currentLED_RunMode.type = normalMode; //lyon eventually these two line cause reboot turn into dynamic mode and led_function
				// 	setRunMode(currentLED_RunMode);
				// 	lampArrayDisable = false;
				// } else {
				// 	if(currentLED_RunMode.type == normalMode){
				// 		currentLED_RunMode.type = dynamticLightingMode; //lyon eventually these two line cause reboot turn into dynamic mode and led_function
				// 		setRunMode(currentLED_RunMode);
				// 	}
				// 	if(currentLED_RunMode.type == offMode){
				// 		currentLED_RunMode.type = normalMode; //lyon eventually these two line cause reboot turn into dynamic mode and led_function
				// 		setRunMode(currentLED_RunMode);
				// 		lampArrayDisable = true;
				// 	}
				// 	// if(currentLED_RunMode.type == dynamticLightingMode){
				// 		dynamaticLightingRangeUpdate(&lamp_array_data_report.data);
				// 	// }
				// }
                break;
            case 6:
            	setLightRunMode(lamp_array_data_report.data[0]);
				// if(lampArrayDisable = true){
				// 	lampArrayDisable = false;
				// }
                break;
            case 7:
                switch (lamp_array_data_report.data[0])
                {
                    case apiControl_LED_Mode:
                    	if (lamp_array_data_report.data[1]!=volSpdMode){
                            currentLED_RunMode.type = lamp_array_data_report.data[1];
                            currentLED_RunMode.mode = lamp_array_data_report.data[2];
                            currentLED_RunMode.brightness = lamp_array_data_report.data[3];
                            currentLED_RunMode.speed = lamp_array_data_report.data[4];
                            setRunMode(currentLED_RunMode);

                    	} else {
                            currentLED_RunMode.brightness = lamp_array_data_report.data[3];
                            currentLED_RunMode.speed = lamp_array_data_report.data[4];
                            LED_SpeedChange(currentLED_RunMode.brightness, currentLED_RunMode.speed);
                    	}
                        break;

                    case apiKeymatrix_GetControl:

                    	dynamaticReciveBuff[1] = 8;
                        dynamaticReciveBuff[2] = keycodeOutput;
                        dynamaticReciveBuff[3] = rapidKeyEnable;
                        dynamaticReciveBuff[4] = keyScanEffect;
                        dynamaticReciveBuff[5] = globleTrigger;
                        dynamaticReciveBuff[6] = ledUpdateFilter;
                    	break;
                    case apiKeymatrix_SetControl:
                    	//uprintf ("1:%X 2:%X 3:%X 4:%x\r\n",lamp_array_data_report.data[1],lamp_array_data_report.data[2],lamp_array_data_report.data[3],lamp_array_data_report.data[14]);
						switch (lamp_array_data_report.data[1]){
						case 0:
							keycodeOutput = lamp_array_data_report.data[2];
							break;
						case 1:
							rapidKeyEnable = lamp_array_data_report.data[2];
							if (rapidKeyEnable){
		    			        currentLED_RunMode.type = offMode;
		    			        setRunMode(currentLED_RunMode);
//		    		            chThdSleepMilliseconds(5);
								rapidKeyIndicate();
							} else {
		    			        currentLED_RunMode.type = normalMode;
		    			        setRunMode(currentLED_RunMode);
		    		            //chThdSleepMilliseconds(50);
							}

							break;
						case 2:
							keyScanEffect = lamp_array_data_report.data[2];
							break;
						case 3:
							globleTrigger = lamp_array_data_report.data[2];
							break;
						case 4:
							ledUpdateFilter = lamp_array_data_report.data[2];
							break;
						}
                    	dynamaticReciveBuff[1] = 8;
                        dynamaticReciveBuff[2] = keycodeOutput;
                        dynamaticReciveBuff[3] = rapidKeyEnable;
                        dynamaticReciveBuff[4] = keyScanEffect;
                        dynamaticReciveBuff[5] = globleTrigger;
                        dynamaticReciveBuff[6] = ledUpdateFilter;
                    	break;

					case apiKeymatrix_Init:
    			        currentLED_RunMode.type = offMode;
    			        setRunMode(currentLED_RunMode);
    		            chThdSleepMilliseconds(50);
    		            write_keymap_to_eeprom();
						//uprintf ("Keymatrix Init\r\n");
						NVIC_SystemReset();
						break;

					case apiFactory_Version:
					//				//uprintf ("Device ID:%04X\r\n",DEVICE_VER);
					break;

                    case apiFactory_Boot:
													currentLED_RunMode.type = offMode;
													setRunMode(currentLED_RunMode);
													chThdSleepMilliseconds(50);
                        switch (lamp_array_data_report.data[1])
                        {
                            case 0x00:
                                //uprintf("Enter Normal Boot\r\n");
                                NVIC_SystemReset();
                                break;
                            case 0x01:
                                //uprintf("Enter HID Bootloader\r\n");
                                	*((uint32_t *)(0x2000FFF0)) = 0xDEADBEEF;
                                	NVIC_SystemReset();
                                break;
                            case 0x02:
                                //uprintf("Enter DFU Bootloader\r\n");
                                *((uint32_t *)(0x2000FFF0)) = 0xE0351AE;
																NVIC_SystemReset();
                                break;
                        }
                        break;

                        case apiFactory_Reset:
                            switch (lamp_array_data_report.data[1])
                            {
                                case 0x00:								//All
                                    //uprintf("Factory Reset All\r\n");
                                    adcOffsetCalc(false, true, 1500);
                                    break;
                                case 0x01:
                                    //uprintf("Factory Reset but calc\r\n");	//All but offset calc
                                    break;
                                case 0x02:
                                    break;
                            }
														eeconfig_rapidTrigger_init();
														dynamic_macro_reset_eeprom();
														currentLED_RunMode.type = offMode;
														setRunMode(currentLED_RunMode);
#ifdef minTriggerCheck

                            keyMatrixTriggerLevel = 3;
														keyMatrixReleaseLevel = 0;
#else
                            keyMatrixTriggerLevel = DEFAULT_TRIGGER_LEVEL;
														keyMatrixReleaseLevel = DEFAULT_RELEASE_LEVEL;
#endif
														eeprom_write_word(EECONFIG_KEYMAP_TRIGGER_LEVEL,(uint16_t)keyMatrixTriggerLevel);

														eeprom_write_word(EECONFIG_KEYMAP_RELEASE_LEVEL,(uint16_t)keyMatrixReleaseLevel);

														write_keymap_to_eeprom();
														write_trigger_to_eeprom();
														write_Release_to_eeprom();
														
														uprintf(
																"[FACTORY RELEASE] RAM=%u EEPROM=%u\r\n",
																(unsigned int)
																		eeconfig_read_Release(5, 5),
																(unsigned int)
																		eeprom_read_word(
																				EECONFIG_RELEASE_LEVEL +
																						TRIGGER_OFFSET(5, 5)
																		)
														);

														chThdSleepMilliseconds(100);		
														eeconfig_rapidTrigger_init();
                            NVIC_SystemReset();

												break;

                    case apiReamp_Version:
                        //uprintf("Set Reamp Version No\r\n");
                        break;

                    case apiRemap_GetKeyValue:
                        //uprintf ("Get Key Value Layer:%2d Row:%2d Column:%2d\r\n" ,lamp_array_data_report.data[1],lamp_array_data_report.data[2],lamp_array_data_report.data[3]);
                        key.row = lamp_array_data_report.data[2];
                        key.col = lamp_array_data_report.data[3];
                        keycode = keymap_key_to_keycode(lamp_array_data_report.data[1], key);
                        triggerLevel = eeconfig_read_Trigger(key.row,key.col);
                        releaseLevel = eeconfig_read_Release(key.row,key.col);

                        dynamaticReciveBuff[3] = keycode >> 8;
                        dynamaticReciveBuff[4] = keycode & 0xff;
                        dynamaticReciveBuff[5] = triggerLevel >>8;
                        dynamaticReciveBuff[6] = triggerLevel & 0xff;
                        dynamaticReciveBuff[7] = releaseLevel >>8;
                        dynamaticReciveBuff[8] = releaseLevel & 0xff;

                        //uprintf ("Keycode:%4X 3:%x 4:%x\r\n", keycode,dynamaticReciveBuff[3],dynamaticReciveBuff[4]);
                        //uprintf ("row:%d col:%d trigger:%d release:%d\r\n",key.row,key.col,triggerLevel,releaseLevel);
                        break;

                    case apiRemap_GetKeyValues:
                        if (lamp_array_data_report.data[1] > 0x0b) { lamp_array_data_report.data[1] = 0x0b; }
                        for (i = 0; i < lamp_array_data_report.data[1]; i++)
                        {
                            key.row = lamp_array_data_report.data[i * 3 + 3];
                            key.col = lamp_array_data_report.data[i * 3 + 4];
                            keycode = keymap_key_to_keycode(lamp_array_data_report.data[i * 3 + 2], key);

                            dynamaticReciveBuff[i * 5 + 6] = keycode >> 8;
                            dynamaticReciveBuff[i * 5 + 7] = keycode & 0xff;

                            dynamaticReciveBuff[i * 5 + 3] = lamp_array_data_report.data[i * 3 + 2];
                            dynamaticReciveBuff[i * 5 + 4] = lamp_array_data_report.data[i * 3 + 3];
                            dynamaticReciveBuff[i * 5 + 5] = lamp_array_data_report.data[i * 3 + 4];
                        }
                        dynamaticReciveBuff[1] = lamp_array_data_report.data[1] * 5 + 4;                        //size
                        dynamaticReciveBuff[2] = lamp_array_data_report.data[1];                            //key counts
                        break;

                    case apiRemap_SetKeyValue:
                        //uprintf ("Get Key Value Layer:%2d Row:%2d Column:%2d\r\n" ,lamp_array_data_report.data[1],lamp_array_data_report.data[2],lamp_array_data_report.data[3]);
                        key.row = lamp_array_data_report.data[2];
                        key.col = lamp_array_data_report.data[3];
                        keycode = lamp_array_data_report.data[4] << 8 | lamp_array_data_report.data[5];
                        eeconfig_write_keymap_key(lamp_array_data_report.data[1], lamp_array_data_report.data[2], lamp_array_data_report.data[3], keycode);
                        //uprintf("Layer:%d Row:%d Col:%d Modifier:%4X keycode:%4X\r\n", lamp_array_data_report.data[1], lamp_array_data_report.data[2], lamp_array_data_report.data[3], keycode >> 8, keycode & 0xff);
                        //uprintf ("release level:%d\r\n",releaseLevel);
                        break;

                    case apiRemap_SetKeyValueTrigger:
                        //uprintf ("Get Key Value Layer:%2d Row:%2d Column:%2d\r\n" ,lamp_array_data_report.data[1],lamp_array_data_report.data[2],lamp_array_data_report.data[3]);
                        key.row = lamp_array_data_report.data[2];
                        key.col = lamp_array_data_report.data[3];
                        keycode = lamp_array_data_report.data[4] << 8 | lamp_array_data_report.data[5];
                        triggerLevel = lamp_array_data_report.data[6] << 8 | lamp_array_data_report.data[7];
                        releaseLevel = lamp_array_data_report.data[8] << 8 | lamp_array_data_report.data[9];
                        eeconfig_write_keymap_key(lamp_array_data_report.data[1], lamp_array_data_report.data[2], lamp_array_data_report.data[3], keycode);
                        eeconfig_write_Trigger(lamp_array_data_report.data[2], lamp_array_data_report.data[3],triggerLevel);
                        eeconfig_write_Release(lamp_array_data_report.data[2], lamp_array_data_report.data[3],releaseLevel);
                        //uprintf("Layer:%d Row:%d Col:%d Modifier:%4X keycode:%4X\r\n", lamp_array_data_report.data[1], lamp_array_data_report.data[2], lamp_array_data_report.data[3], keycode >> 8, keycode & 0xff);
                        //uprintf ("release level:%d\r\n",releaseLevel);
                        break;

                    case apiRemap_getLayerCount:
                    	break;

                    case apiRemap_Reset:
                        write_keymap_to_eeprom();
                        //uprintf("Keymap Reset!\r\n");
                        break;

			case apiMacro_Info:
                    	break;

                    case apiMacro_Read:
                    	//uprintf ("api macro read data0:%d data1:%d\r\n",lamp_array_data_report.data[1],lamp_array_data_report.data[2]);
                    	switch (lamp_array_data_report.data[1]){
                    	case 0:									//read macro length,checksum	[1]:slot
                    		//uprintf ("length:%d",dynamic_macros[lamp_array_data_report.data[2]].length);
                    		//uprintf ("\tchecksum:%04X\r\n",dynamic_macros[lamp_array_data_report.data[2]].checksum);
                    		dynamaticReciveBuff[1]= 7 ;
                    		dynamaticReciveBuff[2]= lamp_array_data_report.data[2] ;
                    		dynamaticReciveBuff[3] = dynamic_macros[lamp_array_data_report.data[2]].length;
                    		dynamaticReciveBuff[4] = dynamic_macros[lamp_array_data_report.data[2]].checksum>>8;
                    		dynamaticReciveBuff[5] = dynamic_macros[lamp_array_data_report.data[2]].checksum&0x00ff;
                    		break;
                    	case 1:										//read records				[2]:slot [3]:start [4]:count		07 87 01 01 00 05
                    		//s//uprintf ("start:%d count:%d\r\n",lamp_array_data_report.data[2],lamp_array_data_report.data[3]);
                    		//dynamic_macro_load_eeprom(lamp_array_data_report.data[2]);
                    		//memset(dynamaticReciveBuff,0x00,LAMP_ARRAY_EPSIZE);
                    		//for (uint8_t i=0;i<LAMP_ARRAY_EPSIZE;i++){dynamaticReciveBuff[i]=0;}
                    		dynamaticReciveBuff[2]= lamp_array_data_report.data[2];
                    		dynamaticReciveBuff[3]= lamp_array_data_report.data[3];
                    		dynamaticReciveBuff[4]= lamp_array_data_report.data[4];
                    		dynamaticReciveBuff[5]= dynamic_macros[lamp_array_data_report.data[2]].length;;						//length
                    		//uprintf ("\r\n==>Read length:%d\r\n",dynamic_macros[lamp_array_data_report.data[2]].length);

                    		for (uint8_t i=0;i<(lamp_array_data_report.data[4]-lamp_array_data_report.data[3]);i++){
                    			memcpy (&dynamaticReciveBuff[6+(i*8)],&dynamic_macros[lamp_array_data_report.data[2]].events[lamp_array_data_report.data[3]+i],8);
                    		}

                    		dynamaticReciveBuff[1]= (lamp_array_data_report.data[4]-lamp_array_data_report.data[3])*8 + 7 ;
                    		break;

                    	case 2:		//write
                    		switch (lamp_array_data_report.data[2]){				//mode				//0:macro 1:write 2:mode 3:slot 4:start 5:end;
                    		case 0:
                    			//dynamic_macros[lamp_array_data_report.data[3]].length = lamp_array_data_report.data[4];	//length
                        		for (uint8_t i=0;i<(lamp_array_data_report.data[5]-lamp_array_data_report.data[4]);i++){
                        			memcpy (&dynamic_macros[lamp_array_data_report.data[3]].events[lamp_array_data_report.data[4]+i],&dynamaticReciveBuff[6+(i*8)],8);
                        		}
                    			break;
                    			//
                    			// 07 87 02 00 00 07 0e 04 01 00 00 b8 00 00 00 05 01 01 00 32 00 00 00 05 01 00 00 64 00 00 00 06 01 01 00 32 00 00 00 06 01 00 00 64 00 00 00 07 01 01 00 32 00 00 00 07 01 00 00 64 00 00 00
                    		case 1:
                    			dynamic_macros[lamp_array_data_report.data[3]].length = lamp_array_data_report.data[4];
                    			//uprintf ("\r\n==>Writelength:%d\r\n",dynamic_macros[lamp_array_data_report.data[3]].length);
                    			dynamic_macro_record_end(lamp_array_data_report.data[3], true);
                    			break;
                    		}
                    		break;
                    	case 3:		//reset
                    		currentLED_RunMode.type = offMode;
							setRunMode(currentLED_RunMode);
                    		dynamic_macro_reset_eeprom();
                    		NVIC_SystemReset();
                    	}

                    	break;

                    case apiTrigger_Reset:
                    	write_trigger_to_eeprom();
                        write_Release_to_eeprom();
                    	//uprintf ("Trigger Reset level!\r\n");
                    	break;

                    case apiRapidTrigger_Reset:
                    	eeconfig_rapidTrigger_init();
                    	if (rapidKeyEnable){
                    		ledAllOff(true);
                    	}
                        //uprintf("RapidTrigger Reset!\r\n");
                    	break;

                    case apiTrigger_GetValue:
                    case apiRelease_GetValue:
                    	switch (lamp_array_data_report.data[0]){
                    		case apiTrigger_GetValue:
                    			triggerLevel = eeconfig_read_Trigger(lamp_array_data_report.data[1], lamp_array_data_report.data[2]);
                    		break;
                    		case apiRelease_GetValue:
                    			triggerLevel = eeconfig_read_Release(lamp_array_data_report.data[1], lamp_array_data_report.data[2]);
							break;

                    	}
                        //triggerLevel = eeconfig_read_Trigger(lamp_array_data_report.data[1], lamp_array_data_report.data[2]);
                        dynamaticReciveBuff[3] = triggerLevel >> 8;
                        dynamaticReciveBuff[4] = triggerLevel & 0xff;
                        //uprintf ("row:%d col:%d trigger:%d\r\n",lamp_array_data_report.data[1],lamp_array_data_report.data[2],triggerLevel);
                    	break;

											case apiRapidTrigger_GetValue:
											{
													rapidTriggerKey =
															eeconfig_read_rapidTrigger(lamp_array_data_report.data[1]);

													const uint8_t *rapidKeyTemp =
															(const uint8_t *)&rapidTriggerKey;

													dynamaticReciveBuff[3] =
															lamp_array_data_report.data[1];

													for (uint8_t i = 0;
															 i < sizeof(rapidTrigger_config_t);
															 i++) {
															dynamaticReciveBuff[4 + i] = rapidKeyTemp[i];
													}

													break;
											}

											case apiRapidTrigger_SetValue:
											{
													rapidTriggerKey.enable =
															lamp_array_data_report.data[2];

													rapidTriggerKey.row =
															lamp_array_data_report.data[3];

													rapidTriggerKey.col =
															lamp_array_data_report.data[4];

													rapidTriggerKey.counts =
															lamp_array_data_report.data[5];

													rapidTriggerKey.keycode[0] =
															(uint16_t)lamp_array_data_report.data[6] |
															((uint16_t)lamp_array_data_report.data[7] << 8);

													rapidTriggerKey.keycode[1] =
															(uint16_t)lamp_array_data_report.data[8] |
															((uint16_t)lamp_array_data_report.data[9] << 8);

													rapidTriggerKey.keycode[2] =
															(uint16_t)lamp_array_data_report.data[10] |
															((uint16_t)lamp_array_data_report.data[11] << 8);

													rapidTriggerKey.keycode[3] =
															(uint16_t)lamp_array_data_report.data[12] |
															((uint16_t)lamp_array_data_report.data[13] << 8);

													rapidTriggerKey.keycode[4] =
															(uint16_t)lamp_array_data_report.data[14] |
															((uint16_t)lamp_array_data_report.data[15] << 8);

													rapidTriggerKey.keycode[5] =
															(uint16_t)lamp_array_data_report.data[16] |
															((uint16_t)lamp_array_data_report.data[17] << 8);

													eeconfig_write_rapidTrigger(
															lamp_array_data_report.data[1],
															rapidTriggerKey
													);

													rapidTriggerProcess(true, 0);

													if (rapidKeyEnable) {
															rapidKeyIndicate();
													}

													dynamaticReciveBuff[3] =
															lamp_array_data_report.data[1];

													const uint8_t *rapidKeyTemp =
															(const uint8_t *)&rapidTriggerKey;

													for (uint8_t i = 0;
															 i < sizeof(rapidTrigger_config_t);
															 i++) {
															dynamaticReciveBuff[4 + i] = rapidKeyTemp[i];
													}

													break;
											}

                    case apiTrigger_SetValue:
                    case apiRelease_SetValue:
                    	triggerLevel = lamp_array_data_report.data[3] << 8 | lamp_array_data_report.data[4];
                    	switch (lamp_array_data_report.data[0]){
                    		case apiTrigger_SetValue:
                    			eeconfig_write_Trigger(lamp_array_data_report.data[1], lamp_array_data_report.data[2],triggerLevel);
                    		break;
                    		case apiRelease_SetValue:
                    			eeconfig_write_Release(lamp_array_data_report.data[1], lamp_array_data_report.data[2],triggerLevel);
							break;

                    	}
                        dynamaticReciveBuff[3] = triggerLevel >> 8;
                        dynamaticReciveBuff[4] = triggerLevel & 0xff;
                        //uprintf ("row:%d col:%d trigger:%d\r\n",lamp_array_data_report.data[1],lamp_array_data_report.data[2],triggerLevel);
                    	break;

                    case apiPostionRead:
                        dynamaticReciveBuff[3] = lamp_array_data_report.data[1];		//Mode                                                    //mode
                        dynamaticReciveBuff[4] = lamp_array_data_report.data[2];                                                    //row
                        dynamaticReciveBuff[5] = lamp_array_data_report.data[3];       	//row start                                             //col start
                        dynamaticReciveBuff[6] = lamp_array_data_report.data[4];		//row count                                                  //col end
                        analogKey_Config_t keyLocation;
                        uint8_t j = 0;
                        for (i = lamp_array_data_report.data[3]; i < (lamp_array_data_report.data[4] + 1); i++)
                        {
                            keyLocation = matrixAnalogRelocate[dynamaticReciveBuff[2]][i];
                            switch (dynamaticReciveBuff[3])
                            {
																case 0:
																{
																		uint16_t filtered_adc;

																		filtered_adc = matrix_get_filtered_adc(keyLocation.colA,keyLocation.rowA);

																		if (filtered_adc > initPostionLevel[keyLocation.colA][keyLocation.rowA])
																		{
																				keycode = filtered_adc - initPostionLevel[keyLocation.colA][keyLocation.rowA];
																		}
																		else
																		{
																				keycode = 0U;
																		}
																		break;
																}

																case 1:
																		keycode = matrix_get_filtered_adc(keyLocation.colA,keyLocation.rowA);
																		break;
                                case 2:

                                    keycode = initPostionLevel[keyLocation.colA][keyLocation.rowA];
                                    break;
                                case 3:
                                    keycode = travelPostionLevel[keyLocation.colA][keyLocation.rowA];
                                    break;
                                case 4:
                                    keycode = travelPostionLevel[keyLocation.colA][keyLocation.rowA] - initPostionLevel[keyLocation.colA][keyLocation.rowA];
                                    break;
                            }

                            dynamaticReciveBuff[7 + (j * 2)] = keycode >> 8;
                            dynamaticReciveBuff[8 + (j++ * 2)] = keycode & 0xff;
                            //					//uprintf ("row:%d col:%d calcValue=%04X\r\n",dynamaticReciveBuff[1],i,keycode);
                        }

                        dynamaticReciveBuff[1] = (lamp_array_data_report.data[4] - lamp_array_data_report.data[3] + 1) * 2 + 8;     //size
                        dynamaticReciveBuff[2] = lamp_array_data_report.data[4] - lamp_array_data_report.data[3] + 1;                   //counts
                                                                                                                                        //				//uprintf ("Get keymatrix Initial Postion cal row:%X %X %X %X \r\n",dynamaticReciveBuff[1],dynamaticReciveBuff[2],dynamaticReciveBuff[3],dynamaticReciveBuff[4],dynamaticReciveBuff[5],dynamaticReciveBuff[6]);

                        break;

                    case apiPostionSet:
                        switch (lamp_array_data_report.data[1])
                        {
                            case 0:
								// uprintf("adcOffsetCalc");
                            	//void adcOffsetCalc(bool findMax, bool update, uint16_t samples)
                                adcOffsetCalc(lamp_array_data_report.data[2], lamp_array_data_report.data[3], lamp_array_data_report.data[4]);
                                //uprintf("Offset level set Max:%d Update:%d Samples:%d\r\n", lamp_array_data_report.data[2], lamp_array_data_report.data[3], lamp_array_data_report.data[4]);
                                break;
                            case 1:
								// uprintf("adcTravelCalc");
                                adcTravelCalc(lamp_array_data_report.data[2]);
                                //uprintf("travel  level cal set\r\n");
                                break;
                            case 2:

                                switch (lamp_array_data_report.data[2])
                                {
                                    case 0:
                                        switch (lamp_array_data_report.data[3])
                                        {
                                            case 0:
												// uprintf("read all trigger\r\n");
                                                keyMatrixTriggerLevel = eeprom_read_word(EECONFIG_KEYMAP_TRIGGER_LEVEL);
                                                dynamaticReciveBuff[4] = (keyMatrixTriggerLevel >> 8) & 0xFF;
                                                dynamaticReciveBuff[5] = keyMatrixTriggerLevel & 0xff;
                                                uprintf("Read Trigger Level:%d\r\n", keyMatrixTriggerLevel);
                                                break;
                                            case 1:
												// uprintf("write all trigger\r\n");
                                                data = lamp_array_data_report.data[4] << 8 | lamp_array_data_report.data[5];
                                                uprintf("Write Trigger Level:%0x\r\n", data);
                                                eeprom_write_word(EECONFIG_KEYMAP_TRIGGER_LEVEL, data);
												for (uint8_t row = 0; row < MATRIX_ROWS; row++)
												{
													for (uint8_t col = 0; col < MATRIX_COLS; col++)
													{
														eeconfig_write_Trigger(row, col, data);
													}
												}
                                                keyMatrixTriggerLevel = eeprom_read_word(EECONFIG_KEYMAP_TRIGGER_LEVEL);
                                                break;
                                        }
                                        break;
                                    case 1:
                                        switch (lamp_array_data_report.data[3])
                                        {
											case 0:
												// uprintf("read all release"); //  瘝?
												keyMatrixReleaseLevel = eeprom_read_word(EECONFIG_KEYMAP_RELEASE_LEVEL);
												dynamaticReciveBuff[4] = (keyMatrixReleaseLevel >> 8) & 0xFF;
												dynamaticReciveBuff[5] = keyMatrixReleaseLevel & 0xff;
												uprintf("Read Release Level:%d\r\n", keyMatrixReleaseLevel);
												break;
											case 1:
												// uprintf("write all release"); //  ��神�read all trigger ��� less two bit
												data = lamp_array_data_report.data[4] << 8 | lamp_array_data_report.data[5];
												uprintf("Write Release Level:%0x\r\n", data);
												eeprom_write_word(EECONFIG_KEYMAP_RELEASE_LEVEL, data);
												for (uint8_t row = 0; row < MATRIX_ROWS; row++)
												{
													for (uint8_t col = 0; col < MATRIX_COLS; col++)
													{
														eeconfig_write_Release(row, col, data);
													}
												}
												keyMatrixReleaseLevel = eeprom_read_word(EECONFIG_KEYMAP_RELEASE_LEVEL);
												break;
                                        }
                                        break;
                                }
                                break;
							// case 3:
							// 	adc2mmCapture(lamp_array_data_report.data[2]);
							// 	break;
                        }
                        break;

                    case apiReserved_code:
                        //uprintf("Set api Reserved code");
                        break;
                }
                break;
            case 8:
                //uprintf("set report id:0x08\r\n");
                break;
            default:
                //uprintf("unknow rePortID:%02X\r\n", lamp_array_data_report.reportID);
                chBSemSignal(&ledDynamaticLightingSem);
        }
        //uprintf ("ID:%02X data[0]:%02X data[0]:%02X data[1]:%02X data[2]:%02X\r\n",lamp_array_data_report.reportID,lamp_array_data_report.data[0],lamp_array_data_report.data[1],lamp_array_data_report.data[2]);
    }
}

//bool raw_hid_receive_kb(uint8_t *data, uint8_t length){
//	uint8_t *command_id   = &(data[0]);
//	uint8_t *command_data = &(data[1]);
//	uint16_t value,triggerLevel,releaseLevel;
//	static sampleEnabled = false;;
//
//
//	switch (*command_id) {
//		case id_set_TMR_value:
//			switch (command_data[0]) {
//				case id_trigger_level:
//					value = command_data[1] << 8 | command_data[2];
//					eeprom_write_word(EECONFIG_KEYMAP_TRIGGER_LEVEL, value);
//					keyMatrixTriggerLevel = eeprom_read_byte(EECONFIG_KEYMAP_TRIGGER_LEVEL);
//					IQS9320_updateActivationThreshlod(keyMatrixTriggerLevel);
//				break;
//				case id_release_level:
//					value = command_data[1] << 8 | command_data[2];
//					eeprom_write_word(EECONFIG_KEYMAP_RELEASE_LEVEL, value);
//					keyMatrixReleaseLevel = eeprom_read_byte(EECONFIG_KEYMAP_RELEASE_LEVEL);
//				break;
//				case id_individual_trigger_level:
//				{
////					uint8_t data;
////					for (uint8_t i=0;i<5;i++){
////						uprintf ("%02X ",command_data[i]);
////					}
////					uprintf ("\r\n");
//					eeconfig_write_Trigger(command_data[1],command_data[2], command_data[3]);
//					//IQS9320_GetIndividualTrigger();
//				}
//					break;
//				case id_individual_release_level:
//					eeconfig_write_Release(command_data[1],command_data[2], command_data[3]);
//					break;
//				case id_individual_update:
//					IQS9320_GetIndividualTrigger(true);
//					break;
//				case id_individual_level_reset:
//                	write_trigger_to_eeprom();
//                    write_Release_to_eeprom();
//                    IQS9320_GetIndividualTrigger(true);
//                    break;
//				case id_rapid_trigger_value:
//                    via_rapidValueSet(&command_data[1]);
//                    break;
//				case id_rapid_trigger_reset:
//                	eeconfig_rapidTrigger_init();
//                	if (rapidKeyEnable){
//                		ledAllOff(true);
//                	}
//                	break;
//				case id_postion_value:
//					switch (command_data[1])
//					{
//						case 0:
//							adcOffsetCalc(command_data[2], command_data[3], command_data[4]);
//							break;
//						case 1:
//							//===>
////							adcTravelCalc(command_data[2]);
//							sampleEnabled = false;
//							IQS9320_Calibration_Complete();
//							break;
//					}
//					break;
//			}
//		break;
//
//		case id_get_TMR_value:
//			switch (command_data[0]) {
//				case id_trigger_level:
//                    keyMatrixTriggerLevel = eeprom_read_byte(EECONFIG_KEYMAP_TRIGGER_LEVEL);
//                    command_data[1] = keyMatrixTriggerLevel >> 8;
//                    command_data[2] = keyMatrixTriggerLevel & 0xff;
//
//				break;
//				case id_release_level:
//					keyMatrixReleaseLevel = eeprom_read_byte(EECONFIG_KEYMAP_RELEASE_LEVEL);
//					command_data[1] = keyMatrixReleaseLevel >> 8;
//					command_data[2] = keyMatrixReleaseLevel & 0xff;
//				break;
//
//				case id_calibration_levels: {
//					uint16_t row,col,count,index;
//					uint8_t *key_data = &(data[6]);
//					uint16_t keycode;
//					uint16_t test = 0;
//					analogKey_Config_t keyLocation;
//
//					if (command_data[1] == id_key_levels){
//						getKeysStatus(true);
//					}
//					if (command_data[1] == id_raw_levels){
//						IQS9320_Calibration(sampleEnabled);
//						sampleEnabled = true;
//					}
//
//
//					for (row = 0;row<sizeof(ledRows);row++){
//						command_data[2] = row;		//row
//						col = 0;
//						command_data[3] = col;		//col
//						index = 0;
//						for (col=0; col<ledRows[row];col++){
////							keyLocation = matrixAnalogRelocate[row][col];
//							switch (command_data[1])
//							{
//								case id_key_levels:				//key
////									keycode = samples2[keyLocation.colA][keyLocation.rowA] - initPostionLevel[keyLocation.colA][keyLocation.rowA];
////									if ((int16_t)keycode < 0) { keycode = 0; }
//									keycode = keyMatrixDelta[row][col];
//									break;
//								case id_raw_levels:				//Raw
////									keycode = samples2[keyLocation.colA][keyLocation.rowA];
//									keycode = keyMatrixDelta[row][col];
//									break;
//								case id_init_levels:				//Init
//									keycode = initPostionLevel[keyLocation.colA][keyLocation.rowA];
//									break;
//								case id_travel_levels:			//Travel
//									keycode = travelPostionLevel[keyLocation.colA][keyLocation.rowA];
//									break;
//								case id_step_levels:				//Steps
//									keycode = travelPostionLevel[keyLocation.colA][keyLocation.rowA] - initPostionLevel[keyLocation.colA][keyLocation.rowA];
//									break;
//								case id_individual_trigger_levels:
//									keycode = eeconfig_read_Trigger(row,col);
//									break;
//								case id_individual_release_levels:
//									keycode = eeconfig_read_Release(row,col);
//									break;
//
//							}
//
//							key_data[index*2] = keycode >>8;
//							key_data[index*2+1] = keycode & 0xff;
//							index ++;
//							if (index>=12 || col >= ledRows[row]-1){
//								command_data[4] = index;		//count
//								raw_hid_send(data, length);
//								chThdSleepMilliseconds(1);
//								for (uint8_t i=5;i<length;i++)
//								{
//									data[i]=0;
//								}
//								command_data[3] = col+1;
//								index = 0;
//							}
//						}
//					}
//
//						return false;
//				}
//				break;
//
//				case id_individual_trigger_level:
//					triggerLevel = eeconfig_read_Trigger(command_data[1],command_data[2]);
//					command_data[1] = triggerLevel >> 8;
//					command_data[2] = triggerLevel & 0xff;
//					break;
//				case id_individual_release_level:
//					releaseLevel = eeconfig_read_Release(command_data[1],command_data[2]);
//					command_data[1] = releaseLevel >> 8;
//					command_data[2] = releaseLevel & 0xff;
//					break;
//
//				}
//		break;
//		case 0xee:
//			break;
//
//	}
//	return true;
//}

bool raw_hid_receive_kb(uint8_t *data, uint8_t length){
	uint8_t *command_id   = &(data[0]);
	uint8_t *command_data = &(data[1]);
	uint16_t value,triggerLevel,releaseLevel;
	const uint8_t ledsPerRow[] = {16, 21, 21, 16, 17, 13};

	// uprintf("set-get for raw hid\r\n");
	switch (*command_id) {
		case id_set_TMR_value:
			switch (command_data[0]) {
				case id_trigger_level: //
					// uprintf("Set command_data[1]:%u, command_data[2]:%u\r\n",command_data[1],command_data[2]);//
					value = command_data[1] << 8 | command_data[2];
					eeprom_write_word(EECONFIG_KEYMAP_TRIGGER_LEVEL, value);
					keyMatrixReleaseLevel = eeprom_read_word(EECONFIG_KEYMAP_TRIGGER_LEVEL);
				break;
				case id_release_level:
					value = command_data[1] << 8 | command_data[2];
					eeprom_write_word(EECONFIG_KEYMAP_RELEASE_LEVEL, value);
					keyMatrixReleaseLevel = eeprom_read_word(EECONFIG_KEYMAP_RELEASE_LEVEL);
				break;
				case id_individual_trigger_level:
					eeconfig_write_Trigger(command_data[1],command_data[2], command_data[3] << 8 | command_data[4]);
					break;
				case id_individual_release_level:
					eeconfig_write_Release(command_data[1],command_data[2], command_data[3] << 8 | command_data[4]);
					break;
				case id_individual_level_reset:
                	write_trigger_to_eeprom();
                    write_Release_to_eeprom();
                    break;
				case id_rapid_trigger_value:
                    via_rapidValueSet(&command_data[1]);
                    break;
				case id_rapid_trigger_reset:
                	eeconfig_rapidTrigger_init();
                	if (rapidKeyEnable){
                		ledAllOff(true);
                	}
                	break;
				case id_postion_value:
					switch (command_data[1])
					{
						case 0:
							uprintf("adcOffsetCalc");
							adcOffsetCalc(command_data[2], command_data[3], command_data[4]);
							break;
						case 1:
							uprintf("adcTravelCalc");
							adcTravelCalc(command_data[2]);
							break;
						case 2:
							uprintf("adcTravelCalc");
							// adc2mmCapture(command_data[2]);
							break;
					}
					break;
			}
		break;

		case id_get_TMR_value:
			switch (command_data[0]) {
				case id_trigger_level:
                    keyMatrixTriggerLevel = eeprom_read_word(EECONFIG_KEYMAP_TRIGGER_LEVEL);
                    command_data[1] = keyMatrixTriggerLevel >> 8;
                    command_data[2] = keyMatrixTriggerLevel & 0xff;
					uprintf("Get command_data[1]:%u, command_data[2]:%u\r\n",command_data[1],command_data[2]);

				break;
				case id_release_level:
					keyMatrixReleaseLevel = eeprom_read_word(EECONFIG_KEYMAP_RELEASE_LEVEL);
					command_data[1] = keyMatrixReleaseLevel >> 8;
					command_data[2] = keyMatrixReleaseLevel & 0xff;
				break;

				case id_calibration_levels: {
					//uint8_t row,col,count,index;
					uint8_t row, col, index;
					uint8_t *key_data = &(data[6]);
					uint16_t keycode;
					analogKey_Config_t keyLocation;
					//uint16_t testData[sizeof(ledsPerRow)][21];
						for (row = 0;row<sizeof(ledsPerRow);row++){
							command_data[2] = row;		//row
							col = 0;
							command_data[3] = col;		//col
							index = 0;
							for (col=0; col<ledsPerRow[row];col++){
								keyLocation = matrixAnalogRelocate[row][col];
								switch (command_data[1])
								{//
									case id_key_levels:
									{
											uint16_t filtered_adc;

											filtered_adc = matrix_get_filtered_adc(keyLocation.colA,keyLocation.rowA);

											if (filtered_adc > initPostionLevel[keyLocation.colA][keyLocation.rowA])
											{
													keycode = filtered_adc - initPostionLevel[keyLocation.colA][keyLocation.rowA];
											}
											else
											{
													keycode = 0U;
											}
											break;
									}

									case id_raw_levels:
											keycode = matrix_get_filtered_adc(keyLocation.colA,keyLocation.rowA);
											break;
									case id_init_levels:				//Init
										keycode = initPostionLevel[keyLocation.colA][keyLocation.rowA];
										break;
									case id_travel_levels:			//Travel
										keycode = travelPostionLevel[keyLocation.colA][keyLocation.rowA];
										break;
									case id_step_levels:				//Steps
										keycode = travelPostionLevel[keyLocation.colA][keyLocation.rowA] - initPostionLevel[keyLocation.colA][keyLocation.rowA];
										break;
									case id_individual_trigger_levels:
										keycode = eeconfig_read_Trigger(row,col);
										break;
									case id_individual_release_levels:
										keycode = eeconfig_read_Release(row,col);
										break;

								}

								key_data[index*2] = keycode >>8;
								key_data[index*2+1] = keycode & 0xff;
								index ++;
								if (index>=12 || col >= ledsPerRow[row]-1){
									command_data[4] = index;		//count
									raw_hid_send(data, length);
									for (uint8_t i=5;i<length;i++)
									{
										data[i]=0;
									}
									command_data[3] = col+1;
									index = 0;
								}
							}
						}
						return false;
					}
				break;

				case id_individual_trigger_level:
					triggerLevel = eeconfig_read_Trigger(command_data[1],command_data[2]);
					command_data[1] = triggerLevel >> 8;
					command_data[2] = triggerLevel & 0xff;
					break;
				case id_individual_release_level:
					releaseLevel = eeconfig_read_Release(command_data[1],command_data[2]);
					command_data[1] = releaseLevel >> 8;
					command_data[2] = releaseLevel & 0xff;
					break;

				}
		break;

		case id_adc_gain_value:
			/*
			 * App protocol:
			 * request:  data[1]=operation (0: read, 1: write),
			 *           data[2]=port, data[3]=gain numerator (write only)
			 * response: data[1]=status, data[2]=port count,
			 *           data[3..9]=gain numerators for ports 0..6.
			 */
			if (command_data[0] == 1U) {
				command_data[0] = adc_gain_set(command_data[1], command_data[2]) ? 0U : 1U;
			}
			else if (command_data[0] == 0U) {
				command_data[0] = 0U;
			}
			else {
				command_data[0] = 2U;
			}

			command_data[1] = ADC_NUM_CHANNELS;
			for (uint8_t port = 0U; port < ADC_NUM_CHANNELS; port++) {
				command_data[2U + port] = adc_gain_get(port);
			}
		break;
		case 0xee:
			break;

	}
	return true;
}


void ledAllOff(bool mode)
{
		(void)mode;
    memset(LED_Datas, 0x00, sizeof(LED_Datas));
//	if (mode){
//		palClearLine(SPI_LED_SDB_PIN);
//		uprintf ("led shutdown on\r\n");
//	} else {
//		palSetLine(SPI_LED_SDB_PIN);
//	}
    chBSemSignal(&ledUpdateSem);
    //    }
}

void led_updateFrameBufferSingle(uint8_t start, uint8_t end, led_config_t* led)
{
    for (uint8_t i = start; i < end; i++)
    {
        LED_Datas[i] = *led;
    }
    chBSemSignal(&ledUpdateSem);
}

void dynamaticLightingRangeUpdate(uint8_t* lamp_array_data)
{
	//
    report_lamp_array_rangeUpdate ledRangeUpdate;
    led_config_t rangeLED;
    float intensity;
    uint8_t location;
    location = 1;
    ledRangeUpdate.start =
        (uint16_t)lamp_array_data[location] |
        ((uint16_t)lamp_array_data[location + 1U] << 8);
    location += 2U;

    ledRangeUpdate.end =
        (uint16_t)lamp_array_data[location] |
        ((uint16_t)lamp_array_data[location + 1U] << 8);
    location += 2U;
    intensity = (float)lamp_array_data[8] / 0xff;
    rangeLED.red = intensity * lamp_array_data[location++];
    rangeLED.green = intensity * lamp_array_data[location++];
    rangeLED.blue = intensity * lamp_array_data[location];
    for (uint16_t i = ledRangeUpdate.start; i <= ledRangeUpdate.end; i++)
    {
        LED_Datas[keyID_Remap[i]] = rangeLED;
    }
    chBSemSignal(&ledUpdateSem);
}

void dynamaticLightingMultiUpdate(uint8_t* lamp_array_data)
{
		(void)lamp_array_data;
    uint8_t lampID;
    float intensity;
    led_config_t LED;
    uint8_t location;
    for (uint8_t i = 0; i < 8; i++)
    {
        location = (i << 2) + 21;
        if (lamp_array_data_report.data[location])
        {
            intensity = (float)lamp_array_data_report.data[location--] / 0xff;
            LED.blue = intensity * lamp_array_data_report.data[location--];
            LED.green = intensity * lamp_array_data_report.data[location--];
            LED.red = intensity * lamp_array_data_report.data[location];
            location = (i << 1) + 2;
            uint16_t key_id =
                (uint16_t)lamp_array_data_report.data[location] |
                ((uint16_t)lamp_array_data_report.data[location + 1U] << 8);

            location += 2U;

            lampID = keyID_Remap[key_id];
            LED_Datas[lampID] = LED;
        }
    }
    chBSemSignal(&ledUpdateSem);
}

void eepromProcess_cb(void)
{
    chVTSetI(&randColor_vt, TIME_MS2I(effectStatus.speed), (vtfunc_t)led_cb, NULL);
}

void led_cb(void)
{
//    bool enable;
//    chSysLockFromISR();
    chBSemSignalI(&ledEffetSem);
//    switch (effectStatus.effect)
//    {
//    case EffectTypeRandColor:
//     enable = effectRandowColorProcess();
//    break;
//	case EffectTypeMosArt:
//    enable = effectMosArtProcess();
//    break;
//	case EffectTypeRainbowFlow:
//    enable = EffectTypeRainbowFlowProcess();
//    break;
//	case EffectTypeColorBrith:
//    enable = EffectTypeRainbowFlowProcess();
//    break;
//	case EffectTypeKeytest:
//	enable = EffectTypeKeytestProcess();
//	break;
//	case EffectTypeOff:
//    break;
//	case EffectIdle:
//    break;
//    default:
//    	chSysUnlockFromISR();
//	return;
//    }
//
//    if (enable)
//    {
//    	 chVTSetI(&randColor_vt, TIME_MS2I(effectStatus.speed), (vtfunc_t)led_cb, NULL);
//    }
//    chSysUnlockFromISR();
}


//
//bool setKeyMatrixLED(uint8_t row, uint8_t column, led_config_t color)
//{
//    int8_t number = ledKeyMatrix[row][column];;
//    if (number != -1)
//    {
//        LED_Datas[number] = color;
//    }
//    return true;
//}

bool setKeymapLED(uint8_t row, uint8_t column, led_config_t color)
{
    int16_t number;

    if (row >= MATRIX_ROWS || column >= MATRIX_COLS)
    {
        return false;
    }

    number = (int16_t)ledKeyMatrix[row][column];

    /*
     * -1 代表這個矩陣位置沒有對應 LED。
     * 有效 LED 編號必須是 0 ~ LedNumbers - 1。
     */
    if (number < 0 || number >= (int16_t)LedNumbers)
    {
        return false;
    }

    LED_Datas[number] = color;

    return true;
}

bool setRemapLED(uint8_t row, uint8_t column, led_config_t color)
{
    int8_t number;
//    const uint8_t LED_Remap[2][6]={ { 3, 55, 60, 62, 63, 95},{ 4, 56, 61, 64, 65, 96} };
    number = ledRemap[row][column];
    if (number > LedNumbers) { return false; }
    if (number != -1)
    {
        LED_Datas[number] = color;
        return true;

    }
    return false;
}



//void ledKeymatrixLocationTest(uint8_t count)
//{
//    uint8_t row, column;
//    while (count--)
//    {
//        for (row = 0; row < 6; row++)
//        {
//            for (column = 0; column < 21; column++)
//            {
//                if (setKeyMatrixLED(row, column, rainbowColor[row % 7]))
//                {
//                    chBSemSignal(&ledUpdateSem);
//                    chThdSleepMilliseconds(150);
//                    memset(LED_Datas, 0x00, sizeof(LED_Datas));
//                }
//            }
//        }
//    }
//}


//uint8_t analogBtnFill(uint8_t group, uint8_t location, led_config_t color, uint16_t bright)
//{
//    led_config_t EffectranbowColor;
//    analogKey_Config_t keyLocation = analogKeyRelocate[group][location];;
//    uint8_t currentStep = 0;
//    if (location > locationMax[group]) { return; }
//    for (uint8_t i=0;i<RAPID_TRIGGER_COUNT;i++){
//    		if (rapidTriggerKeys[i].row == keyLocation.rowA && rapidTriggerKeys[i].col == keyLocation.colA){
//    	    	if (rapidTriggerKeys[i].enable && rapidTriggerKeys[i].counts != 0){
//    			if (bright < rapidTriggerKeys[i].onLevel[0]){
//    				EffectranbowColor = LED_NULL;
//
//    			} else {
//					for (uint8_t j=0;j<rapidTriggerKeys[i].counts;j++){
//						if (bright> rapidTriggerKeys[i].onLevel[j] && bright< rapidTriggerKeys[i].onLevel[j+1]){
//							currentStep ++;
//							break;
//						}else {
//							if (bright>rapidTriggerKeys[i].onLevel[j+1]){currentStep ++;}
//						}
//					}
//					EffectranbowColor = rainbowColor[currentStep-1];
//    			}
//    			if (currentStep && currentStep != rapidTriggerKeys[i].lastStep){
//    				if (rapidTriggerKeys[i].lastKeycode != 0xffff){
//    					unregister_code(rapidTriggerKeys[i].lastKeycode);
//    					register_code(rapidTriggerKeys[i].keycode[currentStep-1]);
//
//    				} else {
//    					register_code(rapidTriggerKeys[i].keycode[currentStep-1]);
//    				}
//    				rapidTriggerKeys[i].lastKeycode = rapidTriggerKeys[i].keycode[currentStep-1];
//    			}
//    			if (currentStep == 0 && rapidTriggerKeys[i].lastStep != 0){
//					unregister_code(rapidTriggerKeys[i].lastKeycode);
//					rapidTriggerKeys[i].lastKeycode = 0xffff;
//					rapidTriggerKeys[i].lastStep = 0;
//    			}
//    			rapidTriggerKeys[i].lastStep = currentStep;
//    			setKeyMatrixLED(keyLocation.rowA, keyLocation.colA, EffectranbowColor);
//    			return 1;
//    		}
//
//    	}
//    }
//
//    float mult;
//    if (keyLocation.rowA == -1 || keyLocation.colA == -1) { return; }
//    EffectranbowColor = rainbowColor[group];
//    if (bright>600){
//    	mult = 1;
//    } else {
//    	mult = (float)bright / 600;         //OP*2	nonOP*10
//    }
//    EffectranbowColor.red *= mult;
//    EffectranbowColor.green *= mult;
//    EffectranbowColor.blue *= mult;
//    setKeyMatrixLED(keyLocation.rowA, keyLocation.colA, EffectranbowColor);
//	return 0U;
//}

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
    ledAllOff(true);
    chThdSleepMilliseconds(500);

}

uint32_t get_system_time_ticks(void)
{
    return (uint32_t)TIME_I2MS((uint32_t)chVTGetSystemTimeX());
}

void led_creatTask(void)
{
    chThdCreateStatic(waLED_ModesThread, sizeof(waLED_ModesThread), NORMALPRIO, LED_ModesThread, NULL);  //original 0 test + 10
    chThdCreateStatic(waLEDUpdateThread, sizeof(waLEDUpdateThread), NORMALPRIO - 2, LEDUpdateThread, NULL); //original -3 test + 7
    chThdCreateStatic(waLED_DynamaticLighting, sizeof(waLED_DynamaticLighting), NORMALPRIO + 3, LED_DynamaticLighting, NULL); //original +3 test + 13 //

}


#ifdef NO_PRINT

int uprintf(const char *fmt, ...)
{
    (void)fmt;
    return 0;
}

#else

int uprintf(const char *fmt, ...)
{
    va_list ap;
    int formatted_bytes;

    va_start(ap, fmt);
    formatted_bytes = chvprintf((BaseSequentialStream*)&SD3, fmt, ap);
    va_end(ap);

    return formatted_bytes;
}

#endif /* NO_PRINT */


void uart_init(void)
{
    static const SerialConfig default_config =
    {
    		// 384000, 0, USART_CTRL2_STOPBN1_BITS, 0
			115200, 0, USART_CTRL2_STOPBN1_BITS, 0 // lyon data smaller
    };
    palSetPadMode(GPIOC, 10, PAL_MODE_ALTERNATE(7)); /* USART3 TX */
    palSetPadMode(GPIOC, 11, PAL_MODE_ALTERNATE(7)); /* USART3 RX */
//    palSetPadMode(GPIOA, 9, PAL_MODE_ALTERNATE(7)); /* USART3 TX */
//    palSetPadMode(GPIOA, 10, PAL_MODE_ALTERNATE(7)); /* USART3 RX */
    sdStart(&SD3, &default_config);
}
