/*
 * TSL2561 Wrap communicate with TSL2561 light-to-digital sensor
 *
 * original file:   Adafruit_TSL2561.cpp
 * origianl author: K.Townsend (Adafruit Industries)
 *
 * Software License Agreement (BSD License)
 *
 * Copyright (c) 2013, Adafruit Industries
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 * 1. Redistributions of source code must retain the above copyright
 * notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 * notice, this list of conditions and the following disclaimer in the
 * documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the copyright holders nor the
 * names of its contributors may be used to endorse or promote products
 * derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS ''AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 * Copyright (c) 2017 Shenghua Su
 *
 */

#include "TSL2561.h"
#include "AppLog.h"
#include "I2c.h"
#include "Semaphore.h"

/////////////////////////////////////////////////////////////////////////////////////////
// TSL2561 light-to-digital sensor I2C
/////////////////////////////////////////////////////////////////////////////////////////
#define TSL2561_I2C_PORT                       I2C_NUM_0
#define TSL2561_I2C_PIN_SCK                    26
#define TSL2561_I2C_PIN_SDA                    27
#define TSL2561_I2C_CLK_SPEED                  100000

// I2C address options
#define TSL2561_ADDR_LOW                       0x29
#define TSL2561_ADDR_FLOAT                     0x39   // Default address (pin left floating)
#define TSL2561_ADDR_HIGH                      0x49

#define TSL2561_ADDR                           TSL2561_ADDR_FLOAT

#define TSL2561_I2C_SEMAPHORE_WAIT_TICKS       1000

I2c    *_sharedTsl2561I2c;

void tsl2561I2cInit()
{
  if (xSemaphoreTake(Semaphore::i2c, I2C_MAX_WAIT_TICKS)) {
    _sharedTsl2561I2c = I2c::instanceForPort(TSL2561_I2C_PORT, TSL2561_I2C_PIN_SCK, TSL2561_I2C_PIN_SDA);
    _sharedTsl2561I2c->setMode(I2C_MODE_MASTER);
    _sharedTsl2561I2c->setMasterClkSpeed(TSL2561_I2C_CLK_SPEED);
    _sharedTsl2561I2c->init();
    xSemaphoreGive(Semaphore::i2c);
  }
}

bool tsl2561Ready(int trials = 3)
{
  bool ready = false;
  if (xSemaphoreTake(Semaphore::i2c, TSL2561_I2C_SEMAPHORE_WAIT_TICKS)) {
    for (int i = 0; i < trials; ++i) {
      if (_sharedTsl2561I2c->deviceReady(TSL2561_ADDR)) {
        ready = true;
        break;
      }
    }
    xSemaphoreGive(Semaphore::i2c);
  }
  return ready;
}

void tsl2561I2cMemTx(uint8_t memAddr, uint8_t *data)
{
  if (xSemaphoreTake(Semaphore::i2c, TSL2561_I2C_SEMAPHORE_WAIT_TICKS)) {
    _sharedTsl2561I2c->masterMemTx(TSL2561_ADDR, memAddr, data, 1);
    xSemaphoreGive(Semaphore::i2c);
  }
}

void tsl2561I2cMemRx(uint8_t memAddr, uint8_t *data, size_t count)
{
  if (xSemaphoreTake(Semaphore::i2c, TSL2561_I2C_SEMAPHORE_WAIT_TICKS)) {
    _sharedTsl2561I2c->masterMemRx(TSL2561_ADDR, memAddr, data, count);
    xSemaphoreGive(Semaphore::i2c);
  }
}

/////////////////////////////////////////////////////////////////////////////////////////
// TSL2561 consts, copied from Adafruit_TSL2561.cpp
/////////////////////////////////////////////////////////////////////////////////////////
#define TSL2561_VISIBLE           2         // channel 0 - channel 1
#define TSL2561_INFRARED          1         // channel 1
#define TSL2561_FULLSPECTRUM      0         // channel 0

