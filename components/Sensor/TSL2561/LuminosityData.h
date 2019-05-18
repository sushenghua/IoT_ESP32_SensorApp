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
    uint8_t         levelLuminosity;

    // luminosity value
    uint32_t        luminosity;

    // clear method
    void clear() {
        luminosity = 0;
        levelLuminosity = 0;
    }

    // calculate level
    void calculateLevel() {
        // levelLuminosity = HS::calculateSampleLevel();
    }
};

#endif // _LUMINOSITY_DATA_H
