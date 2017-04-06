//MPU6050 I2C library for ARM STM32F103xx Microcontrollers - Main source file
//Has bit, byte and buffer I2C R/W functions
// 23/05/2012 by Harinadha Reddy Chintalapalli <harinath.ec@gmail.com>
// Changelog:
//     2012-05-23 - initial release. Thanks to Jeff Rowberg <jeff@rowberg.net> for his AVR/Arduino
//                  based MPU6050 development which inspired me & taken as reference to develop this.
/* ============================================================================================
 MPU6050 device I2C library code for ARM STM32F103xx is placed under the MIT license
 Copyright (c) 2012 Harinadha Reddy Chintalapalli

 Permission is hereby granted, free of charge, to any person obtaining a copy
 of this software and associated documentation files (the "Software"), to deal
 in the Software without restriction, including without limitation the rights
 to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 copies of the Software, and to permit persons to whom the Software is
 furnished to do so, subject to the following conditions:

 The above copyright notice and this permission notice shall be included in
 all copies or substantial portions of the Software.

 THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 THE SOFTWARE.
 ================================================================================================
 */

/* Includes */
#include "MPU6050.h"
#include "stm32f1xx_hal_i2c.h"
#include "inv_mpu.h"
#include "inv_mpu_dmp_motion_driver.h"

/** @defgroup MPU6050_Library
 * @{
 */

// defined in main.c
extern I2C_HandleTypeDef hi2c1;

// hi2c instance pointer
I2C_HandleTypeDef *_hi2c;

// i2c operation timeout
#define I2C_OPERATION_TIMEOUT 1000 // 1 second

int i2cReadByte(uint8_t devAddr, uint8_t regAddr, uint8_t* buf)
{
	if (HAL_I2C_Mem_Read(_hi2c, devAddr, regAddr, I2C_MEMADD_SIZE_8BIT,
	                     buf, 1, I2C_OPERATION_TIMEOUT) != HAL_OK)
		return -1;
	return 0;
}

int i2cReadBytes(uint8_t devAddr, uint8_t regAddr, uint8_t count, uint8_t* buf)
{
	if (HAL_I2C_Mem_Read(_hi2c, devAddr, regAddr, I2C_MEMADD_SIZE_8BIT,
	                     buf, count, I2C_OPERATION_TIMEOUT) != HAL_OK)
		return -1;
	return 0;
}

int i2cWriteByte(uint8_t devAddr, uint8_t regAddr, uint8_t data)
{
	if (HAL_I2C_Mem_Write(_hi2c, devAddr, regAddr, I2C_MEMADD_SIZE_8BIT,
		                    &data, 1, I2C_OPERATION_TIMEOUT) != HAL_OK)
		return -1;
	return 0;
}

int i2cWriteBytes(uint8_t devAddr, uint8_t regAddr, uint8_t count, uint8_t* data)
{
	if (HAL_I2C_Mem_Write(_hi2c, devAddr, regAddr, I2C_MEMADD_SIZE_8BIT,
		                    data, count, I2C_OPERATION_TIMEOUT) != HAL_OK)
		return -1;
	return 0;
}

int mpu6050GetMs(unsigned long *time)
{
		(void)time;
		return 1;
}

int mpu6050IsDeviceReady(uint8_t deviceAddr)
{
	if (HAL_I2C_IsDeviceReady(_hi2c, deviceAddr, 2, 5) != HAL_OK)
		return 0;
	return -1;
}

static signed char gyro_orientation[9] = {-1, 0, 0,
                                           0,-1, 0,
                                           0, 0, 1};

static  unsigned short inv_row_2_scale(const signed char *row)
{
	unsigned short b;

	if (row[0] > 0)
		b = 0;
	else if (row[0] < 0)
		b = 4;
	else if (row[1] > 0)
		b = 1;
	else if (row[1] < 0)
		b = 5;
	else if (row[2] > 0)
		b = 2;
	else if (row[2] < 0)
		b = 6;
	else
		b = 7;      // error
	return b;
}

static unsigned short inv_orientation_matrix_to_scalar(const signed char *mtx)
{
	unsigned short scalar;
	scalar = inv_row_2_scale(mtx);
	scalar |= inv_row_2_scale(mtx + 3) << 3;
	scalar |= inv_row_2_scale(mtx + 6) << 6;

	return scalar;
}

