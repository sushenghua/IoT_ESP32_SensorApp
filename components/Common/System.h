/*
 * System.h system function
 * Copyright (c) 2017 Shenghua Su
 *
 */

#ifndef _SYSTEM_H_
#define _SYSTEM_H_

#include "SensorConfig.h"
#include <string.h>

/////////////////////////////////////////////////////////////////////////////////////////
// enum types
/////////////////////////////////////////////////////////////////////////////////////////
#ifdef __cplusplus
extern "C" {
#endif

// ------ deploy and sensor type config
enum DeployMode {
  HTTPServerMode,
  MQTTClientMode,
  MQTTClientAndHTTPServerMode,
  DeployModeMax
};

const char * deployModeStr(DeployMode type);

struct SysConfig {
  bool        wifiOn;
  bool        displayAutoAdjustOn;
  DeployMode  deployMode;
  SensorType  pmSensorType;
  SensorType  co2SensorType;
  uint32_t    devCapability;
};

// ------ mobile os
enum MobileOS {
  IOS       = 0,
  Android   = 1
};

const char* mobileOSStr(MobileOS);

// ------ token
#define TOKEN_LEN   64
#define TOKEN_COUNT 5

struct MobileToken {
  bool     on;
  MobileOS os;
  char     str[TOKEN_LEN+1]; // null terminate
};

struct MobileTokens {
  uint8_t     head;
  uint8_t     count;
  MobileToken tokens[TOKEN_COUNT];
  // functions
  void init() {
    head = 0; count = 0;
    for (uint8_t i=0; i<TOKEN_COUNT; ++i)
      tokens[i].str[TOKEN_LEN] = '\0';
  }
  void setToken(bool on, MobileOS os, const char *token) {
    int8_t index = findToken(token);
    if (index == -1) {
      if (count < TOKEN_COUNT) { index = (head + count) % TOKEN_COUNT; ++count; }
      else { index = head; head = (head + 1) % TOKEN_COUNT; }
      tokens[index].os = os;
      strncpy(tokens[index].str, token, TOKEN_LEN);
    }
    else { index = (head + index) % TOKEN_COUNT; }
    tokens[index].on = on;
  }
  int8_t findToken(const char *token) {
    for (int8_t index = 0; index < count; ++index) {
      uint8_t realIndex = (head + index) % TOKEN_COUNT;
      if (strncmp(token, tokens[realIndex].str, TOKEN_LEN) == 0) return index;
    }
    return -1;
  }
  MobileToken & token(uint8_t index) { return tokens[(head + index) % TOKEN_COUNT]; }
};

// ------ alert
enum TriggerAlert {
  TriggerNone,
  TriggerL,
  TriggerG
};

struct Alert {
  bool  lEnabled;
  bool  gEnabled;
  float lValue;
  float gValue;
};

struct Alerts {
  bool  soundOn;
  Alert sensors[SensorDataTypeCount];
  void init() {
    soundOn = false;
    for (uint8_t i=0; i<SensorDataTypeCount; ++i) {
      sensors[i].lEnabled = sensors[i].gEnabled = false;
      sensors[i].lValue = sensors[i].gValue = 0;
    }
  }
};

#ifdef __cplusplus
}
#endif


/////////////////////////////////////////////////////////////////////////////////////////
// Sytem class
/////////////////////////////////////////////////////////////////////////////////////////
class System
{
public:
  // type define
  enum State {
    Uninitialized,
    Initializing,
    Running,
    Restarting,
    Error
  };

public:
  // singleton
  static System * instance();

public:
  System();
  void init();
  const char* uid();
  const char* macAddress();
  const char* idfVersion();
  const char* firmwareVersion();
  const char* model();

  DeployMode deployMode();
  SensorType pmSensorType();
  SensorType co2SensorType();
  uint32_t devCapability();
  void setDeployMode(DeployMode mode);
  void toggleDeployMode();
  void setSensorType(SensorType pmType, SensorType co2Type);
  void setDevCapability(uint32_t cap);

  bool alertSoundOn();
  TriggerAlert sensorValueTriggerAlert(SensorDataType type, float value);
  MobileTokens * mobileTokens();
  void setAlertSoundOn(bool on);
  void setAlert(SensorDataType type, bool lEnabled, bool gEnabled, float lValue, float gValue);
  void setPnToken(bool enabled, MobileOS os, const char *token);

  void setRestartRequest();
  void restart();
  bool restarting();

  void pausePeripherals(const char *screenMsg = 0);
  void resumePeripherals();

  void powerOff();
  bool wifiOn() { return _config.wifiOn; }
  bool displayOn();
  bool displayAutoAdjustOn() { return _config.displayAutoAdjustOn; }
  void turnWifiOn(bool on = true);
  void turnDisplayOn(bool on = true);
  void turnDisplayAutoAdjustOn(bool on = true);
  void toggleWifi();
  void toggleDisplay();
  void markPowerEvent();

private:
  void _launchTasks();
  void _setDefaultConfig();
  bool _loadConfig();
  bool _saveConfig();
  bool _loadAlerts();
  bool _saveAlerts();
  bool _loadMobileTokens();
  bool _saveMobileTokens();
  void _saveMemoryData();
  void _updateConfig(bool saveImmediately = false);
  void _updateAlerts(bool saveImmedidately = false);
  void _updateMobileTokens(bool saveImmedidately = false);

private:
  State        _state;
  bool         _configNeedToSave;
  bool         _alertsNeedToSave;
  bool         _tokensNeedToSave;
  SysConfig    _config;
  Alerts       _alerts;
  MobileTokens _mobileTokens;
};

#endif // _SYSTEM_H_
