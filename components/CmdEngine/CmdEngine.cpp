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

  CmdKey cmdKeyRet = DoNothing;

  switch (cmdKey) {

    case SetHostname: {
      cJSON *hostname = cJSON_GetObjectItem(root, "hostname");
      if (hostname) {
        args = _cmdBuf;
        argsSize = strlen(hostname->valuestring);
        memcpy(args, hostname->valuestring, argsSize);
        cmdKeyRet = cmdKey;
      }
      break;
    }

    case SetStaSsidPass:
    case SetApSsidPass:
    case AppendAltApSsidPass: {
      cJSON *ssid = cJSON_GetObjectItem(root, "ssid");
      cJSON *pass = cJSON_GetObjectItem(root, "pass");
      if (ssid && pass) {
        args = _cmdBuf;
        args[0] = strlen(ssid->valuestring);
        size_t passLen = strlen(pass->valuestring);
        memcpy(args + 1, ssid->valuestring, args[0]);
        memcpy(args + 1 + args[0], pass->valuestring, passLen);
        argsSize = 1 + args[0] + passLen;
        cmdKeyRet = cmdKey;
        // APP_LOGC("[CmdEngine]", "setSta: %.*s, ssidLen: %d", argsSize-1, args+1, args[0]);
      }
      break;
    }

    case SetSystemDeployMode: {
      cJSON *mode = cJSON_GetObjectItem(root, "mode");
      if (mode) {
        args = _cmdBuf;
        argsSize = 1;
        for (int i=0; i<DeployModeMax; ++i) {
          DeployMode m = (DeployMode)i;
          const char* mstr = deployModeStr(m);
          if (strlen(mode->valuestring) == strlen(mstr) && strcmp(mode->valuestring, mstr) == 0) {
            args[0] = m;
            cmdKeyRet = cmdKey;
            break;
          }
        }
      }
      break;
    }

    case SetSensorType: {
      args = _cmdBuf;
      args[1] = args[0] = (uint8_t)NoneSensor;
      argsSize = 2;
      bool pmArgOK = false;
      bool co2ArgOK = false;
      cJSON *pm = cJSON_GetObjectItem(root, "pm");
      if (pm) {
        for (int i=0; i<SensorTypeMax; ++i) {
          SensorType st = (SensorType)i;
          const char* ststr = sensorTypeStr(st);
          if (strlen(pm->valuestring) == strlen(ststr) && strcmp(pm->valuestring, ststr) == 0) {
            args[0] = (uint8_t)st;
            pmArgOK = true;
            break;
          }
        }
      }
      cJSON *co2 = cJSON_GetObjectItem(root, "co2");
      if (co2) {
        for (int i=0; i<SensorTypeMax; ++i) {
          SensorType st = (SensorType)i;
          const char* ststr = sensorTypeStr(st);
          if (strlen(co2->valuestring) == strlen(ststr) && strcmp(co2->valuestring, ststr) == 0) {
            args[1] = (uint8_t)st;
            co2ArgOK = true;
            break;
          }
        }
      }
      if (pmArgOK && co2ArgOK)
        cmdKeyRet = cmdKey;
      break;
    }

    case TurnOnDisplay:
    case TurnOnAutoAdjustDisplay: {
      cJSON *on = cJSON_GetObjectItem(root, "on");
      if (on) {
        args = _cmdBuf;
        if (strlen(on->valuestring) == strlen("on") &&
            strcmp(on->valuestring, "on") == 0) {
          args[0] = 1;
          argsSize = 1;
          cmdKeyRet = cmdKey;
        }
        else if (strlen(on->valuestring) == strlen("off") &&
                 strcmp(on->valuestring, "off") == 0) {
          args[0] = 0;
          argsSize = 1;
          cmdKeyRet = cmdKey;
        }
      }
      break;
    }

    default:
      cmdKeyRet = cmdKey;
      break;
  }

  cJSON_Delete(root);

  return cmdKeyRet;
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

