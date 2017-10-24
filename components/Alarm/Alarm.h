/*
 * Alarm: controlled by esp PWM
 * Copyright (c) 2017 Shenghua Su
 *
 */

#ifndef _ALARM_H
#define _ALARM_H

#include <string.h>
#include "driver/mcpwm.h"

class Alarm
{
public:
  // constructor
  Alarm();
  
  // init
  void init();

  void start();
  void stop();

protected:
	void _initStart();

protected:
	bool            _initStarted;
	bool            _started;
    mcpwm_config_t  _pwmConfig;
};

#endif // _ALARM_H
