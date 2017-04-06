/**
 * @file
 * vector table, startup code and SystemInit
 * for STM32F401Xe
 *
 * (c) 2016 Bernd Kreuss <prof7bit@gmail.com>
 * This file is public domain, you may use it, modify
 * it or reproduce it for whatever purpose you want.
 */

#include "stm32f401xe.h"


/*
 * These external symbols are defined in the linker script,
 * we need them in the reset handler and in the vector table.
 */
extern long __bss_start__[];
extern long __bss_end__[];
extern long __etext[];
extern long __data_start__[];
extern long __data_end__[];
extern long __stack[];


/*
 * Define some attributes to attach to force functions
 * or other objects into certain specific locations.
 * These section names are defined in the linker script.
 */
#define SECT_VECTABLE           __attribute__((section(".isr_vector"), __used__))
#define WEAK                    __attribute__((weak))
#define NAKED                   __attribute__((naked))


/**
 * The main() function will be defined in another file (hopefully)
 * but we need to reference it later from within the reset handler.
 */
extern int main(void);


/**
 * Mandated by the CMSIS-Headers this global variable should always
 * exist and contain the current core clock frequency. When changing
 * the clock then this variable should also be updated.
 */
uint32_t SystemCoreClock;


/**
 * The existence of the SystemInit function is mandated by the
 * CMSIS headers, its purpose is to set up the hardware of the
 * controller and the correct clock setup before entry to main(),
 * we implement it here.
 *
 * This function is defined with a weak symbol to allow the
 * application to override it and provide its own implementation.
 */
WEAK void SystemInit(void) {
    /* Enable Power Control clock */
    RCC->APB1ENR |= RCC_APB1LPENR_PWRLPEN;

    /* Regulator voltage scaling output selection: Scale 2 */
    PWR->CR |= PWR_CR_VOS_1;

    /* Wait until HSI ready */
    while ((RCC->CR & RCC_CR_HSIRDY) == 0);

    /* Store calibration value */
    PWR->CR |= (uint32_t)(16 << 3);

    /* Disable main PLL */
    RCC->CR &= ~(RCC_CR_PLLON);

    /* Wait until PLL ready (disabled) */
    while ((RCC->CR & RCC_CR_PLLRDY) != 0);

    /*
     * Configure Main PLL
     * HSI as clock input
     * fvco = 336MHz
     * fpllout = 84MHz
     * fusb = 48MHz
     * PLLM = 16
     * PLLN = 336
     * PLLP = 4
     * PLLQ = 7
     *
     * Please see the reference manual for the meaning of the
     * bits and the mapping between the above numbers and the
     * actual values written to the PLLCFGR register.
     */
    RCC->PLLCFGR = 0x2000000UL  // reset value of reserved bits
                 | (7UL << 24)  // PLLQ
                 | (1UL << 16)  // PLLP
                 | (336UL << 6) // PLLN
                 | (16UL << 0); // PLLM

    /* PLL On */
    RCC->CR |= RCC_CR_PLLON;

    /* Wait until PLL is locked */
    while ((RCC->CR & RCC_CR_PLLRDY) == 0);

    /*
     * FLASH configuration block
     * enable instruction cache
     * enable prefetch
     * set latency to 2WS (3 CPU cycles)
     */
    FLASH->ACR |= FLASH_ACR_ICEN
                | FLASH_ACR_PRFTEN
                | FLASH_ACR_LATENCY_2WS;

    /* Set clock source to PLL */
    RCC->CFGR |= RCC_CFGR_SW_PLL;
    /* Check clock source */
    while ((RCC->CFGR & RCC_CFGR_SWS_PLL) != RCC_CFGR_SWS_PLL);

    /* Set HCLK (AHB1) prescaler (DIV1) */
    RCC->CFGR &= ~(RCC_CFGR_HPRE);

    /* Set APB1 Low speed prescaler (APB1) DIV2 */
    RCC->CFGR |= RCC_CFGR_PPRE1_DIV2;

    /* SET APB2 High speed prescaler (APB2) DIV1 */
    RCC->CFGR &= ~(RCC_CFGR_PPRE2);

    /* allow debug even during sleep modes (otherwise debugger would be of little use) */
    DBGMCU->CR |= DBGMCU_CR_DBG_SLEEP
                | DBGMCU_CR_DBG_STANDBY
                | DBGMCU_CR_DBG_STOP;

    SystemCoreClock = 84000000UL;
}