static int dmp_self_test(void)
{
	int result;
	long gyro[3], accel[3];

	result = mpu_run_self_test(gyro, accel);
	if (result == 0x7) {
		// Test passed. We can trust the gyro data here, so let's push it down to the DMP.
		float sens;
		unsigned short accel_sens;
		mpu_get_gyro_sens(&sens);
		gyro[0] = (long)(gyro[0] * sens);
		gyro[1] = (long)(gyro[1] * sens);
		gyro[2] = (long)(gyro[2] * sens);
		dmp_set_gyro_bias(gyro);
		mpu_get_accel_sens(&accel_sens);
		accel[0] *= accel_sens;
		accel[1] *= accel_sens;
		accel[2] *= accel_sens;
		dmp_set_accel_bias(accel);
		//printf("setting bias succesfully ......\r\n");
		return 0;
	}
	return -1;
}

int mpu6050InitDMP(uint16_t fifoRate)
{
	// load motion driver
	if(dmp_load_motion_driver_firmware()) {
		printf("MPU6050 dmp load motion driver firmware failed!\n");
		return -1;
	}

	// set orientation
	if(dmp_set_orientation(inv_orientation_matrix_to_scalar(gyro_orientation))) {
		printf("MPU6050 dmp set orientation failed!\n");
		return -1;
	}

	// enabled features
	if(dmp_enable_feature(DMP_FEATURE_6X_LP_QUAT | DMP_FEATURE_TAP |
	                      DMP_FEATURE_ANDROID_ORIENT | DMP_FEATURE_SEND_RAW_ACCEL | DMP_FEATURE_SEND_CAL_GYRO |
                        DMP_FEATURE_GYRO_CAL)) {
		printf("MPU6050 dmp enable features failed!\n");
		return -1;
	}

	// set fifo rate
	if(dmp_set_fifo_rate(fifoRate)) {
		printf("MPU6050 dmp set fifo rate failed!\n");
		return -1;
	}

	if (dmp_self_test()) {
		printf("MPU6050 dmp set bias failed!\n");
		return -1;
	}

	if(mpu_set_dmp_state(1)) { // turn on DMP
		printf("MPU6050 set dmp state failed\n");
		return -1;
	}

	return 0;
}

/** Power on and prepare for general usage.
 * This will activate the device and take it out of sleep mode (which must be done
 * after start-up). This function also sets both the accelerometer and the gyroscope
 * to their most sensitive settings, namely +/- accelRange and +/- gyroRange degrees/sec,
 * and sets the clock source to use the clkSource Gyro for reference, which is slightly
 * better than the default internal clock source.
 */
int mpu6050Init(uint8_t clkSource, uint8_t gyroRange, uint8_t accelRange,
                uint16_t sampleRate, int8_t enableDMP)
{
	// save i2c instance pointer
	_hi2c = &hi2c1;

	// check device connected
	if (!mpu6050IsDeviceReady(MPU6050_ADDR)) {
		printf("MPU6050 device not connected!\n");
		return -1;
	}

	// check device who am i
	uint8_t whoami = 0x00;
	i2cReadByte(MPU6050_ADDR, MPU6050_RA_WHO_AM_I, &whoami);
	if (whoami != MPU6050_I_AM) {
		printf("Not MPU6050 device!\n");
		return -1;
	}

	// try to init mpu with default value from InvenSense lib inv_mpu.c
	if(mpu_init(NULL)) {
		printf("MPU6050 init failed!\n");
		return -1;
	}

	// set clock source
	mpu6050SetClockSource(clkSource);

	// set gyro range
	mpu6050SetFullScaleGyroRange(gyroRange);

	// set accelerometer range
	mpu6050SetFullScaleAccelRange(accelRange);

	// disable sleep mode
	mpu6050SetSleepModeStatus(DISABLE);

	// disable mpu6050 i2c bypass
	if (mpu_set_bypass(DISABLE)) {
		printf("MPU6050 disable i2c bypass failed!\n");
		return -1;
	}

	// set sensors
	if(mpu_set_sensors(INV_XYZ_GYRO | INV_XYZ_ACCEL)) {
		printf("MPU6050 set sensor failed!\n");
		return -1;
	}

	// config fifo
	if(mpu_configure_fifo(INV_XYZ_GYRO | INV_XYZ_ACCEL)) {
		printf("MPU6050 configure fifo failed!\n");
		return -1;
	}

	// set sample rate
	if(mpu_set_sample_rate(sampleRate)) {
		printf("MPU6050 set sample rate failed!\n");
		return -1;
	}

	if (enableDMP) {
		if (mpu6050InitDMP(sampleRate))
			return -1;
	}

	return 0;
}

