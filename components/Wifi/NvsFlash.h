/*
 * NvsFlash: Wrap nvs flash
 * Copyright (c) 2017 Shenghua Su
 *
 */

#ifndef _NVS_FLASH_H
#define _NVS_FLASH_H

class NvsFlash
{
public:
    static void init();
    static bool inited() { return _inited; }

protected:
    static bool _inited;
};

#endif // _NVS_FLASH_H
