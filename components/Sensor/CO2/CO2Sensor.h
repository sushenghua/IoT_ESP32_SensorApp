/*
 * CO2Sensor Wrap communicate with CO2 sensor
 * Copyright (c) 2017 Shenghua Su
 *
 */

#ifndef _CO2SENSOR_H
#define _CO2SENSOR_H

#include "PTFrameParser.h"
#include "CO2Data.h"
#include "SensorDisplayController.h"

#define CO2_RX_PROTOCOL_LENGTH  12
#define CO2_RX_BUF_CAPACITY     CO2_RX_PROTOCOL_LENGTH

class CO2Sensor
{
public:
    // constructor
    CO2Sensor();
    void clearCache();
    void setDisplayDelegate(SensorDisplayController *dc) { _dc = dc; }

    // cached values
    CO2Data & co2Data() { return _co2Data; }

    // communication
    void startSampling();

    // virtual from Sampler
    virtual void sampleData();

    // virtual callback from Uart
    virtual void onTxComplete();
    virtual void onRxComplete();
    virtual void onError(uint32_t err);

protected:
    // value cache from sensor
    CO2Data         _co2Data;

    // parser and buf
    PTFrameParser   _parser;
    uint8_t         _rxBuf[CO2_RX_BUF_CAPACITY];
    uint16_t        _rxCount;

    // display delagate
    SensorDisplayController  *_dc;
};

#endif // _CO2SENSOR_H
