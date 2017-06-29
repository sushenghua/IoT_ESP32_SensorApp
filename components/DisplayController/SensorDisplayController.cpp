/*
 * Display: Display snsor content on screen
 * Copyright (c) 2017 Shenghua Su
 *
 */

#include "SensorDisplayController.h"
#include "SensorConfig.h"
#include "HealthyStandard.h"
#include "AppLog.h"

SensorDisplayController::SensorDisplayController(DisplayGFX *dev)
: DisplayController(dev)
, _contentNeedUpdate(true)
, _rotationNeedUpdate(false)
, _devUpdateRotationInProgress(false)
, _mainItemCount(0)
, _subItemCount(0)
{}

void SensorDisplayController::init()
{
  APP_LOGI("[SensorDC]", "init");
  _dev->init();
  _dev->fillScreen(RGB565_BLACK);

  _dev->setTextSize(2);

  _rotation = _dev->rotation();
  _lastRotation = _rotation;
  _initDisplayItems();
}

void SensorDisplayController::tick()
{
}

void SensorDisplayController::setRotation(uint8_t rotation)
{
  if (_devUpdateRotationInProgress) return;

  if (_rotation != rotation) {
    // if restore to previous rotation before device rotation update,
    // then no update needed
    if (_rotationNeedUpdate && _lastRotation == rotation) {
      _rotation = rotation;
      _rotationNeedUpdate = false;
    }
    else {
      _lastRotation = _rotation;
      _rotation = rotation;
      _rotationNeedUpdate = true;
      _contentNeedUpdate = true;
    }
  }
}

void SensorDisplayController::setMpu6050(float r, float p, float y)
{
  roll = r; pitch = p; yaw = y;
  _contentNeedUpdate = true;
}


void SensorDisplayController::setPmData(PMData &pmData, bool update)
{
  // value update
  _pm1d0 = pmData.pm1d0;
  _pm2d5 = pmData.pm2d5;
  _pm10 = pmData.pm10;
  _aqiPm2d5US = pmData.aqiPm2d5US;
  _aqiPm10US = pmData.aqiPm10US;

  // color update
  _pm2d5Color = HS::colorForAirLevel(pmData.levelPm2d5US);
  _pm10Color = HS::colorForAirLevel(pmData.levelPm10US);

  if (update) _contentNeedUpdate = true;
}

void SensorDisplayController::setHchoData(HchoData &hchoData, bool update)
{
  // value update
  _hcho = hchoData.hcho;

  // color update
  _hchoColor = HS::colorForHchoLevel(hchoData.level);

  if (update) _contentNeedUpdate = true;
}

void SensorDisplayController::setTempHumidData(TempHumidData &tempHumidData, bool update)
{
  // value update
  _temp = tempHumidData.temp;
  _humid = tempHumidData.humid;

  // color update
  _tempColor = HS::colorForTempLevel(tempHumidData.levelTemp);
  _humidColor = HS::colorForHumidLevel(tempHumidData.levelHumid);

  if (update) _contentNeedUpdate = true;
}

void SensorDisplayController::setCO2Data(CO2Data &co2Data, bool update)
{
  // value update
  _co2 = co2Data.co2;

  // color update
  _co2Color = HS::colorForCO2Level(co2Data.level);

  if (update) _contentNeedUpdate = true;
}

#include "Bitmap.h"

void SensorDisplayController::_renderScreenBg()
{
  switch(_rotation) {
    case DISPLAY_ROTATION_CW_0:
      // _dev->fillCircle(120, 120, 50, _pm2d5Color);
      // _dev->drawRoundRect(PM_RECT_OFFSET_X, _contentOffsetY + PM_RECT_OFFSET_Y,
      //                     100, 100, 10, _pm2d5Color);
      break;
    case DISPLAY_ROTATION_CW_90:
      break;
    case DISPLAY_ROTATION_CW_180:
      break;
    case DISPLAY_ROTATION_CW_270:
      break;
  }
}

#define ITEM_TITLE_OFFSET_X     4
#define ITEM_TITLE_OFFSET_Y     9

#define ITEM_TITLE_CHAR_W       80
#define ITEM_TITLE_CHAR_H       20

#define ITEM_RECT_RADIUS        10
#define ITEM_RECT_OFFSET_X      100
#define ITEM_RECT_HEIGHT        40
#define ITEM_RECT_WIDTH         140

#define ITEM_VALUE_OFFSET_X     10
#define ITEM_VALUE_OFFSET_Y     9

#define ITEM_LEVEL_OFFSET_X     86
#define ITEM_LEVEL_OFFSET_Y     8
#define ITEM_LEVEL_CHAR_W       48
#define ITEM_LEVEL_CHAR_H       24


