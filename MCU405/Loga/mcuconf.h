/*
    ChibiOS - Copyright (C) 2006..2020 Giovanni Di Sirio

    Licensed under the Apache License, Version 2.0 (the "License");
    you may not use this file except in compliance with the License.
    You may obtain a copy of the License at

        http://www.apache.org/licenses/LICENSE-2.0

    Unless required by applicable law or agreed to in writing, software
    distributed under the License is distributed on an "AS IS" BASIS,
    WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
    See the License for the specific language governing permissions and
    limitations under the License.
*/

#ifndef MCUCONF_H
#define MCUCONF_H

/*
 * STM32F4xx drivers configuration.
 * The following settings override the default settings present in
 * the various device driver implementation headers.
 * Note that the settings for each driver only have effect if the whole
 * driver is enabled in halconf.h.
 *
 * IRQ priorities:
 * 15...0       Lowest...Highest.
 *
 * DMA priorities:
 * 0...3        Lowest...Highest.
 */


#define AT32F405xx_MCUCONF

/*
 * HAL driver system settings.
 */
#define AT32_NO_INIT                       FALSE
#define AT32_PVD_ENABLE                    FALSE
#define AT32_PLS                           AT32_PLS_LEV0
#define AT32_BKPRAM_ENABLE                 FALSE
#define AT32_HSI_ENABLED                   TRUE
#define AT32_LSI_ENABLED                   TRUE
#define AT32_HSE_ENABLED                   TRUE
#define AT32_LSE_ENABLED                   FALSE
#define AT32_CLOCK48_REQUIRED              TRUE
#define AT32_SW                            AT32_SW_PLL
#define AT32_PLLSRC                        AT32_PLLSRC_HSE
#define AT32_PLLM_VALUE                    8
#define AT32_PLLN_VALUE                    360
#define AT32_PLLP_VALUE                    2
#define AT32_PLLQ_VALUE                    7
#define AT32_PLLI2SN_VALUE                 192
#define AT32_PLLI2SM_VALUE                 4
#define AT32_PLLI2SR_VALUE                 4
#define AT32_PLLI2SP_VALUE                 4
#define AT32_PLLI2SQ_VALUE                 4
#define AT32_PLLSAIN_VALUE                 192
#define AT32_PLLSAIM_VALUE                 4
#define AT32_PLLSAIP_VALUE                 8
#define AT32_PLLSAIQ_VALUE                 4
#define AT32_HPRE                          AT32_HPRE_DIV1
#define AT32_PPRE1                         AT32_PPRE1_DIV4
#define AT32_PPRE2                         AT32_PPRE2_DIV2
#define AT32_RTCSEL                        AT32_RTCSEL_LSI
#define AT32_RTCPRE_VALUE                  8
#define AT32_MCO1SEL                       AT32_MCO1SEL_HSI
#define AT32_MCO1PRE                       AT32_MCO1PRE_DIV1
#define AT32_MCO2SEL                       AT32_MCO2SEL_PLLI2S
#define AT32_MCO2PRE                       AT32_MCO2PRE_DIV1
#define AT32_I2SSRC                        AT32_I2SSRC_PLLI2S
#define AT32_SAI1SEL                       AT32_SAI2SEL_PLLR
#define AT32_SAI2SEL                       AT32_SAI2SEL_PLLR
#define AT32_CK48MSEL                      AT32_CK48MSEL_PLLALT

/*
 * IRQ system settings.
 */
#define AT32_IRQ_EXINT0_PRIORITY            6
#define AT32_IRQ_EXINT1_PRIORITY            6
#define AT32_IRQ_EXINT2_PRIORITY            6
#define AT32_IRQ_EXINT3_PRIORITY            6
#define AT32_IRQ_EXINT4_PRIORITY            6
#define AT32_IRQ_EXINT5_9_PRIORITY          6
#define AT32_IRQ_EXINT10_15_PRIORITY        6
#define AT32_IRQ_EXINT16_PRIORITY           6
#define AT32_IRQ_EXINT17_PRIORITY           15
#define AT32_IRQ_EXINT18_PRIORITY           6
#define AT32_IRQ_EXINT19_PRIORITY           6
#define AT32_IRQ_EXINT20_PRIORITY           6
#define AT32_IRQ_EXINT21_PRIORITY           15
#define AT32_IRQ_EXINT22_PRIORITY           15

