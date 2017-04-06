/*
 * PMData: description of PM
 * Copyright (c) 2017 Shenghua Su
 *
 */


#ifndef _PMDATA_H
#define _PMDATA_H

#include "HealthyStandard.h"

struct PMData {
    // US standard Air Quality Index from https://en.wikipedia.org/wiki/Air_quality_index
    uint8_t         levelPm2d5US;
    uint8_t         levelPm10US;
    uint16_t        aqiPm2d5US;
    uint16_t        aqiPm10US;

    // China standard Air Quality Index
    uint8_t         levelPm2d5CN;
    uint8_t         levelPm10CN;
    uint16_t        aqiPm2d5CN;
    uint16_t        aqiPm10CN;

    // pollutant concentration
    float           pm1d0;
    float           pm2d5;
    float           pm10;

    // clear method
    void clear() {
        pm1d0 = 0;
        pm2d5 = 0;
        pm10 = 0;
        levelPm2d5US = 0;
        levelPm10US = 0;
        aqiPm2d5US = 0;
        aqiPm10US = 0;
        levelPm2d5CN = 0;
        levelPm10CN = 0;
        aqiPm2d5CN = 0;
        aqiPm10CN = 0;
    }

    // calculate aqi and level
    void calculateAQIandLevel() {
        HS::calculateAQI(HS::PM25_TABLE, pm2d5, aqiPm2d5US, levelPm2d5US);
        HS::calculateAQI(HS::PM10_TABLE, pm10, aqiPm10US, levelPm10US);
    }
};

#endif // _PMDATA_H
