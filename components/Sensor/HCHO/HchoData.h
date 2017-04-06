/*
 * HchoData: description of hcho
 * Copyright (c) 2017 Shenghua Su
 *
 */


#ifndef _HCHO_DATA_H
#define _HCHO_DATA_H

#include "HealthyStandard.h"

struct HchoData {
    // China standard from https://wenku.baidu.com/view/80e13b956bec0975f465e2a1.html
    uint8_t         level;

    // hcho value
    float           hcho;

    // clear method
    void clear() {
        level = 0;
        hcho = 0;
    }

    // calculate level
    void calculateLevel() {
        level = HS::calculateSampleLevel(HS::HCHO_TABLE, hcho, HCHO_MAX_LEVEL);
    }
};

#endif // _HCHO_DATA_H
