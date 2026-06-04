#ifndef CONFIG_H
#define CONFIG_H
#include "socd.h"
#define AT32F405RCT7

/* USB Device descriptor parameter */
#define VENDOR_ID       0x195d
#define PRODUCT_ID      0x2100//0x2092
#define DEVICE_VER      0x000F//15//0x000E
#define DEVICE_VER_FIX  0x03 // 03
/* in python2: list(u"whatever".encode('utf-16-le')) */
/*   at most 32 characters or the ugly hack in usb_main.c borks */
#define MANUFACTURER "iOne Innovation"
#define USBSTR_MANUFACTURER    'i', '\x00', 'O', '\x00', 'n', '\x00',  'e', '\x00'
#define PRODUCT "LOGA ASURA 75TMR"
#define USBSTR_PRODUCT         'L', '\x00', 'O', '\x00', 'G', '\x00', 'A', '\x00', ' ', '\x00', 'A', '\x00', 'S', '\x00', 'U', '\x00', 'R', '\x00', 'A', '\x00', ' ', '\x00', '7', '\x00', '5', '\x00', 'T', '\x00', 'M', '\x00', 'R', '\x00'
#define DESCRIPTION "LOGA ASURA 75TMR"
#define USBSTR_SERIAL 		   '0', '\x00', '1', '\x00', '2', '\x00', '3', '\x00'

//#define minTriggerCheck
#define DEFAULT_TRIGGER_LEVEL	150
#define DEFAULT_RELEASE_LEVEL	0

/* key matrix size */
#define MATRIX_ROWS 6
#define MATRIX_COLS 15

#define LED_MATRIX_ROWS 7
#define LED_MATRIX_COLS 26

#define PROFILE_COUNT	4
#define LAYERS_PER_PROFILE 3

/* keymap in eeprom */
#define FN_ACTIONS_COUNT 15
#define KEYMAPS_COUNT LAYERS_PER_PROFILE * PROFILE_COUNT
#define RAPID_TRIGGER_COUNT 24
#define RAPID_TRIGGER_LEVELS 6

#include "../../keyboard_core/common/matrix.h"
#include "rappy_snappy.h"

#define MATRIX_SIZE (matrix_rows() * matrix_cols())

#define KEYS_COUNT (KEYMAPS_COUNT * MATRIX_SIZE)

#define DYNAMIC_MACRO_COUNT 12
#define DYNAMIC_MACRO_SIZE 48
#define DYNAMIC_MACRO_EEPROM_STORAGE

#define EECONFIG_Start (uint8_t *)(0)
#define EECONFIG_ModelName (uint8_t *)(EECONFIG_Start)
#define EECONFIG_SerialNo (uint32_t *)(EECONFIG_ModelName + 25)
#define EECONFIG_kbColor (uint8_t *)(EECONFIG_SerialNo + 11)

#define EECONFIG_KB_HUE   (uint8_t *)(EECONFIG_kbColor + 0)
#define EECONFIG_KB_SAT   (uint8_t *)(EECONFIG_kbColor + 1)
#define EECONFIG_KB_VAL   (uint8_t *)(EECONFIG_kbColor + 2)
#define EECONFIG_KB_MAGIC (uint8_t *)(EECONFIG_kbColor + 3)
#define EECONFIG_MAC_MODE             0x006F
#define EECONFIG_POLLING_RATE         0x0070
#define IS_SOCD_ON_ADD                0x0071
#define TACHYON_MODE_ADD              0x0072
#define RAPID_KEY_ENABLE_ADDR         0x0073
#define EEPROM_FACTORY_INIT_FLAG_ADDR 0x0074
#define KB_ENCODER_MODE_ADDR          0x0075
#define SOCD_SETTING_MAGIC_ADDR       0x0076
#define SOCD_SETTING_MAGIC            0xA5
#define KB_COLOR_RED_ADDR             0x007B
#define KB_COLOR_GREEN_ADDR           0x007C
#define KB_COLOR_BLUE_ADDR            0x007D
#define KB_COLOR_MODE_ADDR            0x007E
#define KB_COLOR_MAGIC_ADDR  0x007F
#define KB_COLOR_MAGIC    0x7F