#define SUBITEM_TITLE_OFFSET_X  4
#define SUBITEM_TITLE_OFFSET_Y  6

#define SUBITEM_TITLE_CHAR_W    40
#define SUBITEM_TITLE_CHAR_H    20

#define SUBITEM_RECT_RADIUS     5
#define SUBITEM_RECT_OFFSET_X   51
#define SUBITEM_RECT_HEIGHT     30
#define SUBITEM_RECT_WIDTH      70

#define SUBITEM_VALUE_OFFSET_X  0
#define SUBITEM_VALUE_OFFSET_Y  9

#define SUBITEM_VALUE_UNIT_OFFSET_X 53

uint16_t itemOffsetY = 0;
uint16_t subItemOffsetY = 0;
uint16_t rowDivHeight = 0;
uint16_t subItemRowDivWidth = 0;

void SensorDisplayController::_initDisplayItems()
{
    itemOffsetY = 0;
    subItemOffsetY = 0;
    rowDivHeight = 0;
    subItemRowDivWidth = 0;
    _mainItemCount = 0;
    _subItemCount = 0;

#if PM_SENSOR_TYPE >= PMS5003
    _displayMainItems[_mainItemCount++] = PM;
#endif
#if PM_SENSOR_TYPE >= PMS5003S
    _displayMainItems[_mainItemCount++] = HCHO;
#endif
#if PM_SENSOR_TYPE >= PMS5003ST
    _displaySubItems[_subItemCount++] = TEMP;
    _displaySubItems[_subItemCount++] = HUMID;
#endif
#ifdef CO2_SENSOR
    _displayMainItems[_mainItemCount++] = CO2;
#endif

  float rowCount = _subItemCount > 0 ? _mainItemCount + 1 : _mainItemCount;
  rowDivHeight = (uint16_t) ( (_dev->height() - _contentOffsetY) / rowCount );

  itemOffsetY = (rowDivHeight - ITEM_RECT_HEIGHT) / 2;
  if (itemOffsetY > rowDivHeight) itemOffsetY = 0;

  subItemOffsetY = (rowDivHeight - SUBITEM_RECT_HEIGHT) / 2;
  if (subItemOffsetY > rowDivHeight) subItemOffsetY = 0;

  subItemRowDivWidth = _dev->width() / 2;
}

void SensorDisplayController::_renderMainScreenMainItem(uint8_t refIndex, uint8_t posIndex,
                                                        const char *value, uint8_t lvl, uint16_t color)
{
  uint16_t baseOffsetY = _contentOffsetY + rowDivHeight * posIndex + itemOffsetY;

  // title
  _dev->drawBitmap(ITEM_TITLE_OFFSET_X, baseOffsetY + ITEM_TITLE_OFFSET_Y,
                   TITLE[refIndex], ITEM_TITLE_CHAR_W, ITEM_TITLE_CHAR_H, RGB565_WEAKWHITE);
  // round corner reactange
  _dev->drawRoundRect(ITEM_RECT_OFFSET_X, baseOffsetY,
                      ITEM_RECT_WIDTH, ITEM_RECT_HEIGHT, ITEM_RECT_RADIUS, color);
  // value
  _dev->setCursor(ITEM_RECT_OFFSET_X + ITEM_VALUE_OFFSET_X, baseOffsetY + ITEM_VALUE_OFFSET_Y);
  _dev->setTextColor(color, RGB565_BLACK);
  _dev->write(value);

  // level chars
  _dev->drawBitmap(ITEM_RECT_OFFSET_X + ITEM_LEVEL_OFFSET_X, baseOffsetY + ITEM_LEVEL_OFFSET_Y,
                   LEVEL[lvl], ITEM_LEVEL_CHAR_W, ITEM_LEVEL_CHAR_H, color);
}

const char* unitStr[] = {"", "ug/m3", "ppm", "C", "%"};

