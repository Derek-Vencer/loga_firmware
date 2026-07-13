
#pragma once

#include "eeconfig.h" // for EECONFIG_SIZE
#include "action.h"

// Keyboard level code can change where VIA stores the magic.
// The magic is the build date YYMMDD encoded as BCD in 3 bytes,
// thus installing firmware built on a different date to the one
// already installed can be detected and the EEPROM data is reset.
// The only reason this is important is in case EEPROM usage changes
// and the EEPROM was not explicitly reset by bootmagic lite.
#ifndef VIA_EEPROM_MAGIC_ADDR
#    define VIA_EEPROM_MAGIC_ADDR (EECONFIG_SIZE)
#endif

#define VIA_EEPROM_LAYOUT_OPTIONS_ADDR (VIA_EEPROM_MAGIC_ADDR + 3)

// Changing the layout options size after release will invalidate EEPROM,
// but this is something that should be set correctly on initial implementation.
// 1 byte is enough for most uses (i.e. 8 binary states, or 6 binary + 1 ternary/quaternary )
#ifndef VIA_EEPROM_LAYOUT_OPTIONS_SIZE
#    define VIA_EEPROM_LAYOUT_OPTIONS_SIZE 1
#endif

// Allow override of the layout options default value.
// This requires advanced knowledge of how VIA stores layout options
// and is only really useful for setting a boolean layout option
// state to true by default.
#ifndef VIA_EEPROM_LAYOUT_OPTIONS_DEFAULT
#    define VIA_EEPROM_LAYOUT_OPTIONS_DEFAULT 0x00000000
#endif

// The end of the EEPROM memory used by VIA
// By default, dynamic keymaps will start at this if there is no
// custom config
#define VIA_EEPROM_CUSTOM_CONFIG_ADDR (VIA_EEPROM_LAYOUT_OPTIONS_ADDR + VIA_EEPROM_LAYOUT_OPTIONS_SIZE)

#ifndef VIA_EEPROM_CUSTOM_CONFIG_SIZE
#    define VIA_EEPROM_CUSTOM_CONFIG_SIZE 0
#endif

#define VIA_EEPROM_CONFIG_END (VIA_EEPROM_CUSTOM_CONFIG_ADDR + VIA_EEPROM_CUSTOM_CONFIG_SIZE)

// This is changed only when the command IDs change,
// so VIA Configurator can detect compatible firmware.
#define VIA_PROTOCOL_VERSION 0x0009

// This is a version number for the firmware for the keyboard.
// It can be used to ensure the VIA keyboard definition and the firmware
// have the same version, especially if there are changes to custom values.
// Define this in config.h to override and bump this number.
// This is *not* required if the keyboard is only using basic functionality
// and not using custom values for lighting, rotary encoders, etc.
#ifndef VIA_FIRMWARE_VERSION
#    define VIA_FIRMWARE_VERSION 0x00000000
#endif

enum via_command_id {
    id_get_protocol_version                 = 0x01, // always 0x01
    id_get_keyboard_value                   = 0x02,
    id_set_keyboard_value                   = 0x03,
    id_dynamic_keymap_get_keycode           = 0x04,
    id_dynamic_keymap_set_keycode           = 0x05,
    id_dynamic_keymap_reset                 = 0x06,
    id_custom_set_value                     = 0x07,
    id_custom_get_value                     = 0x08,
    id_custom_save                          = 0x09,
    id_lighting_set_value                   = 0x07,
    id_lighting_get_value                   = 0x08,
    id_lighting_save                        = 0x09,
    id_eeprom_reset                         = 0x0A,
    id_bootloader_jump                      = 0x0B,
    id_dynamic_keymap_macro_get_count       = 0x0C,
    id_dynamic_keymap_macro_get_buffer_size = 0x0D,
    id_dynamic_keymap_macro_get_buffer      = 0x0E,
    id_dynamic_keymap_macro_set_buffer      = 0x0F,
    id_dynamic_keymap_macro_reset           = 0x10,
    id_dynamic_keymap_get_layer_count       = 0x11,
    id_dynamic_keymap_get_buffer            = 0x12,
    id_dynamic_keymap_set_buffer            = 0x13,
	id_loga_api					            = 0x19,

	id_get_TMR_value						= 0x20,
	id_set_TMR_value						= 0x21,
//	id_get_rapid_trigger					= 0x22,
//	id_set_rapid_tirgger					= 0x23,
    id_vial_prefix                          = 0xFE,
    id_unhandled                            = 0xFF,
};

