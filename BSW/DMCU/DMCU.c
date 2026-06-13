/*
|===============================================================================
|
| File:         DMCU.c
|
| Project:      Agnostic
|
| Processor:    TI TMS320F28003x
|
| Compiler:     TI C2000 compiler 22.6.2
|
| Description:  The MCU API functions are defined here.
|
| Copyright:    Copyright (C) 2024-2025 Daanaa Resolution Inc.
|
|               All Rights Reserved. Reproduction or disclosure of this file
|               or its Contents without the prior written consent of Daanaa
|               Resolution Inc is prohibited.
|=============================================================================*/

/*=== INCLUDE FILES ==========================================================*/

#include "DMCU.h"
#include <string.h>

/*=== #DEFINES ===============================================================*/

/*=== TYPE DEFINITIONS =======================================================*/

/*=== ENUMERATIONS ===========================================================*/

/*=== EXTERNALS ==============================================================*/

/*=== PRIVATE FUNCTION PROTOTYPES ============================================*/

/*=== GLOBAL DATA ============================================================*/

/*=== PRIVATE DATA ===========================================================*/

/*
|===============================================================================
|
| Function:         DMCU_init
|
| Description:      Performs the initial hardware initialization of the MCU
|                   chip
|
| Dependencies:
|
| Notes:            Assumed that interrupts are globally disabled
|
| Side Effects:
|
| Return Value:
|
|===============================================================================
| Variable     Access  Description  (I=input O=output I/O=in/out)
|-------------------------------------------------------------------------------
|=============================================================================*/

void DMCU_init
(
    const McuConfig_t *config
)
{
    Uint16_t i;
    /* Disable CPU interrupts and clear all CPU interrupt flags */
    IER = 0x0000;
    IFR = 0x0000;

    /* Initializes device clock and peripherals */

    SysCtl_disableWatchdog();

#ifdef CMDTOOL
    CMD_init();
#endif // CMDTOOL

#ifdef _FLASH
#ifndef CMDTOOL
    /*
     * Copy time critical code and flash setup code to RAM. This includes the
     * following functions: InitFlash();
     *
     * The RamfuncsLoadStart, RamfuncsLoadSize, and RamfuncsRunStart symbols
     * are created by the linker. Refer to the device .cmd file.
     */
    memcpy(&RamfuncsRunStart, &RamfuncsLoadStart, (size_t)&RamfuncsLoadSize);
#endif // !CMDTOOL
    /*
     * Call Flash Initialization to setup flash waitstates. This function must
     * reside in RAM.
     */
    Flash_initModule(config->flashctrlBase, config->flashEccBase, config->flashWaitStates);
#endif // _FLASH

    /* Set up PLL control and clock dividers */
    SysCtl_setClock(MCU_SETCLOCK_CFG);

    SysCtl_setLowSpeedClock(config->lowSpeedClkPrescaler);

    /*
     * These asserts will check that the #defines for the clock rates in
     * DMCU.h match the actual rates that have been configured. If they do
     * not match, check that the calculations of MCU_SYSCLK_FREQ and
     * MCU_LSPCLK_FREQ are accurate. Some examples will not perform as
     * expected if these are not correct.
     */
    ASSERT(SysCtl_getClock(MCU_OSCSRC_FREQ) == MCU_SYSCLK_FREQ);
    ASSERT(SysCtl_getLowSpeedClock(MCU_OSCSRC_FREQ) == MCU_LSPCLK_FREQ);

#ifndef _FLASH
    /*
     * Call Device_cal function when run using debugger
     * This function is called as part of the Boot code. The function is called
     * in the Device_init function since during debug time resets, the boot code
     * will not be executed and the gel script will reinitialize all the
     * registers and the calibrated values will be lost.
	 * Sysctl_deviceCal is a wrapper function for Device_Cal
     */
    SysCtl_deviceCal();
#endif // !_FLASH

    /* Turn on all peripherals */
    for (i=0; i < config->numClkPeripherals; i++)
    {
        SysCtl_enablePeripheral(config->clkPeripherals[i]);
    }
  
    /*
     * Lock VREGCTL Register
     * The register VREGCTL is not supported in this device. It is locked to
     * prevent any writes to this register
     */
    ASysCtl_lockVREG();

    /* initialization of the interrupts */
    Interrupt_initModule();
    Interrupt_initVectorTable();
}

