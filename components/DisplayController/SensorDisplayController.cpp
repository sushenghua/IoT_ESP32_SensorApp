/*
 * Display: Display snsor content on screen
 * Copyright (c) 2017 Shenghua Su
 *
 */

#include "SensorDisplayController.h"
#include "SensorConfig.h"
#include "HealthyStandard.h"
#include "AppLog.h"
#include "Bitmap.h"

SensorDisplayController::SensorDisplayController(DisplayGFX *dev)
: DisplayController(dev)
, _staticContentNeedUpdate(true)
, _dynamicContentNeedUpdate(true)
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
  _layoutScreen();
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
    }
  }
}

void SensorDisplayController::setMpu6050(float r, float p, float y)
{
  roll = r; pitch = p; yaw = y;
  _dynamicContentNeedUpdate = true;
}

void SensorDisplayController::setPmData(PMData &pmData, bool update)
{
  // value update
  _pm1d0 = pmData.pm1d0;
  _pm2d5 = pmData.pm2d5;
  _pm10 = pmData.pm10;
  _aqiPm2d5US = pmData.aqiPm2d5US;
  _aqiPm10US = pmData.aqiPm10US;
  _pm2d5Level = pmData.levelPm2d5US;
  _pm10Level = pmData.levelPm10US;

  // color update
  _pm2d5Color = HS::colorForAirLevel(pmData.levelPm2d5US);
  _pm10Color = HS::colorForAirLevel(pmData.levelPm10US);

  if (update) _dynamicContentNeedUpdate = true;
}

void SensorDisplayController::setHchoData(HchoData &hchoData, bool update)
{
  // value update
  _hcho = hchoData.hcho;
  _hchoLevel = hchoData.level;

  // color update
  _hchoColor = HS::colorForHchoLevel(hchoData.level);

  if (update) _dynamicContentNeedUpdate = true;
}

void SensorDisplayController::setTempHumidData(TempHumidData &tempHumidData, bool update)
{
  // value update
  _temp = tempHumidData.temp;
  _humid = tempHumidData.humid;
  _tempLevel = tempHumidData.levelTemp;
  _humidLevel = tempHumidData.levelHumid;

  // color update
  _tempColor = HS::colorForTempLevel(tempHumidData.levelTemp);
  _humidColor = HS::colorForHumidLevel(tempHumidData.levelHumid);

  if (update) _dynamicContentNeedUpdate = true;
}

void SensorDisplayController::setCO2Data(CO2Data &co2Data, bool update)
{
  // value update
  _co2 = co2Data.co2;
  _co2Level = co2Data.level;

  // color update
  _co2Color = HS::colorForCO2Level(co2Data.level);

  if (update) _dynamicContentNeedUpdate = true;
}

void SensorDisplayController::update()
{
  // update rotation
  if (_rotationNeedUpdate) {
    // APP_LOGE("[SensorDC]", "rotation update");
    _devUpdateRotationInProgress = true;  // lock
    _dev->fillScreen(RGB565_BLACK);
    _dev->setRotation(_rotation);
    _layoutScreen();
    this->updateStatusBar(true);
    _staticContentNeedUpdate = true;
    _dynamicContentNeedUpdate = true;
    _rotationNeedUpdate = false;
    _devUpdateRotationInProgress = false; // unlock
  }
  else {
    this->updateStatusBar();
  }

  // update content
  if (_dynamicContentNeedUpdate && !_devUpdateRotationInProgress) {
    switch(_dev->rotation()) {
      case DISPLAY_ROTATION_CW_0:
        _renderMainScreen();
        break;
      case DISPLAY_ROTATION_CW_90:
        _renderDetailScreen();
        break;
      case DISPLAY_ROTATION_CW_180:
        break;
      case DISPLAY_ROTATION_CW_270:
        break;
    }
    _dynamicContentNeedUpdate = false;
  }
}

void SensorDisplayController::_initDisplayItems()
{
  _mainItemCount = 0;
  _subItemCount = 0;

#if PM_SENSOR_TYPE >= PMS5003
  _displayMainItems[_mainItemCount++] = PM;
#endif
#if PM_SENSOR_TYPE >= PMS5003S
  _displayMainItems[_mainItemCount++] = HCHO;
#endif
#if PM_SENSOR_TYPE >= PMS5003ST || PM_SENSOR_TYPE == PMS5003T
  _displaySubItems[_subItemCount++] = TEMP;
  _displaySubItems[_subItemCount++] = HUMID;
#endif

#if CO2_SENSOR_TYPE != NOT_DEFINED
  _displayMainItems[_mainItemCount++] = CO2;
#endif
}

/////////////////////////////////////////////////////////////////////////////////////////
// used by rendering
/////////////////////////////////////////////////////////////////////////////////////////
const char* unitStr[] = {"", "ug/m3", "ppm", "C", "%"};
char _valueStr[100];
uint16_t _color;
uint8_t  _level;

