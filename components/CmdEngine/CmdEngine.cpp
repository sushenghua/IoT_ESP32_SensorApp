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
#include "AppUpdater.h"
#include "MqttClientDelegate.h"


#define TOPIC_API_CMD           "api/mydev/cmd"
#define TOPIC_API_STR_CMD       "api/mydev/strcmd"

#define CMD_RET_MSG_TOPIC       "api/mydev/cmdret"
#define PUB_MSG_QOS             1


AppUpdater _appUpdater;

CmdEngine::CmdEngine()
: _delegate(NULL)
{}

void CmdEngine::setProtocolDelegate(ProtocolDelegate *delegate)
{
    _delegate = delegate;
    _delegate->setMessageInterpreter(this);
}

bool CmdEngine::init()
{
    bool succeeded = false;
    if (_delegate) {
        _delegate->setup();
        succeeded = true;
    }
    return succeeded;
}

void CmdEngine::enableUpdate(bool enabled)
{
    if (_updateEnabled != enabled) {
        _updateEnabled = enabled;
        if (_updateEnabled && _delegate) {
            _appUpdater.init();
            _appUpdater.setMqttClientDelegate(static_cast<MqttClientDelegate*>(_delegate));
        }
    }
}

uint8_t  _cmdBuf[1024];

CmdKey _parseStringCmd(const char* msg, size_t msgLen, uint8_t *&args, size_t &argsSize)
{
    // parse string
    args = _cmdBuf + 2;
    argsSize = 1;
    args[0] = msg[1] - '0';
    return (CmdKey)(msg[0] - '0');
}

void CmdEngine::interpreteMqttMsg(const char* topic, size_t topicLen, const char* msg, size_t msgLen)
{
    bool exec = false;
    CmdKey cmdKey;
    uint8_t *data = NULL;
    size_t size = 0;

    do {
        // binary data command topic
        if (topicLen == strlen(MqttClientDelegate::cmdTopic()) &&
            strncmp(topic, MqttClientDelegate::cmdTopic(), topicLen) == 0) {
            // data size check
            if (msgLen >= CMD_DATA_AT_LEAST_SIZE) {
                data = (uint8_t *)msg;
                cmdKey = (CmdKey)( *(uint16_t *)(data + CMD_DATA_KEY_OFFSET) );
                data += CMD_DATA_ARG_OFFSET;
                size = msgLen - CMD_DATA_KEY_SIZE;
                exec = true;
            }
            else {
                APP_LOGE("[CmdEngine]", "mqtt msg data size must be at least %d", CMD_DATA_AT_LEAST_SIZE);
            }
            break;
        }
        // string data command topic
        if (topicLen == strlen(MqttClientDelegate::strCmdTopic()) &&
            strncmp(topic, MqttClientDelegate::strCmdTopic(), topicLen) == 0) {
            cmdKey = _parseStringCmd(msg, msgLen, data, size);
            exec = true;
            break;
        }
        // app update topic
        if (strncmp(topic, _appUpdater.updateRxTopic(), _appUpdater.updateRxTopicLen()) == 0) {
            _appUpdater.updateLoop(msg, msgLen);
        }
    } while (false);

    if (exec) execCmd(cmdKey, data, size);
}

void CmdEngine::interpreteSocketMsg(const char* msg, size_t msgLen)
{

}

int CmdEngine::execCmd(CmdKey cmdKey, uint8_t *args, size_t argsSize)
{
    switch (cmdKey) {

        case GetSensorData: {
            size_t count;
            const uint8_t * data = SensorDataPacker::sharedInstance()->dataBlock(count);
            _delegate->replyMessage(data, count);
            break;
        }

        case GetSensorDataString: {
            size_t count;
            const char * data = SensorDataPacker::sharedInstance()->dataString(count);
            _delegate->replyMessage(data, count);
            break;
        }

        case GetUID:
            _delegate->replyMessage(System::instance()->macAddress(),
                                    strlen(System::instance()->macAddress()));
            break;

        case GetIdfVersion:
            _delegate->replyMessage(System::instance()->idfVersion(),
                                    strlen(System::instance()->idfVersion()));
            break;

        case GetFirmwareVersion:
            _delegate->replyMessage(System::instance()->firmwareVersion(),
                                    strlen(System::instance()->firmwareVersion()));
            break;

        case TurnOnDisplay:
            DisplayController::activeInstance()->turnOn(args[0] != 0);
            break;

        case UpdateFirmware:
            _appUpdater.update();
            break;

        case Restart:
            // Todo: save those need to save ...
            System::instance()->restart();
            break;

        default:
            break;
    }

    return 0;
}