#define TSL2561_REGISTER_CONTROL             0x00
#define TSL2561_REGISTER_TIMING              0x01
#define TSL2561_REGISTER_THRESHHOLDL_LOW     0x02
#define TSL2561_REGISTER_THRESHHOLDL_HIGH    0x03
#define TSL2561_REGISTER_THRESHHOLDH_LOW     0x04
#define TSL2561_REGISTER_THRESHHOLDH_HIGH    0x05
#define TSL2561_REGISTER_INTERRUPT           0x06
#define TSL2561_REGISTER_CRC                 0x08
#define TSL2561_REGISTER_ID                  0x0A
#define TSL2561_REGISTER_CHAN0_LOW           0x0C
#define TSL2561_REGISTER_CHAN0_HIGH          0x0D
#define TSL2561_REGISTER_CHAN1_LOW           0x0E
#define TSL2561_REGISTER_CHAN1_HIGH          0x0F

// Lux calculations differ slightly for CS package
//#define TSL2561_PACKAGE_CS
#define TSL2561_PACKAGE_T_FN_CL

#define TSL2561_COMMAND_BIT       (0x80)    // Must be 1
#define TSL2561_CLEAR_BIT         (0x40)    // Clears any pending interrupt (write 1 to clear)
#define TSL2561_WORD_BIT          (0x20)    // 1 = read/write word (rather than byte)
#define TSL2561_BLOCK_BIT         (0x10)    // 1 = using block read/write

#define TSL2561_CONTROL_POWERON   (0x03)
#define TSL2561_CONTROL_POWEROFF  (0x00)

#define TSL2561_LUX_LUXSCALE      (14)      // Scale by 2^14
#define TSL2561_LUX_RATIOSCALE    (9)       // Scale ratio by 2^9
#define TSL2561_LUX_CHSCALE       (10)      // Scale channel values by 2^10
#define TSL2561_LUX_CHSCALE_TINT0 (0x7517)  // 322/11 * 2^TSL2561_LUX_CHSCALE
#define TSL2561_LUX_CHSCALE_TINT1 (0x0FE7)  // 322/81 * 2^TSL2561_LUX_CHSCALE

// T, FN and CL package values
#define TSL2561_LUX_K1T           (0x0040)  // 0.125 * 2^RATIO_SCALE
#define TSL2561_LUX_B1T           (0x01f2)  // 0.0304 * 2^LUX_SCALE
#define TSL2561_LUX_M1T           (0x01be)  // 0.0272 * 2^LUX_SCALE
#define TSL2561_LUX_K2T           (0x0080)  // 0.250 * 2^RATIO_SCALE
#define TSL2561_LUX_B2T           (0x0214)  // 0.0325 * 2^LUX_SCALE
#define TSL2561_LUX_M2T           (0x02d1)  // 0.0440 * 2^LUX_SCALE
#define TSL2561_LUX_K3T           (0x00c0)  // 0.375 * 2^RATIO_SCALE
#define TSL2561_LUX_B3T           (0x023f)  // 0.0351 * 2^LUX_SCALE
#define TSL2561_LUX_M3T           (0x037b)  // 0.0544 * 2^LUX_SCALE
#define TSL2561_LUX_K4T           (0x0100)  // 0.50 * 2^RATIO_SCALE
#define TSL2561_LUX_B4T           (0x0270)  // 0.0381 * 2^LUX_SCALE
#define TSL2561_LUX_M4T           (0x03fe)  // 0.0624 * 2^LUX_SCALE
#define TSL2561_LUX_K5T           (0x0138)  // 0.61 * 2^RATIO_SCALE
#define TSL2561_LUX_B5T           (0x016f)  // 0.0224 * 2^LUX_SCALE
#define TSL2561_LUX_M5T           (0x01fc)  // 0.0310 * 2^LUX_SCALE
#define TSL2561_LUX_K6T           (0x019a)  // 0.80 * 2^RATIO_SCALE
#define TSL2561_LUX_B6T           (0x00d2)  // 0.0128 * 2^LUX_SCALE
#define TSL2561_LUX_M6T           (0x00fb)  // 0.0153 * 2^LUX_SCALE
#define TSL2561_LUX_K7T           (0x029a)  // 1.3 * 2^RATIO_SCALE
#define TSL2561_LUX_B7T           (0x0018)  // 0.00146 * 2^LUX_SCALE
#define TSL2561_LUX_M7T           (0x0012)  // 0.00112 * 2^LUX_SCALE
#define TSL2561_LUX_K8T           (0x029a)  // 1.3 * 2^RATIO_SCALE
#define TSL2561_LUX_B8T           (0x0000)  // 0.000 * 2^LUX_SCALE
#define TSL2561_LUX_M8T           (0x0000)  // 0.000 * 2^LUX_SCALE

