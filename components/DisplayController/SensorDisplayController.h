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
#include "CO2Data.h"

class SensorDisplayController : public DisplayController
{
public:
  SensorDisplayController(DisplayGFX *dev);

public:
  void forceContentUpdate() { _contentNeedUpdate = true; }

  uint8_t rotation() { return _rotation; }
  void setRotation(uint8_t rotation);

  void setPmData(PMData &pmData, bool update = true);
  void setHchoData(HchoData &hchoData, bool update = true);
  void setTempHumidData(TempHumidData &tempHumidData, bool update = true);
  void setCO2Data(CO2Data &co2Data, bool update = true);

public:
  virtual void init();
  virtual void tick();
  virtual void update();

public:
  void _renderScreenBg();
  void _renderMainScreen();
  void _renderCW90Screen();
  void _renderMainScreenItem(uint8_t refIndex, uint8_t posIndex, const char *value, uint8_t lvl, uint16_t color);

protected:
  // content update flag
  bool       _contentNeedUpdate;

  // rotation
  bool       _rotationNeedUpdate;
  bool       _devUpdateRotationInProgress;
  uint8_t    _rotation;
  uint8_t    _lastRotation;

  // co2 display
  uint16_t   _co2Color;
  float      _co2;

  // pm display
  uint16_t   _pm2d5Color;
  uint16_t   _pm10Color;
  uint16_t   _aqiPm2d5US;
  uint16_t   _aqiPm10US;
  float      _pm1d0;
  float      _pm2d5;
  float      _pm10;

  // formaldehyde display
  uint16_t   _hchoColor;
  float      _hcho;

  // temperature and humidity diplay
  uint16_t   _tempColor;
  uint16_t   _humidColor;
  float      _temp;
  float      _humid;
  
public:
  void  setMpu6050(float r, float p, float y);
  // mpu6050
  float roll;
  float pitch;
  float yaw;
};

#endif // _SENSOR_DISPLAY_CONTROLLER_H