const char * padSpace(uint16_t value)
{
  if (value < 10) return "  ";
  else if (value < 100) return " ";
  else return "";
}

/////////////////////////////////////////////////////////////////////////////////////////
// main screen rendering
/////////////////////////////////////////////////////////////////////////////////////////
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

void SensorDisplayController::_layoutScreen()
{
  if (_rotation == DISPLAY_ROTATION_CW_0) {
    float rowCount = _subItemCount > 0 ? _mainItemCount + 1 : _mainItemCount;
    rowDivHeight = (uint16_t) ( (_dev->height() - _contentOffsetY) / rowCount );
    
    itemOffsetY = (rowDivHeight - ITEM_RECT_HEIGHT) / 2;
    if (itemOffsetY > rowDivHeight) itemOffsetY = 0;
    
    subItemOffsetY = (rowDivHeight - SUBITEM_RECT_HEIGHT) / 2;
    if (subItemOffsetY > rowDivHeight) subItemOffsetY = 0;
    
    subItemRowDivWidth = _dev->width() / 2;
  }
}

void SensorDisplayController::_renderMainScreenMainItem(uint8_t refIndex, uint8_t posIndex,
                                                        const char *value, uint8_t lvl, uint16_t color)
{
  uint16_t baseOffsetY = _contentOffsetY + rowDivHeight * posIndex + itemOffsetY;

  // title
  if (_staticContentNeedUpdate)
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
                   LEVEL[lvl], ITEM_LEVEL_CHAR_W, ITEM_LEVEL_CHAR_H, color, RGB565_BLACK);
}

