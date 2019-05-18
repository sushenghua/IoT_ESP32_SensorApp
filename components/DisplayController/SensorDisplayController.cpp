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
#include "System.h"

SensorDisplayController::SensorDisplayController(DisplayGFX *dev)
: DisplayController(dev)
, _inited(false)
, _hasScreenMsg(false)
, _staticContentNeedUpdate(true)
, _dynamicContentNeedUpdate(true)
, _rotationNeedUpdate(false)
, _devUpdateRotationInProgress(false)
, _qrCodeType(QRCodeMax)
, _mainItemCount(0)
, _subItemCount(0)
{}

void SensorDisplayController::init(int displayInitMode)
{
  if (!_inited) {
    DisplayController::init(displayInitMode);

    APP_LOGI("[SensorDC]", "init");

    // cache capability from System instance
    _devCap = System::instance()->devCapability();

    // device init and screen preparation
    _dev->init(displayInitMode);
    _dev->fillScreen(RGB565_BLACK);

    _dev->setTextSize(2);

    _rotation = _dev->rotation();
    _lastRotation = _rotation;
    _initDisplayItems();
    _layoutScreen();

    // setQRCodeType(QRCodeDevice);

    _inited = true;
  }
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

void SensorDisplayController::setPmData(const PMData *pmData, bool update)
{
  // value update
  _sensorData.pm1d0 = pmData->pm1d0;
  _sensorData.pm2d5 = pmData->pm2d5;
  _sensorData.pm10 = pmData->pm10;
  _sensorData.aqiPm2d5US = pmData->aqiPm2d5US;
  _sensorData.aqiPm10US = pmData->aqiPm10US;
  _sensorData.pm2d5Level = pmData->levelPm2d5US;
  _sensorData.pm10Level = pmData->levelPm10US;

  // color update
  _sensorData.pm2d5Color = HS::colorForAirLevel(pmData->levelPm2d5US);
  _sensorData.pm10Color = HS::colorForAirLevel(pmData->levelPm10US);

  if (update) _dynamicContentNeedUpdate = true;
}

void SensorDisplayController::setHchoData(const HchoData *hchoData, bool update)
{
  // value update
  _sensorData.hcho = hchoData->hcho;
  _sensorData.hchoLevel = hchoData->level;

  // color update
  _sensorData.hchoColor = HS::colorForHchoLevel(hchoData->level);

  if (update) _dynamicContentNeedUpdate = true;
}

void SensorDisplayController::setTempHumidData(const TempHumidData *tempHumidData, bool update)
{
  // value update
  _sensorData.temp = tempHumidData->temp;
  _sensorData.humid = tempHumidData->humid;
  _sensorData.tempLevel = tempHumidData->levelTemp;
  _sensorData.humidLevel = tempHumidData->levelHumid;

  // color update
  _sensorData.tempColor = HS::colorForTempLevel(tempHumidData->levelTemp);
  _sensorData.humidColor = HS::colorForHumidLevel(tempHumidData->levelHumid);

  if (update) _dynamicContentNeedUpdate = true;
}

void SensorDisplayController::setCO2Data(const CO2Data *co2Data, bool update)
{
  // value update
  _sensorData.co2 = co2Data->co2;
  _sensorData.co2Level = co2Data->level;

  // color update
  _sensorData.co2Color = HS::colorForCO2Level(co2Data->level);

  if (update) _dynamicContentNeedUpdate = true;
}

void SensorDisplayController::setLuminosityData(const LuminosityData *lmData, bool update)
{
  // value update
  _sensorData.luminosity = lmData->luminosity;

  // color update
  _sensorData.lumiColor = 0x07F9; // cyan

  if (update) _dynamicContentNeedUpdate = true;
}

char _msg[256];

void SensorDisplayController::setScreenMessage(const char * msg)
{
  if (msg) {
    sprintf(_msg, "%s", msg);
    _hasScreenMsg = true;
  }
  else {
    _hasScreenMsg = false;
    _rotationNeedUpdate = true;
    _staticContentNeedUpdate = true;
    _dynamicContentNeedUpdate = true;
  }
}

void SensorDisplayController::update()
{
  if (_updateDisabled) return;

  if (_hasScreenMsg) {
    _dev->fillScreen(RGB565_BLACK);
    _dev->setCursor(10, _dev->height()/2);
    _dev->setTextSize(2);
    _dev->setTextColor(RGB565_YELLOW, RGB565_BLACK);
    _dev->write(_msg);
    return;
  }

  // update rotation
  if (_rotationNeedUpdate) {
    // APP_LOGE("[SensorDC]", "rotation update");
    _devUpdateRotationInProgress = true;  // lock
    _dev->fillScreen(RGB565_BLACK);
    _dev->setRotation(_rotation);
    _layoutScreen();
    if (_rotation == DISPLAY_ROTATION_CW_270) setQRCodeType(QRCodeDevice, true);
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
  if ((_dynamicContentNeedUpdate || _staticContentNeedUpdate)
      && !_devUpdateRotationInProgress) {
    // APP_LOGI("[SensorDC]", "content update");
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
        _renderQRCodeScreen();
        break;
    }
    _dynamicContentNeedUpdate = false;
  }
}

void SensorDisplayController::_initDisplayItems()
{
  _mainItemCount = 0;
  _subItemCount = 0;

  if (_devCap & PM_CAPABILITY_MASK)
    _displayMainItems[_mainItemCount++] = PM;

  if (_devCap & HCHO_CAPABILITY_MASK)
    _displayMainItems[_mainItemCount++] = HCHO;

  if (_devCap & CO2_CAPABILITY_MASK)
    _displayMainItems[_mainItemCount++] = CO2;

  if (_devCap & TEMP_HUMID_CAPABILITY_MASK) {
    _displaySubItems[_subItemCount++] = TEMP;
    _displaySubItems[_subItemCount++] = HUMID;
  }

  if (_devCap & LUMINOSITY_CAPABILITY_MASK) {
    _displayMainItems[_mainItemCount++] = LUMI;
  }
}

/////////////////////////////////////////////////////////////////////////////////////////
// used by rendering
/////////////////////////////////////////////////////////////////////////////////////////
const char* unitStr[] = {"", "ug/m3", "ppm", "C", "%", "lm"};
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
#define ITEM_TITLE_OFFSET_X     20  // original 4
#define ITEM_TITLE_OFFSET_Y     9

#define ITEM_TITLE_CHAR_W       80
#define ITEM_TITLE_CHAR_H       20

#define ITEM_RECT_RADIUS        10
#define ITEM_RECT_OFFSET_X      130 // original 100
#define ITEM_RECT_HEIGHT        40
#define ITEM_RECT_WIDTH         170

#define ITEM_VALUE_OFFSET_X     10
#define ITEM_VALUE_OFFSET_Y     9

#define ITEM_LEVEL_OFFSET_X     116
#define ITEM_LEVEL_OFFSET_Y     8
#define ITEM_LEVEL_CHAR_W       48
#define ITEM_LEVEL_CHAR_H       24


#define SUBITEM_TITLE_OFFSET_X  20  // original 4
#define SUBITEM_TITLE_OFFSET_Y  6

#define SUBITEM_TITLE_CHAR_W    40
#define SUBITEM_TITLE_CHAR_H    20

#define SUBITEM_RECT_RADIUS     5
#define SUBITEM_RECT_OFFSET_X   67  // original 51
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
      if (_sensorData.aqiPm2d5US > _sensorData.aqiPm10US) {
        sprintf(_valueStr, "%d%s", _sensorData.aqiPm2d5US, padSpace(_sensorData.aqiPm2d5US));
        _color = _sensorData.pm2d5Color;
        _level = _sensorData.pm2d5Level;
      } else {
        sprintf(_valueStr, "%d%s", _sensorData.aqiPm10US, padSpace(_sensorData.aqiPm10US));
        _color = _sensorData.pm10Color;
        _level = _sensorData.pm10Level;
      }
      _level = _level == 6 ? 5 : _level;
      break;
    case HCHO:
      sprintf(_valueStr, "%.3f", _sensorData.hcho);
      _color = _sensorData.hchoColor;
      _level = _sensorData.hchoLevel;
      break;
    case CO2:
      sprintf(_valueStr, "%d%s", (int)_sensorData.co2, (int)_sensorData.co2 < 1000 ? " " : "");
      _color = _sensorData.co2Color;
      _level = _sensorData.co2Level;
      break;
    case TEMP:
      sprintf(_valueStr, "%.1f", _sensorData.temp);
      _color = _sensorData.tempColor;
      _level = _sensorData.tempLevel;
      break;
    case HUMID:
      sprintf(_valueStr, "%.1f", _sensorData.humid);
      _color = _sensorData.humidColor;
      _level = _sensorData.hchoLevel;
      break;
    case LUMI:
      sprintf(_valueStr, "%d%s", _sensorData.luminosity, _sensorData.luminosity < 1000.0f ? " " : "");
      _color = _sensorData.lumiColor;
      _level = _sensorData.lumiLevel;
      break;
    default: break;
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
    _targetData(_displaySubItems[i]);
    _renderMainScreenSubItem(_displaySubItems[i], i, _valueStr, _level, _color);
  }
  _staticContentNeedUpdate = false;
}


