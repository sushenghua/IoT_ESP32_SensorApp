/*
 * CmdEngine command interpretation
 * Copyright (c) 2016 Shenghua Su
 *
 */

#include "CmdEngine.h"

#include <string.h>
#include "CmdFormat.h"
#include "AppLog.h"
#include "System.h"
#include "SensorDataPacker.h"
#include "DisplayController.h"


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

#define CMD_RET_MSG_TOPIC   "/api/mydev/cmdret"
#define PUB_MSG_QOS         1

int CmdEngine::execCmd(CmdKey cmdKey, uint8_t *args, size_t argsCount)
{
    switch (cmdKey) {

        case GetSensorData: {
            size_t count;
            const uint8_t * data = SensorDataPacker::sharedInstance()->dataBlock(count);
            _delegate->publish(CMD_RET_MSG_TOPIC, data, count, PUB_MSG_QOS);
            break;
        }

        case GetSensorDataString: {
            size_t count;
            const char * data = SensorDataPacker::sharedInstance()->dataString(count);
            _delegate->publish(CMD_RET_MSG_TOPIC, data, count, PUB_MSG_QOS);
            break;
        }

        case GetUID:
            _delegate->publish(CMD_RET_MSG_TOPIC, System::macAddress(), strlen(System::macAddress()), PUB_MSG_QOS);
            break;

        case GetIdfVersion:
            _delegate->publish(CMD_RET_MSG_TOPIC, System::idfVersion(), strlen(System::idfVersion()), PUB_MSG_QOS);
            break;

        case GetFirmwareVersion:
            _delegate->publish(CMD_RET_MSG_TOPIC, System::firmwareVersion(), strlen(System::firmwareVersion()), PUB_MSG_QOS);
            break;

        case TurnOnLed: {
            bool on = bool(args[0] - '0');
            DisplayController::activeInstance()->turnOn(on);
            break;
        }

        case Restart:
            // Todo: save those need to save ...
            System::restart();
            break;

        default:
            break;
    }

    return 0;
}
