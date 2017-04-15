/*
 * Mongoose: Wrap the mongoose lib
 * Copyright (c) 2017 Shenghua Su
 *
 */

#include "Mongoose.h"

#include "esp_system.h"
#include "esp_log.h"
#include "Wifi.h"

#define MG_LISTEN_ADDR "80"

Mongoose::Mongoose()
: _inited(false)
, _mode(UNDEFINED)
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
            ESP_LOGI("[Mongoose]", "connection %p from %s", nc, addr);
            break;
        }
        // HTTP
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
        // cose
        case MG_EV_CLOSE: {
            ESP_LOGI("[Mongoose]", "connection %p closed", nc);
            break;
        }
    }  
}

#define MQTT_SERVER_ADDR "192.168.0.99:8883"
// static const char *s_address = "192.168.0.99:1883";
static const char *s_user_name = NULL;
static const char *s_password = NULL;
static const char *s_topic = "/mqtttest";
static struct mg_mqtt_topic_expression s_topic_expr = {NULL, 0};

static uint16_t msgId = 0;

static void mongoose_mqtt_event_handler(struct mg_connection *nc, int ev, void *p)
{
    struct mg_mqtt_message *msg = (struct mg_mqtt_message *) p;

    switch (ev) {
        // connected
        case MG_EV_CONNECT:
            ESP_LOGI("[Mongoose]", "MQTT connect event: %p", nc);
            struct mg_send_mqtt_handshake_opts opts;
            memset(&opts, 0, sizeof(opts));
            opts.user_name = s_user_name;
            opts.password = s_password;
            mg_set_protocol_mqtt(nc);
            mg_send_mqtt_handshake_opt(nc, "esp32", opts);
            break;

        // MQTT
        case MG_EV_MQTT_CONNACK:
            if (msg->connack_ret_code == MG_EV_MQTT_CONNACK_ACCEPTED) {
                s_topic_expr.topic = s_topic;
                ESP_LOGI("[Mongoose]", "MQTT subscribe to: %s", s_topic);
                mg_mqtt_subscribe(nc, &s_topic_expr, 1, 42);
            }
            else {
                ESP_LOGE("[Mongoose]", "MQTT connection error: %d", msg->connack_ret_code);
            }
            break;

        case MG_EV_MQTT_SUBACK:
            ESP_LOGI("[Mongoose]", "MQTT subscription acknowledged, forwarding to '/test'");
            break;

        case MG_EV_MQTT_PUBACK:  // for QoS(1)
            ESP_LOGI("[Mongoose]", "MQTT message QoS(1) Pub acknowledged (msg_id: %d)", msg->message_id);
            break;

        case MG_EV_MQTT_PUBREC:  // for QoS(2)
            ESP_LOGI("[Mongoose]", "MQTT message QoS(2) Pub-Receive acknowledged (msg_id: %d)", msg->message_id);
            mg_mqtt_pubrel(nc, msgId);
            break;

        case MG_EV_MQTT_PUBCOMP: // for QoS(2)
            ESP_LOGI("[Mongoose]", "MQTT message QoS(2) Pub-Complete acknowledged (msg_id: %d)", msg->message_id);
            // clear cache
            break;

        case MG_EV_MQTT_PUBLISH:
            printf("Got incoming message (msg_id: %d) %.*s: %.*s\n", msg->message_id,
                   (int) msg->topic.len, msg->topic.p, (int) msg->payload.len, msg->payload.p);
            printf("Forwarding to /test\n");
            mg_mqtt_publish(nc, "/test", ++msgId, MG_MQTT_QOS(1), msg->payload.p, msg->payload.len);
            //mg_mqtt_puback(nc, msgId++);
            break;

        // cose
        case MG_EV_CLOSE: {
            ESP_LOGI("[Mongoose]", "MQTT connection closed: %p", nc);
            break;
        }
    }  
}

// static void mongoose_event_handler(struct mg_connection *nc, int ev, void *p)
// {
//     switch (_mode) {
//         case MQTT_MODE:
//             mongoose_mqtt_event_handler(nc, ev, p);
//             break;

//         case HTTP_MODE:
//             mongoose_http_event_handler(nc, ev, p);
//             break;

//         case WEBSOCKET_MODE:
//     }
// }

void Mongoose::init(MongooseMode mode)
{
    if (!_inited) {
        struct mg_connection *nc;
    
        ESP_LOGI("[Mongoose]", "version: %s", MG_VERSION);
        ESP_LOGI("[Mongoose]", "Free RAM: %d bytes", esp_get_free_heap_size());
    
        _mode = mode;
        mg_mgr_init(&_manager, this);
    
        if (mode == HTTP_MODE) {
            ESP_LOGI("[Mongoose]", "HTTP init server");
            nc = mg_bind(&_manager, MG_LISTEN_ADDR, mongoose_http_event_handler);
            if (nc == NULL) {
                ESP_LOGE("[Mongoose]", "HTTP init server failed");
                return;
            }
            mg_set_protocol_http_websocket(nc);
        }
        else if (mode == MQTT_MODE) {
            Wifi::waitConnected(); // block wait
            ESP_LOGI("[Mongoose]", "MQTT init client");
            nc = mg_connect(&_manager, MQTT_SERVER_ADDR, mongoose_mqtt_event_handler);
            if (nc == NULL) {
                ESP_LOGE("[Mongoose]", "MQTT init client failed");
                return;
            }
        }

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

