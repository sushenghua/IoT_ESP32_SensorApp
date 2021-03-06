/*
 * PMSensor Wrap communicate with PM sensor
 * Copyright (c) 2017 Shenghua Su
 *
 */

#include "PMSensor.h"
#include "HealthyStandard.h"
#include "driver/gpio.h"
#include "Debug.h"
#include "AppLog.h"
#include "System.h"

// ------ use temp/humid sensor inside PMS5xxxT
// #define USING_PMS5XXXT_TEMP_HUMID_SENSOR

// ------ pm sensor pins
#define PM_SENSOR_MCU_RX_PIN          16 // UART_DEFAULT_PIN
#define PM_SENSOR_MCU_TX_PIN          17 // UART_DEFAULT_PIN
#define PM_SENSOR_SET_PIN             13
#define PM_SENSOR_RST_PIN             15

// ------ pm sensor command
// command enum
#define PM_CMD_PASSIVE_READ           0
#define PM_CMD_RX_DATA_MODE_ACTIVE    1
#define PM_CMD_RX_DATA_MODE_PASSIVE   2
#define PM_CMD_RUN_MODE_STANDBY       3
#define PM_CMD_RUN_MODE_NORMAL        4

// real command to wrap
#define _PM_CMD_PASSIVE_READ          0xE2

#define _PM_CMD_RX_DATA_MODE          0xE1
#define _PM_RX_DATA_MODE_ACTIVE       0x01
#define _PM_RX_DATA_MODE_PASSIVE      0x00

#define _PM_CMD_RUN_MODE              0xE4
#define _PM_CMD_RUN_MODE_STANDBY      0x00
#define _PM_CMD_RUN_MODE_NORMAL       0x01

// cmd buf
#define PM_CMD_PROTOCOL_LEN           7
#define PM_CMD_LEN                    5
#define PM_CMD_KEY_POS                2
#define PM_CMD_DATA_H_POS             3
#define PM_CMD_DATA_L_POS             4
#define PM_CMD_CHECKSUM_H_POS         5
#define PM_CMD_CHECKSUM_L_POS         6

static uint8_t PM_CMD_BUF[PM_CMD_PROTOCOL_LEN]
               = { 0x42, 0x4d, 0xe3, 0, 0, 0, 0 };

void _appendPmCmdChecksum()
{
  uint16_t checksum = 0;
  for (int i = 0; i < PM_CMD_LEN; ++i)
    checksum += PM_CMD_BUF[i];
  PM_CMD_BUF[PM_CMD_CHECKSUM_H_POS] = (checksum >> 8) & 0xFF;
  PM_CMD_BUF[PM_CMD_CHECKSUM_L_POS] = checksum & 0xFF;
}

void _preparePmCmd(uint8_t cmd)
{
  switch (cmd) {
    case PM_CMD_PASSIVE_READ:
      PM_CMD_BUF[PM_CMD_KEY_POS] = _PM_CMD_PASSIVE_READ;
      PM_CMD_BUF[PM_CMD_DATA_L_POS] = 0;
      break;

    case PM_CMD_RX_DATA_MODE_ACTIVE:
      PM_CMD_BUF[PM_CMD_KEY_POS] = _PM_CMD_RX_DATA_MODE;
      PM_CMD_BUF[PM_CMD_DATA_L_POS] = _PM_RX_DATA_MODE_ACTIVE;
      break;

    case PM_CMD_RX_DATA_MODE_PASSIVE:
      PM_CMD_BUF[PM_CMD_KEY_POS] = _PM_CMD_RX_DATA_MODE;
      PM_CMD_BUF[PM_CMD_DATA_L_POS] = _PM_RX_DATA_MODE_PASSIVE;
      break;

    case PM_CMD_RUN_MODE_STANDBY:
      PM_CMD_BUF[PM_CMD_KEY_POS] = _PM_CMD_RUN_MODE;
      PM_CMD_BUF[PM_CMD_DATA_L_POS] = _PM_CMD_RUN_MODE_STANDBY;
      break;

    case PM_CMD_RUN_MODE_NORMAL:
      PM_CMD_BUF[PM_CMD_KEY_POS] = _PM_CMD_RUN_MODE;
      PM_CMD_BUF[PM_CMD_DATA_L_POS] = _PM_CMD_RUN_MODE_NORMAL;
      break;
  }
  _appendPmCmdChecksum();
}

/////////////////////////////////////////////////////////////////////////////////////////
// Sensor class
/////////////////////////////////////////////////////////////////////////////////////////
#define PM_1_D_0_POS        3  // it seems that pm data pos for all PMS5003* the same
#define PM_2_D_5_POS        4
#define PM_10_POS           5
#define HCHO_POS            12
// #define TEMPERATURE_POS     13
// #define HUMIDITY_POS        14

uint16_t _tempHumidDataPos;

PMSensor::PMSensor()
: Uart(UART_NUM_2)
{
  clearCache();
}

