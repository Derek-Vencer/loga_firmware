# List of all the board related files.
BOARDSRC = 	$(CHIBIOS)/os/hal/boards/AT_START_F405xx/board.c \
			$(CHIBIOS)/os/hal/boards/AT_START_F405xx/at32f402_405_clock.c

# Required include directories
BOARDINC = $(CHIBIOS)/os/hal/boards/AT_START_F405xx

# Shared variables
ALLCSRC += $(BOARDSRC)
ALLINC  += $(BOARDINC)
