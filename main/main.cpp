/* SPI Master example

   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/
// #include <stdio.h>
// #include <stdlib.h>
// #include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
// #include "esp_system.h"
// #include "driver/spi_master.h"
// #include "soc/gpio_struct.h"
// #include "driver/gpio.h"
#include "esp_log.h"

#include "Wifi.h"
#include "NvsFlash.h"

#include "ILI9341.h"
#include "SensorDisplayController.h"
#include "PMSensor.h"

#ifdef __cplusplus
extern "C" {
#endif

ILI9341 dev;
SensorDisplayController dc(&dev);

void sensor_sample_task(void *p)
{
    PMSensor pmSensor;
    pmSensor.init();
    pmSensor.setDisplayDelegate(&dc);
    while (true) {
        pmSensor.sampleData();
        vTaskDelay(500/portTICK_RATE_MS);
    }
}

Wifi wifi;

#include "tcpip_adapter.h"

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

void app_main()
{
    NvsFlash::init();

    dc.init();

    if (wifi.loadConfig()) {
      ESP_LOGI("wifi", "load config succeeded");
    }
    else {
      // set config
      wifi.setWifiMode(WIFI_MODE_APSTA);
      wifi.setStaConfig("woody@home", "58897@mljd-abcde");
      wifi.setApConfig("DDSensor", "abcd1234");
      wifi.setHostName("SensorApp");
      wifi.enableEap();
      wifi.setEapConfig("eap_test_id", "eap_user_woody", "eap_user_passwd");

      if (wifi.saveConfig()) {
        ESP_LOGI("wifi", "save config succeeded");
      }
    }

    wifi.init();
    wifi.start();

    xTaskCreate(&wpa2_enterprise_task, "wpa2_enterprise_task", 4096, NULL, 5, &enterpriseTaskHandle);
    xTaskCreate(sensor_sample_task, "sensor_sample_task", 4096, NULL, 10, NULL);

    while (true) {
        dc.update();
        vTaskDelay(15/portTICK_RATE_MS);
    }
}

#ifdef __cplusplus
}
#endif
