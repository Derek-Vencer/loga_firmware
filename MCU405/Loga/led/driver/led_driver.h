#ifndef LED_DRIVER_H_
#define LED_DRIVER_H_


#define LED_DriverCount	3

void ledDriver_init(void);
void current_fineTune(int8_t adjRed, int8_t adjGreen,int8_t adjBlue);
void LED_CurrentAdjuest(uint8_t current);
void SPI_LevelShift_Test(void);

void SPI_W_3BYTE(uint8_t port, uint8_t bPage, uint8_t bStartAddr, uint8_t bData);
void SPI_W_NBYTE(uint8_t port, uint8_t bPage, uint8_t bStart_Addr, uint8_t blength);
void SPI_UPDATE_NBYTE(uint8_t port, uint8_t *data);

uint8_t SPI_R_1BYTE(uint8_t port, uint8_t bPage, uint8_t bAddr);
void SPI_R_NBYTE(uint8_t port, uint8_t bPage, uint8_t bStartAddr,	uint8_t blength);

//================================ Ramsetting =======================================
#define SPI_FRAME_ONE_PAGE				0x0		//Setting LED Driver Frame Page 1
// #define SPI_FRAME_TWO_PAGE				0x1		//Setting LED Driver Frame Page 2
#define	SPI_FRAME_FUNCTION_PAGE			0xB		//Setting LED Driver Frame Page 9
#define	SPI_FRAME_DETECTION_PAGE		0xC		//Setting LED Driver Frame Page 10
#define	SPI_FRAME_LED_VAF_PAGE			0xD		//Setting LED Driver Frame Page 11

//------------SPI---------------
extern uint8_t spi_buffer_TX[182]; // lyon 100 -> 182
extern uint8_t spi_buffer_RX[182];

//================================================================================

/*_____ D E F I N I T I O N S ______________________________________________*/

/*--------Registers Page Define---------------------------------------------*/
#define	FRAME1_PAGE 					0X00
#define	FRAME2_PAGE 					0X01
#define	FUNCTION_PAGE 					0X0B
#define	DETECTION_PAGE 					0X0C
#define	LED_VAF_PAGE 					0X0D

/*--------Registers Define--------------------------------------------------*/
#define	CONFIGURATION_REG 				0X00
#define	PICTURE_DISPLAY_REG 			0X01
#define	DISPLAY_OPTION_REG 				0X05
#define	BREATH_CTL_REG 					0X08
#define	BREATH_CTL_REG2 				0X09
#define	SW_SHUT_DOWN_REG 				0X0A

#define	AUDIO_GAIN_CTL_REG 				0X0B
#define	STAGGERED_DELAY_REG 			0X0D
#define	SLEW_RATE_CTL_REG 				0X0E
#define	CURRENT_CTL_REG 				0X0F
#define	OPEN_SHORT_REG 					0X10
#define	OPEN_SHORT_REG2 				0X11
#define	VAF_CTL_REG 					0X14
#define	VAF_CTL_REG2 							0X15
#define ID_Register 			0x1B

/*--------Function Register: address 00H Configuration Reg------------*/
#define	mskDISPLAY_MODE_EN				(0X0<<3)
#define	mskDISPLAY_MODE_DIS				(0X1<<3)		//disable PWM output
#define	mskSHOLD_EN						(0x1<<5)
#define	mskSHOLD_DIS					(0x0<<5)
#define	mskSYNC_HIGH_IMPEDANCE			(0x0<<6)
#define	mskSYNC_MASTER_MODE				(0x1<<6)
#define	mskSYNC_SLAVE_MODE				(0x2<<6)

/*--------Function Register: address 01H Picture Display Reg------------*/
#define	mskMATRIX_TYPE					(0x1<<3)        //Type 3

/*--------Function Register: address 05H Display Option Reg------------*/
#define	mskBLINK_PERIOD_TIME_CONST		(0x7<<0)
#define	mskBLINK_EN						(0x1<<3)
#define	mskBLINK_DIS					(0x0<<3)
#define	mskBLINK_FRAME_300				(0x0<<6)
#define	mskBLINK_FRAME_600				(0x1<<6)
#define	mskBLINK_FRAME_75				(0x2<<6)
#define	mskBLINK_FRAME_150				(0x3<<6)

