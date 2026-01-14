#include "stm32c0xx.h"
#include "ES28.h"
#include "ADC.h"
#include "uart.h"
#include <stdio.h>
#include <stdlib.h>

#define MIDPOINT   2048
#define THRESHOLD  100

// States
typedef enum { INIT, READY, RUNNING, FINISHED, TIMEOUT } GameState;
GameState state = INIT;

// Pin defs
#define BUTTON_PORT    GPIOB
#define BUTTON_PIN     4

#define RED_LED_PORT   GPIOB
#define RED_LED_PIN    3

#define GREEN_LED_PORT GPIOB
#define GREEN_LED_PIN 10

#define START_SENSOR_PORT GPIOA
#define START_SENSOR_PIN  5

#define END_SENSOR_PORT   GPIOA
#define END_SENSOR_PIN    6

void gpio_init(void) {
    RCC->IOPENR |= RCC_IOPENR_GPIOAEN | RCC_IOPENR_GPIOBEN | RCC_IOPENR_GPIOCEN;

    // Step and Dir outputs
    GPIOA->MODER &= ~(GPIO_MODER_MODE7_Msk | GPIO_MODER_MODE9_Msk);
    GPIOA->MODER |= (1 << GPIO_MODER_MODE7_Pos) | (1 << GPIO_MODER_MODE9_Pos);
    GPIOB->MODER &= ~GPIO_MODER_MODE0_Msk;
    GPIOB->MODER |= (1 << GPIO_MODER_MODE0_Pos);
    GPIOC->MODER &= ~GPIO_MODER_MODE7_Msk;
    GPIOC->MODER |= (1 << GPIO_MODER_MODE7_Pos);

    // LEDs
    GPIOB->MODER &= ~(GPIO_MODER_MODE3_Msk | GPIO_MODER_MODE10_Msk);
    GPIOB->MODER |= (1 << GPIO_MODER_MODE3_Pos) | (1 << GPIO_MODER_MODE10_Pos);

    // Button input (PB4)
    GPIOB->MODER &= ~GPIO_MODER_MODE4_Msk;
    GPIOB->PUPDR &= ~GPIO_PUPDR_PUPD4_Msk;
    GPIOB->PUPDR |= (1 << GPIO_PUPDR_PUPD4_Pos);

    // Maze sensors (PA5/PA6)
    GPIOA->MODER &= ~(GPIO_MODER_MODE5_Msk | GPIO_MODER_MODE6_Msk);
    GPIOA->PUPDR &= ~(GPIO_PUPDR_PUPD5_Msk | GPIO_PUPDR_PUPD6_Msk); // no pull
}

int button_pressed(void) {
    return !(BUTTON_PORT->IDR & (1 << BUTTON_PIN)); // active low
}

void set_led(GPIO_TypeDef *port, int pin, int on) {
    if (on)
        port->ODR |= (1 << pin);
    else
        port->ODR &= ~(1 << pin);
}

void step_motor(GPIO_TypeDef *DIR_PORT, uint8_t DIR_PIN,
                GPIO_TypeDef *STEP_PORT, uint8_t STEP_PIN,
                int direction) {
    if (direction) DIR_PORT->ODR |= (1 << DIR_PIN);
    else           DIR_PORT->ODR &= ~(1 << DIR_PIN);

    STEP_PORT->ODR |= (1 << STEP_PIN);
    delay_ms(1);
    STEP_PORT->ODR &= ~(1 << STEP_PIN);
    delay_ms(1);
}

void countdown(void) {
    for (int i = 3; i >= 1; i--) {
        printf("Countdown: %d\n", i);

        // TODO: 7-segment display here

        set_led(RED_LED_PORT, RED_LED_PIN, 1);
        delay_ms(250);
        set_led(RED_LED_PORT, RED_LED_PIN, 0);
        delay_ms(750);
    }
}

int main(void) {
    SystemInit();
    uart2_init();
    gpio_init();
    adc_init();

    printf("Marble Maze game starting...\r\n");

    while (1) {
        switch (state) {
            case INIT:
                set_led(RED_LED_PORT, RED_LED_PIN, 1);
                set_led(GREEN_LED_PORT, GREEN_LED_PIN, 0);
                printf("WAITING FOR BUTTON PRESS...\n");

                while (!button_pressed());
                delay_ms(100); // debounce

                state = READY;
                break;

            case READY:
                printf("GET READY!\n");
                countdown();
                set_led(RED_LED_PORT, RED_LED_PIN, 0);
                set_led(GREEN_LED_PORT, GREEN_LED_PIN, 1);

                // TODO: start timer
                state = RUNNING;
                break;

            case RUNNING: {
                adc_setChannel(1); int joyX = adc_getValue();
                adc_setChannel(0); int joyY = adc_getValue();

                int distX = abs(joyX - MIDPOINT);
                int dirX = (joyX > MIDPOINT);
                if (distX > THRESHOLD) {
                    int steps = (distX > 1000) ? 6 : (distX > 600) ? 4 : (distX > 300) ? 2 : 1;
                    for (int i = 0; i < steps; i++) {
                        step_motor(GPIOA, 9, GPIOC, 7, dirX);
                    }
                }

                int distY = abs(joyY - MIDPOINT);
                int dirY = (joyY > MIDPOINT);
                if (distY > THRESHOLD) {
                    int steps = (distY > 1000) ? 6 : (distY > 600) ? 4 : (distY > 300) ? 2 : 1;
                    for (int i = 0; i < steps; i++) {
                        step_motor(GPIOB, 0, GPIOA, 7, dirY);
                    }
                }

                // Start sensor debug
                if ((GPIOA->IDR & (1 << START_SENSOR_PIN)) == 0) {
                    printf("Marble left the start zone.\n");
                }

                // End sensor detection (PA6)
                if (GPIOA->IDR & (1 << END_SENSOR_PIN)) {
                    printf("🎯 Marble reached the goal!\n");
                    state = FINISHED;
                }

                break;
            }

            case FINISHED:
                printf("YOU WIN!\n");
                set_led(GREEN_LED_PORT, GREEN_LED_PIN, 1);
                set_led(RED_LED_PORT, RED_LED_PIN, 0);

                // TODO: display "DONE" on 7-segment
                while (1); // freeze
                break;

            case TIMEOUT:
                printf("TIME’S UP!\n");
                set_led(RED_LED_PORT, RED_LED_PIN, 1);
                set_led(GREEN_LED_PORT, GREEN_LED_PIN, 0);

                // TODO: display "FAIL"
                while (1); // freeze
                break;
        }
    }
}

