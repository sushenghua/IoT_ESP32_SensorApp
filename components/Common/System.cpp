/*
 * System.h system function
 * Copyright (c) 2017 Shenghua Su
 *
 */

/////////////////////////////////////////////////////////////////////////////////////////
// tasks to launch
/////////////////////////////////////////////////////////////////////////////////////////
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "Config.h"
#include "AppLog.h"
#include "System.h"

//----------------------------------------------
// peripheral task loop control
//----------------------------------------------
bool     _enablePeripheralTaskLoop = true;
uint8_t  _peripheralsStopCount = 0;

// watch dog
// #include "esp_task_wdt.h"
// esp_task_wdt_init(); // put it before task loop
// esp_task_wdt_feed(); // put it in task loop

//----------------------------------------------
// Wifi task
//----------------------------------------------
#include "Wifi.h"
#include "tcpip_adapter.h"

TaskHandle_t wifiTaskHandle;
void wifi_task(void *pvParameters)
{
  // config and start wifi
  if (Wifi::instance()->loadConfig()) {
    APP_LOGI("[Wifi]", "load config succeeded");
  }
  else {
    Wifi::instance()->setDefaultConfig();
    // Wifi::instance()->setStaConfig("woody@home", "58897@mljd-abcde");
    // Wifi::instance()->appendAltApConnectionSsidPassword("iPhone6S", "abcd1234");
    if (Wifi::instance()->saveConfig()) {
      APP_LOGI("[Wifi]", "save config succeeded");
    }
  }
  // set wifi mode accordingly
  DeployMode deployMode = System::instance()->deployMode();
  if (deployMode == MQTTClientMode) Wifi::instance()->setWifiMode(WIFI_MODE_STA);
  else if (deployMode == HTTPServerMode) Wifi::instance()->setWifiMode(WIFI_MODE_AP);
  else if (deployMode == MQTTClientAndHTTPServerMode) Wifi::instance()->setWifiMode(WIFI_MODE_APSTA);

  Wifi::instance()->init();
  // Wifi::instance()->start(true);
  // vTaskDelete(wifiTaskHandle);
  while (true) {
    if (System::instance()->wifiOn()) {
      Wifi::instance()->start();
    }
    else {
      Wifi::instance()->stop();
    }
    vTaskDelay(1000/portTICK_RATE_MS);
  }
}


//----------------------------------------------
// SNTP time task
//----------------------------------------------
#include "SNTP.h"

TaskHandle_t sntpTaskHandle;
static void sntp_task(void *pvParams)
{
  SNTP::init();
  SNTP::waitSync();

  SNTP::test();
  vTaskDelete(sntpTaskHandle);
}


//----------------------------------------------
// display tasks
//----------------------------------------------
// #include "ST7789V.h"
// ST7789V dev;// static ILI9341 dev;
#include "ILI9341.h"
#include "SensorDisplayController.h"
#define DISPLAY_TASK_DELAY_UNIT  100
ILI9341 dev;
bool    _displayOn = true;
static SensorDisplayController dc(&dev);
TaskHandle_t displayTaskHandle = 0;
bool _displayTaskPaused = false;
void display_task(void *p)
{
  dc.init();
  while (true) {
    // APP_LOGI("[display_task]", "update");
    // APP_LOGC("[display_task]", "task schedule state %d", xTaskGetSchedulerState());
    xTaskGetSchedulerState();
    if (_enablePeripheralTaskLoop) dc.update();
    else _displayTaskPaused = true;
    vTaskDelay(DISPLAY_TASK_DELAY_UNIT / portTICK_RATE_MS);
  }
}


//----------------------------------------------
// status check tasks
//----------------------------------------------
#include "Adc.h"
#include "InputMonitor.h"
#include "PowerManager.h"

#define STATUS_TASK_DELAY_UNIT  100

#define TIME_WIFI_UPDATE_COUNT  5
uint8_t _timeWifiUpdateCount = 0;

