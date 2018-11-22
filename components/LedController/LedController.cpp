/*
 * LedController: Led pwm signal control
 * Copyright (c) 2017 Shenghua Su
 *
 */

#include "LedController.h"

LedController::LedController()
: _inited(false)
{}

void LedController::init(int            gpio,
                         uint32_t       duty,
                         uint32_t       frequency,
                         ledc_mode_t    mode,
                         ledc_timer_t   timer,
                         ledc_channel_t channel)
{
  if (_inited) return;

  // config timer
  _timerConfig.timer_num = timer;
  _timerConfig.duty_resolution = LEDC_TIMER_10_BIT;
  _timerConfig.freq_hz = frequency;
  _timerConfig.speed_mode = mode;
  ledc_timer_config(&_timerConfig);

  // config channel
  _channelConfig.channel = channel;
  _channelConfig.gpio_num = gpio;
  _channelConfig.speed_mode = mode;
  _channelConfig.timer_sel = timer;
  _channelConfig.duty = duty;                    // 0 ~ (2**duty_resolution - 1)
  _channelConfig.intr_type = LEDC_INTR_FADE_END; // Enable LEDC interrupt
  ledc_channel_config(&_channelConfig);

  //initialize fade service.
  ledc_fade_func_install(0);

  _inited = true;
}

void LedController::deinit()
{
  if (_inited) {
    ledc_fade_func_uninstall();
    _inited = false;
  }
}

void LedController::setDuty(uint32_t targetDuty)
{
  ledc_set_duty(_timerConfig.speed_mode, _channelConfig.channel, targetDuty);
  ledc_update_duty(_timerConfig.speed_mode, _channelConfig.channel);
}

void LedController::fadeToDuty(uint32_t targetDuty, int fadeDuration)
{
  ledc_set_fade_with_time(_timerConfig.speed_mode, _channelConfig.channel, targetDuty, fadeDuration);
  ledc_fade_start(_timerConfig.speed_mode, _channelConfig.channel, LEDC_FADE_NO_WAIT);
}

void LedController::stop(uint32_t idleLevel)
{
  ledc_stop(_timerConfig.speed_mode, _channelConfig.channel, idleLevel);
}
