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

  // init and set default
  void init();
  void applyDefaultSettings();

  // poll tick called within task
  bool batteryLevelPollTick();

  // communication
  bool powerOff();
  bool powerGood(bool readCache = false);
  float batteryLevel();

  ChargeStatus chargeStatus(bool readCache = false);

protected:
  uint8_t _chargeCurrentReg();
  bool _setChargeCurrent();
};

#endif // end of _POWER_MANAGER_H