// bool _websocketConnected = false;

PowerManager powerManager;
bool _hasPwrEvent = true;

bool _statusTaskPaused = false;
void status_check_task(void *p)
{
  InputMonitor::instance()->init();   // this will launch another task
  powerManager.init();

  while (true) {
    if (_enablePeripheralTaskLoop) {
      // update time and wifi status every 0.5 second
      ++_timeWifiUpdateCount;
      if (_timeWifiUpdateCount >= TIME_WIFI_UPDATE_COUNT) {
        // wifi status
        if (!Wifi::instance()->started())
          dc.setNetworkState(NetworkOff);
        else if (System::instance()->deployMode() == HTTPServerMode)
          dc.setNetworkState(Wifi::instance()->apStaConnected()? NetworkConnected : NetworkNotConnected);
        else
          dc.setNetworkState(Wifi::instance()->connected()? NetworkConnected : NetworkNotConnected);

        // time
        dc.setTimeUpdate(true);

        _timeWifiUpdateCount = 0;
      }
      // battery level check
      if (powerManager.batteryLevelPollTick()) {       
        dc.setBatteryLevel(powerManager.batteryLevel());
      }
      // battery charge check
      if (_hasPwrEvent) {
        PowerManager::ChargeStatus chargeStatus = powerManager.chargeStatus();
        dc.setBatteryCharge(chargeStatus == PowerManager::PreCharge || chargeStatus == PowerManager::FastCharge); 
        _hasPwrEvent = false;
      }
    }
    else {
      _statusTaskPaused = true;
    }
    vTaskDelay(STATUS_TASK_DELAY_UNIT / portTICK_RATE_MS);
  }
}


//----------------------------------------------
// touch check task
//----------------------------------------------
// #include "TouchPad.h"

// TaskHandle_t touchPadTaskHandle;
// static void touch_pad_task(void *pvParams)
// {
//   TouchPad tp(TOUCH_PAD_NUM5);
//   tp.init();

//   while (true) {
//     tp.readValue();
//     vTaskDelay(200 / portTICK_PERIOD_MS);
//   }
// }


//----------------------------------------------
// sensor tasks
//----------------------------------------------
#include "SHT3xSensor.h"
#include "PMSensor.h"
#include "CO2Sensor.h"
#include "OrientationSensor.h"
#include "TSL2561.h"
#include "SensorDataPacker.h"

TaskHandle_t sht3xSensorTaskHandle;
bool _sht3xSensorTaskPaused = false;
void sht3x_sensor_task(void *p)
{
  SHT3xSensor sht3xSensor;
  sht3xSensor.init();
  sht3xSensor.setDisplayDelegate(&dc);
  SensorDataPacker::sharedInstance()->setTempHumidSensor(&sht3xSensor);
  while (true) {
    if (_enablePeripheralTaskLoop) sht3xSensor.sampleData();
    else _sht3xSensorTaskPaused = true;
    vTaskDelay(500/portTICK_RATE_MS);
  }
}

TaskHandle_t pmSensorTaskHandle;
bool _pmSensorTaskPaused = false;
void pm_sensor_task(void *p)
{
  PMSensor pmSensor;
  pmSensor.init();
  pmSensor.setDisplayDelegate(&dc);
  SensorDataPacker::sharedInstance()->init();
  SensorDataPacker::sharedInstance()->setPmSensor(&pmSensor);
  while (true) {
    if (_enablePeripheralTaskLoop) pmSensor.sampleData();
    else _pmSensorTaskPaused = true;
    vTaskDelay(500/portTICK_RATE_MS);
  }
}

