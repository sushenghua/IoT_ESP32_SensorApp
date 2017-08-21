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
#include "AppLog.h"
#include "System.h"

//----------------------------------------------
// peripheral task loop control
//----------------------------------------------
bool     _enablePeripheralTaskLoop = true;
uint8_t  _peripheralsStopCount = 0;

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
    Wifi::instance()->setStaConfig("woody@home", "58897@mljd-abcde");
    Wifi::instance()->appendAltApConnectionSsidPassword("iPhone6S", "abcd1234");
    if (Wifi::instance()->saveConfig()) {
      APP_LOGI("[Wifi]", "save config succeeded");
    }
  }
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
// display tasks
//----------------------------------------------
// #include "ST7789V.h"
// ST7789V dev;// static ILI9341 dev;
#include "ILI9341.h"
#include "SensorDisplayController.h"
ILI9341 dev;
bool    _displayOn = true;
static SensorDisplayController dc(&dev);
// static xSemaphoreHandle _dcUpdateSemaphore = 0;
// #define DC_UPDATE_SEMAPHORE_TAKE_WAIT_TICKS 1000
TaskHandle_t displyTaskHandle = 0;
bool _displayTaskPaused = false;
void display_task(void *p)
{
  dc.init();
  while (true) {
    if (_enablePeripheralTaskLoop) dc.update();
    else _displayTaskPaused = true;
    vTaskDelay(30/portTICK_RATE_MS);
  }
}


//----------------------------------------------
// status check tasks
//----------------------------------------------
#include "Adc.h"
#include "InputMonitor.h"

#define STATUS_TASK_DELAY_UNIT  100

#define SAMPLE_ACTIVE_COUNT     2    // 0.2 seconds
#define CALCULATE_AVERAGE_COUNT 5
#define BAT_VOLTAGE_CORRECTION  0.4f // issue: 3.3 gives 4095, 0.0 gives 0, but 1.8 does not produce 2234
#define BAT_VOLTAGE_MAX         4.2f
#define BAT_VOLTAGE_MIN         3.2f
Adc     _voltageReader;
uint8_t _sampleActiveCounter = SAMPLE_ACTIVE_COUNT;
uint8_t _sampleCount = 0;
float   _sampleValue = 0;
float   _batVoltage = 0;

#define TIME_WIFI_UPDATE_COUNT  5
uint8_t _timeWifiUpdateCount = 0;

