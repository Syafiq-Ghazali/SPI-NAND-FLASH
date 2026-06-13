/*
|===============================================================================
|
| File:         DGPIO.h
|
| Project:      Agnostic
|
| Processor:    TI TMS320F28003x
|
| Compiler:     TI C2000 compiler 22.6.2
|
| Description:  GPIO Software Component Defines Constants and Function prototypes
|
| Copyright:    Copyright (C) 2024-2025 Daanaa Resolution Inc.
|
|               All Rights Reserved. Reproduction or disclosure of this file
|               or its Contents without the prior written consent of Daanaa
|               Resolution Inc is prohibited.
|=============================================================================*/

#ifndef _DGPIO_H_
#define _DGPIO_H_    // make sure header is not included again

/*=== INCLUDE FILES ==========================================================*/

#include "driverlib.h"
#include "DATATYPE.h"

/*=== #DEFINES ===============================================================*/

/*=== TYPE DEFINITIONS =======================================================*/

/*=== ENUMERATIONS ===========================================================*/

/*
 * GPIO direction (input, output, or neither)
 * 
 * This enum was made as the C2000Ware enum did not have
 * an undefined entry.
 */
typedef enum
{
    GPIO_DIR_IN           = (Uint16_t)GPIO_DIR_MODE_IN,     // 0u
    GPIO_DIR_OUT          = (Uint16_t)GPIO_DIR_MODE_OUT,    // 1u
    GPIO_DIR_UNDEFINED    = (Uint16_t)(GPIO_DIR_MODE_OUT + 1u)
} GpioDirection_t;

typedef enum
{
    GPIO_EDGE_FALLING     = (Uint16_t)GPIO_INT_TYPE_FALLING_EDGE,
    GPIO_EDGE_RISING      = (Uint16_t)GPIO_INT_TYPE_RISING_EDGE,
    GPIO_EDGE_BOTH        = (Uint16_t)GPIO_INT_TYPE_BOTH_EDGES
} GpioEdge_t;

/* GPIO output state (on or off) */
typedef enum
{
    GPIO_STATE_OFF        = 0u,
    GPIO_STATE_ON         = 1u,
    GPIO_STATE_DONT_CARE  = 2u
} GpioState_t;

/*=== STRUCTURES =============================================================*/

typedef struct GpioInterruptConfig_t
{
    Uint32_t               interruptNumber;                /* Interrupt number */
    void          (* const interruptHandler)(void);        /* Pointer to function called when interrupt fires */
    GPIO_ExternalIntNum    externalIntNum;                 /* External Interrupt Number */
    GpioEdge_t             edgeType;                       /* Falling Edge, Rising Edge, Both */
    Uint32_t               ackGroup;                       /* TI Interrupt acknowledgement group */       
    void                 (*interruptCallback)(void *ctx);  /* Callback function */
    void                  *ctx;                            /* Interrupt context */
} GpioInterruptConfig_t;

/* GPIO initialization configuration */
typedef struct GpioConfig_t
{
    Uint32_t               pinNumber;                  /* Pin number */
    Uint32_t               pinConfig;                  /* Pin mode */
    GpioDirection_t        direction;                  /* GPIO direction */
    GPIO_QualificationMode qualification;              /* GPIO qualification */
    Uint32_t               pinType;                    /* GPIO pin type */
    GpioState_t            initState;                  /* Initial output state */
    Bool_t                 claControlled;              /* CLA controlled GPIO */
    Bool_t                 isAnalog;                   /* ADC or DAC pin */ 
} GpioConfig_t;

/*=== EXTERNAL FUNCTION PROTOTYPES ===========================================*/

extern void     DGPIO_init               (const GpioConfig_t *config);
extern void     DGPIO_interruptAttach    (GpioInterruptConfig_t *interruptConfig, Uint32_t pinNumber, 
                                          GpioEdge_t edgeType, void (*interruptCallback)(void *ctx),
                                          void *ctx);

/*=== EXTERNAL VARIABLE DEFINITIONS ==========================================*/

/*=== INLINE FUNCTIONS =======================================================*/

/*
|===============================================================================
|
| Function:         DGPIO_pinWrite
|
| Description:      Sets or clears a GPIO pin, assuming the pin has been initialized
|                   to be a digital output.
|                   
| Dependencies:     The pin must be configured to be a digital output.
|
| Notes:            
|
| Side Effects:
|
| Return Value:
|
|===============================================================================
| Variable     Access  Description  (I=input O=output I/O=in/out)
|-------------------------------------------------------------------------------
| pinNumber    I       GPIO pin number
| pinState     I       GPIO pin state (GPIO_STATE_OFF or GPIO_STATE_ON)
|=============================================================================*/

