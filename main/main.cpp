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

// the following line must be place after #include "ILI9341.h", 
// as mongoose.h has macro write (s, b, l)
#include "Mongoose.h"

/////////////////////////////////////////////////////////////////////////////////////////
// Wifi task
/////////////////////////////////////////////////////////////////////////////////////////
#include "tcpip_adapter.h"

Wifi wifi;

TaskHandle_t enterpriseTaskHandle;

void wpa2_enterprise_task(void *pvParameters)
{
    tcpip_adapter_ip_info_t ip;
    memset(&ip, 0, sizeof(tcpip_adapter_ip_info_t));
    vTaskDelay(2000 / portTICK_PERIOD_MS);

    while (true) {
        vTaskDelay(2000 / portTICK_PERIOD_MS);

        if (wifi.connected()) {

          if (tcpip_adapter_get_ip_info(TCPIP_ADAPTER_IF_STA, &ip) == 0) {
              ESP_LOGI("wifi test", "~~~~~~~~~~~");
              ESP_LOGI("wifi test", "  IP: %d.%d.%d.%d", IP2STR(&ip.ip));
              ESP_LOGI("wifi test", "MASK: %d.%d.%d.%d", IP2STR(&ip.netmask));
              ESP_LOGI("wifi test", "  GW: %d.%d.%d.%d", IP2STR(&ip.gw));
              ESP_LOGI("wifi test", "~~~~~~~~~~~");
              vTaskDelete(enterpriseTaskHandle);
          }
        }
    }
}

/////////////////////////////////////////////////////////////////////////////////////////
// Mongoose task
/////////////////////////////////////////////////////////////////////////////////////////

// #define MG_TASK_STACK_SIZE 4096
// #define MG_TASK_PRIORITY   1

static void mongoose_task(void *pvParams)
{
    Mongoose mongoose;
    mongoose.init();
    while (true) {
        mongoose.poll();
    }
}


/////////////////////////////////////////////////////////////////////////////////////////
// sensor tasks
/////////////////////////////////////////////////////////////////////////////////////////
ILI9341 dev;
SensorDisplayController dc(&dev);

void pm_sensor_task(void *p)
{
    PMSensor pmSensor;
    pmSensor.init();
    pmSensor.setDisplayDelegate(&dc);
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

void app_main()
{
    NvsFlash::init();

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
    wifi.start();

    dc.init();

    xTaskCreate(&wpa2_enterprise_task, "wpa2_enterprise_task", 4096, NULL, 5, &enterpriseTaskHandle);
    xTaskCreate(&mongoose_task, "mongoose_task", 4096, NULL, 5, NULL);
    xTaskCreate(pm_sensor_task, "pm_sensor_task", 4096, NULL, 10, NULL);
    xTaskCreate(orientation_sensor_task, "orientation_sensor_task", 4096, NULL, 10, NULL);

    while (true) {
        dc.update();
        vTaskDelay(15/portTICK_RATE_MS);
    }
}

#ifdef __cplusplus
}
#endif
