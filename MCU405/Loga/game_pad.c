
#include "game_pad.h"
#include "led/led_function.h"
#include "report.h"
#include "host.h"

binary_semaphore_t gamepadAnalogSem;
mutex_t analogStickMutex;

extern AnalogStickStatus_t gameAnalogStickStatus;
extern analog_stickValue gameAnalogValue, lastGameAnalogValue;
extern const analogKey_Config_t matrixAnalogRelocate[MATRIX_ROWS][MATRIX_COLS];
extern adcsample_t samples2[ADC_BUF_DEPTH][ADC_NUM_CHANNELS], initPostionLevel[ADC_BUF_DEPTH][ADC_NUM_CHANNELS], travelPostionLevel[ADC_BUF_DEPTH][ADC_NUM_CHANNELS];
extern report_gamepad_report extra_game_report;
uint16_t analogX,analogY,analogRx,analogRy,analogZ,analogRz;
uint16_t currentLength,LastLength;
travelPostion_t xTravelPostionLevelPlus,yTravelPostionLevelPlus,rxTravelPostionLevelPlus,ryTravelPostionLevelPlus,zTravelPostionLevelPlus,rzTravelPostionLevelPlus;
travelPostion_t xTravelPostionLevelMinus,yTravelPostionLevelMinus,rxTravelPostionLevelMinus,ryTravelPostionLevelMinus,zTravelPostionLevelMinus,rzTravelPostionLevelMinus;


int16_t getCurrentKeyPostion(uint8_t col, uint8_t row,travelPostion_t *padTravelPostion){
	//uprintf ("row:%d col:%d\r\n",row,col);
	analogKey_Config_t keyLocation = matrixAnalogRelocate[row][col];
	if (keyLocation.rowA < 0 || keyLocation.colA < 0 ||
		keyLocation.rowA >= ADC_NUM_CHANNELS || keyLocation.colA >= ADC_BUF_DEPTH) {
		return 0;
	}
	if (padTravelPostion->travelPostion != travelPostionLevel [keyLocation.colA][keyLocation.rowA]){
		float range = (float)travelPostionLevel[keyLocation.colA][keyLocation.rowA] -
				      (float)initPostionLevel[keyLocation.colA][keyLocation.rowA];
		if (range <= 0.0f) {
			padTravelPostion->postioMutl = 0.0f;
		} else {
			padTravelPostion->postioMutl = 600.0f / range * 1.05f;
		}
		padTravelPostion->travelPostion = travelPostionLevel [keyLocation.colA][keyLocation.rowA];
		uint16_t multInt = padTravelPostion->postioMutl*1000;
////		uprintf ("Travel:%d init:%d multi:%d.%d\r\n",travelPostionLevel [keyLocation.colA][keyLocation.rowA],initPostionLevel[keyLocation.colA][keyLocation.rowA],multInt/1000,multInt%1000);
////		uprintf ("multi:%d.%d\r\n",multInt/1000,multInt%1000);
	}
	uint16_t multInt = padTravelPostion->postioMutl*1000;
	//uprintf ("read:%d Travel:%d init:%d multi:%d.%d\r\n",(samples2[keyLocation.colA][keyLocation.rowA] - initPostionLevel[keyLocation.colA][keyLocation.rowA]),travelPostionLevel [keyLocation.colA][keyLocation.rowA],initPostionLevel[keyLocation.colA][keyLocation.rowA],multInt/1000,multInt%1000);
	float value = (float)(samples2[keyLocation.colA][keyLocation.rowA] - initPostionLevel[keyLocation.colA][keyLocation.rowA])* padTravelPostion->postioMutl;
	//uprintf ("value:%d\r\n",(int16_t)value);
	return (int16_t)value;
}

