/*
 * HttpServer: Wrap the mongoose lib
 * Copyright (c) 2017 Shenghua Su
 *
 */

#ifndef _HTTPSERVER_H
#define _HTTPSERVER_H

#include "mongoose.h"

#define MONGOOSE_DEFAULT_POLL_SLEEP 1000

class HttpServer
{
public:
    // constructor
    HttpServer();

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

#endif // _HTTPSERVER_H