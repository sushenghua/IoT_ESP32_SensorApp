/*
 * CmdEngine command interpretation
 * Copyright (c) 2016 Shenghua Su
 *
 */

#include "CmdEngine.h"

#include "CmdFormat.h"
#include "AppLog.h"
#include <string.h>
#include "System.h"

CmdEngine::CmdEngine()
: _delegate(NULL)
{}

void CmdEngine::setMqttClientDelegate(MqttClientDelegate *delegate)
{
	_delegate = delegate;
	_delegate->setMessageInterpreter(this);
}

bool CmdEngine::init()
{
    bool succeeded = false;
    if (_delegate) {
        _delegate->addSubTopic("/api/mydev/cmd");
        _delegate->subscribeTopics();
        succeeded = true;
    }
    return succeeded;
}

void CmdEngine::interpreteMqttMsg(const char* topic, size_t topicLen, const char* msg, size_t msgLen)
{
    // data size check
    if (msgLen < CMD_DATA_AT_LEAST_SIZE) {
        APP_LOGE("[CmdEngine]", "mqtt msg data size must be at least %d", CMD_DATA_AT_LEAST_SIZE);
        return;
    }

    // exec cmd accordingly
    uint8_t *data = (uint8_t *)msg;
    CmdKey cmdKey = (CmdKey)( *(uint16_t *)(data + CMD_DATA_KEY_OFFSET) );

    // only test, should be removed later
    cmdKey = (CmdKey)(msg[0] - '0');

    // execute cmd
    execCmd(cmdKey, (data + CMD_DATA_ARG_OFFSET), msgLen - CMD_DATA_KEY_SIZE);
}

int CmdEngine::execCmd(CmdKey cmdKey, uint8_t *args, size_t argsCount)
{
    switch (cmdKey) {

        case Restart:
            // Todo: save those need to save ...
            System::restart();
            break;

        case GetUID:
        	_delegate->publish("/api/mydev/cmdret", System::macAddress(), strlen(System::macAddress()), 1);
            break;

        case GetIdfVersion:
            _delegate->publish("/api/mydev/cmdret", System::idfVersion(), strlen(System::idfVersion()), 1);
            break;

        case GetFirmwareVersion:
            _delegate->publish("/api/mydev/cmdret", System::firmwareVersion(), strlen(System::firmwareVersion()), 1);
            break;

        case TurnOffLed:
            break;

        default:
            break;
    }

    return 0;
}
