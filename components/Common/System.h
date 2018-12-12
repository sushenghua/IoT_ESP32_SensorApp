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

#define DEV_NAME_MAX_LEN 64

struct SysConfig1 {
  bool        wifiOn;
  bool        displayAutoAdjustOn;
  DeployMode  deployMode;
  int64_t     reserve[2]; // reserve for future use
};

struct SysConfig2 {
  SensorType  pmSensorType;
  SensorType  co2SensorType;
  uint32_t    devCapability;
  char        devName[DEV_NAME_MAX_LEN+1];
  int64_t     reserve[2];
};

typedef uint32_t LifeTime;

struct Maintenance {
  LifeTime    allSessionsLife;    // seconds
  LifeTime    recentSessionLife;  // seconds
  int64_t     reserve[2];
  void init() {
    allSessionsLife = 0;
    recentSessionLife = 0;
  }
};

struct SysResetRestore {
  uint32_t    deepSleepResetCount;
  uint32_t    lAlertReactiveCounter;
  uint32_t    gAlertReactiveCounter;
  int64_t     reserve[2];
  void init() {
    deepSleepResetCount = 0;
    lAlertReactiveCounter = 0;
    gAlertReactiveCounter = 0;
  }
};

struct Bias {
  bool        mbTempNeedCalibrate;
  float       mbTempBias;
  int64_t     reserve[2];
  void init() {
    mbTempNeedCalibrate = false;
    mbTempBias = 0;
  }
};

// ------ mobile os
enum MobileOS {
  iOS       = 0,
  Android   = 1
};

const char* mobileOSStr(MobileOS);

// ------ token
#define GROUP_LEN   64
#define TOKEN_LEN   64
#define TOKEN_COUNT 5

struct MobileToken {
  bool     on;
  MobileOS os;
  char     str[TOKEN_LEN+1];   // null terminated
  uint8_t  groupLen;
  char     group[GROUP_LEN+1]; // null terminated
  int64_t  reserve[2];
};

struct MobileTokens {
  uint8_t     head;
  uint8_t     count;
  MobileToken tokens[TOKEN_COUNT];
  // functions
  void init() {
    head = 0; count = 0;
    for (uint8_t i=0; i<TOKEN_COUNT; ++i) {
      tokens[i].str[TOKEN_LEN] = '\0';
      tokens[i].groupLen = 0;
      tokens[i].group[GROUP_LEN] = '\0';
    }
  }
  void setToken(bool on, MobileOS os, const char *token, size_t groupLen=0, const char *group=NULL) {
    int8_t index = findToken(token);
    if (index == -1) {
      if (count < TOKEN_COUNT) { index = (head + count) % TOKEN_COUNT; ++count; }
      else { index = head; head = (head + 1) % TOKEN_COUNT; }
      tokens[index].os = os;
      strncpy(tokens[index].str, token, TOKEN_LEN);
      if (groupLen > 0) {
        tokens[index].groupLen = groupLen < GROUP_LEN ? groupLen : GROUP_LEN;
        memcpy(tokens[index].group, group, tokens[index].groupLen);
        tokens[index].group[tokens[index].groupLen] = '\0';
      }
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
  bool        lEnabled;
  bool        gEnabled;
  float       lValue;
  float       gValue;
  int64_t     reserve[2];
};

#define ALERT_REACTIVE_COUNT   60000  // 60000 * 10ms(mqtt_task delay) = 10 min

struct Alerts {
  bool  pnEnabled;
  bool  soundEnabled;
  uint32_t reactiveTimeCount;
  Alert sensors[SensorDataTypeCount];
  void init() {
    pnEnabled = false;
    soundEnabled = false;
    reactiveTimeCount = ALERT_REACTIVE_COUNT;
    for (uint8_t i=0; i<SensorDataTypeCount; ++i) {
      sensors[i].lEnabled = sensors[i].gEnabled = false;
      sensors[i].lValue = sensors[i].gValue = 0.0f;
    }
  }
};

struct SysData {
  SysConfig1        config1;
  SysConfig2        config2;
  Maintenance       maintenance;
  SysResetRestore   resetRestore;
  Bias              bias;
  Alerts            alerts;
  MobileTokens      mobileTokens;
  void init() {
    maintenance.init();
    resetRestore.init();
    bias.init();
    alerts.init();
    mobileTokens.init();
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
  const char* boardVersion();
  const char* model();
  bool flashEncryptionEnabled();

  DeployMode deployMode();
  SensorType pmSensorType();
  SensorType co2SensorType();
  uint32_t devCapability();
  const char* deviceName();
  void setDeployMode(DeployMode mode);
  void toggleDeployMode();
  void setSensorType(SensorType pmType, SensorType co2Type);
  void setDevCapability(uint32_t cap);
  void setDeviceName(const char* name, size_t len = 0);

#ifdef DEBUG_BATTERY_LIFE
  void clearMaintenance();
#endif
  const Maintenance * maintenance();
  SysResetRestore * resetRestoreData();

  Bias * bias() { return &_data.bias; }
  void setMbTempCalibration(bool need, float tempBias=0);

  bool alertPnEnabled();
  bool alertSoundEnabled();
  Alerts * alerts();
  TriggerAlert sensorValueTriggerAlert(SensorDataType type, float value);
  MobileTokens * mobileTokens();
  bool tokenEnabled(MobileOS os, const char* token);
  void setAlertPnEnabled(bool enabled);
  void setAlertSoundEnabled(bool enabled);
  bool alertSoundOn();
  void turnAlertSoundOn(bool on);
  void setAlert(SensorDataType type, bool lEnabled, bool gEnabled, float lValue, float gValue);
  void setPnToken(bool enabled, MobileOS os, const char *token, size_t groupLen=0, const char *group=NULL);
  void resetAlertReactiveCounter();

  void setDebugFlag(uint8_t flag);
  void restoreFactory();
  void deepSleepReset();
  void setRestartRequest();
  void restart();
  bool restarting();

  void pausePeripherals(const char *screenMsg = 0);
  void resumePeripherals();

  void powerOff();
  bool wifiOn() { return _data.config1.wifiOn; }
  bool displayAutoAdjustOn() { return _data.config1.displayAutoAdjustOn; }
  void turnWifiOn(bool on = true);
  void turnDisplayOn(bool on = true);
  void turnDisplayAutoAdjustOn(bool on = true);
  void toggleWifi();
  void toggleDisplay();

  void onEvent(int eventId);

private:
  void _logInfo();
  void _launchTasks();
  void _setDefaultConfig();
  bool _loadData();
  bool _saveData();
  void _updateData(bool saveImmedidately = false);
  void _calculateMaintenance();
  void _saveMemoryData();
  void _updateConfig1(bool saveImmediately = false);
  void _updateConfig2(bool saveImmediately = false);
  void _updateMaintenance(bool saveImmediately = false);
  void _updateResetRestore(bool saveImmediately = false);
  void _updateBias(bool saveImmediately = false);
  void _updateAlerts(bool saveImmedidately = false);
  void _updateMobileTokens(bool saveImmedidately = false);

private:
  State             _state;
  bool              _dataNeedToSave;
  LifeTime          _currentSessionLife;
  SysData           _data;
};

#endif // _SYSTEM_H_
