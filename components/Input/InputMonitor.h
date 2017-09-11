/*
 * InputMonitor: touch pad and button press monitor
 * Copyright (c) 2017 Shenghua Su
 *
 */

#ifndef _INPUT_MONITOR_H
#define _INPUT_MONITOR_H

#include "driver/gpio.h"

class InputMonitor
{
public:
  static InputMonitor * instance();

public:
  // constructor
  InputMonitor();

  void init();
  void deinit();
  void tick();
  void setTaskPaused(bool paused = true);
  bool taskPaused();

protected:
  bool            _inited;
  gpio_config_t   _gpioConfig;
};

#endif // end of _INPUT_MONITOR_H
