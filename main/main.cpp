/*
 * main: manage tasks of different modules
 * Copyright (c) 2017 Shenghua Su
 *
 */

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "esp_log.h"

#include "NvsFlash.h"
#include "Wifi.h"

#include "ILI9341.h"
#include "SensorDisplayController.h"

#include "PMSensor.h"
#include "OrientationSensor.h"
#include "SensorDataPacker.h"

#include "SNTP.h"

// the following line must be place after #include "ILI9341.h", 
// as mongoose.h has macro write (s, b, l)
#include "MqttClient.h"

#include "CmdEngine.h"

/////////////////////////////////////////////////////////////////////////////////////////
// Wifi task
/////////////////////////////////////////////////////////////////////////////////////////
#include "tcpip_adapter.h"

Wifi wifi;

TaskHandle_t wifiTaskHandle;

void wifi_task(void *pvParameters)
{
    // config and start wifi
    if (wifi.loadConfig()) {
      ESP_LOGI("wifi", "load config succeeded");
    }
    else {
      wifi.setDefaultConfig();
      // if (wifi.saveConfig()) {
      //   ESP_LOGI("wifi", "save config succeeded");
      // }
    }
    wifi.init();
    wifi.start(true);
    vTaskDelete(wifiTaskHandle);

    // while (true) {
    //     vTaskDelay(2000 / portTICK_PERIOD_MS);
    //     if (wifi.connected()) vTaskDelete(wifiTaskHandle);
    // }
}

/////////////////////////////////////////////////////////////////////////////////////////
// SNTP time task
/////////////////////////////////////////////////////////////////////////////////////////
TaskHandle_t sntpTaskHandle;
static void sntp_task(void *pvParams)
{
    SNTP::init();
    SNTP::waitSync();

    SNTP::test();
    vTaskDelete(sntpTaskHandle);
}


/////////////////////////////////////////////////////////////////////////////////////////
// Mongoose task
/////////////////////////////////////////////////////////////////////////////////////////
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
// display tasks
/////////////////////////////////////////////////////////////////////////////////////////
ILI9341 dev;
SensorDisplayController dc(&dev);

void display_task(void *p)
{
    dc.init();

    while (true) {
        dc.update();
        vTaskDelay(15/portTICK_RATE_MS);
    }
}


/////////////////////////////////////////////////////////////////////////////////////////
// sensor tasks
/////////////////////////////////////////////////////////////////////////////////////////
void pm_sensor_task(void *p)
{
    PMSensor pmSensor;
    pmSensor.init();
    pmSensor.setDisplayDelegate(&dc);
    SensorDataPacker::sharedInstance()->setPmSensor(&pmSensor);
    while (true) {
        pmSensor.sampleData();
        vTaskDelay(500/portTICK_RATE_MS);
    }
}

void orientation_sensor_task(void *p)
{
    OrientationSensor orientationSensor;
    orientationSensor.init();
    orientationSensor.setDisplayDelegate(&dc);
    while (true) {
        orientationSensor.tick();
        vTaskDelay(250/portTICK_RATE_MS);
    }
}


/////////////////////////////////////////////////////////////////////////////////////////
// app_main task
/////////////////////////////////////////////////////////////////////////////////////////
#ifdef __cplusplus
extern "C" {
#endif

#define DISPLAY_TASK_PRIORITY               100
#define WIFI_TASK_PRIORITY                  50
#define SNTP_TASK_PRIORITY                  40
#define MQTTCLIENT_TASK_PRIORITY            30
#define PM_SENSOR_TASK_PRIORITY             80
#define ORIENTATION_TASK_PRIORITY           81

void app_main()
{
    NvsFlash::init();

    xTaskCreate(&display_task, "display_task", 4096, NULL, DISPLAY_TASK_PRIORITY, NULL);
    xTaskCreate(&wifi_task, "wifi_connection_task", 4096, NULL, WIFI_TASK_PRIORITY, &wifiTaskHandle);
    vTaskDelay(100 / portTICK_PERIOD_MS);
    xTaskCreate(&sntp_task, "sntp_task", 4096, NULL, SNTP_TASK_PRIORITY, &sntpTaskHandle);
    vTaskDelay(100 / portTICK_PERIOD_MS);
    xTaskCreate(&mongoose_task, "mongoose_task", 4096, NULL, MQTTCLIENT_TASK_PRIORITY, NULL);
    xTaskCreate(pm_sensor_task, "pm_sensor_task", 4096, NULL, PM_SENSOR_TASK_PRIORITY, NULL);
    xTaskCreate(orientation_sensor_task, "orientation_sensor_task", 4096, NULL, ORIENTATION_TASK_PRIORITY, NULL);

    // while (true) {
    //     vTaskDelay(portMAX_DELAY/portTICK_RATE_MS);
    // }
}

#ifdef __cplusplus
}
#endif
