#include <stm32f401xe.h>


/*
 * The green LED is connected to port A5,
 * -> see schematic of NUCLEO-F401RE board
 */
#define LED_GPIO        GPIOA
#define LED_PIN         5


/**
 * Quick 'n' dirty delay
 *
 * @param time the larger it is the longer it will block
 */
static void delay(unsigned time) {
    for (unsigned i=0; i<time; i++)
        for (volatile unsigned j=0; j<20000; j++);
}


/**
 * Hello world blinky program
 *
 * @return never
 */
int main(void) {

    /*
     * Turn on the GPIOA unit,
     * -> see section 6.3.9 in the manual
     */
    RCC->AHB1ENR  |= RCC_AHB1ENR_GPIOAEN;


    /*
     * Set LED-Pin as output
     * Note: For simplicity this assumes the pin was configured
     * as input before, as it is when it comes out of reset.
     * -> see section 8.4.1 in the manual
     */
    LED_GPIO->MODER |= (0b01 << (LED_PIN << 1));


    while(1) {

        /*
         * LED on (drive the pin high)
         * A GPIO output pin is set to high by writing 1 to the
         * corresponding bit in the lower half of the BSRR register
         * -> see section 8.4.7 in the manual
         */
        LED_GPIO->BSRR = (1 << LED_PIN);

        delay(200);

        /*
         * LED off (drive the pin low)
         * A GPIO output pin is set to low by writing 1 to the
         * corresponding bit in the upper half of the BSRR register
         * -> see section 8.4.7 in the manual
         */
        LED_GPIO->BSRR = (1 << (LED_PIN + 16));

        delay(200);
    }
}

