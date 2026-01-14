/*
 * sysinit.c
 *
 *  Created on: Oct 25, 2024
 *
 */

#include <stm32c0xx.h>
#include <stdint.h>

uint32_t milliseconds( void );

void delay_ms( uint32_t ms );

__attribute__((weak)) void SysTickInit( void );


void SystemInit( void ) {
	SysTickInit();
}


/*
 * delay( uint32_t ) is the standard dead loop delay.  Here we are using
 * the static currentMilliseconds to do the work for us, so the resulting
 * delay will be from n-1 to n milliseconds.  If the caller is very unlucky,
 * the delay will be exceptionally brief.
 *
 * More precise delays can be obtained with delay_us, which uses timer 17 in
 * a polled mode to wait the specified number of microseconds.  This code assumes
 * the system clock is running at 12MHz.
 */

static uint32_t currentMilliseconds = 0;

/*
 *  Public functions
 */
void delay_ms(uint32_t ms) {
	uint32_t stop = milliseconds() + ms;
	while (milliseconds() < stop)
		;
}

void SysTickInit(void) {

	// Assumes 12MHz clock...could be better
	SysTick->LOAD = 12000 - 1;

	SysTick->CTRL |= SysTick_CTRL_CLKSOURCE_Msk;  // Select the processor clock
	SysTick->CTRL |= SysTick_CTRL_TICKINT_Msk;    // Enable the interrupt
	SysTick->CTRL |= SysTick_CTRL_ENABLE_Msk;     // Turn it on.

}

void SysTick_Handler(void) {
	currentMilliseconds++;
}

inline uint32_t milliseconds(void) {
	return currentMilliseconds;
}