/**
 * The existence of this function is mandated by the CMSIS headers.
 * Its purpose is to somehow determine the current clock frequency
 * from the register settings and then update the SystemCoreClock
 * variable. We will implement it as a no-op here because we are
 * not going to need this functionality.
 *
 * This function is defined with a weak symbol to allow the
 * application to override it and provide its own implementation.
 */
WEAK void SystemCoreClockUpdate(void) {
    // empty
}


/**
 * Initialize bss section with zero (C standard guarantees that global
 * and static variables will be initialized with zero) and initialize
 * the data section (initialized variables) with the data found in the
 * flash after the end of the text section.
 */
static void ram_init(void) {
    for (long* dest = __bss_start__; dest < __bss_end__; ++dest) {
        *dest = 0;
    }

    long* src = __etext;
    for (long* dest = __data_start__; dest < __data_end__; ++dest) {
        *dest = *src++;
    }
}


/**
 * This is the entry point right after reset. The address of this
 * function will be placed in the vector table as the reset handler
 * so code execution after reset will begin exactly here.
 *
 * It is implemented in asm because this is the only reliable way
 * to have a naked function without a stack frame and at the same
 * time make sure the optimizer will not try to inline anything
 * into it.
 *
 * The function will end in an infinite loop should the main()
 * function ever return (which would be an error because main()
 * must never return).
 */
NAKED void Reset_Handler(void) {
    asm("bl  ram_init"           :: "i" (ram_init));
    asm("bl  SystemInit"         :: "i" (SystemInit));
    asm("bl  main"               :: "i" (main));
    while(1);
}


/**
 * The default empty exception handler. Its address will be placed
 * in the vector table for all exceptions for which the linker cannot
 * find an implementation elsewhere in the code.
 *
 * This function is defined with a weak symbol to allow the
 * application to override it and provide its own implementation.
 */
WEAK void Default_Handler(void) {
    while(1);
}


/**
 * Declare weak symbols with the names of all handlers. This does not yet
 * produce any code, its just a bunch of declarations of aliases that all
 * point to the same empty default handler. The application can define
 * functions with these names in which case the linker will replace the
 * the weak aliases with these existing functions.
 */
#define WEAK_DEFAULT        __attribute__((weak, alias("Default_Handler")))

