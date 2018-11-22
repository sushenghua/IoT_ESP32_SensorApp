/*
 * Adc: Wrap the esp ADC
 * Copyright (c) 2017 Shenghua Su
 *
 */

#include "Adc.h"
#include "AppLog.h"

Adc::Adc()
{}

void Adc::init(adc1_channel_t channel, adc_bits_width_t bitWidth, adc_atten_t attenuaton)
{
    APP_LOGI("[Adc]", "init ADC with channel %d", channel);
    _channel = channel;
    adc1_config_width(bitWidth);
    adc1_config_channel_atten(channel, attenuaton);
}

int Adc::readVoltage()
{
    return adc1_get_raw(_channel);
}
