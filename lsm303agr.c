/**************************************************************************
 * Simple C library for the Adafruit LSM303AGR accelerometer.
 * ENGS 28
 * Accelerometer only, no magnetometer
 */

#include "i2c.h"
#include "lsm303agr.h"

/**************************************************************************
 * Write one byte to an accelerometer register (Datasheet, Table 20)
 */
void lsm303_AccelRegisterWrite(uint8_t RegisterAddress, uint8_t data) {
	i2c1_memWrite(LSM303_ADDRESS_ACCEL, RegisterAddress, 1, &data);
}

/**************************************************************************
 * Read one byte from an accelerometer register (Datasheet, Table 13)
 */
uint8_t lsm303_AccelRegisterRead(uint8_t RegisterAddress) {
	uint8_t data;
	i2c1_memRead(LSM303_ADDRESS_ACCEL, RegisterAddress, 1, &data);
	return data;
}

/**************************************************************************
 * Initialize the sensor
 * WHO_AM_I check, then configure control registers
 */
uint8_t lsm303_AccelInit() {
	if (lsm303_AccelRegisterRead(LSM303_WHO_AM_I_A) != 0x33) {
		return 0; // failure
	} else {
		// CTRL_REG1_A = 0x67 = 400Hz data rate, all axes on
		uint8_t ctrl_reg1 = 0x67;
		lsm303_AccelRegisterWrite(LSM303_CTRL_REG1_A, ctrl_reg1);

		// CTRL_REG4_A = 0x00 = ±2g range, continuous update
		uint8_t ctrl_reg4 = 0x00;
		lsm303_AccelRegisterWrite(LSM303_CTRL_REG4_A, ctrl_reg4);

		return 1; // success
	}
}

/**************************************************************************
 * Read raw acceleration data for X, Y, Z axes
 */
void lsm303_AccelReadRaw(lsm303AccelData_s *result) {
	uint8_t data[6];
	// Use auto-increment bit (MSB = 1) to read all 6 registers at once
	i2c1_memRead(LSM303_ADDRESS_ACCEL, LSM303_OUT_X_L_A | (1 << 7), 6, data);

	result->x = ((int16_t)data[1] << 8) | data[0];
	result->y = ((int16_t)data[3] << 8) | data[2];
	result->z = ((int16_t)data[5] << 8) | data[4];
}
