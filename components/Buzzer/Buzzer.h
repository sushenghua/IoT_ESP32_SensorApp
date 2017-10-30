/*
 * Buzzer: controlled by esp PWM
 * Copyright (c) 2017 Shenghua Su
 *
 */

#ifndef _BUZZER_H
#define _BUZZER_H

#include <string.h>
#include "driver/mcpwm.h"

class Buzzer
{
public:
  // constructor
  Buzzer();
  
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

#endif // _BUZZER_H
