/*
 * NvsFlash: Wrap nvs flash
 * Copyright (c) 2017 Shenghua Su
 *
 */

#include "NvsFlash.h"
#include "esp_err.h"
#include "nvs.h"
#include "nvs_flash.h"
#include "esp_partition.h"

bool NvsFlash::_inited = false;

void NvsFlash::init()
{
    if (!_inited) {
        // Initialize NVS.
        esp_err_t err = nvs_flash_init();
        if (err == ESP_ERR_NVS_NO_FREE_PAGES) {
            // OTA app partition table has a smaller NVS partition size than the non-OTA
            // partition table. This size mismatch may cause NVS initialization to fail.
            // If this happens, we erase NVS partition and initialize NVS again.
            const esp_partition_t* nvs_partition = esp_partition_find_first(
                    ESP_PARTITION_TYPE_DATA, ESP_PARTITION_SUBTYPE_DATA_NVS, NULL);
            assert(nvs_partition && "partition table must have an NVS partition");
            ESP_ERROR_CHECK( esp_partition_erase_range(nvs_partition, 0, nvs_partition->size) );
            err = nvs_flash_init();
        }
        ESP_ERROR_CHECK( err );
        _inited = (err == ESP_OK);   
    }
}
