/*
 Copyright (C) 2023 iOne Electrionc
 */

#include "hal.h"
#include "led_driver.h"
#include "pin_defs.h"
#include "led_RGBTable.h"
#include "../led_function.h"
//#include "hal_spi_v2.h"

static void ledDriver_end(SPIDriver *spip);

SPIConfig ledDriver_spiConfig[LED_DriverCount];

uint8_t currentPort = 0, lastPort = 0xff;

//const uint32_t *endCB[LED_DriverCount]={
//		ledDriver_end1,
//		ledDriver_end2,
//		ledDriver_end3,
//};
const ioportid_t spiCS_Port[LED_DriverCount] = { SPI_LED_CS1_PORT,
		SPI_LED_CS2_PORT, SPI_LED_CS3_PORT, };

const uint_fast8_t spiCS_Pad[LED_DriverCount] = { SPI_LED_CS1_PAD,
		SPI_LED_CS2_PAD, SPI_LED_CS3_PAD };

//spicb_t endCB[LED_DriverCount] = {
//		ledDriver_end1,
//		ledDriver_end2,
//		ledDriver_end3,
//};

binary_semaphore_t ledDriver_Sem;
mutex_t ledWriteMutex;

uint8_t spi_buffer_TX[182]; // lyon 100 -> 182
uint8_t spi_buffer_RX[182];


//extern uint8_t ledDriverData[LED_DriverCount][PWM_FRAME_LENGTH+2];

void ledDriver_init(void) {
	uint8_t i;
	palSetLineMode(SPI_LED_SDB_PIN, PAL_MODE_OUTPUT_PUSHPULL);			//SDB
	palClearLine(SPI_LED_SDB_PIN);
	palSetLineMode(SPI_LED_CS1_PIN, PAL_MODE_OUTPUT_PUSHPULL);			//CS1
	palSetLineMode(SPI_LED_CS2_PIN, PAL_MODE_OUTPUT_PUSHPULL);			//CS2
	palSetLineMode(SPI_LED_CS3_PIN, PAL_MODE_OUTPUT_PUSHPULL);			//CS3

	palSetLineMode(SPI_LED_SCK_PIN, PAL_MODE_ALTERNATE(SPI_LED_SCK_PAL_MODE));//SCK
	palSetLineMode(SPI_LED_MOSI_PIN, PAL_MODE_ALTERNATE(SPI_LED_MOSI_PAL_MODE));//MOSI
	palSetLineMode(SPI_LED_MISO_PIN, PAL_MODE_ALTERNATE(SPI_LED_MISO_PAL_MODE));//MISO

	chBSemObjectInit(&ledDriver_Sem, true);
	chMtxObjectInit(&ledWriteMutex);
	__LED_SetShutDownPinHigh;

	chThdSleepMilliseconds(10);

	for (i = 0; i < LED_DriverCount; i++) {
		ledDriver_spiConfig[i].circular = false;
		ledDriver_spiConfig[i].slave = false;
		ledDriver_spiConfig[i].error_cb = NULL;
		ledDriver_spiConfig[i].ctrl1 = SPI_CTRL1_MDIV_64;		//1.5Mhz
		ledDriver_spiConfig[i].ctrl2 = 0U;
//		ledDriver_spiConfig[i].data_cb = endCB[i];
		ledDriver_spiConfig[i].data_cb = ledDriver_end;
		ledDriver_spiConfig[i].ssport = spiCS_Port[i];
		ledDriver_spiConfig[i].sspad = spiCS_Pad[i];

//		ledDriverData[i][0] = 0x20 | SPI_FRAME_ONE_PAGE;
//		ledDriverData[i][1] = PWM_FRAME_FIRST_ADDR;

		chThdSleepMilliseconds(20);
		LED_SledInit(i);
	}
}

static void ledDriver_end(SPIDriver *spip)
{
    (void)spip;

    spiUnselect(&SPI_LED_DRIVER);
    chBSemSignalI(&ledDriver_Sem);
}

void SPI_W_3BYTE(uint8_t port, uint8_t bPage, uint8_t bAddr, uint8_t bData) {
	spi_buffer_TX[2] = bData;
	SPI_W_NBYTE(port, bPage, bAddr, 1);
}