/** Write multiple bits in an 8-bit device register.
 * @param slaveAddr I2C slave device address
 * @param regAddr Register regAddr to write to
 * @param bitStart First bit position to write (0-7)
 * @param length Number of bits to write (not more than 8)
 * @param data Right-aligned value to write
 */
void mpu6050WriteBits(uint8_t slaveAddr, uint8_t regAddr, uint8_t bitStart, uint8_t length, uint8_t data)
{
	//      010 value to write
	// 76543210 bit numbers
	//    xxx   args: bitStart=4, length=3
	// 00011100 mask byte
	// 10101111 original value (sample)
	// 10100011 original & ~mask
	// 10101011 masked | value
	uint8_t tmp;
	if (!i2cReadByte(MPU6050_ADDR, regAddr, &tmp)) {
		uint8_t mask = ((1 << length) - 1) << (bitStart - length + 1);
		data <<= (bitStart - length + 1); // shift data into correct position
		data &= mask; // zero all non-important bits in data
		tmp &= ~(mask); // zero all important bits in existing byte
		tmp |= data; // combine data with existing byte
		i2cWriteByte(slaveAddr, regAddr, tmp);
	}
}

/** write a single bit in an 8-bit device register.
 * @param slaveAddr I2C slave device address
 * @param regAddr Register regAddr to write to
 * @param bitNum Bit position to write (0-7)
 * @param value New bit value to write
 */
void mpu6050WriteBit(uint8_t slaveAddr, uint8_t regAddr, uint8_t bitNum, uint8_t data)
{
	uint8_t tmp;
	if (!i2cReadByte(MPU6050_ADDR, regAddr, &tmp)) {
		tmp = (data != 0) ? (tmp | (1 << bitNum)) : (tmp & ~(1 << bitNum));
		i2cWriteByte(slaveAddr, regAddr, tmp);
	}
}

/** Read multiple bits from an 8-bit device register.
 * @param slaveAddr I2C slave device address
 * @param regAddr Register regAddr to read from
 * @param bitStart First bit position to read (0-7)
 * @param length Number of bits to read (not more than 8)
 * @param data Container for right-aligned value (i.e. '101' read from any bitStart position will equal 0x05)
 * @param timeout Optional read timeout in milliseconds (0 to disable, leave off to use default class value in readTimeout)
 */
void mpu6050ReadBits(uint8_t slaveAddr, uint8_t regAddr, uint8_t bitStart, uint8_t length, uint8_t *data)
{
	// 01101001 read byte
	// 76543210 bit numbers
	//    xxx   args: bitStart=4, length=3
	//    010   masked
	//   -> 010 shifted
	if (!i2cReadByte(MPU6050_ADDR, regAddr, data)) {
		uint8_t mask = ((1 << length) - 1) << (bitStart - length + 1);
		*data &= mask;
		*data >>= (bitStart - length + 1);
	}
}

/** Read a single bit from an 8-bit device register.
 * @param slaveAddr I2C slave device address
 * @param regAddr Register regAddr to read from
 * @param bitNum Bit position to read (0-7)
 * @param data Container for single bit value
 * @param timeout Optional read timeout in milliseconds (0 to disable, leave off to use default class value in readTimeout)
 */
void mpu6050ReadBit(uint8_t slaveAddr, uint8_t regAddr, uint8_t bitNum, uint8_t *data)
{
	if (!i2cReadByte(MPU6050_ADDR, regAddr, data)) {
		*data &= (1 << bitNum);
	}
}

/** Set clock source setting.
 * An internal 8MHz oscillator, gyroscope based clock, or external sources can
 * be selected as the MPU-60X0 clock source. When the internal 8 MHz oscillator
 * or an external source is chosen as the clock source, the MPU-60X0 can operate
 * in low power modes with the gyroscopes disabled.
 *
 * Upon power up, the MPU-60X0 clock source defaults to the internal oscillator.
 * However, it is highly recommended that the device be configured to use one of
 * the gyroscopes (or an external clock source) as the clock reference for
 * improved stability. The clock source can be selected according to the following table:
 *
 * <pre>
 * CLK_SEL | Clock Source
 * --------+--------------------------------------
 * 0       | Internal oscillator
 * 1       | PLL with X Gyro reference
 * 2       | PLL with Y Gyro reference
 * 3       | PLL with Z Gyro reference
 * 4       | PLL with external 32.768kHz reference
 * 5       | PLL with external 19.2MHz reference
 * 6       | Reserved
 * 7       | Stops the clock and keeps the timing generator in reset
 * </pre>
 *
 * @param source New clock source setting
 * @see mpu6050GetClockSource()
 * @see MPU6050_RA_PWR_MGMT_1
 * @see MPU6050_PWR1_CLKSEL_BIT
 * @see MPU6050_PWR1_CLKSEL_LENGTH
 */
