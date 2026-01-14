/*
 * uart.c
 *
 *  Created on: Jul 19, 2024
 *
 */

#include "uart.h"
#include "ES28.h"

#define SYS_FREQ		  12000000U		// standard clock HSISYS (by default scaled down by a factor 4 from HSI48 RC)
#define APB_CLK		  	  SYS_FREQ		// No additional prescaling
#define UART_BAUD_RATE    9600

static void uart_set_baudrate(USART_TypeDef *USARTx, uint32_t PeriphClk, uint32_t BaudRate);
static uint16_t compute_uart_bd(uint32_t PeriphClk, uint32_t BaudRate);
void uart2_write(int ch);

/* Remap the system putchar() to use our uart2_write() function. */
int __io_putchar(int ch) {
	uart2_write(ch);
	return ch;
}

/* Initializations */
void uart2_init(void) {
	/*********** configure uart gpio pin ******/
	/* enable clock access to gpioa */
	RCC->IOPENR |= RCC_IOPENR_GPIOAEN;

	/* set PA2 (rx) mode to alternate function mode */
	GPIOA->MODER &= ~GPIO_MODER_MODE2_Msk;
	GPIOA->MODER |= (GPIO_ALTERNATE << GPIO_MODER_MODE2_Pos);

	/* set PA3 (tx) mode to alternate function mode */
	GPIOA->MODER &= ~GPIO_MODER_MODE3_Msk;
	GPIOA->MODER |= (GPIO_ALTERNATE << GPIO_MODER_MODE3_Pos);

	/* set PA2 alternate function type to AF01 which is mapped to UART_TX for PA2 */
	GPIOA->AFR[0] &= ~GPIO_AFRL_AFSEL2_Msk;
	GPIOA->AFR[0] |= (GPIO_AF1 << GPIO_AFRL_AFSEL2_Pos);

	/* set PA3 alternate function type to AF01 which is mapped to UART_RX for PA3 */
	GPIOA->AFR[0] &= ~GPIO_AFRL_AFSEL3_Msk;
	GPIOA->AFR[0] |= (GPIO_AF1 << GPIO_AFRL_AFSEL3_Pos);

	/***********configure uart module **********/
	/* enable clock access to uart2 */
	RCC->APBENR1 |= RCC_APBENR1_USART2EN;

	/* configure baudrate */
	uart_set_baudrate(USART2, APB_CLK, UART_BAUD_RATE);

	/* enable transmitter and receiver, clear other bits (set them back to defaults) */
	USART2->CR1 = USART_CR1_RE | USART_CR1_TE;

	/* enable uart module */
	USART2->CR1 |= USART_CR1_UE;
}

/* UART2 write function (putchar) */
void uart2_write(int ch) {
	/* Make sure the transmit data register is empty */
	while(!(USART2->ISR & USART_ISR_TXE_TXFNF)){} // Block until transmitter ready

	/* Write to transmit data register */
	USART2->TDR = (ch & 0xFF); // restrict to 8 bit
}

/* UART2 read function */
int uart2_read() {
	/* Make sure the receive data register is non-empty */
	while(!(USART2->ISR & USART_ISR_RXNE_RXFNE)){}

	/* Return data register */
	return(USART2->RDR & 0xFF);
}


/* Set the baud rate for the USART */
static void uart_set_baudrate(USART_TypeDef *USARTx, uint32_t PeriphClk, uint32_t BaudRate) {
	USARTx->BRR = compute_uart_bd(PeriphClk, BaudRate);
}

/* Determine what value to load into the baud rate register */
static uint16_t compute_uart_bd(uint32_t PeriphClk, uint32_t BaudRate) {
	/* This formula computes round(PeriphClk / BaudRate) (add 1/2 and truncate) */
	return ((PeriphClk + (BaudRate/2U))/BaudRate);
}
