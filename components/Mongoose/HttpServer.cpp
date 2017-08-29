/*
 * HttpServer: Wrap the mongoose lib
 * Copyright (c) 2017 Shenghua Su
 *
 */

#include "HttpServer.h"

#include "esp_system.h"
#include "AppLog.h"
#include "Wifi.h"


/////////////////////////////////////////////////////////////////////////////////////////
// ------ mongoose http event handler
/////////////////////////////////////////////////////////////////////////////////////////

static void mongoose_http_event_handler(struct mg_connection *nc, int event, void *p)
{
    HttpServer *httpServer = static_cast<HttpServer*>(nc->user_data);

    switch (event) {

        case MG_EV_ACCEPT:
            httpServer->onAccept(nc);
            break;

        case MG_EV_HTTP_REQUEST:
            httpServer->onHttpRequest(nc, (struct http_message *)p);
            break;

        case MG_EV_WEBSOCKET_HANDSHAKE_REQUEST:
            httpServer->onWebsocketHandshakeRequest(nc);
            break;

        case MG_EV_WEBSOCKET_HANDSHAKE_DONE:
            httpServer->onWebsocketHandshakeDone(nc);
            break;

        case MG_EV_WEBSOCKET_FRAME:
            httpServer->onWebsocketFrame(nc, (struct websocket_message *)p);
            break;

        case MG_EV_CLOSE:
            httpServer->onClose(nc);
            break;
    }  
}


/////////////////////////////////////////////////////////////////////////////////////////
// ------ HttpServer class
/////////////////////////////////////////////////////////////////////////////////////////
#define MG_HTTP_LISTEN_ADDR "80"

HttpServer::HttpServer()
: _inited(false)
, _clientConnected(false)
{}

void HttpServer::init()
{
    if (!_inited) {
        APP_LOGI("[HttpServer]", "init server (Mongoose version: %s, Free RAM: %d bytes)",
                                  MG_VERSION, esp_get_free_heap_size());
        mg_mgr_init(&_manager, NULL);

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

void HttpServer::start()
{
    if (_inited) {
        struct mg_bind_opts opts;
        memset(&opts, 0, sizeof(opts));
        opts.user_data = static_cast<void*>(this);
#if MG_ENABLE_SSL
        // opts.ssl_cert = 
        // opts.ssl_key = 
        // opts.ssl_ca_cert = 
#endif
        struct mg_connection *nc;
        APP_LOGI("[HttpServer]", "init http server");
        nc = mg_bind_opt(&_manager, MG_HTTP_LISTEN_ADDR, mongoose_http_event_handler, opts);
        if (nc == NULL) {
            APP_LOGE("[HttpServer]", "init http server failed");
            return;
        }
        mg_set_protocol_http_websocket(nc);
    }
}

void HttpServer::setup()
{}

void HttpServer::replyMessage(const void *data, size_t length, void *userdata, int flag)
{
    // WEBSOCKET_OP_TEXT 1,   WEBSOCKET_OP_BINARY 2
    if (flag == PROTOCOL_MSG_FORMAT_BINARY)
        mg_send_websocket_frame((mg_connection *)userdata, WEBSOCKET_OP_BINARY, data, length);
    else if (flag == PROTOCOL_MSG_FORMAT_TEXT)
        mg_send_websocket_frame((mg_connection *)userdata, WEBSOCKET_OP_TEXT, data, length);
}

static char addr[32];

void HttpServer::onAccept(struct mg_connection *nc)
{
    mg_sock_addr_to_str(&nc->sa, addr, sizeof(addr),
                        MG_SOCK_STRINGIFY_IP | MG_SOCK_STRINGIFY_PORT);
    APP_LOGI("[HttpServer]", "new http connection from %s ((nc: %p)", addr, nc);
}

void HttpServer::onHttpRequest(struct mg_connection *nc, struct http_message *hm)
{
    static const char *reply_fmt =
        "HTTP/1.0 200 OK\r\n"
        "Connection: close\r\n"
        "Content-Type: text/plain\r\n"
        "\r\n"
        "Sensor %s\n";
    mg_sock_addr_to_str(&nc->sa, addr, sizeof(addr),
                        MG_SOCK_STRINGIFY_IP | MG_SOCK_STRINGIFY_PORT);
    APP_LOGI("[HttpServer]", "http request from %s: %.*s %.*s", addr, (int) hm->method.len,
                              hm->method.p, (int) hm->uri.len, hm->uri.p);
    mg_printf(nc, reply_fmt, addr);
    nc->flags |= MG_F_SEND_AND_CLOSE;
}

void HttpServer::onWebsocketHandshakeRequest(struct mg_connection *nc)
{
    APP_LOGI("[HttpServer]", "new websocket connection request (nc: %p)", nc);
}

void HttpServer::onWebsocketHandshakeDone(struct mg_connection *nc)
{
    APP_LOGI("[HttpServer]", "websocket connection opened");
    _clientConnected = true;
}

void HttpServer::onWebsocketFrame(struct mg_connection *nc, struct websocket_message *wm)
{
    // struct mg_str d = {(char *) wm->data, wm->size};
    APP_LOGI("[HttpServer]", "got message: %.*s (nc: %p)", wm->size, wm->data, nc);
    if (_msgInterpreter) {
        _msgInterpreter->interpreteSocketMsg(wm->data, wm->size, nc);
    }
}

void HttpServer::onClose(struct mg_connection *nc)
{
    if (nc->flags & MG_F_IS_WEBSOCKET) {
        APP_LOGI("[HttpServer]", "websocket connection closed (nc: %p)", nc);
        _clientConnected = false;
    }
    else {
        APP_LOGI("[HttpServer]", "http connection closed (nc: %p)", nc);
    }
}
