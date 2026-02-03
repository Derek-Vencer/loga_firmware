# include "hal.h"
# include "keymap_common.h"
# include "report.h"
# include "matrix.h"
# include "dynamic_macro.h"
# include "usb_main.h"
# include "../led/led_function.h"
# include "loga_function.h" // lyon for layer_keypos_t
# include "socd.h" 

#include "via.h"

#define DYNAMIC_KEYMAP_MACRO_EEPROM_SIZE sizeof(dynamic_macro_t) * DYNAMIC_MACRO_COUNT

binary_semaphore_t VIA_ReciveSem;
extern uint8_t raw_report[32];
extern uint16_t eepromKeyBuffer[KEYMAPS_COUNT][MATRIX_ROWS][MATRIX_COLS];
extern int16_t keyMatrixTriggerLevel, keyMatrixReleaseLevel;
extern led_runMode_t currentLED_RunMode;
extern uint8_t currentProfile;

extern led_config_t LED_Datas[LedNumbers]; // lyon

extern EffectStatus_t effectStatus; //set led by lyon
extern binary_semaphore_t ledEffetSem;
extern EffectMosArt_t mosArt;

extern uint8_t knobState; // lyon for encoder in matrix.c

extern uint8_t rapidKeyEnable; // test for rapid trigger state

// extern layer_keypos_t snappy_key1;
// extern layer_keypos_t snappy_key2;

#ifdef socd 
// extern socd_keyrecord_t firstSnap; // for socd function in socd.h lyon
extern socd_policy_t g_policy;
// extern pair_state_t g_pairs[];
// extern SOCDPair g_pairs[];
// socd_pos_t A, B;
#endif // socd

#ifdef buttonLock
extern bool winLock, altTabLock, altF4Lock;
#endif // buttonLock

#ifdef modTap
extern void mt_schedule_set(uint8_t L,uint8_t R,uint8_t C,uint8_t h,uint8_t k);
#endif //modTap

#ifdef toggleKey
void    toggle_key_set(uint8_t layer, uint8_t row, uint8_t col, uint8_t keycode);
uint8_t toggle_key_get(uint8_t layer, uint8_t row, uint8_t col);
#endif // toggleKey

// extern int16_t keyMatrixTriggerLevel; // for set rapid trigger value
// extern int16_t keyMatrixReleaseLevel;

extern rapidTriggerKey_config_t rapidTriggerKey[RAPID_TRIGGER_COUNT];

//USB Recived
static THD_WORKING_AREA(waVIA_Thread, 256);
static THD_FUNCTION(VIA_Thread, arg)
{
    (void)arg;

    chRegSetThreadName("VIA Recive");
    chBSemObjectInit(&VIA_ReciveSem, true);

    while (true)
    {
        chBSemWait(&VIA_ReciveSem);

//        for (uint8_t i=0;i<sizeof(raw_report);i++){
//        	printf("%02X",raw_report[i]);
//        }
        raw_hid_receive(raw_report, sizeof(raw_report));
    }
}

void via_creatTask(void)
{
    chThdCreateStatic(waVIA_Thread, sizeof(waVIA_Thread), NORMALPRIO + 3, VIA_Thread, NULL);
}


void via_init(void){
	via_creatTask();
}

//void dynamic_keymap_get_buffer(uint16_t offset, uint16_t size, uint8_t *data) {
//    uint16_t * source = (uint16_t *)(&eepromKeyBuffer[0][0][0] + offset/2);
//    uint8_t *target = data;
//    for (uint16_t i = 0; i < size/2; i++) {
//        if (offset + i < sizeof(eepromKeyBuffer)) {
//        	*target = (*source)>>8;
//            target++;
//            *target = (*source)&0xff;
//
//        } else {
//            *target = 0x00;
//            target++;
//            *target = 0x00;
//        }
//        target++;
//        source++;
//
//    }
//}

void dynamic_keymap_get_buffer(uint16_t offset, uint16_t size, uint8_t *data) {
    uint16_t * source = (uint16_t *)(&eepromKeyBuffer[0][0][0] + offset);
    uint16_t *target = data;
    for (uint16_t i = 0; i < size; i++) {
        if (offset + i < sizeof(eepromKeyBuffer)) {
        	*target = *source<<8;
        	*target |= (*source&0xff00)>>8;
        } else {
            *target = 0x00;
        }
        target++;
        source++;

    }
}

void dynamic_keymap_set_buffer(uint16_t offset, uint16_t size, uint8_t *data) {
    uint16_t * source = (uint16_t *)(&eepromKeyBuffer[0][0][0] + offset);
    uint16_t *target = data;
    for (uint16_t i = 0; i < size; i++) {
        if (offset + i < sizeof(eepromKeyBuffer)) {
//            eeprom_update_byte(target, *source);
        	*target = *source<<8;
        	*target |= (*source&0xff00)>>8;
        }
        source++;
        target++;
    }

}

void dynamic_keymap_macro_get_buffer(uint16_t offset, uint16_t size, uint8_t *data) {
    void *   source = (void *)(DYNAMIC_MACRO_EEPROM_BLOCK0_ADDR + offset);
    uint8_t *target = data;
    for (uint16_t i = 0; i < size; i++) {
        if (offset + i < DYNAMIC_KEYMAP_MACRO_EEPROM_SIZE) {
            *target = eeprom_read_byte(source);
        } else {
            *target = 0x00;
        }
        source++;
        target++;
    }
}

void dynamic_keymap_macro_set_buffer(uint16_t offset, uint16_t size, uint8_t *data) {
    void *   target = (void *)(DYNAMIC_MACRO_EEPROM_BLOCK0_ADDR + offset);
    uint8_t *source = data;
    for (uint16_t i = 0; i < size; i++) {
        if (offset + i < DYNAMIC_KEYMAP_MACRO_EEPROM_SIZE) {
            eeprom_update_byte(target, *source);
        }
        source++;
        target++;
    }
}

void dynamic_keymap_macro_reset(void) {
    void *p   = (void *)(DYNAMIC_MACRO_EEPROM_BLOCK0_ADDR);
    void *end = (void *)(DYNAMIC_MACRO_EEPROM_BLOCK0_ADDR + DYNAMIC_KEYMAP_MACRO_EEPROM_SIZE);
    while (p != end) {
        eeprom_update_byte(p, 0);
        ++p;
    }
}

// Keyboard level code can override this to handle custom messages from VIA.
// See raw_hid_receive() implementation.
// DO NOT call raw_hid_send() in the override function.
__attribute__((weak)) bool raw_hid_receive_kb(uint8_t *data, uint8_t length) {
    uint8_t *command_id = &(data[0]);
    *command_id         = id_unhandled;
    // uprintf("kb\r\n");
}