void replyJsonResult(ProtocolDelegate *delegate, const char *str, CmdKey cmdKey, void *userdata)
{
  sprintf(_strBuf, "{\"ret\":\"%s\", \"cmd\":\"%s\"}", str, cmdKeyToStr(cmdKey));
  delegate->replyMessage(_strBuf, strlen(_strBuf), userdata);
}

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

    case GetDeviceInfo:
      if (retFmt == JSON) {
        sprintf(_strBuf, "{\"ret\":{\"uid\":\"%s\",\"cap\":\"%u\",\"libv\":\"%s\",\"firmv\":\"%s\",\"model\":\"%s\",\"alcd\":\"%s\",\"deploy\":\"%s\",\"hostname\":\"%s\"}, \"cmd\":\"%s\"}",
                System::instance()->uid(),
                System::instance()->devCapability(),
                System::instance()->idfVersion(),
                System::instance()->firmwareVersion(),
                System::instance()->model(),
                System::instance()->displayAutoAdjustOn()? "on" : "off",
                deployModeStr(System::instance()->deployMode()),
                Wifi::instance()->getHostName(),
                cmdKeyToStr(cmdKey));
        _delegate->replyMessage(_strBuf, strlen(_strBuf), userdata);
      }
      break;

    case GetUID:
      if (retFmt == JSON) {
        replyJsonResult(_delegate, System::instance()->uid(), cmdKey, userdata);
      }
      else {
        _delegate->replyMessage(System::instance()->uid(),
                    strlen(System::instance()->uid()), userdata);
      }
      break;

    case GetSensorCapability: {
      uint32_t capability = System::instance()->devCapability();
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
        replyJsonResult(_delegate, System::instance()->idfVersion(), cmdKey, userdata);
      }
      else {
        _delegate->replyMessage(System::instance()->idfVersion(),
                                strlen(System::instance()->idfVersion()), userdata);
      }
      break;

    case GetFirmwareVersion:
      if (retFmt == JSON) {
        replyJsonResult(_delegate, System::instance()->firmwareVersion(), cmdKey, userdata);
      }
      else {
        _delegate->replyMessage(System::instance()->firmwareVersion(),
                                strlen(System::instance()->firmwareVersion()), userdata);
      }
      break;

    case GetHostname: {
      const char *hostname = Wifi::instance()->getHostName();
      if (retFmt == JSON) {
        replyJsonResult(_delegate, hostname, cmdKey, userdata);
      }
      else {
        _delegate->replyMessage(hostname, strlen(hostname), userdata);
      }
      break;
    }

    case SetHostname:
      sprintf(_strBuf, "%.*s", argsSize, (const char*)args);
      Wifi::instance()->setHostName(_strBuf);
      Wifi::instance()->saveConfig();
      break;

    case TurnOnDisplay:
      System::instance()->turnDisplayOn(args[0] != 0);
      break;

    case TurnOnAutoAdjustDisplay:
      System::instance()->turnDisplayAutoAdjustOn(args[0] != 0);
      break;

    case UpdateFirmware:
      _appUpdater.update();
      break;

    case Restart:
      System::instance()->restart();
      break;

    case GetStaSsidPass:
    case GetApSsidPass: {
      const char *ssid = NULL;
      const char *pass = NULL;
      if (cmdKey == GetStaSsidPass) {
        ssid = Wifi::instance()->staSsid();
        pass = Wifi::instance()->staPassword();
      }
      else if (cmdKey == GetApSsidPass) {
        ssid = Wifi::instance()->apSsid();
        pass = Wifi::instance()->apPassword();
      }
      if (retFmt == JSON) {
        sprintf(_strBuf, "{\"ret\":{\"ssid\":\"%s\",\"pass\":\"%s\"}, \"cmd\":\"%s\"}",
                ssid, pass, cmdKeyToStr(cmdKey));
        _delegate->replyMessage(_strBuf, strlen(_strBuf), userdata);
      }
      else {
        _strBuf[0] = strlen(ssid);
        sprintf(_strBuf + 1, "%s%s", ssid, pass);
        _delegate->replyMessage(_strBuf, 1 + strlen(_strBuf + 1), userdata);
      }
      break;
    }

    case GetAltApSsidPassList: {
      SsidPasswd *list = NULL;
      uint8_t head, count;
      Wifi::instance()->getAltApConnectionSsidPassword(list, head, count);
      if (retFmt == JSON) {
        size_t offset = 0;
        sprintf(_strBuf + offset, "{\"cmd\":\"%s\",\"ret\":[", cmdKeyToStr(cmdKey));
        offset += strlen(_strBuf + offset);
        uint8_t index = 0;
        for (uint8_t i = 0; i < count; ++i) {
          index = (head + i) % count;
          sprintf(_strBuf + offset, "{\"ssid\":\"%s\",\"pass\":\"%s\"}%s",
                  list[index].ssid, list[index].password, (i<count-1)?",":"");
          offset += strlen(_strBuf + offset);
        }
        sprintf(_strBuf + offset, "]}");
        offset += strlen(_strBuf + offset);
        _delegate->replyMessage(_strBuf, offset, userdata);
      }
      break;
    }

    case SetStaSsidPass:
    case SetApSsidPass:
    case AppendAltApSsidPass: {
      uint8_t ssidLen = args[0];
      char ssid[32] = {0};
      char pass[64] = {0};
      strncat(ssid, (const char*)(args+1), ssidLen);
      strncat(pass, (const char*)(args+1+ssidLen), argsSize-1-ssidLen);
      if (cmdKey == SetStaSsidPass)
        Wifi::instance()->setStaConfig(ssid, pass, true);
      else if (cmdKey == SetApSsidPass)
        Wifi::instance()->setApConfig(ssid, pass, true);
      else if (cmdKey == AppendAltApSsidPass)
        Wifi::instance()->appendAltApConnectionSsidPassword(ssid, pass);
      Wifi::instance()->saveConfig();
      break;
    }

    case ClearAltApSsidPassList:
      Wifi::instance()->clearAltApConnectionSsidPassword();
      Wifi::instance()->saveConfig();
      break;

    case GetSystemDeployMode: {
      const char * modeStr = deployModeStr(System::instance()->deployMode());
      if (retFmt == JSON) {
        replyJsonResult(_delegate, modeStr, cmdKey, userdata);
      }
      else {
        _delegate->replyMessage(modeStr, strlen(modeStr), userdata);
      }
      break;
    }

    case SetSystemDeployMode:
      System::instance()->setDeployMode((DeployMode)args[0]);
      break;

    case SetSensorType:
      System::instance()->setSensorType((SensorType)args[0], (SensorType)args[1]);
      break;

    default:
      break;
  }

  return 0;
}
