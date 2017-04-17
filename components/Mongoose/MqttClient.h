/*
 * MqttClient: Wrap the mongoose lib
 * Copyright (c) 2017 Shenghua Su
 *
 */

#ifndef _MQTT_CLIENT_H
#define _MQTT_CLIENT_H

#include "mongoose.h"

#define MONGOOSE_DEFAULT_POLL_SLEEP     1000   // 1 second

class MqttClient
{
public:
    // constructor
    MqttClient();

    // loop poll
    void poll(int sleepMilli = MONGOOSE_DEFAULT_POLL_SLEEP) {
        mg_mgr_poll(&_manager, sleepMilli);
    }

    // config connection
    void setServerAddress(const char* serverAddress);
    void setOnServerUnavailableReconnectDelay(TickType_t delayTicks);
    void setOnDisconnectedReconnectDelay(TickType_t delayTicks);
    void setSubscribeImmediateOnConnected(bool immediate = true);

    // config mqtt client connection protocol
    void setClientId(const char *clientId);
    void setCleanSession(bool cleanSenssion = true);
    void setUserPassword(const char *user, const char *password);
    void setKeepAlive(uint16_t value);
    void setLastWill(const char* topic, uint8_t qos, const char* msg, bool retain);

    // init and deinit
    void init();
    void deinit();

    // communication
    void start();
    void addSubTopic(const char *topic, uint8_t qos = 0);
    void clearTopics();
    void subscribeTopics();
    void publish();

public:
    // for event handler
    const char * serverAddress() { return _serverAddress; }
    const char* clientId() { return _clientId; }
    void setConnected(bool connected) { _connected = connected; }
    void setSubscribeSucceeded(bool succeeded) { _subscribeSucceeded = succeeded; }
    mg_send_mqtt_handshake_opts& handShakeOpt() { return _handShakeOpt; }
    bool subscribeImmediatelyOnConnected() { return _subscribeImmediatelyOnConnected; }
    TickType_t reconnectTicksOnServerUnavailable() { return _reconnectTicksOnServerUnavailable; }    
    TickType_t reconnectTicksOnDisconnection() { return _reconnectTicksOnDisconnection; }
    bool connectServer();

protected:
    // init and connection
    bool                                _inited;
    bool                                _connected;
    bool                                _subscribeImmediatelyOnConnected;
    TickType_t                          _reconnectTicksOnServerUnavailable;
    TickType_t                          _reconnectTicksOnDisconnection;
    const char                         *_serverAddress;
    // mqtt connection protocol
    bool                                _subscribeSucceeded;
    const char                         *_clientId;
    struct mg_mgr                       _manager;
    struct mg_send_mqtt_handshake_opts  _handShakeOpt;

    // mqtt topics
    #define TOPIC_CACHE_SIZE            10    
    uint16_t                            _topicCount;
    struct mg_mqtt_topic_expression     _topics[TOPIC_CACHE_SIZE];
};

#endif // _MQTT_CLIENT_H
