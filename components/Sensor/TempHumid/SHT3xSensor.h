/*
 * SHT3xSensor Wrap communicate with SHT3xSensor temperature and humidity sensor
 * Copyright (c) 2017 Shenghua Su
 *
 */

#ifndef _SHT3X_SENSOR_H
#define _SHT3X_SENSOR_H

#include "SensorDisplayController.h"
#include "TempHumidData.h"

#define PM_RX_BUF_CAPACITY      PM_RX_PROTOCOL_MAX_LENGTH

class SHT3xSensor
{
public:
  // constructor
  SHT3xSensor();

  // init and display delegate
  void init();
  void setDisplayDelegate(SensorDisplayController *dc) { _dc = dc; }

  // cached values
  TempHumidData & tempHumidData() { return _tempHumidData; }

  // sample
  void sampleData();

protected:
  // value cache from sensor
  TempHumidData   _tempHumidData;

  // display delagate
  SensorDisplayController  *_dc;
};

#endif // _SHT3X_SENSOR_H
