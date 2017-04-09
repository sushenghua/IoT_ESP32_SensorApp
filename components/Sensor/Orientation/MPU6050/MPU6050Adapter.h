/*
 * MPU6050Adapter: adapter for mpu6060 i2c and other stuff
 * Copyright (c) 2017 Shenghua Su
 *
 */

#ifndef _MPU6050_ADAPTER_H
#define _MPU6050_ADAPTER_H

#ifndef MPU6050_ADDR
#define MPU6050_ADDRESS_AD0_LOW     0x68 // address pin low (GND), default for InvenSense evaluation board
#define MPU6050_ADDRESS_AD0_HIGH    0x69 // address pin high (VCC)
#define MPU6050_ADDR                MPU6050_ADDRESS_AD0_LOW
#endif

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#ifdef __cplusplus
extern "C" {
#endif

int i2cReadBytes(uint8_t addr, uint8_t reg, uint8_t len, uint8_t *buf);
int i2cWriteBytes(uint8_t addr, uint8_t reg, uint8_t len, uint8_t *data);
int mpu6050GetMs(unsigned long *time);

#ifndef delay
#define delay(x) vTaskDelay((x)/portTICK_RATE_MS)
#endif

int mpu6050GetMs(unsigned long *time);

#ifdef __cplusplus
}
#endif

#endif // _MPU6050_ADAPTER_H