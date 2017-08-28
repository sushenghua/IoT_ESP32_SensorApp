/*
 * LedController: Led pwm signal control
 * Copyright (c) 2017 Shenghua Su
 *
 */

#ifndef _LED_CONTROLLER_H
#define _LED_CONTROLLER_H

#include "driver/ledc.h"

class LedController
{
public:
  // constructor
  LedController();

  // init
  void init(int            gpio,
            uint32_t       duty      = 0,
            uint32_t       frequency = 5000,
            ledc_mode_t    mode      = LEDC_HIGH_SPEED_MODE,
            ledc_timer_t   timer     = LEDC_TIMER_0,
            ledc_channel_t channel   = LEDC_CHANNEL_0);
  void deinit();

  // communication
  void setDuty(uint32_t targetDuty);
  void fadeToDuty(uint32_t targetDuty, int fadeDuration = 500);
  void stop(uint32_t idleLevel = 0);

protected:
  bool                   _inited;
  ledc_timer_config_t    _timerConfig;
  ledc_channel_config_t  _channelConfig;
};

#endif // end of _LED_CONTROLLER_H