/*
|===============================================================================
|
| Function:     DMCU_gpioUnlockPortConfigs
|
| Description:  
|               
|
| Dependencies: None
|
| Notes:        
|               
|
| Side Effects: None
|
| Return Value: 
|
|===============================================================================
| Variable     Access  Description  (I=input O=output I/O=in/out)
|-------------------------------------------------------------------------------
|=============================================================================*/

void DMCU_gpioUnlockPortConfigs
(
    void
)
{
    /* Disable pin locks. */
    GPIO_unlockPortConfig(GPIO_PORT_A, 0xFFFFFFFF);
    GPIO_unlockPortConfig(GPIO_PORT_B, 0xFFFFFFFF);
    GPIO_unlockPortConfig(GPIO_PORT_H, 0xFFFFFFFF);
}

/*
|===============================================================================
|
| Function:         DMCU_verifyXTAL
|
| Description:      Verify the XTAL frequency using DCC to compare against 
|                   INTOSC2 as reference clock
|
| Dependencies:
|
| Notes:            This function assumes that the PLL is not already configured
|                   and hence uses SysClk freq = 10MHz for DCC calculation
|
| Side Effects:
|
| Return Value:     Returns TRUE if the actual XTAL frequency matches the input
|                   value, FALSE otherwise
|
|===============================================================================
| Variable     Access  Description  (I=input O=output I/O=in/out)
|-------------------------------------------------------------------------------
| freq           I     XTAL frequency in MHz
|=============================================================================*/
Bool_t DMCU_verifyXTAL
(
    Float32_t freq
)
{
    /* 
     * Use DCC to verify the XTAL frequency using INTOSC2 as reference clock 
     * Turn on XTAL and wait for it to power up using X1CNT 
     */
    SysCtl_turnOnOsc(SYSCTL_OSCSRC_XTAL);
    SysCtl_clearExternalOscCounterValue();
    while(SysCtl_getExternalOscCounterValue() != SYSCTL_X1CNT_X1CNT_M);


    /* Enable DCC0 clock */
    SysCtl_enablePeripheral(SYSCTL_PERIPH_CLK_DCC0);


    /* Insert atleast 5 cycles delay after enabling the peripheral clock */
    asm(" RPT #5 || NOP");

    /*
     * Configures XTAL as CLKSRC0 and INTOSC2 as CLKSRC1
     * Fclk0 = XTAL frequency (input parameter)
     * Fclk1 = INTOSC2 frequency = 10MHz
     *
     * Configuring DCC error tolerance of +/-1%
     * INTOSC2 can have a variance in frequency of +/-10%
     *
     * Assuming PLL is not already configured, SysClk freq = 10MHz
     *
     * Note : Update the tolerance and INTOSC2 frequency variance as necessary.
     */

    return (DCC_verifyClockFrequency(DCC0_BASE,
                                     DCC_COUNT1SRC_INTOSC2, 10.0F,
                                     DCC_COUNT0SRC_XTAL, freq,
                                     1.0F, 10.0F, 10.0F));

}

/*
|===============================================================================
|
| Function:          __error__
|
| Description:       Error handling function to be called when an ASSERT is 
|                    violated. Halts execution with ESTOP0.
|
| Dependencies:      None
|
| Notes:             An ASSERT condition was evaluated as false. You can use 
|                    the filename and line parameters to determine what went wrong.
|
|                    __error__ is reference by driverlib/debug.h
|
| Side Effects:      Halts CPU execution
|
| Return Value:      This function does not return
|===============================================================================
| Variable     Access  Description  (I=input O=output I/O=in/out)
|-------------------------------------------------------------------------------
|=============================================================================*/

void __error__
(
    const char *filename, Uint32_t line
)
{
    /*
     * An ASSERT condition was evaluated as false. You can use the filename and
     * line parameters to determine what went wrong.
     */
    ESTOP0;
}

/*
|===============================================================================
|
| Function:     DMCU_fatalError
|
| Description:  This function disables all interrupts to prevent any other code from
|               running and flashes the debug LEDs
|
| Dependencies: None
|
| Notes:        Error Code Values:
|               0 = Undefined, simple fast blink
|
| Side Effects: None
|
| Return Value: This function does not return
|
|===============================================================================
| Variable     Access  Description  (I=input O=output I/O=in/out)
|-------------------------------------------------------------------------------
| errorCode      I     Error Code, see above for definitions.
|===============================================================================
*/

void DMCU_fatalError
(
    McuErrorCode_t errorCode
)
{
    (void)errorCode;
    DISABLE_GLOBAL_INTERRUPT();
    ESTOP0;

    for (;;)
    {
        /* Fatal errors require a debugger reset. */
    }
}

/*=== End of File ============================================================*/