/*--------Function Register: address 08H Breath Control Reg------------*/
#define	mskBC_FADE_IN_TIME_CONST		(0x7<<0)
#define	mskBC_FADE_OUT_TIME_CONST		(0x7<<4)

/*--------Function Register: address 09H Breath Control Reg2------------*/
#define	mskBC_EXTINGUISH_TIME_CONST		(0x7<<0)
#define	mskBC_BREATH_EN					(0x1<<4)
#define	mskBC_BREATH_DIS				(0x0<<4)
#define	mskBC_CONTINUOUS_BREATH_EN		(0x1<<5)
#define	mskBC_CONTINUOUS_BREATH_DIS		(0x0<<5)

/*--------Function Register: address 0AH Shutdown Reg------------*/
#define	mskSW_SHUT_DOWN_MODE			(0x0<<0)
#define	mskSW_NORMAL_MODE				(0x1<<0)

/*--------Function Register: address 0BH AGC Control Reg------------*/
#define	mskAGS_CONST					(0x7<<0)
#define	mskAGC_EN						(0x0<<3)
#define	mskAGC_DIS						(0x1<<3)
#define	mskAGC_MODE_SLOW				(0x0<<4)
#define	mskAGC_MODE_FAST				(0x1<<4)

/*--------Function Register: address 0DH Staggered Delay Reg------------*/
#define	mskSTD1							(0x3<<0)
#define	mskSTD2							(0x3<<2)
#define	mskSTD3							(0x3<<4)
#define	mskSTD4							(0x3<<6)

#define	CONST_STD_GROUP1				0x00
#define	CONST_STD_GROUP2				0x55
#define	CONST_STD_GROUP3				0xAA
#define	CONST_STD_GROUP4				0xFF

/*--------Function Register: address 0EH Slew Rate Control Reg------------*/
#define	mskSLEW_RATE_CTL_EN				(0x1<<0)
#define	mskSLEW_RATE_CTL_DIS			(0x0<<0)

/*--------Function Register: address 0FH Current Control Reg------------*/
#define	mskCURRENT_STEP_CONST			(0x3F<<0)
#define	CONST_CURRENT_STEP_10mA			(0x5<<0)
#define	CONST_CURRENT_STEP_15mA			(0xF<<0)
#define	CONST_CURRENT_STEP_20mA			(0x19<<0)
#define	CONST_CURRENT_STEP_21mA			(0x20<<0)
#define	CONST_CURRENT_STEP_25mA			(0x23<<0)
#define	CONST_CURRENT_STEP_32mA			(0x31<<0)
#define	CONST_CURRENT_STEP_40mA			(0x3F<<0)

#define	mskCURRENT_CTL_EN				(0x1<<7)
#define	mskCURRENT_CTL_DIS				(0x0<<7)

/*--------Function Register: address 10H Open/Short Test Reg------------*/
#define	mskOPEN_SHORT_DETECT_DUTY		(0x3F<<0)
#define	mskSHORT_DETECT_START			(0x1<<6) // HW will clear this bits automatically
#define	mskOPEN_DETECT_START			(0x1<<7) // HW will clear this bits automatically

/*--------Function Register: address 11H Open/Short Test Reg2------------*/
#define	mskSHORT_DETECT_INT				(0x1<<6) //when open/short detect done, HW will set this bit as 1 automatically
#define	mskOPEN_DETECT_INT				(0x1<<7) //when open/short detect done, HW will set this bit as 1 automatically

/*--------Function Register: address 14H VAF Reg1 ------------*/
#define	mskVAF1							(0x4<<0)
#define	mskVAF2							(0x4<<4)

/*--------Function Register: address 15H VAF Reg2 ------------*/
#define	mskVAF3							(0x4<<0)

#define	mskTP4VAF3						(0x0<<0)

#define	mskFORCEVAFTIME_CONST			(0x0<<3)
#define	mskFORCEVAFCTL_ALWAYSON			(0x0<<6)
#define	mskFORCEVAFCTL_VAFTIMECTL		(0x1<<6)
#define	mskFORCEVAFCTL_DISABLE			(0x2<<6)

