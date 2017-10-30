/*
 * Buzzer: controlled by esp PWM
 * Copyright (c) 2017 Shenghua Su
 *
 */

#include "Buzzer.h"
#include "AppLog.h"

#define BUZZER_PWM_UINT      MCPWM_UNIT_0
#define BUZZER_PWM_TIMER     MCPWM_TIMER_0
#define BUZZER_PWM_IO_SIGNAL MCPWM0A
#define BUZZER_PWM_OPERATOR  MCPWM_OPR_A
#define BUZZER_PWM_GPIP_PIN  12
#define BUZZER_PWM_FREQUENCY 4000

Buzzer::Buzzer()
: _initStarted(false)
, _started(false)
{}

void Buzzer::init()
{
  APP_LOGI("[Buzzer]", "init alarm with PWM uint %d, gpio %d", BUZZER_PWM_IO_SIGNAL, BUZZER_PWM_GPIP_PIN);
  mcpwm_gpio_init(BUZZER_PWM_UINT, BUZZER_PWM_IO_SIGNAL, (gpio_num_t)BUZZER_PWM_GPIP_PIN);
  gpio_pulldown_en((gpio_num_t)BUZZER_PWM_GPIP_PIN);

  _pwmConfig.frequency = BUZZER_PWM_FREQUENCY;
  _pwmConfig.cmpr_a = 5.0;    // duty cycle of PWMxA = 50.0%
  _pwmConfig.cmpr_b = 5.0;    // duty cycle of PWMxb = 50.0%
  _pwmConfig.counter_mode = MCPWM_UP_COUNTER;
  _pwmConfig.duty_mode = MCPWM_DUTY_MODE_0;
}

void Buzzer::_initStart()
{
  mcpwm_init(BUZZER_PWM_UINT, BUZZER_PWM_TIMER, &_pwmConfig);   //Configure PWM0A & PWM0B with above settings
  _initStarted = true;
}

void Buzzer::start()
{
  if (!_started) {
  	_initStart();
    // if (!_initStarted) _initStart();
    // else mcpwm_start(BUZZER_PWM_UINT, BUZZER_PWM_TIMER);
    _started = true;
  }
}

void Buzzer::stop()
{
  if (_started) {
    mcpwm_stop(BUZZER_PWM_UINT, BUZZER_PWM_TIMER);
    mcpwm_set_signal_low(BUZZER_PWM_UINT, BUZZER_PWM_TIMER, BUZZER_PWM_OPERATOR);
    // gpio_set_level((gpio_num_t)BUZZER_PWM_GPIP_PIN, 0);
    _started = false;
  }
}