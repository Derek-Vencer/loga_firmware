
/*
    Copyright (C) 2023 iOne Electrionc
*/

# include <stdio.h>
# include <string.h>
# include "hal.h"

# include "shell.h"
# include "chprintf.h"

# include "usbcfg.h"
# include "main.h"
# include "led_effect.h"

/*===========================================================================*/
/* Command line related.                                                     */
/*===========================================================================*/

#define SHELL_WA_SIZE   THD_WORKING_AREA_SIZE(2048)
#define verAddress	0x0800580C
#define SerialDebugPort					SD1

//extern uint32_t spi_timer,spi_timer_last;
extern uint32_t LED_UpdateCount;

uint32_t get_system_time_ticks(void);


int uprintf(const char* fmt, ...)
{
    va_list ap;
    int formatted_bytes;

    va_start(ap, fmt);
    formatted_bytes = chvprintf((BaseSequentialStream *)&SerialDebugPort, fmt, ap);
    va_end(ap);

    return formatted_bytes;
}


/* Can be measured using dd if=/dev/xxxx of=/dev/null bs=512 count=10000.*/
static void cmd_write(BaseSequentialStream* chp, int argc, char* argv[])
{
    static uint8_t buf[] =
        "0123456789abcdef0123456789abcdef0123456789abcdef0123456789abcdef"
      "0123456789abcdef0123456789abcdef0123456789abcdef0123456789abcdef"
      "0123456789abcdef0123456789abcdef0123456789abcdef0123456789abcdef"
      "0123456789abcdef0123456789abcdef0123456789abcdef0123456789abcdef"
      "0123456789abcdef0123456789abcdef0123456789abcdef0123456789abcdef"
      "0123456789abcdef0123456789abcdef0123456789abcdef0123456789abcdef"
      "0123456789abcdef0123456789abcdef0123456789abcdef0123456789abcdef"
      "0123456789abcdef0123456789abcdef0123456789abcdef0123456789abcdef"
      "0123456789abcdef0123456789abcdef0123456789abcdef0123456789abcdef"
      "0123456789abcdef0123456789abcdef0123456789abcdef0123456789abcdef"
      "0123456789abcdef0123456789abcdef0123456789abcdef0123456789abcdef"
      "0123456789abcdef0123456789abcdef0123456789abcdef0123456789abcdef"
      "0123456789abcdef0123456789abcdef0123456789abcdef0123456789abcdef"
      "0123456789abcdef0123456789abcdef0123456789abcdef0123456789abcdef"
      "0123456789abcdef0123456789abcdef0123456789abcdef0123456789abcdef"
      "0123456789abcdef0123456789abcdef0123456789abcdef0123456789abcdef";

    (void)argv;
    if (argc > 0)
    {
        chprintf(chp, "Usage: write\r\n");
        return;
    }

    while (chnGetTimeout((BaseChannel*)chp, TIME_IMMEDIATE) == Q_TIMEOUT)
    {
#if 1
    /* Writing in channel mode.*/
    chnWrite(&SDU1, buf, sizeof buf - 1);
#else
        /* Writing in buffer mode.*/
        (void)obqGetEmptyBufferTimeout(&SDU1.obqueue, TIME_INFINITE);
        memcpy(SDU1.obqueue.ptr, buf, SERIAL_USB_BUFFERS_SIZE);
        obqPostFullBuffer(&SDU1.obqueue, SERIAL_USB_BUFFERS_SIZE);
#endif
    }
    chprintf(chp, "\r\n\nstopped\r\n");
}

static const ShellCommand commands[] = {
  {"write", cmd_write},
  {NULL, NULL}
};

static const ShellConfig shell_cfg1 = {
  (BaseSequentialStream *)&SDU1,
  commands
};

static const ShellConfig shell_cfg2 = {
  (BaseSequentialStream *)&SDU2,
  commands
};

void uart_init(void)
{
	static const SerialConfig default_config =
	{
			57600, 0, USART_CR2_STOP1_BITS, 0
	};

    palSetPadMode(GPIOC, 10, PAL_MODE_ALTERNATE(7)); /* USART3 TX */
    palSetPadMode(GPIOC, 11, PAL_MODE_ALTERNATE(7)); /* USART3 RX */
    sdStart(&SerialDebugPort, &default_config);
    uprintf("\r\niOne M18 Start .....\r\n\r\n");
}