bool _statusTaskPaused = false;
void status_check_task(void *p)
{
  _voltageReader.init(ADC1_CHANNEL_4);
  InputMonitor::instance()->init();   // this will launch another task

  while (true) {
    if (_enablePeripheralTaskLoop) {
      // update time and wifi status every 0.5 second
      ++_timeWifiUpdateCount;
      if (_timeWifiUpdateCount >= TIME_WIFI_UPDATE_COUNT) {
        dc.setWifiConnected(Wifi::instance()->connected());
        dc.setTimeUpdate(true);
        _timeWifiUpdateCount = 0;
      }

      // battery voltage read
      ++_sampleActiveCounter;
      if (_sampleActiveCounter >= SAMPLE_ACTIVE_COUNT) {
        int tmp = _voltageReader.readVoltage();
        _sampleValue += tmp;
        // APP_LOGC("[ADC test]", "sample(%d): %d", _sampleCount, tmp);
        ++_sampleCount;
        if (_sampleCount == CALCULATE_AVERAGE_COUNT) {
          _sampleValue /= _sampleCount;
          _batVoltage = _sampleValue * 6.6f / 4095 + BAT_VOLTAGE_CORRECTION;
          dc.setBatteryLevel((_batVoltage - BAT_VOLTAGE_MIN) / (BAT_VOLTAGE_MAX - BAT_VOLTAGE_MIN) * 100);
          // APP_LOGC("[ADC test]", "--> average sample: %.0f, voltage: %.2f", _sampleValue, _batVoltage);
          _sampleValue = 0;
          _sampleCount = 0;
        }
        _sampleActiveCounter = 0;
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
#include "PMSensor.h"
#include "CO2Sensor.h"
#include "OrientationSensor.h"
#include "SensorDataPacker.h"

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

TaskHandle_t co2SensorTaskHandle;
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
    vTaskDelay(500/portTICK_RATE_MS);
  }
}

TaskHandle_t orientationSensorTaskHandle;
bool _orientationSensorTaskPaused = false;
void orientation_sensor_task(void *p)
{
  OrientationSensor orientationSensor;
  orientationSensor.init();
  orientationSensor.setDisplayDelegate(&dc);
  while (true) {
    if (_enablePeripheralTaskLoop) orientationSensor.tick();
    else _orientationSensorTaskPaused = true;
    vTaskDelay(100/portTICK_RATE_MS);
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
// Mqtt client task
//----------------------------------------------
// the following line must be place after #include "ILI9341.h", 
// as mongoose.h has macro write (s, b, l)
#include "MqttClient.h"
#include "CmdEngine.h"

static void mqtt_task(void *pvParams)
{
  CmdEngine cmdEngine;
  MqttClient mqtt;
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
    vTaskDelay(10 / portTICK_PERIOD_MS);
  }
}


//**********************************************
// prepare env
//**********************************************
static void beforeCreateTasks()
{
  // _dcUpdateSemaphore = xSemaphoreCreateMutex();
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
{
  _setDefaultConfig();
}

void System::_setDefaultConfig()
{
  _config.wifiOn = true;
  _config.deployMode = HTTPServerMode;
  _config.pmSensorType = PMS5003ST;
  _config.co2SensorType = DSCO220;
  _config.devCapability = ( capabilityForSensorType(_config.pmSensorType) |
                            capabilityForSensorType(_config.co2SensorType) );
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

#define DISPLAY_TASK_PRIORITY               100
#define WIFI_TASK_PRIORITY                  50
#define SNTP_TASK_PRIORITY                  40
#define MQTTCLIENT_TASK_PRIORITY            30
#define HTTPSERVER_TASK_PRIORITY            30
#define PM_SENSOR_TASK_PRIORITY             80
#define CO2_SENSOR_TASK_PRIORITY            80
#define ORIENTATION_TASK_PRIORITY           81
#define STATUS_CHECK_TASK_PRIORITY          82
#define TOUCH_PAD_TASK_PRIORITY             81

#define PRO_CORE    0
#define APP_CORE    1

#define RUN_ON_CORE APP_CORE

void System::_launchTasks()
{
  beforeCreateTasks();

  xTaskCreatePinnedToCore(&display_task, "display_task", 8192, NULL, DISPLAY_TASK_PRIORITY, &displyTaskHandle, RUN_ON_CORE);

  if (_config.devCapability & PM_CAPABILITY_MASK)
    xTaskCreatePinnedToCore(pm_sensor_task, "pm_sensor_task", 4096, NULL, PM_SENSOR_TASK_PRIORITY, &pmSensorTaskHandle, RUN_ON_CORE);
  if (_config.devCapability & CO2_CAPABILITY_MASK)
    xTaskCreatePinnedToCore(co2_sensor_task, "co2_sensor_task", 2048, NULL, CO2_SENSOR_TASK_PRIORITY, &co2SensorTaskHandle, RUN_ON_CORE);

  xTaskCreatePinnedToCore(orientation_sensor_task, "orientation_sensor_task", 4096, NULL, ORIENTATION_TASK_PRIORITY, &orientationSensorTaskHandle, RUN_ON_CORE);

  xTaskCreatePinnedToCore(status_check_task, "status_check_task", 4096, NULL, STATUS_CHECK_TASK_PRIORITY, NULL, RUN_ON_CORE);

  xTaskCreate(&wifi_task, "wifi_connection_task", 4096, NULL, WIFI_TASK_PRIORITY, &wifiTaskHandle);
  vTaskDelay(100 / portTICK_PERIOD_MS);

  xTaskCreate(&sntp_task, "sntp_task", 4096, NULL, SNTP_TASK_PRIORITY, &sntpTaskHandle);
  vTaskDelay(100 / portTICK_PERIOD_MS);

  if (_config.deployMode == MQTTClientMode || _config.deployMode == MQTTClientAndHTTPServerMode)
    xTaskCreatePinnedToCore(&mqtt_task, "mqtt_task", 8192, NULL, MQTTCLIENT_TASK_PRIORITY, NULL, RUN_ON_CORE);
  else if (_config.deployMode == HTTPServerMode || _config.deployMode == MQTTClientAndHTTPServerMode)
    xTaskCreatePinnedToCore(&http_task, "http_task", 8192, NULL, HTTPSERVER_TASK_PRIORITY, NULL, RUN_ON_CORE);

  // xTaskCreatePinnedToCore(touch_pad_task, "touch_pad_task", 2048, NULL, TOUCH_PAD_TASK_PRIORITY, NULL, RUN_ON_CORE);
}

void System::pausePeripherals()
{
  _enablePeripheralTaskLoop = false;

  while (!_displayTaskPaused || !_statusTaskPaused ||
         !_pmSensorTaskPaused || !_co2SensorTaskPaused || !_orientationSensorTaskPaused) {
    vTaskDelay(100 / portTICK_PERIOD_MS);
    APP_LOGC("[System]", "pause sync delay");
  }
}

void System::resumePeripherals()
{
  _displayTaskPaused = false;
  _statusTaskPaused = false;
  _pmSensorTaskPaused = false;
  _co2SensorTaskPaused = false;
  _orientationSensorTaskPaused = false;
  _enablePeripheralTaskLoop = true;
}

bool System::wifiOn()
{
  return _config.wifiOn;
}

bool System::displayOn()
{
  return _displayOn;
}

void System::turnWifiOn(bool on)
{
  _config.wifiOn = on;
  _saveConfig();
}

void System::turnDisplayOn(bool on)
{
  _displayOn = on;
  // DisplayController::activeInstance()->turnOn(on);
  dc.turnOn(on);
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

  return succeeded;
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

void System::restart()
{
  _state = Restarting;
  esp_restart();
}

bool System::restarting()
{
  return _state == Restarting;
}