TaskHandle_t co2SensorTaskHandle = NULL;
bool _co2SensorTaskPaused = false;
void co2_sensor_task(void *p)
{
  CO2Sensor co2Sensor;
  co2Sensor.init();
  co2Sensor.setDisplayDelegate(&dc);
  SensorDataPacker::sharedInstance()->setCO2Sensor(&co2Sensor);

  vTaskDelay(3000/portTICK_RATE_MS); // delay 3 seconds
  while (true) {
    if (_enablePeripheralTaskLoop) co2Sensor.sampleData(3000);
    else _co2SensorTaskPaused = true;
    vTaskDelay(1000/portTICK_RATE_MS);
  }
}

TaskHandle_t tsl2561SensorTaskHandle;
bool _tsl2561SensorTaskPaused = false;
uint32_t _luminsity = 0;
void tsl2561_sensor_task(void *p)
{
  TSL2561 tsl2561Sensor;
  tsl2561Sensor.init();
  while (true) {
    if (_enablePeripheralTaskLoop) {
      if (System::instance()->displayAutoAdjustOn()) {
        tsl2561Sensor.sampleData();
        _luminsity = tsl2561Sensor.luminosity();
        // APP_LOGC("[TSL2561 Task]", "lux: %d", _luminsity);
        if (_luminsity >= 100) dc.fadeBrightness(100);
        else if (_luminsity < 10) dc.fadeBrightness(10);
        else dc.fadeBrightness(_luminsity);
      }
      else if (_luminsity != 100) {
        _luminsity = 100;
        dc.fadeBrightness(100);
      }
    }
    else _tsl2561SensorTaskPaused = true;
    vTaskDelay(1000/portTICK_RATE_MS);
  }
}

#define ORI_SENSOR_TEMP_READ_COUNT 30
TaskHandle_t orientationSensorTaskHandle;
uint16_t _oriSensorTempReadCount = 0;
float _oriSensorTemperature = 0;
bool _orientationSensorTaskPaused = false;
void orientation_sensor_task(void *p)
{
  OrientationSensor orientationSensor;
  orientationSensor.init();
  orientationSensor.setDisplayDelegate(&dc);
  while (true) {
    if (_enablePeripheralTaskLoop) {
      orientationSensor.sampleData();
      if (_oriSensorTempReadCount++ == ORI_SENSOR_TEMP_READ_COUNT) {
        _oriSensorTemperature = orientationSensor.readTemperature();
        // APP_LOGC("[orientation_sensor_task]", "temp: %.2f", _oriSensorTemperature);
        _oriSensorTempReadCount = 0;
      }
    }
    else _orientationSensorTaskPaused = true;
    vTaskDelay(100/portTICK_RATE_MS);
  }
}


//----------------------------------------------
// Mqtt client task
//----------------------------------------------
// the following line must be place after #include "ILI9341.h", 
// as mongoose.h has macro write (s, b, l)
#include "MqttClient.h"
#include "CmdEngine.h"

MqttClient mqtt;
static void mqtt_task(void *pvParams)
{
  CmdEngine cmdEngine;
  mqtt.init();
  mqtt.start();

  cmdEngine.setProtocolDelegate(&mqtt);
  cmdEngine.init();
  cmdEngine.enableUpdate();

  while (true) {
    mqtt.poll();
    vTaskDelay(10 / portTICK_PERIOD_MS);
    // APP_LOGE("[Task]", "task count: %d", uxTaskGetNumberOfTasks());
  }
}


//----------------------------------------------
// Http server task
//----------------------------------------------
// the following line must be place after #include "ILI9341.h", 
// as mongoose.h has macro write (s, b, l)
#include "HttpServer.h"

static void http_task(void *pvParams)
{
  CmdEngine cmdEngine;
  HttpServer server;
  server.init();
  server.start();

  cmdEngine.setProtocolDelegate(&server);
  cmdEngine.init();

  while (true) {
    server.poll();
    // _websocketConnected = server.websocketConnected();
    vTaskDelay(10 / portTICK_PERIOD_MS);
  }
}


