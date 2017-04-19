/*
 * MqttMessageInterpreter: communication message interpreter
 * Copyright (c) 2017 Shenghua Su
 *
 */

#ifndef _MQTT_MESSAGE_INTERPRETER_H
#define _MQTT_MESSAGE_INTERPRETER_H

#include <stddef.h>

class MqttMessageInterpreter
{
public:
    // virtual interface
    virtual void interpreteMqttMsg(const char* topic, size_t topicLen, const char* msg, size_t msgLen) = 0;
};

#endif // _MQTT_MESSAGE_INTERPRETER_H