/*
    ChibiOS - Copyright (C) 2006..2018 Giovanni Di Sirio

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

/**
 * @file    GPIOv2/hal_pal_lld.c
 * @brief   STM32 PAL low level driver code.
 *
 * @addtogroup PAL
 * @{
 */

#include "hal.h"

#if HAL_USE_PAL || defined(__DOXYGEN__)

/*===========================================================================*/
/* Driver local definitions.                                                 */
/*===========================================================================*/

/*===========================================================================*/
/* Driver exported variables.                                                */
/*===========================================================================*/

#if (PAL_USE_WAIT == TRUE) || (PAL_USE_CALLBACKS == TRUE) || defined(__DOXYGEN__)
/**
 * @brief   Event records for the 16 GPIO EXTI channels.
 */
palevent_t _pal_events[16];
#endif

/*===========================================================================*/
/* Driver local variables and types.                                         */
/*===========================================================================*/

/*===========================================================================*/
/* Driver local functions.                                                   */
/*===========================================================================*/

/*===========================================================================*/
/* Driver interrupt handlers.                                                */
/*===========================================================================*/

/*===========================================================================*/
/* Driver exported functions.                                                */
/*===========================================================================*/

/**
 * @brief   PAL driver initialization.
 *
 * @notapi
 */
void _pal_lld_init(void) {

#if PAL_USE_CALLBACKS || PAL_USE_WAIT || defined(__DOXYGEN__)
  unsigned i;

  for (i = 0; i < 16; i++) {
    _pal_init_event(i);
  }
#endif
}

/**
 * @brief   Pads mode setup.
 * @details This function programs a pads group belonging to the same port
 *          with the specified mode.
 * @note    @p PAL_MODE_UNCONNECTED is implemented as push pull at minimum
 *          speed.
 *
 * @param[in] port      the port identifier
 * @param[in] mask      the group mask
 * @param[in] mode      the mode
 *
 * @notapi
 */
void _pal_lld_setgroupmode(ioportid_t port,
                           ioportmask_t mask,
                           iomode_t mode) {

  uint32_t moder   = (mode & PAL_STM32_MODE_MASK) >> 0;
  uint32_t otyper  = (mode & PAL_STM32_OTYPE_MASK) >> 2;
  uint32_t ospeedr = (mode & PAL_STM32_OSPEED_MASK) >> 3;
  uint32_t pupdr   = (mode & PAL_STM32_PUPDR_MASK) >> 5;
  uint32_t altr    = (mode & PAL_STM32_ALTERNATE_MASK) >> 7;
  uint32_t bit     = 0;
  while (true) {
    if ((mask & 1) != 0) {
      uint32_t altrmask, m1, m2, m4;

      altrmask = altr << ((bit & 7) * 4);
      m1 = 1 << bit;
      m2 = 3 << (bit * 2);
      m4 = 15 << ((bit & 7) * 4);
      port->OMODE  = (port->OMODE & ~m1) | otyper;
      port->ODRVR = (port->ODRVR & ~m2) | ospeedr;
      port->PULL   = (port->PULL & ~m2) | pupdr;
      if ((mode & PAL_STM32_MODE_MASK) == PAL_STM32_MODE_ALTERNATE) {
        /* If going in alternate mode then the alternate number is set
           before switching mode in order to avoid glitches.*/
        if (bit < 8)
          port->MUXL = (port->MUXL & ~m4) | altrmask;
        else
          port->MUXH = (port->MUXH & ~m4) | altrmask;
        port->CFGR   = (port->CFGR & ~m2) | moder;
      }
      else {
        /* If going into a non-alternate mode then the mode is switched
           before setting the alternate mode in order to avoid glitches.*/
        port->CFGR   = (port->CFGR & ~m2) | moder;
        if (bit < 8)
          port->MUXL = (port->MUXL & ~m4) | altrmask;
        else
          port->MUXH = (port->MUXH & ~m4) | altrmask;
      }
    }
    mask >>= 1;
    if (!mask)
      return;
    otyper <<= 1;
    ospeedr <<= 2;
    pupdr <<= 2;
    moder <<= 2;
    bit++;
  }
}

#if PAL_USE_CALLBACKS || PAL_USE_WAIT || defined(__DOXYGEN__)
/**
 * @brief   Pad event enable.
 * @note    Programming an unknown or unsupported mode is silently ignored.
 *
 * @param[in] port      port identifier
 * @param[in] pad       pad number within the port
 * @param[in] mode      pad event mode
 *
 * @notapi
 */