//----------------------------------------------
// daemon task
//----------------------------------------------
bool _hasRebootRequest = false;
static void daemon_task(void *pvParams)
{
  while (true) {
    // if (displayTaskHandle) vTaskResume(displayTaskHandle);
    // if (displayTaskHandle) vTaskSuspend(displayTaskHandle);
    if (displayTaskHandle) {
      // uxTaskPriorityGet(displayTaskHandle);
      // APP_LOGC("[daemon_task]", "task schedule state %d", xTaskGetSchedulerState());

      // vTaskGetRunTimeStats(pcWriteBuf);
      // APP_LOGC("[daemon_task]", "display task priority %d", uxTaskPriorityGet(displayTaskHandle));
      // APP_LOGC("[daemon_task]", "display task state %d", eTaskGetState(displayTaskHandle));
    }
    if (_hasRebootRequest && !mqtt.hasUnackPub()) System::instance()->restart();
    vTaskDelay(100 / portTICK_PERIOD_MS);
  }
}


//**********************************************
// prepare env
//**********************************************
#include "Semaphore.h"

static void beforeCreateTasks()
{
  // _dcUpdateSemaphore = xSemaphoreCreateMutex();
  Semaphore::init();
}


/////////////////////////////////////////////////////////////////////////////////////////
// enum types
/////////////////////////////////////////////////////////////////////////////////////////
static const char * const DeployModeStr[] = {
  "HTTPServerMode",                  // 0
  "MQTTClientMode",                  // 1
  "MQTTClientAndHTTPServerMode"      // 2
};

const char * deployModeStr(DeployMode mode)
{
  return DeployModeStr[mode];
}


/////////////////////////////////////////////////////////////////////////////////////////
// Sytem class
/////////////////////////////////////////////////////////////////////////////////////////
#include "NvsFlash.h"
#include "esp_system.h"
#include <string.h>
#include "SensorConfig.h"

static char MAC_ADDR[13] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0xFF };

static void _initMacADDR()
{
  uint8_t macAddr[6];
  char *target = MAC_ADDR;
  esp_efuse_mac_get_default(macAddr);
  for (int i = 0; i < 6; ++i) {
    sprintf(target, "%02x", macAddr[i]);
    target += 2;
  }
  MAC_ADDR[12] = '\0';
}

static System _sysInstance;

System * System::instance()
{
  return &_sysInstance;
}

System::System()
: _state(Uninitialized)
, _configNeedToSave(false)
{
  _setDefaultConfig();
}

void System::_setDefaultConfig()
{
  _config.wifiOn = true;
  _config.displayAutoAdjustOn = true;
  _config.deployMode = HTTPServerMode;
  _config.pmSensorType = PMS5003ST;
  _config.co2SensorType = DSCO220;
  _config.devCapability = ( capabilityForSensorType(_config.pmSensorType) |
                            capabilityForSensorType(_config.co2SensorType) );
  _config.devCapability |= DEV_BUILD_IN_CAPABILITY_MASK;
}

void System::init()
{
  _state = Initializing;
  NvsFlash::init();
  _initMacADDR();
  _loadConfig();
  _launchTasks();
  _state = Running;
}

// configMAX_PRIORITIES defined in "FreeRTOSConfig.h"
#define DISPLAY_TASK_PRIORITY               4
#define WIFI_TASK_PRIORITY                  3
#define SNTP_TASK_PRIORITY                  3
#define MQTTCLIENT_TASK_PRIORITY            3
#define HTTPSERVER_TASK_PRIORITY            3
#define PM_SENSOR_TASK_PRIORITY             3
#define CO2_SENSOR_TASK_PRIORITY            3
#define SHT3X_TASK_PRIORITY                 3
#define TSL2561_TASK_PRIORITY               3
#define ORIENTATION_TASK_PRIORITY           3
#define STATUS_CHECK_TASK_PRIORITY          3
#define TOUCH_PAD_TASK_PRIORITY             3
#define DAEMON_TASK_PRIORITY                3

#define PRO_CORE    0
#define APP_CORE    1

