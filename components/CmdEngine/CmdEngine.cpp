/*
 * CmdEngine command interpretation
 * Copyright (c) 2016 Shenghua Su
 *
 */

#include "CmdEngine.h"

#include "CmdFormat.h"
#include "CmdKey.h"
#include "AppLog.h"

CmdEngine::CmdEngine()
: _delegate(NULL)
{}

void CmdEngine::interpreteMqttMsg(const char* topic, size_t topicLen, const char* msg, size_t msgLen)
{
    if (msgLen < CMD_DATA_AT_LEAST_SIZE) {
        APP_LOGE("[CmdEngine]", "mqtt msg data size must be at least %d", CMD_DATA_AT_LEAST_SIZE);
        return;
    }
    uint8_t *data = (uint8_t *)msg;
    CmdKey cmdKey = (CmdKey)( *(uint8_t *)(data + CMD_DATA_KEY_OFFSET) );
}