#define AT32_IRQ_TMR1_BRK_TMR9_PRIORITY     7
#define AT32_IRQ_TMR1_OVF_TMR10_PRIORITY    7
#define AT32_IRQ_TMR1_HALL_TMR11_PRIORITY   7
#define AT32_IRQ_TMR1_CH_PRIORITY           7
#define AT32_IRQ_TMR2_PRIORITY              7
#define AT32_IRQ_TMR3_PRIORITY              7
#define AT32_IRQ_TMR4_PRIORITY              7
#define AT32_IRQ_TMR5_PRIORITY              7
#define AT32_IRQ_TMR6_PRIORITY              7
#define AT32_IRQ_TMR7_PRIORITY              7
#define AT32_IRQ_TMR13_PRIORITY             7
#define AT32_IRQ_TMR14_PRIORITY             7

#define AT32_IRQ_USART1_PRIORITY           12
#define AT32_IRQ_USART2_PRIORITY           12
#define AT32_IRQ_USART3_PRIORITY           12
#define AT32_IRQ_UART4_PRIORITY            12
#define AT32_IRQ_UART5_PRIORITY            12
#define AT32_IRQ_USART6_PRIORITY           12
#define AT32_IRQ_UART7_PRIORITY            12
#define AT32_IRQ_UART8_PRIORITY            12

#define AT32_MCU_HAVE_TMR1      TRUE
#define AT32_MCU_HAVE_TMR2      TRUE
#define AT32_MCU_HAVE_TMR3      TRUE
#define AT32_MCU_HAVE_TMR4      TRUE
#define AT32_MCU_HAVE_TMR5      FALSE
#define AT32_MCU_HAVE_TMR6      TRUE
#define AT32_MCU_HAVE_TMR7      TRUE
#define AT32_MCU_HAVE_TMR8      FALSE
#define AT32_MCU_HAVE_TMR9      TRUE
#define AT32_MCU_HAVE_TMR10      TRUE
#define AT32_MCU_HAVE_TMR11      TRUE
#define AT32_MCU_HAVE_TMR12      FALSE
#define AT32_MCU_HAVE_TMR13      TRUE
#define AT32_MCU_HAVE_TMR14      TRUE
#define AT32_MCU_HAVE_TMR15      FALSE
#define AT32_MCU_HAVE_TMR16      FALSE
#define AT32_MCU_HAVE_TMR17      FALSE
#define AT32_MCU_HAVE_TMR18      FALSE
#define AT32_MCU_HAVE_TMR19      FALSE
#define AT32_MCU_HAVE_TMR20      FALSE

/*
 * ADC driver system settings.
 */
//#define AT32_ADC_ADCDIV                    8
//#define AT32_ADC_USE_ADC1                  TRUE
//#define AT32_ADC_USE_ADC2                  FALSE
//#define AT32_ADC_USE_ADC3                  FALSE
//#define AT32_ADC_ADC1_DMA_STREAM           AT32_DMA_STREAM_ID_ANY
//#define AT32_ADC_ADC2_DMA_STREAM           AT32_DMA_STREAM_ID_ANY
//#define AT32_ADC_ADC3_DMA_STREAM           AT32_DMA_STREAM_ID_ANY
////#define AT32_ADC_ADC1_DMA_PRIORITY         2
//#define AT32_ADC_ADC2_DMA_PRIORITY         2
//#define AT32_ADC_ADC3_DMA_PRIORITY         2
//#define AT32_ADC_IRQ_PRIORITY              6
////#define AT32_ADC_ADC1_DMA_IRQ_PRIORITY     6
//// #define AT32_ADC_ADC2_DMA_IRQ_PRIORITY     6
//// #define AT32_ADC_ADC3_DMA_IRQ_PRIORITY     6


/*
 * ADC driver system settings.
 */
