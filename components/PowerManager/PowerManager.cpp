/*
 * PowerManager: battery boost and charge management
 * Copyright (c) 2017 Shenghua Su
 *
 */

#include "PowerManager.h"
#include "Config.h"
#include "AppLog.h"
#include "Adc.h"
#include "Semaphore.h"
#include "I2cPeripherals.h"

/////////////////////////////////////////////////////////////////////////////////////////
// Power chip I2C
/////////////////////////////////////////////////////////////////////////////////////////
#define POWER_CHIP_ADDR                      0x6B

bool pwrChipMemTx(uint8_t memAddr, uint8_t *data)
{
  return I2cPeripherals::masterMemTx(POWER_CHIP_ADDR, memAddr, data, 1);
}

bool pwrChipMemRx(uint8_t memAddr, uint8_t *data)
{
  return I2cPeripherals::masterMemRx(POWER_CHIP_ADDR, memAddr, data, 1);
}

bool pwrChipReady()
{
  return I2cPeripherals::deviceReady(POWER_CHIP_ADDR);
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

#define PREG_RST_PWR_ON_CONF                 0x3B // 0011 1011
#define PREG_SYS_MIN_VOLTAGE_31V             0x33 // 0011 0011

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

#define  PWR_CHIP_TRY_INIT_DELAY                         200
#define  PWR_CHIP_RESET_PERIPHERALS_ON_SAMPLE_FAIL_COUNT 1
uint16_t _pwrChipSampleFailCount = 0;
uint32_t _i2cPeripheralsPwrResetStamp = 0;

void _resetI2cPeripheralsOnPwrChipNotReady()
{
  while (!pwrChipReady()) {
    APP_LOGE("[Power]", "Power chip not found");
    if (_i2cPeripheralsPwrResetStamp == I2cPeripherals::resetStamp()) {
      APP_LOGC("[Power]", "Power chip require i2c I2cPeripherals reset");
      _i2cPeripheralsPwrResetStamp = I2cPeripherals::reset();
    }
    vTaskDelay( PWR_CHIP_TRY_INIT_DELAY / portTICK_RATE_MS );
  }
  // sync power reset count
  _i2cPeripheralsPwrResetStamp = I2cPeripherals::resetStamp();
  // reset sample fail count
  _pwrChipSampleFailCount = 0;
}

void PowerManager::init()
{
  APP_LOGI("[Power]", "Power chip init");

  // check ready, it blocks task here if not ready
  _resetI2cPeripheralsOnPwrChipNotReady();

  // default settings
  applyDefaultSettings();

  // init adc
  _voltageReader.init(ADC1_CHANNEL_4);
}

void PowerManager::applyDefaultSettings()
{
  _setChargeCurrent();
  _setSysMinVoltage();
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

inline bool _readSysStatus()
{
  return pwrChipMemRx(PREG_SYS_STATUS, &_statusCache);
}

PowerManager::ChargeStatus PowerManager::chargeStatus(bool readCache)
{
  if (!readCache) {
    if (!_readSysStatus()) {
      if (++_pwrChipSampleFailCount == PWR_CHIP_RESET_PERIPHERALS_ON_SAMPLE_FAIL_COUNT)
        _resetI2cPeripheralsOnPwrChipNotReady();
    }
  }
  return (ChargeStatus)( (_statusCache >> 4) & 0x03 );
}

bool PowerManager::powerGood(bool readCache)
{
  if (!readCache) _readSysStatus();
  return (_statusCache >> 2) & 0x01;
}

uint8_t _data;

bool PowerManager::powerOff()
{
  // disable watchdog timer
  _data = PREG_RST_CHRG_TERM_TIMER & PREG_WATCHDOG_TIMER_OFF_AND_MASK;
  if (!pwrChipMemTx(PREG_CHRG_TERM_TIMER, &_data)) return false;

  // set BATFET_DISABLE bit
  _data = PREG_RST_MISC_OPERATION | PREG_BATFET_OFF_OR_MASK;
  if (!pwrChipMemTx(PREG_MISC_OPERATION, &_data)) return false;

  return true;
}

uint8_t PowerManager::_chargeCurrentReg()
{
  pwrChipMemRx(PREG_CHRG_CURRENT, &_data);
  // APP_LOGC("[Power]", "charge current reg: %#X", data());
  return _data;
}

bool PowerManager::_setChargeCurrent()
{
  _data = PREG_1536mA_CHRG_CURRENT;
  return pwrChipMemTx(PREG_CHRG_CURRENT, &_data);
}

bool PowerManager::_setSysMinVoltage()
{
  _data = PREG_SYS_MIN_VOLTAGE_31V;
  return pwrChipMemTx(PREG_PWR_ON_CONF, &_data);
}