#define KEYMAP_SIZE (sizeof(uint16_t) * FN_ACTIONS_COUNT + sizeof(uint16_t) * KEYS_COUNT)

#define EECONFIG_KEYMAP_OFFSET_CALIBRATION	(uint16_t *)(0x80)
#define EECONFIG_KEYMAP_TRAVEL_CALIBRATION	(uint16_t *)(EECONFIG_KEYMAP_OFFSET_CALIBRATION + (ADC_NUM_CHANNELS*ADC_BUF_DEPTH*2))
#define EECONFIG_KEYMAP_TRIGGER_LEVEL       (uint16_t *)(EECONFIG_KEYMAP_TRAVEL_CALIBRATION + (ADC_NUM_CHANNELS*ADC_BUF_DEPTH*2))
#define EECONFIG_KEYMAP_RELEASE_LEVEL       (uint16_t *)(EECONFIG_KEYMAP_TRIGGER_LEVEL+4)

#define EECONFIG_KEYMAP_IN_EEPROM 			(uint16_t *)(EECONFIG_KEYMAP_RELEASE_LEVEL+4)
#define EECONFIG_END						(uint16_t *)(EECONFIG_KEYMAP_IN_EEPROM + KEYMAP_SIZE)

#define EECONFIG_TRIGGER_LEVEL				(uint16_t *)(EECONFIG_END)
#define EECONFIG_TRIGGER_END				(uint16_t *)(EECONFIG_TRIGGER_LEVEL + (KEYS_COUNT*2))

#define EECONFIG_RELEASE_LEVEL				(uint16_t *)(EECONFIG_TRIGGER_END)
#define EECONFIG_RELEASE_END				(uint16_t *)(EECONFIG_RELEASE_LEVEL + (KEYS_COUNT*2))

#define EECONFIG_RAPID_TRIGGER				(uint16_t *)(EECONFIG_RELEASE_END)
#define EECONFIG_RAPID_TRIGGER_END			(uint16_t *)(EECONFIG_RAPID_TRIGGER + (24*RAPID_TRIGGER_COUNT))

#define DYNAMIC_MACRO_EEPROM_MAGIC_ADDR 	(uint16_t*)(EECONFIG_RAPID_TRIGGER_END)

#define DYNAMIC_MACRO_EEPROM_BLOCK0_ADDR 	(uint16_t*)(DYNAMIC_MACRO_EEPROM_MAGIC_ADDR+2) //
#define DYNAMIC_MACRO_EEPROM_BLOCK0_ADDR_END 	(uint16_t*)(DYNAMIC_MACRO_EEPROM_BLOCK0_ADDR+2) //

// SOCD
#define EECONFIG_SOCD_ADDR                  (DYNAMIC_MACRO_EEPROM_BLOCK0_ADDR_END)
#define EECONFIG_SOCD_SIZE                  (sizeof(socd_config_t) * SOCD_SLOT_COUNT)
#define EECONFIG_SOCD_ADDR_END              (EECONFIG_SOCD_ADDR + EECONFIG_SOCD_SIZE) //0x3BAC not sure after fix (socd_config_t)

// modTap
#define EECONFIG_MODTAP_ADDR                (EECONFIG_SOCD_ADDR_END)
#define EECONFIG_MODTAP_SIZE                (sizeof(mt_entry_t) * MT_CAP_MAP)
#define EECONFIG_MODTAP_ADDR_END            (EECONFIG_MODTAP_ADDR + EECONFIG_MODTAP_SIZE)

// rappy snappy
#define EECONFIG_RAPPYSNAPPY_ADDR           (EECONFIG_MODTAP_ADDR_END)
#define EECONFIG_RAPPYSNAPPY_SIZE           (sizeof(rappy_snappy_pair_t) * RAPPY_SNAPPY_PAIR_COUNT)
#define EECONFIG_RAPPYSNAPPY_ADDR_END       (EECONFIG_RAPPYSNAPPY_ADDR + EECONFIG_RAPPYSNAPPY_SIZE)

