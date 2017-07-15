/*
 * PMSensor Wrap communicate with PM sensor
 * Copyright (c) 2017 Shenghua Su
 *
 */

#ifndef _PMSENSOR_H
#define _PMSENSOR_H

#include "PTFrameParser.h"
#include "SensorConfig.h"
#include "SensorDisplayController.h"
#include "Uart.h"
#include "PMData.h"
#include "HchoData.h"
#include "TempHumidData.h"

#define PM_RX_BUF_CAPACITY      PM_RX_PROTOCOL_MAX_LENGTH

class PMSensor : public Uart
{
public:
  void test();
public:
  // constructor
  PMSensor();
  void init();
  void reset();
  void wakeup();
  void sleep();
  void enableActiveDataTx(bool enabled);
  void clearCache();
  void setDisplayDelegate(SensorDisplayController *dc) { _dc = dc; }

  // cached values
  PMData & pmData() { return _pmData; }
  HchoData & hchoData() { return _hchoData; }
  TempHumidData & tempHumidData() { return _tempHumidData; }

  // communication
  void sampleData(TickType_t waitTicks = UART_MAX_RX_WAIT_TICKS);

  // tx, rx completed
  void onTxComplete();
  void onRxComplete();

protected:
  // capability cache
  uint32_t        _cap;

  // value cache from sensor
  PMData          _pmData;
  HchoData        _hchoData;
  TempHumidData   _tempHumidData;

  // parser and buf
  uint16_t        _protocolLen;
  PTFrameParser   _parser;
  uint8_t         _rxBuf[PM_RX_BUF_CAPACITY];

  // display delagate
  SensorDisplayController  *_dc;
};

#endif // _PMSENSOR_H