void otg_test(void)
{
    thread_t* shelltp1 = NULL;
    thread_t* shelltp2 = NULL;
    event_listener_t shell_el;
    /*
	* Initializes two serial-over-USB CDC drivers.��ޢ�
	*/
    sduObjectInit(&SDU1);

    sduStart(&SDU1, &serusbcfg1);

    sduObjectInit(&SDU2);

    sduStart(&SDU2, &serusbcfg2);


    /*
	* Activates the USB driver and then the USB bus pull-up on D+.
	* Note, a delay is inserted in order to not have to disconnect the cable
	* after a reset.
	*/

    usbDisconnectBus(serusbcfg1.usbp);

    chThdSleepMilliseconds(1500);

    usbStart(serusbcfg1.usbp, &usbcfg);

    usbConnectBus(serusbcfg1.usbp);

    shellInit();

    chEvtRegister(&shell_terminated, &shell_el, 0);

    /*
	* Normal main() thread activity, managing two shells.
	*/
    while (true)
    {

        if (SDU1.config->usbp->state == USB_ACTIVE)
        {

            /* Starting shells.*/

            if (shelltp1 == NULL)
            {
                shelltp1 = chThdCreateFromHeap(NULL, SHELL_WA_SIZE,
                                                 "shell1", NORMALPRIO + 1,
                                                 shellThread, (void*)&shell_cfg1);
            }

            if (shelltp2 == NULL)
            {
                shelltp2 = chThdCreateFromHeap(NULL, SHELL_WA_SIZE,
                                               "shell2", NORMALPRIO + 1,
                                               shellThread, (void*)&shell_cfg2);
            }

            /* Waiting for an exit event then freeing terminated shells.*/
            chEvtWaitAny(EVENT_MASK(0));
            if (chThdTerminatedX(shelltp1))
            {
                chThdRelease(shelltp1);
                shelltp1 = NULL;
            }
            if (chThdTerminatedX(shelltp2))
            {
                chThdRelease(shelltp2);
                shelltp2 = NULL;
            }
        }
        else
        {
            chThdSleepMilliseconds(1000);
        }
    }

}


#define ADC_GRP1_NUM_CHANNELS   1
#define ADC_GRP1_BUF_DEPTH      8

#define ADC_GRP2_NUM_CHANNELS   8
#define ADC_GRP2_BUF_DEPTH      16

static adcsample_t samples1[ADC_GRP1_NUM_CHANNELS * ADC_GRP1_BUF_DEPTH];
static adcsample_t samples2[ADC_GRP2_NUM_CHANNELS * ADC_GRP2_BUF_DEPTH];

/*
 * ADC streaming callback.
 */
size_t nx = 0, ny = 0;
static void adccallback(ADCDriver* adcp)
{

    if (adcIsBufferComplete(adcp))
    {
        nx += 1;
    }
    else
    {
        ny += 1;
    }
}

static void adcerrorcallback(ADCDriver* adcp, adcerror_t err)
{

    (void)adcp;
    (void)err;
}

/*
 * ADC conversion group.
 * Mode:        Linear buffer, 8 samples of 1 channel, SW triggered.
 * Channels:    IN11.
 */
static const ADCConversionGroup adcgrpcfg1 = {
  FALSE,
  ADC_GRP1_NUM_CHANNELS,
  NULL,
  adcerrorcallback,
  0,                        /* CR1 */
  ADC_CR2_SWSTART,          /* CR2 */
  ADC_SMPR1_SMP_AN11(ADC_SAMPLE_640_5),
  0,                        /* SMPR2 */
  0,                        /* HTR */
  0,                        /* LTR */
  0,                        /* SQR1 */
  0,                        /* SQR2 */
  ADC_SQR3_SQ1_N(ADC_CHANNEL_IN11)
};

/*
 * ADC conversion group.
 * Mode:        Continuous, 16 samples of 8 channels, SW triggered.
 * Channels:    IN11, IN12, IN11, IN12, IN11, IN12, Sensor, VRef.
 */
