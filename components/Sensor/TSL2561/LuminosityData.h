/*
 * LuminosityData: description of luminosity
 * Copyright (c) 2017 Shenghua Su
 *
 */


#ifndef _LUMINOSITY_DATA_H
#define _LUMINOSITY_DATA_H

#include "HealthyStandard.h"

struct LuminosityData {
    // standard from 
    uint8_t         level;

    // luminosity value
    uint32_t        luminosity;

    // clear method
    void clear() {
        luminosity = 0;
        level = 0;
    }

    // calculate level
    void calculateLevel() {
        level = HS::calculateSampleLevel(HS::LUMI_TABLE, luminosity, LUMI_MAX_LEVEL);
    }
};

#endif // _LUMINOSITY_DATA_H