static THD_WORKING_AREA(waGamepadAnalog_ProcessThread, 256);
static THD_FUNCTION(GamepadAnalog_ProcessThread, arg){
	static bool reportUpdate;
	uint8_t *lengthAddress;
	chRegSetThreadName("Gamepad Analog Control");
	chBSemObjectInit(&gamepadAnalogSem, true);
	osalMutexObjectInit(&analogStickMutex);
	lengthAddress = &extra_game_report;
	while (true){
			chBSemWait(&gamepadAnalogSem);
//			//uprintf ("gameAnalogStickStatus:%d\r\n",gameAnalogStickStatus);
				if(gameAnalogStickStatus.x_plus){
					gameAnalogValue.x_plus.value = getCurrentKeyPostion(gameAnalogValue.x_plus.event.key.col,gameAnalogValue.x_plus.event.key.row,&xTravelPostionLevelPlus);
					if (gameAnalogValue.x_plus.value>0){
						if (lastGameAnalogValue.x_plus.value != gameAnalogValue.x_plus.value){
//							uprintf ("analog x plus changed value:%x\r\n",gameAnalogValue.x_plus.value);
							analogX = 0x0258 + (uint16_t)gameAnalogValue.x_plus.value;
//							uprintf ("analogx plus:%d\r\n",analogX);
						}
						lastGameAnalogValue.x_plus.value = gameAnalogValue.x_plus.value;
					}

				}
				if(gameAnalogStickStatus.x_minus){

					gameAnalogValue.x_minus.value = getCurrentKeyPostion(gameAnalogValue.x_minus.event.key.col,gameAnalogValue.x_minus.event.key.row,&xTravelPostionLevelMinus);
					if (gameAnalogValue.x_minus.value>0){
						if (lastGameAnalogValue.x_minus.value != gameAnalogValue.x_minus.value){
							if ((uint16_t)gameAnalogValue.x_minus.value < 0x0258){
								analogX = 0x0258 - (uint16_t)gameAnalogValue.x_minus.value;
							} else {
								analogX = 0;
							}
//							uprintf ("analogx min:%d\r\n",analogX);
						}
						lastGameAnalogValue.x_minus.value = gameAnalogValue.x_minus.value;
					}
				}
				if(gameAnalogStickStatus.y_plus){
					gameAnalogValue.y_plus.value = getCurrentKeyPostion(gameAnalogValue.y_plus.event.key.col,gameAnalogValue.y_plus.event.key.row,&yTravelPostionLevelPlus);
					if (gameAnalogValue.y_plus.value>0){
						if (lastGameAnalogValue.y_plus.value != gameAnalogValue.y_plus.value){
							if ((uint16_t)gameAnalogValue.y_plus.value < 0x0258){
								analogY = 0x0258 - (uint16_t)gameAnalogValue.y_plus.value;
							} else {
								analogY = 0;
							}
						}
						lastGameAnalogValue.y_plus.value = gameAnalogValue.y_plus.value;
					}

				}

				if(gameAnalogStickStatus.y_minus){

					gameAnalogValue.y_minus.value = getCurrentKeyPostion(gameAnalogValue.y_minus.event.key.col,gameAnalogValue.y_minus.event.key.row,&yTravelPostionLevelMinus);
					if (gameAnalogValue.y_minus.value>0){
						if (lastGameAnalogValue.y_minus.value != gameAnalogValue.y_minus.value){
							analogY = 0x0258 + (uint16_t)gameAnalogValue.y_minus.value;
						}
						lastGameAnalogValue.y_minus.value = gameAnalogValue.y_minus.value;
					}
				}

//====
				if(gameAnalogStickStatus.z_plus){
					gameAnalogValue.z_plus.value = getCurrentKeyPostion(gameAnalogValue.z_plus.event.key.col,gameAnalogValue.z_plus.event.key.row,&zTravelPostionLevelPlus);
					if (gameAnalogValue.z_plus.value>0){
						if (lastGameAnalogValue.z_plus.value != gameAnalogValue.z_plus.value){


							if ((uint16_t)(gameAnalogValue.z_plus.value) < 0x0258){
								analogZ = 0x0258 - (uint16_t)(gameAnalogValue.z_plus.value);
							} else {
								analogZ = 0;
							}
							if (analogZ>512){analogZ = 512;}
							//uprintf ("analogZ plus:%d\r\n",analogZ);
						}
						lastGameAnalogValue.z_plus.value = gameAnalogValue.z_plus.value;
					}

				}
				if(gameAnalogStickStatus.z_minus){

					gameAnalogValue.z_minus.value = getCurrentKeyPostion(gameAnalogValue.z_minus.event.key.col,gameAnalogValue.z_minus.event.key.row,&zTravelPostionLevelMinus);
					if (gameAnalogValue.z_minus.value>0){
						if (lastGameAnalogValue.z_minus.value != gameAnalogValue.z_minus.value){
							analogZ = 512 + (uint16_t)(gameAnalogValue.z_minus.value);
							if (analogZ>1023){analogZ = 1023;}
							//uprintf ("analogZ z_minus:%d def:%d\r\n",analogZ,gameAnalogValue.z_minus.value);
						}
						lastGameAnalogValue.z_minus.value = gameAnalogValue.z_minus.value;
					}
				}
				if(gameAnalogStickStatus.Rz_plus){
					gameAnalogValue.Rz_plus.value = getCurrentKeyPostion(gameAnalogValue.Rz_plus.event.key.col,gameAnalogValue.Rz_plus.event.key.row,&rzTravelPostionLevelPlus);
					if (gameAnalogValue.Rz_plus.value>0){

						if (lastGameAnalogValue.Rz_plus.value != gameAnalogValue.Rz_plus.value){
							analogRz = 512 + (uint16_t)(gameAnalogValue.Rz_plus.value);
							if (analogRz>1023){analogRz = 1023;}
							//uprintf ("analogZ z_minus:%d def:%d\r\n",analogRz,gameAnalogValue.Rz_plus.value);
						}
						lastGameAnalogValue.Rz_plus.value = gameAnalogValue.Rz_plus.value;
					}

				}

				if(gameAnalogStickStatus.Rz_minus){

					gameAnalogValue.Rz_minus.value = getCurrentKeyPostion(gameAnalogValue.Rz_minus.event.key.col,gameAnalogValue.Rz_minus.event.key.row,&rzTravelPostionLevelMinus);
					if (gameAnalogValue.Rz_minus.value>0){
						if (lastGameAnalogValue.Rz_minus.value != gameAnalogValue.Rz_minus.value){

							if ((uint16_t)(gameAnalogValue.Rz_minus.value) < 512){
								analogRz = 512 - (uint16_t)(gameAnalogValue.Rz_minus.value);
							} else {
								analogRz = 0;
							}
							if (analogRz>512){analogRz=512;}
							//uprintf ("analogRz min:%d\r\n",analogRz);
						}
						lastGameAnalogValue.Rz_minus.value = gameAnalogValue.Rz_minus.value;
					}
				}

				reportUpdate = false;
				if (extra_game_report.GD_GamePadX !=analogX){
					extra_game_report.GD_GamePadX = analogX;
					reportUpdate = true;
				}
				if (extra_game_report.GD_GamePadY !=analogY){
					extra_game_report.GD_GamePadY = analogY;
					reportUpdate = true;
				}
//====
				if (extra_game_report.GD_GamePadZ !=analogZ){
					extra_game_report.GD_GamePadZ = analogZ;
					reportUpdate = true;
				}
				if (extra_game_report.GD_GamePadRz !=analogRz){
					extra_game_report.GD_GamePadRz = analogRz;
					reportUpdate = true;
				}
				if (reportUpdate){
					send_gamepad(extra_game_report);
					chThdSleepMilliseconds(1);
					send_gamepad(extra_game_report);
				}
	}
}

void gamePad_init(void){
	extra_game_report.GD_GamePadX = analogX = 0x04b0/2;
	extra_game_report.GD_GamePadY = analogY = 0x04b0/2;
	extra_game_report.GD_GamePadRx = analogRx = 0x04b0/2;
	extra_game_report.GD_GamePadRy = analogRy = 0x04b0/2;

	extra_game_report.GD_GamePadZ = analogZ = 0x3ff/2;
	extra_game_report.GD_GamePadRz = analogRz = 0x3ff/2;
	chThdCreateStatic(waGamepadAnalog_ProcessThread, sizeof(waGamepadAnalog_ProcessThread), NORMALPRIO -10 , GamepadAnalog_ProcessThread, NULL);

}



