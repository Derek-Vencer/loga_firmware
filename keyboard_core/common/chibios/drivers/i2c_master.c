//#include "quantum.h"
#include "pin_defs.h"
#include "..\chibios_config.h"
#include "i2c_master.h"
#include <string.h>
#include <ch.h>
#include <hal.h>

#ifndef I2C1_SCL_PIN
#    define I2C1_SCL_PIN B8
#endif
#ifndef I2C1_SDA_PIN
#    define I2C1_SDA_PIN B9
#endif

#ifdef USE_I2CV1
#    ifndef I2C1_OPMODE
#        define I2C1_OPMODE OPMODE_I2C
#    endif
#    ifndef I2C1_CLOCK_SPEED
#        define I2C1_CLOCK_SPEED 100000 /* 400000 //*/
#    endif
#    ifndef I2C1_DUTY_CYCLE
#        define I2C1_DUTY_CYCLE STD_DUTY_CYCLE /* FAST_DUTY_CYCLE_2 */
#    endif
#else
// The default timing values below configures the I2C clock to 400khz assuming a 72Mhz clock
// For more info : https://www.st.com/en/embedded-software/stsw-stm32126.html
#    ifndef I2C1_TIMINGR_PRESC
#        define I2C1_TIMINGR_PRESC 0U
#    endif
#    ifndef I2C1_TIMINGR_SCLDEL
#        define I2C1_TIMINGR_SCLDEL 7U
#    endif
#    ifndef I2C1_TIMINGR_SDADEL
#        define I2C1_TIMINGR_SDADEL 0U
#    endif
#    ifndef I2C1_TIMINGR_SCLH
#        define I2C1_TIMINGR_SCLH 38U
#    endif
#    ifndef I2C1_TIMINGR_SCLL
#        define I2C1_TIMINGR_SCLL 129U
#    endif
#endif

#ifndef I2C_DRIVER
#    define I2C_DRIVER I2CD1
#endif

#ifdef USE_GPIOV1
#    ifndef I2C1_SCL_PAL_MODE
#        define I2C1_SCL_PAL_MODE PAL_MODE_ALTERNATE_OPENDRAIN
#    endif
#    ifndef I2C1_SDA_PAL_MODE
#        define I2C1_SDA_PAL_MODE PAL_MODE_ALTERNATE_OPENDRAIN
#    endif
#else
// The default PAL alternate modes are used to signal that the pins are used for I2C
#    ifndef I2C1_SCL_PAL_MODE
#        define I2C1_SCL_PAL_MODE 4
#    endif
#    ifndef I2C1_SDA_PAL_MODE
#        define I2C1_SDA_PAL_MODE 4
#    endif
#endif

static uint8_t i2c_address;

static const I2CConfig i2cconfig = {
#if defined(USE_I2CV1_CONTRIB)
    I2C1_CLOCK_SPEED,
#elif defined(USE_I2CV1)
    I2C1_OPMODE,
    I2C1_CLOCK_SPEED,
    I2C1_DUTY_CYCLE,
#elif defined(WB32F3G71xx)
    I2C1_OPMODE,
    I2C1_CLOCK_SPEED,
#else
    // This configures the I2C clock to 400khz assuming a 72Mhz clock
    // For more info : https://www.st.com/en/embedded-software/stsw-stm32126.html
    STM32_TIMINGR_PRESC(I2C1_TIMINGR_PRESC) | STM32_TIMINGR_SCLDEL(I2C1_TIMINGR_SCLDEL) | STM32_TIMINGR_SDADEL(I2C1_TIMINGR_SDADEL) | STM32_TIMINGR_SCLH(I2C1_TIMINGR_SCLH) | STM32_TIMINGR_SCLL(I2C1_TIMINGR_SCLL), 0, 0
#endif
};

static i2c_status_t chibios_to_qmk(const msg_t* status) {
    switch (*status) {
        case I2C_NO_ERROR:
            return I2C_STATUS_SUCCESS;
        case I2C_TIMEOUT:
            return I2C_STATUS_TIMEOUT;
        // I2C_BUS_ERROR, I2C_ARBITRATION_LOST, I2C_ACK_FAILURE, I2C_OVERRUN, I2C_PEC_ERROR, I2C_SMB_ALERT
        default:
            return I2C_STATUS_ERROR;
    }
}