void SPI_W_NBYTE(uint8_t port, uint8_t bPage, uint8_t bStart_Addr, uint8_t blength) {
	chMtxLock(&ledWriteMutex);
	spi_buffer_TX[0] = (0x20 | bPage);
	spi_buffer_TX[1] = bStart_Addr;
	currentPort = port;
	if (lastPort != currentPort) {
		spiStart(&SPI_LED_DRIVER, &ledDriver_spiConfig[currentPort]);
		lastPort = port;
	}
	spiSelect(&SPI_LED_DRIVER);
	spiStartSend(&SPI_LED_DRIVER, blength + 2, spi_buffer_TX);

//Check buffer data
//	for (uint8_t j=0;j<=20+2;j++){
//		uprintf (" %02X",spi_buffer_TX[j]);
//	}
//	uprintf ("\r\n");

	chBSemWait(&ledDriver_Sem);
	chMtxUnlock(&ledWriteMutex);
}

//void SPI_UPDATE_NBYTE(uint8_t port, uint8_t *data)
//{
//	currentPort = port;
//	spi_buffer_TX[0] = (0x20|SPI_FRAME_ONE_PAGE);
//	spi_buffer_TX[1] = PWM_FRAME_FIRST_ADDR;
//
//	if (lastPort != currentPort ){
//		spiStart(&SPI_LED_DRIVER, &ledDriver_spiConfig[currentPort]);
//		lastPort = port;
//	}
//	spiSelect(&SPI_LED_DRIVER);
//	spiSend(&SPI_LED_DRIVER, 2, spi_buffer_TX);
//	spiStartSend(&SPI_LED_DRIVER, PWM_FRAME_LENGTH, data);
//	chBSemWait(&ledDriver_Sem);
//
//}

void SPI_UPDATE_NBYTE(uint8_t port, uint8_t *data) {
	currentPort = port;
//	uint8_t length;
	chMtxLock(&ledWriteMutex);
	if (lastPort != currentPort) {
		spiStart(&SPI_LED_DRIVER, &ledDriver_spiConfig[currentPort]);
		lastPort = port;
	}

	spiSelect(&SPI_LED_DRIVER);
//	switch (port){
//		case 0:
//			length = 40*3 +2;
//			break;
//		case 1:
//			length = 39*3 +2;
//			break;
//		case 2:
//			length = 26*3 +2;
//		break;
//	}
	spiStartSend(&SPI_LED_DRIVER, PWM_FRAME_LENGTH + 2, data);
	chBSemWait(&ledDriver_Sem);
	chMtxUnlock(&ledWriteMutex);

}

//uint8_t SPI_R_1BYTE(uint8_t port,uint8_t bPage, uint8_t bAddr){
//uint8_t data [3]LED_CurrentAdjuest
//uint8_t bData;
//	data [0] = 0xA0 | bPage;
//	data [1] = bAddr;
//	currentPort = port;
//	if (lastPort != currentPort ){
//		spiStart(&SPI_LED_DRIVER, &ledDriver_spiConfig[currentPort]);
//		lastPort = port;
//	}
//	spiSelect(&SPI_LED_DRIVER);
//	spiSend(&SPI_LED_DRIVER, 2, data);
//	spiReceive(&SPI_LED_DRIVER, 1, bData);
//	chBSemWait(&ledDriver_Sem);
//return bData;
//}

uint8_t SPI_R_1BYTE(uint8_t port, uint8_t bPage, uint8_t bAddr) {

	SPI_R_NBYTE(port, bPage, bAddr, 1);
	return spi_buffer_RX[0];
}

void SPI_R_NBYTE(uint8_t port, uint8_t bPage, uint8_t bStartAddr,	uint8_t blength) {
	uint8_t data[3];
	data[0] = 0xA0 | bPage;
	data[1] = bStartAddr;
	currentPort = port;
	chMtxLock(&ledWriteMutex);
	if (lastPort != currentPort) {
		spiStart(&SPI_LED_DRIVER, &ledDriver_spiConfig[currentPort]);
		lastPort = port;
	}
	spiSelect(&SPI_LED_DRIVER);
	spiSend(&SPI_LED_DRIVER, 2, data);
	spiReceive(&SPI_LED_DRIVER, blength, spi_buffer_RX);
	chBSemWait(&ledDriver_Sem);
	chMtxUnlock(&ledWriteMutex);
}

