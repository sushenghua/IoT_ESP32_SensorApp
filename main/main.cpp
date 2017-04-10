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
#include "OrientationSensor.h"

#ifdef __cplusplus
extern "C" {
#endif

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

#include "mongoose.h"

#include "esp_system.h"

#define MG_LISTEN_ADDR "80"
#define MG_TASK_STACK_SIZE 4096 /* bytes */
#define MG_TASK_PRIORITY 1

void mongoose_event_handler(struct mg_connection *nc, int ev, void *p)
{
  static const char *reply_fmt =
      "HTTP/1.0 200 OK\r\n"
      "Connection: close\r\n"
      "Content-Type: text/plain\r\n"
      "\r\n"
      "Sensor %s\n";

  switch (ev) {
    case MG_EV_ACCEPT: {
      char addr[32];
      mg_sock_addr_to_str(&nc->sa, addr, sizeof(addr),
                          MG_SOCK_STRINGIFY_IP | MG_SOCK_STRINGIFY_PORT);
      printf("Connection %p from %s\n", nc, addr);
      break;
    }
    case MG_EV_HTTP_REQUEST: {
      char addr[32];
      struct http_message *hm = (struct http_message *) p;
      mg_sock_addr_to_str(&nc->sa, addr, sizeof(addr),
                          MG_SOCK_STRINGIFY_IP | MG_SOCK_STRINGIFY_PORT);
      printf("HTTP request from %s: %.*s %.*s\n", addr, (int) hm->method.len,
             hm->method.p, (int) hm->uri.len, hm->uri.p);
      // mg_printf(nc, reply_fmt, addr);
      mg_printf(nc, reply_fmt, addr);
      nc->flags |= MG_F_SEND_AND_CLOSE;
      break;
    }
    case MG_EV_CLOSE: {
      printf("Connection %p closed\n", nc);
      break;
    }
  }
}

static void mongoose_task(void *pvParams)
{
  struct mg_mgr mgr;
  struct mg_connection *nc;

  ESP_LOGI("[Mongoose]", "version: %s", MG_VERSION);
  ESP_LOGI("[Mongoose]", "Free RAM: %d", esp_get_free_heap_size());

  mg_mgr_init(&mgr, NULL);

  nc = mg_bind(&mgr, MG_LISTEN_ADDR, mongoose_event_handler);
  if (nc == NULL) {
    ESP_LOGE("[Mongoose]", "setting up listener failed");
    return;
  }
  mg_set_protocol_http_websocket(nc);

  while (true) {
    mg_mgr_poll(&mgr, 1000);
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
