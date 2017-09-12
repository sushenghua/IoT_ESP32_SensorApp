/*
 * PowerManager: battery boost and charge management
 * Copyright (c) 2017 Shenghua Su
 *
 */

#include "PowerManager.h"
#include "AppLog.h"
#include "Adc.h"
#include "Semaphore.h"

/////////////////////////////////////////////////////////////////////////////////////////
// Power chip I2C
/////////////////////////////////////////////////////////////////////////////////////////
#define POWER_I2C_PORT                       I2C_NUM_0
#define POWER_I2C_PIN_SCK                    26
#define POWER_I2C_PIN_SDA                    27
#define POWER_I2C_CLK_SPEED                  100000

#define POWER_ADDR                           0x6B

#define PWR_I2C_SEMAPHORE_WAIT_TICKS         1000

I2c    *_sharedPwrI2c;

void pwrI2cInit()
{
  if (xSemaphoreTake(Semaphore::i2c, I2C_MAX_WAIT_TICKS)) {
    _sharedPwrI2c = I2c::instanceForPort(POWER_I2C_PORT, POWER_I2C_PIN_SCK, POWER_I2C_PIN_SDA);
    _sharedPwrI2c->setMode(I2C_MODE_MASTER);
    _sharedPwrI2c->setMasterClkSpeed(POWER_I2C_CLK_SPEED);
    _sharedPwrI2c->init();
    xSemaphoreGive(Semaphore::i2c);
  }
}

void pwrI2cMemTx(uint8_t memAddr, uint8_t *data)
{
  if (xSemaphoreTake(Semaphore::i2c, PWR_I2C_SEMAPHORE_WAIT_TICKS)) {
    _sharedPwrI2c->masterMemTx(POWER_ADDR, memAddr, data, 1);
    xSemaphoreGive(Semaphore::i2c);
  }
}

void pwrI2cMemRx(uint8_t memAddr, uint8_t *data)
{
  if (xSemaphoreTake(Semaphore::i2c, PWR_I2C_SEMAPHORE_WAIT_TICKS)) {
    _sharedPwrI2c->masterMemRx(POWER_ADDR, memAddr, data, 1);
    xSemaphoreGive(Semaphore::i2c);
  }
}

bool pwrI2cReady(int trials = 3)
{
  bool ready = false;
  if (xSemaphoreTake(Semaphore::i2c, PWR_I2C_SEMAPHORE_WAIT_TICKS)) {
    for (int i = 0; i < trials; ++i) {
      if (_sharedPwrI2c->deviceReady(POWER_ADDR)) {
        ready = true;
        break;
      }
    }
    xSemaphoreGive(Semaphore::i2c);
  }
  return ready;
}

/////////////////////////////////////////////////////////////////////////////////////////
// Power chip register
/////////////////////////////////////////////////////////////////////////////////////////
#define PREG_INPUT_SOURCE                    0x00
#define PREG_PWR_ON_CONF                     0x01
#define PREG_CHRG_CURRENT                    0x02
#define PREG_PRECHRG_TERM_CURRENT            0x03
#define PREG_CHRG_VOLTAGE                    0x04
#define PREG_CHRG_TERM_TIMER                 0x05
#define PREG_BOOST_VOLTAGE_THERMAL           0x06
#define PREG_MISC_OPERATION                  0x07
#define PREG_SYS_STATUS                      0x08
#define PREG_NEW_FAULT                       0x09
#define PREG_VENDER_STATUS                   0x0A

/////////////////////////////////////////////////////////////////////////////////////////
// Power chip register reset(default) value and set mask
/////////////////////////////////////////////////////////////////////////////////////////
#define PREG_RST_CHRG_TERM_TIMER             0x9C // 1001 1100  charge safety timer 12hrs
#define PREG_WATCHDOG_TIMER_OFF_AND_MASK     0xCF // 1100 1111

#define PREG_RST_MISC_OPERATION              0x4B // 0100 1011
#define PREG_BATFET_OFF_OR_MASK              0x20 // 0010 0000

#define PREG_RST_CHRG_CURRENT                0x20 // 0010 0000
#define PREG_1536mA_CHRG_CURRENT             0x40 // 0100 0000  for 18650 panasonic battery