// CS package values
#define TSL2561_LUX_K1C           (0x0043)  // 0.130 * 2^RATIO_SCALE
#define TSL2561_LUX_B1C           (0x0204)  // 0.0315 * 2^LUX_SCALE
#define TSL2561_LUX_M1C           (0x01ad)  // 0.0262 * 2^LUX_SCALE
#define TSL2561_LUX_K2C           (0x0085)  // 0.260 * 2^RATIO_SCALE
#define TSL2561_LUX_B2C           (0x0228)  // 0.0337 * 2^LUX_SCALE
#define TSL2561_LUX_M2C           (0x02c1)  // 0.0430 * 2^LUX_SCALE
#define TSL2561_LUX_K3C           (0x00c8)  // 0.390 * 2^RATIO_SCALE
#define TSL2561_LUX_B3C           (0x0253)  // 0.0363 * 2^LUX_SCALE
#define TSL2561_LUX_M3C           (0x0363)  // 0.0529 * 2^LUX_SCALE
#define TSL2561_LUX_K4C           (0x010a)  // 0.520 * 2^RATIO_SCALE
#define TSL2561_LUX_B4C           (0x0282)  // 0.0392 * 2^LUX_SCALE
#define TSL2561_LUX_M4C           (0x03df)  // 0.0605 * 2^LUX_SCALE
#define TSL2561_LUX_K5C           (0x014d)  // 0.65 * 2^RATIO_SCALE
#define TSL2561_LUX_B5C           (0x0177)  // 0.0229 * 2^LUX_SCALE
#define TSL2561_LUX_M5C           (0x01dd)  // 0.0291 * 2^LUX_SCALE
#define TSL2561_LUX_K6C           (0x019a)  // 0.80 * 2^RATIO_SCALE
#define TSL2561_LUX_B6C           (0x0101)  // 0.0157 * 2^LUX_SCALE
#define TSL2561_LUX_M6C           (0x0127)  // 0.0180 * 2^LUX_SCALE
#define TSL2561_LUX_K7C           (0x029a)  // 1.3 * 2^RATIO_SCALE
#define TSL2561_LUX_B7C           (0x0037)  // 0.00338 * 2^LUX_SCALE
#define TSL2561_LUX_M7C           (0x002b)  // 0.00260 * 2^LUX_SCALE
#define TSL2561_LUX_K8C           (0x029a)  // 1.3 * 2^RATIO_SCALE
#define TSL2561_LUX_B8C           (0x0000)  // 0.000 * 2^LUX_SCALE
#define TSL2561_LUX_M8C           (0x0000)  // 0.000 * 2^LUX_SCALE

// Auto-gain thresholds
#define TSL2561_AGC_THI_13MS      (4850)    // Max value at Ti 13ms = 5047
#define TSL2561_AGC_TLO_13MS      (100)
#define TSL2561_AGC_THI_101MS     (36000)   // Max value at Ti 101ms = 37177
#define TSL2561_AGC_TLO_101MS     (200)
#define TSL2561_AGC_THI_402MS     (63000)   // Max value at Ti 402ms = 65535
#define TSL2561_AGC_TLO_402MS     (500)

