ifeq ($(USE_SMART_BUILD),yes)
ifneq ($(findstring HAL_USE_CRC TRUE,$(HALCONF)),)
PLATFORMSRC += ${CHIBIOS}/os/hal/ports/AT32/LLD/CRCv1/hal_crc_lld.c
endif
else
PLATFORMSRC += ${CHIBIOS}/os/hal/ports/AT32/LLD/CRCv1/hal_crc_lld.c
endif

PLATFORMINC += ${CHIBIOS}/os/hal/ports/AT32/LLD/CRCv1