/*----------------------------------------------------------*/
/*											Frame1Page													*/
/*----------------------------------------------------------*/
#define	mskLED_FRAME_REG_ADDR			(0xFF<<0)
#define	FRAME1PAGE_LENGTH				((PWM_FRAME_LAST_ADDR-LED_FRAME_FIRST_ADDR)+1)

//=======================//
// LED Control Registers //
//=======================//
//-------Frame1----------//
#define	LED_FRAME_FIRST_ADDR			0x00
#define	LED_FRAME_LAST_ADDR				0x11
#define	LED_FRAME_LENGTH				((LED_FRAME_LAST_ADDR-LED_FRAME_FIRST_ADDR)+1)
//=========================//
// Blink Control Registers //
//=========================//
//-------Frame1----------//
#define	mskBLINK_FRAME_REG_ADDR			(0xFF<<0)
#define	BLINK_FRAME_FIRST_ADDR			0x12
#define	BLINK_FRAME_LAST_ADDR			0x23
#define	BLINK_FRAME_LENGTH				((BLINK_FRAME_LAST_ADDR-BLINK_FRAME_FIRST_ADDR)+1)

//=======================//
// PWM Control Registers //
//=======================//
//-------Frame1----------//
#define	mskPWM_FRAME_REG_ADDR			(0xFF<<0)
#define	PWM_FRAME_FIRST_ADDR			0x24
#define	PWM_FRAME_LAST_ADDR				0xB3
#define	PWM_FRAME_LENGTH				((PWM_FRAME_LAST_ADDR-PWM_FRAME_FIRST_ADDR)+1)

#define	RGB_PWM_LENGTH					0x28 //Total control 40 RGB LEDs

// //========================//
// // Current Tune Registers //
// //========================//	
#define	CURRENT_TUNE_FIRST_ADDR			0x24
#define	CURRENT_TUNE_LAST_ADDR			0x6B
#define	CURRENT_TUNE_LENGTH				0x48

//#define	CURRENT_TUNE_FIRST_ADDR			0x40
//#define	CURRENT_TUNE_LAST_ADDR			0xBF
//#define	CURRENT_TUNE_LENGTH				0x7F


/*----------------------------------------------------------*/
/*											Frame Data 													*/
/*----------------------------------------------------------*/
/*--------Frame Register: Data -----------------------------*/
#define	mskLED_FRAME_REG_DATA			(0xFF<<0)
#define	LED_FRAME_CLR_DATA				0x00
#define	LED_FRAME_MAX_DATA				0xFF

#define	mskBLINK_FRAME_REG_DATA			(0xFF<<0)
#define	BLINK_FRAME_CLR_DATA			0x00
#define	BLINK_FRAME_MAX_DATA			0xFF

#define	mskPWM_FRAME_REG_DATA			(0xFF<<0)
#define	PWM_FRAME_CLR_DATA				0x00
#define	PWM_FRAME_MAX_DATA				0xFF/8

/*----------------------------------------------------------*/
/*											FrameVAF1Page												*/
/*----------------------------------------------------------*/
//-------Frame1----------//
#define	VAF_FRAME_FIRST_ADDR			0x00
#define	VAF_FRAME_LAST_ADDR				0x23
#define	VAF_FRAME_LENGTH				((VAF_FRAME_LAST_ADDR-VAF_FRAME_FIRST_ADDR)+1)

/*_____ M A C R O S ________________________________________________________*/

/*********SDB function***************/
//Setting LED Driver HW Shutdown Pin H: Driver works, L: Driver standby
#define __LED_SetShutDownPinHigh  	palSetLine(SPI_LED_SDB_PIN);
#define	__LED_SetShutDownPinLow		palClearLine(SPI_LED_SDB_PIN);
/***************************************/
/***************************************/

extern void LED_ClearFrame1Page(uint8_t port);
extern void UT_DelayNx10us(uint32_t N);
extern void UT_DelayNms(uint32_t N);

//================================================================================

#endif /* LED_DRIVER_H_ */