void SPI_LevelShift_Test(void) {

//	LED_RGB_Individual(1); //RGB(white) Single scan
	return;
	//SPI_R_NBYTE(0, 0x0B, 0x1B, 2, data);

//	spiStart(&SPI_LED_DRIVER, &ledDriver_spiConfig[currentPort]);

//	SPI_W_3BYTE(SPI_FRAME_FUNCTION_PAGE, 0x0F, 0x8F);
//	uint8_t current = 0x81;
	while (1) {
//		LED_RGB_Full(0); //RGB panel turn on
//		LED_RGB_Full(1); //RGB panel turn on
//		LED_RGB_Full(2); //RGB panel turn on

//		LED_SledInit(0);
//		LED_SledInit(1);
//		LED_SledInit(2);
//		LED_ClearFrame1Page(0);
//		LED_ClearFrame1Page(1);
//		LED_ClearFrame1Page(2);
//		LED_RGB_Individual(0); //RGB(white) Single scan
//		LED_RGB_Individual(1); //RGB(white) Single scan
//		LED_RGB_Individual(2); //RGB(white) Single scan
	}
}

const uint8_t tabLED_Vaf[36] = { //Reference LED Driver Datasheet  Map
		0x50, 0x55, 0x55, //CA1:	C1-A ~ C1-L
		0x00, 0x00, 0x00, //CA2:	C2-A ~ C2-L
		0x00, 0x00, 0x00, //CA3:	C3-A ~ C3-L
		0x15, 0x54, 0x55, //CA4:	C4-A ~ C4-L
		0x00, 0x00, 0x00, //CA5:	C5-A ~ C5-L
		0x00, 0x00, 0x00, //CA6:	C6-A ~ C6-L
		0x55, 0x05, 0x55, //CA7:	C7-A ~ C7-L
		0x00, 0x00, 0x00, //CA8:	C8-A ~ C8-L
		0x00, 0x00, 0x00, //CA9:	C9-A ~ C9-L
		0x55, 0x55, 0x41, //CA10:	C10-A ~ C10-L
		0x00, 0x00, 0x00, //CA11:	C11-A ~ C11-L
		0x00, 0x00, 0x00, //CA12:	C12-A ~ C12-L
	};

void LED_CurrentAdjuest(uint8_t current){
	if (current > 100) {
		current = 100;
	}

	uprintf ("current:%d\t",current);

	if (current == 0) {
		current = 0;
	} else {
		current = (uint8_t)(((uint16_t)current * 63u + 99u) / 100u);
		if (current == 0) {
			current = 1;
		}
	}

	current &= 0x3F;
	for (uint8_t i=0;i<LED_DriverCount;i++){
	SPI_W_3BYTE(i, SPI_FRAME_FUNCTION_PAGE, CURRENT_CTL_REG,(mskCURRENT_CTL_EN | current));
	}
	uprintf ("Bright level:%d\r\n", current);

}

uint8_t cacluate_fintuneCurrent (int8_t value){
	if  (value == 0){return 0;}
	if (value<0){
		value = (abs(value)-1) | 0x08;
	}
	return value<<4 | value;
}

