/*
 * Mongoose: Wrap the mongoose lib
 * Copyright (c) 2017 Shenghua Su
 *
 */

#include "Mongoose.h"

#include "esp_system.h"
#include "esp_log.h"

#define MG_LISTEN_ADDR "80"

Mongoose::Mongoose()
: _inited(false)
{}

static void mongoose_event_handler(struct mg_connection *nc, int ev, void *p)
{
  static const char *reply_fmt =
        "HTTP/1.0 200 OK\r\n"
        "Connection: close\r\n"
        "Content-Type: text/plain\r\n"
        "\r\n"
        "Sensor %s\n";

    switch (ev) {
        case MG_EV_ACCEPT: {
            char addr[32];
            mg_sock_addr_to_str(&nc->sa, addr, sizeof(addr),
                                MG_SOCK_STRINGIFY_IP | MG_SOCK_STRINGIFY_PORT);
            ESP_LOGI("[Mongoose]", "Connection %p from %s", nc, addr);
            break;
        }
        case MG_EV_HTTP_REQUEST: {
            char addr[32];
            struct http_message *hm = (struct http_message *) p;
            mg_sock_addr_to_str(&nc->sa, addr, sizeof(addr),
                                MG_SOCK_STRINGIFY_IP | MG_SOCK_STRINGIFY_PORT);
            ESP_LOGI("[Mongoose]", "HTTP request from %s: %.*s %.*s", addr, (int) hm->method.len,
                                   hm->method.p, (int) hm->uri.len, hm->uri.p);
            // mg_printf(nc, reply_fmt, addr);
            mg_printf(nc, reply_fmt, addr);
            nc->flags |= MG_F_SEND_AND_CLOSE;
            break;
        }
        case MG_EV_CLOSE: {
            ESP_LOGI("[Mongoose]", "Connection %p closed", nc);
            break;
        }
    }  
}

void Mongoose::init()
{
    if (!_inited) {
        struct mg_connection *nc;
    
        ESP_LOGI("[Mongoose]", "version: %s", MG_VERSION);
        ESP_LOGI("[Mongoose]", "Free RAM: %d bytes", esp_get_free_heap_size());
    
        mg_mgr_init(&_manager, this);
    
        nc = mg_bind(&_manager, MG_LISTEN_ADDR, mongoose_event_handler);
        if (nc == NULL) {
            ESP_LOGE("[Mongoose]", "setting up listener failed");
            return;
        }
        mg_set_protocol_http_websocket(nc);

        _inited = true;
    }
}

void Mongoose::deinit()
{
    if (_inited) {
        mg_mgr_free(&_manager);
        _inited = false;
    }
}