// toggle key
#define EECONFIG_TOGGLE_KEY_ADDR             (EECONFIG_RAPPYSNAPPY_ADDR + EECONFIG_RAPPYSNAPPY_SIZE)
#define EECONFIG_TOGGLE_KEY_SIZE             (sizeof(toggle_key_slot_t) * toggle_key_pair_count)
#define EECONFIG_TOGGLE_KEY_ADDR_END         (EECONFIG_TOGGLE_KEY_ADDR + EECONFIG_TOGGLE_KEY_SIZE)

// new macro
#define EEPROM_MACRO_SLOT_ADDR             (EECONFIG_TOGGLE_KEY_ADDR + EECONFIG_TOGGLE_KEY_SIZE)
#define EEPROM_MACRO_SLOT_SIZE             (sizeof(macro_config_t) * MACRO_GROUP_COUNT)
#define EEPROM_MACRO_SLOT_ADDR_END         (EEPROM_MACRO_SLOT_ADDR + EEPROM_MACRO_SLOT_SIZE)

// #define EECONFIG_KEYMAP_TRAVEL_2MM 	        (uint16_t*)(DYNAMIC_MACRO_EEPROM_BLOCK0_ADDR+2)


//#define EECONFIG_KEYMAP_CHECKSUM   (uint16_t *)(EECONFIG_KEYMAP_IN_EEPROM)

#define EXTRAKEY_ENABLE
#define COMMAND_ENABLE
#define ENCODER_ENABLE
// #define CONSOLE_ENABLE // lyon see uprintf message
#define LAMP_ARRAY_ENABLE
#define MOUSE_ENABLE
#define CONTROL_ENABLE // 
#define RAW_ENABLE
#define NKRO_ENABLE
#define KEYMAP_IN_EEPROM_ENABLE

#define ADC_NUM_CHANNELS   	7
#define ADC_BUF_DEPTH      16

/* Locking resynchronize hack */
#define LOCKING_RESYNC_ENABLE

/* key combination for command */
#define IS_COMMAND() ( \
    keyboard_report->mods == (MOD_BIT(KC_LSHIFT) | MOD_BIT(KC_RSHIFT)) \
)

#define ENCODERS_PAD_A { D2}
#define ENCODERS_PAD_B { B13}
#define ENCODERS_PAD_BTN { A15}


#define ENCODER_RESOLUTION {2}

#define TAP_HOLD_CAPS_DELAY 350
#define TAP_CODE_DELAY 1

#define EEPROM_DRIVER
#define EEPROM_I2C
#define EEPROM_USE_I2C

//#define EXTERNAL_EEPROM_I2C_BASE_ADDRESS 0xA0
#define EXTERNAL_EEPROM_I2C_BASE_ADDRESS 0b10101000		//non_OP
//#define EXTERNAL_EEPROM_I2C_BASE_ADDRESS 0b10100000
#define EXTERNAL_EEPROM_BYTE_COUNT 65536 - EXTERNAL_EEPROM_PAGE_SIZE
#define EXTERNAL_EEPROM_PAGE_SIZE 128
#define EXTERNAL_EEPROM_ADDRESS_SIZE 2
#define EXTERNAL_EEPROM_WRITE_TIME 5
#define EXTERNAL_EEPROM_WP_PIN B7

#define AT32_I2C_USE_DMA TRUE
#define I2C_DRIVER I2CD1
#define I2C1_SCL_PIN B8
#define I2C1_SDA_PIN B9
#define I2C1_SCL_PAL_MODE 4
#define I2C1_SDA_PAL_MODE 4
#define I2C_START_RETRY_COUNT 10
#define I2C_TIMEOUT 1000 // milliseconds
#define I2C1_CLOCK_SPEED 400000 /* 400000 */ //

//#define CRC_USE_DMA	TRUE

