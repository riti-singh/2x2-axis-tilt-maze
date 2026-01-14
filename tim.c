#include "tim.h"
#include "ES28.h"

// Clock runs at 12MHz
/* Prescaler for Timer 16, which interrupts every second:
 * Need to prescale by 12000 to have counting to 1000 take 1 second  */
#define TIM16_PSC_FACTOR	12000		// 12 MHz / 12000 = 1 kHz

void tim16_ms_interrupt_init(int milliseconds) {
	/* enable clock access to timer 16 (on APB bus) */
	RCC->APBENR2 |= RCC_APBENR2_TIM16EN;

	/* Set prescaler value */
	TIM16->PSC = TIM16_PSC_FACTOR - 1; // starts counting at 0

	/* Set auto-reload value */
	TIM16->ARR = milliseconds - 1;

	/* Clear counter */
	TIM16->CNT = 0;

	/* Enable update interrupt */
	TIM16->DIER |= TIM_DIER_UIE;

	/* Enable timer interrupt in NVIC */
	NVIC_EnableIRQ(TIM16_IRQn);

	/* Enable timer */
	TIM16->CR1 |= TIM_CR1_CEN;
}

void tim14_pwm_set(uint16_t value) {
	TIM14->CCR1 = value-1;
}

void tim14_pa7_pwm_init(uint16_t prescaleFactor, uint16_t timerMax) {
	// Enable clock access to GPIOA
	RCC->IOPENR |=  RCC_IOPENR_GPIOAEN;

	// Set PA7 in alternate function mode (10)
	GPIOA->MODER &= ~GPIO_MODER_MODE7_Msk;
	GPIOA->MODER |= GPIO_ALTERNATE << GPIO_MODER_MODE7_Pos;

	// Set PA7 alternate function type (AF4, 0100)
	GPIOA->AFR[0] &= ~GPIO_AFRL_AFSEL7_Msk;
	GPIOA->AFR[0] |= GPIO_AF4 << GPIO_AFRL_AFSEL7_Pos;

	// Enable clock access to Timer 14
	RCC->APBENR2 |= RCC_APBENR2_TIM14EN;

	// Set prescaler
	TIM14->PSC = prescaleFactor-1;

	// Set autoreload value
	TIM14->ARR = timerMax;

	// Set the compare register value - start with a speed of 0
	TIM14->CCR1 = 0;

	// Set pwm mode 1 (0110): pin is high up to capture value, then goes low
	TIM14->CCMR1 &= ~TIM_CCMR1_OC1M_Msk;
	TIM14->CCMR1 |= TIM_OC1_PWM1 << TIM_CCMR1_OC1M_Pos;

	// Make sure timer is in output mode; this is default value
	TIM14->CCMR1 &= ~TIM_CCMR1_CC1S_Msk;
	TIM14->CCMR1 |= TIM_CC1_OUTPUT << TIM_CCMR1_CC1S_Pos;

	// Enable TIM14_CH1 in output mode
	TIM14->CCER |= TIM_CCER_CC1E;

	// Generate an update event - this clears counter, prescaler counter, updates registers
	TIM14->EGR |= TIM_EGR_UG;

	// Enable timer
	TIM14->CR1 |= TIM_CR1_CEN;
}
