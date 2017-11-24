/*
 * InputMonitor: touch pad and button press monitor
 * Copyright (c) 2017 Shenghua Su
 *
 */

#ifndef _INPUT_MONITOR_H
#define _INPUT_MONITOR_H

#include "driver/gpio.h"

#define INPUT_EVENT_PWR_BTN_SHORT_RELEASE     1
#define INPUT_EVENT_PWR_BTN_LONG_PRESS        2
#define INPUT_EVENT_USR_BTN_SHORT_RELEASE     3
#define INPUT_EVENT_USR_BTN_MEDIUM_RELEASE    4
#define INPUT_EVENT_USR_BTN_XLONG_PRESS       5
#define INPUT_EVENT_PWR_CHIP_INT              6
#define INPUT_EVENT_MB_TEMP_CALIBRATE         7

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
