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
  Wifi::instance()->start(true);
  vTaskDelete(wifiTaskHandle);
}


//----------------------------------------------
// display tasks
//----------------------------------------------
// #include "ILI9341.h"
#include "ST7789V.h"
#include "SensorDisplayController.h"
// static ILI9341 dev;
ST7789V dev;
static SensorDisplayController dc(&dev);
// static xSemaphoreHandle _dcUpdateSemaphore = 0;
// #define DC_UPDATE_SEMAPHORE_TAKE_WAIT_TICKS 1000
TaskHandle_t displyTaskHandle = 0;

void display_task(void *p)
{
  dc.init();
  while (true) {
    // if (xSemaphoreTake(_dcUpdateSemaphore, DC_UPDATE_SEMAPHORE_TAKE_WAIT_TICKS)) {
      dc.update();
      // xSemaphoreGive(_dcUpdateSemaphore);
    // }
    // APP_LOGE("[display_task]", "alive");
    vTaskDelay(30/portTICK_RATE_MS);
  }
}


//----------------------------------------------
// status check tasks
//----------------------------------------------
#include "Adc.h"
#define SAMPLE_ACTIVE_COUNT     2 // 10 seconds
Adc _voltageReader;
uint8_t _sampleActiveCounter = SAMPLE_ACTIVE_COUNT;
#define CALCULATE_AVERAGE_COUNT 10
uint8_t _sampleCount = 0;
float   _sampleValue = 0;
void status_check_task(void *p)
{
  _voltageReader.init(ADC1_CHANNEL_4);

  while (true) {
    dc.setWifiConnected(Wifi::instance()->connected());
    dc.setTimeUpdate(true);
    // battery voltage read
    // if (_sampleActiveCounter == SAMPLE_ACTIVE_COUNT) {
    //   int tmp = _voltageReader.readVoltage();
    //   _sampleValue += tmp;
    //   APP_LOGC("[ADC test]", "sample(%d): %d", _sampleCount, tmp);

    //   ++_sampleCount;
    //   if (_sampleCount == CALCULATE_AVERAGE_COUNT) {
    //     _sampleValue /= _sampleCount;
    //     APP_LOGC("[ADC test]", "--> average sample: %.0f", _sampleValue);
    //     _sampleValue = 0;
    //     _sampleCount = 0;
    //   }
    //   _sampleActiveCounter = 0;
    // } else {
    //   ++_sampleActiveCounter;
    // }
    vTaskDelay(500/portTICK_RATE_MS);
  }
}


//----------------------------------------------
// sensor tasks
//----------------------------------------------
#include "PMSensor.h"
#include "CO2Sensor.h"
#include "OrientationSensor.h"
#include "SensorDataPacker.h"

void pm_sensor_task(void *p)
{
  PMSensor pmSensor;
  pmSensor.init();
  pmSensor.setDisplayDelegate(&dc);
  SensorDataPacker::sharedInstance()->init();
  SensorDataPacker::sharedInstance()->setPmSensor(&pmSensor);
  while (true) {
    // if (xSemaphoreTake(_dcUpdateSemaphore, DC_UPDATE_SEMAPHORE_TAKE_WAIT_TICKS)) {
      pmSensor.sampleData();
      // xSemaphoreGive(_dcUpdateSemaphore);
    // }
    vTaskDelay(500/portTICK_RATE_MS);
  }
}

void co2_sensor_task(void *p)
{
  CO2Sensor co2Sensor;
  co2Sensor.init();
  co2Sensor.setDisplayDelegate(&dc);
  SensorDataPacker::sharedInstance()->setCO2Sensor(&co2Sensor);

  vTaskDelay(3000/portTICK_RATE_MS); // delay 3 seconds
  while (true) {
    co2Sensor.sampleData(3000);
    vTaskDelay(500/portTICK_RATE_MS);
  }
}

void orientation_sensor_task(void *p)
{
  OrientationSensor orientationSensor;
  orientationSensor.init();
  orientationSensor.setDisplayDelegate(&dc);
  while (true) {
    // if (xSemaphoreTake(_dcUpdateSemaphore, DC_UPDATE_SEMAPHORE_TAKE_WAIT_TICKS)) {
      orientationSensor.tick();
      // xSemaphoreGive(_dcUpdateSemaphore);
    // }
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
#include "System.h"
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
  _config.deployMode = MQTTClientMode;
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

#define PRO_CORE    0
#define APP_CORE    1

#define RUN_ON_CORE APP_CORE

void System::_launchTasks()
{
  beforeCreateTasks();

  xTaskCreatePinnedToCore(&display_task, "display_task", 8192, NULL, DISPLAY_TASK_PRIORITY, &displyTaskHandle, RUN_ON_CORE);

  xTaskCreate(&wifi_task, "wifi_connection_task", 4096, NULL, WIFI_TASK_PRIORITY, &wifiTaskHandle);
  vTaskDelay(100 / portTICK_PERIOD_MS);

  xTaskCreate(&sntp_task, "sntp_task", 4096, NULL, SNTP_TASK_PRIORITY, &sntpTaskHandle);
  vTaskDelay(100 / portTICK_PERIOD_MS);

  if (_config.deployMode == MQTTClientMode || _config.deployMode == MQTTClientAndHTTPServerMode)
    xTaskCreatePinnedToCore(&mqtt_task, "mqtt_task", 8192, NULL, MQTTCLIENT_TASK_PRIORITY, NULL, RUN_ON_CORE);
  if (_config.deployMode == HTTPServerMode || _config.deployMode == MQTTClientAndHTTPServerMode)
    xTaskCreatePinnedToCore(&http_task, "http_task", 8192, NULL, HTTPSERVER_TASK_PRIORITY, NULL, RUN_ON_CORE);

  if (_config.devCapability & PM_CAPABILITY_MASK)
    xTaskCreatePinnedToCore(pm_sensor_task, "pm_sensor_task", 4096, NULL, PM_SENSOR_TASK_PRIORITY, NULL, RUN_ON_CORE);
  if (_config.devCapability & CO2_CAPABILITY_MASK)
    xTaskCreatePinnedToCore(co2_sensor_task, "co2_sensor_task", 2048, NULL, CO2_SENSOR_TASK_PRIORITY, NULL, RUN_ON_CORE);

  xTaskCreatePinnedToCore(orientation_sensor_task, "orientation_sensor_task", 4096, NULL, ORIENTATION_TASK_PRIORITY, NULL, RUN_ON_CORE);

  xTaskCreatePinnedToCore(status_check_task, "status_check_task", 4096, NULL, STATUS_CHECK_TASK_PRIORITY, NULL, RUN_ON_CORE);
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
