/*
 * SHT3xSensor Wrap communicate with Temperature Humidity sensor
 * Copyright (c) 2017 Shenghua Su
 *
 */

#include "SHT3xSensor.h"
#include "Config.h"
#include "AppLog.h"
#include "I2c.h"
#include "Semaphore.h"
#include "I2cPeripherals.h"
#include "System.h"

/////////////////////////////////////////////////////////////////////////////////////////
// SHT3X Temperature Humidity sensor I2C
/////////////////////////////////////////////////////////////////////////////////////////
#define SHT3X_ADDR                           0x44

#define SHT3X_I2C_SEMAPHORE_WAIT_TICKS       1000

bool sht3xReady()
{
  return I2cPeripherals::deviceReady(SHT3X_ADDR);
}

/////////////////////////////////////////////////////////////////////////////////////////
// SHT3X comamnd, copied from Adafruit_SHT31.cpp
/////////////////////////////////////////////////////////////////////////////////////////

#define SHT3X_MEAS_HIGHREP_STRETCH     0x2C06
#define SHT3X_MEAS_MEDREP_STRETCH      0x2C0D
#define SHT3X_MEAS_LOWREP_STRETCH      0x2C10
#define SHT3X_MEAS_HIGHREP             0x2400
#define SHT3X_MEAS_MEDREP              0x240B
#define SHT3X_MEAS_LOWREP              0x2416
#define SHT3X_READSTATUS               0xF32D
#define SHT3X_CLEARSTATUS              0x3041
#define SHT3X_SOFTRESET                0x30A2
#define SHT3X_HEATEREN                 0x306D
#define SHT3X_HEATERDIS                0x3066

#ifndef delay
#define delay(x)                 vTaskDelay((x)/portTICK_RATE_MS)
#endif

uint8_t _sht3xTxBuf[2];

void sht3xSendCmd(uint16_t cmd)
{
  _sht3xTxBuf[0] = cmd >> 8;
  _sht3xTxBuf[1] = cmd & 0xFF;
  I2cPeripherals::masterTx(SHT3X_ADDR, _sht3xTxBuf, 2);
}

bool sht3xRxData(uint8_t *buf, size_t count, TickType_t waitTicks = I2C_DEFAULT_WAIT_TICKS)
{
  return I2cPeripherals::masterRx(SHT3X_ADDR, buf, count, waitTicks);
}

void sht3xReset()
{
  sht3xSendCmd(SHT3X_SOFTRESET);
  delay(10);
}

void sht3xEnableHeater(bool enabled)
{
  sht3xSendCmd(enabled ? SHT3X_HEATEREN : SHT3X_HEATERDIS);
}

const uint8_t CRC_POLYNOMIAL(0x31);

uint8_t sht3xCRC8(const uint8_t *data, int len)
{
 /*
  * CRC-8 formula from section 4.11 of SHT spec pdf
  *
  * Test data 0xBE, 0xEF should yield 0x92
  *
  * Initialization data 0xFF
  * Polynomial 0x31 (x8 + x5 +x4 +1)
  * Final XOR 0x00
  */
  uint8_t crc(0xFF);
  for ( int j = len; j; --j ) {
    crc ^= *data++;
    for ( int i = 8; i; --i ) {
      crc = ( crc & 0x80 ) ? (crc << 1) ^ CRC_POLYNOMIAL : (crc << 1);
    }
  }
  return crc;
}

uint8_t _sht3xRxBuf[6];

bool sht3xReadTempHumid(float &temperature, float &humidity)
{
  sht3xSendCmd(SHT3X_MEAS_HIGHREP);

  delay(400);

  bool ok = true;
  uint16_t temp, humid;

  do {

    ok = sht3xRxData(_sht3xRxBuf, 6);
    if (!ok) break;

    temp = _sht3xRxBuf[0]; temp <<= 8;
    temp |= _sht3xRxBuf[1];

    if (_sht3xRxBuf[2] != sht3xCRC8(_sht3xRxBuf, 2)) {
      ok = false; break;
    }

    humid = _sht3xRxBuf[3]; humid <<= 8;
    humid |= _sht3xRxBuf[4];

    if (_sht3xRxBuf[5] != sht3xCRC8(_sht3xRxBuf + 3, 2)) {
      ok = false; break;
    }

    temperature = 175.0 * temp / 0xFFFF - 45;
    humidity = 100.0 * humid / 0xFFFF;

  } while (false);

  return ok;
}

bool sht3xReadStatus(uint16_t &status)
{
  sht3xSendCmd(SHT3X_READSTATUS);
  // delay(5);
  if (sht3xRxData(_sht3xRxBuf, 3)) {
    status = _sht3xRxBuf[0]; status <<= 8;
    status |= _sht3xRxBuf[1];
    return true;
  }
  return false;
}

/////////////////////////////////////////////////////////////////////////////////////////
// SHT3xSensor class
/////////////////////////////////////////////////////////////////////////////////////////
bool _needCalibrateMbTemp = false;
float _mainBoardTempBias = 0;
SHT3xSensor::SHT3xSensor()
: _dc(NULL)
{
  _tempHumidData.clear();
  _needCalibrateMbTemp = System::instance()->bias()->mbTempNeedCalibrate;
  _mainBoardTempBias = System::instance()->bias()->mbTempBias;
}

