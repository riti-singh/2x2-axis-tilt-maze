#include "stm32c0xx.h"
#include <stdint.h>

#define SENSOR_PIN   1  // PA1
#define SENSOR_MASK  (1U << SENSOR_PIN)

static volatile uint32_t pulse_count = 0;
extern uint32_t milliseconds(void);  // declare it here

void rpm_sensor_init(void) {
    RCC->IOPENR |= RCC_IOPENR_GPIOAEN;

    GPIOA->MODER &= ~(3U << (2 * SENSOR_PIN));  // input mode

    // Enable internal pull-up
    GPIOA->PUPDR &= ~(3U << (2 * SENSOR_PIN));
    GPIOA->PUPDR |=  (1U << (2 * SENSOR_PIN));  // pull-up

    // Enable rising edge interrupt on PA1
    EXTI->RTSR1 |= SENSOR_MASK;
    EXTI->IMR1  |= SENSOR_MASK;

    NVIC_EnableIRQ(EXTI0_1_IRQn);
}

void EXTI0_1_IRQHandler(void) {
    if (EXTI->RPR1 & SENSOR_MASK) {
        pulse_count++;
        EXTI->RPR1 |= SENSOR_MASK;  // clear flag
    }
}

int rpm_read(void) {
    pulse_count = 0;
    uint32_t start = milliseconds();

    while ((milliseconds() - start) < 1000);

    return (60 * pulse_count) / 20;
}