/////////////////////////////////////////////////////////////////////////////////////////
// ADC voltage sample
/////////////////////////////////////////////////////////////////////////////////////////
#define SAMPLE_ACTIVE_COUNT     2     // 2 * taskDelay milliseconds
#define CALCULATE_AVERAGE_COUNT 5
#define BAT_VOLTAGE_CORRECTION  0.43f // issue: 3.3 gives 4095, 0.0 gives 0, but 1.8 does not produce 2234
#define BAT_VOLTAGE_MAX         4.05f
#define BAT_VOLTAGE_MIN         3.0f
Adc     _voltageReader;
uint8_t _sampleActiveCounter = SAMPLE_ACTIVE_COUNT;
uint8_t _sampleCount = 0;
float   _sampleValue = 0;
float   _batVoltage = 0;
float   _batLevel = 0;

/////////////////////////////////////////////////////////////////////////////////////////
// PowerManager class
/////////////////////////////////////////////////////////////////////////////////////////
PowerManager::PowerManager()
{}

void PowerManager::init()
{
  // init i2c
  pwrI2cInit();

  // check ready
  if (!pwrI2cReady()) APP_LOGE("[Power]", "Power chip not found");
  else APP_LOGI("[Power]", "Power chip init");

  // default settings
  applyDefaultSettings();

  // init adc
  _voltageReader.init(ADC1_CHANNEL_4);
}

void PowerManager::applyDefaultSettings()
{
  _setChargeCurrent();
}

bool PowerManager::batteryLevelPollTick()
{
  bool hasOutput = false;

  ++_sampleActiveCounter;
  if (_sampleActiveCounter >= SAMPLE_ACTIVE_COUNT) {
    int tmp = _voltageReader.readVoltage();
    _sampleValue += tmp;
    // APP_LOGC("[Power]", "sample(%d): %d", _sampleCount, tmp);
    ++_sampleCount;
    if (_sampleCount == CALCULATE_AVERAGE_COUNT) {
      _sampleValue /= _sampleCount;
      _batVoltage = _sampleValue * 6.6f / 4095 + BAT_VOLTAGE_CORRECTION;
      _batLevel = (_batVoltage - BAT_VOLTAGE_MIN) / (BAT_VOLTAGE_MAX - BAT_VOLTAGE_MIN) * 100;
      _batLevel = _batLevel < 0 ? 0 : _batLevel;
      // APP_LOGC("[Power]", "--> average sample: %.0f, voltage: %.2f", _sampleValue, _batVoltage);
      _sampleValue = 0;
      _sampleCount = 0;
      hasOutput = true;
    }
    _sampleActiveCounter = 0;
  }

  return hasOutput;
}

float PowerManager::batteryLevel()
{
  return _batLevel;
}

uint8_t _statusCache;

inline void _readSysStatus()
{
  pwrI2cMemRx(PREG_SYS_STATUS, &_statusCache);
}

PowerManager::ChargeStatus PowerManager::chargeStatus(bool readCache)
{
  if (!readCache) _readSysStatus();
  return (ChargeStatus)( (_statusCache >> 4) & 0x03 );
}

bool PowerManager::powerGood(bool readCache)
{
  if (!readCache) _readSysStatus();
  return (_statusCache >> 2) & 0x01;
}

uint8_t _data;

void PowerManager::powerOff()
{
  // disable watchdog timer
  _data = PREG_RST_CHRG_TERM_TIMER & PREG_WATCHDOG_TIMER_OFF_AND_MASK;
  pwrI2cMemTx(PREG_CHRG_TERM_TIMER, &_data);

  // set BATFET_DISABLE bit
  _data = PREG_RST_MISC_OPERATION | PREG_BATFET_OFF_OR_MASK;
  pwrI2cMemTx(PREG_MISC_OPERATION, &_data);
}

uint8_t PowerManager::_chargeCurrentReg()
{
  pwrI2cMemRx(PREG_CHRG_CURRENT, &_data);
  // APP_LOGC("[Power]", "charge current reg: %#X", data());
  return _data;
}

void PowerManager::_setChargeCurrent()
{
  _data = PREG_1536mA_CHRG_CURRENT;
  pwrI2cMemTx(PREG_CHRG_CURRENT, &_data);
}