#define AT32_ADC_ADCDIV                    8
#define AT32_ADC_USE_ADC1                   TRUE
#define AT32_ADC_ADC1_DMA_PRIORITY          2
#define AT32_ADC_ADC1_DMA_IRQ_PRIORITY     5

/*
 * CAN driver system settings.
 */
#define AT32_CAN_USE_CAN1                  FALSE
#define AT32_CAN_USE_CAN2                  FALSE
#define AT32_CAN_CAN1_IRQ_PRIORITY         11
#define AT32_CAN_CAN2_IRQ_PRIORITY         11

/*
 * DAC driver system settings.
 */
#define AT32_DAC_DUAL_MODE                 FALSE
#define AT32_DAC_USE_DAC1_CH1              FALSE
#define AT32_DAC_USE_DAC1_CH2              FALSE
#define AT32_DAC_DAC1_CH1_IRQ_PRIORITY     10
#define AT32_DAC_DAC1_CH2_IRQ_PRIORITY     10
#define AT32_DAC_DAC1_CH1_DMA_PRIORITY     2
#define AT32_DAC_DAC1_CH2_DMA_PRIORITY     2
#define AT32_DAC_DAC1_CH1_DMA_STREAM       AT32_DMA_STREAM_ID_ANY
#define AT32_DAC_DAC1_CH2_DMA_STREAM       AT32_DMA_STREAM_ID_ANY

/*
 * GPT driver system settings.
 */
#define AT32_GPT_USE_TMR1                  FALSE
#define AT32_GPT_USE_TMR2                  FALSE
#define AT32_GPT_USE_TMR3                  FALSE
#define AT32_GPT_USE_TMR4                  TRUE
#define AT32_GPT_USE_TMR5                  FALSE
#define AT32_GPT_USE_TMR6                  FALSE
#define AT32_GPT_USE_TMR7                  FALSE
#define AT32_GPT_USE_TMR8                  FALSE
#define AT32_GPT_USE_TMR9                  TRUE
#define AT32_GPT_USE_TMR10                 FALSE
#define AT32_GPT_USE_TMR11                 FALSE
#define AT32_GPT_USE_TMR12                 FALSE
#define AT32_GPT_USE_TMR13                 FALSE
#define AT32_GPT_USE_TMR14                 FALSE
#define AT32_GPT_USE_TMR20                 TRUE

/*
 * I2C driver system settings.
 */
#define AT32_I2C_USE_I2C1                  TRUE
#define AT32_I2C_USE_I2C2                  FALSE
#define AT32_I2C_USE_I2C3                  FALSE
//#define I2C_TypeDef                         i2c_type
#define AT32_I2C_BUSY_TIMEOUT              50
#define AT32_I2C_I2C1_RX_DMA_STREAM        AT32_DMA_STREAM_ID_ANY
#define AT32_I2C_I2C1_TX_DMA_STREAM        AT32_DMA_STREAM_ID_ANY
#define AT32_I2C_I2C2_RX_DMA_STREAM        AT32_DMA_STREAM_ID_ANY
#define AT32_I2C_I2C2_TX_DMA_STREAM        AT32_DMA_STREAM_ID_ANY
#define AT32_I2C_I2C3_RX_DMA_STREAM        AT32_DMA_STREAM_ID_ANY
#define AT32_I2C_I2C3_TX_DMA_STREAM        AT32_DMA_STREAM_ID_ANY
#define AT32_I2C_I2C1_IRQ_PRIORITY         5
#define AT32_I2C_I2C2_IRQ_PRIORITY         5
#define AT32_I2C_I2C3_IRQ_PRIORITY         5
#define AT32_I2C_I2C1_DMA_PRIORITY         3
#define AT32_I2C_I2C2_DMA_PRIORITY         3
#define AT32_I2C_I2C3_DMA_PRIORITY         3
#define AT32_I2C_DMA_ERROR_HOOK(i2cp)      osalSysHalt("DMA failure")
/*
 * I2S driver system settings.
 */