void mpu6050SetClockSource(uint8_t source)
{
	mpu6050WriteBits(MPU6050_ADDR, MPU6050_RA_PWR_MGMT_1, MPU6050_PWR1_CLKSEL_BIT, MPU6050_PWR1_CLKSEL_LENGTH, source);
}

/** Set full-scale gyroscope range.
 * @param range New full-scale gyroscope range value
 * @see mpu6050GetFullScaleGyroRange()
 * @see MPU6050_GYRO_FS_250
 * @see MPU6050_RA_GYRO_CONFIG
 * @see MPU6050_GCONFIG_FS_SEL_BIT
 * @see MPU6050_GCONFIG_FS_SEL_LENGTH
 */
void mpu6050SetFullScaleGyroRange(uint8_t range)
{
	mpu6050WriteBits(MPU6050_ADDR, MPU6050_RA_GYRO_CONFIG, MPU6050_GCONFIG_FS_SEL_BIT, MPU6050_GCONFIG_FS_SEL_LENGTH, range);
}

// GYRO_CONFIG register

/** Get full-scale gyroscope range.
 * The FS_SEL parameter allows setting the full-scale range of the gyro sensors,
 * as described in the table below.
 *
 * <pre>
 * 0 = +/- 250 degrees/sec
 * 1 = +/- 500 degrees/sec
 * 2 = +/- 1000 degrees/sec
 * 3 = +/- 2000 degrees/sec
 * </pre>
 *
 * @return Current full-scale gyroscope range setting
 * @see MPU6050_GYRO_FS_250
 * @see MPU6050_RA_GYRO_CONFIG
 * @see MPU6050_GCONFIG_FS_SEL_BIT
 * @see MPU6050_GCONFIG_FS_SEL_LENGTH
 */
uint8_t mpu6050GetFullScaleGyroRange(void)
{
	uint8_t tmp;
	mpu6050ReadBits(MPU6050_ADDR, MPU6050_RA_GYRO_CONFIG, MPU6050_GCONFIG_FS_SEL_BIT, MPU6050_GCONFIG_FS_SEL_LENGTH, &tmp);
	return tmp;
}

/** Get full-scale accelerometer range.
 * The FS_SEL parameter allows setting the full-scale range of the accelerometer
 * sensors, as described in the table below.
 *
 * <pre>
 * 0 = +/- 2g
 * 1 = +/- 4g
 * 2 = +/- 8g
 * 3 = +/- 16g
 * </pre>
 *
 * @return Current full-scale accelerometer range setting
 * @see MPU6050_ACCEL_FS_2
 * @see MPU6050_RA_ACCEL_CONFIG
 * @see MPU6050_ACONFIG_AFS_SEL_BIT
 * @see MPU6050_ACONFIG_AFS_SEL_LENGTH
 */
uint8_t mpu6050GetFullScaleAccelRange(void)
{
	uint8_t tmp;
	mpu6050ReadBits(MPU6050_ADDR, MPU6050_RA_ACCEL_CONFIG, MPU6050_ACONFIG_AFS_SEL_BIT, MPU6050_ACONFIG_AFS_SEL_LENGTH, &tmp);
	return tmp;
}

/** Set full-scale accelerometer range.
 * @param range New full-scale accelerometer range setting
 * @see mpu6050GetFullScaleAccelRange()
 */
void mpu6050SetFullScaleAccelRange(uint8_t range)
{
	mpu6050WriteBits(MPU6050_ADDR, MPU6050_RA_ACCEL_CONFIG, MPU6050_ACONFIG_AFS_SEL_BIT, MPU6050_ACONFIG_AFS_SEL_LENGTH, range);
}

/** Get sleep mode status.
 * Setting the SLEEP bit in the register puts the device into very low power
 * sleep mode. In this mode, only the serial interface and internal registers
 * remain active, allowing for a very low standby current. Clearing this bit
 * puts the device back into normal mode. To save power, the individual standby
 * selections for each of the gyros should be used if any gyro axis is not used
 * by the application.
 * @return Current sleep mode enabled status
 * @see MPU6050_RA_PWR_MGMT_1
 * @see MPU6050_PWR1_SLEEP_BIT
 */