bool raw_hid_receive_ione(uint8_t *data, uint8_t length) {
	uint8_t *data_start = &(data[0]);
	uint8_t recive_payload[32-6];
	uint16_t hid_vid = *data++ <<8 | *data++;
	uint8_t *payload_length = &(data[0]);
	data++;
	//*data |= 0x80;
    uint16_t command_id = *data++ <<8 | *data++;
    uint8_t *send_payload = data;
    uint8_t *send_status = &(data[0]);
    uint8_t *send_data = &(data[1]);
	// uprintf ("iOne API Receive:\r\nvid:0x%04X\r\nlength:%d\r\ncommand_id:0x%04X\r\npayload:",hid_vid,*payload_length,command_id);
	memcpy (recive_payload,data,sizeof(recive_payload));
//    for (uint8_t i=0;i<payload_length;i++){
//    	uprintf ("%02X ",recive_payload[i]);
//    }
//    uprintf ("\r\n");

	memset (send_payload,0,sizeof(recive_payload));
	switch(command_id){
		case ione_api_cmd_get_firmware_version:
			uprintf ("API=>Get Firmware Version:\r\n");
			if (*payload_length == 0){
				*send_status = ione_api_status_success;
				*send_data = DEVICE_VER>>8;
				uprintf("Major Version:%02X ",*send_data++);
				*send_data = DEVICE_VER & 0xFF;
				uprintf ("Minor: Version:%02X ",*send_data++);
				*send_data = DEVICE_VER_FIX;//0x00;
				uprintf ("Build: Version:%02X\r\n",*send_data++);
			} else {
				*send_status = ione_api_status_fail;
				uprintf ("Fail\r\n");
			}
			*payload_length = 4;
		break;
		case ione_api_cmd_Set_Profil:
			uprintf ("API=>Set Profile:\r\n");
			if (*payload_length == 1 && (recive_payload[0]<PROFILE_COUNT)){
				*send_status = ione_api_status_success;
				currentProfile = recive_payload[0];
				layer_move(currentProfile*LAYERS_PER_PROFILE);
				uprintf ("Profile:%d \r\n",recive_payload[0]);
				clear_keyboard();
			}
			else {
				*send_status = ione_api_status_fail;
				uprintf ("Fail\r\n");
			}
			*payload_length = 1;
		break;
		case ione_api_cmd_get_Profile_state:
			uprintf ("API=>Get Profile:\r\n");
			if (*payload_length == 0){
			*send_status = ione_api_status_success;
			*send_data = currentProfile;
			uprintf ("Profile:%d\r\n",*send_data++);
			} else {
				*send_status = ione_api_status_fail;
				uprintf ("Fail\r\n");
			}
			*payload_length = 2;
		break;
        case ione_api_cmd_set_key_mapping: // from here 
            uprintf ("API=>Set Key Mapping:\r\n");
            if (*payload_length == 5 && (recive_payload[0]<KEYMAPS_COUNT && recive_payload[1]<MATRIX_ROWS && recive_payload[2]<MATRIX_COLS)){
                *send_status = ione_api_status_success;
                eeconfig_write_keymap_key(recive_payload[0], recive_payload[1], recive_payload[2], (recive_payload[3] << 8) | recive_payload[4]); //Add comment More actions
            } else {
                *send_status = ione_api_status_fail;
				uprintf ("Fail\r\n");
            }
            *payload_length = 1;
		break;
		case ione_api_cmd_get_key_mapping: //ft
            uprintf ("API=>Get Key Mapping:\r\n");
            if (*payload_length == 3 && (recive_payload[0]<KEYMAPS_COUNT && recive_payload[1]<MATRIX_ROWS && recive_payload[2]<MATRIX_COLS)){
                *send_status = ione_api_status_success;

                uint8_t layer = recive_payload[0];
                uint8_t row = recive_payload[1];
                uint8_t col = recive_payload[2];

                uint16_t keycode = eepromKeyBuffer[layer][row][col];

                uint8_t modifier = (keycode >> 8) & 0xFF;  // 擃���modifier
                uint8_t key_code = keycode & 0xFF;         // 雿���key code

                *send_data = layer;      // Layer
                send_data++;
                *send_data = row;        // Row
                send_data++;
                *send_data = col;        // Column
                send_data++;
                *send_data = modifier;   // Modifier
                send_data++;
                *send_data = key_code;   // Key Code
                // send_data++;

                uprintf("Layer:%d, Row:%d, Col:%d, Modifier:0x%02X, KeyCode:0x%02X\r\n",
                        layer, row, col, modifier, key_code);

            } else {
                *send_status = ione_api_status_fail;
				uprintf ("Fail\r\n");
            }
            *payload_length = 6;
		break;
		case ione_api_cmd_get_all_key_mapping:
            uprintf ("API=>Get All Key Mapping:\r\n");

            uint8_t payload_length_count = 0;
            const size_t EP = LAMP_ARRAY_EPSIZE;   // 32 或 64，依你的端點大小
            uint8_t *w;  

            if(*payload_length == 0){

                for(uint8_t layerNum = 0; layerNum < KEYMAPS_COUNT; layerNum++){
                    uprintf("=== layer number is %d ===\r\n",layerNum);
                    for(uint8_t rowNum = 0; rowNum < MATRIX_ROWS; rowNum++){
                        for(uint8_t colNum = 0; colNum < MATRIX_COLS; colNum++){
                            *(data_start + 3) |= 0x80;
                            w = send_data;
                            *send_status = ione_api_status_sending;

                            uint16_t keycode = eepromKeyBuffer[layerNum][rowNum][colNum];

                            uint8_t modifier = (keycode >> 8) & 0xFF;
                            uint8_t key_code = keycode & 0xFF;

                            uprintf("(%d,%d),modifier %u, keycode is %u\r\n",rowNum,colNum,modifier, key_code);

                            *w = layerNum;      // Layer
                            w++;
                            *w = rowNum;        // Row
                            w++;
                            *w = colNum;        // Column
                            w++;
                            *w = modifier;   // Modifier
                            w++;
                            *w = key_code;   // Key Code
                            w++;

                            *payload_length = (uint8_t)(1 + (w - send_data));  // 1 = status
                            raw_hid_send(data_start, EP);
                            payload_length_count+=5;   
                        }
                    }
                }

                *send_status = ione_api_status_success;

            } else {
                *send_status = ione_api_status_fail;
				uprintf ("Fail\r\n");
            }
            *payload_length = payload_length_count + 2;
		break;
		case ione_api_cmd_set_ligthing_effect:
            uprintf("API=>Set Lighting Effect:\r\n");
            
            if (*payload_length == 1) {
                
                *send_status = ione_api_status_success;

                currentLED_RunMode.mode = recive_payload[0];
                setRunMode(currentLED_RunMode);

                // uint8_t effect = recive_payload[0];

                /*switch(effect){
                    // case 0x00: // Static
                    // break;
                    // case 0x01: // Disco
                    // break;
                    // case 0x02: // Wave
                    // break;
                    // case 0x03: // Prism
                    // break;
                    // case 0x04: // Rainfall
                    // break;


                    currentLED_RunMode.mode = ;
                    setRunMode(currentLED_RunMode);
                }*/
            } else {
                *send_status = ione_api_status_fail;
                uprintf("Payload should be empty\r\n");
            }
            *payload_length = 1;
		break;
		case ione_api_cmd_get_lighting_effect:
            uprintf("API=>Get Lighting Effect:\r\n");
            if (*payload_length == 0) {
                *send_status = ione_api_status_success;
                *send_data = currentLED_RunMode.mode;
                uprintf("Current Lighting Effect: %d\r\n", currentLED_RunMode.mode);
            } else {
                *send_status = ione_api_status_fail;
                uprintf("Fail\r\n");
            }
            *payload_length = 2;
		break;
		case ione_api_cmd_set_lighting_color:
            uprintf("API=>Set Lighting Color:\r\n");
            
            if (*payload_length == 8) {
                *send_status = ione_api_status_success;

                recive_payload[0];
                mosArt.LED.red = recive_payload[1];
                mosArt.LED.green = recive_payload[2];
                mosArt.LED.blue = recive_payload[3];
                recive_payload[4];
                mosArt.LED.red = recive_payload[5];
                mosArt.LED.green = recive_payload[6];
                mosArt.LED.blue = recive_payload[7];

                effectStatus.effect = EffectTypeMosArt;
                effectStatus.mode = mosArtStatic;
                chBSemSignal(&ledEffetSem);  // 通知 LED_EffectThread 執行


                // setRunMode(currentLED_RunMode);
            } else {
                *send_status = ione_api_status_fail;
                uprintf("Payload should be empty\r\n");
            }
            *payload_length = 1;
		break;
		case ione_api_cmd_get_lighting_color:
            uprintf("API=>Get Lighting Color:\r\n");
            if (*payload_length == 0) {
                *send_status = ione_api_status_success;
                *send_data = 0xa1; // alpha
                send_data++;
                *send_data = LED_Datas[0].red;
                send_data++;
                *send_data = LED_Datas[0].green;
                send_data++;
                *send_data = LED_Datas[0].blue;
                send_data++;
                *send_data = 0xa2; // alpha
                send_data++;
                *send_data = mosArt.LED.red;
                send_data++;
                *send_data = mosArt.LED.green;
                send_data++;
                *send_data = mosArt.LED.blue;

                uprintf("Current Lighting Effect: red : %d, green : %d, blue : %d\r\n", LED_Datas[0].red, LED_Datas[0].green, LED_Datas[0].blue);
            } else {
                *send_status = ione_api_status_fail;
                uprintf("Fail\r\n");
            }
            *payload_length = 9;
		break;
        case ione_api_cmd_set_konb_state:
            if (*payload_length == 1){
                knobState = recive_payload[0];
            } else {
                *send_status = ione_api_status_fail;
            }
            payload_length = 1;
        break;
		case ione_api_cmd_konb_state:
            uprintf ("API=>Get Konb state:\r\n");
			if (*payload_length == 0){
			*send_status = ione_api_status_success;
			*send_data = knobState;
			uprintf ("Current knob state:%d\r\n",knobState);
			} else {
				*send_status = ione_api_status_fail;
				uprintf ("Fail\r\n");
			}
			*payload_length = 2;
            /*uprintf("API=>Get knob state:\r\n");
            if (*payload_length == 0) {
                *send_status = ione_api_status_success;
                *send_data = knobState;
                uprintf("Current knob state: %d\r\n", knobState);
            } else {
                *send_status = ione_api_status_fail;
                uprintf("Fail\r\n");
            }
            *payload_length = 2;*/
		break;
#ifdef buttonLock
        case ione_api_cmd_set_lock_state:
            uprintf("API=>Set Rapid Lock State:\r\n");
            if (*payload_length == 1) {      
                *send_status = ione_api_status_success;
                uint8_t mask = recive_payload[0];
                winLock    = (mask & 0x01u) != 0;
                altTabLock = (mask & 0x02u) != 0;
                altF4Lock  = (mask & 0x04u) != 0;
            } else {
                *send_status = ione_api_status_fail;
                uprintf("Payload should be empty\r\n");
            }
        break;
		case ione_api_cmd_get_lock_state:
            uprintf("API=>Get Lock state:\r\n");
            if (*payload_length == 0) {
                *send_status = ione_api_status_success;
                uint8_t mask =  (winLock    ? 1u : 0u)       |   // bit0
                                (altTabLock ? 1u : 0u) << 1  |   // bit1
                                (altF4Lock  ? 1u : 0u) << 2 ;
                *send_data = mask;
                uprintf("Current Lock state: %d\r\n", mask);
            } else {
                *send_status = ione_api_status_fail;
                uprintf("Fail\r\n");
            }
            *payload_length = 2;
		break;
#endif // buttonLock
		case ione_api_cmd_set_rapid_trigger_state:
        uprintf("API=>Set Rapid Trigger State:\r\n");
            if (*payload_length == 1) {             
                *send_status = ione_api_status_success;
                rapidKeyEnable = recive_payload[0];
            } else {
                *send_status = ione_api_status_fail;
                uprintf("Payload should be empty\r\n");
            }
            *payload_length = 1;
		break;
		case ione_api_cmd_get_rapid_trigger_state:
            uprintf ("API=>Get Rapid Trigger State:\r\n");
			if (*payload_length == 0){
			*send_status = ione_api_status_success;
			*send_data = rapidKeyEnable; // test for rapid trigger state
			uprintf ("Get Rapid Trigger State:%d\r\n",*send_data++);
			} else {
				*send_status = ione_api_status_fail;
				uprintf ("Fail\r\n");
			}
			*payload_length = 2;
		break;
		case ione_api_cmd_set_rapid_trigger_value:
            uprintf("API=>Set rapid trigger value:\r\n");
            if (*payload_length == 4) {             
                *send_status = ione_api_status_success;              
                uprintf ("recive_payload:%d\r\an",recive_payload[0]);

                // uprintf("triggerlevel: %d\r\n", keyMatrixTriggerLevel);
                keyMatrixTriggerLevel = ((uint16_t)recive_payload[0] << 8) | recive_payload[1];
                keyMatrixReleaseLevel = ((uint16_t)recive_payload[2] << 8) | recive_payload[3]; 
                uprintf("triggerlevel: %d\r\n", keyMatrixTriggerLevel); 
                uprintf("ReleaseLevel: %d\r\n", keyMatrixReleaseLevel); 

                // Trigger level
                if (keyMatrixTriggerLevel>TriggerMax || keyMatrixTriggerLevel<TriggerMin){
                    keyMatrixTriggerLevel = TriggerInitial;
                    // eeprom_write_word(EECONFIG_KEYMAP_TRIGGER_LEVEL, keyMatrixTriggerLevel);
                }

                //Release level
                if (keyMatrixReleaseLevel>ReleaseMax || keyMatrixReleaseLevel<ReleaseMin || keyMatrixReleaseLevel>=keyMatrixTriggerLevel){
                    keyMatrixReleaseLevel = ReleaseInitial;
                    // eeprom_write_word(EECONFIG_KEYMAP_RELEASE_LEVEL, keyMatrixReleaseLevel);
                }
                eeprom_write_word(EECONFIG_KEYMAP_TRIGGER_LEVEL, keyMatrixTriggerLevel);
	            eeprom_write_word(EECONFIG_KEYMAP_RELEASE_LEVEL, keyMatrixReleaseLevel);
                write_trigger_to_eeprom();
	            write_Release_to_eeprom();
            } else {
                *send_status = ione_api_status_fail;
                // uprintf("Payload should be 6\r\n");
            }
            *payload_length = 1;
		break;
		case ione_api_cmd_get_Rapid_trigger_value:
            uprintf ("API=>Get Rapid Trigger Value:\r\n");
			if (*payload_length == 0){
			*send_status = ione_api_status_success;
			*send_data = (keyMatrixTriggerLevel >> 8) & 0xFF;// press high byte
            // uprintf ("Get Rapid Trigger value 1:%d\r\n",*send_data);
            send_data++;
            *send_data = keyMatrixTriggerLevel & 0xFF;// press low byte
            // uprintf ("Get Rapid Trigger value 2:%d\r\n",*send_data);
            send_data++;
            *send_data = (keyMatrixReleaseLevel >> 8) & 0xFF;// release high byte
            // uprintf ("Get Rapid Trigger value 3:%d\r\n",*send_data);
            send_data++;
            *send_data = keyMatrixReleaseLevel & 0xFF;// release low byte
			// uprintf ("Get Rapid Trigger value 4:%d\r\n",*send_data);
            } else {
				*send_status = ione_api_status_fail;
				uprintf ("Fail\r\n");
			}
			*payload_length = 5;
		break;

		case ione_api_cmd_set_tachyon_mode:
            uprintf("API=>Set tachyon_mode:\r\n");
            if (*payload_length == 1) {             
                *send_status = ione_api_status_success;     
                if(recive_payload[0] == 0x01){
                    currentLED_RunMode.mode = 1;
                    setRunMode(currentLED_RunMode);  
                } else {
                    currentLED_RunMode.mode = 8;
                    setRunMode(currentLED_RunMode);  
                }
            } else {
                *send_status = ione_api_status_fail;
                uprintf("Failed\r\n");
            }
            *payload_length = 1;
		break;
		case ione_api_cmd_tachyon_mode:
            uprintf("API=>get tachyon_mode:\r\n");
            if (*payload_length == 0) {             
                *send_status = ione_api_status_success;     
                if(currentLED_RunMode.mode == 1){
                    *send_data = 0x01;
                } else {
                    *send_data = 0x00;
                }
                        
            } else {
                *send_status = ione_api_status_fail;
                uprintf("Failed\r\n");
            }
            *payload_length = 2;
		break;
#ifdef socd
		case ione_api_cmd_set_snap_tap_SOCD_Binding:
            uprintf("API=>Set Snap Tap SOCD Binding:\r\n");
            uprintf("payload_length is %d", *payload_length);
            if (*payload_length == 8) {//s
                *send_status = ione_api_status_success;              
                uprintf ("recive_payload:%d\r\n",recive_payload[0]);
                socd_apply_report(&recive_payload[0]);
            } else {
                *send_status = ione_api_status_fail;
                uprintf("Failed\r\n");
            }
            *payload_length = 1;
		break;
		case ione_api_cmd_get_snap_tap_SOCD_Binding:
            uprintf ("API=>Get Snap Tap SOCD Binding:\r\n");
			if (*payload_length == 0){
			*send_status = ione_api_status_success;
            *send_data = socd_fill_report(send_data);
            } else {
				*send_status = ione_api_status_fail;
				uprintf ("Fail\r\n");
			}
			*payload_length = 10;
            
		break;
        case ione_api_test:
            // eeconfig_write_socd(0, &cfg);
            // socd_test_eeprom();
        break;
#endif //socd
#ifdef socd_void
		case ione_api_cmd_set_snap_tap_SOCD_Binding:
            uprintf("API=>Set Snap Tap SOCD Binding:\r\n");
            if (*payload_length == 8) {             
                *send_status = ione_api_status_success;
                recive_payload[0]; // primary layer
                firstSnap.event1.key.row = recive_payload[1];
                firstSnap.event1.key.col = recive_payload[2];
                recive_payload[3]; // secondary layer
                firstSnap.event2.key.row = recive_payload[4];
                firstSnap.event2.key.col = recive_payload[5];
                firstSnap.type =  recive_payload[6];
                recive_payload[7]; // alternative fully pressed behaviour on/off
            } else {
                *send_status = ione_api_status_fail;
                uprintf("Payload should be 6\r\n");
            }
            *payload_length = 1;
		break;
		case ione_api_cmd_get_snap_tap_SOCD_Binding:
            uprintf ("API=>Get Snap Tap SOCD Binding:\r\n");
			if (*payload_length == 0){
			*send_status = ione_api_status_success;
			*send_data = 0xaa; // index
            send_data++;
            *send_data = 0xa1; // primary layer
            send_data++;
            *send_data = firstSnap.event1.key.row; // primary row
            send_data++;
            *send_data = firstSnap.event1.key.col; // primary col
            send_data++;
            *send_data = 0xa2; // secondary layer
            send_data++;
            *send_data = firstSnap.event2.key.row; // secondary row
            send_data++;
            *send_data = firstSnap.event2.key.col; // secondary col
            send_data++;
            *send_data = firstSnap.type; // type of SOCD
            send_data++;
            *send_data = 0xa3; // alternative fully pressed behaviour on/off
			} else {
				*send_status = ione_api_status_fail;
				uprintf ("Fail\r\n");
			}
			*payload_length = 10;
		break;
#endif
		case ione_api_cmd_set_dynamic_keystroke:
        uprintf("API=>Set dynamic keystroke:\r\n");
            if (*payload_length == 9) {             
                *send_status = ione_api_status_success;              
                // uprintf ("recive_payload:%d\r\n",recive_payload[0]);

                // uprintf("triggerlevel: %d\r\n", keyMatrixTriggerLevel);
                // keyMatrixTriggerLevel = ((uint16_t)recive_payload[0] << 8) | recive_payload[1];
                // keyMatrixReleaseLevel = ((uint16_t)recive_payload[2] << 8) | recive_payload[3]; 
                // uprintf("triggerlevel: %d\r\n", keyMatrixTriggerLevel); 
                // uprintf("ReleaseLevel: %d\r\n", keyMatrixReleaseLevel); 

                for (uint8_t i=0;i<RAPID_TRIGGER_COUNT;i++){ //logic have some issue
                    if(rapidTriggerKey[i].key.row == recive_payload[1] && rapidTriggerKey[i].key.col == recive_payload[2]){
                        rapidTriggerKey[i].key.keycode[0] == recive_payload[3];   // no.1
                        rapidTriggerKey[i].key.keycode[1] == recive_payload[4];  // no.2
                        rapidTriggerKey[i].key.keycode[2] == recive_payload[5];  // no.3
                        rapidTriggerKey[i].key.keycode[3] == recive_payload[6];  // no.4
                    } else {
                        // *w++ = 0xaa;// = mt_map[i].L;      // layer
                        // rapidTriggerKey[i].key.row == recive_payload[1];      // row
                        // rapidTriggerKey[i].key.col == recive_payload[2];      // col
                        // rapidTriggerKey[i].key.keycode[0] == recive_payload[3];   // no.1
                        // rapidTriggerKey[i].key.keycode[1] == recive_payload[4];  // no.2
                        // rapidTriggerKey[i].key.keycode[2] == recive_payload[5];  // no.3
                        // rapidTriggerKey[i].key.keycode[3] == recive_payload[6];  // no.4
                    }
                }

            } else {
                *send_status = ione_api_status_fail;
                // uprintf("Payload should be 6\r\n");
            }
            *payload_length = 1;
		break;
		case ione_api_cmd_get_dynamic_keysytoke:
            if (*payload_length == 0) {
                uprintf("layer 1\r\n");
                uint8_t payload_length_count = 0;
                const size_t EP = LAMP_ARRAY_EPSIZE;   // 32 或 64，依你的端點大小
                uint8_t *w;                            // 寫入指標（每包都重設）

                // rapidTrigger_config_t rapidTrigger;

                for (uint8_t i=0;i<RAPID_TRIGGER_COUNT;i++){ // print dynamic keystroke example - lyon
                    // uprintf("i = %d, enable status = %d\r\n",i,rapidTrigger.enable);

                    // uprintf ("enable:%d row:%d col:%d counts:%d code[0]:%x code[1]:%x code[2]:%x code[3]:%x code[4]:%x code[5]:%x\r\n",
                    //     	rapidTrigger.enable,rapidTrigger.row,rapidTrigger.col,rapidTrigger.counts
                    //     	,rapidTrigger.keycode[0],rapidTrigger.keycode[1],rapidTrigger.keycode[2],rapidTrigger.keycode[3],rapidTrigger.keycode[4],rapidTrigger.keycode[5]);
                    
                    if(rapidTriggerKey[i].key.enable == TRUE){
                        uprintf("layer 2\r\n");
                        *(data_start + 3) |= 0x80;
                        w = send_data;
                        *send_status = ione_api_status_sending;

                        *w++ = 0xaa;// = mt_map[i].L;      // layer
                        *w++ = rapidTriggerKey[i].key.row;      // row
                        *w++ = rapidTriggerKey[i].key.col;      // col
                        *w++ = rapidTriggerKey[i].key.keycode[0];   // no.1
                        *w++ = rapidTriggerKey[i].key.keycode[1];  // no.2
                        *w++ = rapidTriggerKey[i].key.keycode[2];  // no.3
                        *w++ = rapidTriggerKey[i].key.keycode[3];  // no.4

                        *payload_length = (uint8_t)(1 + (w - send_data));  // 1 = status
                        raw_hid_send(data_start, EP);  
                        payload_length_count+=7;  

                        // uprintf ("enable:%d row:%d col:%d counts:%d code[0]:%x code[1]:%x code[2]:%x code[3]:%x code[4]:%x code[5]:%x\r\n",
                        // 	rapidTrigger.enable,rapidTrigger.row,rapidTrigger.col,rapidTrigger.counts
                        // 	,rapidTrigger.keycode[0],rapidTrigger.keycode[1],rapidTrigger.keycode[2],rapidTrigger.keycode[3],rapidTrigger.keycode[4],rapidTrigger.keycode[5]);
                    }
                }
            } else {
                *send_status = ione_api_status_fail;
            }
            *payload_length = payload_length_count;
		break;
#ifdef toggleKey
		case ione_api_cmd_set_toggle_key: //
            uprintf("API=>Set Toggle Key:\r\n");
            // payload: [0]=layer, [1]=row, [2]=col, [3]=keycode
            if (*payload_length == 4 &&
                recive_payload[0] < KEYMAPS_COUNT &&
                recive_payload[1] < MATRIX_ROWS &&
                recive_payload[2] < MATRIX_COLS) {

                uint8_t layer   = recive_payload[0];
                uint8_t row     = recive_payload[1];
                uint8_t col     = recive_payload[2];
                uint8_t keycode = recive_payload[3];

                toggle_key_set(layer, row, col, keycode);

                *send_status    = ione_api_status_success;
                *payload_length = 1;

                uprintf("  L:%u R:%u C:%u KC:0x%02X\r\n",
                        layer, row, col, keycode);
            } else {
                *send_status    = ione_api_status_fail;
                *payload_length = 1;
                uprintf("  Fail (len=%u)\r\n", *payload_length);
            }
		break;
		case ione_api_cmd_get_toggle_key:
            uprintf("API=>Get Toggle Key:\r\n");
            // request payload: [0]=layer, [1]=row, [2]=col
            if (*payload_length == 3 &&
                recive_payload[0] < KEYMAPS_COUNT &&
                recive_payload[1] < MATRIX_ROWS &&
                recive_payload[2] < MATRIX_COLS) {

                uint8_t layer = recive_payload[0];
                uint8_t row   = recive_payload[1];
                uint8_t col   = recive_payload[2];

                uint8_t keycode = toggle_key_get(layer, row, col);

                *send_status = ione_api_status_success;
                *payload_length = 4;

                *send_data++ = layer;
                *send_data++ = row;
                *send_data++ = col;
                *send_data++ = keycode;

                uprintf("  L:%u R:%u C:%u KC:0x%02X\r\n",
                        layer, row, col, keycode);
            } else {
                *send_status    = ione_api_status_fail;
                *payload_length = 1;
                uprintf("  Fail (len=%u)\r\n", *payload_length);
            }
		break;
#endif // toggleKey
#ifdef modTap
		case ione_api_cmd_set_mod_tap:
            uprintf("API=>Set mod Tap State:\r\n");
            if (*payload_length == 5) {             
                *send_status = ione_api_status_success;
                uint8_t layer, row, col, hold, click;
                layer = recive_payload[0];
                row = recive_payload[1];
                col = recive_payload[2];
                hold = recive_payload[3];
                click = recive_payload[4];
                mt_schedule_set(layer, row, col, hold, click);
            } else {
                *send_status = ione_api_status_fail;
                uprintf("Payload should be 6\r\n");
            }
            *payload_length = 1;
		break;
		case ione_api_cmd_get_mod_tap:
            if (*payload_length == 0) {
                const size_t EP = LAMP_ARRAY_EPSIZE;   // 32 或 64，依你的端點大小
                uint8_t *w;                            // 寫入指標（每包都重設）

                uint8_t payload_length_count = 0;

                for (int i=0;i<MT_CAP_MAP;i++){
                    if (mt_map[i].used){
                        *(data_start + 3) |= 0x80;
                        w = send_data;
                        *send_status = ione_api_status_sending;
                        *w++ = mt_map[i].L;      // layer
                        *w++ = mt_map[i].R;      // row
                        *w++ = mt_map[i].C;      // col
                        *w++ = mt_map[i].hold;   // hold
                        *w++ = mt_map[i].click;  // click

                        *payload_length = (uint8_t)(1 + (w - send_data));  // 1 = status
                        raw_hid_send(data_start, EP);  
                        payload_length_count+=5;  
                    }
                        // uprintf("[MT] MAP[%d]: L%u R%u C%u hold=0x%02X click=0x%02X\r\n",
                        //         i, mt_map[i].L, mt_map[i].R, mt_map[i].C, mt_map[i].hold, mt_map[i].click);
                }
            } else {
                *send_status = ione_api_status_fail;
            }
            *payload_length = payload_length_count;
            break;
        #ifdef modTap_test
        {
            if (*payload_length == 0) {
            const size_t EP = LAMP_ARRAY_EPSIZE;   // 32 或 64，依你的端點大小
            uint8_t *w;                            // 寫入指標（每包都重設）
            // 把這個 Command 標記為「回覆」
            *(data_start + 3) |= 0x80;             // CMD_H |= 0x80

            /* -------- 第 1 包 -------- */
            w = send_data;                         // payload+1（status 後面）
            *send_status = ione_api_status_sending;
            *w++ = 0x01;   // index
            *w++ = 0x02;   // primary layer
            *w++ = 0x03;   // primary row
            *w++ = 0x04;   // primary col
            *w++ = 0x05;   // secondary layer
            *w++ = 0x06;   // secondary row
            *w++ = 0x08;   // secondary col
            *w++ = 0x09;   // type of SOCD
            *w++ = 0x0A;   // alt fully pressed on/off
            *payload_length = (uint8_t)(1 + (w - send_data));  // 1 = status
            raw_hid_send(data_start, EP);          // ← 傳整包（從 header 起）

            /* -------- 第 2 包 -------- */
            w = send_data;                         // 重設回 payload 起點+1
            *send_status = ione_api_status_sending;
            *w++ = 0xA1;  // index
            *w++ = 0xA2;  // primary layer
            *w++ = 0xA3;  // primary row
            *w++ = 0xA4;  // primary col
            *w++ = 0xA5;  // secondary layer
            *w++ = 0xA6;  // secondary row
            *w++ = 0xA7;  // secondary col
            *w++ = 0xA8;  // type of SOCD
            *w++ = 0xA9;  // alt fully pressed on/off
            *payload_length = (uint8_t)(1 + (w - send_data));
            raw_hid_send(data_start, EP);

            w = send_data;
            *send_status = ione_api_status_success;
            *w++ = 0xB1;  // index
            *w++ = 0xB2;  // primary layer
            *w++ = 0xB3;  // primary row
            *w++ = 0xB4;  // primary col
            *w++ = 0xB5;  // secondary layer
            *w++ = 0xB6;  // secondary row
            *w++ = 0xB7;  // secondary col
            *w++ = 0xB8;  // type of SOCD
            *w++ = 0xB9;  // alt fully pressed on/off
            *payload_length = (uint8_t)(1 + (w - send_data));

            break;
        }
        break;}
        #endif
#endif // modTap
#ifdef rappySnappy
		// case ione_api_cmd_set_rappy_snappy:
        //     uprintf("API=>Set Rapid Snappy State:\r\n");
        //     if (*payload_length == 6) {             
        //         *send_status = ione_api_status_success;
        //         recive_payload[0];
        //         snappy_key1.row = recive_payload[1];
        //         snappy_key1.col = recive_payload[2];
        //         recive_payload[3];
        //         snappy_key2.row = recive_payload[4];
        //         snappy_key2.col = recive_payload[5];
        //     } else {
        //         *send_status = ione_api_status_fail;
        //         uprintf("Payload should be 6\r\n");
        //     }
        //     *payload_length = 1;
		// break;
		// case ione_api_cmd_get_rappy_snappy:
        // uprintf ("API=>Get Rapid Snappy key:\r\n");
		// 	if (*payload_length == 0){
		// 	*send_status = ione_api_status_success;
		// 	*send_data = snappy_key1.layer;     //key1 layer
        //     send_data++;
        //     *send_data = snappy_key1.row;       //key1 row
        //     send_data++;
        //     *send_data = snappy_key1.col;       //key1 col
        //     send_data++;
        //     *send_data = snappy_key2.layer;     //key2 layer
        //     send_data++;
        //     *send_data = snappy_key2.row;       //key2 row
        //     send_data++;
        //     *send_data = snappy_key2.col;       //key2 col
        //     send_data++;
		// 	// uprintf ("Get Rapid Snappy key:%d\r\n",*send_data++);
		// 	} else {
		// 		*send_status = ione_api_status_fail;
		// 		uprintf ("Fail\r\n");
		// 	}
		// 	*payload_length = 7;
		// break;
#endif // rappySnappy
		case ione_api_cmd_set_factory_reset:
			uprintf("API=>Factory Reset:\r\n");
			if (*payload_length == 0){
				*payload_length = 1;
				*send_status = ione_api_status_success;
				uprintf("Start ");
				currentLED_RunMode.mode = 8;
				currentLED_RunMode.type = offMode;
				setRunMode(currentLED_RunMode);
				uprintf (".");
				*send_status = ione_api_status_success;
				eeconfig_rapidTrigger_init();
				keyMatrixTriggerLevel = DEFAULT_TRIGGER_LEVEL;
				keyMatrixReleaseLevel = DEFAULT_RELEASE_LEVEL;
				eeprom_write_word(EECONFIG_KEYMAP_TRIGGER_LEVEL, keyMatrixTriggerLevel);
				uprintf (".");
				eeprom_write_word(EECONFIG_KEYMAP_RELEASE_LEVEL, keyMatrixReleaseLevel);
				uprintf (".");
				write_trigger_to_eeprom();
				uprintf (".");
				write_Release_to_eeprom();
				uprintf (".");
				eeconfig_rapidTrigger_init();
				uprintf (".");
				write_keymap_to_eeprom();
				uprintf (".");
				dynamic_macro_reset_eeprom();
				uprintf (".");
				*(data_start+3) |= 0x80;
				raw_hid_send(data_start, length);
				uprintf (".");
				uprintf("\r\nEnd\r\n");
				chThdSleepMilliseconds(1000);
				NVIC_SystemReset();
				return false;
			} else {
				*send_status = ione_api_status_fail;
				uprintf ("Fail\r\n");
			}
			*payload_length = 1;
		break;
		case ione_api_cmd_set_test_mode:
		break;

        case ione_api_cmd_get_eeprom_max_trigger_value:
            uprintf("API=>Get EEPROM Max Trigger Value:\r\n");
            if (*payload_length == 0){
                uint8_t payload_length_count = 0;
                const size_t EP = LAMP_ARRAY_EPSIZE;   // 32 或 64，依你的端點大小
                uint8_t *w;  
                uint16_t i = 0; // count

                if(*payload_length == 0){

                    for(uint8_t rowNum = 0; rowNum < MATRIX_ROWS; rowNum++){
                        for(uint8_t colNum = 0; colNum < MATRIX_COLS; colNum++){
                            *(data_start + 3) |= 0x80;
                            w = send_data;
                            *send_status = ione_api_status_sending;

                            uint16_t val = eeprom_read_word(EECONFIG_KEYMAP_TRAVEL_CALIBRATION + i * 2);
                            uint8_t high = (val >> 8) & 0xFF;
                            uint8_t low  = val & 0xFF;
                            uprintf("number %d (%d,%d)value read is %u\r\n",i,rowNum,colNum,val);//

                            *w = rowNum;        // Row
                            w++;
                            *w = colNum;        // Column
                            w++;
                            *w = high;   // level first byte
                            w++;
                            *w = low;   // level second byte
                            w++;

                            i++;

                            *payload_length = (uint8_t)(1 + (w - send_data));  // 1 = status
                            raw_hid_send(data_start, EP);
                            payload_length_count+=4;   
                        }
                    }

                *send_status = ione_api_status_success;
                } else {
                    *send_status = ione_api_status_fail;
                    uprintf ("Fail\r\n");
                }
            }
        break;
        
		default:
			return false;
	}
	//*payload_length = send_data-send_status;


//    for (uint8_t i=0;i<payload_length;i++){
//    	uprintf ("%02X ",send_payload[i]);
//    }
//    uprintf ("\r\n");
	*(data_start+3) |= 0x80;
    return true;
}

