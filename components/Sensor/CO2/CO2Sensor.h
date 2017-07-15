/*
 * CO2Sensor Wrap communicate with CO2 sensor
 * Copyright (c) 2017 Shenghua Su
 *
 */

#ifndef _CO2SENSOR_H
#define _CO2SENSOR_H

#include "PTFrameParser.h"
#include "SensorConfig.h"
#include "CO2Data.h"
#include "SensorDisplayController.h"
#include "Uart.h"
#include "CO2Data.h"

// #define CO2_RX_PROTOCOL_LENGTH  12
#define CO2_RX_BUF_CAPACITY     CO2_RX_PROTOCOL_MAX_LENGTH

class CO2Sensor : public Uart
{
public:
  // constructor
  CO2Sensor();

  void init();
  void reset();
  void clearCache();
  void setDisplayDelegate(SensorDisplayController *dc) { _dc = dc; }

  // cached values
  CO2Data & co2Data() { return _co2Data; }

  // communication
  void startSampling();

  // communication
  void sampleData(TickType_t waitTicks = UART_MAX_RX_WAIT_TICKS);

  // tx, rx completed
  void onTxComplete();
  void onRxComplete();

protected:
  // value cache from sensor
  CO2Data         _co2Data;

  // parser and buf
  uint16_t        _protocolLen;
  PTFrameParser   _parser;
  uint8_t         _rxBuf[CO2_RX_BUF_CAPACITY];

  // display delagate
  SensorDisplayController  *_dc;
};

#endif // _CO2SENSOR_H
