ifeq ($(USE_SMART_BUILD),yes)
ifneq ($(findstring HAL_USE_PAL TRUE,$(HALCONF)),)
PLATFORMSRC += $(CHIBIOS)/os/hal/ports/AT32/AT32F402_405xx/drivers/at32f402_405_acc.c
PLATFORMSRC += $(CHIBIOS)/os/hal/ports/AT32/AT32F402_405xx/drivers/at32f402_405_adc.c
PLATFORMSRC += $(CHIBIOS)/os/hal/ports/AT32/AT32F402_405xx/drivers/at32f402_405_can.c
PLATFORMSRC += $(CHIBIOS)/os/hal/ports/AT32/AT32F402_405xx/drivers/at32f402_405_crc.c
PLATFORMSRC += $(CHIBIOS)/os/hal/ports/AT32/AT32F402_405xx/drivers/at32f402_405_crm.c
PLATFORMSRC += $(CHIBIOS)/os/hal/ports/AT32/AT32F402_405xx/drivers/at32f402_405_debug.c
PLATFORMSRC += $(CHIBIOS)/os/hal/ports/AT32/AT32F402_405xx/drivers/at32f402_405_dma.c
PLATFORMSRC += $(CHIBIOS)/os/hal/ports/AT32/AT32F402_405xx/drivers/at32f402_405_ertc.c
PLATFORMSRC += $(CHIBIOS)/os/hal/ports/AT32/AT32F402_405xx/drivers/at32f402_405_exint.c
PLATFORMSRC += $(CHIBIOS)/os/hal/ports/AT32/AT32F402_405xx/drivers/at32f402_405_flash.c
PLATFORMSRC += $(CHIBIOS)/os/hal/ports/AT32/AT32F402_405xx/drivers/at32f402_405_gpio.c
PLATFORMSRC += $(CHIBIOS)/os/hal/ports/AT32/AT32F402_405xx/drivers/at32f402_405_i2c.c
PLATFORMSRC += $(CHIBIOS)/os/hal/ports/AT32/AT32F402_405xx/drivers/at32f402_405_misc.c
PLATFORMSRC += $(CHIBIOS)/os/hal/ports/AT32/AT32F402_405xx/drivers/at32f402_405_pwc.c
PLATFORMSRC += $(CHIBIOS)/os/hal/ports/AT32/AT32F402_405xx/drivers/at32f402_405_qspi.c
PLATFORMSRC += $(CHIBIOS)/os/hal/ports/AT32/AT32F402_405xx/drivers/at32f402_405_scfg.c
PLATFORMSRC += $(CHIBIOS)/os/hal/ports/AT32/AT32F402_405xx/drivers/at32f402_405_spi.c
PLATFORMSRC += $(CHIBIOS)/os/hal/ports/AT32/AT32F402_405xx/drivers/at32f402_405_tmr.c
PLATFORMSRC += $(CHIBIOS)/os/hal/ports/AT32/AT32F402_405xx/drivers/at32f402_405_usart.c
PLATFORMSRC += $(CHIBIOS)/os/hal/ports/AT32/AT32F402_405xx/drivers/at32f402_405_usb.c
PLATFORMSRC += $(CHIBIOS)/os/hal/ports/AT32/AT32F402_405xx/drivers/at32f402_405_wdt.c
PLATFORMSRC += $(CHIBIOS)/os/hal/ports/AT32/AT32F402_405xx/drivers/at32f402_405_wwdt.c
PLATFORMSRC += $(CHIBIOS)/os/hal/ports/AT32/AT32F402_405xx/drivers/system_at32f402_405.c
endif
else
PLATFORMSRC += $(CHIBIOS)/os/hal/ports/AT32/AT32F402_405xx/drivers/at32f402_405_acc.c
PLATFORMSRC += $(CHIBIOS)/os/hal/ports/AT32/AT32F402_405xx/drivers/at32f402_405_adc.c
PLATFORMSRC += $(CHIBIOS)/os/hal/ports/AT32/AT32F402_405xx/drivers/at32f402_405_can.c
PLATFORMSRC += $(CHIBIOS)/os/hal/ports/AT32/AT32F402_405xx/drivers/at32f402_405_crc.c
PLATFORMSRC += $(CHIBIOS)/os/hal/ports/AT32/AT32F402_405xx/drivers/at32f402_405_crm.c
PLATFORMSRC += $(CHIBIOS)/os/hal/ports/AT32/AT32F402_405xx/drivers/at32f402_405_debug.c
PLATFORMSRC += $(CHIBIOS)/os/hal/ports/AT32/AT32F402_405xx/drivers/at32f402_405_dma.c
PLATFORMSRC += $(CHIBIOS)/os/hal/ports/AT32/AT32F402_405xx/drivers/at32f402_405_ertc.c
PLATFORMSRC += $(CHIBIOS)/os/hal/ports/AT32/AT32F402_405xx/drivers/at32f402_405_exint.c
PLATFORMSRC += $(CHIBIOS)/os/hal/ports/AT32/AT32F402_405xx/drivers/at32f402_405_flash.c
PLATFORMSRC += $(CHIBIOS)/os/hal/ports/AT32/AT32F402_405xx/drivers/at32f402_405_gpio.c
PLATFORMSRC += $(CHIBIOS)/os/hal/ports/AT32/AT32F402_405xx/drivers/at32f402_405_i2c.c
PLATFORMSRC += $(CHIBIOS)/os/hal/ports/AT32/AT32F402_405xx/drivers/at32f402_405_misc.c
PLATFORMSRC += $(CHIBIOS)/os/hal/ports/AT32/AT32F402_405xx/drivers/at32f402_405_pwc.c
PLATFORMSRC += $(CHIBIOS)/os/hal/ports/AT32/AT32F402_405xx/drivers/at32f402_405_qspi.c
PLATFORMSRC += $(CHIBIOS)/os/hal/ports/AT32/AT32F402_405xx/drivers/at32f402_405_scfg.c
PLATFORMSRC += $(CHIBIOS)/os/hal/ports/AT32/AT32F402_405xx/drivers/at32f402_405_spi.c
PLATFORMSRC += $(CHIBIOS)/os/hal/ports/AT32/AT32F402_405xx/drivers/at32f402_405_tmr.c
PLATFORMSRC += $(CHIBIOS)/os/hal/ports/AT32/AT32F402_405xx/drivers/at32f402_405_usart.c
PLATFORMSRC += $(CHIBIOS)/os/hal/ports/AT32/AT32F402_405xx/drivers/at32f402_405_usb.c
PLATFORMSRC += $(CHIBIOS)/os/hal/ports/AT32/AT32F402_405xx/drivers/at32f402_405_wdt.c
PLATFORMSRC += $(CHIBIOS)/os/hal/ports/AT32/AT32F402_405xx/drivers/at32f402_405_wwdt.c
PLATFORMSRC += $(CHIBIOS)/os/hal/ports/AT32/AT32F402_405xx/drivers/system_at32f402_405.c
endif

PLATFORMINC += $(CHIBIOS)/os/hal/ports/AT32/AT32F402_405xx/drivers
