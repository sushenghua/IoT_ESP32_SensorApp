/*
 * DisplayController: manage content display on device
 * Copyright (c) 2017 Shenghua Su
 *
 */

#include "DisplayController.h"
#include "SNTP.h"

static const uint8_t wifiIcon [] = { // 24 x 24
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0xff, 0x80, 0x07, 0xff, 0xe0, 0x1f, 
0xff, 0xf8, 0x3e, 0x00, 0x7c, 0x78, 0x3c, 0x1e, 0x71, 0xff, 0x8e, 0x03, 0xff, 0xc0, 0x0f, 0xc3, 
0xf0, 0x0f, 0x00, 0xf0, 0x0c, 0x7e, 0x30, 0x00, 0xff, 0x00, 0x01, 0xff, 0x80, 0x01, 0xc3, 0x80, 
0x00, 0x00, 0x00, 0x00, 0x18, 0x00, 0x00, 0x3c, 0x00, 0x00, 0x18, 0x00, 0x00, 0x00, 0x00, 0x00, 
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };

#define WHITE_COLOR 0xCE79

static DisplayController *_activeDisplayController = NULL;

#define DEFAULT_STATUS_BAR_HEIGHT 24

DisplayController * DisplayController::activeInstance()
{
  return _activeDisplayController;
}

DisplayController::DisplayController(DisplayGFX *dev)
: _showWifi(true)
, _showTime(true)
, _showBattery(true)
, _contentOffsetY(DEFAULT_STATUS_BAR_HEIGHT + 15)
, _dev(dev)
{
  _activeDisplayController = this;
}

static char strftime_buf[8];

void DisplayController::displayStatusBar()
{
  if (_showWifi) _dev->drawBitmap(2, 2, wifiIcon, 24, 24, WHITE_COLOR);
  if (_showTime) {
    _dev->setCursor(85, 8);
    _dev->setTextColor(WHITE_COLOR, RGB565_BLACK);
    SNTP::setTimezone("CST-8CDT-9,M4.2.0/2,M9.2.0/3");
    strftime(strftime_buf, sizeof(strftime_buf), "%H:%M", &SNTP::timeInfo(SNTP::timeNow()));
    _dev->write(strftime_buf);
  }
  if (_showBattery) {

  }
}
