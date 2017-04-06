/*
 * CO2Data: description of CO2
 * Copyright (c) 2017 Shenghua Su
 *
 */


#ifndef _CO2DATA_H
#define _CO2DATA_H

#include "HealthyStandard.h"

struct CO2Data {
    // standard from https://zhidao.baidu.com/question/475675906.html
    uint8_t         level;

    // CO2 value
    float           co2;

    // clear method
    void clear() {
        level = 0;
        co2 = 0;
    }

    // calculate level
    void calculateLevel() {
        level = HS::calculateSampleLevel(HS::CO2_TABLE, co2, CO2_MAX_LEVEL);
    }
};

#endif // _CO2DATA_H
