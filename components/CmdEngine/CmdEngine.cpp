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
#include "Wifi.h"

#include "cJSON.h"


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

CmdKey _parseJsonStringCmd(const char* msg, size_t msgLen, uint8_t *&args, size_t &argsSize, CmdEngine::RetFormat &retFmt)
{
    cJSON *root = cJSON_Parse(msg);
    cJSON *cmd = cJSON_GetObjectItem(root, "cmd");

    APP_LOGC("[CmdEngine]", "json cmd %s", cmd->valuestring);

    CmdKey cmdKey = strToCmdKey(cmd->valuestring);

    cJSON *retfmt = cJSON_GetObjectItem(root, "retfmt");
    if (retfmt && strcmp("json", retfmt->valuestring) == 0) retFmt = CmdEngine::JSON;
    else retFmt = CmdEngine::Binary;

    switch (cmdKey) {
        case SetStaSsidPasswd: {
            cJSON *ssid = cJSON_GetObjectItem(root, "ssid");
            cJSON *pass = cJSON_GetObjectItem(root, "pass");
            if (ssid && pass) {
                _cmdBuf[0] = strlen(ssid->valuestring);
                size_t passLen = strlen(pass->valuestring);
                memcpy(_cmdBuf + 1, ssid->valuestring, _cmdBuf[0]);
                memcpy(_cmdBuf + 1 + _cmdBuf[0], pass->valuestring, passLen);
                args = _cmdBuf;
                argsSize = 1 + _cmdBuf[0] + passLen;
                // APP_LOGC("[CmdEngine]", "setSta: %.*s, ssidLen: %d", argsSize-1, args+1, args[0]);
            }
            else {
                args = _cmdBuf;
                args[0] = 0;
                argsSize = 0;
            }
            break;
        }

        default:
            break;
    }

    cJSON_Delete(root);

    return cmdKey;
}

void appendCmdKeyToJsonString(CmdKey cmdKey, char *jsonStr, size_t &count)
{
    sprintf(jsonStr + count - 1, ",\"cmd\":\"%s\"}", cmdKeyToStr(cmdKey));
    count += strlen(jsonStr + count);
}

void CmdEngine::interpreteMqttMsg(const char* topic, size_t topicLen, const char* msg, size_t msgLen)
{
    bool exec = false;
    CmdKey cmdKey;
    uint8_t *data = NULL;
    size_t size = 0;
    RetFormat retFmt = Binary;

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
                retFmt = Binary;
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
            cmdKey = _parseJsonStringCmd(msg, msgLen, data, size, retFmt);
            exec = true;
            break;
        }
        // app update topic
        if (strncmp(topic, _appUpdater.updateRxTopic(), _appUpdater.updateRxTopicLen()) == 0) {
            _appUpdater.updateLoop(msg, msgLen);
        }
    } while (false);

    if (exec) execCmd(cmdKey, retFmt, data, size);
}

void CmdEngine::interpreteSocketMsg(const void* msg, size_t msgLen, void *userdata)
{
    bool exec = false;
    CmdKey cmdKey;
    uint8_t *data = NULL;
    size_t size = 0;
    RetFormat retFmt = Binary;

    cmdKey = _parseJsonStringCmd((const char *)msg, msgLen, data, size, retFmt);
    exec = true;

    if (exec) execCmd(cmdKey, retFmt, data, size, userdata);
}

char _strBuf[1024];

int CmdEngine::execCmd(CmdKey cmdKey, RetFormat retFmt, uint8_t *args, size_t argsSize, void *userdata)
{
    switch (cmdKey) {

        case GetSensorData: {
            size_t count = 0;
            const void * data = NULL;
            if (retFmt == Binary)
                data = SensorDataPacker::sharedInstance()->dataBlock(count);
            else if (retFmt == JSON) {
                data = SensorDataPacker::sharedInstance()->dataJsonString(count);
                appendCmdKeyToJsonString(cmdKey, (char*)data, count);
            }
            _delegate->replyMessage(data, count, userdata);
            break;
        }

        case GetUID:
            if (retFmt == JSON) {
                sprintf(_strBuf, "{\"ret\":\"%s\", \"cmd\":\"%s\"}", System::instance()->macAddress(), cmdKeyToStr(cmdKey));
                _delegate->replyMessage(_strBuf, strlen(_strBuf), userdata);
            }
            else {
                _delegate->replyMessage(System::instance()->macAddress(),
                                        strlen(System::instance()->macAddress()), userdata);
            }
            break;

        case GetSensorCapability: {
            uint32_t capability = SensorDataPacker::sharedInstance()->sensorCapability();
            if (retFmt == JSON) {
                sprintf(_strBuf, "{\"ret\":\"%u\", \"cmd\":\"%s\"}", capability, cmdKeyToStr(cmdKey));
                _delegate->replyMessage(_strBuf, strlen(_strBuf), userdata);
            }
            else {
                _delegate->replyMessage(&capability, sizeof(capability), userdata);
            }
            break;
        }

        case GetIdfVersion:
            if (retFmt == JSON) {
                sprintf(_strBuf, "{\"ret\":\"%s\", \"cmd\":\"%s\"}", System::instance()->idfVersion(), cmdKeyToStr(cmdKey));
                _delegate->replyMessage(_strBuf, strlen(_strBuf), userdata);
            }
            else {
                _delegate->replyMessage(System::instance()->idfVersion(),
                                        strlen(System::instance()->idfVersion()), userdata);
            }
            break;

        case GetFirmwareVersion:
            if (retFmt == JSON) {
                sprintf(_strBuf, "{\"ret\":\"%s\", \"cmd\":\"%s\"}", System::instance()->firmwareVersion(), cmdKeyToStr(cmdKey));
                _delegate->replyMessage(_strBuf, strlen(_strBuf), userdata);
            }
            else {
                _delegate->replyMessage(System::instance()->firmwareVersion(),
                                        strlen(System::instance()->firmwareVersion()), userdata);
            }
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

        case SetStaSsidPasswd: {
            uint8_t ssidLen = args[0];
            char ssid[32] = {0};
            char pass[64] = {0};
            strncat(ssid, (const char*)(args+1), ssidLen);
            strncat(pass, (const char*)(args+1+ssidLen), argsSize-1-ssidLen);
            Wifi::instance()->setStaConfig(ssid, pass);
            Wifi::instance()->saveConfig();
            break;
        }

        case SetSystemConfigMode:
            System::instance()->setConfigMode((System::ConfigMode)args[0]);
            break;

        default:
            break;
    }

    return 0;
}
