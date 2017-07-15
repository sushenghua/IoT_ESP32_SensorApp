/*
 * SensorDataPacker: for package different sensors' data into single block
 * Copyright (c) 2016 Shenghua Su
 *
 */

#include "SensorDataPacker.h"
#include <string.h>
#include "System.h"

static SensorDataPacker _sharedSensorDataPacker;

SensorDataPacker * SensorDataPacker::sharedInstance()
{
  return &_sharedSensorDataPacker;
}

SensorDataPacker::SensorDataPacker()
: _pmSensor(NULL)
, _sensorCapability(System::instance()->devCapability())
{}

void SensorDataPacker::setPmSensor(PMSensor *sensor)
{
  _pmSensor = sensor;
}

void SensorDataPacker::setCO2Sensor(CO2Sensor *sensor)
{
  _co2Sensor = sensor;
}

void SensorDataPacker::setOrientationSensor(OrientationSensor *sensor)
{
  _orientationSensor = sensor;
}

const uint8_t* SensorDataPacker::dataBlock(size_t &size)
{
  size_t packCount = 0;
  size_t sz;

  if (_pmSensor) {
    if (_sensorCapability & PM_CAPABILITY_MASK) {
      sz = sizeof(_pmSensor->pmData());
      memcpy(_dataBlockBuf + packCount, &(_pmSensor->pmData()), sz);
      packCount += sz;
    }

    if (_sensorCapability & HCHO_CAPABILITY_MASK) {
      sz = sizeof(_pmSensor->hchoData());
      memcpy(_dataBlockBuf + packCount, &(_pmSensor->hchoData()), sz);
      packCount += sz;
    }

    if (_sensorCapability & TEMP_HUMID_CAPABILITY_MASK) {
      sz = sizeof(_pmSensor->tempHumidData());
      memcpy(_dataBlockBuf + packCount, &(_pmSensor->tempHumidData()), sz);
      packCount += sz;
    }
  }

  if (_co2Sensor && (_sensorCapability & CO2_CAPABILITY_MASK)) {
    sz = sizeof(_co2Sensor->co2Data());
    memcpy(_dataBlockBuf + packCount, &(_co2Sensor->co2Data()), sz);
    packCount += sz;
  }

  size = packCount;
  return _dataBlockBuf;
}

char _dataStringBuf[1024];

const char* SensorDataPacker::dataJsonString(size_t &size)
{
  size_t packCount = 0;
  if (_pmSensor || _co2Sensor) {
    sprintf(_dataStringBuf + packCount, "{\"ret\":{");
    packCount += strlen(_dataStringBuf + packCount);

    if (_pmSensor) {
      if (_sensorCapability & PM_CAPABILITY_MASK) {
        PMData& pm = _pmSensor->pmData();
        sprintf(_dataStringBuf + packCount,
            "\"pm1.0\":%.1f,\"pm2.5\":%.1f,\"pm10\":%.1f,\"pm2.5us\":%d,\"pm2.5uslvl\":%d,"\
            "\"pm2.5cn\":%d,\"pm2.5cnlvl\":%d,\"pm10us\":%d,\"pm10uslvl\":%d",
            pm.pm1d0, pm.pm2d5, pm.pm10, pm.aqiPm2d5US, pm.levelPm2d5US,
            pm.aqiPm2d5CN, pm.levelPm2d5CN, pm.aqiPm10US, pm.levelPm10US);
        packCount += strlen(_dataStringBuf + packCount);
      }
      if (_sensorCapability & HCHO_CAPABILITY_MASK) {
        HchoData hcho = _pmSensor->hchoData();
        sprintf(_dataStringBuf + packCount, ",\"hcho\":%.2f,\"hcholvl\":%d", hcho.hcho, hcho.level);
        packCount += strlen(_dataStringBuf + packCount);
      }
      if (_sensorCapability & TEMP_HUMID_CAPABILITY_MASK) {
        TempHumidData th = _pmSensor->tempHumidData();
        sprintf(_dataStringBuf + packCount,
            ",\"temp\":%.1f,\"templvl\":%d,\"humid\":%.1f,\"humidlvl\":%d",
            th.temp, th.levelTemp, th.humid, th.levelHumid);
        packCount += strlen(_dataStringBuf + packCount);
      }

    }
    if (_co2Sensor && (_sensorCapability & CO2_CAPABILITY_MASK)) {
      CO2Data co2Data = _co2Sensor->co2Data();
      sprintf(_dataStringBuf + packCount,
          ",\"co2\":%d,\"co2lvl\":%d", (int)co2Data.co2, co2Data.level);
      packCount += strlen(_dataStringBuf + packCount);
    }
    sprintf(_dataStringBuf + packCount, "}}");
    packCount += strlen(_dataStringBuf + packCount);
  }

  size = packCount;
  return _dataStringBuf;
}