void current_fineTune(int8_t adjRed, int8_t adjGreen,int8_t adjBlue){
	uint8_t valueRed = cacluate_fintuneCurrent(adjRed);
	uint8_t valueGreen = cacluate_fintuneCurrent(adjGreen);
	uint8_t valueBlue = cacluate_fintuneCurrent(adjBlue);
	uint8_t value = 0;
	
	// uprintf ("red:%02X green:%02X blue:%02X\r\n",
	//  cacluate_fintuneCurrentif (i/6(adjRed),
	//  cacluate_fintuneCurrent(adjGreen),
	//  cacluate_fintuneCurrent(adjBlue)
	//  );
        for (uint8_t i = 0; i < CURRENT_TUNE_LENGTH; i++) {
          switch ((i/6)%3){
          case 0:
        	  value = valueRed;
        	  break;
          case 1:
        	  value = valueGreen;
        	  break;
          case 2:
        	  value = valueBlue;
        	  break;
          }
//          uprintf ("%2X ",value);
          //data [i] = value;
          spi_buffer_TX[i+2] = value;
          if ((i+1)%6 == 0){
//        	  uprintf ("\r\n");
          }
        }
        for (uint8_t i=0;i<LED_DriverCount;i++){
        	SPI_W_NBYTE(i, SPI_FRAME_DETECTION_PAGE, CURRENT_TUNE_FIRST_ADDR, CURRENT_TUNE_LENGTH);
        	//        	SPI_W_NBYTE(i, 0x0c, 0x24, 0x48);
        	//Current finetune Read test
//        	SPI_R_NBYTE(i, SPI_FRAME_DETECTION_PAGE, CURRENT_TUNE_FIRST_ADDR, CURRENT_TUNE_LENGTH);
//			for (uint8_t i=0;i<CURRENT_TUNE_LENGTH;i++){
//				uprintf ("%02X ",spi_buffer_TX[i+2]);
//				if ((i+1)%6 == 0){uprintf("\r\n");}
//			}
//			uprintf("\r\n");


        }
        uprintf ("Red:%2d Green:%2d Blue:%2d\r\n",adjRed,adjGreen,adjBlue);

//    	uprintf ("\r\nBuffer tx:%02X %02x\r\n",spi_buffer_TX[0],spi_buffer_TX[1]);
//    	for (uint8_t i=0;i<CURRENT_TUNE_LENGTH;i++){
//    		uprintf ("%02X ",spi_buffer_TX[i+2]);
//    		if ((i+1)%6 == 0){uprintf("\r\n");}
//    	}
//    	uprintf("\r\n");



}


/*_____ F U N C T I O N S __________________________________________________*/

/*****************************************************************************
 * Function		: LED_Driver_Init
 * Description	: Set Initial Value for LED Driver ( Matrix)
 * Input			: None
 * Output		: None
 * Return		: None
 * Note			: None
 *****************************************************************************/
void LED_SledInit(uint8_t port) {
	//Setting LED Driver Ram Page to Function Page
	// System must go to SW shutdowm mode when initialization
	SPI_W_3BYTE(port, SPI_FRAME_FUNCTION_PAGE, SW_SHUT_DOWN_REG,
			mskSW_SHUT_DOWN_MODE);
	//Setting Matrix Type = 2
	SPI_W_3BYTE(port, SPI_FRAME_FUNCTION_PAGE, PICTURE_DISPLAY_REG,
			mskMATRIX_TYPE);
	//Setting Staggered Delay
	SPI_W_3BYTE(port, SPI_FRAME_FUNCTION_PAGE, STAGGERED_DELAY_REG,
			((mskSTD4 & CONST_STD_GROUP4) | (mskSTD3 & CONST_STD_GROUP3)
					| (mskSTD2 & CONST_STD_GROUP2)
					| (mskSTD1 & CONST_STD_GROUP1)));
	//Enable Slew Rate control
	SPI_W_3BYTE(port, SPI_FRAME_FUNCTION_PAGE, SLEW_RATE_CTL_REG,
			mskSLEW_RATE_CTL_EN);

	//===============================================================
	//VAF Control settings base on the LED type.
	//================================================================
	SPI_W_3BYTE(port, SPI_FRAME_FUNCTION_PAGE, VAF_CTL_REG,
			(mskVAF2 | mskVAF1));
	SPI_W_3BYTE(port, SPI_FRAME_FUNCTION_PAGE, VAF_CTL_REG2,
			(mskFORCEVAFCTL_VAFTIMECTL | (mskFORCEVAFTIME_CONST & 0x01)
					| mskVAF3));
	//================================================================

	//Setting LED driving current = 20mA and Enable current control
	SPI_W_3BYTE(port, SPI_FRAME_FUNCTION_PAGE, CURRENT_CTL_REG,
			(mskCURRENT_CTL_EN | CONST_CURRENT_STEP_21mA));

	//Init Frame1Page(Clear all Ram)
	LED_ClearFrame1Page(port);

	//======================================================//
	//Init  FrameVAFPage : Single R/G/B ,              //
	// RGB or Cathode RGB have different VAF settings  //
	//which can choice by " VAF_OPTION ".             //
	//======================================================//
	for (uint16_t i = 0; i < VAF_FRAME_LENGTH; i++) {
		spi_buffer_TX[i + 2] = tabLED_Vaf[i];
	}
	SPI_W_NBYTE(port, SPI_FRAME_LED_VAF_PAGE, VAF_FRAME_FIRST_ADDR,
			VAF_FRAME_LENGTH);

	//After initialization , system back to SW Normal mode.
	SPI_W_3BYTE(port, SPI_FRAME_FUNCTION_PAGE, SW_SHUT_DOWN_REG,
			mskSW_NORMAL_MODE);

	SPI_W_NBYTE(port, SPI_FRAME_ONE_PAGE, LED_FRAME_FIRST_ADDR,
			LED_FRAME_LENGTH);

	//=== =============== 2nd init ====================

	for (uint8_t i = 0; i < LED_FRAME_LENGTH; i++) {
		spi_buffer_TX[i + 2] = (tabLED_CTLRed[i] | tabLED_CTLGreen[i]
				| tabLED_CTLBlue[i]);
	}
	SPI_W_NBYTE(port, SPI_FRAME_ONE_PAGE, LED_FRAME_FIRST_ADDR,
				LED_FRAME_LENGTH);

	for (uint8_t i = 0; i < PWM_FRAME_LENGTH; i++) {
		spi_buffer_TX[i + 2] = PWM_FRAME_CLR_DATA;
	}

	SPI_W_NBYTE(port, SPI_FRAME_ONE_PAGE, PWM_FRAME_FIRST_ADDR,
				PWM_FRAME_LENGTH);
}

