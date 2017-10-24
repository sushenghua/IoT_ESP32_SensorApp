/*
 * Alarm: controlled by esp PWM
 * Copyright (c) 2017 Shenghua Su
 *
 */

#include "Alarm.h"
#include "AppLog.h"

#define ALARM_PWM_UINT      MCPWM_UNIT_0
#define ALARM_PWM_TIMER     MCPWM_TIMER_0
#define ALARM_PWM_IO_SIGNAL MCPWM0A
#define ALARM_PWM_OPERATOR  MCPWM_OPR_A
#define ALARM_PWM_GPIP_PIN  12
#define ALARM_PWM_FREQUENCY 4000

Alarm::Alarm()
: _initStarted(false)
, _started(false)
{}

void Alarm::init()
{
  APP_LOGI("[Alarm]", "init alarm with PWM uint %d, gpio %d", ALARM_PWM_IO_SIGNAL, ALARM_PWM_GPIP_PIN);
  mcpwm_gpio_init(ALARM_PWM_UINT, ALARM_PWM_IO_SIGNAL, (gpio_num_t)ALARM_PWM_GPIP_PIN);
  gpio_pulldown_en((gpio_num_t)ALARM_PWM_GPIP_PIN);

  _pwmConfig.frequency = ALARM_PWM_FREQUENCY;
  _pwmConfig.cmpr_a = 5.0;    // duty cycle of PWMxA = 50.0%
  _pwmConfig.cmpr_b = 5.0;    // duty cycle of PWMxb = 50.0%
  _pwmConfig.counter_mode = MCPWM_UP_COUNTER;
  _pwmConfig.duty_mode = MCPWM_DUTY_MODE_0;
}

void Alarm::_initStart()
{
  mcpwm_init(ALARM_PWM_UINT, ALARM_PWM_TIMER, &_pwmConfig);   //Configure PWM0A & PWM0B with above settings
  _initStarted = true;
}

void Alarm::start()
{
  if (!_started) {
  	_initStart();
    // if (!_initStarted) _initStart();
    // else mcpwm_start(ALARM_PWM_UINT, ALARM_PWM_TIMER);
    _started = true;
  }
}

void Alarm::stop()
{
  if (_started) {
    mcpwm_stop(ALARM_PWM_UINT, ALARM_PWM_TIMER);
    mcpwm_set_signal_low(ALARM_PWM_UINT, ALARM_PWM_TIMER, ALARM_PWM_OPERATOR);
    // gpio_set_level((gpio_num_t)ALARM_PWM_GPIP_PIN, 0);
    _started = false;
  }
}