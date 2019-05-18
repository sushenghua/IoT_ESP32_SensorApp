/*
 * TSL2561 Wrap communicate with TSL2561 light-to-digital sensor
 * Copyright (c) 2017 Shenghua Su
 *
 */

#ifndef _TSL2561_H
#define _TSL2561_H

#include <stdint.h>
#include "SensorDisplayController.h"
#include "LuminosityData.h"

class TSL2561
{
public:
  // constructor
  TSL2561();

  // init and display delegate
  void init(bool checkDeviceReady=true);
  void setDisplayDelegate(SensorDisplayController *dc) { _dc = dc; }

  // cached values
  LuminosityData & luminosityData() { return _luminosityData; }

  // sample
  void sampleData();

protected:
  // value cache from sensor
  LuminosityData            _luminosityData;

  // display delagate
  SensorDisplayController  *_dc;
};

#endif // _TSL2561_H
