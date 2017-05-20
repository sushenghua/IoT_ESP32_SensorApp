/*
 * CmdKey command key
 * Copyright (c) 2016 Shenghua Su
 *
 */

#include "CmdKey.h"

#include <string.h>

CmdKey parseCmdKeyString(const char *str)
{
    size_t len = strlen(str);

    if (!str || len == 0) return DoNothing;

    if (strlen("GetSensorData") == len && strcmp("GetSensorData", str) == 0)
        return GetSensorData;

    else if (strlen("GetSensorDataString") == len && strcmp("GetSensorDataString", str) == 0)
        return GetSensorDataString;

    else if (strlen("GetSensorDataJsonString") == len && strcmp("GetSensorDataJsonString", str) == 0)
        return GetSensorDataJsonString;

    else if (strlen("GetSensorCapability") == len && strcmp("GetSensorCapability", str) == 0)
        return GetSensorCapability;

    else if (strlen("Restart") == len && strcmp("Restart", str) == 0)
        return Restart;

    else if (strlen("GetUID") == len && strcmp("GetUID", str) == 0)
        return GetUID;

    else if (strlen("GetFirmwareVersion") == len && strcmp("GetFirmwareVersion", str) == 0)
        return GetFirmwareVersion;

    else if (strlen("GetRawData") == len && strcmp("GetRawData", str) == 0)
        return GetRawData;

    else if (strlen("GetIdfVersion") == len && strcmp("GetIdfVersion", str) == 0)
        return GetIdfVersion;

    else if (strlen("TurnOnDisplay") == len && strcmp("TurnOnDisplay", str) == 0)
        return TurnOnDisplay;

    else if (strlen("UpdateFirmware") == len && strcmp("UpdateFirmware", str) == 0)
        return UpdateFirmware;

    else if (strlen("SetStaSsidPasswd") == len && strcmp("SetStaSsidPasswd", str) == 0)
        return SetStaSsidPasswd;

    else if (strlen("SetSystemConfigMode") == len && strcmp("SetSystemConfigMode", str) == 0)
        return SetSystemConfigMode;

    else if (strlen("DoNothing") == len && strcmp("DoNothing", str) == 0)
        return DoNothing;

    else
        return DoNothing;
}
