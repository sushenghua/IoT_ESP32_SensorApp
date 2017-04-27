/*
 * HttpServer: Wrap the mongoose lib
 * Copyright (c) 2017 Shenghua Su
 *
 */

#ifndef _HTTPSERVER_H
#define _HTTPSERVER_H

#include "mongoose.h"

#define MONGOOSE_HTTP_DEFAULT_POLL_SLEEP 1000

class HttpServer
{
public:
    // constructor
    HttpServer();

    // loop poll
    void poll(int sleepMilli = MONGOOSE_HTTP_DEFAULT_POLL_SLEEP) {
        mg_mgr_poll(&_manager, sleepMilli);
    }

    // config, init and deinit
    void init();
    void deinit();

    void start();

public:
    // for event handler
    void onAccept(struct mg_connection *nc);
    void onHttpRequest(struct mg_connection *nc, struct http_message *hm);
    void onWebsocketHandshakeRequest(struct mg_connection *nc);
    void onWebsocketHandshakeDone(struct mg_connection *nc);
    void onWebsocketFrame(struct mg_connection *nc, struct websocket_message *wm);
    void onClose(struct mg_connection *nc);

protected:
    bool                     _inited;
    struct mg_mgr            _manager;
};

#endif // _HTTPSERVER_H