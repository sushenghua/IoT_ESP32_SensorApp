/*
 * SensorDataPacker: for package different sensors' data into single block
 * Copyright (c) 2016 Shenghua Su
 *
 */

#include "SensorDataPacker.h"
#include <string.h>

static SensorDataPacker _sharedSensorDataPacker;

SensorDataPacker * SensorDataPacker::sharedInstance()
{
	return &_sharedSensorDataPacker;
}

SensorDataPacker::SensorDataPacker()
: _pmSensor(NULL)
{}

const uint8_t* SensorDataPacker::dataBlock(size_t &size)
{
    size_t packCount = 0;
    size_t sz;

    if (_pmSensor) {
        #if SENSOR_TYPE >= PMS5003
            sz = sizeof(_pmSensor->pmData());
            memcpy(_dataBlockBuf + packCount, &(_pmSensor->pmData()), sz);
            packCount += sz;
        #endif
        #if SENSOR_TYPE >= PMS5003S
            sz = sizeof(_pmSensor->hchoData());
            memcpy(_dataBlockBuf + packCount, &(_pmSensor->hchoData()), sz);
            packCount += sz;
        #endif
        #if SENSOR_TYPE >= PMS5003ST
            sz = sizeof(_pmSensor->tempHumidData());
            memcpy(_dataBlockBuf + packCount, &(_pmSensor->tempHumidData()), sz);
            packCount += sz;
        #endif
    }

    size = packCount;
    return _dataBlockBuf;
}

char _tmpBuf[1024];

const char* SensorDataPacker::dataString(size_t &size)
{
	size_t packCount = 0;
    if (_pmSensor) {
        #if SENSOR_TYPE >= PMS5003
            sprintf(_tmpBuf + packCount, "pm 2.5: %.1f", _pmSensor->pmData().pm2d5);
            packCount += strlen(_tmpBuf + packCount);
        #endif
        #if SENSOR_TYPE >= PMS5003S
            sprintf(_tmpBuf + packCount, ", hcho: %.2f", _pmSensor->hchoData().hcho);
            packCount += strlen(_tmpBuf + packCount);
        #endif
        #if SENSOR_TYPE >= PMS5003ST
        #endif
    }

    size = packCount;
    return _tmpBuf;
}