/*
 * PowerManager: battery boost and charge management
 * Copyright (c) 2017 Shenghua Su
 *
 */

#ifndef _POWER_MANAGER_H
#define _POWER_MANAGER_H

#include "I2c.h"

class PowerManager
{
public:
  // types
  typedef enum {
    NotCharging          = 0,
    PreCharge            = 1,
    FastCharge           = 2,
    ChargeTermination    = 3
  } ChargeStatus;

public:
  // constructor
  PowerManager();

  void init();
  ChargeStatus chargeStatus(bool readCache = false);
  bool powerGood(bool readCache = false);  
  void powerOff();
  void setChargeCurrent();

protected:
};

#endif // end of _POWER_MANAGER_H