// I2C ADJ
#define I2C1_TIMINGR_PRESC 0U
#define I2C1_TIMINGR_SCLDEL 7U
#define I2C1_TIMINGR_SDADEL 0U

//591Khz
//#define I2C1_TIMINGR_SCLH 120U
//#define I2C1_TIMINGR_SCLL 138U

//1000Khz //
#define I2C1_TIMINGR_SCLH 45U
#define I2C1_TIMINGR_SCLL 50U


// I2C ADJ
//#define I2C1_TIMINGR_PRESC 2U
//#define I2C1_TIMINGR_SCLDEL 4U
//#define I2C1_TIMINGR_SDADEL 4U
//
////400k		//1.21ms
////#define I2C1_TIMINGR_SCLH 50U
////#define I2C1_TIMINGR_SCLL 60U
//
////800k
////#define I2C1_TIMINGR_SCLH 25U
////#define I2C1_TIMINGR_SCLL 25U
//
////1000k		//488uS
//#define I2C1_TIMINGR_SCLH 18U
//#define I2C1_TIMINGR_SCLL 18U

//====




// 400K
//   #define I2C1_TIMINGR_SCLH   55U
//   #define I2C1_TIMINGR_SCLL   55U

// 368K
//   #define I2C1_TIMINGR_SCLH   60U
//   #define I2C1_TIMINGR_SCLL   100U

// 368K v2
//   #define I2C1_TIMINGR_SCLH   45U
//   #define I2C1_TIMINGR_SCLL   90u

// 368K v3


//??K
//  #define I2C1_TIMINGR_SCLH   43U
//  #define I2C1_TIMINGR_SCLL   63U
// END OF 400k ADJ




//====


//#define I2C1_TIMINGR_PRESC 15U
//#define I2C1_TIMINGR_SCLDEL 4U
//#define I2C1_TIMINGR_SDADEL 2U
//
//#define I2C1_TIMINGR_SCLH 15U
//#define I2C1_TIMINGR_SCLL 21U

//  .timingr          = AT32_TIMINGR_PRESC(15U) | AT32_TIMINGR_SCLDEL(4U) |
//                      AT32_TIMINGR_SDADEL(2U) | AT32_TIMINGR_SCLH(15U) |
//                      AT32_TIMINGR_SCLL(21U),




/*
 * Feature disable options
 *  These options are also useful to firmware size reduction.
 */

/* disable debug print */
//#define NO_DEBUG

/* disable print */
//#define NO_PRINT

/* disable action features */
//#define NO_ACTION_LAYER
//#define NO_ACTION_TAPPING
//#define NO_ACTION_ONESHOT
//#define NO_ACTION_MACRO
//#define NO_ACTION_FUNCTION



#define SPI_LED_DRIVER			SPID2
#define SPI_LED_SDB_PIN			B4
#define SPI_LED_SCK_PIN			A9
#define SPI_LED_MOSI_PIN		A10
#define SPI_LED_MISO_PIN		A12
//#define SPI_LED_MISO_PIN		C2

//#define SPI_LED_CS1_PIN			F6
//#define SPI_LED_CS1_PORT		GPIOF
//#define SPI_LED_CS1_PAD			6U

#define SPI_LED_CS1_PIN			A8
#define SPI_LED_CS1_PORT		GPIOA
#define SPI_LED_CS1_PAD			8U


#define SPI_LED_CS2_PIN			A11
#define SPI_LED_CS2_PORT		GPIOA
#define SPI_LED_CS2_PAD			11U

#define SPI_LED_CS3_PIN			F7
#define SPI_LED_CS3_PORT		GPIOF
#define SPI_LED_CS3_PAD			7U

#define SPI_LED_SCK_PAL_MODE	5
#define SPI_LED_MOSI_PAL_MODE	5
#define SPI_LED_MISO_PAL_MODE	5

//tapMod
#define TAPPING_TERM 200  // ms，依手感調

#define PREVENT_STUCK_MODIFIERS
// #define MATRIX_HAS_GHOST
#endif
