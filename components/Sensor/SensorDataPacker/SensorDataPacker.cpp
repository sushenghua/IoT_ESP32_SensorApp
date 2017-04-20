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

char _dataStringBuf[1024];

const char* SensorDataPacker::dataString(size_t &size)
{
	size_t packCount = 0;
    if (_pmSensor) {
        #if SENSOR_TYPE >= PMS5003
            PMData& pmData = _pmSensor->pmData();
            sprintf(_dataStringBuf + packCount, "pm1.0: %.1f, pm2.5: %.1f (index: %d), pm10: %.1f (index: %d)",
                    pmData.pm1d0, pmData.pm2d5, pmData.aqiPm2d5US, pmData.pm10, pmData.aqiPm10US);
            packCount += strlen(_dataStringBuf + packCount);
        #endif
        #if SENSOR_TYPE >= PMS5003S
            sprintf(_dataStringBuf + packCount, ", hcho: %.2f", _pmSensor->hchoData().hcho);
            packCount += strlen(_dataStringBuf + packCount);
        #endif
        #if SENSOR_TYPE >= PMS5003ST
        #endif
    }

    size = packCount;
    return _dataStringBuf;
}