/*****************************************************************************
 * Function		: LED_RGB_Full
 * Description	: RGB LED Turn On
 * Input			: None
 * Output		: None
 * Return		: None
 * Note			: None
 *****************************************************************************/
//void LED_RGB_Full(uint8_t port)
//{
//
//	uint32_t i;
//
//	//=====================================//
//	//Clear LED CTL Registers (Frame1Page) //
//	//=====================================//
//	for( i = 0; i< LED_FRAME_LENGTH ; i++)
//	{
//		spi_buffer_TX[i+2] = LED_FRAME_CLR_DATA;
//	}
//	SPI_W_NBYTE(port, SPI_FRAME_ONE_PAGE, LED_FRAME_FIRST_ADDR, LED_FRAME_LENGTH);
//
//	//=========================================================//
//	//SET PWM CTL Registers Value =0xFF(Max Value, Frame1Page) //
//	//=========================================================//
//	for( i = 0; i< PWM_FRAME_LENGTH ; i++)
//	{
//		spi_buffer_TX[i+2] = PWM_FRAME_MAX_DATA;
//	}
//	SPI_W_NBYTE(port, SPI_FRAME_ONE_PAGE, PWM_FRAME_FIRST_ADDR, PWM_FRAME_LENGTH);
//
//	//=======================================//
//	//Start Function1 light on RED LED Flow  //
//	//=======================================//
//	//Write Frame1Page LED CTL Register:
//	//mskLED_FRAME_REG_ADDR means Frame1Page Location address ,
//	//mskLED_FRAME_REG_DATA means each bits mapping to one LED.
//
//	//=======================================//
//	//			Turn On Red LED Panel					   //
//	//=======================================//
//	for( i = 0; i< LED_FRAME_LENGTH ; i++)
//	{
//		spi_buffer_TX[i+2] = tabLED_CTLRed[i];
//	}
//	SPI_W_NBYTE(port, SPI_FRAME_ONE_PAGE, LED_FRAME_FIRST_ADDR, LED_FRAME_LENGTH);
//
//	//=========================//
//	//			Delay for 3s		   //
//	//=========================//
//
//	chThdSleepMilliseconds(1000);
//
//	//=======================================//
//	//			Turn On Green LED Panel				   //
//	//=======================================//
//	for( i = 0; i< LED_FRAME_LENGTH ; i++)
//	{
//		spi_buffer_TX[i+2] = tabLED_CTLGreen[i];
//	}
//	SPI_W_NBYTE(port, SPI_FRAME_ONE_PAGE, LED_FRAME_FIRST_ADDR, LED_FRAME_LENGTH);
//	//=========================//
//	//			Delay for 3s		   //
//	//=========================//
//	chThdSleepMilliseconds(1000);
//
//	//=======================================//
//	//			Turn On Blue LED Panel				   //
//	//=======================================//
//	for( i = 0; i< LED_FRAME_LENGTH ; i++)
//	{
//		spi_buffer_TX[i+2] = tabLED_CTLBlue[i];
//	}
//	SPI_W_NBYTE(port, SPI_FRAME_ONE_PAGE, LED_FRAME_FIRST_ADDR, LED_FRAME_LENGTH);
//	//=========================//
//	//			Delay for 3s		   //
//	//=========================//
//	chThdSleepMilliseconds(1000);
//
//	//=======================================//
//	//			Turn On RGB LED Panel				   //
//	//=======================================//
//	for( i = 0; i< LED_FRAME_LENGTH ; i++)
//	{
//		spi_buffer_TX[i+2] = (tabLED_CTLRed[i]|tabLED_CTLGreen[i]|tabLED_CTLBlue[i]);
//	}
//	SPI_W_NBYTE(port, SPI_FRAME_ONE_PAGE, LED_FRAME_FIRST_ADDR, LED_FRAME_LENGTH);
//	//=========================//
//	//			Delay for 3s		   //
//	//=========================//
//	chThdSleepMilliseconds(1000);
//
//}

