/*
 * SensorDataPacker: for package different sensors' data into single block
 * Copyright (c) 2016 Shenghua Su
 *
 */

#ifndef _SENSOR_DATA_PACKER_H
#define _SENSOR_DATA_PACKER_H

#include <stdint.h>
#include "PMSensor.h"
#include "OrientationSensor.h"

#if SENSOR_TYPE >= PMS5003
	#define BUF_SIZE_1  sizeof(PMData)
	#define BUF_SIZE    BUF_SIZE_1
#endif
#if SENSOR_TYPE >= PMS5003S
	#define BUF_SIZE_2 (BUF_SIZE_1 + sizeof(HchoData))
	#undef  BUF_SIZE
	#define BUF_SIZE    BUF_SIZE_2
#endif
#if SENSOR_TYPE >= PMS5003ST
	#define BUF_SIZE_3 (BUF_SIZE_2 + sizeof(TempHumidData))
	#undef  BUF_SIZE
	#define BUF_SIZE    BUF_SIZE_3
#endif

class SensorDataPacker
{
public:
	// shared instance
    static SensorDataPacker * sharedInstance();

   	// add sensor obj ref
    void setPmSensor(PMSensor *sensor);
    void setOrientationSensor(OrientationSensor *sensor);
    void setCO2Sensor();

    // sensor capability
    uint32_t sensorCapability() { return _sensorCapability; }

    // get data
    const uint8_t * dataBlock(size_t &size);
    const char*     dataJsonString(size_t &size);

public:
	SensorDataPacker();

protected:
    PMSensor            *_pmSensor;
    OrientationSensor   *_orientationSensor;
    uint32_t             _sensorCapability;
    uint8_t              _dataBlockBuf[BUF_SIZE];
};

#endif // _SENSOR_DATA_PACKER_H
