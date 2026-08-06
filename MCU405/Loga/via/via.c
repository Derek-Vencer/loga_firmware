# include "hal.h"
# include "keymap_common.h"
# include "report.h"
# include "matrix.h"
# include "dynamic_macro.h"
# include "macro_engine.h"
# include "usb_main.h"
# include "../led/led_function.h"
# include "loga_function.h" // lyon for layer_keypos_t
# include "socd.h" 
#include <string.h>
#include "via.h"
#include "rappy_snappy.h"
#include "led/led_function.h"
#include "rappy_snappy.h"
#include "keymap_in_eeprom.h"
# include "loga_function.h" 
#include "led/led_effect.h"
#include "timer.h"
#include "config.h"

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
void eeconfig_modtap_clear(void);
void eeconfig_clear_all_toggle_entry(void);

static uint8_t current_macro_playback_index = 0U;
static uint8_t current_macro_playback_mode  = 0U;

static volatile bool factory_reset_pending = false;
static volatile bool reset_all_key_pending = false;
static volatile bool factory_system_reset_pending = false;
static volatile bool test_mode_enabled = false;

static uint8_t clamp_percent_u8(uint8_t value) {
    return (value > 100u) ? 100u : value;
}

static uint8_t brightness_percent_to_internal(uint8_t percent) {
    return clamp_percent_u8(percent);
}

static uint8_t brightness_internal_to_percent(uint8_t brightness, bool is_off) {
    if (is_off || brightness == 0u) {
        return 0u;
    }

    return clamp_percent_u8(brightness);
}

static uint8_t speed_percent_to_internal(uint8_t percent) {
    return clamp_percent_u8(percent);
}

static uint8_t speed_internal_to_percent(uint8_t speed) {
    return clamp_percent_u8(speed);
}

// extern layer_keypos_t snappy_key1;
// extern layer_keypos_t snappy_key2;
#ifdef rappySnappy
extern rappy_snappy_pair_t g_rappy_pairs[RAPPY_SNAPPY_PAIR_COUNT];
#endif // rappysnappy
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
bool toggle_key_set(uint8_t layer, uint8_t row, uint8_t col, uint8_t keycode);
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

static void factory_reset_execute(void)
{
    uprintf("[FACTORY] Start\r\n");

    knobState = 0U;

    eeprom_update_byte(
        KB_ENCODER_MODE_ADDR,
        knobState
    );

    eeconfig_socd_clear();
    eeconfig_rappy_snappy_clear();
    eeconfig_modtap_clear();
    eeconfig_clear_all_toggle_entry();

    currentLED_RunMode.mode = 8U;
    currentLED_RunMode.type = offMode;

    setRunMode(currentLED_RunMode);

    eeprom_update_byte(
        KB_COLOR_MODE_ADDR,
        currentLED_RunMode.mode
    );

    eeprom_update_byte(
        (uint8_t *)EECONFIG_POLLING_RATE,
        0x00U
    );

    keyMatrixTriggerLevel = DEFAULT_TRIGGER_LEVEL;
    keyMatrixReleaseLevel = DEFAULT_RELEASE_LEVEL;

    eeprom_write_word(
        EECONFIG_KEYMAP_TRIGGER_LEVEL,
        keyMatrixTriggerLevel
    );

    eeprom_write_word(
        EECONFIG_KEYMAP_RELEASE_LEVEL,
        keyMatrixReleaseLevel
    );

    write_trigger_to_eeprom();
    write_Release_to_eeprom();

    eeconfig_rapidTrigger_init();
    write_keymap_to_eeprom();
    dynamic_macro_reset_eeprom();

    eeprom_update_byte(
        (uint8_t *)EEPROM_FACTORY_INIT_FLAG_ADDR,
        0xFAU
    );

    uprintf("[FACTORY] End, system reset\r\n");

    chThdSleepMilliseconds(100U);

    NVIC_SystemReset();

    while (true) {
        /*
         * Wait for the system reset.
         */
    }
}

static THD_WORKING_AREA(waResetAllKeyThread,512);
static THD_FUNCTION(ResetAllKeyThread,arg)
{
    (void)arg;

    chRegSetThreadName("ResetAllKey");

    uprintf("[RESET KEY] Thread started\r\n");

    while (true) {
        if (reset_all_key_pending) {
            /*
             * Allow the USB ACK to finish transmitting first.
             */
            chThdSleepMilliseconds(300U);

            reset_all_key_pending = false;

            uprintf("[RESET KEY] Start\r\n");

            write_keymap_to_eeprom();

            uprintf("[RESET KEY] End\r\n");
        }

        chThdSleepMilliseconds(10U);
    }
}

void reset_all_key_init(void)
{
    chThdCreateStatic(waResetAllKeyThread,sizeof(waResetAllKeyThread),NORMALPRIO,ResetAllKeyThread,NULL);
}

static THD_WORKING_AREA(waFactoryResetThread,512);
static THD_FUNCTION(FactoryResetThread,arg)
{
    (void)arg;

    chRegSetThreadName("FactoryReset");

    uprintf("[FACTORY] Thread started\r\n");

    while (true) {
        if (factory_reset_pending) {
            /*
             * Allow the USB success ACK to be transmitted first.
             */
            chThdSleepMilliseconds(300U);

            factory_reset_pending = false;

            uprintf("[FACTORY] Pending request detected\r\n");

            factory_reset_execute();
        }

        chThdSleepMilliseconds(10U);
    }
}

void factory_reset_init(void)
{
    chThdCreateStatic(waFactoryResetThread,sizeof(waFactoryResetThread),NORMALPRIO,FactoryResetThread,NULL);
}

static void factory_system_reset_execute(void)
{
    uprintf("[FACTORY SYS] Start\r\n");

    macro_engine_stop();

    knobState = 0U;

    eeprom_update_byte(
        KB_ENCODER_MODE_ADDR,
        knobState
    );

    eeconfig_socd_clear();
    eeconfig_rappy_snappy_clear();
    eeconfig_modtap_clear();
    eeconfig_clear_all_toggle_entry();

    write_keymap_to_eeprom();

    dynamic_macro_reset_eeprom();
    macro_delete();

    eeconfig_rapidTrigger_init();

    eeprom_update_byte(
        (uint8_t *)EEPROM_FACTORY_INIT_FLAG_ADDR,
        0xFAU
    );

    uprintf("[FACTORY SYS] End, system reset\r\n");

    chThdSleepMilliseconds(100U);

    NVIC_SystemReset();

    while (true) {
        /*
         * Wait for system reset.
         */
    }
}

static THD_WORKING_AREA(waFactorySystemResetThread,512);
static THD_FUNCTION(FactorySystemResetThread,arg)
{
    (void)arg;

    chRegSetThreadName("FactorySysReset");

    uprintf("[FACTORY SYS] Thread started\r\n");

    while (true) {
        if (factory_system_reset_pending) {
            /*
             * Allow the USB ACK to finish transmitting.
             */
            chThdSleepMilliseconds(300U);

            factory_system_reset_pending = false;

            uprintf(
                "[FACTORY SYS] Pending request detected\r\n"
            );

            factory_system_reset_execute();
        }

        chThdSleepMilliseconds(10U);
    }
}

void factory_system_reset_init(void)
{
    chThdCreateStatic(waFactorySystemResetThread,sizeof(waFactorySystemResetThread),NORMALPRIO,FactorySystemResetThread,NULL);
}

void dynamic_keymap_get_buffer(
    uint16_t offset,
    uint16_t size,
    uint8_t *data
)
{
    const uint16_t *source =
        &eepromKeyBuffer[0][0][0] + offset;

    for (uint16_t i = 0U; i < size; i++)
    {
        uint16_t value = source[i];

        data[(i * 2U) + 0U] =
            (uint8_t)(value >> 8);

        data[(i * 2U) + 1U] =
            (uint8_t)(value & 0xFFU);
    }
}

