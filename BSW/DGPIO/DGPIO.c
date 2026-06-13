/*
|===============================================================================
|
| File:         DGPIO.c
|
| Project:      Agnostic
|
| Processor:    TI TMS320F28003x
|
| Compiler:     TI C2000 compiler 22.6.2
|
| Description:  The GPIO API functions are defined here
|
| Copyright:    Copyright (C) 2024-2025 Daanaa Resolution Inc.
|
|               All Rights Reserved. Reproduction or disclosure of this file
|               or its Contents without the prior written consent of Daanaa
|               Resolution Inc is prohibited.
|=============================================================================*/

/*=== INCLUDE FILES ==========================================================*/

#include "DGPIO.h"
#include "DMCU.h"

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
| Function:         DGPIO_init
|
| Description:      Initializes a GPIO pin, given a gpio configuration.
|
|                   This function is used for configuring any type of GPIO pin,
|                   regardless of the function (digital I/O, PWM, ADC, DAC, etc.).
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
| config       I       GPIO pin config
|=============================================================================*/

void DGPIO_init
(
    const GpioConfig_t *config
)
{
    DMCU_gpioUnlockPortConfigs();

    EALLOW;
    
    /* Set the pin config */
    GPIO_setPinConfig(config->pinConfig);
    
    /* 
     * Set the analog pin configuration. If the pin is an analog pin, we can return 
     * immediately after this.
     *
     * On 100PZ package for F28003x, if both "B5, GPIO20" and "B5" are selected as analog,
     * "B5, GPIO20" will drive the ADC instead of "B5"
     * On 100PZ package for F28003x, if both "B11, GPIO21" and "B11" are selected as analog,
     * "B11, GPIO21" will drive the ADC instead of "B11"
     * 
     * On 100PZ package for F28003x, if both "B5, GPIO20" and "B5" are selected as analog,
     * "B5, GPIO20" will drive the ADC instead of "B5"
     * On 100PZ package for F28003x, if both "B11, GPIO21" and "B11" are selected as analog,
     * "B11, GPIO21" will drive the ADC instead of "B11"
     */
    if (config->isAnalog)
    {
        GPIO_setAnalogMode(config->pinNumber, GPIO_ANALOG_ENABLED);
        EDIS;
        return;
    }
    
    /*
     * For AIO and AGPIO pins that are used as digital pins, we need to 
     * explicitly disable its analog mode. The following condition is used
     * as GPIO_setAnalogMode() is not to be called for non-AIO and non-AGPIO
     * pins.
     */
    if (((config->pinNumber >= 224u) && 
         (config->pinNumber <= 253u) && 
         (config->pinNumber != 234u) && 
         (config->pinNumber != 235u) && 
         (config->pinNumber != 243u) && 
         (config->pinNumber != 246u) && 
         (config->pinNumber != 250u)) ||
         (config->pinNumber == 20u) || 
         (config->pinNumber == 21u))
    {
        GPIO_setAnalogMode(config->pinNumber, GPIO_ANALOG_DISABLED);
    }

    /* Set the pad config and qualification mode */
    GPIO_setPadConfig(config->pinNumber, config->pinType);
    GPIO_setQualificationMode(config->pinNumber, config->qualification);

    /*
     * Set the output latch before changing the direction. This prevents an
     * unintended pulse when an active-low control pin becomes an output.
     */
    if (config->initState != GPIO_STATE_DONT_CARE)
    {
        GPIO_writePin(config->pinNumber, config->initState);
    }

    /* Set direction if required */
    if (config->direction != GPIO_DIR_UNDEFINED)
    {
        GPIO_setDirectionMode(config->pinNumber, (GPIO_Direction)config->direction);
    }

    /* Make CLA-controlled if required */
    if (config->claControlled)
    {
        GPIO_setMasterCore(config->pinNumber, GPIO_CORE_CPU1_CLA1);
    }

    EDIS;
}

/*
|===============================================================================
|
| Function:         DGPIO_interruptAttach
|
| Description:      Initializes a interrupt for a GPIO pin
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
| Variable         Access     Description  (I=input O=output I/O=in/out)
|-------------------------------------------------------------------------------
| config              I       interrupt Config
| pinNumber           I       GPIO pin number
| edgeType            I       Falling edge, Rising edge, or both
| interruptCallback   I       call back function placed inside the interrupt
|=============================================================================*/

void DGPIO_interruptAttach
(
    GpioInterruptConfig_t *interruptConfig,
    Uint32_t pinNumber,
    GpioEdge_t edgeType,
    void (*interruptCallback)(void *ctx),
    void *ctx
)
{
    if (interruptConfig != NULL)
    {
        GPIO_setInterruptType(interruptConfig->externalIntNum, (GPIO_IntType)edgeType);
        GPIO_setInterruptPin(pinNumber, interruptConfig->externalIntNum);
        GPIO_enableInterrupt(interruptConfig->externalIntNum);
        
        Interrupt_register(interruptConfig->interruptNumber, interruptConfig->interruptHandler);
        Interrupt_enable(interruptConfig->interruptNumber);

        interruptConfig->edgeType = edgeType;
        interruptConfig->interruptCallback = interruptCallback;
        interruptConfig->ctx = ctx;
    }
}

/*=== End of File ============================================================*/
