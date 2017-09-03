/*
 * TSL2561 Wrap communicate with TSL2561 light-to-digital sensor
 * Copyright (c) 2017 Shenghua Su
 *
 */

#ifndef _TSL2561_H
#define _TSL2561_H

#include <stdint.h>

class TSL2561
{
public:
  // constructor
  TSL2561();

  // init and display delegate
  void init();

  // cached values
  uint32_t & luminosity() { return _luminosity; }

  // sample
  void sampleData();

protected:
  // value cache from sensor
  uint32_t   _luminosity;
};

#endif // _TSL2561_H
