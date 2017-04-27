/*
 * CmdEngine command interpretation
 * Copyright (c) 2016 Shenghua Su
 *
 */

#ifndef _CMD_ENGINE_H
#define _CMD_ENGINE_H

#include "ProtocolMessageInterpreter.h"
#include "ProtocolDelegate.h"
#include "CmdKey.h"

class CmdEngine : public ProtocolMessageInterpreter
{
public:
    CmdEngine();

    bool init();

    void enableUpdate(bool enabled = true);

    int execCmd(CmdKey cmdKey, uint8_t *args = NULL, size_t argsSize = 0);

    void setProtocolDelegate(ProtocolDelegate *delegate);

    // ProtocolMessageInterpreter interface
    virtual void interpreteMqttMsg(const char* topic, size_t topicLen, const char* msg, size_t msgLen);
    virtual void interpreteSocketMsg(const char* msg, size_t msgLen);

protected:
	bool                   _updateEnabled;
    ProtocolDelegate      *_delegate;
};

#endif // _CMD_ENGINE_H