void dynamic_keymap_set_buffer(
    uint16_t offset,
    uint16_t size,
    const uint8_t *data
)
{
    uint16_t *target =
        &eepromKeyBuffer[0][0][0] + offset;

    for (uint16_t i = 0U; i < size; i++)
    {
        target[i] =
            ((uint16_t)data[(i * 2U) + 0U] << 8) |
            (uint16_t)data[(i * 2U) + 1U];
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
__attribute__((weak))
bool raw_hid_receive_kb(uint8_t *data, uint8_t length)
{
    (void)length;

    if (data == NULL)
    {
        return false;
    }

    data[0] = id_unhandled;

    return true;
}
#ifndef IONE_RAW_HID_PACKET_SIZE
#define IONE_RAW_HID_PACKET_SIZE 32U
#endif

static void ione_prepare_response_header(uint8_t *packet, uint16_t command_id)
{
    packet[3] = (uint8_t)(((command_id >> 8) & 0x7FU) | 0x80U);
    packet[4] = (uint8_t)(command_id & 0xFFU);
}

static void ione_send_stream_packet(
    uint8_t *tx_packet,
    const uint8_t *source,
    size_t tx_length,
    uint16_t command_id)
{
    if ((tx_packet == NULL) || (source == NULL)) {
        return;
    }

    if (tx_length > IONE_RAW_HID_PACKET_SIZE) {
        tx_length = IONE_RAW_HID_PACKET_SIZE;
    }

    memset(tx_packet, 0, IONE_RAW_HID_PACKET_SIZE);
    memcpy(tx_packet, source, tx_length);
    ione_prepare_response_header(tx_packet, command_id);
    raw_hid_send(tx_packet, (uint8_t)tx_length);
    chThdSleepMilliseconds(1);
}

bool raw_hid_receive_ione(uint8_t *data, uint8_t length)
{
    uint8_t *data_start;
    uint8_t tx_packet[IONE_RAW_HID_PACKET_SIZE];
    uint8_t recive_payload[IONE_RAW_HID_PACKET_SIZE - 6U];

    uint16_t hid_vid;
    uint16_t command_id;

    uint8_t *payload_length;
    uint8_t *send_payload;
    uint8_t *send_status;
    uint8_t *send_data;

    const size_t tx_length =
        (length < IONE_RAW_HID_PACKET_SIZE) ?
        (size_t)length :
        (size_t)IONE_RAW_HID_PACKET_SIZE;
		
	
		uprintf("\r\n========== RAW HID RX ==========\r\n");
		uprintf("data=%p len=%d\r\n", data, length);
	
    if ((data == NULL) || (length < 32U))
    {
				uprintf("Invalid packet! data=%p len=%d\r\n", data, length);
        return false;
    }


    data_start = data;
		
		uprintf("RAW:");

		for (uint8_t i = 0; i < length; i++)
		{
				uprintf(" %02X", data_start[i]);
		}

		uprintf("\r\n");

    /*
     * Byte 0~1: HID VID
     */
    hid_vid =
        ((uint16_t)data[0] << 8) |
        (uint16_t)data[1];
    data += 2;
		(void)hid_vid;
		
    /*
     * Byte 2: payload length
     */
    payload_length = data;
    data++;

    /*
     * Byte 3~4: command ID
     */
    command_id =
        ((uint16_t)data[0] << 8) |
        (uint16_t)data[1];
    data += 2;
		
		uprintf("VID      : %04X\r\n", hid_vid);
		uprintf("Payload  : %d\r\n", *payload_length);
		uprintf("Command  : 0x%04X\r\n", command_id);

    /*
     * Byte 5: status
     * Byte 6~31: response data
     */
    send_payload = data;
    send_status  = &data[0];
    send_data    = &data[1];

    /*
     * Incoming payload cannot exceed bytes 5~31.
     */
    if (*payload_length > sizeof(recive_payload))
    {
        *payload_length = 1U;
        *send_status = ione_api_status_fail;
        return true;
    }		

    /*
     * Copy only the valid incoming payload.
     * Clear the whole local buffer first.
     */
    memset(recive_payload, 0, sizeof(recive_payload));

    if (*payload_length > 0U)
    {
        memcpy(recive_payload,
               data,
               *payload_length);
    }

		uprintf("Payload Data:");

		for (uint8_t i = 0; i < *payload_length; i++)
		{
				uprintf(" %02X", recive_payload[i]);
		}

		uprintf("\r\n");
		
    /*
     * Clear response/status area, bytes 5~31.
     */
    memset(send_payload, 0, sizeof(recive_payload));

		uprintf("Dispatch Command 0x%04X\r\n", command_id);
		
    switch (command_id)
    {
        case ione_api_cmd_get_firmware_version:
        {
            uprintf("API=>Get Firmware Version:\r\n");

            if (*payload_length == 0U)
            {
                *send_status = ione_api_status_success;

                send_data[0] = (uint8_t)(DEVICE_VER >> 8);
                send_data[1] = (uint8_t)(DEVICE_VER & 0xFFU);
                send_data[2] = (uint8_t)DEVICE_VER_FIX;

                uprintf("Major Version:%02X ", send_data[0]);
                uprintf("Minor Version:%02X ", send_data[1]);
                uprintf("Build Version:%02X\r\n", send_data[2]);

                /*
                 * status 1 byte + version 3 bytes
                 */
                *payload_length = 4U;
            }
            else
            {
                *send_status = ione_api_status_fail;
                *payload_length = 1U;

                uprintf("Fail\r\n");
            }
        }
        break;
			case ione_api_cmd_Set_Profil:
			{
					uint8_t profile;
					uint8_t target_layer;

					uprintf("API=>Set Profile:\r\n");

					if (*payload_length != 1) {
							*send_status = ione_api_status_fail;

							uprintf("Fail: invalid length=%u\r\n",
											(unsigned int)*payload_length);

							*payload_length = 1;
							break;
					}

					profile = recive_payload[0];

					if ((profile < 1U) || (profile > PROFILE_COUNT)) {
							*send_status = ione_api_status_fail;

							uprintf("Fail: invalid profile=%u\r\n",
											(unsigned int)profile);

							*payload_length = 1;
							break;
					}

					target_layer =
							(uint8_t)((profile - 1U) * LAYERS_PER_PROFILE);

					currentProfile = profile;

					layer_move(target_layer);

					uprintf("Profile:%u Layer:%u\r\n",
									(unsigned int)currentProfile,
									(unsigned int)target_layer);

					clear_keyboard();

					*send_status = ione_api_status_success;
					*payload_length = 1;
			}
			break;

			case ione_api_cmd_get_Profile_state:
					uprintf("API=>Get Profile:\r\n");

					if (*payload_length == 0) {
							*send_status = ione_api_status_success;

							*send_data = currentProfile;

							uprintf("Profile:%u\r\n",
											(unsigned int)*send_data);

							send_data++;
					} else {
							*send_status = ione_api_status_fail;

							uprintf("Fail: invalid length=%u\r\n",
											(unsigned int)*payload_length);
					}

					*payload_length = 2;
					break;
					case ione_api_cmd_set_key_mapping:
					{
							uint8_t layer;
							uint8_t row;
							uint8_t col;
							uint16_t keycode;

							uprintf("API=>Set Key Mapping:\r\n");

							if (*payload_length != 5U) {
									*send_status = ione_api_status_fail;
									*payload_length = 1U;

									uprintf(
											"Fail: invalid payload length=%u\r\n",
											(unsigned int)*payload_length
									);

									break;
							}

							layer = recive_payload[0];
							row   = recive_payload[1];
							col   = recive_payload[2];

							if (layer >= KEYMAPS_COUNT ||
									row >= MATRIX_ROWS ||
									col >= MATRIX_COLS) {

									*send_status = ione_api_status_fail;
									*payload_length = 1U;

									uprintf(
											"Fail: invalid position L=%u R=%u C=%u\r\n",
											(unsigned int)layer,
											(unsigned int)row,
											(unsigned int)col
									);

									break;
							}

							keycode =
									((uint16_t)recive_payload[3] << 8) |
									(uint16_t)recive_payload[4];

							eeconfig_write_keymap_key(
									layer,
									row,
									col,
									keycode
							);

							if (eepromKeyBuffer[layer][row][col] == keycode) {
									*send_status = ione_api_status_success;

									uprintf(
											"Set Key Mapping OK "
											"L=%u R=%u C=%u Key=0x%04X\r\n",
											(unsigned int)layer,
											(unsigned int)row,
											(unsigned int)col,
											(unsigned int)keycode
									);
							} else {
									*send_status = ione_api_status_fail;

									uprintf(
											"Set Key Mapping verify fail "
											"write=0x%04X read=0x%04X\r\n",
											(unsigned int)keycode,
											(unsigned int)eepromKeyBuffer[layer][row][col]
									);
							}

							*payload_length = 1U;
					}
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
		{
				uint8_t *w;

				uprintf("API=>Get All Key Mapping:\r\n");

				if (*payload_length != 0U) {
						*send_status = ione_api_status_fail;
						*payload_length = 1U;

						uprintf(
								"Fail: invalid payload length=%u\r\n",
								(unsigned int)*payload_length
						);
						break;
				}

				for (uint8_t layer_num = 0U;
						 layer_num < KEYMAPS_COUNT;
						 layer_num++) {

						uprintf(
								"=== layer number is %u ===\r\n",
								(unsigned int)layer_num
						);

						for (uint8_t row_num = 0U;
								 row_num < MATRIX_ROWS;
								 row_num++) {

								for (uint8_t col_num = 0U;
										 col_num < MATRIX_COLS;
										 col_num++) {

										uint16_t keycode;
										uint8_t keycode_high;
										uint8_t keycode_low;

										memset(
												send_payload,
												0,
												sizeof(recive_payload)
										);

										w = send_data;
										*send_status = ione_api_status_sending;

										keycode =
												eepromKeyBuffer
														[layer_num]
														[row_num]
														[col_num];

										keycode_high =
												(uint8_t)((keycode >> 8) & 0xFFU);

										keycode_low =
												(uint8_t)(keycode & 0xFFU);

										*w++ = layer_num;
										*w++ = row_num;
										*w++ = col_num;
										*w++ = keycode_high;
										*w++ = keycode_low;

										/*
										 * Payload:
										 * status       1 byte
										 * layer        1 byte
										 * row          1 byte
										 * column       1 byte
										 * keycode high 1 byte
										 * keycode low  1 byte
										 */
										*payload_length =
												(uint8_t)(1U +
												(uint8_t)(w - send_data));

										uprintf(
												"L%u R%u C%u keycode=0x%04X "
												"high=0x%02X low=0x%02X\r\n",
												(unsigned int)layer_num,
												(unsigned int)row_num,
												(unsigned int)col_num,
												(unsigned int)keycode,
												(unsigned int)keycode_high,
												(unsigned int)keycode_low
										);

										ione_send_stream_packet(
												tx_packet,
												data_start,
												tx_length,
												ione_api_cmd_get_all_key_mapping
										);
								}
						}
				}

				memset(
						send_payload,
						0,
						sizeof(recive_payload)
				);

				*send_status = ione_api_status_success;
				*payload_length = 1U;

				uprintf("Get All Key Mapping completed.\r\n");
		}
		break;
        case ione_api_cmd_set_ligthing_effect:
            uprintf("API=>Set Lighting Effect:\r\n");
            
            if (*payload_length == 1) {
                
                *send_status = ione_api_status_success;

                currentLED_RunMode.mode = recive_payload[0];
                setRunMode(currentLED_RunMode);
                eeprom_update_byte(KB_COLOR_MODE_ADDR, recive_payload[0]);
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
                *send_data = eeprom_read_byte(KB_COLOR_MODE_ADDR);//currentLED_RunMode.mode;
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

                //recive_payload[0];
                mosArt.LED.red = recive_payload[1];
                mosArt.LED.green = recive_payload[2];
                mosArt.LED.blue = recive_payload[3];
                //recive_payload[4];
                mosArt.LED.red = recive_payload[5];
                mosArt.LED.green = recive_payload[6];
                mosArt.LED.blue = recive_payload[7];

                eeprom_update_byte(KB_COLOR_RED_ADDR, recive_payload[1]);
                eeprom_update_byte(KB_COLOR_GREEN_ADDR, recive_payload[2]);
                eeprom_update_byte(KB_COLOR_BLUE_ADDR, recive_payload[3]);

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
                *send_data = eeprom_read_byte(KB_COLOR_RED_ADDR);//LED_Datas[0].red;
                send_data++;
                *send_data = eeprom_read_byte(KB_COLOR_GREEN_ADDR);//LED_Datas[0].green;
                send_data++;
                *send_data = eeprom_read_byte(KB_COLOR_BLUE_ADDR);//LED_Datas[0].blue;
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
                eeprom_update_byte(KB_ENCODER_MODE_ADDR, knobState);
            } else {
                *send_status = ione_api_status_fail;
            }
            *payload_length = 1;
        break;
			case ione_api_cmd_konb_state:
			{
					uint8_t knob_mode;

					uprintf("API=>Get Knob State:\r\n");

					if (*payload_length == 0) {

							knob_mode = eeprom_read_byte(KB_ENCODER_MODE_ADDR);

							*send_status = ione_api_status_success;
							*send_data   = knob_mode;

							uprintf("Knob mode:%u\r\n", knob_mode);

							switch (knob_mode) {
									case 0:
											uprintf("Mode = Volume\r\n");
											break;

									case 1:
											uprintf("Mode = Effect\r\n");
											break;

									case 2:
											uprintf("Mode = Light Color\r\n");
											break;

									case 3:
											uprintf("Mode = Multimedia\r\n");
											break;

									default:
											uprintf("Mode = Unknown (%u)\r\n", knob_mode);
											break;
							}

					} else {

							*send_status = ione_api_status_fail;
							uprintf("Fail\r\n");
					}

					*payload_length = 2;
			}
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
                // eeprom_update_byte(KEYLOCK_ADDR, mask);
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
		{
				uint8_t rx_state;
				uint8_t verify_state;

				uprintf("API=>Set Rapid Trigger State:\r\n");

				if (*payload_length != 1U) {
						uint8_t invalid_length = *payload_length;

						*send_status = ione_api_status_fail;
						*payload_length = 1U;

						uprintf(
								"Set Rapid Trigger State Fail: "
								"invalid payload length=%u\r\n",
								(unsigned int)invalid_length
						);

						break;
				}

				rx_state = recive_payload[0];

				/*
				 * Valid protocol values:
				 * 0x00: Disable
				 * 0x01: Enable mode 1
				 * 0x02: Enable mode 2
				 */
				if (rx_state > 2U) {
						*send_status = ione_api_status_fail;
						*payload_length = 1U;

						uprintf(
								"Set Rapid Trigger State Fail: "
								"invalid state=0x%02X\r\n",
								(unsigned int)rx_state
						);

						break;
				}

				uprintf(
						"RX Rapid Trigger State: %u\r\n",
						(unsigned int)rx_state
				);

				eeprom_update_byte(
						(uint8_t *)RAPID_KEY_ENABLE_ADDR,
						rx_state
				);

				verify_state = eeprom_read_byte(
						(const uint8_t *)RAPID_KEY_ENABLE_ADDR
				);

				if (verify_state != rx_state) {
						*send_status = ione_api_status_fail;

						uprintf(
								"Set Rapid Trigger State Fail: "
								"EEPROM verify=%u expected=%u\r\n",
								(unsigned int)verify_state,
								(unsigned int)rx_state
						);
				} else {
						rapidKeyEnable = verify_state;
						*send_status = ione_api_status_success;

						uprintf(
								"Rapid Trigger State Saved: %u\r\n",
								(unsigned int)verify_state
						);
				}

				*payload_length = 1U;
		}
		break;


		case ione_api_cmd_get_rapid_trigger_state:
		{
				uint8_t stored_state;

				uprintf("API=>Get Rapid Trigger State:\r\n");

				if (*payload_length != 0U) {
						uint8_t invalid_length = *payload_length;

						*send_status = ione_api_status_fail;
						*payload_length = 1U;

						uprintf(
								"Get Rapid Trigger State Fail: "
								"payload length=%u, expected=0\r\n",
								(unsigned int)invalid_length
						);

						break;
				}

				stored_state = eeprom_read_byte(
						(const uint8_t *)RAPID_KEY_ENABLE_ADDR
				);

				/*
				 * Valid stored values are 0x00, 0x01 and 0x02.
				 * Erased or invalid EEPROM data is restored to 0x00.
				 */
				if (stored_state > 2U) {
						uprintf(
								"Rapid Trigger State invalid: 0x%02X, "
								"initialize to Disable\r\n",
								(unsigned int)stored_state
						);

						stored_state = 0U;

						eeprom_update_byte(
								(uint8_t *)RAPID_KEY_ENABLE_ADDR,
								stored_state
						);
				}

				rapidKeyEnable = stored_state;

				*send_status = ione_api_status_success;
				*send_data = rapidKeyEnable;
				*payload_length = 2U;

				uprintf(
						"Get Rapid Trigger State: %u\r\n",
						(unsigned int)rapidKeyEnable
				);
		}
		break;


		case ione_api_cmd_set_rapid_trigger_value_mm:
		{
				uint16_t new_trigger_level;
				uint16_t new_release_level;
				uint16_t verify_trigger_level;
				uint16_t verify_release_level;

				uprintf("API=>Set Rapid Trigger Value mm:\r\n");

				if (*payload_length != 4U)
				{
						*send_status = ione_api_status_fail;
						*payload_length = 1U;

						uprintf("Set Rapid Trigger Value Fail: "
										"payload length=%u, expected=4\r\n",
										*payload_length);
						break;
				}

				new_trigger_level =
						((uint16_t)recive_payload[0] << 8) |
						(uint16_t)recive_payload[1];

				new_release_level =
						((uint16_t)recive_payload[2] << 8) |
						(uint16_t)recive_payload[3];

				uprintf("RX Trigger Level: %u.%02u mm (0x%04X)\r\n",
								new_trigger_level / 100U,
								new_trigger_level % 100U,
								new_trigger_level);

				uprintf("RX Release Level: %u.%02u mm (0x%04X)\r\n",
								new_release_level / 100U,
								new_release_level % 100U,
								new_release_level);

				/*
				 * Trigger Level 合法範圍檢查。
				 */
				if ((new_trigger_level < TriggerMin) ||
						(new_trigger_level > TriggerMax))
				{
						*send_status = ione_api_status_fail;
						*payload_length = 1U;

						uprintf("Set Rapid Trigger Value Fail: "
										"trigger=%u, valid=%u~%u\r\n",
										new_trigger_level,
										TriggerMin,
										TriggerMax);
						break;
				}

				/*
				 * Release Level 合法範圍檢查。
				 *
				 * 保留你原本的條件：
				 * release 必須小於 trigger。
				 *
				 * 如果你的實際產品定義不是如此，
				 * 再移除 new_release_level >= new_trigger_level 條件。
				 */
				if ((new_release_level > ReleaseMax) ||
						(new_release_level >= new_trigger_level))
				{
						*send_status = ione_api_status_fail;
						*payload_length = 1U;

						uprintf("Set Rapid Trigger Value Fail: "
										"release=%u, valid=%u~%u, trigger=%u\r\n",
										new_release_level,
										ReleaseMin,
										ReleaseMax,
										new_trigger_level);
						break;
				}

				keyMatrixTriggerLevel = new_trigger_level;
				keyMatrixReleaseLevel = new_release_level;

				/*
				 * 用 update_word，資料相同時不重複寫 EEPROM。
				 */
				eeprom_update_word(
						(uint16_t *)EECONFIG_KEYMAP_TRIGGER_LEVEL,
						keyMatrixTriggerLevel);

				eeprom_update_word(
						(uint16_t *)EECONFIG_KEYMAP_RELEASE_LEVEL,
						keyMatrixReleaseLevel);

				/*
				 * 如果這兩個函數內部也會再次寫入同一組 EEPROM，
				 * 建議刪除，避免重複寫入。
				 *
				 * write_trigger_to_eeprom();
				 * write_Release_to_eeprom();
				 */

				verify_trigger_level =
						eeprom_read_word(
								(const uint16_t *)EECONFIG_KEYMAP_TRIGGER_LEVEL);

				verify_release_level =
						eeprom_read_word(
								(const uint16_t *)EECONFIG_KEYMAP_RELEASE_LEVEL);

				if ((verify_trigger_level != keyMatrixTriggerLevel) ||
						(verify_release_level != keyMatrixReleaseLevel))
				{
						*send_status = ione_api_status_fail;
						*payload_length = 1U;

						uprintf("Set Rapid Trigger Value Fail: "
										"EEPROM verify error\r\n");

						uprintf("Trigger write=%u read=%u\r\n",
										keyMatrixTriggerLevel,
										verify_trigger_level);

						uprintf("Release write=%u read=%u\r\n",
										keyMatrixReleaseLevel,
										verify_release_level);
						break;
				}

				*send_status = ione_api_status_success;
				*payload_length = 1U;

				uprintf("Rapid Trigger Value Saved.\r\n");
		}
		break;


		case ione_api_cmd_get_rapid_trigger_value_mm:
		{
				uint16_t stored_trigger_level;
				uint16_t stored_release_level;

				uprintf("API=>Get Rapid Trigger Value mm:\r\n");

				if (*payload_length != 0U)
				{
						*send_status = ione_api_status_fail;
						*payload_length = 1U;

						uprintf("Get Rapid Trigger Value Fail: "
										"payload length=%u, expected=0\r\n",
										*payload_length);
						break;
				}

				stored_trigger_level =
						eeprom_read_word(
								(const uint16_t *)EECONFIG_KEYMAP_TRIGGER_LEVEL);

				stored_release_level =
						eeprom_read_word(
								(const uint16_t *)EECONFIG_KEYMAP_RELEASE_LEVEL);

				/*
				 * EEPROM 尚未初始化或資料不合法時，
				 * 載入預設值並寫回 EEPROM。
				 */
				if ((stored_trigger_level == 0xFFFFU) ||
						(stored_trigger_level < TriggerMin) ||
						(stored_trigger_level > TriggerMax))
				{
						uprintf("Trigger Level invalid: 0x%04X, "
										"initialize to %u\r\n",
										stored_trigger_level,
										TriggerInitial);

						stored_trigger_level = TriggerInitial;

						eeprom_update_word(
								(uint16_t *)EECONFIG_KEYMAP_TRIGGER_LEVEL,
								stored_trigger_level);
				}

				if ((stored_release_level == 0xFFFFU) ||
						(stored_release_level > ReleaseMax) ||
						(stored_release_level >= stored_trigger_level))
				{
						uprintf("Release Level invalid: 0x%04X, "
										"initialize to %u\r\n",
										stored_release_level,
										ReleaseInitial);

						stored_release_level = ReleaseInitial;

						eeprom_update_word(
								(uint16_t *)EECONFIG_KEYMAP_RELEASE_LEVEL,
								stored_release_level);
				}

				keyMatrixTriggerLevel = stored_trigger_level;
				keyMatrixReleaseLevel = stored_release_level;

				*send_status = ione_api_status_success;

				/*
				 * 回傳格式：
				 * status
				 * trigger high
				 * trigger low
				 * release high
				 * release low
				 */
				*send_data++ =
						(uint8_t)((keyMatrixTriggerLevel >> 8) & 0xFFU);

				*send_data++ =
						(uint8_t)(keyMatrixTriggerLevel & 0xFFU);

				*send_data++ =
						(uint8_t)((keyMatrixReleaseLevel >> 8) & 0xFFU);

				*send_data =
						(uint8_t)(keyMatrixReleaseLevel & 0xFFU);

				*payload_length = 5U;

				uprintf("Get Trigger Level: %u.%02u mm\r\n",
								keyMatrixTriggerLevel / 100U,
								keyMatrixTriggerLevel % 100U);

				uprintf("Get Release Level: %u.%02u mm\r\n",
								keyMatrixReleaseLevel / 100U,
								keyMatrixReleaseLevel % 100U);
		}
		break;

		case ione_api_cmd_set_tachyon_mode:
		{
				uint8_t tachyon_mode;
				led_runMode_t tachyon_led_mode;

				uprintf("API=>Set tachyon_mode:\r\n");

				if (*payload_length != 1) {
						*send_status = ione_api_status_fail;
						*payload_length = 1;

						uprintf(
								"Tachyon failed: invalid payload length=%u\r\n",
								*payload_length
						);
						break;
				}

				tachyon_mode = recive_payload[0];

				if (tachyon_mode != 0x00 &&
						tachyon_mode != 0x01) {
						*send_status = ione_api_status_fail;
						*payload_length = 1;

						uprintf(
								"Tachyon failed: invalid value=%02X\r\n",
								tachyon_mode
						);
						break;
				}

				if (tachyon_mode == 0x01) {
						/*
						 * Enable Tachyon Mode.
						 * Turn off LED output without changing the saved lighting mode.
						 */
						tachyon_led_mode = currentLED_RunMode;
						tachyon_led_mode.type = offMode;

						eeprom_update_byte(TACHYON_MODE_ADD, 0x01);
						setRunMode(tachyon_led_mode);

						uprintf(
								"Tachyon enabled: LED off, saved mode=%d\r\n",
								currentLED_RunMode.mode
						);
				} else {
						/*
						 * Disable Tachyon Mode.
						 * Restore the current lighting configuration.
						 */
						eeprom_update_byte(TACHYON_MODE_ADD, 0x00);
						setRunMode(currentLED_RunMode);

						uprintf(
								"Tachyon disabled: LED restored, mode=%d\r\n",
								currentLED_RunMode.mode
						);
				}

				*send_status = ione_api_status_success;
				*payload_length = 1;
		}
		break;

		case ione_api_cmd_tachyon_mode:
		{
				uint8_t tachyon_mode;

				uprintf("API=>Get tachyon_mode:\r\n");

				if (*payload_length != 0) {
						*send_status = ione_api_status_fail;
						*payload_length = 1;

						uprintf(
								"Get Tachyon failed: invalid payload length=%u\r\n",
								*payload_length
						);
						break;
				}

				tachyon_mode =
						(eeprom_read_byte(TACHYON_MODE_ADD) == 0x01)
								? 0x01
								: 0x00;

				*send_status = ione_api_status_success;
				*send_data = tachyon_mode;
				*payload_length = 2;

				uprintf(
						"Tachyon mode=%02X\r\n",
						tachyon_mode
				);
		}
		break;
#ifdef socd
		case ione_api_cmd_set_snap_tap_SOCD_Binding:
		{
				bool result;

				uprintf("API=>Set Snap Tap SOCD Binding:\r\n");
				uprintf(
						"Payload length=%u\r\n",
						(unsigned int)*payload_length
				);

				if (*payload_length != 8U) {
						*send_status = ione_api_status_fail;
						*payload_length = 1U;

						uprintf("Failed: invalid payload length\r\n");
						break;
				}

				uprintf(
						"SOCD RX: %02X %02X %02X %02X "
						"%02X %02X %02X %02X\r\n",
						recive_payload[0],
						recive_payload[1],
						recive_payload[2],
						recive_payload[3],
						recive_payload[4],
						recive_payload[5],
						recive_payload[6],
						recive_payload[7]
				);

				result = socd_apply_report(recive_payload);

				if (result) {
						*send_status = ione_api_status_success;
						uprintf("SOCD binding saved successfully\r\n");
				} else {
						*send_status = ione_api_status_fail;
						uprintf("SOCD binding save failed\r\n");
				}

				*payload_length = 1U;
		}
		break;
		case ione_api_cmd_get_snap_tap_SOCD_Binding:
        {
            bool sent = false;

            uprintf("API=>Get Snap Tap SOCD Binding:\r\n");

            if (*payload_length == 0U) {
                for (uint8_t i = 0U; i < SOCD_SLOT_COUNT; i++) {
                    uint8_t report_len;

                    memset(send_payload, 0, sizeof(recive_payload));
                    send_data[0] = i;
                    report_len = socd_fill_report(send_data);

                    if (report_len == 0U) {
                        continue;
                    }

                    *send_status = ione_api_status_sending;
                    *payload_length = (uint8_t)(1U + report_len);

                    ione_send_stream_packet(
                        tx_packet,
                        data_start,
                        tx_length,
                        ione_api_cmd_get_snap_tap_SOCD_Binding);
                    sent = true;
                }

                memset(send_payload, 0, sizeof(recive_payload));
                *send_status = sent ? ione_api_status_success : ione_api_status_fail;
                *payload_length = 1U;

                if (!sent) {
                    uprintf("Fail: no SOCD slots\r\n");
                }
            } else if (*payload_length == 1U) {
                uint8_t report_len;

                memset(send_payload, 0, sizeof(recive_payload));
                send_data[0] = recive_payload[0];
                report_len = socd_fill_report(send_data);

                if (report_len > 0U) {
                    *send_status = ione_api_status_success;
                    *payload_length = (uint8_t)(1U + report_len);
                } else {
                    *send_status = ione_api_status_fail;
                    *payload_length = 1U;
                    uprintf("Fail: invalid SOCD slot\r\n");
                }
            } else {
                *send_status = ione_api_status_fail;
                *payload_length = 1U;
                uprintf("Fail\r\n");
            }
        }
        break;
        case ione_api_cmd_delete_snap_tap_SOCD_Binding:
            uprintf ("API=>Delete Snap Tap SOCD Binding:\r\n");
            if (*payload_length == 0) {
                eeconfig_socd_clear();
                *send_status = ione_api_status_success;
                *payload_length = 1;
                uprintf ("[SOCD] CLEAR all\r\n");
            } else if (*payload_length == 1) {
                if (recive_payload[0] == 0xFF) {
                    eeconfig_socd_clear();
                    *send_status = ione_api_status_success;
                    uprintf ("[SOCD] CLEAR all\r\n");
                } else if (recive_payload[0] < SOCD_SLOT_COUNT) {
                    eeconfig_socd_clear_slot(recive_payload[0]);
                    *send_status = ione_api_status_success;
                    uprintf ("[SOCD] CLEAR slot=%u\r\n", recive_payload[0]);
                } else {
                    *send_status = ione_api_status_fail;
                    uprintf ("[SOCD] CLEAR invalid slot=%u\r\n", recive_payload[0]);
                }
                *payload_length = 1;
            } else {
                *send_status = ione_api_status_fail;
                *payload_length = 1;
                uprintf ("[SOCD] CLEAR invalid payload length\r\n");
            }
		break;
        case ione_api_test:
            // eeconfig_write_socd(0, &cfg);
            // socd_test_eeprom();
        break;
#endif //socd
			case ione_api_cmd_set_dynamic_keystroke:
			{
					bool saved = false;
					uint8_t layer;
					uint8_t row;
					uint8_t col;
					uint8_t key_press;
					uint8_t key_release;

					uprintf("API=>Set Dynamic Keystroke:\r\n");

					if (*payload_length != 9U) {
							*send_status = ione_api_status_fail;
							*payload_length = 1U;

							uprintf(
									"Fail: invalid payload length=%u\r\n",
									(unsigned int)*payload_length
							);
							break;
					}

					layer       = recive_payload[0];
					row         = recive_payload[1];
					col         = recive_payload[2];
					key_press   = recive_payload[7];
					key_release = recive_payload[8];

					uprintf(
							"DKS RX: layer=%u row=%u col=%u "
							"keys=%02X %02X %02X %02X "
							"press=%u release=%u\r\n",
							(unsigned int)layer,
							(unsigned int)row,
							(unsigned int)col,
							recive_payload[3],
							recive_payload[4],
							recive_payload[5],
							recive_payload[6],
							(unsigned int)key_press,
							(unsigned int)key_release
					);

					/*
					 * Current firmware does not store layer per DKS slot.
					 */
					if (layer != 0U) {
							*send_status = ione_api_status_fail;
							*payload_length = 1U;

							uprintf("Fail: only layer 0 is currently supported\r\n");
							break;
					}

					if (row >= MATRIX_ROWS || col >= MATRIX_COLS) {
							*send_status = ione_api_status_fail;
							*payload_length = 1U;

							uprintf("Fail: invalid row or column\r\n");
							break;
					}

					/*
					 * Store global trigger and release levels.
					 * These values are shared by all DKS entries.
					 */
					keyMatrixTriggerLevel = key_press;
					keyMatrixReleaseLevel = key_release;

					for (uint8_t i = 0U; i < RAPID_TRIGGER_COUNT; i++) {

							if (rapidTriggerKey[i].key.enable == 0x01U &&
									rapidTriggerKey[i].key.row == row &&
									rapidTriggerKey[i].key.col == col) {

									rapidTriggerKey[i].key.enable = 0x01U;
									rapidTriggerKey[i].key.counts = 0x04U;

									rapidTriggerKey[i].key.keycode[0] = recive_payload[3];
									rapidTriggerKey[i].key.keycode[1] = recive_payload[4];
									rapidTriggerKey[i].key.keycode[2] = recive_payload[5];
									rapidTriggerKey[i].key.keycode[3] = recive_payload[6];

									eeconfig_write_rapidTrigger(
											i,
											rapidTriggerKey[i].key
									);

									rapidTriggerProcess(true, i);

									saved = true;

									uprintf(
											"DKS updated: index=%u row=%u col=%u\r\n",
											(unsigned int)i,
											(unsigned int)row,
											(unsigned int)col
									);

									break;
							}
					}

					if (!saved) {
							for (uint8_t i = 0U; i < RAPID_TRIGGER_COUNT; i++) {

									if (rapidTriggerKey[i].key.enable != 0x01U) {

											memset(
													&rapidTriggerKey[i],
													0,
													sizeof(rapidTriggerKey[i])
											);

											rapidTriggerKey[i].key.enable = 0x01U;
											rapidTriggerKey[i].key.counts = 0x04U;
											rapidTriggerKey[i].key.row = row;
											rapidTriggerKey[i].key.col = col;

											rapidTriggerKey[i].key.keycode[0] = recive_payload[3];
											rapidTriggerKey[i].key.keycode[1] = recive_payload[4];
											rapidTriggerKey[i].key.keycode[2] = recive_payload[5];
											rapidTriggerKey[i].key.keycode[3] = recive_payload[6];

											eeconfig_write_rapidTrigger(
													i,
													rapidTriggerKey[i].key
											);

											rapidTriggerProcess(true, i);

											saved = true;

											uprintf(
													"DKS created: index=%u row=%u col=%u\r\n",
													(unsigned int)i,
													(unsigned int)row,
													(unsigned int)col
											);

											break;
									}
							}
					}

					if (saved) {
							*send_status = ione_api_status_success;

							uprintf(
									"DKS saved: press=%u release=%u\r\n",
									(unsigned int)keyMatrixTriggerLevel,
									(unsigned int)keyMatrixReleaseLevel
							);
					} else {
							*send_status = ione_api_status_fail;
							uprintf("Fail: no available DKS slot\r\n");
					}

					*payload_length = 1U;
			}
			break;
			case ione_api_cmd_get_dynamic_keysytoke:
			{
					bool sent = false;
					uint8_t *w;

					uprintf("API=>Get Dynamic Keystroke:\r\n");

					if (*payload_length != 0U) {
							*send_status = ione_api_status_fail;
							*payload_length = 1U;

							uprintf("Fail: invalid payload length\r\n");
							break;
					}

					for (uint8_t i = 0U; i < RAPID_TRIGGER_COUNT; i++) {

							if (rapidTriggerKey[i].key.enable != 0x01U) {
									continue;
							}

							memset(
									send_payload,
									0,
									sizeof(recive_payload)
							);

							w = send_data;
							*send_status = ione_api_status_sending;

							*w++ = i;
							*w++ = 0x00U;
							*w++ = rapidTriggerKey[i].key.row;
							*w++ = rapidTriggerKey[i].key.col;
							*w++ = (uint8_t)rapidTriggerKey[i].key.keycode[0];
							*w++ = (uint8_t)rapidTriggerKey[i].key.keycode[1];
							*w++ = (uint8_t)rapidTriggerKey[i].key.keycode[2];
							*w++ = (uint8_t)rapidTriggerKey[i].key.keycode[3];
							*w++ = (uint8_t)keyMatrixTriggerLevel;
							*w++ = (uint8_t)keyMatrixReleaseLevel;

							*payload_length =
									(uint8_t)(1U + (uint8_t)(w - send_data));

							uprintf(
									"DKS TX: index=%u layer=0 row=%u col=%u "
									"keys=%02X %02X %02X %02X "
									"press=%u release=%u\r\n",
									(unsigned int)i,
									(unsigned int)rapidTriggerKey[i].key.row,
									(unsigned int)rapidTriggerKey[i].key.col,
									(unsigned int)rapidTriggerKey[i].key.keycode[0],
									(unsigned int)rapidTriggerKey[i].key.keycode[1],
									(unsigned int)rapidTriggerKey[i].key.keycode[2],
									(unsigned int)rapidTriggerKey[i].key.keycode[3],
									(unsigned int)keyMatrixTriggerLevel,
									(unsigned int)keyMatrixReleaseLevel
							);

							ione_send_stream_packet(
									tx_packet,
									data_start,
									tx_length,
									ione_api_cmd_get_dynamic_keysytoke
							);

							sent = true;
					}

					memset(
							send_payload,
							0,
							sizeof(recive_payload)
					);

					*send_status =
							sent
									? ione_api_status_success
									: ione_api_status_fail;

					*payload_length = 1U;

					if (!sent) {
							uprintf("Fail: no enabled DKS entries\r\n");
					}
			}
			break;
#ifdef toggleKey
        
			case ione_api_cmd_set_toggle_key:
			{
					uint8_t request_length;
					uint8_t layer;
					uint8_t row;
					uint8_t col;
					uint8_t keycode;
					bool result;

					uprintf("API=>Set Toggle Key:\r\n");

					request_length = *payload_length;

					if (request_length != 4U) {
							*send_status = ione_api_status_fail;
							*payload_length = 1U;

							uprintf(
									"Fail: invalid payload length=%u\r\n",
									(unsigned int)request_length
							);

							break;
					}

					layer   = recive_payload[0];
					row     = recive_payload[1];
					col     = recive_payload[2];
					keycode = recive_payload[3];

					if (layer >= KEYMAPS_COUNT ||
							row >= MATRIX_ROWS ||
							col >= MATRIX_COLS) {

							*send_status = ione_api_status_fail;
							*payload_length = 1U;

							uprintf(
									"Fail: invalid position "
									"L=%u R=%u C=%u\r\n",
									(unsigned int)layer,
									(unsigned int)row,
									(unsigned int)col
							);

							break;
					}

					result = toggle_key_set(
							layer,
							row,
							col,
							keycode
					);

					if (result) {
							*send_status = ione_api_status_success;

							uprintf(
									"Toggle key saved: "
									"L=%u R=%u C=%u KC=0x%02X\r\n",
									(unsigned int)layer,
									(unsigned int)row,
									(unsigned int)col,
									(unsigned int)keycode
							);
					} else {
							*send_status = ione_api_status_fail;

							uprintf(
									"Toggle key save failed: "
									"L=%u R=%u C=%u KC=0x%02X\r\n",
									(unsigned int)layer,
									(unsigned int)row,
									(unsigned int)col,
									(unsigned int)keycode
							);
					}

					*payload_length = 1U;
			}
			break;
		case ione_api_cmd_get_toggle_key:
        {
            uint8_t *w;

            uprintf("API=>Get Toggle Key:\r\n");

            if (*payload_length != 0U) {
                *send_status = ione_api_status_fail;
                *payload_length = 1U;
                uprintf("Fail: invalid payload length\r\n");
                break;
            }

            for (uint8_t layer_num = 0U; layer_num < KEYMAPS_COUNT; layer_num++) {
                for (uint8_t row_num = 0U; row_num < MATRIX_ROWS; row_num++) {
                    for (uint8_t col_num = 0U; col_num < MATRIX_COLS; col_num++) {
                        uint8_t key_code = toggle_key_get(layer_num, row_num, col_num);

                        if (key_code == 0U) {
                            continue;
                        }

                        memset(send_payload, 0, sizeof(recive_payload));

                        w = send_data;
                        *send_status = ione_api_status_sending;

                        *w++ = layer_num;
                        *w++ = row_num;
                        *w++ = col_num;
                        *w++ = key_code;

                        *payload_length = (uint8_t)(1U + (uint8_t)(w - send_data));

                        ione_send_stream_packet(
                            tx_packet,
                            data_start,
                            tx_length,
                            ione_api_cmd_get_toggle_key);
                    }
                }
            }

            memset(send_payload, 0, sizeof(recive_payload));
            *send_status = ione_api_status_success;
            *payload_length = 1U;
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
		{
				uint8_t *w;

				uprintf("API=>Get Mod Tap:\r\n");

				if (*payload_length != 0U) {
						*send_status = ione_api_status_fail;
						*payload_length = 1U;
						uprintf("Fail: invalid payload length\r\n");
						break;
				}

				for (uint8_t i = 0; i < MT_CAP_MAP; i++) {

						/* Skip invalid / unused entry */
						if (mt_map[i].L == 0xFF &&
								mt_map[i].R == 0xFF &&
								mt_map[i].C == 0xFF) {
								continue;
						}

						memset(send_payload, 0, sizeof(recive_payload));

						w = send_data;
						*send_status = ione_api_status_sending;

						*w++ = mt_map[i].L;
						*w++ = mt_map[i].R;
						*w++ = mt_map[i].C;
						*w++ = mt_map[i].hold;
						*w++ = mt_map[i].click;

						*payload_length = (uint8_t)(1U + (w - send_data));

						uprintf("MT TX: L=%u R=%u C=%u Hold=0x%02X Click=0x%02X\r\n",
										mt_map[i].L,
										mt_map[i].R,
										mt_map[i].C,
										mt_map[i].hold,
										mt_map[i].click);

						ione_send_stream_packet(
								tx_packet,
								data_start,
								tx_length,
								ione_api_cmd_get_mod_tap);
				}

				memset(send_payload, 0, sizeof(recive_payload));

				*send_status = ione_api_status_success;
				*payload_length = 1U;
		}
		break;
#endif // modTap
#ifdef rappySnappy
        case ione_api_cmd_set_rappy_snappy:
        {
            uprintf("API=>Set Rappy Snappy (Analog Pair)\r\n");

            if (*payload_length == 6) {

                uint8_t index = recive_payload[0];

                if (index >= RAPPY_SNAPPY_PAIR_COUNT) {
                    *send_status = ione_api_status_fail;
                    *payload_length = 1;
                    break;
                }

                rappy_snappy_pair_t *p = &g_rappy_pairs[index];

                if (recive_payload[1] == 0xFF &&
                    recive_payload[2] == 0xFF &&
                    recive_payload[4] == 0xFF &&
                    recive_payload[5] == 0xFF) {

                    p->enable = 0;
                    p->state  = RS2_IDLE;

                    *send_status = ione_api_status_success;
                    *payload_length = 1;
                    break;
                }

                p->enable = 1;

                // Key A
                p->lay_a = recive_payload[0];
                p->row_a = recive_payload[1];
                p->col_a = recive_payload[2];
                // p->kc_a  = keymap_get_keycode(0, p->row_a, p->col_a);
                // p->kc_a = eepromKeyBuffer[0][p->row_a][p->col_a];
                p->kc_a = keymaps[p->lay_a][p->row_a][p->col_a];
                

                // Key B
                p->lay_b = recive_payload[3];
                p->row_b = recive_payload[4];
                p->col_b = recive_payload[5];
                // p->kc_b  = keymap_get_keycode(0, p->row_b, p->col_b);
                // p->kc_b = eepromKeyBuffer[0][p->row_b][p->col_b];
                p->kc_b = keymaps[p->lay_b][p->row_b][p->col_b];
                // 新版 rappy snappy：固定使用 Analog Depth Priority
                p->hysteresis = 40;     // 你之後可調
                p->state      = RS2_IDLE;

                *send_status = ione_api_status_success;
                eeconfig_write_rappy_snappy(index, p);
                // uprintf("g_rappy_pairs[index].enable is %d\r\n",g_rappy_pairs[index].enable);
                // uprintf("p->enable is %d\r\n",p->enable);
                // uprintf("set print a layer %d row %d col %d, b layer %d, row %d col %d\r\n",
                //     g_rappy_pairs[index].lay_a,
                //     g_rappy_pairs[index].row_a,
                //     g_rappy_pairs[index].col_a,
                //     g_rappy_pairs[index].lay_b,
                //     g_rappy_pairs[index].row_b,
                //     g_rappy_pairs[index].col_b);
            }
            else {
                *send_status = ione_api_status_fail;
                uprintf("Payload should be 6\r\n");
            }

            *payload_length = 1;
        }
        break;
        case ione_api_cmd_get_rappy_snappy:
        {
            uprintf("API=>Get Rappy Snappy (Analog Pair)\r\n");

            if (*payload_length == 0) {

                uint8_t index = 0;//recive_payload[0];

                if (index >= RAPPY_SNAPPY_PAIR_COUNT ||
                    !g_rappy_pairs[index].enable) {
                    // uprintf("g_rappy_pairs[index].enable?%d\r\n", g_rappy_pairs[index].enable);

                    *send_status = ione_api_status_fail;
                    *payload_length = 1;
                    break;
                }

                rappy_snappy_pair_t *p = &g_rappy_pairs[index];
                // eeconfig_read_rappy_snappy();
                // uprintf("set print a layer %d row %d col %d, b layer %d, row %d col %d\r\n",
                //     g_rappy_pairs[index].lay_a,
                //     g_rappy_pairs[index].row_a,
                //     g_rappy_pairs[index].col_a,
                //     g_rappy_pairs[index].lay_b,
                //     g_rappy_pairs[index].row_b,
                //     g_rappy_pairs[index].col_b);
                *send_status = ione_api_status_success;

                // Key A
                *send_data++ = p->lay_a;
                *send_data++ = p->row_a;
                *send_data++ = p->col_a;

                // Key B
                *send_data++ = p->lay_b;
                *send_data++ = p->row_b;
                *send_data++ = p->col_b;

                *payload_length = 7;        // status + 6 bytes
            }
            else {
                *send_status = ione_api_status_fail;
                *payload_length = 1;
                uprintf("Fail\r\n");
            }
        }
        break;

#endif // rappySnappy
				case ione_api_cmd_set_factory_reset:
				{
						uprintf("API=>Factory Reset:\r\n");

						if (*payload_length != 0U) {
								*send_status = ione_api_status_fail;
								*payload_length = 1U;

								uprintf(
										"Factory Reset fail: invalid payload length=%u\r\n",
										(unsigned int)*payload_length
								);

								break;
						}

						if (factory_reset_pending) {
								*send_status = ione_api_status_fail;
								*payload_length = 1U;

								uprintf("Factory Reset fail: request already pending\r\n");

								break;
						}

						factory_reset_pending = true;

						/*
						 * Return success immediately.
						 * The reset thread performs the EEPROM reset later.
						 */
						*send_status = ione_api_status_success;
						*payload_length = 1U;

						uprintf(
								"Factory Reset ACK queued pending=%u\r\n",
								(unsigned int)factory_reset_pending
						);
				}
				break;
				
        case ione_api_set_polling_rate:
            uprintf("API=>Set Polling Rate:\r\n");
            if(*payload_length == 1 && (recive_payload[0] < 4)){
                *send_status = ione_api_status_success;
                usb_set_polling_rate(recive_payload[0]);
                eeprom_update_byte((uint8_t *)EECONFIG_POLLING_RATE, recive_payload[0]);
            }else{
                *send_status = ione_api_status_fail;
				uprintf ("Fail\r\n");
            }
            *payload_length = 1;
        break;
        case ione_api_get_polling_rate:
            uprintf("API=>Get Polling Rate:\r\n");
            if(*payload_length == 0){
                *send_status = ione_api_status_success;
                *send_data = get_usb_poll_rate();
                *payload_length = 2;
            }else{
                *send_status = ione_api_status_fail;
				uprintf ("Fail\r\n");
                *payload_length = 1;
            }
        break;
				case ione_api_cmd_set_macro_record:
				{
						uint8_t mode;
						uint8_t macro_index;

						uprintf("API=>Set Macro Record:\r\n");

						if (*payload_length != 2)
						{
								*send_status = ione_api_status_fail;
								*payload_length = 1;
								uprintf("Invalid payload length\r\n");
								break;
						}

						mode        = recive_payload[0];
						macro_index = recive_payload[1];

						uprintf("Record Mode : %u\r\n", mode);
						uprintf("Macro Index : %u\r\n", macro_index);

						/* TODO:
							 Real macro record engine will be implemented later.
						*/

						*send_status = ione_api_status_success;
						*payload_length = 1;
				}
				break;

				case ione_api_cmd_set_stop_macro_record:
				{
						uprintf("API=>Stop Macro Record:\r\n");

						/* TODO:
							 Real macro record stop will be implemented later.
						*/

						*send_status = ione_api_status_success;
						*payload_length = 1;
				}
				break;
				
				case ione_api_cmd_set_macro_playback:
				{
						uint8_t request_length;
						uint8_t macro_index;
						uint8_t playback_mode;
						bool start_result;

						uprintf("API=>Set Macro Playback:\r\n");

						request_length = *payload_length;

						if (request_length != 2U) {
								*send_status = ione_api_status_fail;
								*payload_length = 1U;

								uprintf(
										"Fail: invalid payload length=%u\r\n",
										(unsigned int)request_length
								);

								break;
						}

						macro_index  = recive_payload[0];
						playback_mode = recive_payload[1];

						uprintf(
								"Macro Index=%u Playback Mode=%u\r\n",
								(unsigned int)macro_index,
								(unsigned int)playback_mode
						);

						if (macro_index >= MACRO_GROUP_COUNT) {
								*send_status = ione_api_status_fail;
								*payload_length = 1U;

								uprintf(
										"Fail: invalid macro index=%u\r\n",
										(unsigned int)macro_index
								);

								break;
						}

						if (playback_mode > 0x02U) {
								*send_status = ione_api_status_fail;
								*payload_length = 1U;

								uprintf(
										"Fail: invalid playback mode=%u\r\n",
										(unsigned int)playback_mode
								);

								break;
						}

					uprintf(
							"[API] Before macro_engine_start index=%u\r\n",
							(unsigned int)macro_index
					);

					start_result = macro_engine_start(macro_index);

					uprintf(
							"[API] After macro_engine_start result=%u\r\n",
							(unsigned int)start_result
					);

						if (!start_result) {
								*send_status = ione_api_status_fail;
								*payload_length = 1U;

								uprintf(
										"Fail: macro_engine_start index=%u\r\n",
										(unsigned int)macro_index
								);

								break;
						}

						current_macro_playback_index = macro_index;
						current_macro_playback_mode  = playback_mode;

						*send_status = ione_api_status_success;
						*payload_length = 1U;

						switch (playback_mode) {
								case 0x00U:
										uprintf("Playback Mode: Play once\r\n");
										break;

								case 0x01U:
										uprintf("Playback Mode: Click to toggle loop\r\n");
										break;

								case 0x02U:
										uprintf("Playback Mode: While pressed\r\n");
										break;

								default:
										break;
						}

						uprintf(
								"Macro Playback saved: index=%u mode=%u\r\n",
								(unsigned int)current_macro_playback_index,
								(unsigned int)current_macro_playback_mode
						);
				}
				break;

				case ione_api_cmd_get_macro_playback:
				{
						uint8_t request_length;

						uprintf("API=>Get Macro Playback:\r\n");

						request_length = *payload_length;

						if (request_length != 0U) {
								*send_status = ione_api_status_fail;
								*payload_length = 1U;

								uprintf(
										"Fail: invalid payload length=%u\r\n",
										(unsigned int)request_length
								);

								break;
						}

						if (current_macro_playback_mode > 0x02U) {
								current_macro_playback_mode = 0x00U;
						}

						*send_status = ione_api_status_success;
						*send_data = current_macro_playback_mode;
						*payload_length = 2U;

						uprintf(
								"Current Macro Playback: index=%u mode=%u\r\n",
								(unsigned int)current_macro_playback_index,
								(unsigned int)current_macro_playback_mode
						);

						switch (current_macro_playback_mode) {
								case 0x00U:
										uprintf("Playback Mode: Play once\r\n");
										break;

								case 0x01U:
										uprintf("Playback Mode: Click to toggle loop\r\n");
										break;

								case 0x02U:
										uprintf("Playback Mode: While pressed\r\n");
										break;

								default:
										break;
						}
				}
				break;
        case ione_api_cmd_set_macro_data:
            uprintf("API=>Set Macro Data:\r\n");
            /*
             * payload:
             * [0] macro index
             * [1] step index
             * [2] keycode low
             * [3] keycode high
             * [4] delay_type (0x00 fix)
             * [5] delay_ms low
             * [6] delay_ms high 
             * [7] action(0x01 press 0x02 release)
             */
            if (*payload_length == 8 && recive_payload[0] < MACRO_GROUP_COUNT && recive_payload[1] < MACRO_MAX_STEPS) {
                uint8_t macro_index = recive_payload[0];
                uint8_t step_index  = recive_payload[1];
                macro_config_t *cfg = macro_engine_get_config(macro_index);

                if (cfg != NULL) {
                    cfg->step[step_index].keycode    = (uint16_t)recive_payload[2] | ((uint16_t)recive_payload[3] << 8);
                    cfg->step[step_index].delay_type = recive_payload[4];
                    cfg->step[step_index].delay_ms   = (uint16_t)recive_payload[5] | ((uint16_t)recive_payload[6] << 8);
                    cfg->step[step_index].action     = recive_payload[7];

                    if (cfg->step_count <= step_index) {
                        cfg->step_count = (uint8_t)(step_index + 1);
                    }

                    macro_save_to_eeprom(macro_index);
                    *send_status = ione_api_status_success;
                } else {
                    *send_status = ione_api_status_fail;
                    uprintf("Fail\r\n");
                }
            } else {
                *send_status = ione_api_status_fail;
                uprintf("Fail\r\n");
            }
            *payload_length = 1;
        break;
        case ione_api_cmd_get_macro_data:
        {
            uprintf("API=>Get Macro Data:\r\n");

            if ((*payload_length == 1U) &&
                (recive_payload[0] < MACRO_GROUP_COUNT)) {
                uint8_t macro_index = recive_payload[0];
                macro_config_t *cfg = macro_engine_get_config(macro_index);
                uint8_t *w;

                if (cfg == NULL) {
                    *send_status = ione_api_status_fail;
                    *payload_length = 1U;
                    uprintf("Fail: config NULL\r\n");
                    break;
                }

                if ((cfg->step_count < MACRO_MIN_STEPS) ||
                    (cfg->step_count > MACRO_MAX_STEPS)) {
                    *send_status = ione_api_status_fail;
                    *payload_length = 1U;
                    uprintf("Fail: invalid macro step_count=%u for idx=%u\r\n",
                            (unsigned int)cfg->step_count,
                            (unsigned int)macro_index);
                    break;
                }

                for (uint8_t step_index = 0U;
                     step_index < cfg->step_count;
                     step_index++) {
                    const macro_step_t *step = &cfg->step[step_index];

                    memset(send_payload, 0, sizeof(recive_payload));

                    w = send_data;
                    *send_status = ione_api_status_sending;

                    *w++ = macro_index;
                    *w++ = step_index;
                    *w++ = (uint8_t)(step->keycode & 0xFFU);
                    *w++ = (uint8_t)((step->keycode >> 8) & 0xFFU);
                    *w++ = step->delay_type;
                    *w++ = (uint8_t)(step->delay_ms & 0xFFU);
                    *w++ = (uint8_t)((step->delay_ms >> 8) & 0xFFU);
                    *w++ = step->action;

                    *payload_length = (uint8_t)(1U + (uint8_t)(w - send_data));

                    ione_send_stream_packet(
                        tx_packet,
                        data_start,
                        tx_length,
                        ione_api_cmd_get_macro_data);
                }

                memset(send_payload, 0, sizeof(recive_payload));
                *send_status = ione_api_status_success;
                send_data[0] = macro_index;
                send_data[1] = cfg->step_count;
                *payload_length = 3U;
            } else {
                *send_status = ione_api_status_fail;
                *payload_length = 1U;
                uprintf("Fail\r\n");
            }
        }
        break;
				case ione_api_cmd_clear_macro:
				{
						uprintf("API=>Clear Macro:\r\n");

						macro_engine_stop();

						if (*payload_length == 1U &&
								recive_payload[0] < MACRO_GROUP_COUNT)
						{
								uint8_t macro_index = recive_payload[0];

								uprintf(
										"Clear Macro index=%u\r\n",
										(unsigned int)macro_index
								);

								macro_clear(macro_index);

								*send_status = ione_api_status_success;
						}
						else if (*payload_length == 0U)
						{
								uprintf("Clear all macros\r\n");

								macro_delete();

								*send_status = ione_api_status_success;
						}
						else
						{
								*send_status = ione_api_status_fail;

								uprintf(
										"Clear Macro fail: len=%u index=%u\r\n",
										(unsigned int)*payload_length,
										(unsigned int)recive_payload[0]
								);
						}

						*payload_length = 1U;
				}
				break;
			case ione_api_cmd_reset_all_key:
			{
					uprintf("API=>Reset All Key:\r\n");

					if (*payload_length != 0U) {
							*send_status = ione_api_status_fail;
							*payload_length = 1U;

							uprintf(
									"Reset All Key fail: invalid payload length=%u\r\n",
									(unsigned int)*payload_length
							);

							break;
					}

					if (reset_all_key_pending) {
							*send_status = ione_api_status_fail;
							*payload_length = 1U;

							uprintf("Reset All Key fail: already pending\r\n");
							break;
					}

					reset_all_key_pending = true;

					/*
					 * Return ACK immediately.
					 */
					*send_status = ione_api_status_success;
					*payload_length = 1U;

					uprintf("Reset All Key ACK queued\r\n");
			}
			break;

			case ione_api_cmd_get_dead_zone_settings:
			{
					uint16_t top_dead_zone;
					uint16_t bottom_dead_zone;

					uprintf("API=>Get Dead Zone Settings:\r\n");

					if (*payload_length != 0U)
					{
							uprintf("Get Dead Zone Fail: invalid payload length=%u\r\n",
											*payload_length);

							*send_status = ione_api_status_fail;
							*payload_length = 1U;
							break;
					}

					top_dead_zone =
							eeprom_read_word(
									(const uint16_t *)DEAD_ZONE_TOP_ADDR);

					bottom_dead_zone =
							eeprom_read_word(
									(const uint16_t *)DEAD_ZONE_BOTTOM_ADDR);

					/*
					 * EEPROM 尚未初始化時為 0xFFFF。
					 * 使用預設值並寫回 EEPROM。
					 */
					if (top_dead_zone == 0xFFFFU)
					{
							top_dead_zone = DEAD_ZONE_TOP_DEFAULT;

							eeprom_update_word(
									(uint16_t *)DEAD_ZONE_TOP_ADDR,
									top_dead_zone);

							uprintf(
									"Top Dead Zone uninitialized, "
									"set default %u.%02u mm\r\n",
									top_dead_zone / 100U,
									top_dead_zone % 100U);
					}
					else if (top_dead_zone < DEAD_ZONE_MIN)
					{
							uprintf(
									"Top Dead Zone below minimum: "
									"%u.%02u mm -> %u.%02u mm\r\n",
									top_dead_zone / 100U,
									top_dead_zone % 100U,
									DEAD_ZONE_MIN / 100U,
									DEAD_ZONE_MIN % 100U);

							top_dead_zone = DEAD_ZONE_MIN;

							eeprom_update_word(
									(uint16_t *)DEAD_ZONE_TOP_ADDR,
									top_dead_zone);
					}
					else if (top_dead_zone > DEAD_ZONE_MAX)
					{
							uprintf(
									"Top Dead Zone above maximum: "
									"%u.%02u mm -> %u.%02u mm\r\n",
									top_dead_zone / 100U,
									top_dead_zone % 100U,
									DEAD_ZONE_MAX / 100U,
									DEAD_ZONE_MAX % 100U);

							top_dead_zone = DEAD_ZONE_MAX;

							eeprom_update_word(
									(uint16_t *)DEAD_ZONE_TOP_ADDR,
									top_dead_zone);
					}

					if (bottom_dead_zone == 0xFFFFU)
					{
							bottom_dead_zone = DEAD_ZONE_BOTTOM_DEFAULT;

							eeprom_update_word(
									(uint16_t *)DEAD_ZONE_BOTTOM_ADDR,
									bottom_dead_zone);

							uprintf(
									"Bottom Dead Zone uninitialized, "
									"set default %u.%02u mm\r\n",
									bottom_dead_zone / 100U,
									bottom_dead_zone % 100U);
					}
					else if (bottom_dead_zone < DEAD_ZONE_MIN)
					{
							uprintf(
									"Bottom Dead Zone below minimum: "
									"%u.%02u mm -> %u.%02u mm\r\n",
									bottom_dead_zone / 100U,
									bottom_dead_zone % 100U,
									DEAD_ZONE_MIN / 100U,
									DEAD_ZONE_MIN % 100U);

							bottom_dead_zone = DEAD_ZONE_MIN;

							eeprom_update_word(
									(uint16_t *)DEAD_ZONE_BOTTOM_ADDR,
									bottom_dead_zone);
					}
					else if (bottom_dead_zone > DEAD_ZONE_MAX)
					{
							uprintf(
									"Bottom Dead Zone above maximum: "
									"%u.%02u mm -> %u.%02u mm\r\n",
									bottom_dead_zone / 100U,
									bottom_dead_zone % 100U,
									DEAD_ZONE_MAX / 100U,
									DEAD_ZONE_MAX % 100U);

							bottom_dead_zone = DEAD_ZONE_MAX;

							eeprom_update_word(
									(uint16_t *)DEAD_ZONE_BOTTOM_ADDR,
									bottom_dead_zone);
					}

					uprintf("Top Dead Zone   : %u.%02u mm\r\n",
									top_dead_zone / 100U,
									top_dead_zone % 100U);

					uprintf("Bottom Dead Zone: %u.%02u mm\r\n",
									bottom_dead_zone / 100U,
									bottom_dead_zone % 100U);

					*send_status = ione_api_status_success;

					*send_data++ = (uint8_t)(top_dead_zone >> 8);
					*send_data++ = (uint8_t)(top_dead_zone & 0xFFU);

					*send_data++ = (uint8_t)(bottom_dead_zone >> 8);
					*send_data++ = (uint8_t)(bottom_dead_zone & 0xFFU);

					/*
					 * Payload:
					 * status 1 byte
					 * top    2 bytes
					 * bottom 2 bytes
					 */
					*payload_length = 5U;
			}
			break;

			case ione_api_cmd_set_dead_zone_settings:
			{
					uint16_t rx_top_dead_zone;
					uint16_t rx_bottom_dead_zone;
					uint16_t top_dead_zone;
					uint16_t bottom_dead_zone;
					uint16_t verify_top_dead_zone;
					uint16_t verify_bottom_dead_zone;

					uprintf("API=>Set Dead Zone Settings:\r\n");

					if (*payload_length != 4U)
					{
							uprintf("Set Dead Zone Fail: invalid payload length=%u\r\n",
											*payload_length);

							*send_status = ione_api_status_fail;
							*payload_length = 1U;
							break;
					}

					rx_top_dead_zone =
							((uint16_t)recive_payload[0] << 8) |
							(uint16_t)recive_payload[1];

					rx_bottom_dead_zone =
							((uint16_t)recive_payload[2] << 8) |
							(uint16_t)recive_payload[3];

					top_dead_zone = rx_top_dead_zone;
					bottom_dead_zone = rx_bottom_dead_zone;

					uprintf("RX Top Dead Zone   : %u.%02u mm\r\n",
									rx_top_dead_zone / 100U,
									rx_top_dead_zone % 100U);

					uprintf("RX Bottom Dead Zone: %u.%02u mm\r\n",
									rx_bottom_dead_zone / 100U,
									rx_bottom_dead_zone % 100U);

					/*
					 * 將 App 傳入值限制在合法範圍內。
					 */
					if (top_dead_zone < DEAD_ZONE_MIN)
					{
							top_dead_zone = DEAD_ZONE_MIN;
					}
					else if (top_dead_zone > DEAD_ZONE_MAX)
					{
							top_dead_zone = DEAD_ZONE_MAX;
					}

					if (bottom_dead_zone < DEAD_ZONE_MIN)
					{
							bottom_dead_zone = DEAD_ZONE_MIN;
					}
					else if (bottom_dead_zone > DEAD_ZONE_MAX)
					{
							bottom_dead_zone = DEAD_ZONE_MAX;
					}

					if (top_dead_zone != rx_top_dead_zone)
					{
							uprintf(
									"Top Dead Zone adjusted: "
									"%u.%02u mm -> %u.%02u mm\r\n",
									rx_top_dead_zone / 100U,
									rx_top_dead_zone % 100U,
									top_dead_zone / 100U,
									top_dead_zone % 100U);
					}

					if (bottom_dead_zone != rx_bottom_dead_zone)
					{
							uprintf(
									"Bottom Dead Zone adjusted: "
									"%u.%02u mm -> %u.%02u mm\r\n",
									rx_bottom_dead_zone / 100U,
									rx_bottom_dead_zone % 100U,
									bottom_dead_zone / 100U,
									bottom_dead_zone % 100U);
					}

					eeprom_update_word(
							(uint16_t *)DEAD_ZONE_TOP_ADDR,
							top_dead_zone);

					eeprom_update_word(
							(uint16_t *)DEAD_ZONE_BOTTOM_ADDR,
							bottom_dead_zone);

					/*
					 * 寫入後讀回驗證。
					 */
					verify_top_dead_zone =
							eeprom_read_word(
									(const uint16_t *)DEAD_ZONE_TOP_ADDR);

					verify_bottom_dead_zone =
							eeprom_read_word(
									(const uint16_t *)DEAD_ZONE_BOTTOM_ADDR);

					if ((verify_top_dead_zone != top_dead_zone) ||
							(verify_bottom_dead_zone != bottom_dead_zone))
					{
							uprintf(
									"Dead Zone Save Fail: "
									"top=%u/%u bottom=%u/%u\r\n",
									verify_top_dead_zone,
									top_dead_zone,
									verify_bottom_dead_zone,
									bottom_dead_zone);

							*send_status = ione_api_status_fail;
							*payload_length = 1U;
							break;
					}

					uprintf("Top Dead Zone Saved   : %u.%02u mm\r\n",
									verify_top_dead_zone / 100U,
									verify_top_dead_zone % 100U);

					uprintf("Bottom Dead Zone Saved: %u.%02u mm\r\n",
									verify_bottom_dead_zone / 100U,
									verify_bottom_dead_zone % 100U);

					uprintf("Dead Zone Saved.\r\n");

					*send_status = ione_api_status_success;
					*payload_length = 1U;
			}
			break;

        case ione_api_cmd_system_reset:
            uprintf("API=>System Reset:\r\n");
            if (*payload_length == 0) {
                *send_status = ione_api_status_success;
                *payload_length = 1;
                *(data_start+3) |= 0x80;
                raw_hid_send(data_start, length);
                chThdSleepMilliseconds(100);
                NVIC_SystemReset();
                return false;
            } else {
                *send_status = ione_api_status_fail;
                uprintf("Fail\r\n");
                *payload_length = 1;
            }
        break;
				case ione_api_cmd_factory_reset_and_system_reset:
				{
						uprintf("API=>Factory Reset And System Reset:\r\n");

						if (*payload_length != 0U) {
								*send_status = ione_api_status_fail;
								*payload_length = 1U;

								uprintf(
										"Factory/System Reset fail: invalid payload length=%u\r\n",
										(unsigned int)*payload_length
								);

								break;
						}

						if (factory_system_reset_pending) {
								*send_status = ione_api_status_fail;
								*payload_length = 1U;

								uprintf(
										"Factory/System Reset fail: request already pending\r\n"
								);

								break;
						}

						factory_system_reset_pending = true;

						/*
						 * Return success ACK immediately.
						 * The background thread performs reset later.
						 */
						*send_status = ione_api_status_success;
						*payload_length = 1U;

						uprintf(
								"Factory/System Reset ACK queued pending=%u\r\n",
								(unsigned int)factory_system_reset_pending
						);
				}
				break;
        case ione_api_cmd_set_lighting_value:
        {
            uprintf("API=>Set Lighting Value:\r\n");

            if ((*payload_length == 4U) &&
                (recive_payload[0] > 0U) &&
                (recive_payload[0] < 5U))
            {
                *send_status = ione_api_status_success;

                switch (recive_payload[0])
                {
                    case 1:
                    {
                        uprintf("API=>Bright:\r\n");

                        currentLED_RunMode.brightness =
                            brightness_percent_to_internal(
                                recive_payload[1]
                            );

                        if (currentLED_RunMode.brightness == 0U)
                        {
                            currentLED_RunMode.type = offMode;
                        }
                        else if (currentLED_RunMode.type == offMode)
                        {
                            currentLED_RunMode.type = normalMode;
                        }

                        LED_SpeedChange(
                            currentLED_RunMode.brightness,
                            currentLED_RunMode.speed
                        );

                        setRunMode(currentLED_RunMode);
                    }
                    break;

                    case 2:
                    {
                        uint8_t mode = recive_payload[1];

                        uprintf(
                            "API=>Effect: mode=%u\r\n",
                            mode
                        );

                        currentLED_RunMode.mode = mode;

                        eeprom_update_byte(
                            KB_COLOR_MODE_ADDR,
                            mode
                        );

                        setRunMode(currentLED_RunMode);
                    }
                    break;

                    case 3:
                    {
                        uprintf("API=>Speed:\r\n");

                        currentLED_RunMode.speed =
                            speed_percent_to_internal(
                                recive_payload[1]
                            );

                        setRunMode(currentLED_RunMode);
                    }
                    break;

                    case 4:
                    {
                        uint8_t red   = recive_payload[1];
                        uint8_t green = recive_payload[2];
                        uint8_t blue  = recive_payload[3];

                        uprintf("API=>Color:\r\n");

                        uprintf(
                            "API=>Color: R=%u G=%u B=%u current_mode=%u\r\n",
                            red,
                            green,
                            blue,
                            currentLED_RunMode.mode
                        );

                        /*
                         * 更新 LED RAM 使用中的顏色：
                         * c、mosArt.LED、Reactive preset。
                         */
                        led_set_effect_color(
                            red,
                            green,
                            blue
                        );

                        /*
                         * 保存顏色到 EEPROM。
                         */
                        eeprom_update_byte(
                            KB_COLOR_RED_ADDR,
                            red
                        );

                        eeprom_update_byte(
                            KB_COLOR_GREEN_ADDR,
                            green
                        );

                        eeprom_update_byte(
                            KB_COLOR_BLUE_ADDR,
                            blue
                        );

                        /*
                         * Reactive mode 5 正在使用時，
                         * 重新載入目前模式，讓新顏色立即生效。
                         */
                        if (currentLED_RunMode.mode == 5U)
                        {
                            setRunMode(currentLED_RunMode);
                        }

                        uprintf(
                            "[LED] Color saved R=%u G=%u B=%u mode=%u\r\n",
                            red,
                            green,
                            blue,
                            currentLED_RunMode.mode
                        );

                        uprintf(
                            "[LED EEPROM] R=%u G=%u B=%u\r\n",
                            eeprom_read_byte(
                                KB_COLOR_RED_ADDR
                            ),
                            eeprom_read_byte(
                                KB_COLOR_GREEN_ADDR
                            ),
                            eeprom_read_byte(
                                KB_COLOR_BLUE_ADDR
                            )
                        );
                    }
                    break;

                    default:
                    {
                        *send_status = ione_api_status_fail;
                        uprintf(
                            "API=>Lighting item invalid: %u\r\n",
                            recive_payload[0]
                        );
                    }
                    break;
                }
            }
            else
            {
                *send_status = ione_api_status_fail;

                uprintf(
                    "API=>Set Lighting invalid length=%u item=%u\r\n",
                    *payload_length,
                    recive_payload[0]
                );
            }

            *payload_length = 1U;
        }
        break;
        case ione_api_cmd_get_lighting_value:
            uprintf("API=>Get Lighting Value:\r\n");
            if (*payload_length == 1 && (recive_payload[0] < 5)) {
                *send_status = ione_api_status_success;
                switch(recive_payload[0]){
                    case 1:
                        uprintf("API=>Bright:\r\n");
                        *send_data = brightness_internal_to_percent(
                            currentLED_RunMode.brightness,
                            currentLED_RunMode.type == offMode
                        );
                    break;
                    case 2:
                        uprintf("API=>Effect:\r\n");
                        *send_data = eeprom_read_byte(KB_COLOR_MODE_ADDR);//currentLED_RunMode.mode;
                    break;
                    case 3:
                        uprintf("API=>Speed:\r\n");
                        *send_data = speed_internal_to_percent(currentLED_RunMode.speed);
                    break;
                    case 4:
                        uprintf("API=>Color:\r\n");
                        *send_data = eeprom_read_byte(KB_COLOR_RED_ADDR);//LED_Datas[0].red;
                        send_data++;
                        *send_data = eeprom_read_byte(KB_COLOR_GREEN_ADDR);//LED_Datas[0].green;
                        send_data++;
                        *send_data = eeprom_read_byte(KB_COLOR_BLUE_ADDR);//LED_Datas[0].blue;
                    break;
                } 
            } else {
                *send_status = ione_api_status_fail;
                uprintf("Fail\r\n");
            }
            *payload_length = 4;
        break;
						
				case ione_api_cmd_set_test_mode:
				{
						uint8_t test_mode;

						uprintf("API=>Set Test Mode:\r\n");

						if (*payload_length != 1U) {
								uint8_t invalid_length = *payload_length;

								*send_status = ione_api_status_fail;
								*payload_length = 1U;

								uprintf(
										"Set Test Mode fail: invalid payload length=%u\r\n",
										(unsigned int)invalid_length
								);

								break;
						}

						test_mode = recive_payload[0];

						switch (test_mode) {
								case 0x01U:
										test_mode_enabled = true;

										uprintf(
												"Test Mode enabled, state=%u\r\n",
												(unsigned int)test_mode_enabled
										);

										*send_status = ione_api_status_success;
										break;

								case 0x02U:
										test_mode_enabled = false;

										uprintf(
												"Test Mode disabled, state=%u\r\n",
												(unsigned int)test_mode_enabled
										);

										*send_status = ione_api_status_success;
										break;

								default:
										*send_status = ione_api_status_fail;

										uprintf(
												"Set Test Mode fail: invalid mode=0x%02X\r\n",
												(unsigned int)test_mode
										);
										break;
						}

						*payload_length = 1U;
				}
				break;

        case ione_api_cmd_get_eeprom_max_trigger_value:
        {
            uint16_t index = 0U;
            uint8_t *w;

            uprintf("API=>Get EEPROM Max Trigger Value:\r\n");

            if (*payload_length != 0U) {
                *send_status = ione_api_status_fail;
                *payload_length = 1U;
                uprintf("Fail: invalid payload length\r\n");
                break;
            }

            for (uint8_t row_num = 0U; row_num < MATRIX_ROWS; row_num++) {
                for (uint8_t col_num = 0U; col_num < MATRIX_COLS; col_num++) {
                    uint16_t value;
                    uint8_t high;
                    uint8_t low;

                    memset(send_payload, 0, sizeof(recive_payload));

                    w = send_data;
                    *send_status = ione_api_status_sending;

                    value = eeprom_read_word(
                        EECONFIG_KEYMAP_TRAVEL_CALIBRATION +
                        ((uint32_t)index * 2U));

                    high = (uint8_t)((value >> 8) & 0xFFU);
                    low = (uint8_t)(value & 0xFFU);

                    *w++ = row_num;
                    *w++ = col_num;
                    *w++ = high;
                    *w++ = low;

                    *payload_length = (uint8_t)(1U + (uint8_t)(w - send_data));

                    ione_send_stream_packet(
                        tx_packet,
                        data_start,
                        tx_length,
                        ione_api_cmd_get_eeprom_max_trigger_value);

                    index++;
                }
            }

            memset(send_payload, 0, sizeof(recive_payload));
            *send_status = ione_api_status_success;
            *payload_length = 1U;
        }
        break;
        		default:

				uprintf("Unknown Command : 0x%04X\r\n", command_id);

				*payload_length = 1;
				*send_status = ione_api_status_fail;
				break;
		
			return false;
	}
	//*payload_length = send_data-send_status;


//    for (uint8_t i=0;i<payload_length;i++){
//    	uprintf ("%02X ",send_payload[i]);
//    }
//    uprintf ("\r\n");
    ione_prepare_response_header(data_start, command_id);
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
            {
                dynamic_keymap_macro_set_buffer(offset,size,&command_data[3]
                );

                command_data[3] = 100;
            }
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
        		return;
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

