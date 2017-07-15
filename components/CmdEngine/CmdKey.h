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
    GetStaSsidPass          ,//= 9
    SetStaSsidPass          ,//= 10,
    GetApSsidPass           ,//= 11,
    SetApSsidPass           ,//= 12,
    GetAltApSsidPassList    ,//= 13,
    AppendAltApSsidPass     ,//= 14,
    ClearAltApSsidPassList  ,//= 15,
    SetSystemDeployMode     ,//= 16,
    TurnOnDisplay           ,//= 17,
    UpdateFirmware          ,//= 18,
    Restart                 ,//= 19,
    CmdKeyMaxValue

} CmdKey;

CmdKey strToCmdKey(const char *str);
const char* cmdKeyToStr(CmdKey cmdKey);

#endif // _CMD_KEY_H_INCLUDED
