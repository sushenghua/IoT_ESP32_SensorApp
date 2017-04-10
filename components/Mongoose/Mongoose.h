/*
 * Mongoose: Wrap the mongoose lib
 * Copyright (c) 2017 Shenghua Su
 *
 */

#ifndef _MONGOOSE_H
#define _MONGOOSE_H

#include "mongoose.h"

#define MONGOOSE_DEFAULT_POLL_SLEEP 1000

class Mongoose
{
public:
    // constructor
    Mongoose();

    // loop poll
    void poll(int sleepMilli = MONGOOSE_DEFAULT_POLL_SLEEP) {
        mg_mgr_poll(&_manager, sleepMilli);
    }

    // config, init and deinit
    void init();
    void deinit();

protected:
    bool                     _inited;
    struct mg_mgr            _manager;
};

#endif // _MONGOOSE_H