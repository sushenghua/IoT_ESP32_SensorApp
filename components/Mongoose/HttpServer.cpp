/*
 * HttpServer: Wrap the mongoose lib
 * Copyright (c) 2017 Shenghua Su
 *
 */

#include "HttpServer.h"

#include "esp_system.h"
#include "AppLog.h"
#include "Wifi.h"

#define MG_HTTP_LISTEN_ADDR "80"

HttpServer::HttpServer()
: _inited(false)
{}

static void mongoose_http_event_handler(struct mg_connection *nc, int ev, void *p)
{
  static const char *reply_fmt =
        "HTTP/1.0 200 OK\r\n"
        "Connection: close\r\n"
        "Content-Type: text/plain\r\n"
        "\r\n"
        "Sensor %s\n";

    switch (ev) {
        // connected
        case MG_EV_ACCEPT: {
            char addr[32];
            mg_sock_addr_to_str(&nc->sa, addr, sizeof(addr),
                                MG_SOCK_STRINGIFY_IP | MG_SOCK_STRINGIFY_PORT);
            APP_LOGI("[HttpServer]", "connection %p from %s", nc, addr);
            break;
        }
        // HTTP
        case MG_EV_HTTP_REQUEST: {
            char addr[32];
            struct http_message *hm = (struct http_message *) p;
            mg_sock_addr_to_str(&nc->sa, addr, sizeof(addr),
                                MG_SOCK_STRINGIFY_IP | MG_SOCK_STRINGIFY_PORT);
            APP_LOGI("[HttpServer]", "HTTP request from %s: %.*s %.*s", addr, (int) hm->method.len,
                                   hm->method.p, (int) hm->uri.len, hm->uri.p);
            // mg_printf(nc, reply_fmt, addr);
            mg_printf(nc, reply_fmt, addr);
            nc->flags |= MG_F_SEND_AND_CLOSE;
            break;
        }
        // cose
        case MG_EV_CLOSE: {
            APP_LOGI("[HttpServer]", "connection %p closed", nc);
            break;
        }
    }  
}

void HttpServer::init()
{
    if (!_inited) {
        struct mg_connection *nc;
    
        APP_LOGI("[HttpServer]", "mongoose version: %s", MG_VERSION);
        APP_LOGI("[HttpServer]", "Free RAM: %d bytes", esp_get_free_heap_size());
    
        _mode = mode;
        mg_mgr_init(&_manager, this);
    
        APP_LOGI("[HttpServer]", "HTTP init server");
        nc = mg_bind(&_manager, MG_HTTP_LISTEN_ADDR, mongoose_http_event_handler);
        if (nc == NULL) {
            APP_LOGE("[HttpServer]", "HTTP init server failed");
            return;
        }
        mg_set_protocol_http_websocket(nc);

        _inited = true;
    }
}

void HttpServer::deinit()
{
    if (_inited) {
        mg_mgr_free(&_manager);
        _inited = false;
    }
}

