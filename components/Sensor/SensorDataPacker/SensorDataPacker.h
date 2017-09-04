/*
 * SensorDataPacker: for package different sensors' data into single block
 * Copyright (c) 2016 Shenghua Su
 *
 */

#ifndef _SENSOR_DATA_PACKER_H
#define _SENSOR_DATA_PACKER_H

#include <stdint.h>
#include "SHT3xSensor.h"
#include "PMSensor.h"
#include "CO2Sensor.h"
#include "OrientationSensor.h"

#define BUF_SIZE (sizeof(PMData) + sizeof(HchoData) + sizeof(TempHumidData) + sizeof(CO2Data))

class SensorDataPacker
{
public:
    // shared instance
    static SensorDataPacker * sharedInstance();

    // init
    void init();

    // add sensor obj ref
    void setTempHumidSensor(SHT3xSensor *sensor);
    void setPmSensor(PMSensor *sensor);
    void setOrientationSensor(OrientationSensor *sensor);
    void setCO2Sensor(CO2Sensor *sensor);

    // sensor capability
    uint32_t sensorCapability() { return _sensorCapability; }

    // get data
    const uint8_t * dataBlock(size_t &size);
    const char*     dataJsonString(size_t &size);

public:
    SensorDataPacker();

protected:
    SHT3xSensor         *_thSensor;
    PMSensor            *_pmSensor;
    CO2Sensor           *_co2Sensor;
    OrientationSensor   *_orientationSensor;
    uint32_t             _sensorCapability;
    uint8_t              _dataBlockBuf[BUF_SIZE];
};

#endif // _SENSOR_DATA_PACKER_H