/////////////////////////////////////////////////////////////////////////////////////////
// detail screen rendering
/////////////////////////////////////////////////////////////////////////////////////////
#define DETAIL_LINE_BASE_OFFSET_X   10
#define DETAIL_LINE_BASE_OFFSET_Y   40
#define DETAIL_LINE_HEIGHT          20
#define DETAIL_LINE_VALUE_OFFSET    150
uint8_t detailRowCount = 0;

void SensorDisplayController::_renderDetailScreenItem(SensorDataType type)
{
  uint16_t offsetX = DETAIL_LINE_BASE_OFFSET_X;
  uint16_t offsetY = _contentOffsetY + DETAIL_LINE_BASE_OFFSET_Y;
  if (_staticContentNeedUpdate) {
    _dev->setTextSize(2);
    _dev->setTextColor(RGB565_WEAKWHITE, RGB565_BLACK);
    switch (type) {
    case PM:
      _dev->setCursor(offsetX, offsetY + DETAIL_LINE_HEIGHT * detailRowCount);
      _dev->write("PM2.5(ug/m");
      _dev->setTextSize(1); _dev->write("3");
      _dev->setTextSize(2); _dev->write("):");
      _dev->setCursor(offsetX, offsetY + DETAIL_LINE_HEIGHT * (detailRowCount + 1));
      _dev->write("PM2.5 index:");  
      _dev->setCursor(offsetX, offsetY + DETAIL_LINE_HEIGHT * (detailRowCount + 2));
      _dev->write("PM10(ug/m");
      _dev->setTextSize(1); _dev->write("3");
      _dev->setTextSize(2); _dev->write("):");
      _dev->setCursor(offsetX, offsetY + DETAIL_LINE_HEIGHT * (detailRowCount + 3));
      _dev->write("PM10 index:");
      break;

    case HCHO:
      _dev->setCursor(offsetX, offsetY + DETAIL_LINE_HEIGHT * detailRowCount);
      _dev->write("HCHO(mg/m");
      _dev->setTextSize(1); _dev->write("3");
      _dev->setTextSize(2); _dev->write("):");
      break;

    case CO2:
      _dev->setCursor(offsetX, offsetY + DETAIL_LINE_HEIGHT * detailRowCount);
      _dev->write("CO2(ppm):");
      break;

    case TEMP:
      _dev->setCursor(offsetX, offsetY + DETAIL_LINE_HEIGHT * detailRowCount);
      _dev->write("Temp(C):");
      break;

    case HUMID:
      _dev->setCursor(offsetX, offsetY + DETAIL_LINE_HEIGHT * detailRowCount);
      _dev->write("Humid(%):");
      break;

    case LUMI:
      _dev->setCursor(offsetX, offsetY + DETAIL_LINE_HEIGHT * detailRowCount);
      _dev->write("Lumi(lm):");
      break;

    default: break;
    }
  }

  offsetX = DETAIL_LINE_BASE_OFFSET_X + DETAIL_LINE_VALUE_OFFSET;
  switch (type) {
    case PM:
      _dev->setTextColor(_sensorData.pm2d5Color, RGB565_BLACK);
      _dev->setCursor(offsetX, offsetY + DETAIL_LINE_HEIGHT * detailRowCount);
      sprintf(_valueStr, "%.1f", _sensorData.pm2d5); _dev->write(_valueStr);
      _dev->setCursor(offsetX, offsetY + DETAIL_LINE_HEIGHT * (detailRowCount + 1));
      sprintf(_valueStr, "%d    ", _sensorData.aqiPm2d5US); _dev->write(_valueStr);

      _dev->setTextColor(_sensorData.pm10Color, RGB565_BLACK);
      _dev->setCursor(offsetX, offsetY + DETAIL_LINE_HEIGHT * (detailRowCount + 2));
      sprintf(_valueStr, "%.1f", _sensorData.pm10); _dev->write(_valueStr);
      _dev->setCursor(offsetX, offsetY + DETAIL_LINE_HEIGHT * (detailRowCount + 3));
      sprintf(_valueStr, "%d    ", _sensorData.aqiPm10US); _dev->write(_valueStr);

      detailRowCount += 4;
      break;

    case HCHO:
      _dev->setTextColor(_sensorData.hchoColor, RGB565_BLACK);
      _dev->setCursor(offsetX, offsetY + DETAIL_LINE_HEIGHT * detailRowCount++);
      sprintf(_valueStr, "%.3f", _sensorData.hcho); _dev->write(_valueStr);
      break;

    case CO2:
      _dev->setTextColor(_sensorData.co2Color, RGB565_BLACK);
      _dev->setCursor(offsetX, offsetY + DETAIL_LINE_HEIGHT * detailRowCount++);
      sprintf(_valueStr, "%d ", (int)_sensorData.co2); _dev->write(_valueStr);
      break;

    case TEMP:
      _dev->setTextColor(_sensorData.tempColor, RGB565_BLACK);
      _dev->setCursor(offsetX, offsetY + DETAIL_LINE_HEIGHT * detailRowCount++);
      sprintf(_valueStr, "%.1f", _sensorData.temp); _dev->write(_valueStr);
      break;

    case HUMID:
      _dev->setTextColor(_sensorData.humidColor, RGB565_BLACK);
      _dev->setCursor(offsetX, offsetY + DETAIL_LINE_HEIGHT * detailRowCount++);
      sprintf(_valueStr, "%.1f", _sensorData.humid); _dev->write(_valueStr);
      break;

    case LUMI:
      _dev->setTextColor(_sensorData.lumiColor, RGB565_BLACK);
      _dev->setCursor(offsetX, offsetY + DETAIL_LINE_HEIGHT * detailRowCount++);
      sprintf(_valueStr, "%d", _sensorData.luminosity); _dev->write(_valueStr);
      break;

    default: break;
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


/////////////////////////////////////////////////////////////////////////////////////////
// QR code screen rendering
/////////////////////////////////////////////////////////////////////////////////////////
#include "qrcodegen.h"
#include "SharedBuffer.h"

uint8_t tempBuffer[qrcodegen_BUFFER_LEN_MAX];
uint8_t qrcode[qrcodegen_BUFFER_LEN_MAX];

inline bool _genQRCode(const char *text)
{
  return qrcodegen_encodeText(text, tempBuffer, qrcode, qrcodegen_Ecc_LOW,
                              qrcodegen_VERSION_MIN, qrcodegen_VERSION_MAX,
                              qrcodegen_Mask_AUTO, true);
}

#define BORDER         1
#define PIXEL_SCALE    4
char *  _qrStr = NULL;

static const char * const QRCodeTitle[] = {
    "Device",           // 0
    "iOS App",          // 1
    "Android App",      // 2
    "Manual",           // 4
};

#define IOS_APP_URL         "https://itunes.apple.com/cn/app/q-monitor/id1318735622"
#define ANDROID_APP_URL     "http://appsgenuine.com/download/qmonitor.apk"
#define DEVICE_MANUAL_URL   "http://appsgenuine.com/#eighth"

void SensorDisplayController::_renderQRCodeScreen()
{
  if (_staticContentNeedUpdate) {

    if (_genQRCode(_qrStr)) {
      // clear non status bar area
      _dev->fillRect(0, _contentOffsetY, _dev->width(), _dev->height()-_contentOffsetY, RGB565_BLACK);

      // layout
      int size = qrcodegen_getSize(qrcode);
      uint16_t imgSize = (size + BORDER) * PIXEL_SCALE;
      uint16_t xOffset = (_dev->width() - imgSize) / 2;
      uint16_t yOffset =  _contentOffsetY + (_dev->height() - _contentOffsetY - imgSize) / 2;

      // title
      _dev->setTextColor(RGB565_WHITE, RGB565_BLACK);
      _dev->setCursor(xOffset, yOffset - 30);
      _dev->write(QRCodeTitle[_qrCodeType]);

      // QR Code
      for (int y = -BORDER; y < size + BORDER; y++) {
        for (int x = -BORDER; x < size + BORDER; x++) {
          uint16_t color = qrcodegen_getModule(qrcode, x, y) ? RGB565_BLACK : RGB565_WHITE;
          _dev->fillRect(xOffset + x * PIXEL_SCALE, yOffset + y * PIXEL_SCALE, PIXEL_SCALE, PIXEL_SCALE, color);
        }
      }
    }

    _staticContentNeedUpdate = false;
  }
}

void SensorDisplayController::setQRCodeType(QRCodeType type, bool forceUpdate)
{
  if (_qrCodeType != type || forceUpdate) {
    _qrCodeType = type;

    _qrStr = SharedBuffer::qrStrBuffer();

    System *sys = System::instance();

    switch(type) {
      case QRCodeDevice:
        sprintf(_qrStr, "{\"ret\":{\"uid\":\"%s\",\"cap\":\"%u\",\"firmv\":\"%s\",\"bdv\":\"%s\",\"model\":\"%s\",\"alcd\":%s,\"deploy\":\"%s\",\"devname\":\"%s\",\"life\":\"%d\"}, \"cmd\":\"%s\"}",
                sys->uid(),
                sys->devCapability(),
                sys->firmwareVersion(),
                sys->boardVersion(),
                sys->model(),
                sys->displayAutoAdjustOn()? "true" : "false",
                deployModeStr(sys->deployMode()),
                sys->deviceName(),
                sys->maintenance()->allSessionsLife,
                "GetDeviceInfo");
        break;

      case QRCodeIOS:
        sprintf(_qrStr, "%s", IOS_APP_URL);
        break;

      case QRCodeAndroid:
        sprintf(_qrStr, "%s", ANDROID_APP_URL);
        break;

      case QRCodeManual:
        sprintf(_qrStr, "%s", DEVICE_MANUAL_URL);
        break;

      default:
        break;
    }

    if (_dev->rotation() == DISPLAY_ROTATION_CW_270) _staticContentNeedUpdate = true;
  }
}

void SensorDisplayController::onUsrButtonRelease()
{
  if (!_dev->on()) {
    _dev->turnOn(true);
  }
  else {
    if (_dev->rotation() == DISPLAY_ROTATION_CW_270) {
      setQRCodeType( (QRCodeType)( (_qrCodeType + 1) % QRCodeMax) );
    }
  }
}