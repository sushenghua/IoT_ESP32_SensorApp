/*
 * CmdEngine command interpretation
 * Copyright (c) 2016 Shenghua Su
 *
 */

#ifndef _CMD_ENGINE_H
#define _CMD_ENGINE_H

#include "MqttMessageInterpreter.h"
#include "MqttClientDelegate.h"

class CmdEngine : public MqttMessageInterpreter
{
public:
    CmdEngine();

    void init();

    void setMqttClientDelegate(MqttClientDelegate *delegate);

    // MqttMessageInterpreter interface
    virtual void interpreteMqttMsg(const char* topic, size_t topicLen, const char* msg, size_t msgLen);

protected:
	MqttClientDelegate      *_delegate;
};

#endif // _CMD_ENGINE_H
