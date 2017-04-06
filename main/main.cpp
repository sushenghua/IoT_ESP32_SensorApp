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

#ifdef __cplusplus
extern "C"
{
#endif

#ifndef delay(x)
#define delay(x)                 vTaskDelay((x)/portTICK_RATE_MS)
#endif


ILI9341 dev;
SensorDisplayController dc(&dev);

void app_main()
{
    
    dc.init();

    while (true) {
        dc.forceUpdate();
        dc.update();
        // delay(10);
    }

    // ILI9341 dev;
    // dev.init();
    // dev.test();
    // while (1) {
    //     dev.test();
    // }
}

#ifdef __cplusplus
}
#endif