void SensorDisplayController::_renderMainScreenSubItem(uint8_t refIndex, uint8_t posIndex,
                                                       const char *value, uint8_t lvl, uint16_t color)
{
  uint16_t baseOffsetY = _contentOffsetY + rowDivHeight * (_mainItemCount +  posIndex / 2) + subItemOffsetY;
  uint16_t baseOffsetX = subItemRowDivWidth * (posIndex % 2);

  // title
  _dev->drawBitmap(baseOffsetX + SUBITEM_TITLE_OFFSET_X, baseOffsetY + SUBITEM_TITLE_OFFSET_Y,
                   TITLE[refIndex], SUBITEM_TITLE_CHAR_W, SUBITEM_TITLE_CHAR_H, RGB565_WEAKWHITE);
  // // round corner reactange
  // _dev->drawRoundRect(baseOffsetX + SUBITEM_RECT_OFFSET_X, baseOffsetY,
  //                     SUBITEM_RECT_WIDTH, SUBITEM_RECT_HEIGHT, SUBITEM_RECT_RADIUS, color);
  // value
  _dev->setCursor(baseOffsetX + SUBITEM_RECT_OFFSET_X + SUBITEM_VALUE_OFFSET_X, baseOffsetY + ITEM_VALUE_OFFSET_Y);
  _dev->setTextColor(color, RGB565_BLACK);
  _dev->write(value);

  // value unit
  _dev->setCursor(baseOffsetX + SUBITEM_RECT_OFFSET_X + SUBITEM_VALUE_UNIT_OFFSET_X,
                  baseOffsetY + SUBITEM_VALUE_OFFSET_Y);
  _dev->write(unitStr[refIndex]);
}

void SensorDisplayController::_renderMainScreen()
{
  bool _needRenderBg = true;
  if (_needRenderBg) {
    // _dev->setTextColor(RGB565_WEAKWHITE, RGB565_BLACK);
    _dev->setTextSize(3);
    for (int i = 0; i < _mainItemCount; ++i) {
      _renderMainScreenMainItem(_displayMainItems[i], i, "0.38", 0, _pm2d5Color);
    }
    _dev->setTextSize(2);
    for (int i = 0; i < _subItemCount; ++i) {
      _renderMainScreenSubItem(_displaySubItems[i], i, "38.5", 0, _pm2d5Color);
    }
  }
}

void SensorDisplayController::_renderCW90Screen()
{
  // APP_LOGE("[SensorDC]", "content update");
  _dev->setCursor(0, _contentOffsetY);
  _dev->setTextSize(2);
  
  _dev->setTextColor(RGB565_CYAN, RGB565_BLACK);
  _dev->write("PM1.0: "); _dev->write(_pm1d0, 1); _dev->writeln();
  
  _dev->setTextColor(_pm2d5Color, RGB565_BLACK);
  _dev->write("PM2.5: "); _dev->write(_pm2d5, 1); _dev->write(" "); _dev->write((long)_aqiPm2d5US);
  _dev->write("  "); _dev->writeln();
  
  _dev->setTextColor(_pm10Color, RGB565_BLACK);
  _dev->write("PM10 : "); _dev->write(_pm10, 1); _dev->write(" "); _dev->write((long)_aqiPm10US);
  _dev->write("  "); _dev->writeln();
  
  //_dev->writeln();
  
  _dev->setTextColor(_hchoColor, RGB565_BLACK);
  _dev->write("HCHO : "); _dev->write(_hcho, 2); _dev->writeln();
  
  _dev->setTextColor(_tempColor, RGB565_BLACK);
  _dev->write("Temp : "); _dev->write(_temp, 1); _dev->writeln();
  
  _dev->setTextColor(_humidColor, RGB565_BLACK);
  _dev->write("Humid: "); _dev->write(_humid, 1); _dev->writeln();
  
  _dev->setTextColor(_co2Color, RGB565_BLACK);
  _dev->write("CO2  : "); _dev->write(_co2, 1); _dev->writeln();
  
  // mpu6050
  _dev->setTextColor(_co2Color, RGB565_BLACK);
  _dev->write("\nroll  : "); _dev->write(roll, 2); _dev->writeln();
  _dev->write("pitch : "); _dev->write(pitch, 2); _dev->writeln();
  _dev->write("yaw   : "); _dev->write(yaw, 2); _dev->writeln();
}

void SensorDisplayController::update()
{
  // update rotation
  if (_rotationNeedUpdate) {
    // APP_LOGE("[SensorDC]", "rotation update");
    _devUpdateRotationInProgress = true;  // lock
    _dev->fillScreen(RGB565_BLACK);
    _dev->setRotation(_rotation);
    _initDisplayItems();
    this->updateStatusBar(true);
    _renderScreenBg();
    _rotationNeedUpdate = false;
    _devUpdateRotationInProgress = false; // unlock
  }
  else {
    this->updateStatusBar();
  }

  // update content
  if (_contentNeedUpdate) {
    switch(_rotation) {
      case DISPLAY_ROTATION_CW_0:
        _renderMainScreen();
        break;
      case DISPLAY_ROTATION_CW_90:
        _renderCW90Screen();
        break;
      case DISPLAY_ROTATION_CW_180:
        break;
      case DISPLAY_ROTATION_CW_270:
        break;
    }
    _contentNeedUpdate = false;
  }
}
