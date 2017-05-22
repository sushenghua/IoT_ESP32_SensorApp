/*
 * CmdKey Command univeral key
 * Copyright (c) 2016 Shenghua Su
 *
 */

#ifndef _CMD_KEY_H_INCLUDED
#define _CMD_KEY_H_INCLUDED

typedef enum CmdKey {

    DoNothing               = 0,
    GetSensorData           ,//= 1,
    GetSensorCapability     ,//= 2,
    GetDeviceInfo           ,//= 3,
    GetUID                  ,//= 4,
    GetFirmwareVersion      ,//= 5,
    GetIdfVersion           ,//= 6,
    GetHostname             ,//= 7,
    SetHostname             ,//= 8,
    GetStaSsidPasswd        ,//= 9
    SetStaSsidPasswd        ,//= 10,
    GetApSsidPasswd         ,//= 11,
    SetApSsidPasswd         ,//= 12,
    SetSystemConfigMode     ,//= 13,
    TurnOnDisplay           ,//= 14,
    UpdateFirmware          ,//= 15,
    Restart                 ,//= 16,
    CmdKeyMaxValue

} CmdKey;

CmdKey strToCmdKey(const char *str);
const char* cmdKeyToStr(CmdKey cmdKey);

#endif // _CMD_KEY_H_INCLUDED