static inline void DGPIO_pinWrite
(
    Uint32_t pinNumber,
    GpioState_t pinState
)
{
    volatile uint32_t *gpioDataReg;
    uint32_t pinMask;

    gpioDataReg = (uint32_t *)((uintptr_t)GPIODATA_BASE) +
                  ((pinNumber / 32U) * GPIO_DATA_REGS_STEP);

    pinMask = (uint32_t)1U << (pinNumber % 32U);

    if(pinState == 0U)
    {
        gpioDataReg[GPIO_GPxCLEAR_INDEX] = pinMask;
    }
    else
    {
        gpioDataReg[GPIO_GPxSET_INDEX] = pinMask;
    }
}

/*
|===============================================================================
|
| Function:         DGPIO_pinRead
|
| Description:      Reads the state of a GPIO pin, assuming the pin has been 
|                   initialized to be a digital input.
|                   
| Dependencies:     The pin must be configured to be a digital input.
|
| Notes:            
|
| Side Effects:
|
| Return Value: returns 1 if driven high 0 if driven low
|
|===============================================================================
| Variable     Access  Description  (I=input O=output I/O=in/out)
|-------------------------------------------------------------------------------
| pinNumber    I       GPIO pin number
|=============================================================================*/

static inline Uint32_t DGPIO_pinRead
(
    Uint32_t pinNumber
)
{
    volatile uint32_t *gpioDataReg;

    gpioDataReg = (uint32_t *)((uintptr_t)GPIODATA_BASE) +
                  ((pinNumber / 32U) * GPIO_DATA_REGS_STEP);

    return((gpioDataReg[GPIO_GPxDAT_INDEX] >> (pinNumber % 32U)) & (uint32_t)0x1U);
}

/*
|===============================================================================
|
| Function:         DGPIO_portRead
|
| Description:      Reads the state of a GPIO pin, assuming the pin has been 
|                   initialized to be a digital input.
|                   
| Dependencies:     The pin must be configured to be a digital input.
|
| Notes:            
|
| Side Effects:
|
| Return Value: returns 1 if driven high 0 if driven low
|
|===============================================================================
| Variable     Access  Description  (I=input O=output I/O=in/out)
|-------------------------------------------------------------------------------
| pinNumber    I       GPIO pin number
|=============================================================================*/

static inline Uint32_t DGPIO_portRead
(
    GPIO_Port port
)
{
    volatile uint32_t *gpioDataReg;

    /* Get the starting address of the port's registers */
    gpioDataReg = (uint32_t *)((uintptr_t)GPIODATA_BASE) +
                  ((uint32_t)port * GPIO_DATA_REGS_STEP);

    return(gpioDataReg[GPIO_GPxDAT_INDEX]);
}

/*
|===============================================================================
|
| Function:         DGPIO_portWrite
|
| Description:      Reads the state of a GPIO pin, assuming the pin has been 
|                   initialized to be a digital input.
|                   
| Dependencies:     The pin must be configured to be a digital input.
|
| Notes:            
|
| Side Effects:
|
| Return Value: returns 1 if driven high 0 if driven low
|
|===============================================================================
| Variable     Access  Description  (I=input O=output I/O=in/out)
|-------------------------------------------------------------------------------
| pinNumber    I       GPIO pin number
|=============================================================================*/

static inline Bool_t DGPIO_portWrite
(
    GPIO_Port port,
    Uint32_t clrMask,
    Uint32_t setMask
)
{
    volatile uint32_t *gpioDataReg;

    /* Get the starting address of the port's registers */
    gpioDataReg = (uint32_t *)((uintptr_t)GPIODATA_BASE) +
                  ((uint32_t)port * GPIO_DATA_REGS_STEP);
    
    /* Check if there is a contradiction in the clr and set masks */
    if ((clrMask & setMask) != 0u)
    {
        return FALSE;
    }
    gpioDataReg[GPIO_GPxCLEAR_INDEX] = clrMask;
    gpioDataReg[GPIO_GPxSET_INDEX] = setMask;
    return TRUE; 
}


/*
|===============================================================================
|
| Function:         DGPIO_pinToggle
|
| Description:      Toggles a GPIO pin, assuming the pin has been initialized
|                   to be a digital output.
|                   
| Dependencies:     The pin must be configured to be a digital output.
|
| Notes:            
|
| Side Effects:
|
| Return Value:
|
|===============================================================================
| Variable     Access  Description  (I=input O=output I/O=in/out)
|-------------------------------------------------------------------------------
| pinNumber    I       GPIO pin number
|=============================================================================*/

static inline void DGPIO_pinToggle
(
    Uint32_t pinNumber
)
{
    volatile uint32_t *gpioDataReg;

    gpioDataReg = (uint32_t *)((uintptr_t)GPIODATA_BASE) +
                  ((pinNumber / 32U) * GPIO_DATA_REGS_STEP);

    gpioDataReg[GPIO_GPxTOGGLE_INDEX] = (uint32_t)1U << (pinNumber % 32U);
}

#endif // _DGPIO_H_