#define RUN_ON_CORE APP_CORE

void System::_launchTasks()
{
  beforeCreateTasks();

  xTaskCreatePinnedToCore(display_task, "display_task", 8192, NULL, DISPLAY_TASK_PRIORITY, &displayTaskHandle, PRO_CORE);

  xTaskCreatePinnedToCore(sht3x_sensor_task, "sht3x_sensor_task", 4096, NULL, SHT3X_TASK_PRIORITY, &sht3xSensorTaskHandle, RUN_ON_CORE);

  if (_config.devCapability & PM_CAPABILITY_MASK)
    xTaskCreatePinnedToCore(pm_sensor_task, "pm_sensor_task", 4096, NULL, PM_SENSOR_TASK_PRIORITY, &pmSensorTaskHandle, RUN_ON_CORE);
  if (_config.devCapability & CO2_CAPABILITY_MASK)
    xTaskCreatePinnedToCore(co2_sensor_task, "co2_sensor_task", 4096, NULL, CO2_SENSOR_TASK_PRIORITY, &co2SensorTaskHandle, RUN_ON_CORE);

  xTaskCreatePinnedToCore(tsl2561_sensor_task, "tsl2561_sensor_task", 4096, NULL, TSL2561_TASK_PRIORITY, &tsl2561SensorTaskHandle, RUN_ON_CORE);

  xTaskCreatePinnedToCore(orientation_sensor_task, "orientation_sensor_task", 4096, NULL, ORIENTATION_TASK_PRIORITY, &orientationSensorTaskHandle, RUN_ON_CORE);

  xTaskCreatePinnedToCore(status_check_task, "status_check_task", 2048, NULL, STATUS_CHECK_TASK_PRIORITY, NULL, RUN_ON_CORE);

  xTaskCreate(&wifi_task, "wifi_connection_task", 4096, NULL, WIFI_TASK_PRIORITY, &wifiTaskHandle);
  vTaskDelay(100 / portTICK_PERIOD_MS);

  xTaskCreate(&sntp_task, "sntp_task", 4096, NULL, SNTP_TASK_PRIORITY, &sntpTaskHandle);
  vTaskDelay(100 / portTICK_PERIOD_MS);

  if (_config.deployMode == MQTTClientMode || _config.deployMode == MQTTClientAndHTTPServerMode)
    xTaskCreatePinnedToCore(mqtt_task, "mqtt_task", 8192, NULL, MQTTCLIENT_TASK_PRIORITY, NULL, RUN_ON_CORE);
  else if (_config.deployMode == HTTPServerMode || _config.deployMode == MQTTClientAndHTTPServerMode)
    xTaskCreatePinnedToCore(http_task, "http_task", 8192, NULL, HTTPSERVER_TASK_PRIORITY, NULL, RUN_ON_CORE);

  // xTaskCreatePinnedToCore(touch_pad_task, "touch_pad_task", 2048, NULL, TOUCH_PAD_TASK_PRIORITY, NULL, RUN_ON_CORE);

  xTaskCreatePinnedToCore(daemon_task, "daemon_task", 2048, NULL, DAEMON_TASK_PRIORITY, NULL, RUN_ON_CORE);
}

void System::pausePeripherals()
{
  _enablePeripheralTaskLoop = false;

  while (!_displayTaskPaused || !_statusTaskPaused ||
         !_pmSensorTaskPaused || (co2SensorTaskHandle && !_co2SensorTaskPaused) ||
         !_orientationSensorTaskPaused || !_sht3xSensorTaskPaused || !_tsl2561SensorTaskPaused) {
    // APP_LOGC("[System]", "pause dis: %d, sta: %d, pm: %d, co2: %d, ori: %d, sht: %d, tsl: %d",
    //   _displayTaskPaused, _statusTaskPaused, _pmSensorTaskPaused, !co2SensorTaskHandle || _co2SensorTaskPaused,
    //   _orientationSensorTaskPaused, _statusTaskPaused, _tsl2561SensorTaskPaused);
    vTaskDelay(100 / portTICK_PERIOD_MS);
    // APP_LOGC("[System]", "pause sync delay");
  }
}

