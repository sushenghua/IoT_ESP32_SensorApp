/*
 * SensorDataPacker: for package different sensors' data into single block
 * Copyright (c) 2016 Shenghua Su
 *
 */

#include "SensorDataPacker.h"
#include <string.h>

#define PM_CAPABILITY_MASK             0x00000001
#define HCHO_CAPABILITY_MASK           0x00000002
#define TEMP_HUMID_CAPABILITY_MASK     0x00000004
#define CO2_CAPABILITY_MASK            0x00000008
#define ORIENTATION_CAPABILITY_MASK    0x00000010

static SensorDataPacker _sharedSensorDataPacker;

SensorDataPacker * SensorDataPacker::sharedInstance()
{
	return &_sharedSensorDataPacker;
}

SensorDataPacker::SensorDataPacker()
: _pmSensor(NULL)
, _sensorCapability(0)
{}

void SensorDataPacker::setPmSensor(PMSensor *sensor)
{
    _pmSensor = sensor;
    if (_pmSensor) {
        #if SENSOR_TYPE >= PMS5003
            _sensorCapability |= PM_CAPABILITY_MASK;
        #endif
        #if SENSOR_TYPE >= PMS5003S
            _sensorCapability |= HCHO_CAPABILITY_MASK;
        #endif
        #if SENSOR_TYPE >= PMS5003ST
            _sensorCapability |= TEMP_HUMID_CAPABILITY_MASK;
        #endif
        
    }
}

void SensorDataPacker::setOrientationSensor(OrientationSensor *sensor)
{
    _orientationSensor = sensor;
    if (_orientationSensor) _sensorCapability |= ORIENTATION_CAPABILITY_MASK;
}

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

const char* SensorDataPacker::dataJsonString(size_t &size)
{
    size_t packCount = 0;
    if (_pmSensor) {
        sprintf(_dataStringBuf + packCount, "{\"ret\":{");
        packCount += strlen(_dataStringBuf + packCount);
        #if SENSOR_TYPE >= PMS5003
            PMData& pmData = _pmSensor->pmData();
            sprintf(_dataStringBuf + packCount,
                    "\"pm1.0\":%.1f,\"pm2.5\":%.1f,\"pm2.5us\":%d,\"pm10\":%.1f,\"pm10us\":%d",
                     pmData.pm1d0, pmData.pm2d5, pmData.aqiPm2d5US, pmData.pm10, pmData.aqiPm10US);
            packCount += strlen(_dataStringBuf + packCount);
        #endif
        #if SENSOR_TYPE >= PMS5003S
            sprintf(_dataStringBuf + packCount, ",\"hcho\":%.2f", _pmSensor->hchoData().hcho);
            packCount += strlen(_dataStringBuf + packCount);
        #endif
        #if SENSOR_TYPE >= PMS5003ST
        #endif
        sprintf(_dataStringBuf + packCount, "}}");
        packCount += strlen(_dataStringBuf + packCount);
    }

    size = packCount;
    return _dataStringBuf;
}
