/*
 * DisplayController: manage content display on device
 * Copyright (c) 2017 Shenghua Su
 *
 */

#ifndef _DISPLAY_CONTROLLER_H
#define _DISPLAY_CONTROLLER_H

#include "DisplayGFX.h"

enum NetworkState {
  NetworkOff,
  NetworkConnected,
  NetworkNotConnected
};

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

  void setNetworkState(NetworkState state) { _networkState = state; _networkIconNeedUpdate = true; }
  void setTimeUpdate(bool update) { _timeNeedUpdate = true; }
  void setBatteryLevel(uint16_t level) { _batteryLevel = level; _batterNeedUpdate = true; }

protected:
  void updateStatusBar(bool foreUpdateAll = false);

protected:
  NetworkState  _networkState;
  bool          _networkIconNeedUpdate;
  bool          _timeNeedUpdate;
  bool          _batterNeedUpdate;
  uint16_t      _contentOffsetY;
  uint16_t      _batteryLevel;
  DisplayGFX   *_dev;
};

#endif // _DISPLAY_CONTROLLER_H