// Clipping thresholds
#define TSL2561_CLIPPING_13MS     (4900)
#define TSL2561_CLIPPING_101MS    (37000)
#define TSL2561_CLIPPING_402MS    (65000)


#ifndef delay
#define delay(x)                 vTaskDelay((x)/portTICK_RATE_MS)
#endif

#define TSL2561_DELAY_INTTIME_13MS    10
#define TSL2561_DELAY_INTTIME_101MS   100
#define TSL2561_DELAY_INTTIME_402MS   400

enum TSL2561IntegrationTime
{
  TSL2561_INTEGRATIONTIME_13MS      = 0x00,    // 13.7ms
  TSL2561_INTEGRATIONTIME_101MS     = 0x01,    // 101ms
  TSL2561_INTEGRATIONTIME_402MS     = 0x02     // 402ms
};

enum TSL2561Gain
{
  TSL2561_GAIN_1X                   = 0x00,    // No gain
  TSL2561_GAIN_16X                  = 0x10,    // 16x gain
};

TSL2561IntegrationTime _tsl2561IntegrationTime = TSL2561_INTEGRATIONTIME_402MS;
TSL2561Gain _tsl2561Gain = TSL2561_GAIN_1X;

uint8_t _tsl2561RxBuf[2];
uint8_t _tsl2561TxVal;

void tsl2561Enable(bool enable)
{
  _tsl2561TxVal = enable ? TSL2561_CONTROL_POWERON : TSL2561_CONTROL_POWEROFF;
  tsl2561I2cMemTx(TSL2561_COMMAND_BIT | TSL2561_REGISTER_CONTROL, &_tsl2561TxVal);
}

void tsl2561SetIntegrationTimeAndGain(TSL2561IntegrationTime time, TSL2561Gain gain)
{
  tsl2561Enable(true);

  // update the timing register
  _tsl2561TxVal = time | gain;
  tsl2561I2cMemTx(TSL2561_COMMAND_BIT | TSL2561_REGISTER_TIMING, &_tsl2561TxVal);

  // update value placeholder
  _tsl2561IntegrationTime = time;
  _tsl2561Gain = gain;

  tsl2561Enable(false);
}

void tsl2561GetLuminosityData(uint16_t &broadband, uint16_t &ir)
{
  tsl2561Enable(true);

  // wait x ms for ADC to complete
  switch (_tsl2561IntegrationTime) {
    case TSL2561_INTEGRATIONTIME_13MS:
      delay(TSL2561_DELAY_INTTIME_13MS);
      break;
    case TSL2561_INTEGRATIONTIME_101MS:
      delay(TSL2561_DELAY_INTTIME_101MS);
      break;
    default:
      delay(TSL2561_DELAY_INTTIME_402MS);
      break;
  }

  // reads a two byte value from channel 0 (visible + infrared)
  tsl2561I2cMemRx(TSL2561_COMMAND_BIT | TSL2561_WORD_BIT | TSL2561_REGISTER_CHAN0_LOW, _tsl2561RxBuf, 2);
  broadband = _tsl2561RxBuf[0]; broadband <<= 8;
  broadband |= _tsl2561RxBuf[1] & 0xFF;

  // reads a two byte value from channel 1 (infrared)
  tsl2561I2cMemRx(TSL2561_COMMAND_BIT | TSL2561_WORD_BIT | TSL2561_REGISTER_CHAN1_LOW, _tsl2561RxBuf, 2);
  ir = _tsl2561RxBuf[0]; ir <<= 8;
  ir |= _tsl2561RxBuf[1] & 0xFF;

  APP_LOGC("[TSL2561]", "broadband: %d, ir: %d", broadband, ir);

  tsl2561Enable(false); // save power
}

