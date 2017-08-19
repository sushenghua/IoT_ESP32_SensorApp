/*
 * PowerManager: battery boost and charge management
 * Copyright (c) 2017 Shenghua Su
 *
 */

#include "PowerManager.h"

/////////////////////////////////////////////////////////////////////////////////////////
// Power chip I2C
/////////////////////////////////////////////////////////////////////////////////////////
#define POWER_I2C_PORT       I2C_NUM_0
#define POWER_I2C_PIN_SCK    26
#define POWER_I2C_PIN_SDA    27
#define POWER_I2C_CLK_SPEED  100000

#define POWER_ADDR           0x6B

I2c    *_i2c;

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
// PowerManager class
/////////////////////////////////////////////////////////////////////////////////////////
PowerManager::PowerManager()
{}

void PowerManager::init()
{
  _i2c = I2c::instanceForPort(POWER_I2C_PORT, POWER_I2C_PIN_SCK, POWER_I2C_PIN_SDA);
  _i2c->setMode(I2C_MODE_MASTER);
  _i2c->setMasterClkSpeed(POWER_I2C_CLK_SPEED);
  _i2c->init();
}

uint8_t _statusCache;

inline void _readSysStatus()
{
  _i2c->masterMemRx(POWER_ADDR, PREG_SYS_STATUS, &_statusCache, 1);
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
  _i2c->masterMemTx(POWER_ADDR, PREG_CHRG_TERM_TIMER, &_data, 1);

  // set BATFET_DISABLE bit
  _data = PREG_RST_MISC_OPERATION | PREG_BATFET_OFF_OR_MASK;
  _i2c->masterMemTx(POWER_ADDR, PREG_MISC_OPERATION, &_data, 1);
}

void PowerManager::setChargeCurrent()
{
  _data = PREG_1536mA_CHRG_CURRENT;
  _i2c->masterMemTx(POWER_ADDR, PREG_CHRG_CURRENT, &_data, 1);
}