void SensorDisplayController::_renderMainScreenSubItem(uint8_t refIndex, uint8_t posIndex,
                                                       const char *value, uint8_t lvl, uint16_t color)
{
  uint16_t baseOffsetY = _contentOffsetY + rowDivHeight * (_mainItemCount +  posIndex / 2) + subItemOffsetY;
  uint16_t baseOffsetX = subItemRowDivWidth * (posIndex % 2);

  // title
  if (_staticContentNeedUpdate)
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

void SensorDisplayController::_targetData(SensorDataType t)
{
  switch(t) {
    case PM:
      if (_aqiPm2d5US > _aqiPm10US) {
        sprintf(_valueStr, "%d%s", _aqiPm2d5US, padSpace(_aqiPm2d5US));
        _color = _pm2d5Color;
        _level = _pm2d5Level;
      } else {
        sprintf(_valueStr, "%d%s", _aqiPm10US, padSpace(_aqiPm10US));
        _color = _pm10Color;
        _level = _pm10Level;
      }
      _level = _level == 6 ? 5 : _level;
      break;
    case HCHO:
      sprintf(_valueStr, "%.2f", _hcho);
      _color = _hchoColor;
      _level = _hchoLevel;
      break;
    case CO2:
      sprintf(_valueStr, "%d%s", (int)_co2, (int)_co2 < 1000 ? " " : "");
      _color = _co2Color;
      _level = _co2Level;
      break;
    case TEMP:
      sprintf(_valueStr, "%.1f", _temp);
      _color = _tempColor;
      _level = _tempLevel;
      break;
    case HUMID:
      sprintf(_valueStr, "%d", (int)_humid);
      _color = _humidColor;
      _level = _hchoLevel;
      break;
  }
}

void SensorDisplayController::_renderMainScreen()
{
  _dev->setTextSize(3);
  for (int i = 0; i < _mainItemCount; ++i) {
    _targetData(_displayMainItems[i]);
    _renderMainScreenMainItem(_displayMainItems[i], i, _valueStr, _level, _color);
  }
  _dev->setTextSize(2);
  for (int i = 0; i < _subItemCount; ++i) {
    _targetData(_displayMainItems[i]);
    _renderMainScreenSubItem(_displaySubItems[i], i, _valueStr, _level, _color);
  }
  _staticContentNeedUpdate = false;
}


/////////////////////////////////////////////////////////////////////////////////////////
// detail screen rendering
/////////////////////////////////////////////////////////////////////////////////////////
#define DETAIL_LINE_HEIGHT        20
#define DETAIL_LINE_VALUE_OFFSET  150
uint8_t detailRowCount = 0;

void SensorDisplayController::_renderDetailScreenItem(SensorDataType type)
{
  if (_staticContentNeedUpdate) {
     _dev->setTextSize(2);
    _dev->setTextColor(RGB565_WEAKWHITE, RGB565_BLACK);
    switch (type) {
    case PM:
      _dev->setCursor(0, _contentOffsetY + DETAIL_LINE_HEIGHT * detailRowCount);
      _dev->write("PM2.5(ug/m");
      _dev->setTextSize(1); _dev->write("3");
      _dev->setTextSize(2); _dev->write("):");
      _dev->setCursor(0, _contentOffsetY + DETAIL_LINE_HEIGHT * (detailRowCount + 1));
      _dev->write("PM2.5 index:");  
      _dev->setCursor(0, _contentOffsetY + DETAIL_LINE_HEIGHT * (detailRowCount + 2));
      _dev->write("PM10(ug/m");
      _dev->setTextSize(1); _dev->write("3");
      _dev->setTextSize(2); _dev->write("):");
      _dev->setCursor(0, _contentOffsetY + DETAIL_LINE_HEIGHT * (detailRowCount + 3));
      _dev->write("PM2.5 index:");
      break;

    case HCHO:
      _dev->setCursor(0, _contentOffsetY + DETAIL_LINE_HEIGHT * detailRowCount);
      _dev->write("HCHO(ug/m");
      _dev->setTextSize(1); _dev->write("3");
      _dev->setTextSize(2); _dev->write("):");
      break;

    case CO2:
      _dev->setCursor(0, _contentOffsetY + DETAIL_LINE_HEIGHT * detailRowCount);
      _dev->write("CO2(ppm):");
      break;

    case TEMP:
      _dev->setCursor(0, _contentOffsetY + DETAIL_LINE_HEIGHT * detailRowCount);
      _dev->write("Temp(C):");
      break;

    case HUMID:
      _dev->setCursor(0, _contentOffsetY + DETAIL_LINE_HEIGHT * detailRowCount);
      _dev->write("Humid(%):");
      break;
    }
  }

  switch (type) {
    case PM:
      _dev->setTextColor(_pm2d5Color, RGB565_BLACK);
      _dev->setCursor(DETAIL_LINE_VALUE_OFFSET, _contentOffsetY + DETAIL_LINE_HEIGHT * detailRowCount);
      sprintf(_valueStr, "%.1f", _pm2d5); _dev->write(_valueStr);
      _dev->setCursor(DETAIL_LINE_VALUE_OFFSET, _contentOffsetY + DETAIL_LINE_HEIGHT * (detailRowCount + 1));
      sprintf(_valueStr, "%d    ", _aqiPm2d5US); _dev->write(_valueStr);

      _dev->setTextColor(_pm10Color, RGB565_BLACK);
      _dev->setCursor(DETAIL_LINE_VALUE_OFFSET, _contentOffsetY + DETAIL_LINE_HEIGHT * (detailRowCount + 2));
      sprintf(_valueStr, "%.1f", _pm10); _dev->write(_valueStr);
      _dev->setCursor(DETAIL_LINE_VALUE_OFFSET, _contentOffsetY + DETAIL_LINE_HEIGHT * (detailRowCount + 3));
      sprintf(_valueStr, "%d    ", _aqiPm10US); _dev->write(_valueStr);

      detailRowCount += 4;
      break;

    case HCHO:
      _dev->setTextColor(_hchoColor, RGB565_BLACK);
      _dev->setCursor(DETAIL_LINE_VALUE_OFFSET, _contentOffsetY + DETAIL_LINE_HEIGHT * detailRowCount++);
      sprintf(_valueStr, "%.2f", _hcho); _dev->write(_valueStr);
      break;

    case CO2:
      _dev->setTextColor(_co2Color, RGB565_BLACK);
      _dev->setCursor(DETAIL_LINE_VALUE_OFFSET, _contentOffsetY + DETAIL_LINE_HEIGHT * detailRowCount++);
      sprintf(_valueStr, "%d ", (int)_co2); _dev->write(_valueStr);
      break;

    case TEMP:
      _dev->setTextColor(_tempColor, RGB565_BLACK);
      _dev->setCursor(DETAIL_LINE_VALUE_OFFSET, _contentOffsetY + DETAIL_LINE_HEIGHT * detailRowCount++);
      sprintf(_valueStr, "%.1f", _temp); _dev->write(_valueStr);
      break;

    case HUMID:
      _dev->setTextColor(_hchoColor, RGB565_BLACK);
      _dev->setCursor(DETAIL_LINE_VALUE_OFFSET, _contentOffsetY + DETAIL_LINE_HEIGHT * detailRowCount++);
      sprintf(_valueStr, "%d", (int)_humid); _dev->write(_valueStr);
      break;
  }
}

void SensorDisplayController::_renderDetailScreen()
{
  detailRowCount = 0;
  _dev->setTextSize(2);
  for (int i = 0; i < _mainItemCount; ++i) {
    _renderDetailScreenItem(_displayMainItems[i]);
  }
  _dev->setTextSize(2);
  for (int i = 0; i < _subItemCount; ++i) {
    _renderDetailScreenItem(_displaySubItems[i]);
  }
  _staticContentNeedUpdate = false;
}
