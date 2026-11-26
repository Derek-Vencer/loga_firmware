LED_DIR = led

SRC += led/led_function.c \
	   led/led_effect.c	\
	   led/driver/led_driver.c

OPT_DEFS += -DPROTOCOL_LED

VPATH += $(LED_DIR)
