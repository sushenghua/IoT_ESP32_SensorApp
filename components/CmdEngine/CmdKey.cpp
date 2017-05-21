/*
 * CmdKey command key
 * Copyright (c) 2016 Shenghua Su
 *
 */

#include "CmdKey.h"

#include <string.h>

static const char * const CmdKeyStr[] = {
    "DoNothing",
    "GetSensorData",
    "GetSensorCapability",
    "GetDeviceInfo",
    "GetUID",
    "GetFirmwareVersion",
    "GetIdfVersion",
    "GetHostname",
    "SetHostname",
    "SetStaSsidPasswd",
    "SetSystemConfigMode",
    "TurnOnDisplay",
    "UpdateFirmware",
    "Restart"
};

CmdKey strToCmdKey(const char *str)
{
    size_t len = strlen(str);

    if (!str || len == 0) return DoNothing;

    for (int i = 0; i < CmdKeyMaxValue; ++i) {
        if (strlen(CmdKeyStr[i]) == len && strcmp(CmdKeyStr[i], str) == 0)
            return CmdKey(i);
    }

    return DoNothing;
}

const char* cmdKeyToStr(CmdKey cmdKey)
{
    return CmdKeyStr[cmdKey];
}
