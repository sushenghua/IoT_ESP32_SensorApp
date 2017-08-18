/*
 * OrientationSensor: communicate with MPU6050 sensor
 * Copyright (c) 2017 Shenghua Su
 *
 */

#include "OrientationSensor.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "AppLog.h"

/////////////////////////////////////////////////////////////////////////////////////////
// Configuration
/////////////////////////////////////////////////////////////////////////////////////////
//-----------------------------------------------------------------------
//---------------- MPU6050 config
#define MPU6050_CONF_CLK_SOURCE   MPU6050_CLOCK_PLL_XGYRO
#define MPU6050_CONF_GYRO_RANGE   MPU6050_GYRO_FS_2000
#define MPU6050_CONF_ACCEL_RANGE  MPU6050_ACCEL_FS_2
#define MPU6050_CONF_DATA_RATE    4       // 250 ms
#define MPU6050_CONF_ENABLE_DMP   0       // 0: DISABLED, 1: ENABLED

#define MPU6050_GYRO_SCALE_FACTOR 16.4    // Sensitivity Scale Factor 16.4 (FS_SEL=3) from Gyroscope Specifications
#define MPU6050_ACCE_SCALE_FACTOR 16384.0 // Sensitivity Scale Factor 16384 (AFS_SEL=0) from Gyroscope Specifications

//-----------------------------------------------------------------------
// ---------------- MPU6050 read delay
#ifndef MPU6050_READY_DELAY
#if MPU6050_CONF_ENABLE_DMP == 1
#define MPU6050_READY_DELAY       200
#else
#define MPU6050_READY_DELAY       100
#endif
#endif

/////////////////////////////////////////////////////////////////////////////////////////
// Orientation class
/////////////////////////////////////////////////////////////////////////////////////////

// delay definition
#ifndef delay
#define delay(x)                 vTaskDelay((x)/portTICK_RATE_MS)
#endif

void OrientationSensor::init()
{
  delay(MPU6050_READY_DELAY);
  if (_mpu6050.init(MPU6050_CONF_CLK_SOURCE, MPU6050_CONF_GYRO_RANGE,
      MPU6050_CONF_ACCEL_RANGE, MPU6050_CONF_DATA_RATE, MPU6050_CONF_ENABLE_DMP)) {
  }
}

#include <math.h>

const float RADIANS_TO_DEGREES = 180/3.14159269;

inline float calculateAngle(float x, float y)
{
  float angle = asinf(y / sqrtf(x*x + y*y)) * RADIANS_TO_DEGREES;
  if (x > 0) {
    if (y < 0) angle += 360;
  }
  else {
    angle = 180 - angle;
  }
  angle -= 90; // install rotation adjust
  return angle;
}

#define ORITENTATION_THRESHOLD_VALUE  0.9

void OrientationSensor::_updateOrientation()
{
  if (!_mpu6050.getRawData(&_mpuData)) {
//		printf("accel: %f, %f, %f  ",
//					 _mpuData.accel[0] / MPU6050_ACCE_SCALE_FACTOR,
//					 _mpuData.accel[1] / MPU6050_ACCE_SCALE_FACTOR,
//					 _mpuData.accel[2] / MPU6050_ACCE_SCALE_FACTOR);

//		printf("gyro: %f, %f, %f\n",
//					 _mpuData.gyro[0] / MPU6050_GYRO_SCALE_FACTOR,
//					 _mpuData.gyro[1] / MPU6050_GYRO_SCALE_FACTOR,
//					 _mpuData.gyro[2] / MPU6050_GYRO_SCALE_FACTOR);

    float accelX = _mpuData.accel[0] / MPU6050_ACCE_SCALE_FACTOR;
    float accelY = _mpuData.accel[1] / MPU6050_ACCE_SCALE_FACTOR;
    float angle = calculateAngle(accelX, accelY);

#ifdef DEBUG_APP
    float accelZ = _mpuData.accel[2] / MPU6050_ACCE_SCALE_FACTOR;
    uint8_t rotation = _dc->rotation();
    APP_LOGC("[OriSensor]", "rotation: %d,  aX: %f  aY: %f  aZ: %f  angle: %f", rotation, accelX, accelY, accelZ, angle);
#endif

    // for MPU6050 chip(on PCB) and LCD installed as both face front
    // if (fabs(angle - 0) < 20) {
    //   _dc->setRotation(DISPLAY_ROTATION_CW_0);
    // }
    // else if (fabs(angle - 90.f) < 20) {
    //   _dc->setRotation(DISPLAY_ROTATION_CW_270);
    // }
    // else if (fabs(angle + 90.f) < 20 || fabs(angle - 270.f) < 20) {
    //   _dc->setRotation(DISPLAY_ROTATION_CW_90);
    // }
    // else if (fabs(angle - 180.f) < 20) {
    //   _dc->setRotation(DISPLAY_ROTATION_CW_180);
    // }

    // for MPU6050 chip(on PCB) and LCD installed as back-to-back
    if (fabs(angle - 0) < 20) {
      _dc->setRotation(DISPLAY_ROTATION_CW_0);
    }
    else if (fabs(angle - 90.f) < 20) {
      _dc->setRotation(DISPLAY_ROTATION_CW_90);
    }
    else if (fabs(angle + 90.f) < 20 || fabs(angle - 270.f) < 20) {
      _dc->setRotation(DISPLAY_ROTATION_CW_270);
    }
    else if (fabs(angle - 180.f) < 20) {
      _dc->setRotation(DISPLAY_ROTATION_CW_180);
    }
  }
}
