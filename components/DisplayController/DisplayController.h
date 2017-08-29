/*
 * DisplayController: manage content display on device
 * Copyright (c) 2017 Shenghua Su
 *
 */

#ifndef _DISPLAY_CONTROLLER_H
#define _DISPLAY_CONTROLLER_H

#include "DisplayGFX.h"

class DisplayController
{
public:
  static DisplayController * activeInstance();

  DisplayController(DisplayGFX *dev);

public:
  virtual void init();
  virtual void tick() {};
  virtual void update();

  void reset() { _dev->reset(); }
  void turnOn(bool on) { _dev->turnOn(on); }

  void setWifiConnected(bool connected) { _wifiConnected = connected; _wifiIconNeedUpdate = true; }
  void setTimeUpdate(bool update) { _timeNeedUpdate = true; }
  void setBatteryLevel(uint16_t level) { _batteryLevel = level; _batterNeedUpdate = true; }

protected:
  void updateStatusBar(bool foreUpdateAll = false);

protected:
  bool        _wifiIconNeedUpdate;
  bool        _wifiConnected;
  bool        _timeNeedUpdate;
  bool        _batterNeedUpdate;
  uint16_t    _contentOffsetY;
  uint16_t    _batteryLevel;
  DisplayGFX *_dev;
};

#endif // _DISPLAY_CONTROLLER_H