WEAK_DEFAULT void NMI_Handler(void);
WEAK_DEFAULT void HardFault_Handler(void);
WEAK_DEFAULT void MemManage_Handler(void);
WEAK_DEFAULT void BusFault_Handler(void);
WEAK_DEFAULT void UsageFault_Handler(void);
WEAK_DEFAULT void SVC_Handler(void);
WEAK_DEFAULT void DebugMon_Handler(void);
WEAK_DEFAULT void PendSV_Handler(void);
WEAK_DEFAULT void SysTick_Handler(void);
WEAK_DEFAULT void WWDG_IRQHandler(void);                    /* Window WatchDog              */
WEAK_DEFAULT void PVD_IRQHandler(void);                     /* PVD through EXTI Line detection */
WEAK_DEFAULT void TAMP_STAMP_IRQHandler(void);              /* Tamper and TimeStamps through the EXTI line */
WEAK_DEFAULT void RTC_WKUP_IRQHandler(void);                /* RTC Wakeup through the EXTI line */
WEAK_DEFAULT void FLASH_IRQHandler(void);                   /* FLASH                        */
WEAK_DEFAULT void RCC_IRQHandler(void);                     /* RCC                          */
WEAK_DEFAULT void EXTI0_IRQHandler(void);                   /* EXTI Line0                   */
WEAK_DEFAULT void EXTI1_IRQHandler(void);                   /* EXTI Line1                   */
WEAK_DEFAULT void EXTI2_IRQHandler(void);                   /* EXTI Line2                   */
WEAK_DEFAULT void EXTI3_IRQHandler(void);                   /* EXTI Line3                   */
WEAK_DEFAULT void EXTI4_IRQHandler(void);                   /* EXTI Line4                   */
WEAK_DEFAULT void DMA1_Stream0_IRQHandler(void);            /* DMA1 Stream 0                */
WEAK_DEFAULT void DMA1_Stream1_IRQHandler(void);            /* DMA1 Stream 1                */
WEAK_DEFAULT void DMA1_Stream2_IRQHandler(void);            /* DMA1 Stream 2                */
WEAK_DEFAULT void DMA1_Stream3_IRQHandler(void);            /* DMA1 Stream 3                */
WEAK_DEFAULT void DMA1_Stream4_IRQHandler(void);            /* DMA1 Stream 4                */
WEAK_DEFAULT void DMA1_Stream5_IRQHandler(void);            /* DMA1 Stream 5                */
WEAK_DEFAULT void DMA1_Stream6_IRQHandler(void);            /* DMA1 Stream 6                */
WEAK_DEFAULT void ADC_IRQHandler(void);                     /* ADC1(void); ADC2 and ADC3s   */
WEAK_DEFAULT void EXTI9_5_IRQHandler(void);                 /* External Line[9:5]s          */
WEAK_DEFAULT void TIM1_BRK_TIM9_IRQHandler(void);           /* TIM1 Break and TIM9          */
WEAK_DEFAULT void TIM1_UP_TIM10_IRQHandler(void);           /* TIM1 Update and TIM10        */
WEAK_DEFAULT void TIM1_TRG_COM_TIM11_IRQHandler(void);      /* TIM1 Trigger and Commutation and TIM11 */
WEAK_DEFAULT void TIM1_CC_IRQHandler(void);                 /* TIM1 Capture Compare         */
WEAK_DEFAULT void TIM2_IRQHandler(void);                    /* TIM2                         */
WEAK_DEFAULT void TIM3_IRQHandler(void);                    /* TIM3                         */
WEAK_DEFAULT void TIM4_IRQHandler(void);                    /* TIM4                         */
WEAK_DEFAULT void I2C1_EV_IRQHandler(void);                 /* I2C1 Event                   */
WEAK_DEFAULT void I2C1_ER_IRQHandler(void);                 /* I2C1 Error                   */
WEAK_DEFAULT void I2C2_EV_IRQHandler(void);                 /* I2C2 Event                   */
WEAK_DEFAULT void I2C2_ER_IRQHandler(void);                 /* I2C2 Error                   */
WEAK_DEFAULT void SPI1_IRQHandler(void);                    /* SPI1                         */
WEAK_DEFAULT void SPI2_IRQHandler(void);                    /* SPI2                         */
WEAK_DEFAULT void USART1_IRQHandler(void);                  /* USART1                       */
WEAK_DEFAULT void USART2_IRQHandler(void);                  /* USART2                       */
WEAK_DEFAULT void EXTI15_10_IRQHandler(void);               /* External Line[15:10]s        */
WEAK_DEFAULT void RTC_Alarm_IRQHandler(void);               /* RTC Alarm (A and B) through EXTI Line */
WEAK_DEFAULT void OTG_FS_WKUP_IRQHandler(void);             /* USB OTG FS Wakeup through EXTI line */
WEAK_DEFAULT void DMA1_Stream7_IRQHandler(void);            /* DMA1 Stream7                 */
WEAK_DEFAULT void SDIO_IRQHandler(void);                    /* SDIO                         */
WEAK_DEFAULT void TIM5_IRQHandler(void);                    /* TIM5                         */
WEAK_DEFAULT void SPI3_IRQHandler(void);                    /* SPI3                         */
WEAK_DEFAULT void DMA2_Stream0_IRQHandler(void);            /* DMA2 Stream 0                */
WEAK_DEFAULT void DMA2_Stream1_IRQHandler(void);            /* DMA2 Stream 1                */
WEAK_DEFAULT void DMA2_Stream2_IRQHandler(void);            /* DMA2 Stream 2                */
WEAK_DEFAULT void DMA2_Stream3_IRQHandler(void);            /* DMA2 Stream 3                */
WEAK_DEFAULT void DMA2_Stream4_IRQHandler(void);            /* DMA2 Stream 4                */
WEAK_DEFAULT void OTG_FS_IRQHandler(void);                  /* USB OTG FS                   */
WEAK_DEFAULT void DMA2_Stream5_IRQHandler(void);            /* DMA2 Stream 5                */
WEAK_DEFAULT void DMA2_Stream6_IRQHandler(void);            /* DMA2 Stream 6                */
WEAK_DEFAULT void DMA2_Stream7_IRQHandler(void);            /* DMA2 Stream 7                */
WEAK_DEFAULT void USART6_IRQHandler(void);                  /* USART6                       */
WEAK_DEFAULT void I2C3_EV_IRQHandler(void);                 /* I2C3 event                   */
WEAK_DEFAULT void I2C3_ER_IRQHandler(void);                 /* I2C3 error                   */
WEAK_DEFAULT void FPU_IRQHandler(void);                     /* FPU                          */
WEAK_DEFAULT void SPI4_IRQHandler(void);                    /* SPI4                         */


