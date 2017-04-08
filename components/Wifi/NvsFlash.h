/*
 * NvsFlash: Wrap nvs flash
 * Copyright (c) 2017 Shenghua Su
 *
 */

#ifndef _NVS_FLASH_H
#define _NVS_FLASH_H

#include "esp_err.h"
#include "nvs_flash.h"

class NvsFlash
{
public:
    static void init() {
        if (!_inited) {
            ESP_ERROR_CHECK( nvs_flash_init() );
            _inited = true;
        }
    }
    static bool inited() { return _inited; }

protected:
    static bool _inited;
};

#endif // _NVS_FLASH_H
