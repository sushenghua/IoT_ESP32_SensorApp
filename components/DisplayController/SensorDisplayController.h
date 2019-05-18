/*
 * SensorDisplayController: sensor content display management
 * Copyright (c) 2017 Shenghua Su
 *
 */

#ifndef _SENSOR_DISPLAY_CONTROLLER_H
#define _SENSOR_DISPLAY_CONTROLLER_H

#include "DisplayController.h"
#include "PMData.h"
#include "HchoData.h"
#include "TempHumidData.h"
#include "LuminosityData.h"
#include "CO2Data.h"
#include "SensorConfig.h"
#include "SensorData.h"

enum QRCodeType {
  QRCodeDevice    = 0,
  QRCodeIOS       = 1,
  QRCodeAndroid   = 2,
  QRCodeManual    = 3,
  QRCodeMax
};

class SensorDisplayController : public DisplayController
{
public:
  SensorDisplayController(DisplayGFX *dev);

public:
  void forceContentUpdate() { _dynamicContentNeedUpdate = true; }

  uint8_t rotation() { return _rotation; }
  void setRotation(uint8_t rotation);

  const SensorData * sensorData() { return &_sensorData; }

  void setPmData(const PMData *pmData, bool update = true);
  void setHchoData(const HchoData *hchoData, bool update = true);
  void setTempHumidData(const TempHumidData *tempHumidData, bool update = true);
  void setCO2Data(const CO2Data *co2Data, bool update = true);
  void setLuminosityData(const LuminosityData *lmData, bool update = true);
  void setScreenMessage(const char * msg);

  void setQRCodeType(QRCodeType type, bool forceUpdate = false);
  void onUsrButtonRelease();

public:
  virtual void init(int displayInitMode=DISPLAY_INIT_ALL);
  virtual void tick();
  virtual void update();

public:
  void _targetData(SensorDataType t);
  void _initDisplayItems();
  void _layoutScreen();
  void _renderMainScreen();
  void _renderMainScreenMainItem(uint8_t refIndex, uint8_t posIndex,
                                 const char *value, uint8_t lvl, uint16_t color);
  void _renderMainScreenSubItem(uint8_t refIndex, uint8_t posIndex,
                                const char *value, uint8_t lvl, uint16_t color);
  void _renderDetailScreen();
  void _renderDetailScreenItem(SensorDataType type);
  void _renderQRCodeScreen();

protected:
  // init
  bool            _inited;

  // content update flag
  bool            _hasScreenMsg;
  bool            _staticContentNeedUpdate;
  bool            _dynamicContentNeedUpdate;

  // rotation
  bool            _rotationNeedUpdate;
  bool            _devUpdateRotationInProgress;
  uint8_t         _rotation;
  uint8_t         _lastRotation;

  // sensor data pack
  SensorData      _sensorData;

  // qr code
  QRCodeType      _qrCodeType;

  // screen layout
  uint32_t        _devCap;
  uint16_t        _mainItemCount;
  uint16_t        _subItemCount;
  SensorDataType  _displayMainItems[6];
  SensorDataType  _displaySubItems[6];
public:
  void  setMpu6050(float r, float p, float y);
  // mpu6050
  float roll;
  float pitch;
  float yaw;
};

#endif // _SENSOR_DISPLAY_CONTROLLER_H
