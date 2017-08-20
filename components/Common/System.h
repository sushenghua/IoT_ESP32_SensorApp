/*
 * System.h system function
 * Copyright (c) 2017 Shenghua Su
 *
 */

#ifndef _SYSTEM_H_
#define _SYSTEM_H_

#include "SensorConfig.h"

enum DeployMode {
  HTTPServerMode,
  MQTTClientMode,
  MQTTClientAndHTTPServerMode
};

struct SysConfig {
  bool        wifiOn;
  DeployMode  deployMode;
  SensorType  pmSensorType;
  SensorType  co2SensorType;
  uint32_t    devCapability;
};

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
  void setSensorType(SensorType pmType, SensorType co2Type);
  void setDevCapability(uint32_t cap);
  void restart();
  bool restarting();

  void pausePeripherals();
  void resumePeripherals();
  bool wifiOn();
  void toggleWifi();
  void toggleScreen();

private:
  void _setDefaultConfig();
  bool _loadConfig();
  bool _saveConfig();
  void _launchTasks();

private:
  State        _state;
  SysConfig    _config;
};

#endif // _SYSTEM_H_
