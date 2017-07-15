/*
 * CO2Sensor Wrap communicate with CO2 sensor
 * Copyright (c) 2017 Shenghua Su
 *
 */

#include "CO2Sensor.h"
#include "HealthyStandard.h"
#include "driver/gpio.h"
#include "AppLog.h"
#include "System.h"

// ------ co2 sensor pins
// https://www.esp32.com/viewtopic.php?t=1201
// the following pins at boot time can affect the operation of device
//  GPIO0
//  GPIO2
//  GPIO05
//  GPIO12 - MTDI
//  GPIO15 - MTDO

// #define CO2_SENSOR_MCU_RX_PIN       12 // UART_DEFAULT_PIN
#define CO2_SENSOR_MCU_RX_PIN          13
#define CO2_SENSOR_MCU_TX_PIN          14 // UART_DEFAULT_PIN
#define CO2_SENSOR_RST_PIN             12

// ------ co2 sensor
#define CO2_VALUE_POS                  0
#define CO2_ACQUIRE_CMD_PROTOCOL_LEN   7
#define CO2_ACQUIRE_CMD_LEN            5

static uint8_t CO2_ACQUIRE_CMD[CO2_ACQUIRE_CMD_PROTOCOL_LEN]
               = { 0x42, 0x4d, 0xe3, 0, 0, 0, 0 }; 

void _prepareCO2Cmd()
{
  uint16_t checksum = 0;
  for (int i = 0; i < CO2_ACQUIRE_CMD_LEN; ++i)
    checksum += CO2_ACQUIRE_CMD[i];
  CO2_ACQUIRE_CMD[CO2_ACQUIRE_CMD_LEN] = (checksum >> 8) & 0xFF;
  CO2_ACQUIRE_CMD[CO2_ACQUIRE_CMD_LEN + 1] = checksum & 0xFF;
}

/////////////////////////////////////////////////////////////////////////////////////////
// Sensor class
/////////////////////////////////////////////////////////////////////////////////////////
CO2Sensor::CO2Sensor()
: Uart(UART_NUM_1)
{
  _prepareCO2Cmd();
  clearCache();
}

void CO2Sensor::clearCache()
{
  _co2Data.clear();
}

void CO2Sensor::init()
{
  // set baudrate to 9600 which required by sensor
  setParams(9600);

  // initialize gpio
  gpio_set_direction((gpio_num_t)CO2_SENSOR_RST_PIN, GPIO_MODE_OUTPUT);
  gpio_set_pull_mode((gpio_num_t)CO2_SENSOR_RST_PIN, GPIO_PULLUP_ONLY);

  // init uart
  Uart::init(CO2_SENSOR_MCU_TX_PIN, CO2_SENSOR_MCU_RX_PIN);

  // init rx protocol length
  _protocolLen = rxProtocolLengthForSensorType(System::instance()->co2SensorType());
}

// delay definition
#define CO2_SENSOR_RST_DELAY     10
#ifndef delay
#define delay(x)                 vTaskDelay((x)/portTICK_RATE_MS)
#endif

void CO2Sensor::reset()
{
  gpio_set_level((gpio_num_t)CO2_SENSOR_RST_PIN, 0);
  delay(CO2_SENSOR_RST_DELAY);
  gpio_set_level((gpio_num_t)CO2_SENSOR_RST_PIN, 1);
  delay(CO2_SENSOR_RST_DELAY);
}

#define CO2_SENSOR_WAIT_RESPONSE_DELAY 20

void CO2Sensor::sampleData(TickType_t waitTicks)
{
  // send cmd
  int ret = tx(CO2_ACQUIRE_CMD, CO2_ACQUIRE_CMD_PROTOCOL_LEN);
  if (ret != CO2_ACQUIRE_CMD_PROTOCOL_LEN) {
    APP_LOGE("[CO2Sensor]", "err: tx cmd ret: %d", ret);
    return;
  }

  // give some delay
  delay(CO2_SENSOR_WAIT_RESPONSE_DELAY);

  // rx data
  int rxLen = rx(_rxBuf, _protocolLen, waitTicks);
#ifdef DEBUG_APP
  APP_LOGI("[CO2Sensor]", "sampleData rx len %d", rxLen);
#endif
  if (rxLen == _protocolLen) {
    onRxComplete();
  }
}

void CO2Sensor::onTxComplete()
{
}

void CO2Sensor::onRxComplete()
{
  for (int i = 0; i < _protocolLen; ++i) {
    _parser.parse(_rxBuf[i]);
    if (_parser.frameState() == FRAME_READY) {
      _co2Data.co2 = _parser.valueAt(CO2_VALUE_POS);
      _co2Data.calculateLevel();
      if (_dc) _dc->setCO2Data(_co2Data, false);
    }
  }
}