void PMSensor::init()
{
  // set baudrate to 9600 which required by sensor
  setParams(9600);

  // initialize gpio
  gpio_set_direction((gpio_num_t)PM_SENSOR_SET_PIN, GPIO_MODE_OUTPUT);
  gpio_set_direction((gpio_num_t)PM_SENSOR_RST_PIN, GPIO_MODE_OUTPUT);
  gpio_set_pull_mode((gpio_num_t)PM_SENSOR_SET_PIN, GPIO_PULLUP_ONLY);
  gpio_set_pull_mode((gpio_num_t)PM_SENSOR_RST_PIN, GPIO_PULLUP_ONLY);

  // init uart
  Uart::init(PM_SENSOR_MCU_TX_PIN, PM_SENSOR_MCU_RX_PIN);

  // init temp, humid data pos
  _tempHumidDataPos = tempHumidDataPosForSensorType(System::instance()->pmSensorType());

  // init rx protocol length
  _protocolLen = rxProtocolLengthForSensorType(System::instance()->pmSensorType());

  // cache capability from System instance
  _cap = System::instance()->devCapability();

  // wakeup
  wakeup();

  // reset the sensor
  reset();
}

void PMSensor::enableActiveDataTx(bool enabled)
{
  if (enabled) {
    // turn on sensor's active data tx
    _preparePmCmd(PM_CMD_RX_DATA_MODE_ACTIVE);
    tx(PM_CMD_BUF, PM_CMD_PROTOCOL_LEN);
  }
  else {
    // turn off sensor's active data tx
    _preparePmCmd(PM_CMD_RX_DATA_MODE_PASSIVE);
    tx(PM_CMD_BUF, PM_CMD_PROTOCOL_LEN);

    // prepare tx buf for cmd passive read
    _preparePmCmd(PM_CMD_PASSIVE_READ);
  }
}

// delay definition
#define PM_SENSOR_RST_DELAY      10
#ifndef delay
#define delay(x)                 vTaskDelay((x)/portTICK_RATE_MS)
#endif

void PMSensor::reset()
{
  gpio_set_level((gpio_num_t)PM_SENSOR_RST_PIN, 0);
  delay(PM_SENSOR_RST_DELAY);
  gpio_set_level((gpio_num_t)PM_SENSOR_RST_PIN, 1);
  delay(PM_SENSOR_RST_DELAY);
}

void PMSensor::wakeup()
{
  gpio_set_level((gpio_num_t)PM_SENSOR_SET_PIN, 1);
}

void PMSensor::sleep()
{
  gpio_set_level((gpio_num_t)PM_SENSOR_SET_PIN, 0);
}

void PMSensor::clearCache()
{
  _pmData.clear();
  _hchoData.clear();
  _tempHumidData.clear();
}

void PMSensor::sampleData(TickType_t waitTicks)
{
  int rxLen = rx(_rxBuf, _protocolLen, waitTicks);
#ifdef DEBUG_APP_OK
  APP_LOGI("[PMSensor]", "sampleData rx len %d", rxLen);
#endif
  if (rxLen == _protocolLen) {
    onRxComplete();
  }
  else {
    // init rx protocol length
    _protocolLen = rxProtocolLengthForSensorType(System::instance()->pmSensorType());
    // cache capability from System instance
    _cap = System::instance()->devCapability();
    // reset
    reset();
  }
}

void PMSensor::onTxComplete()
{
}

void PMSensor::onRxComplete()
{
  for (int i = 0; i < _protocolLen; ++i) {
    _parser.parse(_rxBuf[i]);
    if (_parser.frameState() == FRAME_READY) {
      //printBuf(_rxBuf, _protocolLen);
      if (_cap & PM_CAPABILITY_MASK) {
        _pmData.pm1d0 = _parser.valueAt(PM_1_D_0_POS);
        _pmData.pm2d5 = _parser.valueAt(PM_2_D_5_POS);
        _pmData.pm10 = _parser.valueAt(PM_10_POS);
        _pmData.calculateAQIandLevel();
        if (_dc) _dc->setPmData(&_pmData);
      }
      if (_cap & HCHO_CAPABILITY_MASK) {
        _hchoData.hcho = _parser.valueAt(HCHO_POS) / 1000.0f;
        _hchoData.calculateLevel();
        if (_dc) _dc->setHchoData(&_hchoData, false);
      }
#ifdef DEBUG_APP_OK
      APP_LOGI("[PMSensor]", "--->pm1.0: %2.2f  pm2.5: %2.2f  pm10: %2.2f  hcho: %2.2f\n",
               _pmData.pm1d0, _pmData.pm2d5, _pmData.pm10, _hchoData.hcho);
#endif
#ifdef USING_PMS5XXXT_TEMP_HUMID_SENSOR
      if (_cap & TEMP_HUMID_CAPABILITY_MASK) {
        _tempHumidData.temp = _parser.valueAt(_tempHumidDataPos) / 10.0f;
        _tempHumidData.humid = _parser.valueAt(_tempHumidDataPos + 1) / 10.0f;
        _tempHumidData.calculateLevel();
        if (_dc) _dc->setTempHumidData(&_tempHumidData, false);
      }
#endif
    }
  }
}
