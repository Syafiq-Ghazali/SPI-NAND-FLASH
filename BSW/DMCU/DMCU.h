/*
|===============================================================================
|
| File:         DMCU.h
|
| Project:      Agnostic
|
| Processor:    TI TMS320F28003x
|
| Compiler:     TI C2000 compiler 22.6.2
|
| Description:  MCU Software Component Defines Constants and Function prototypes
|
| Copyright:    Copyright (C) 2024-2025 Daanaa Resolution Inc.
|
|               All Rights Reserved. Reproduction or disclosure of this file
|               or its Contents without the prior written consent of Daanaa
|               Resolution Inc is prohibited.
|=============================================================================*/

#ifndef _DMCU_H_
#define _DMCU_H_    // make sure header is not included again

/*=== INCLUDE FILES ==========================================================*/

#include "driverlib.h"
#include "DATATYPE.h"

/*=== #DEFINES ===============================================================*/

/* 
 * Temporary location for the define for the frequency of the internal oscillator
 * 
 * This parameter __needs__ to be defined, or else you will get a preprocessor
 * error (see the check below).
 */
#define MCU_OSCSRC_FREQ                            (20000000U)

#ifndef MCU_OSCSRC_FREQ
#error "[DMCU] MCU_OSCSRC_FREQ is not defined. This must be defined to the frequency of the internal oscillator, in Hz."
#endif // MCU_OSCSRC_FREQ

#define MCU_UNKNOWN_ERROR_DELAY_TIME               (50000u)
#define MCU_FATAL_ERROR_DELAY_TIME                 (500000u)

#define MCU_SETCLOCK_CFG                           (SYSCTL_OSCSRC_XTAL | SYSCTL_IMULT(48) | \
                                                    SYSCTL_REFDIV(2) | SYSCTL_ODIV(4) |     \
                                                    SYSCTL_SYSDIV(1) | SYSCTL_PLL_ENABLE |  \
                                                    SYSCTL_DCC_BASE_0)
                          
#define MCU_SYSCLK_FREQ                           ((Uint32_t)((MCU_OSCSRC_FREQ * 48) / (2 * 4 * 1)))

#define MCU_LSPCLK_FREQ                           ((Uint32_t)MCU_SYSCLK_FREQ / 2)

#define DMCU_DELAY_US(x)                            SysCtl_delay(((((Float64_t)(x)) / (1000000.0L /  \
                                                   (Float64_t)MCU_SYSCLK_FREQ)) - 9.0L) / 5.0L)
/*  Defines for setting FSI clock speeds */
#define FSI_PRESCALE_60MHZ                          1U
#define FSI_PRESCALE_30MHZ                          2U
#define FSI_PRESCALE_12MHZ                          5U
#define FSI_PRESCALE_6MHZ                           10U

/* 
 * Interrupt disable/enable/protect/restore macros
 *
 * Adjust these with __caution__.
 */
#define DISABLE_GLOBAL_INTERRUPT()                  {DINT;}
#define ENABLE_GLOBAL_INTERRUPT()                   {EINT;}
#define DISABLE_GLOBAL_REALTIME_INTERRUPT_DBGM()    {DRTM;}
#define ENABLE_GLOBAL_REALTIME_INTERRUPT_DBGM()     {ERTM;}
#define DISABLE_GLOBAL_INTERRUPT_FLAG()
#define ENABLE_GLOBAL_INTERRUPT_FLAG()
#define INTERRUPT_PROTECT()
#define INTERRUPT_RESTORE()

#ifndef C2000_WAIT_STEP
#define C2000_WAIT_STEP                             (asm(" RPT #255 || NOP"))
#endif

/*=== TYPE DEFINITIONS =======================================================*/

/*=== STRUCTURES =============================================================*/

/* MCU Configuration struct */
typedef struct McuConfig_t
{
    SysCtl_LSPCLKPrescaler              lowSpeedClkPrescaler;   /* Low Speed Clock Prescaler */
    const SysCtl_PeripheralPCLOCKCR    *clkPeripherals;         /* Clock Peripherals */
    Uint16_t                            numClkPeripherals;      /* Number of Clock Peripherals */
    
    /* Flash Peripherals */
    Uint32_t                            flashctrlBase;          /* Flash Control Base */
    Uint32_t                            flashEccBase;           /* Flash ECC Base */
    Uint16_t                            flashWaitStates;        /* Flash Wait States */
} McuConfig_t;

/*=== ENUMERATIONS ===========================================================*/

typedef enum
{
    MCU_ERROR_UNKNOWN = 0u
} McuErrorCode_t;

/*=== EXTERNAL FUNCTION PROTOTYPES ===========================================*/

extern void     DMCU_init                         (const McuConfig_t *config);
extern void     DMCU_gpioUnlockPortConfigs        (void);
extern void     DMCU_fatalError                   (McuErrorCode_t errorCode);
extern Bool_t   DMCU_verifyXTAL                   (Float32_t freq);

/*=== EXTERNAL VARIABLE DEFINITIONS ==========================================*/

#ifdef _FLASH
#include <stddef.h>

#ifndef CMDTOOL
extern uint16_t RamfuncsLoadStart;
extern uint16_t RamfuncsLoadEnd;
extern uint16_t RamfuncsLoadSize;
extern uint16_t RamfuncsRunStart;
extern uint16_t RamfuncsRunEnd;
extern uint16_t RamfuncsRunSize;
#endif // !CMDTOOL

#endif // _FLASH

#endif // _DMCU_H_
