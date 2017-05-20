/*
 * CmdKey Command univeral key
 * Copyright (c) 2016 Shenghua Su
 *
 */

#ifndef _CMD_KEY_H_INCLUDED
#define _CMD_KEY_H_INCLUDED

typedef enum CmdKey {

    DoNothing               = 0,
    Restart                 = 1,
    GetUID                  = 2,
    GetFirmwareVersion      = 3,
    GetSensorCapability     = 4,
    GetRawData              = 5,
    GetIdfVersion           = 6,
    GetSensorData           = 7,
    GetSensorDataString     = 8,
    GetSensorDataJsonString = 9,
    TurnOnDisplay           = 10,
    UpdateFirmware          = 11,
    SetStaSsidPasswd        = 12,
    SetSystemConfigMode     = 13,

} CmdKey;

CmdKey parseCmdKeyString(const char *str);

#endif // _CMD_KEY_H_INCLUDED
