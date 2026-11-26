ifeq ($(USE_SMART_BUILD),yes)
ifneq ($(findstring HAL_USE_SPI TRUE,$(HALCONF)),)
PLATFORMSRC += $(CHIBIOS)/os/hal/ports/AT32/LLD/SPIv2/hal_spi_v2_lld.c
endif
else
PLATFORMSRC += $(CHIBIOS)/os/hal/ports/AT32/LLD/SPIv2/hal_spi_v2_lld.c
endif

PLATFORMINC += $(CHIBIOS)/os/hal/ports/AT32/LLD/SPIv2