void tsl2561GetLuminosity(uint16_t &broadband, uint16_t &ir, bool autoGain = false)
{
  if(!autoGain) {
    tsl2561GetLuminosityData(broadband, ir);
    return;
  }

  bool valid = false;
  bool autoGainCheck = false;
  uint16_t broadbandSample, irSample;
  uint16_t hi, lo;
  // get the hi/low threshold for the current integration time
  switch(_tsl2561IntegrationTime) {
    case TSL2561_INTEGRATIONTIME_13MS:
      hi = TSL2561_AGC_THI_13MS;
      lo = TSL2561_AGC_TLO_13MS;
      break;
    case TSL2561_INTEGRATIONTIME_101MS:
      hi = TSL2561_AGC_THI_101MS;
      lo = TSL2561_AGC_TLO_101MS;
      break;
    default:
      hi = TSL2561_AGC_THI_402MS;
      lo = TSL2561_AGC_TLO_402MS;
      break;
  }

  // read data until we find a valid range
  do {
    tsl2561GetLuminosityData(broadbandSample, irSample);

    // run an auto-gain check if we haven't already done so
    if (!autoGainCheck) {
      if ((broadbandSample < lo) && (_tsl2561Gain == TSL2561_GAIN_1X)) {
        // increase the gain and try again
        tsl2561SetIntegrationTimeAndGain(_tsl2561IntegrationTime, TSL2561_GAIN_16X);
        // drop the previous conversion results
        tsl2561GetLuminosityData(broadbandSample, irSample);
        // set a flag to indicate we've adjusted the gain
        autoGainCheck = true;
      }
      else if ((broadbandSample > hi) && (_tsl2561Gain == TSL2561_GAIN_16X)) {
        // drop gain to 1x and try again
        tsl2561SetIntegrationTimeAndGain(_tsl2561IntegrationTime, TSL2561_GAIN_1X);
        // drop the previous conversion results
        tsl2561GetLuminosityData(broadbandSample, irSample);
        // set a flag to indicate we've adjusted the gain
        autoGainCheck = true;
      }
      else {
        // nothing to look at here, keep moving ....
        // reading is either valid, or we're already at the chips limits
        broadband = broadbandSample;
        ir = irSample;
        valid = true;
      }
    }
    else {
      // if we've already adjusted the gain once, just return the new results.
      // this avoids endless loops where a value is at one extreme pre-gain,
      // and the the other extreme post-gain
      broadband = broadbandSample;
      ir = irSample;
      valid = true;
    }
  } while (!valid);
}