#define AT32_I2S_USE_SPI2                  FALSE
#define AT32_I2S_USE_SPI3                  FALSE
#define AT32_I2S_SPI2_IRQ_PRIORITY         10
#define AT32_I2S_SPI3_IRQ_PRIORITY         10
#define AT32_I2S_SPI2_DMA_PRIORITY         1
#define AT32_I2S_SPI3_DMA_PRIORITY         1
#define AT32_I2S_SPI2_RX_DMA_STREAM        AT32_DMA_STREAM_ID_ANY
#define AT32_I2S_SPI2_TX_DMA_STREAM        AT32_DMA_STREAM_ID_ANY
#define AT32_I2S_SPI3_RX_DMA_STREAM        AT32_DMA_STREAM_ID_ANY
#define AT32_I2S_SPI3_TX_DMA_STREAM        AT32_DMA_STREAM_ID_ANY
#define AT32_I2S_DMA_ERROR_HOOK(i2sp)      osalSysHalt("DMA failure")

/*
 * ICU driver system settings.
 */
#define AT32_ICU_USE_TIM1                  FALSE
#define AT32_ICU_USE_TIM2                  FALSE
#define AT32_ICU_USE_TIM3                  FALSE
#define AT32_ICU_USE_TIM4                  FALSE
#define AT32_ICU_USE_TIM9                  FALSE
#define AT32_ICU_USE_TIM10                 TRUE
#define AT32_ICU_USE_TIM11                 FALSE
#define AT32_ICU_USE_TIM13                 FALSE
#define AT32_ICU_USE_TIM14                 FALSE
#define AT32_ICU_USE_TIM20                 FALSE

/*
 * MAC driver system settings.
 */
#define AT32_MAC_TRANSMIT_BUFFERS          2
#define AT32_MAC_RECEIVE_BUFFERS           4
#define AT32_MAC_BUFFERS_SIZE              1522
#define AT32_MAC_PHY_TIMEOUT               100
#define AT32_MAC_ETH1_CHANGE_PHY_STATE     TRUE
#define AT32_MAC_ETH1_IRQ_PRIORITY         13
#define AT32_MAC_IP_CHECKSUM_OFFLOAD       0

/*
 * PWM driver system settings.
 */
#define AT32_PWM_USE_TIM1                  TRUE
#define AT32_PWM_USE_TIM2                  TRUE
#define AT32_PWM_USE_TIM3                  FALSE
#define AT32_PWM_USE_TIM4                  FALSE
#define AT32_PWM_USE_TIM5                  FALSE
#define AT32_PWM_USE_TIM8                  FALSE
#define AT32_PWM_USE_TIM9                  TRUE
#define AT32_PWM_USE_TIM10                 FALSE
#define AT32_PWM_USE_TIM11                 FALSE
#define AT32_PWM_USE_TIM12                 FALSE
#define AT32_PWM_USE_TIM13                 FALSE
#define AT32_PWM_USE_TIM14                 FALSE
#define AT32_PWM_USE_TIM20                 FALSE

/*
 * RTC driver system settings.
 */
#define AT32_RTC_PRESA_VALUE               32
#define AT32_RTC_PRESS_VALUE               1024
#define AT32_RTC_CR_INIT                   0
#define AT32_RTC_TAMPCR_INIT               0

/*
 * SDC driver system settings.
 */
#define AT32_SDC_SDIO_DMA_PRIORITY         3
#define AT32_SDC_SDIO_IRQ_PRIORITY         9
#define AT32_SDC_WRITE_TIMEOUT_MS          1000
#define AT32_SDC_READ_TIMEOUT_MS           1000
#define AT32_SDC_CLOCK_ACTIVATION_DELAY    10
#define AT32_SDC_SDIO_UNALIGNED_SUPPORT    TRUE
#define AT32_SDC_SDIO_DMA_STREAM          AT32_DMA_STREAM_ID_ANY

/*
 * SERIAL driver system settings.
 */
#define AT32_SERIAL_USE_USART1             FALSE			//FALSE
#define AT32_SERIAL_USE_USART2             FALSE
#define AT32_SERIAL_USE_USART3             TRUE			//TRUE
#define AT32_SERIAL_USE_UART4              FALSE
#define AT32_SERIAL_USE_UART5              FALSE
#define AT32_SERIAL_USE_USART6             FALSE
#define AT32_SERIAL_USE_UART7              FALSE
#define AT32_SERIAL_USE_UART8              FALSE

