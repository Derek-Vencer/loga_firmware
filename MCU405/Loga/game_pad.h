#ifndef GAMEPAD_H
#define GAMEPAD_H

# include "ch.h"
# include "hal.h"
# include "keyboard.h"

void gamePad_init(void);

typedef union {
    uint8_t raw;
    struct {
        bool    up : 1;
        bool    down : 1;
        bool    left : 1;
        bool    right : 1;

    };
} HatSwitch_config_t;

typedef struct {
	//uint8_t layer;
	uint16_t travelPostion;
	float postioMutl;
} travelPostion_t;

typedef union {
    uint16_t raw;
    struct {
        bool    x_plus : 1;
        bool    x_minus : 1;
        bool    y_plus : 1;
        bool    y_minus : 1;
        bool    z_plus : 1;
        bool    z_minus : 1;
        bool    Rz_plus : 1;
        bool    Rz_minus : 1;
        bool    Rx_plus : 1;
        bool    Rx_minus : 1;
        bool    Ry_plus : 1;
        bool    Ry_minus : 1;
    };
} AnalogStickStatus_t;

typedef struct {
	keyevent_t	event;
	int16_t	value;
} analogKeyInfo_t;

typedef struct {
	analogKeyInfo_t x_plus;
	analogKeyInfo_t x_minus;
	analogKeyInfo_t y_plus;
	analogKeyInfo_t y_minus;
	analogKeyInfo_t z_plus;
	analogKeyInfo_t z_minus;
	analogKeyInfo_t Rz_plus;
	analogKeyInfo_t Rz_minus;
	analogKeyInfo_t Rx_plus;
	analogKeyInfo_t Rx_minus;
	analogKeyInfo_t Ry_plus;
	analogKeyInfo_t Ry_minus;
} analog_stickValue;

#endif