enum via_keyboard_value_id {
    id_uptime              = 0x01,
    id_layout_options      = 0x02,
    id_switch_matrix_state = 0x03,
    id_firmware_version    = 0x04,
    id_device_indication   = 0x05,
	id_control			   = 0x06,
};

enum via_channel_id {
    id_custom_channel         = 0,
    id_qmk_backlight_channel  = 1,
    id_qmk_rgblight_channel   = 2,
    id_qmk_rgb_matrix_channel = 3,
    id_qmk_audio_channel      = 4,
    id_qmk_led_matrix_channel = 5,
};

enum via_TMR_id {
	id_trigger_level			= 0,
	id_release_level			= 1,
	id_calibration_levels		= 2,
	id_individual_trigger_level = 3,
	id_individual_release_level = 4,
	id_individual_level_reset 	= 5,
	id_rapid_trigger_value 		= 6,
	id_rapid_trigger_reset 		= 7,
	id_postion_value			= 8,
	id_individual_update =9,
};

enum via_TMR_level {
	id_key_levels = 0,
	id_raw_levels = 1,
	id_init_levels = 2,
	id_travel_levels = 3,
	id_step_levels = 4,
	id_individual_trigger_levels = 5,
	id_individual_release_levels = 6,
};

enum via_light_id{
	id_light_runMode = 0,
	id_light_mode = 1,
};

enum via_control_id{
	id_keyControl = 0x00,
	id_rapidEnable = 0x01,
	id_keyScanEffect = 0x02,
	id_GloableTriggle = 0x03,
	id_LED_Filter = 0x04,
};


enum via_qmk_backlight_value {
    id_qmk_backlight_brightness = 0x09,
    id_qmk_backlight_effect     = 0x0A,
};

enum via_qmk_rgblight_value {
    id_qmk_rgblight_brightness   = 0x80,
    id_qmk_rgblight_effect       = 0x81,
    id_qmk_rgblight_effect_speed = 0x82,
    id_qmk_rgblight_color        = 0x83,
};

enum via_qmk_rgb_matrix_value {
    id_qmk_rgb_matrix_brightness   = 1,
    id_qmk_rgb_matrix_effect       = 2,
    id_qmk_rgb_matrix_effect_speed = 3,
    id_qmk_rgb_matrix_color        = 4,
};

enum via_qmk_led_matrix_value {
    id_qmk_led_matrix_brightness   = 1,
    id_qmk_led_matrix_effect       = 2,
    id_qmk_led_matrix_effect_speed = 3,
};

enum via_qmk_audio_value {
    id_qmk_audio_enable        = 1,
    id_qmk_audio_clicky_enable = 2,
};

enum ione_api_return {
    ione_api_status_success        	= 0x00,
	ione_api_status_sending			= 0x01,
	ione_api_status_fail			= 0xff,
};

enum ione_api_command {
	ione_api_cmd_get_firmware_version	= 0x0180,
	ione_api_cmd_Set_Profil				= 0x0101,
	ione_api_cmd_get_Profile_state 		= 0x0181,
	ione_api_cmd_set_key_mapping		= 0x0102,
	ione_api_cmd_get_key_mapping		= 0x0182,
	ione_api_cmd_get_all_key_mapping	= 0x0183,
	ione_api_cmd_set_ligthing_effect	= 0x0104,
	ione_api_cmd_get_lighting_effect	= 0x0184,
	ione_api_cmd_set_lighting_color		= 0x0105,
	ione_api_cmd_get_lighting_color		= 0x0185,
	ione_api_cmd_set_konb_state         = 0x0106,
	ione_api_cmd_konb_state				= 0x0186,
	ione_api_cmd_set_lock_state         = 0x0107,
	ione_api_cmd_get_lock_state			= 0x0187,
	ione_api_cmd_set_rapid_trigger_state = 0x0108,
	ione_api_cmd_get_rapid_trigger_state = 0x0188,
	ione_api_cmd_set_rapid_trigger_value = 0x0109,
	ione_api_cmd_get_Rapid_trigger_value = 0x0189,
	ione_api_cmd_set_tachyon_mode		= 0x010A,
	ione_api_cmd_tachyon_mode			= 0x018A,
	ione_api_cmd_set_snap_tap_SOCD_Binding 	= 0x010B,
	ione_api_cmd_get_snap_tap_SOCD_Binding	= 0x018B,
	ione_api_cmd_delete_snap_tap_SOCD_Binding = 0x019B,
	ione_api_test                           = 0x012B,
	ione_api_set_polling_rate           = 0x0110,
	ione_api_get_polling_rate           = 0x0190,
	ione_api_cmd_set_dynamic_keystroke	= 0x010C,
	ione_api_cmd_get_dynamic_keysytoke	= 0x018C,
	ione_api_cmd_set_toggle_key			= 0x010D,
	ione_api_cmd_get_toggle_key			= 0x018D,
	ione_api_cmd_set_mod_tap			= 0x010E,
	ione_api_cmd_get_mod_tap			= 0x018E,
	ione_api_cmd_set_rappy_snappy		= 0x010F,
	ione_api_cmd_get_rappy_snappy		= 0x018F,
	ione_api_cmd_set_factory_reset		= 0x0201,
	ione_api_cmd_get_rapid_trigger_value_mm   = 0x028B,
	ione_api_cmd_set_rapid_trigger_value_mm   = 0x020B,
	ione_api_cmd_get_eeprom_max_trigger_value = 0x0282,
	ione_api_cmd_set_macro_record       = 0x0202,
	ione_api_cmd_set_stop_macro_record  = 0x0203,
	ione_api_cmd_set_macro_playback     = 0x0204,
	ione_api_cmd_get_macro_playback     = 0x0284,
	ione_api_cmd_set_macro_data         = 0x0205,
	ione_api_cmd_get_macro_data         = 0x0285,
	ione_api_cmd_clear_macro            = 0x0206,
	ione_api_cmd_reset_all_key          = 0x0207,
	ione_api_cmd_set_dead_zone_settings = 0x0208,
	ione_api_cmd_get_dead_zone_settings = 0x0288,
	ione_api_cmd_system_reset           = 0x0209,
	ione_api_cmd_factory_reset_and_system_reset = 0x020A,
	ione_api_cmd_set_lighting_value     = 0x020C,
	ione_api_cmd_get_lighting_value     = 0x028C,
	ione_api_cmd_set_test_mode			= 0x0700,


};

