/*
 * Adc: Wrap the esp ADC
 * Copyright (c) 2017 Shenghua Su
 *
 */

#ifndef _ADC_H
#define _ADC_H

#include <string.h>
#include "driver/adc.h"

#define ADC_DEFAULT_ATTENUATON     ADC_ATTEN_11db
#define ADC_DEFAULT_BITWIDTH       ADC_WIDTH_12Bit

class Adc
{
public:
    // constructor
    Adc();

    // init
    void init(adc1_channel_t channel,
              adc_bits_width_t bitWidth = ADC_DEFAULT_BITWIDTH,
              adc_atten_t attenuaton = ADC_DEFAULT_ATTENUATON);

    // communication
    int readVoltage();

protected:
    adc1_channel_t      _channel;
};

#endif // _ADC_H
