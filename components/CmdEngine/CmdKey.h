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
    TurnOnDisplay           ,//= 7,
    UpdateFirmware          ,//= 8,
    SetStaSsidPasswd        ,//= 9,
    SetSystemConfigMode     ,//= 10,
    Restart                 ,//= 11,
    CmdKeyMaxValue

} CmdKey;

CmdKey strToCmdKey(const char *str);
const char* cmdKeyToStr(CmdKey cmdKey);

#endif // _CMD_KEY_H_INCLUDED
