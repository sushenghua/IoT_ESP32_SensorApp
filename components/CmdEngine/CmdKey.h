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
    GetRawData              = 4,
    GetIdfVersion           = 5,
    GetSensorData           = 6,
    GetSensorDataString     = 7,
    TurnOnLed               = 8,

} CmdKey;

#endif // _CMD_KEY_H_INCLUDED