void System::resumePeripherals()
{
  _displayTaskPaused = false;
  _statusTaskPaused = false;
  _pmSensorTaskPaused = false;
  _co2SensorTaskPaused = false;
  _sht3xSensorTaskPaused = false;
  _tsl2561SensorTaskPaused = false;
  _orientationSensorTaskPaused = false;
  _enablePeripheralTaskLoop = true;
}

void System::powerOff()
{
  APP_LOGC("[System]", "power off");
  // save memory to data to flash before power off
  if (_configNeedToSave) _saveConfig();

  // power off
  if (powerManager.powerOff()) {
#ifdef DEBUG_APP_OK
    APP_LOGC("[System]", "power off cmd succeeded");
#endif
  }
#ifdef DEBUG_APP_ERR
  else {
    APP_LOGE("[System]", "pwer off cmd failed");
  }
#endif
}

bool System::displayOn()
{
  return _displayOn;
}

void System::turnWifiOn(bool on)
{
  if (_config.wifiOn != on) {
    _config.wifiOn = on;
    _saveConfig();
  }
}

void System::turnDisplayOn(bool on)
{
  _displayOn = on;
  // DisplayController::activeInstance()->turnOn(on);
  dc.turnOn(on);
}

void System::turnDisplayAutoAdjustOn(bool on)
{
  if (_config.displayAutoAdjustOn != on) {
    _config.displayAutoAdjustOn = on;
    _saveConfig();
  }
}

void System::toggleWifi()
{
  _config.wifiOn = !_config.wifiOn;
  _saveConfig();
}

void System::toggleDisplay()
{
  _displayOn = !_displayOn;
  dc.turnOn(_displayOn);
}

void System::markPowerEvent()
{
  _hasPwrEvent = true;
}

#include "nvs.h"
#define SYSTEM_STORAGE_NAMESPACE          "app"
#define SYSTEM_CONFIG_TAG                 "appConf"

bool System::_loadConfig()
{
  bool succeeded = false;
  bool nvsOpened = false;

  nvs_handle nvsHandle;
  esp_err_t err;

  do {
    // open nvs
    err = nvs_open(SYSTEM_STORAGE_NAMESPACE, NVS_READONLY, &nvsHandle);
    if (err == ESP_ERR_NVS_NOT_FOUND) {
      APP_LOGE("[System]", "loadConfig open nvs: storage \"%s\" not found", SYSTEM_STORAGE_NAMESPACE);
      break;
    }
    else if (err != ESP_OK) {
      APP_LOGE("[System]", "loadConfig open nvs failed %d", err);
      break;
    }
    nvsOpened = true;

    // read sys config
    size_t requiredSize = 0;  // value will default to 0, if not set yet in NVS
    err = nvs_get_blob(nvsHandle, SYSTEM_CONFIG_TAG, NULL, &requiredSize);
    if (err == ESP_ERR_NVS_NOT_FOUND) {
      // _setDefaultConfig();  // already set default in constructor
      break;
    }
    if (err != ESP_OK) {
      APP_LOGE("[System]", "loadConfig read \"sys-config-size\" failed %d", err);
      break;
    }
    if (requiredSize != sizeof(_config)) {
      APP_LOGE("[System]", "loadConfig read \"sys-config-size\" got unexpected value");
      break;
    }
    // read previously saved config
    err = nvs_get_blob(nvsHandle, SYSTEM_CONFIG_TAG, &_config, &requiredSize);
    if (err != ESP_OK) {
      // _setDefaultConfig();  // already set default in constructor
      APP_LOGE("[System]", "loadConfig read \"sys-config-content\" failed %d", err);
      break;
    }
    succeeded = true;

  } while(false);

  // close nvs
  if (nvsOpened) nvs_close(nvsHandle);

  return succeeded;
}