/*****************************************************************************
 * Function		: LED_RGB_Individual : Single Scan
 * Description	: RGB LED move on the Panel
 * Input			: None
 * Output		: None
 * Return		: None
 * Note			: None
 *****************************************************************************/
//void LED_RGB_Individual(uint8_t port)
//{
//	uint32_t i;
//
//	//=====================================//
//	//Clear LED CTL Registers (Frame1Page) //
//	//=====================================//
//	for( i = 0; i< LED_FRAME_LENGTH ; i++)
//	{
//		spi_buffer_TX[i+2] = LED_FRAME_MAX_DATA;
//	}
//	SPI_W_NBYTE(port, SPI_FRAME_ONE_PAGE, LED_FRAME_FIRST_ADDR, LED_FRAME_LENGTH);
//
//	//=========================================================//
//	//SET PWM CTL Registers Value =0x00(Frame1Page) //
//	//=========================================================//
//	for( i = 0; i< PWM_FRAME_LENGTH ; i++)
//	{
//		spi_buffer_TX[i+2] = PWM_FRAME_CLR_DATA;
//	}
//	SPI_W_NBYTE(port, SPI_FRAME_ONE_PAGE, PWM_FRAME_FIRST_ADDR, PWM_FRAME_LENGTH);
//
//	//===============================//
//	//		Start Function3  Flow 		 //
//	//===============================//
//	//i loop for the  RED LED control Registers' address
//	for(i=0; i<RGB_PWM_LENGTH ; i++)
//	{
//		if (port == 1 && i == 6){
//			continue;
//		}
//		if (port ==2 && i>25){
//			continue;
//		}
//			SPI_W_3BYTE(port, SPI_FRAME_ONE_PAGE, tabLED_PWMCTLRed[i] , 	PWM_FRAME_MAX_DATA);		//turn on one LED
//			SPI_W_3BYTE(port, SPI_FRAME_ONE_PAGE, tabLED_PWMCTLGreen[i] , PWM_FRAME_MAX_DATA);		//turn on one LED
//			SPI_W_3BYTE(port, SPI_FRAME_ONE_PAGE, tabLED_PWMCTLBlue[i] , 	PWM_FRAME_MAX_DATA);		//turn on one LED
//			uprintf ("Port:%d\tLED:%d R:%02X G:%02X B:%02X\r\n",port,i,tabLED_PWMCTLRed[i],tabLED_PWMCTLGreen[i],tabLED_PWMCTLBlue[i]);
//			chThdSleepMilliseconds(300);				//delay 100ms
//			SPI_W_3BYTE(port, SPI_FRAME_ONE_PAGE, tabLED_PWMCTLRed[i] , 	PWM_FRAME_CLR_DATA);		//turn off one LED
//			SPI_W_3BYTE(port, SPI_FRAME_ONE_PAGE, tabLED_PWMCTLGreen[i] ,	PWM_FRAME_CLR_DATA);		//turn off one LED
//			SPI_W_3BYTE(port, SPI_FRAME_ONE_PAGE, tabLED_PWMCTLBlue[i] , 	PWM_FRAME_CLR_DATA);		//turn off one LED
//	}
//}
/*****************************************************************************
 * Function		: LED_SledShutDownFun
 * Description	: RLED Shutdown
 * Input			: None
 * Output		: None
 * Return		: None
 * Note			: None
 *****************************************************************************/
