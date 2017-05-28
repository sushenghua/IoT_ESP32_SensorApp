/*
 * CO2Sensor Wrap communicate with CO2 sensor
 * Copyright (c) 2017 Shenghua Su
 *
 */

#include "CO2Sensor.h"
// #include "../Timer/SampleTimer.h"

// // ------ co2 sample timer
// extern TIM_HandleTypeDef htim2;
// SampleTimer _co2SampleTimer(&htim2);

// ------ co2 sensor
#define CO2_VALUE_POS                 0
#define CO2_ACQUIRE_CMD_PROTOCOL_LEN  7
#define CO2_ACQUIRE_CMD_LEN           5

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
: _rxCount(CO2_RX_PROTOCOL_LENGTH)
{
    _prepareCO2Cmd();
    clearCache();
    //Uart::registerCallback(this);
}

void CO2Sensor::clearCache()
{
    _co2Data.clear();
}

void CO2Sensor::startSampling()
{
    // _co2SampleTimer.setSampler(this);
    // _co2SampleTimer.start();
}

void CO2Sensor::sampleData()
{
    //txDMA(CO2_ACQUIRE_CMD, CO2_ACQUIRE_CMD_PROTOCOL_LEN);
    //printf("->require data call %s\n", f ? "success" : "failed");
}

void CO2Sensor::onTxComplete()
{
    //rxDMA(_rxBuf, _rxCount);
    //printf("->accept data call %s\n", f ? "success" : "failed");
}

void CO2Sensor::onRxComplete()
{
    for (int i = 0; i < _rxCount; ++i) {
        _parser.parse(_rxBuf[i]);
        if (_parser.frameState() == FRAME_READY) {
            _co2Data.co2 = _parser.valueAt(CO2_VALUE_POS);
            _co2Data.calculateLevel();
            if (_dc) _dc->setCO2Data(_co2Data, false);
        }
    }
    //requireData(); // called in timer
}

void CO2Sensor::onError(uint32_t err)
{
    //printf("--->co2 sensor uart err code: %d\n", err);
}