bool System::_saveConfig()
{
  bool succeeded = false;
  bool nvsOpened = false;

  nvs_handle nvsHandle;
  esp_err_t err;

  do {
    // open nvs
    err = nvs_open(SYSTEM_STORAGE_NAMESPACE, NVS_READWRITE, &nvsHandle);
    if (err != ESP_OK) {
      APP_LOGE("[System]", "saveConfig open nvs failed %d", err);
      break;
    }
    nvsOpened = true;

    // write wifi config
    err = nvs_set_blob(nvsHandle, SYSTEM_CONFIG_TAG, &_config, sizeof(_config));
    if (err != ESP_OK) {
      APP_LOGE("[System]", "saveConfig write \"system-config\" failed %d", err);
      break;
    }

    // commit written value.
    err = nvs_commit(nvsHandle);
    if (err != ESP_OK) {
      ESP_LOGE("[System]", "saveConfig commit failed %d", err);
      break;
    }
    succeeded = true;

  } while(false);

  // close nvs
  if (nvsOpened) nvs_close(nvsHandle);

  _configNeedToSave = false; // ? or _configNeedToSave = !succeeded;

  return succeeded;
}

void System::_updateConfig(bool saveImmedidately)
{
  if (saveImmedidately)
    _saveConfig();
  else
    _configNeedToSave = true; // save when reboot or power off
}

void System::setDeployMode(DeployMode mode)
{
  if (_config.deployMode != mode) {
    _config.deployMode = mode;
    _saveConfig();
  }
}

void System::toggleDeployMode()
{
  if (_config.deployMode == HTTPServerMode) {
    _config.deployMode = MQTTClientMode;
  }
  else {
    _config.deployMode = HTTPServerMode;
  }
  _config.wifiOn = true; // when deploy mode changed, always turn on wifi
  _saveConfig();
  restart();
}

void System::setSensorType(SensorType pmType, SensorType co2Type)
{
  if (_config.pmSensorType != pmType || _config.co2SensorType != co2Type) {
    _config.pmSensorType = pmType;
    _config.co2SensorType = co2Type;
    _config.devCapability = ( capabilityForSensorType(_config.pmSensorType) |
                              capabilityForSensorType(_config.co2SensorType) );
    _config.devCapability |= DEV_BUILD_IN_CAPABILITY_MASK;
    _saveConfig();
  }
}

void System::setDevCapability(uint32_t cap)
{
  if (_config.devCapability != cap) {
    _config.devCapability = cap;
    _saveConfig();
  }
}

DeployMode System::deployMode()
{
  return _config.deployMode;
}

SensorType System::pmSensorType()
{
  return _config.pmSensorType;
}

SensorType System::co2SensorType()
{
  return _config.co2SensorType;
}

uint32_t System::devCapability()
{
  return _config.devCapability;
}

const char* System::uid()
{
  return macAddress();
}

const char* System::macAddress()
{
  if (MAC_ADDR[12] == 0xFF)
    _initMacADDR();
  return MAC_ADDR;
}

const char* System::idfVersion()
{
  return esp_get_idf_version();
}

const char* System::firmwareVersion()
{
  return "1.0";
}

static char MODEL[20];

const char* System::model()
{
  sprintf(MODEL, sensorTypeStr(_config.pmSensorType));
  sprintf(MODEL+strlen(MODEL), "-");
  sprintf(MODEL+strlen(MODEL), sensorTypeStr(_config.co2SensorType));
  return MODEL;
}

void System::setRestartRequest()
{
  _hasRebootRequest = true;
}

void System::restart()
{
  // save those need to save ...
  if (_configNeedToSave) _saveConfig();

  // stop those need to stop ...
  pausePeripherals();

  _state = Restarting;
  esp_restart();
}

bool System::restarting()
{
  return _state == Restarting;
}
