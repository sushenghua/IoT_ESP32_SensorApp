/*
 * HealthyStandard: standards for different test parameters
 * Copyright (c) 2017 Shenghua Su
 *
 */

#ifndef _HEALTHY_STANDARD_H
#define _HEALTHY_STANDARD_H

#include <stdint.h>

class HS // stands for HealthyStandard
{
public:
/////////////////////////////////////////////////////////////////////////////////////////
// Common
/////////////////////////////////////////////////////////////////////////////////////////

static uint16_t RGB888toRGB565(uint8_t r, uint8_t g, uint8_t b);

/////////////////////////////////////////////////////////////////////////////////////////
// PM standard
// ref: https://en.wikipedia.org/wiki/Air_quality_index
/////////////////////////////////////////////////////////////////////////////////////////

#define PM_MAX_LEVEL  6

static float PM25US_TABLE[];
static float PM10US_TABLE[];
static float PM25CN_TABLE[];

static uint16_t colorForAirLevel(uint8_t level);

static void calculateAQI(const float *table, float concertration, uint16_t &indexValue, uint8_t &level);

/////////////////////////////////////////////////////////////////////////////////////////
// formaldehyde standard (GB/T50235-2001)
// ref: https://wenku.baidu.com/view/80e13b956bec0975f465e2a1.html
/////////////////////////////////////////////////////////////////////////////////////////

#define HCHO_MAX_LEVEL  4

static float HCHO_TABLE[];

static uint16_t colorForHchoLevel(uint8_t level);

/////////////////////////////////////////////////////////////////////////////////////////
// temperature and humidity standard
// ref: https://www.zhihu.com/question/31515876
/////////////////////////////////////////////////////////////////////////////////////////

#define TEMP_MAX_LEVEL  6

static float TEMP_TABLE[];

static uint16_t colorForTempLevel(uint8_t level);

#define HUMID_MAX_LEVEL  2

static float HUMID_TABLE[];

static uint16_t colorForHumidLevel(uint8_t level);

/////////////////////////////////////////////////////////////////////////////////////////
// CO2 standard
// ref: https://zhidao.baidu.com/question/475675906.html
/////////////////////////////////////////////////////////////////////////////////////////

#define CO2_MAX_LEVEL   4

static float CO2_TABLE[];

static uint16_t colorForCO2Level(uint8_t level);

static uint8_t calculateSampleLevel(const float *table, float sampleValue, uint8_t maxLevel);

};

#endif // _HEALTHY_STANDARD_H
