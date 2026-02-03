#pragma once

#include <stdint.h>

typedef int16_t i2c_status_t;

#define I2C_STATUS_SUCCESS (0)
#define I2C_STATUS_ERROR (-1)
#define I2C_STATUS_TIMEOUT (-2)

void         i2c_init(void);
i2c_status_t i2c_start(uint8_t address);
i2c_status_t i2c_transmit(uint8_t address, const uint8_t* data, uint16_t length, uint16_t timeout);
i2c_status_t i2c_receive(uint8_t address, uint8_t* data, uint16_t length, uint16_t timeout);
i2c_status_t i2c_writeReg(uint8_t devaddr, uint8_t regaddr, const uint8_t* data, uint16_t length, uint16_t timeout);
i2c_status_t i2c_writeReg16(uint8_t devaddr, uint16_t regaddr, const uint8_t* data, uint16_t length, uint16_t timeout);
i2c_status_t i2c_readReg(uint8_t devaddr, uint8_t regaddr, uint8_t* data, uint16_t length, uint16_t timeout);
i2c_status_t i2c_readReg16(uint8_t devaddr, uint16_t regaddr, uint8_t* data, uint16_t length, uint16_t timeout);
void         i2c_stop(void);
