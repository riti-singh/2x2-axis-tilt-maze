#include "ES28.h"
#include "i2c.h"

//  Pinout:
//	SCL: PB8
//  SDA: PB9
//  Use Alternate function mode AF6

void i2c1_init(void){

	// Configure pins PB8 and PB9
	/* Enable clock access to GPIOB (for PB8 and PB9) */
	RCC->IOPENR |= RCC_IOPENR_GPIOBEN;

	/* Set PB8 and PB9 to alternate function mode (0b10 = 0x2) */
	GPIOB->MODER = (GPIOB->MODER & ~GPIO_MODER_MODE8_Msk) | (GPIO_ALTERNATE << GPIO_MODER_MODE8_Pos);
	GPIOB->MODER = (GPIOB->MODER & ~GPIO_MODER_MODE9_Msk) | (GPIO_ALTERNATE << GPIO_MODER_MODE9_Pos);

	/* Next, select AF6 for these pins.	*/
	GPIOB->AFR[1] = ((GPIOB->AFR[1]) & ~GPIO_AFRH_AFSEL8_Msk) | (GPIO_AF6 << GPIO_AFRH_AFSEL8_Pos);
	GPIOB->AFR[1] = ((GPIOB->AFR[1]) & ~GPIO_AFRH_AFSEL9_Msk) | (GPIO_AF6 << GPIO_AFRH_AFSEL9_Pos);

	/* Set PB8 and PB9 to open drain */
	GPIOB->OTYPER |= (GPIO_OTYPER_OT8 | GPIO_OTYPER_OT9);

	/* Enable pull-ups for PB8 and PB9 if needed: our devices have their own pull-ups. */

	// Configure the i2c module
	/* Enable clock access to I2C1 */
	RCC->APBENR1 |= RCC_APBENR1_I2C1EN;

	/* Enter reset mode (disable peripheral) */
	I2C1->CR1 &= ~I2C_CR1_PE;

	// Timing settings: Standard mode (100kHz) with 12MHz I2CCLK
	I2C1->TIMINGR &= 0x0; // clear timing register
	I2C1->TIMINGR |= (0x2  << I2C_TIMINGR_PRESC_Pos);
	I2C1->TIMINGR |= (0x4  << I2C_TIMINGR_SCLDEL_Pos) | (0x2 << I2C_TIMINGR_SDADEL_Pos);
	I2C1->TIMINGR |= (0xf << I2C_TIMINGR_SCLH_Pos) | (0x13 << I2C_TIMINGR_SCLL_Pos);

	/* Exit reset mode (enable peripheral) */
	I2C1->CR1 |= I2C_CR1_PE;
}

/* Transmit one byte to deviceAddr */
void i2c1_byteWrite(uint8_t deviceAddr, uint8_t data) {
	// Ping target address for writing 1 byte, use "=" to ensure all other bits are cleared
	I2C1->CR2  = (deviceAddr<<1)
			    | (1U << I2C_CR2_NBYTES_Pos)      // will send 1 byte
				| I2C_CR2_AUTOEND				  // Send Stop condition after transfer
				| I2C_CR2_START;				  // Send Start condition
	while( !(I2C1->ISR & I2C_ISR_BUSY) );		  // Wait until Start is received

	// Send the data
	if( !(I2C1->ISR & I2C_ISR_NACKF) ) {		  // Make sure ACK received so we are actually talking
		while( !(I2C1->ISR & I2C_ISR_TXIS) );     // Wait for Tx to be ready
		I2C1->TXDR = data;				 		  // Write the data
	}
}

/* Receive one bytes from deviceAddr */
void i2c1_byteRead(uint8_t deviceAddr, uint8_t *data) {
	// Ping target address for reading 1 byte, use "=" to ensure all other bits are cleared
	I2C1->CR2 = (deviceAddr<<1)                  // target address
			    | (1U << I2C_CR2_NBYTES_Pos)     // will read 1 bytes
			    | I2C_CR2_RD_WRN                 // ping for reading
				| I2C_CR2_AUTOEND                // Send Stop condition after transfer
			    | I2C_CR2_START;				 // Send Start condition
	while( !(I2C1->ISR & I2C_ISR_BUSY) );		 // Wait until Start is received

	// Read the data
	if( !(I2C1->ISR & I2C_ISR_NACKF) ) {		 // Make sure ACK received so we are actually talking
		while( !(I2C1->ISR & I2C_ISR_RXNE) );    // Wait for Rx to be ready
		*data = I2C1->RXDR;				 		 // Read the data
	}
}

/* Write n bytes to a register location on deviceAddr */
void i2c1_memWrite(uint8_t deviceAddr, uint8_t registerAddr, uint8_t nbytes, uint8_t *data) {
	// Ping target address for writing n+1 bytes, first byte is the memory location on the device to which to write
	I2C1->CR2 = (deviceAddr<<1)                      // target address
			    | ((nbytes+1) << I2C_CR2_NBYTES_Pos) // n+1 to account for first sending the registerAddr
				| I2C_CR2_AUTOEND                // Send Stop condition after transfer
			    | I2C_CR2_START;				 // Send Start condition
	while( !(I2C1->ISR & I2C_ISR_BUSY) );		 // Wait until Start is received

	if( !(I2C1->ISR & I2C_ISR_NACKF) ) {		  // Make sure ACK received so we are actually talking
		// Send register address
		while( !(I2C1->ISR & I2C_ISR_TXIS) );     // Wait for Tx to be ready
		I2C1->TXDR = registerAddr;			      // Send register address

		// Send data
		for(uint8_t i=0; i<nbytes; i++) {
			while( !(I2C1->ISR & I2C_ISR_TXIS) ); // Wait for Tx to be ready
			I2C1->TXDR = data[i];				  // Write the data
		}
	}
}


/* Read n bytes from a register location on deviceAddr */
void i2c1_memRead(uint8_t deviceAddr, uint8_t registerAddr, uint8_t nbytes, uint8_t *data) {
	// Ping target address for writing 1 byte (the register address within target from which to read)
	I2C1->CR2 = (deviceAddr<<1)                  // target address
			    | (1U << I2C_CR2_NBYTES_Pos)     // for registerAddr
				| I2C_CR2_AUTOEND                // Send Stop condition after transfer
			    | I2C_CR2_START;				 // Send Start condition
	while( !(I2C1->ISR & I2C_ISR_BUSY) );		 // Wait until Start is received

	if( !(I2C1->ISR & I2C_ISR_NACKF) ) {		  // Make sure ACK received so we are actually talking
		// Send register address
		while( !(I2C1->ISR & I2C_ISR_TXIS) );     // Wait for Tx to be ready
		I2C1->TXDR = registerAddr;			      // Send register address
	}

	// Restart, with read request for n bytes
	I2C1->CR2 = (deviceAddr<<1)                  // target address
			    | (nbytes << I2C_CR2_NBYTES_Pos) // will read n bytes
				| I2C_CR2_RD_WRN                 // ping for reading
			    | I2C_CR2_AUTOEND                // Send Stop condition after transfer
				| I2C_CR2_START;				 // Send Start condition
	while( !(I2C1->ISR & I2C_ISR_BUSY) );		 // Wait until Start is received

	// Read the data
	if( !(I2C1->ISR & I2C_ISR_NACKF) ) {			// Make sure ACK received so we are actually talking
		for(uint8_t i=0; i<nbytes; i++) {
			while( !(I2C1->ISR & I2C_ISR_RXNE) );		// Wait for Rx to be ready
			data[i] = I2C1->RXDR;						// Read the data
		}
	}
}