/*
 * SPI driver system settings.
 */
#define AT32_SPI_USE_SPI1                  TRUE
#define AT32_SPI_USE_SPI2                  TRUE
#define AT32_SPI_USE_SPI3                  TRUE
#define AT32_SPI_USE_SPI4                  FALSE
#define AT32_SPI_USE_SPI5                  FALSE
#define AT32_SPI_USE_SPI6                  FALSE
#define AT32_SPI_SPI1_RX_DMA_STREAM        AT32_DMA_STREAM_ID_ANY
#define AT32_SPI_SPI1_TX_DMA_STREAM        AT32_DMA_STREAM_ID_ANY
#define AT32_SPI_SPI2_RX_DMA_STREAM        AT32_DMA_STREAM_ID_ANY
#define AT32_SPI_SPI2_TX_DMA_STREAM        AT32_DMA_STREAM_ID_ANY
#define AT32_SPI_SPI3_RX_DMA_STREAM        AT32_DMA_STREAM_ID_ANY
#define AT32_SPI_SPI3_TX_DMA_STREAM        AT32_DMA_STREAM_ID_ANY
#define AT32_SPI_SPI4_RX_DMA_STREAM        AT32_DMA_STREAM_ID_ANY
#define AT32_SPI_SPI4_TX_DMA_STREAM        AT32_DMA_STREAM_ID_ANY
#define AT32_SPI_SPI5_RX_DMA_STREAM        AT32_DMA_STREAM_ID_ANY
#define AT32_SPI_SPI5_TX_DMA_STREAM        AT32_DMA_STREAM_ID_ANY
#define AT32_SPI_SPI6_RX_DMA_STREAM        AT32_DMA_STREAM_ID_ANY
#define AT32_SPI_SPI6_TX_DMA_STREAM        AT32_DMA_STREAM_ID_ANY
#define AT32_SPI_SPI1_DMA_PRIORITY         1
#define AT32_SPI_SPI2_DMA_PRIORITY         1
#define AT32_SPI_SPI3_DMA_PRIORITY         1
#define AT32_SPI_SPI4_DMA_PRIORITY         1
#define AT32_SPI_SPI5_DMA_PRIORITY         1
#define AT32_SPI_SPI6_DMA_PRIORITY         1
#define AT32_SPI_SPI1_IRQ_PRIORITY         10
#define AT32_SPI_SPI2_IRQ_PRIORITY         10
#define AT32_SPI_SPI3_IRQ_PRIORITY         10
#define AT32_SPI_SPI4_IRQ_PRIORITY         10
#define AT32_SPI_SPI5_IRQ_PRIORITY         10
#define AT32_SPI_SPI6_IRQ_PRIORITY         10
#define AT32_SPI_DMA_ERROR_HOOK(spip)      osalSysHalt("DMA failure")

/*
 * ST driver system settings.
 */
#define AT32_ST_IRQ_PRIORITY               8
#define AT32_ST_USE_TIMER                  2

/*
 * UART driver system settings.
 */