__attribute__((weak)) void i2c_init(void) {
    static bool is_initialised = false;
    if (!is_initialised) {
        is_initialised = true;

        // Try releasing special pins for a short time
        palSetLineMode(I2C1_SCL_PIN, PAL_MODE_INPUT);
        palSetLineMode(I2C1_SDA_PIN, PAL_MODE_INPUT);

        chThdSleepMilliseconds(10);
#if defined(USE_GPIOV1)
        palSetLineMode(I2C1_SCL_PIN, I2C1_SCL_PAL_MODE);
        palSetLineMode(I2C1_SDA_PIN, I2C1_SDA_PAL_MODE);
#else
        palSetLineMode(I2C1_SCL_PIN, PAL_MODE_ALTERNATE(I2C1_SCL_PAL_MODE) | PAL_OUTPUT_TYPE_OPENDRAIN);
        palSetLineMode(I2C1_SDA_PIN, PAL_MODE_ALTERNATE(I2C1_SDA_PAL_MODE) | PAL_OUTPUT_TYPE_OPENDRAIN);
#endif
        palSetLineMode(EXTERNAL_EEPROM_WP_PIN, PAL_MODE_OUTPUT_OPENDRAIN);
        palSetLine(EXTERNAL_EEPROM_WP_PIN);
    }
}

i2c_status_t i2c_abort(void){
    i2cAbort(&I2C_DRIVER);
}

i2c_status_t i2c_start(uint8_t address) {
    i2c_address = address;
    i2cStart(&I2C_DRIVER, &i2cconfig);
    return I2C_STATUS_SUCCESS;
}

i2c_status_t i2c_transmit(uint8_t address, const uint8_t* data, uint16_t length, uint16_t timeout) {
    i2c_address = address;
    i2cStart(&I2C_DRIVER, &i2cconfig);
    msg_t status = i2cMasterTransmitTimeout(&I2C_DRIVER, (i2c_address >> 1), data, length, 0, 0, TIME_MS2I(timeout));
    return chibios_to_qmk(&status);
}

i2c_status_t i2c_receive(uint8_t address, uint8_t* data, uint16_t length, uint16_t timeout) {
    i2c_address = address;
    i2cStart(&I2C_DRIVER, &i2cconfig);
    msg_t status = i2cMasterReceiveTimeout(&I2C_DRIVER, (i2c_address >> 1), data, length, TIME_MS2I(timeout));
    return chibios_to_qmk(&status);
}

i2c_status_t i2c_writeReg(uint8_t devaddr, uint8_t regaddr, const uint8_t* data, uint16_t length, uint16_t timeout) {
    i2c_address = devaddr;
    i2cStart(&I2C_DRIVER, &i2cconfig);

    uint8_t complete_packet[length + 1];
    for (uint16_t i = 0; i < length; i++) {
        complete_packet[i + 1] = data[i];
    }
    complete_packet[0] = regaddr;

    msg_t status = i2cMasterTransmitTimeout(&I2C_DRIVER, (i2c_address >> 1), complete_packet, length + 1, 0, 0, TIME_MS2I(timeout));
    return chibios_to_qmk(&status);
}

i2c_status_t i2c_writeReg16(uint8_t devaddr, uint16_t regaddr, const uint8_t* data, uint16_t length, uint16_t timeout) {
    i2c_address = devaddr;
    i2cStart(&I2C_DRIVER, &i2cconfig);

    uint8_t complete_packet[length + 2];
    for (uint16_t i = 0; i < length; i++) {
        complete_packet[i + 2] = data[i];
    }
    complete_packet[0] = regaddr >> 8;
    complete_packet[1] = regaddr & 0xFF;

    msg_t status = i2cMasterTransmitTimeout(&I2C_DRIVER, (i2c_address >> 1), complete_packet, length + 2, 0, 0, TIME_MS2I(timeout));
    return chibios_to_qmk(&status);
}

i2c_status_t i2c_readReg(uint8_t devaddr, uint8_t regaddr, uint8_t* data, uint16_t length, uint16_t timeout) {
    i2c_address = devaddr;
    i2cStart(&I2C_DRIVER, &i2cconfig);
    msg_t status = i2cMasterTransmitTimeout(&I2C_DRIVER, (i2c_address >> 1), &regaddr, 1, data, length, TIME_MS2I(timeout));
    return chibios_to_qmk(&status);
}

i2c_status_t i2c_readReg16(uint8_t devaddr, uint16_t regaddr, uint8_t* data, uint16_t length, uint16_t timeout) {
    i2c_address = devaddr;
    i2cStart(&I2C_DRIVER, &i2cconfig);
    uint8_t register_packet[2] = {regaddr >> 8, regaddr & 0xFF};
    msg_t   status             = i2cMasterTransmitTimeout(&I2C_DRIVER, (i2c_address >> 1), register_packet, 2, data, length, TIME_MS2I(timeout));
    return chibios_to_qmk(&status);
}

void i2c_stop(void) {
    i2cStop(&I2C_DRIVER);
}
