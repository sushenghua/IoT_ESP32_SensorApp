/*
 * PMSensor Wrap communicate with PM sensor
 * Copyright (c) 2017 Shenghua Su
 *
 */

#ifndef _PMSENSOR_H
#define _PMSENSOR_H

#include "PTFrameParser.h"
#include "SensorDisplayController.h"
#include "Uart.h"
#include "PMData.h"
#include "HchoData.h"
#include "TempHumidData.h"

#ifndef SENSOR_TYPE
#define PMS5003                 1
#define PMS5003S                2
#define PMS5003ST               3
#define SENSOR_TYPE             PMS5003S
#endif

#if SENSOR_TYPE == PMS5003
#define PM_RX_PROTOCOL_LENGTH   40
#elif SENSOR_TYPE == PMS5003S
#define PM_RX_PROTOCOL_LENGTH   32
#elif SENSOR_TYPE == PMS5003ST
#define PM_RX_PROTOCOL_LENGTH   40
#endif

#define PM_RX_BUF_CAPACITY      PM_RX_PROTOCOL_LENGTH

class PMSensor : public Uart
{
public:
    void test();
public:
    // constructor
    PMSensor();
    void init();
    void reset();
    void enableActiveDataTx(bool enabled);
    void clearCache();
    void setDisplayDelegate(SensorDisplayController *dc) { _dc = dc; }

    // cached values
#if SENSOR_TYPE >= PMS5003
    PMData & pmData() { return _pmData; }
#endif
#if SENSOR_TYPE >= PMS5003S
    HchoData & hchoData() { return _hchoData; }
#endif
#if SENSOR_TYPE >= PMS5003ST
    TempHumidData & tempHumidData() { return _tempHumidData; }
#endif

    // communication
    void sampleData(TickType_t waitTicks = UART_MAX_RX_WAIT_TICKS);

    // tx, rx completed
    void onTxComplete();
    void onRxComplete();

protected:
    // value cache from sensor
#if SENSOR_TYPE >= PMS5003
    PMData          _pmData;
#endif
#if SENSOR_TYPE >= PMS5003S
    HchoData        _hchoData;
#endif
#if SENSOR_TYPE >= PMS5003ST
    TempHumidData   _tempHumidData;
#endif

    // parser and buf
    const uint16_t  _protocolLen;
    PTFrameParser   _parser;
    uint8_t         _rxBuf[PM_RX_BUF_CAPACITY];

    // display delagate
    SensorDisplayController  *_dc;
};

#endif // _PMSENSOR_H
