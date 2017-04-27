/*
 * ProtocolDelegate: interface for communication with protocol object
 * Copyright (c) 2017 Shenghua Su
 *
 */

#ifndef _PROTOCOL_DELEGATE_H
#define _PROTOCOL_DELEGATE_H

#include <stdint.h>
#include "ProtocolMessageInterpreter.h"

class ProtocolDelegate
{
public:
	ProtocolDelegate(): _msgInterpreter(NULL) {}
    // message interpreter
    void setMessageInterpreter(ProtocolMessageInterpreter *interpreter) {
        _msgInterpreter = interpreter;
    }
    // virtual functions
    virtual void setup() = 0;
    virtual void replyMessage(const void *data, size_t length) = 0;

protected:
    ProtocolMessageInterpreter *_msgInterpreter;
};

#endif // _PROTOCOL_DELEGATE_H