void _pal_lld_enablepadevent(ioportid_t port,
                             iopadid_t pad,
                             ioeventmode_t mode) {

  uint32_t padmask, cridx, croff, crmask, portidx;

  crm_periph_clock_enable(CRM_SCFG_PERIPH_CLOCK, TRUE);
  /* Mask of the pad.*/
  padmask = 1U << (uint32_t)pad;

  /* Multiple channel setting of the same channel not allowed, first disable
     it. This is done because on STM32 the same channel cannot be mapped on
     multiple ports.*/
  osalDbgAssert(((EXTI->polcfg1 & padmask) == 0U) &&
                ((EXTI->polcfg2 & padmask) == 0U), "channel already in use");

  /* Port index is obtained assuming that GPIO ports are placed at regular
     0x400 intervals in memory space. So far this is true for all devices.*/
  portidx = (((uint32_t)port - (uint32_t)GPIOA) >> 10U) & 0xFU;

  /* Index and mask of the CR register to be used.*/
  cridx  = (uint32_t)pad >> 2U;
#if STM32_EXTI_HAS_CR == FALSE
  croff  = ((uint32_t)pad & 3U) * 4U;
    crmask = ~(0xFU << croff);
   // SCFG->EXTICR[cridx] = (SYSCFG->EXTICR[cridx] & crmask) | (portidx << croff);
    if(cridx == 0)
    {
  	  SCFG->exintc1 = (SCFG->exintc1 & crmask) | (portidx << croff);
    }
    if(cridx == 1)
    {
    	  SCFG->exintc2 = (SCFG->exintc2 & crmask) | (portidx << croff);
    }
    if(cridx == 3)
    {
    	  SCFG->exintc3 = (SCFG->exintc3 & crmask) | (portidx << croff);
    }
    if(cridx == 4)
    {
    	  SCFG->exintc4 = (SCFG->exintc4 & crmask) | (portidx << croff);
    }
#else
    croff  = ((uint32_t)pad & 3U) * 4U;
      crmask = ~(0xFU << croff);
     // SCFG->EXTICR[cridx] = (SYSCFG->EXTICR[cridx] & crmask) | (portidx << croff);
      if(cridx == 0)
      {
    	  SCFG->exintc1 = (SCFG->exintc1 & crmask) | (portidx << croff);
      }
      if(cridx == 1)
      {
      	  SCFG->exintc2 = (SCFG->exintc2 & crmask) | (portidx << croff);
      }
      if(cridx == 3)
      {
      	  SCFG->exintc3 = (SCFG->exintc3 & crmask) | (portidx << croff);
      }
      if(cridx == 4)
      {
      	  SCFG->exintc4 = (SCFG->exintc4 & crmask) | (portidx << croff);
      }
#endif

  /* Programming edge registers.*/
  if (mode & PAL_EVENT_MODE_RISING_EDGE)
    EXTI->polcfg1 |= padmask;
  else
    EXTI->polcfg1 &= ~padmask;
  if (mode & PAL_EVENT_MODE_FALLING_EDGE)
    EXTI->polcfg2 |= padmask;
  else
    EXTI->polcfg2 &= ~padmask;

  /* Programming interrupt and event registers.*/
#if defined(STM32_EXTI_ENHANCED)
  EXTI_D1->inten |= padmask;
  EXTI_D1->evten &= ~padmask;
#else
  EXTI->inten |= padmask;
  EXTI->evten &= ~padmask;
#endif
}

/**
 * @brief   Pad event disable.
 * @details This function disables previously programmed event callbacks.
 *
 * @param[in] port      port identifier
 * @param[in] pad       pad number within the port
 *
 * @notapi
 */
void _pal_lld_disablepadevent(ioportid_t port, iopadid_t pad) {
  uint32_t padmask, rtsr1, ftsr1;

  rtsr1 = EXTI->polcfg1;
  ftsr1 = EXTI->polcfg2;

  /* Mask of the pad.*/
  padmask = 1U << (uint32_t)pad;

  /* If either RTRS1 or FTSR1 is enabled then the channel is in use.*/
  if (((rtsr1 | ftsr1) & padmask) != 0U) {
    uint32_t cridx, croff, crport, portidx;

    /* Port index is obtained assuming that GPIO ports are placed at regular
       0x400 intervals in memory space. So far this is true for all devices.*/
    portidx = (((uint32_t)port - (uint32_t)GPIOA) >> 10U) & 0xFU;

    /* Index and mask of the CR register to be used.*/
    cridx  = (uint32_t)pad >> 2U;
#if STM32_EXTI_HAS_CR == FALSE
    croff  = ((uint32_t)pad & 3U) * 4U;
        if(cridx == 0)
        {
        	crport = ((SCFG->exintc1) >> croff) & 0xFU;
        }
        if(cridx == 1)
        {
            crport = ((SCFG->exintc2) >> croff) & 0xFU;
        }
        if(cridx == 2)
        {
            crport = ((SCFG->exintc3) >> croff) & 0xFU;
        }
        if(cridx == 3)
        {
            crport = ((SCFG->exintc4) >> croff) & 0xFU;
        }
#else
        croff  = ((uint32_t)pad & 3U) * 4U;
        if(cridx == 0)
        {
          crport = ((SCFG->exintc1) >> croff) & 0xFU;
        }
        if(cridx == 1)
        {
          crport = ((SCFG->exintc2) >> croff) & 0xFU;
        }
        if(cridx == 2)
        {
          crport = ((SCFG->exintc3) >> croff) & 0xFU;
        }
        if(cridx == 3)
        {
            crport = ((SCFG->exintc4) >> croff) & 0xFU;
        }
#endif

    osalDbgAssert(crport == portidx, "channel mapped on different port");

#if defined(STM32_EXTI_ENHANCED)
    /* Disabling channel.*/
    EXTI_D1->inten  &= ~padmask;
    EXTI_D1->evten  &= ~padmask;
    EXTI->polcfg1     = rtsr1 & ~padmask;
    EXTI->polcfg2     = ftsr1 & ~padmask;
    EXTI_D1->PR1    = padmask;
#else
    /* Disabling channel.*/
    EXTI->inten  &= ~padmask;
    EXTI->evten  &= ~padmask;
    EXTI->polcfg1  = rtsr1 & ~padmask;
    EXTI->polcfg2  = ftsr1 & ~padmask;
#if STM32_EXTI_SEPARATE_RF == FALSE
    EXTI->intsts    = padmask;
#else
    EXTI->intsts   = padmask;
    EXTI->intsts   = padmask;
#endif
#endif

#if PAL_USE_CALLBACKS || PAL_USE_WAIT
  /* Callback cleared and/or thread reset.*/
  _pal_clear_event(pad);
#endif
  }
}
#endif /* PAL_USE_CALLBACKS || PAL_USE_WAIT */

#endif /* HAL_USE_PAL */

/** @} */