/**
 * This is the structure that every ARM Cortex expects to find at
 * the very beginning of the flash memory after reset. The first
 * long word is the initial stack pointer, followed by an array
 * of function pointers that hold the addresses of the exception
 * handlers.
 */
typedef struct {
    long* initial_stack;
    void(*vectors[])(void);
} vector_table_t;


/**
 * Now we let the linker emit an instance of this structure filled
 * with the correct addresses at the very beginning of the flash
 * memory by referring to the ".vector_table" section which is defined
 * in the linker script to be at that exact memory address.
 */
SECT_VECTABLE const vector_table_t __vector_table = {
    .initial_stack = __stack,
    .vectors = {
        /*
         * Exception handlers that belong to the ARM core itself
         */
        Reset_Handler,
        NMI_Handler,
        HardFault_Handler,
        MemManage_Handler,
        BusFault_Handler,
        UsageFault_Handler,
        0,
        0,
        0,
        0,
        SVC_Handler,
        DebugMon_Handler,
        0,
        PendSV_Handler,
        SysTick_Handler,

        /* External Interrupts */
        WWDG_IRQHandler,                    /* Window WatchDog              */
        PVD_IRQHandler,                     /* PVD through EXTI Line detection */
        TAMP_STAMP_IRQHandler,              /* Tamper and TimeStamps through the EXTI line */
        RTC_WKUP_IRQHandler,                /* RTC Wakeup through the EXTI line */
        FLASH_IRQHandler,                   /* FLASH                        */
        RCC_IRQHandler,                     /* RCC                          */
        EXTI0_IRQHandler,                   /* EXTI Line0                   */
        EXTI1_IRQHandler,                   /* EXTI Line1                   */
        EXTI2_IRQHandler,                   /* EXTI Line2                   */
        EXTI3_IRQHandler,                   /* EXTI Line3                   */
        EXTI4_IRQHandler,                   /* EXTI Line4                   */
        DMA1_Stream0_IRQHandler,            /* DMA1 Stream 0                */
        DMA1_Stream1_IRQHandler,            /* DMA1 Stream 1                */
        DMA1_Stream2_IRQHandler,            /* DMA1 Stream 2                */
        DMA1_Stream3_IRQHandler,            /* DMA1 Stream 3                */
        DMA1_Stream4_IRQHandler,            /* DMA1 Stream 4                */
        DMA1_Stream5_IRQHandler,            /* DMA1 Stream 5                */
        DMA1_Stream6_IRQHandler,            /* DMA1 Stream 6                */
        ADC_IRQHandler,                     /* ADC1, ADC2 and ADC3s         */
        0,                                  /* Reserved                     */
        0,                                  /* Reserved                     */
        0,                                  /* Reserved                     */
        0,                                  /* Reserved                     */
        EXTI9_5_IRQHandler,                 /* External Line[9:5]s          */
        TIM1_BRK_TIM9_IRQHandler,           /* TIM1 Break and TIM9          */
        TIM1_UP_TIM10_IRQHandler,           /* TIM1 Update and TIM10        */
        TIM1_TRG_COM_TIM11_IRQHandler,      /* TIM1 Trigger and Commutation and TIM11 */
        TIM1_CC_IRQHandler,                 /* TIM1 Capture Compare         */
        TIM2_IRQHandler,                    /* TIM2                         */
        TIM3_IRQHandler,                    /* TIM3                         */
        TIM4_IRQHandler,                    /* TIM4                         */
        I2C1_EV_IRQHandler,                 /* I2C1 Event                   */
        I2C1_ER_IRQHandler,                 /* I2C1 Error                   */
        I2C2_EV_IRQHandler,                 /* I2C2 Event                   */
        I2C2_ER_IRQHandler,                 /* I2C2 Error                   */
        SPI1_IRQHandler,                    /* SPI1                         */
        SPI2_IRQHandler,                    /* SPI2                         */
        USART1_IRQHandler,                  /* USART1                       */
        USART2_IRQHandler,                  /* USART2                       */
        0,                                  /* Reserved                     */
        EXTI15_10_IRQHandler,               /* External Line[15:10]s        */
        RTC_Alarm_IRQHandler,               /* RTC Alarm (A and B) through EXTI Line */
        OTG_FS_WKUP_IRQHandler,             /* USB OTG FS Wakeup through EXTI line */
        0,                                  /* Reserved                     */
        0,                                  /* Reserved                     */
        0,                                  /* Reserved                     */
        0,                                  /* Reserved                     */
        DMA1_Stream7_IRQHandler,            /* DMA1 Stream7                 */
        0,                                  /* Reserved                     */
        SDIO_IRQHandler,                    /* SDIO                         */
        TIM5_IRQHandler,                    /* TIM5                         */
        SPI3_IRQHandler,                    /* SPI3                         */
        0,                                  /* Reserved                     */
        0,                                  /* Reserved                     */
        0,                                  /* Reserved                     */
        0,                                  /* Reserved                     */
        DMA2_Stream0_IRQHandler,            /* DMA2 Stream 0                */
        DMA2_Stream1_IRQHandler,            /* DMA2 Stream 1                */
        DMA2_Stream2_IRQHandler,            /* DMA2 Stream 2                */
        DMA2_Stream3_IRQHandler,            /* DMA2 Stream 3                */
        DMA2_Stream4_IRQHandler,            /* DMA2 Stream 4                */
        0,                                  /* Reserved                     */
        0,                                  /* Reserved                     */
        0,                                  /* Reserved                     */
        0,                                  /* Reserved                     */
        0,                                  /* Reserved                     */
        0,                                  /* Reserved                     */
        OTG_FS_IRQHandler,                  /* USB OTG FS                   */
        DMA2_Stream5_IRQHandler,            /* DMA2 Stream 5                */
        DMA2_Stream6_IRQHandler,            /* DMA2 Stream 6                */
        DMA2_Stream7_IRQHandler,            /* DMA2 Stream 7                */
        USART6_IRQHandler,                  /* USART6                       */
        I2C3_EV_IRQHandler,                 /* I2C3 event                   */
        I2C3_ER_IRQHandler,                 /* I2C3 error                   */
        0,                                  /* Reserved                     */
        0,                                  /* Reserved                     */
        0,                                  /* Reserved                     */
        0,                                  /* Reserved                     */
        0,                                  /* Reserved                     */
        0,                                  /* Reserved                     */
        0,                                  /* Reserved                     */
        FPU_IRQHandler,                     /* FPU                          */
        0,                                  /* Reserved                     */
        0,                                  /* Reserved                     */
        SPI4_IRQHandler                     /* SPI4                         */
    }
};

