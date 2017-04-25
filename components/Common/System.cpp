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

Wifi wifi;

TaskHandle_t wifiTaskHandle;

void wifi_task(void *pvParameters)
{
    // config and start wifi
    if (wifi.loadConfig()) {
      APP_LOGI("wifi", "load config succeeded");
    }
    else {
      wifi.setDefaultConfig();
      wifi.setStaConfig("woody@home", "58897@mljd-abcde");
      if (wifi.saveConfig()) {
        APP_LOGI("wifi", "save config succeeded");
      }
    }
    wifi.init();
    wifi.start(true);
    vTaskDelete(wifiTaskHandle);

    // while (true) {
    //     vTaskDelay(2000 / portTICK_PERIOD_MS);
    //     if (wifi.connected()) vTaskDelete(wifiTaskHandle);
    // }
}


//----------------------------------------------
// display tasks
//----------------------------------------------
// #include "ILI9341.h"
#include "ST7789V.h"
#include "SensorDisplayController.h"
// ILI9341 dev;
ST7789V dev;
SensorDisplayController dc(&dev);
// static xSemaphoreHandle _dcUpdateSemaphore = 0;
// #define DC_UPDATE_SEMAPHORE_TAKE_WAIT_TICKS 1000

void display_task(void *p)
{
    dc.init();
    // _dcUpdateSemaphore = xSemaphoreCreateMutex();
    while (true) {
        // if (xSemaphoreTake(_dcUpdateSemaphore, DC_UPDATE_SEMAPHORE_TAKE_WAIT_TICKS)) {
            dc.update();
        //     xSemaphoreGive(_dcUpdateSemaphore);
        // }
        vTaskDelay(30/portTICK_RATE_MS);
    }
}


//----------------------------------------------
// sensor tasks
//----------------------------------------------
#include "PMSensor.h"
#include "OrientationSensor.h"
#include "SensorDataPacker.h"

void pm_sensor_task(void *p)
{
    PMSensor pmSensor;
    pmSensor.init();
    pmSensor.setDisplayDelegate(&dc);
    SensorDataPacker::sharedInstance()->setPmSensor(&pmSensor);
    while (true) {
        // if (xSemaphoreTake(_dcUpdateSemaphore, DC_UPDATE_SEMAPHORE_TAKE_WAIT_TICKS)) {
            pmSensor.sampleData();
            // xSemaphoreGive(_dcUpdateSemaphore);
        // }
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
        //     xSemaphoreGive(_dcUpdateSemaphore);
        // }
        vTaskDelay(250/portTICK_RATE_MS);
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
// Mongoose task
//----------------------------------------------
// the following line must be place after #include "ILI9341.h", 
// as mongoose.h has macro write (s, b, l)
#include "MqttClient.h"
#include "CmdEngine.h"

static void mongoose_task(void *pvParams)
{
    CmdEngine cmdEngine;
    MqttClient mqtt;
    mqtt.init();
    // mqtt.addSubTopic("/mqtttest", 0);
    mqtt.start();

    cmdEngine.setMqttClientDelegate(&mqtt);
    cmdEngine.init();

    while (true) {
        mqtt.poll();
        vTaskDelay(10 / portTICK_PERIOD_MS);
    }
}


/////////////////////////////////////////////////////////////////////////////////////////
// Sytem class
/////////////////////////////////////////////////////////////////////////////////////////
#include "System.h"
#include "NvsFlash.h"
#include "esp_system.h"
#include <string.h>

static char MAC_ADDR[13] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0xFF };

static void _initMacADDR()
{
    uint8_t macAddr[6];
    char *target = MAC_ADDR;
    esp_efuse_read_mac(macAddr);
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
, _mode(HTTPServerMode)
{}

void System::init()
{
    _state = Initializing;
    NvsFlash::init();
    _initMacADDR();
    _loadConfig();
    _launchTasks();
    _state = Running;
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
        if (err != ESP_OK) {
            APP_LOGE("[System]", "loadConfig open nvs failed %d", err);
            break;
        }
        nvsOpened = true;

        // read sys config
        size_t requiredSize = 0;  // value will default to 0, if not set yet in NVS
        err = nvs_get_blob(nvsHandle, SYSTEM_CONFIG_TAG, NULL, &requiredSize);
        if (err == ESP_ERR_NVS_NOT_FOUND) {
            _mode = HTTPServerMode;
            break;
        }
        if (err != ESP_OK) {
            APP_LOGE("[System]", "loadConfig read \"sys-config-size\" failed %d", err);
            break;
        }
        if (requiredSize != sizeof(_mode)) {
            APP_LOGE("[System]", "loadConfig read \"sys-config-size\" got unexpected value");
            break;
        }
        // read previously saved config
        err = nvs_get_blob(nvsHandle, SYSTEM_CONFIG_TAG, &_mode, &requiredSize);
        if (err != ESP_OK) {
            _mode = MQTTClientMode;
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
        err = nvs_set_blob(nvsHandle, SYSTEM_CONFIG_TAG, &_mode, sizeof(_mode));
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

void System::setConfigMode(ConfigMode mode)
{
    if (_mode != mode) {
        _mode = mode;
        _saveConfig();
    }
}

#define DISPLAY_TASK_PRIORITY               100
#define WIFI_TASK_PRIORITY                  50
#define SNTP_TASK_PRIORITY                  40
#define MQTTCLIENT_TASK_PRIORITY            30
#define PM_SENSOR_TASK_PRIORITY             80
#define ORIENTATION_TASK_PRIORITY           81

void System::_launchTasks()
{
    xTaskCreate(&display_task, "display_task", 8192, NULL, DISPLAY_TASK_PRIORITY, NULL);
    xTaskCreate(&wifi_task, "wifi_connection_task", 4096, NULL, WIFI_TASK_PRIORITY, &wifiTaskHandle);
    vTaskDelay(100 / portTICK_PERIOD_MS);
    xTaskCreate(&sntp_task, "sntp_task", 4096, NULL, SNTP_TASK_PRIORITY, &sntpTaskHandle);
    vTaskDelay(100 / portTICK_PERIOD_MS);
    xTaskCreate(&mongoose_task, "mongoose_task", 8192, NULL, MQTTCLIENT_TASK_PRIORITY, NULL);
    xTaskCreate(pm_sensor_task, "pm_sensor_task", 4096, NULL, PM_SENSOR_TASK_PRIORITY, NULL);
    xTaskCreate(orientation_sensor_task, "orientation_sensor_task", 4096, NULL, ORIENTATION_TASK_PRIORITY, NULL);
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

void System::restart()
{
	_state = Restarting;
	esp_restart();
}

bool System::restarting()
{
	return _state == Restarting;
}