void LED_SledShutDownFun(uint8_t port) {

//	for (currentPort = 0; currentPort<LED_DriverCount; currentPort ++){
	//System into S/W Shutdown	mode
	SPI_W_3BYTE(port, SPI_FRAME_FUNCTION_PAGE, SW_SHUT_DOWN_REG,
			mskSW_SHUT_DOWN_MODE);

	//==============================//
	//Turn Off All LED  (Frame1Page)//
	//==============================//
	for (uint16_t i = 0; i < LED_FRAME_LENGTH; i++) {
		spi_buffer_TX[i + 2] = LED_FRAME_CLR_DATA;
	}
	SPI_W_NBYTE(port, SPI_FRAME_ONE_PAGE, LED_FRAME_FIRST_ADDR,
			LED_FRAME_LENGTH);
//	}

	//into H/W Shutdown
	__LED_SetShutDownPinLow

	//==============================//
	//H/W shutdown mode							//
	//==============================//
	chThdSleepMilliseconds(1000);				//delay 1000ms
	//leave H/W Shutdown
	__LED_SetShutDownPinHigh
	UT_DelayNx10us(20);				//!!delay 200us

//	for (currentPort = 0; currentPort<LED_DriverCount; currentPort ++){

	//System leave S/W Shutdown	mode, return Normal mode
	SPI_W_3BYTE(port, SPI_FRAME_FUNCTION_PAGE, SW_SHUT_DOWN_REG,
			mskSW_NORMAL_MODE);
	UT_DelayNx10us(20);				//!!delay 200us
//	}
}

/*****************************************************************************
 * Function		: LED_ClearFrame1Page by SPI
 * Description	: Clear Type1 / 2 /4  Frame 1 Page
 * Input			: None
 * Output		: None
 * Return		: None
 * Note			: None
 *****************************************************************************/
void LED_ClearFrame1Page(uint8_t port) {
	uint32_t i;
	for (i = 0; i < FRAME1PAGE_LENGTH; i++) {
		spi_buffer_TX[i + 2] = LED_FRAME_CLR_DATA;
	}
	//Setting LED Driver Ram Page to Frame 1 Page
	//send 0xB3 bytes length Data From address 0x00
	SPI_W_NBYTE(port, SPI_FRAME_ONE_PAGE, (mskLED_FRAME_REG_ADDR & 0x00),
			FRAME1PAGE_LENGTH);
}

/*****************************************************************************
 * Function		: UT_DelayNx10us
 * Description	: Delay N x 10 us @ Fcpu = 12MHz
 * Input			: N
 * Output		: None
 * Return		: None
 * Note			: May have some deviation (+/-10%)
 *****************************************************************************/
void UT_DelayNx10us(uint32_t N) {
	N *= 210;
	while (N--)
		;
}

/*****************************************************************************
 * Function		: UT_DelayNms
 * Description	: Delay N ms @ Fcpu = 12MHz
 * Input			: N
 * Output		: None
 * Return		: None
 * Note			: May have some deviation (+/-10%)
 *****************************************************************************/
//void UT_DelayNms (uint32_t N)
//{
////	while (N--)
////	{
////		SN_WDT->FEED = 0x5AFA55AA;	//Feed WDT
////		UT_DelayNx10us(100);				//10us x 100 = 1ms
////	}
//	chThdSleepMilliseconds(N);
//}