void raw_hid_receive(uint8_t *data, uint8_t length) {
    uint8_t *command_id   = &(data[0]);
    uint8_t *command_data = &(data[1]);
    // uprintf("raw_hid_receive command_id: %u, data 0: %u, data 1: %u\r\n",*command_id,data[0],data[1]); //
    switch (*command_id) {
        case id_get_protocol_version: {
            command_data[0] = VIA_PROTOCOL_VERSION >> 8;
            command_data[1] = VIA_PROTOCOL_VERSION & 0xFF;
            break;
        }
        case id_get_keyboard_value: {
            switch (command_data[0]) {
                case id_uptime: {
                    uint32_t value  = timer_read32();
                    command_data[1] = (value >> 24) & 0xFF;
                    command_data[2] = (value >> 16) & 0xFF;
                    command_data[3] = (value >> 8) & 0xFF;
                    command_data[4] = value & 0xFF;
                    break;
                }
                case id_layout_options: {
                    //uint32_t value  = via_get_layout_options();
                	uint32_t value  = 0x000000;
                    command_data[1] = (value >> 24) & 0xFF;
                    command_data[2] = (value >> 16) & 0xFF;
                    command_data[3] = (value >> 8) & 0xFF;
                    command_data[4] = value & 0xFF;
                    break;
                }
                case id_switch_matrix_state: {
//#ifdef VIAL_ENABLE
//                    /* Disable wannabe keylogger unless unlocked */
//                    if (!vial_unlocked)
//                        goto skip;
//#endif

#if ((MATRIX_COLS / 8 + 1) * MATRIX_ROWS <= 28)
                    uint8_t i = 1;
                    for (uint8_t row = 0; row < MATRIX_ROWS; row++) {
                        matrix_row_t value = matrix_get_row(row);
#    if (MATRIX_COLS > 24)
                        command_data[i++] = (value >> 24) & 0xFF;
#    endif
#    if (MATRIX_COLS > 16)
                        command_data[i++] = (value >> 16) & 0xFF;
#    endif
#    if (MATRIX_COLS > 8)
                        command_data[i++] = (value >> 8) & 0xFF;
#    endif
                        command_data[i++] = value & 0xFF;
                    }
#endif
                    break;
                }

                case id_firmware_version:	{
					uint32_t blVersion;
					switch (command_data[1])
					{
					case 0:
						command_data[2] = VENDOR_ID & 0x0ff;
						command_data[3] = VENDOR_ID >> 8;
						break;
					case 1:
						command_data[2] = PRODUCT_ID & 0x0ff;
						command_data[3] = PRODUCT_ID >> 8;
						break;
					case 2:
						command_data[2] = DEVICE_VER & 0x0ff;
						command_data[3] = DEVICE_VER >> 8;
						break;
					case 3:
						blVersion = *((uint32_t *)(0x0800580c));
						command_data[2] = blVersion & 0xff;
						command_data[3] = (blVersion >> 8) & 0xff;
						break;
					}
				}



                	break;
                default: {
                    // uprintf("raw_hid_receive call user\r\n");
                    raw_hid_receive_kb(data, length);
                    break;
                }
            }
            break;
        }
        case id_set_keyboard_value: {
            switch (command_data[0]) {
//                case id_layout_options: {
//                    uint32_t value = ((uint32_t)command_data[1] << 24) | ((uint32_t)command_data[2] << 16) | ((uint32_t)command_data[3] << 8) | (uint32_t)command_data[4];
//                    via_set_layout_options(value);
//                    break;
//                }
            	case id_control:
            		via_controlSet(&command_data[1]);
            		break;
                default: {
                    raw_hid_receive_kb(data, length);
                    break;
                }
            }
            break;
        }
        case id_dynamic_keymap_get_keycode: {
        	uint16_t keycode = eepromKeyBuffer[command_data[0]][command_data[1]][command_data[2]];
            command_data[3]  = keycode >> 8;
            command_data[4]  = keycode & 0xFF;
            break;
        }
        case id_dynamic_keymap_set_keycode: {
        	eeconfig_write_keymap_key(command_data[0], command_data[1], command_data[2], (command_data[3] << 8) | command_data[4]);
            break;
        }
        case id_dynamic_keymap_reset: {
        	write_keymap_to_eeprom();
            break;
        }
        case id_lighting_set_value:
        	switch (command_data[0]){
        		case id_light_runMode:
        			setLightRunMode(command_data[1]);
        			break;
        		case id_light_mode:
                	if (command_data[1]!=volSpdMode){
                        currentLED_RunMode.type = command_data[1];
                        currentLED_RunMode.mode = command_data[2];
                        currentLED_RunMode.brightness = command_data[3];
                        currentLED_RunMode.speed = command_data[4];
                        setRunMode(currentLED_RunMode);

                	} else {
                        currentLED_RunMode.brightness = command_data[3];
                        currentLED_RunMode.speed = command_data[4];
                        LED_SpeedChange(currentLED_RunMode.brightness, currentLED_RunMode.speed);
                	}

        			break;
        	}
            break;

        case id_lighting_get_value:
            break;

//#ifdef VIA_EEPROM_ALLOW_RESET
        case id_eeprom_reset: {
//        	write_keymap_to_eeprom();
            eeconfig_rapidTrigger_init();
            dynamic_macro_reset_eeprom();
#ifdef minTriggerCheck

            keyMatrixTriggerLevel = 3;
			keyMatrixReleaseLevel = 0;
#else
            keyMatrixTriggerLevel = DEFAULT_TRIGGER_LEVEL;
			keyMatrixReleaseLevel = DEFAULT_RELEASE_LEVEL;
#endif
    		eeprom_write_word(EECONFIG_KEYMAP_TRIGGER_LEVEL, keyMatrixTriggerLevel);
    		eeprom_write_word(EECONFIG_KEYMAP_RELEASE_LEVEL, keyMatrixReleaseLevel);

            write_trigger_to_eeprom();
            write_Release_to_eeprom();
            eeconfig_rapidTrigger_init();
            NVIC_SystemReset();
            break;
        }
//#endif
        case id_dynamic_keymap_macro_get_count: {
            command_data[0] = DYNAMIC_MACRO_COUNT;
            break;
        }
        case id_dynamic_keymap_macro_get_buffer_size: {
            //uint16_t size = DYNAMIC_MACRO_COUNT * DYNAMIC_MACRO_SIZE;		//!!
            //uint16_t size = DYNAMIC_KEYMAP_MACRO_EEPROM_SIZE;
        	uint16_t size = 10;
            command_data[0] = size >> 8;
            command_data[1] = size & 0xFF;
            break;
        }
        case id_dynamic_keymap_macro_get_buffer: {
            uint16_t offset = (command_data[0] << 8) | command_data[1];
            uint16_t size   = command_data[2]; // size <= 28
            if (size <= 28)
                dynamic_keymap_macro_get_buffer(offset, size, &command_data[3]);
            break;
        }
        case id_dynamic_keymap_macro_set_buffer: {
//#ifdef VIAL_ENABLE
//            /* Until keyboard is unlocked, don't allow changing macros */
//            if (!vial_unlocked)
//                goto skip;
//#endif
            uint16_t offset = (command_data[0] << 8) | command_data[1];
            uint16_t size   = command_data[2]; // size <= 28
            if (size <= 28)
                dynamic_keymap_macro_set_buffer(offset, size, &command_data[3]);	//!!
            	command_data[3] = 100;
            break;
        }
        case id_dynamic_keymap_macro_reset: {
            dynamic_keymap_macro_reset();		//!!
            break;
        }
        case id_dynamic_keymap_get_layer_count: {
//            command_data[0] = dynamic_keymap_get_layer_count();		//!!
        	command_data[0] = KEYMAPS_COUNT;
            break;
        }
        case id_dynamic_keymap_get_buffer: {
            uint16_t offset = (command_data[0] << 8) | command_data[1];
            uint16_t size   = command_data[2]; // size <= 28
            if (size <= 28)
                dynamic_keymap_get_buffer(offset>>1, size>>1, &command_data[3]);		//!!
            break;
        }
        case id_dynamic_keymap_set_buffer: {
            uint16_t offset = (command_data[0] << 8) | command_data[1];
            uint16_t size   = command_data[2]; // size <= 28
            if (size <= 28)
                dynamic_keymap_set_buffer(offset>>1, size>>1, &command_data[3]);		//!!
            break;
        }
        case id_loga_api: //
            // uprintf("call ione\r\n");
        	if (!raw_hid_receive_ione(data, length)){
        		return false;
        	}
        	break;
        /* As VIA removed bootloader jump entirely, we shall only keep it for secure builds */
        case id_bootloader_jump: {
            /* Until keyboard is unlocked, don't allow jumping to bootloader */
            // Need to send data back before the jump
            // Informs host that the command is handled
            raw_hid_send(data, length);
            // Give host time to read it
            chThdSleepMilliseconds(100);

            switch (command_data[0])
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


//            bootloader_jump();
            break;
        }


//#if defined(VIAL_ENABLE) && !defined(VIAL_INSECURE)
//        /* As VIA removed bootloader jump entirely, we shall only keep it for secure builds */
//        case id_bootloader_jump: {
//            /* Until keyboard is unlocked, don't allow jumping to bootloader */
//            if (!vial_unlocked)
//                goto skip;
//            // Need to send data back before the jump
//            // Informs host that the command is handled
//            raw_hid_send(data, length);
//            // Give host time to read it
//            wait_ms(100);
//            bootloader_jump();
//            break;
//        }
//#endif
//#ifdef VIAL_ENABLE
//        case id_vial_prefix: {
//            vial_handle_cmd(data, length);
//            break;
//        }
//#endif
//        default: {
//            // The command ID is not known let the keyboard implement it
//            raw_hid_receive_kb(data, length);
//            break;
//        }
        default:
            // uprintf("call kb\r\n");
            if (!raw_hid_receive_kb(data, length)){return;}
            break;
    }
#ifdef VIAL_ENABLE
skip:
#endif
    // Return the same buffer, optionally with values changed
    // (i.e. returning state to the host, or the unhandled state).
    raw_hid_send(data, length);
}