#define AT32_UART_USE_USART1               TRUE			//TRUE
#define AT32_UART_USE_USART2               FALSE
#define AT32_UART_USE_USART3               FALSE			//FALSE
#define AT32_UART_USE_UART4                FALSE
#define AT32_UART_USE_UART5                FALSE
#define AT32_UART_USE_USART6               FALSE
#define AT32_UART_USART1_RX_DMA_STREAM     AT32_DMA_STREAM_ID_ANY
#define AT32_UART_USART1_TX_DMA_STREAM     AT32_DMA_STREAM_ID_ANY
#define AT32_UART_USART2_RX_DMA_STREAM     AT32_DMA_STREAM_ID_ANY
#define AT32_UART_USART2_TX_DMA_STREAM     AT32_DMA_STREAM_ID_ANY
#define AT32_UART_USART3_RX_DMA_STREAM     AT32_DMA_STREAM_ID_ANY
#define AT32_UART_USART3_TX_DMA_STREAM     AT32_DMA_STREAM_ID_ANY
#define AT32_UART_UART4_RX_DMA_STREAM      AT32_DMA_STREAM_ID_ANY
#define AT32_UART_UART4_TX_DMA_STREAM      AT32_DMA_STREAM_ID_ANY
#define AT32_UART_UART5_RX_DMA_STREAM      AT32_DMA_STREAM_ID_ANY
#define AT32_UART_UART5_TX_DMA_STREAM      AT32_DMA_STREAM_ID_ANY
#define AT32_UART_USART6_RX_DMA_STREAM     AT32_DMA_STREAM_ID_ANY
#define AT32_UART_USART6_TX_DMA_STREAM     AT32_DMA_STREAM_ID_ANY
#define AT32_UART_USART1_DMA_PRIORITY      0
#define AT32_UART_USART2_DMA_PRIORITY      0
#define AT32_UART_USART3_DMA_PRIORITY      0
#define AT32_UART_UART4_DMA_PRIORITY       0
#define AT32_UART_UART5_DMA_PRIORITY       0
#define AT32_UART_USART6_DMA_PRIORITY      0
#define AT32_UART_DMA_ERROR_HOOK(uartp)    osalSysHalt("DMA failure")

// /*
//  * USB driver system settings.
//  */
// #define AT32_USB_USE_OTG1                  FALSE
// #define AT32_USB_USE_OTG2                  TRUE
// #if AT32_USB_USE_OTG2
// #define AT32_USE_USB_OTG2_HS				TRUE
// #define AT32_USE_USB_OTG2_HS_DMA           TRUE
// #endif
// #define AT32_USB_OTG1_IRQ_PRIORITY         13
// #define AT32_USB_OTG2_IRQ_PRIORITY         13
// #define AT32_USB_OTG1_RX_FIFO_SIZE         512
// #define AT32_USB_OTG2_RX_FIFO_SIZE         512
// #define AT32_USB_HOST_WAKEUP_DURATION      2

/*
 * USB driver system settings.
 */
#define AT32_USB_USE_OTG1                   FALSE
#define AT32_USB_OTG1_IRQ_PRIORITY          14
#define AT32_USB_OTG1_RX_FIFO_SIZE          512

#define AT32_USB_USE_OTG2                   TRUE
#define AT32_USB_OTG2_IRQ_PRIORITY          14
#define AT32_USB_OTG2_RX_FIFO_SIZE          512
#define AT32_USE_USB_OTG2_HS                TRUE
#define AT32_OTG2_SUPPORTS_HS               TRUE
#define AT32_USE_USB_OTG2_HS_DMA			TRUE

#define STM32_CRC_USE_CRC1                  TRUE
#define AT32_CRC_USE_CRC1                   TRUE
#define CRC_USE_DMA                         TRUE
#define AT32_CRC_CRC1_DMA_IRQ_PRIORITY      5
#define AT32_CRC_CRC1_DMA_PRIORITY          2
//#define AT32_CRC_CRC1_DMA_STREAM            AT32_DMA_STREAM_ID_ANY
#define CRCSW_USE_CRC1                      FALSE


//#define AT32_USE_DMA_V1                     TRUE


/*
 * CRC driver system settings.
 */
#define AT32_CRC_USE_CRC1                  TRUE
#define AT32_CRC_CRC1_DMA_IRQ_PRIORITY     5
#define AT32_CRC_CRC1_DMA_PRIORITY         2
#define AT32_CRC_CRC1_DMA_STREAM           AT32_DMA1_STREAM2

#define CRCSW_USE_CRC1                      FALSE
#define CRCSW_CRC32_TABLE                   TRUE
#define CRCSW_CRC16_TABLE                   TRUE
#define CRCSW_PROGRAMMABLE                  TRUE


/*
 * WDG driver system settings.
 */
#define AT32_WDG_USE_IWDG                  TRUE // lyon

/*
 * EXTI driver system settings.
 */
//#define AT32_EXTI_REQUIRED                 TRUE
#define EXTI                                EXINT

#endif /* MCUCONF_H */