static const ADCConversionGroup adcgrpcfg2 = {
  TRUE,
  ADC_GRP2_NUM_CHANNELS,
  adccallback,
  adcerrorcallback,
  0,                        /* CR1 */
  ADC_CR2_SWSTART,          /* CR2 */
  ADC_SMPR1_SMP_AN12(ADC_SAMPLE_47_5) | ADC_SMPR1_SMP_AN11(ADC_SAMPLE_47_5) |
  ADC_SMPR1_SMP_SENSOR(ADC_SAMPLE_92_5) | ADC_SMPR1_SMP_VREF(ADC_SAMPLE_92_5),
  0,                        /* SMPR2 */
  0,                        /* HTR */
  0,                        /* LTR */
  0,                        /* SQR1 */
  ADC_SQR2_SQ8_N(ADC_CHANNEL_SENSOR) | ADC_SQR2_SQ7_N(ADC_CHANNEL_VREFINT),
  ADC_SQR3_SQ6_N(ADC_CHANNEL_IN12)   | ADC_SQR3_SQ5_N(ADC_CHANNEL_IN11) |
  ADC_SQR3_SQ4_N(ADC_CHANNEL_IN12)   | ADC_SQR3_SQ3_N(ADC_CHANNEL_IN11) |
  ADC_SQR3_SQ2_N(ADC_CHANNEL_IN12)   | ADC_SQR3_SQ1_N(ADC_CHANNEL_IN11)
};

//void adc_test(void)
//{
//    uprintf ("ADC Test:");
//    palSetGroupMode(GPIOC, PAL_PORT_BIT(1) | PAL_PORT_BIT(2),
//                    0, PAL_MODE_INPUT_ANALOG);
//
//    /*
//	 * Activates the ADC1 driver and the temperature sensor.
//	 */
//    adcStart(&ADCD1, NULL);
//    adcSTM32EnableTSVREFE();
//
//    /*
//	 * Linear conversion.
//	 */
//    adcConvert(&ADCD1, &adcgrpcfg1, samples1, ADC_GRP1_BUF_DEPTH);
//    for (uint8_t i=0;i<ADC_GRP2_BUF_DEPTH;i++){
//    	uprintf ("AN4[%d]:0x%04X\r\n",samples1[i]);
//    }
//    chThdSleepMilliseconds(1000);
//
//    /*
//	 * Starts an ADC continuous conversion.
//	 */
//    adcStartConversion(&ADCD1, &adcgrpcfg2, samples2, ADC_GRP2_BUF_DEPTH);
//
//
//}

int main(void)
{//
    uint16_t blVerNumber, fwVerNumber;
    uint16_t temp;
    temp = *((uint16_t *)(verAddress));
    blVerNumber = temp >>8;
    blVerNumber |= (temp<<8);
    temp = *((uint16_t *)(verAddress+2));
    fwVerNumber = temp >>8;
    fwVerNumber |= (temp<<8);

    // osalThreadSleepMilliseconds(10000); // lyon led delay not add in here 
    // chThdSleepMilliseconds(1000);   // 延遲 1000 毫秒 = 1 秒
    halInit();
    chSysInit();
    uart_init();
    uprintf("\r\niOne M18 Start .....\r\n\r\n");
    //uprintf ("Bootloader ver:%04X Firmware ver:%04X\r\n",*((uint16_t *)(verAddress)),*((uint16_t *)(verAddress+2)));
    uprintf ("Bootloader ver:%04X Firmware ver:%04X\r\n",blVerNumber,fwVerNumber);
//	LED_init();

    /* Watchdog 設定 */
    static const WDGConfig wdgcfg = {
        .pr  = STM32_IWDG_PR_256,   // 分頻 256
        .rlr = 156 * 2              // Reload 值 -> 約 2 秒 timeout
    };
    wdgStart(&WDGD1, &wdgcfg);

    while (1)
    {
    	while(1){
    	//uprintf ("Timer:%d LED_SPI:%d\r\n",get_system_time_ticks()/1000,spi_timer_last);
    		uprintf ("Timer:%d SPI_End:%d\r\n",get_system_time_ticks()/1000,LED_UpdateCount);
    		LED_UpdateCount = 0;
    	chThdSleepMilliseconds(1000);
        wdgReset(&WDGD1); //lyon WD
    	}

    adc_test();
    otg_test();
    }

}

