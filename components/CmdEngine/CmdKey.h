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
    GetUID                  ,//= 3,
    GetFirmwareVersion      ,//= 4,
    GetIdfVersion           ,//= 5,
    TurnOnDisplay           ,//= 6,
    UpdateFirmware          ,//= 7,
    SetStaSsidPasswd        ,//= 8,
    SetSystemConfigMode     ,//= 9,
    Restart                 ,//= 10,
    CmdKeyMaxValue

} CmdKey;

CmdKey strToCmdKey(const char *str);
const char* cmdKeyToStr(CmdKey cmdKey);

#endif // _CMD_KEY_H_INCLUDED
