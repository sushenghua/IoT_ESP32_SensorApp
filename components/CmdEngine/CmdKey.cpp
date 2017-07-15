/*
 * CmdKey command key
 * Copyright (c) 2016 Shenghua Su
 *
 */

#include "CmdKey.h"

#include <string.h>

static const char * const CmdKeyStr[] = {
    "DoNothing",                // 0
    "GetSensorData",            // 1
    "GetSensorCapability",      // 2
    "GetDeviceInfo",            // 3
    "GetUID",                   // 4
    "GetFirmwareVersion",       // 5
    "GetIdfVersion",            // 6
    "GetHostname",              // 7
    "SetHostname",              // 8
    "GetStaSsidPass",           // 9
    "SetStaSsidPass",           // 10
    "GetApSsidPass",            // 11
    "SetApSsidPass",            // 12
    "GetAltApSsidPassList",     // 13
    "AppendAltApSsidPass",      // 14
    "ClearAltApSsidPassList",   // 15
    "SetSystemDeployMode",      // 16
    "TurnOnDisplay",            // 17
    "UpdateFirmware",           // 18
    "Restart"                   // 19
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
