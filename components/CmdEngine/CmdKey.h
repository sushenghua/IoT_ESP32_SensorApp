/*
 * CmdKey Command univeral key
 * Copyright (c) 2016 Shenghua Su
 *
 */

#ifndef _CMD_KEY_H_INCLUDED
#define _CMD_KEY_H_INCLUDED

typedef enum CmdKey {

    DoNothing               = 0,
    Reset                   = 1,
    GetUID                  = 2,
    GetFirmwareVersion      = 3,
    GetRawData              = 4,

} CmdKey;

#endif // _CMD_KEY_H_INCLUDED
