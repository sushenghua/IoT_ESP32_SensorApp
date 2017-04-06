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
// #include "esp_log.h"

#include "ILI9341.h"
#include "SensorDisplayController.h"
#include "PMSensor.h"

#ifdef __cplusplus
extern "C"
{
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

void app_main()
{
    dc.init();
    // pmSensor.init();
    // pmSensor.setDisplayDelegate(&dc);

    xTaskCreate(sensor_sample_task, "sensor_sample_task", 4096, NULL, 0, NULL);

    while (true) {
        dc.update();
        vTaskDelay(15/portTICK_RATE_MS);
    }
}

#ifdef __cplusplus
}
#endif