void tsl2561CalculateLuminosity(uint32_t &ret, uint16_t broadband, uint16_t ir)
{
  unsigned long chScale;
  unsigned long channel1;
  unsigned long channel0;  

  // make sure the sensor isn't saturated
  uint16_t clipThreshold;
  switch (_tsl2561IntegrationTime) {
    case TSL2561_INTEGRATIONTIME_13MS:
      clipThreshold = TSL2561_CLIPPING_13MS;
      chScale = TSL2561_LUX_CHSCALE_TINT0;
      break;
    case TSL2561_INTEGRATIONTIME_101MS:
      clipThreshold = TSL2561_CLIPPING_101MS;
      chScale = TSL2561_LUX_CHSCALE_TINT1;
      break;
    default:
      clipThreshold = TSL2561_CLIPPING_402MS;
      chScale = (1 << TSL2561_LUX_CHSCALE);  // no scaling ...
      break;
  }

  // return 65536 lux if the sensor is saturated
  if ((broadband > clipThreshold) || (ir > clipThreshold)) {
    ret = 65536; return;
  }

  // scale for gain (1x or 16x)
  if (!_tsl2561Gain) chScale = chScale << 4;

  // scale the channel values
  channel0 = (broadband * chScale) >> TSL2561_LUX_CHSCALE;
  channel1 = (ir * chScale) >> TSL2561_LUX_CHSCALE;

  // find the ratio of the channel values (Channel1/Channel0)
  unsigned long ratio = 0;
  if (channel0 != 0) ratio = (channel1 << (TSL2561_LUX_RATIOSCALE+1)) / channel0;
  // round the ratio value
  ratio = (ratio + 1) >> 1;

  unsigned int b = 0, m = 0;

#ifdef TSL2561_PACKAGE_CS
  if (ratio <= TSL2561_LUX_K1C)      { b = TSL2561_LUX_B1C; m = TSL2561_LUX_M1C; }
  else if (ratio <= TSL2561_LUX_K2C) { b = TSL2561_LUX_B2C; m = TSL2561_LUX_M2C; }
  else if (ratio <= TSL2561_LUX_K3C) { b = TSL2561_LUX_B3C; m = TSL2561_LUX_M3C; }
  else if (ratio <= TSL2561_LUX_K4C) { b = TSL2561_LUX_B4C; m = TSL2561_LUX_M4C; }
  else if (ratio <= TSL2561_LUX_K5C) { b = TSL2561_LUX_B5C; m = TSL2561_LUX_M5C; }
  else if (ratio <= TSL2561_LUX_K6C) { b = TSL2561_LUX_B6C; m = TSL2561_LUX_M6C; }
  else if (ratio <= TSL2561_LUX_K7C) { b = TSL2561_LUX_B7C; m = TSL2561_LUX_M7C; }
  else if (ratio > TSL2561_LUX_K8C)  { b = TSL2561_LUX_B8C; m = TSL2561_LUX_M8C; }
#else
  if (ratio <= TSL2561_LUX_K1T)      { b = TSL2561_LUX_B1T; m = TSL2561_LUX_M1T; }
  else if (ratio <= TSL2561_LUX_K2T) { b = TSL2561_LUX_B2T; m = TSL2561_LUX_M2T; }
  else if (ratio <= TSL2561_LUX_K3T) { b = TSL2561_LUX_B3T; m = TSL2561_LUX_M3T; }
  else if (ratio <= TSL2561_LUX_K4T) { b = TSL2561_LUX_B4T; m = TSL2561_LUX_M4T; }
  else if (ratio <= TSL2561_LUX_K5T) { b = TSL2561_LUX_B5T; m = TSL2561_LUX_M5T; }
  else if (ratio <= TSL2561_LUX_K6T) { b = TSL2561_LUX_B6T; m = TSL2561_LUX_M6T; }
  else if (ratio <= TSL2561_LUX_K7T) { b = TSL2561_LUX_B7T; m = TSL2561_LUX_M7T; }
  else if (ratio > TSL2561_LUX_K8T)  { b = TSL2561_LUX_B8T; m = TSL2561_LUX_M8T; }
#endif

  unsigned long temp = (channel0 * b);
  // do not allow negative lux value
  if (temp < channel1 * m) temp = 0;
  else temp -= channel1 * m;
  // round lsb (2^(LUX_SCALE-1))
  temp += (1 << (TSL2561_LUX_LUXSCALE - 1));

  // strip off fractional portion
  ret = temp >> TSL2561_LUX_LUXSCALE;
}

/////////////////////////////////////////////////////////////////////////////////////////
// TSL2561 class
/////////////////////////////////////////////////////////////////////////////////////////

TSL2561::TSL2561()
{}

void TSL2561::init()
{
  // init i2c
  tsl2561I2cInit();

  // check ready
  if (!tsl2561Ready()) APP_LOGE("[TSL2561]", "TSL2651 sensor not found");

  // init integration time and gain
  tsl2561SetIntegrationTimeAndGain(TSL2561_INTEGRATIONTIME_13MS, TSL2561_GAIN_1X);
}

uint16_t _broadbandCache;
uint16_t _irCache;

void TSL2561::sampleData()
{
  tsl2561GetLuminosity(_broadbandCache, _irCache);
  tsl2561CalculateLuminosity(_luminosity, _broadbandCache, _irCache);
// #ifdef DEBUG_APP
  APP_LOGC("[TSL2561]", "--->lux: %d b: %d, ir: %d", _luminosity, _broadbandCache, _irCache);
// #endif
}