bool mpu6050GetSleepModeStatus(void)
{
	uint8_t tmp;
	mpu6050ReadBit(MPU6050_ADDR, MPU6050_RA_PWR_MGMT_1, MPU6050_PWR1_SLEEP_BIT, &tmp);
	return tmp == 0x00 ? false : true;
}

/** Set sleep mode status.
 * @param enabled New sleep mode enabled status
 * @see mpu6050GetSleepModeStatus()
 * @see MPU6050_RA_PWR_MGMT_1
 * @see MPU6050_PWR1_SLEEP_BIT
 */
void mpu6050SetSleepModeStatus(FunctionalState NewState)
{
	mpu6050WriteBit(MPU6050_ADDR, MPU6050_RA_PWR_MGMT_1, MPU6050_PWR1_SLEEP_BIT, NewState);
}

/** Get raw 6-axis motion sensor readings (accel/gyro).
 * Retrieves all currently available motion sensor values.
 * @param AccelGyro 16-bit signed integer array of length 6
 * @see MPU6050_RA_ACCEL_XOUT_H
 */
void mpu6050GetRawAccelGyro(int16_t* accelGyro)
{
	uint8_t tmpBuffer[14];
	if (!i2cReadBytes(MPU6050_ADDR, MPU6050_RA_ACCEL_XOUT_H, 14, tmpBuffer)) {
		// calculate acceleration
		for (int i = 0; i < 3; i++)
			accelGyro[i] = ((int16_t) ((uint16_t) tmpBuffer[2 * i] << 8) + tmpBuffer[2 * i + 1]);
		// calculate Angular rate
		for (int i = 4; i < 7; i++)
			accelGyro[i - 1] = ((int16_t) ((uint16_t) tmpBuffer[2 * i] << 8) + tmpBuffer[2 * i + 1]);
	}
}

/** Get temperature value
 *  @return temperature value of sensor
 */
float mpu6050GetTemperature(void)
{
	uint8_t data[2];
	uint16_t temp;
	i2cReadBytes(MPU6050_ADDR, MPU6050_RA_TEMP_OUT_H, 2, data);
	temp = data[0] << 8 | data[1];
	return temp / 340.0f + 36.53f;
	//return (int)((temp / 340.0f + 36.53f) * 10);
}

//typedef struct {
//	short gyro[3];
//	short accel[3];
//	float pitch;
//	float roll;
//} MPU6050Data;

int mpu6050GetRawData(MPU6050Data *data)
{
	uint8_t tmp[14];
	if (i2cReadBytes(MPU6050_ADDR, MPU6050_RA_ACCEL_XOUT_H, 14, tmp))
		return -1;

	data->accel[0]= tmp[0] << 8 | tmp[1];
	data->accel[1]= tmp[2] << 8 | tmp[3];
	data->accel[2]= tmp[4] << 8 | tmp[5];
	// tmp[6], tmp[7] are temperature data
	data->gyro[0] = tmp[8] << 8 | tmp[9];
	data->gyro[1] = tmp[10] << 8 | tmp[11];
	data->gyro[2] = tmp[12] << 8 | tmp[13];

	return 0;
}

#include <math.h>

#define Q30 1073741824.0f

int mpu6050GetDmpData(MPU6050Data *data)
{
	short sensors;	
	unsigned long sensor_timestamp;
	unsigned char more;
	float quat[4];
	long quat_raw[4];

	if (dmp_read_fifo(data->gyro, data->accel, quat_raw, &sensor_timestamp, &sensors, &more))
		return -1;

	if (sensors & INV_WXYZ_QUAT ) {    
		quat[0] = quat_raw[0] / Q30;
		quat[1] = quat_raw[1] / Q30;
		quat[2] = quat_raw[2] / Q30;
		quat[3] = quat_raw[3] / Q30;
		data->pitch = asin(-2 * quat[1] * quat[3] + 2 * quat[0]* quat[2])* 57.3; 	
		data->roll = atan2( 2 * quat[2] * quat[3] + 2 * quat[0] * quat[1],
		                   -2 * quat[1] * quat[1] - 2 * quat[2]* quat[2] + 1)* 57.3;
		return 0;
	}
	else
		return -1;
}

/**
 * @}
 *//* end of group MPU6050_Library */
