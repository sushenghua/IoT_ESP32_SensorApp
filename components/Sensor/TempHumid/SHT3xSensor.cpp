/*
 * SHT3xSensor Wrap communicate with Temperature Humidity sensor
 * Copyright (c) 2017 Shenghua Su
 *
 */

#include "SHT3xSensor.h"
#include "AppLog.h"
#include "I2c.h"
#include "Semaphore.h"

/////////////////////////////////////////////////////////////////////////////////////////
// SHT3X Temperature Humidity sensor I2C
/////////////////////////////////////////////////////////////////////////////////////////
#define SHT3X_I2C_PORT                       I2C_NUM_0
#define SHT3X_I2C_PIN_SCK                    26
#define SHT3X_I2C_PIN_SDA                    27
#define SHT3X_I2C_CLK_SPEED                  100000

#define SHT3X_ADDR                           0x44

#define SHT3X_I2C_SEMAPHORE_WAIT_TICKS       1000

I2c    *_sharedSHT3xI2c;

void sht3xI2cInit()
{
  if (xSemaphoreTake(Semaphore::i2c, I2C_MAX_WAIT_TICKS)) {
    _sharedSHT3xI2c = I2c::instanceForPort(SHT3X_I2C_PORT, SHT3X_I2C_PIN_SCK, SHT3X_I2C_PIN_SDA);
    _sharedSHT3xI2c->setMode(I2C_MODE_MASTER);
    _sharedSHT3xI2c->setMasterClkSpeed(SHT3X_I2C_CLK_SPEED);
    _sharedSHT3xI2c->init();
    xSemaphoreGive(Semaphore::i2c);
  }
}

bool sht3xI2cReady(int trials = 3)
{
  bool ready = false;
  if (xSemaphoreTake(Semaphore::i2c, SHT3X_I2C_SEMAPHORE_WAIT_TICKS)) {
    for (int i = 0; i < trials; ++i) {
      if (_sharedSHT3xI2c->deviceReady(SHT3X_ADDR)) {
        ready = true;
        break;
      }
    }
    xSemaphoreGive(Semaphore::i2c);
  }
  return ready;
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
  if (xSemaphoreTake(Semaphore::i2c, SHT3X_I2C_SEMAPHORE_WAIT_TICKS)) {
    _sharedSHT3xI2c->masterTx(SHT3X_ADDR, _sht3xTxBuf, 2);
    xSemaphoreGive(Semaphore::i2c);
  }
}

bool sht3xRxData(uint8_t *buf, size_t count, TickType_t waitTicks = I2C_DEFAULT_WAIT_TICKS)
{
  bool ret = false;
  if (xSemaphoreTake(Semaphore::i2c, SHT3X_I2C_SEMAPHORE_WAIT_TICKS)) {
    ret = _sharedSHT3xI2c->masterRx(SHT3X_ADDR, buf, count, waitTicks);
    xSemaphoreGive(Semaphore::i2c);
  }
  return ret;
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

SHT3xSensor::SHT3xSensor()
: _dc(NULL)
{
  _tempHumidData.clear();
}

void SHT3xSensor::init()
{
  // init SHT3x I2C
  sht3xI2cInit();

  // check ready
  if (!sht3xI2cReady()) APP_LOGE("[SHT3X]", "SHT3X sensor not found");
  else APP_LOGI("[SHT3X]", "SHT3X sensor init");

  // soft-reset the sensor
  sht3xReset();
}

void SHT3xSensor::sampleData()
{
  if ( sht3xReadTempHumid(_tempHumidData.temp, _tempHumidData.humid) ) {
    _tempHumidData.calculateLevel();
    if (_dc) _dc->setTempHumidData(_tempHumidData, true);
#ifdef DEBUG_APP
    APP_LOGC("[SHT3X]", "--->temp: %2.2f  humid: %2.2f", _tempHumidData.temp, _tempHumidData.humid);
#endif
  }
}