// Can be called in an overriding via_init_kb() to test if keyboard level code usage of
// EEPROM is invalid and use/save defaults.
bool via_eeprom_is_valid(void);

// Sets VIA/keyboard level usage of EEPROM to valid/invalid
// Keyboard level code (eg. via_init_kb()) should not call this
void via_eeprom_set_valid(bool valid);

// Called by QMK core to initialize dynamic keymaps etc.
void eeconfig_init_via(void);
void via_init(void);

// Used by VIA to store and retrieve the layout options.
uint32_t via_get_layout_options(void);
void     via_set_layout_options(uint32_t value);
void     via_set_layout_options_kb(uint32_t value);

// Used by VIA to tell a device to flash LEDs (or do something else) when that
// device becomes the active device being configured, on startup or switching
// between devices.
void via_set_device_indication(uint8_t value);

// Called by QMK core to process VIA-specific keycodes.
bool process_record_via(uint16_t keycode, keyrecord_t *record);

void raw_hid_receive(uint8_t *data, uint8_t length);
bool raw_hid_receive_kb(uint8_t *data, uint8_t length);
bool raw_hid_receive_ione(uint8_t *data, uint8_t length);

// These are made external so that keyboard level custom value handlers can use them.
#if defined(BACKLIGHT_ENABLE)
void via_qmk_backlight_command(uint8_t *data, uint8_t length);
void via_qmk_backlight_set_value(uint8_t *data);
void via_qmk_backlight_get_value(uint8_t *data);
void via_qmk_backlight_save(void);
#endif

#if defined(RGBLIGHT_ENABLE)
void via_qmk_rgblight_command(uint8_t *data, uint8_t length);
void via_qmk_rgblight_set_value(uint8_t *data);
void via_qmk_rgblight_get_value(uint8_t *data);
void via_qmk_rgblight_save(void);
#endif

#if defined(RGB_MATRIX_ENABLE)
void via_qmk_rgb_matrix_command(uint8_t *data, uint8_t length);
void via_qmk_rgb_matrix_set_value(uint8_t *data);
void via_qmk_rgb_matrix_get_value(uint8_t *data);
void via_qmk_rgb_matrix_save(void);
#endif

#if defined(LED_MATRIX_ENABLE)
void via_qmk_led_matrix_command(uint8_t *data, uint8_t length);
void via_qmk_led_matrix_set_value(uint8_t *data);
void via_qmk_led_matrix_get_value(uint8_t *data);
void via_qmk_led_matrix_save(void);
#endif

#if defined(AUDIO_ENABLE)
void via_qmk_audio_command(uint8_t *data, uint8_t length);
void via_qmk_audio_set_value(uint8_t *data);
void via_qmk_audio_get_value(uint8_t *data);
void via_qmk_audio_save(void);
#endif
