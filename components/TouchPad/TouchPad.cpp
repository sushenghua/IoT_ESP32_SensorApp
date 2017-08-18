/*
 * TouchPad: Wrap the esp touch_pad interface
 * Copyright (c) 2017 Shenghua Su
 *
 */

#include "TouchPad.h"
#include "AppLog.h"

TouchPad::TouchPad(touch_pad_t padNum)
: _padNum(padNum)
, _inited(false)
{}

void TouchPad::init()
{
  if (!_inited) {
    touch_pad_init();
    touch_pad_config(_padNum, 50);
    _inited = true;
  }
}

void TouchPad::deinit()
{
  if (_inited) {
    touch_pad_deinit();
    _inited = false;
  }
}

uint16_t TouchPad::readValue()
{
  touch_pad_read(_padNum, &_value);
  APP_LOGC("[TP]", "value: %d", _value);
  return _value;
}

bool TouchPad::touched()
{
  return _value > 0;
}
