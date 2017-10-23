/*
 * OrientationSensor: communicate with MPU6050 sensor
 * Copyright (c) 2017 Shenghua Su
 *
 */

#ifndef _ORIENTATION_SENSOR_H
#define _ORIENTATION_SENSOR_H

#include "SensorDisplayController.h"
#include "MPU6050.h"

class OrientationSensor
{
public:
    void init(bool checkDeviceReady=true);    
    void setDisplayDelegate(SensorDisplayController *dc) { _dc = dc; }

    // sample
    void sampleData();
    float readTemperature();

protected:
    uint16_t                  _pin;
    SensorDisplayController  *_dc;
    MPU6050Data               _mpuData;
    MPU6050Sensor             _mpu6050;
};

#endif // _ORIENTATION_SENSOR_H