#define  SHT3X_TRY_INIT_DELAY             100
#define  SHT3X_RESET_ON_SAMPLE_FAIL_COUNT 1
uint16_t _sht3xSampleFailCount = 0;
uint32_t _sht3xPwrResetStamp = 0;

void SHT3xSensor::init(bool checkDeviceReady)
{
  APP_LOGI("[SHT3X]", "SHT3X sensor init");

  // check ready, it blocks task here if not ready
  while (checkDeviceReady && !sht3xReady()) {
    APP_LOGE("[SHT3X]", "SHT3X sensor not found");
    if (_sht3xPwrResetStamp == I2cPeripherals::resetStamp()) {
      APP_LOGC("[SHT3X]", "SHT3X sensor require reset");
      _sht3xPwrResetStamp = I2cPeripherals::reset();
    }
    delay(SHT3X_TRY_INIT_DELAY);
  }
  // sync power reset count
  _sht3xPwrResetStamp = I2cPeripherals::resetStamp();
  // reset sample fail count
  _sht3xSampleFailCount = 0;
  // soft-reset the sensor
  sht3xReset();
}

#define UNDEFINED_TEMPERATURE             1000

#define TEMP_CALIBRATION_OFFSET           4
#define TEMP_CALIBRATION_DELTA_BASE1      3
#define TEMP_CALIBRATION_DELTA_BASE2      8.2
const float factor1 = TEMP_CALIBRATION_OFFSET / (8.2 - 1);
const float factor2 = 0.698;

bool  _hasChargeHeat = false;

float _mainBoardTemp = UNDEFINED_TEMPERATURE;
float _mainBoardTempDiscreted = 0;
float _mainBoardTempDiscretedDelta = 0;

float _sensorRawTemp = UNDEFINED_TEMPERATURE;
float _sensorRawTempDiscreted = 0;
float _sensorRawTempDiscretedDelta = 0;

bool SHT3xSensor::_calibrateTemperature(float &temperature)
{
  float offset = 0;
  if (_mainBoardTemp == UNDEFINED_TEMPERATURE) {
    // offset = TEMP_CALIBRATION_OFFSET;
    return false;
  }
  else {
    float delta = _mainBoardTemp - temperature;
    if (delta > TEMP_CALIBRATION_DELTA_BASE2) {
      if (_hasChargeHeat)
        offset = TEMP_CALIBRATION_OFFSET + factor2 * (delta - TEMP_CALIBRATION_DELTA_BASE2);
      else
        offset = TEMP_CALIBRATION_OFFSET;
    }
    else if (delta > TEMP_CALIBRATION_DELTA_BASE1) { // 
      offset = factor1 * (delta - TEMP_CALIBRATION_DELTA_BASE1);
    }
    else {
      offset = 0;
    }
  }
  temperature -= offset;
  return true;
}

void SHT3xSensor::_discrete(float temp)
{
  _sensorRawTemp = temp;
  if (abs(_sensorRawTemp - _sensorRawTempDiscreted) > 0.1) {
    _sensorRawTempDiscretedDelta = _sensorRawTemp - _sensorRawTempDiscreted;
    _sensorRawTempDiscreted = _sensorRawTemp;
  }
  if (abs(_mainBoardTemp - _mainBoardTempDiscreted) > 0.5) {
    _mainBoardTempDiscretedDelta = _mainBoardTemp - _mainBoardTempDiscreted;
    _sensorRawTempDiscreted = _mainBoardTemp;
  }
}

void SHT3xSensor::setMainboardTemperature(float mainboardTemp, bool charge)
{
  if (_needCalibrateMbTemp && _sensorRawTemp != UNDEFINED_TEMPERATURE) {
    _mainBoardTempBias = mainboardTemp - _sensorRawTemp;
    System::instance()->setMbTempCalibration(false, _mainBoardTempBias);
    APP_LOGC("[SHT3X]", "set MB temp bias: %.2f", _mainBoardTempBias);
  }
  _mainBoardTemp = mainboardTemp - _mainBoardTempBias;
  _hasChargeHeat = charge;
}

void SHT3xSensor::sampleData()
{
  // other tasks has reset the i2c peripherals power
  if (_sht3xPwrResetStamp < I2cPeripherals::resetStamp()) {
    APP_LOGE("[SHT3X]", "reset stamp out of sync");
    init(false);
  }

  if ( sht3xReadTempHumid(_tempHumidData.temp, _tempHumidData.humid) ) {

    _discrete(_tempHumidData.temp);

    if (_calibrateTemperature(_tempHumidData.temp)) {
      _tempHumidData.calculateLevel();
      if (_dc) _dc->setTempHumidData(&_tempHumidData, true);
    }

    // APP_LOGC("[SHT3xSensor]", "mb temp: %.2f, sht3x temp: %0.2f, cali temp: %0.2f",
    //           _mainBoardTemp, _sensorRawTemp, _tempHumidData.temp);

#ifdef DEBUG_APP_OK
    APP_LOGC("[SHT3X]", "--->temp: %2.2f  humid: %2.2f", _tempHumidData.temp, _tempHumidData.humid);
#endif
  }
  else {
#ifdef DEBUG_APP_ERR
    APP_LOGE("[SHT3X]", "sample data failed");
#endif
    if (++_sht3xSampleFailCount == SHT3X_RESET_ON_SAMPLE_FAIL_COUNT) init();
  }
}
