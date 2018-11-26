/*
 * MqttClient: Wrap the mongoose lib
 * Copyright (c) 2017 Shenghua Su
 *
 */

#ifndef _MQTT_CLIENT_H
#define _MQTT_CLIENT_H

#include "ProtocolMessageInterpreter.h"
#include "MqttClientDelegate.h"
#include "MessagePubPool.h"

#include "mongoose.h"


/////////////////////////////////////////////////////////////////////////////////////////
// ------ SubTopics, UnsubTopics
/////////////////////////////////////////////////////////////////////////////////////////
#define TOPIC_CACHE_CAPACITY            10
typedef struct mg_mqtt_topic_expression MqttSubTopic;

struct UnsubTopics
{
    void addUnsubTopic(const char *topic) {
        if (count < TOPIC_CACHE_CAPACITY) {
            topics[count++] = topic;
        }
    }

    void clear() {
        for (uint16_t i = 0; i < TOPIC_CACHE_CAPACITY; ++i) {
            topics[i] = NULL;
        }
        count = 0;
    }

    uint16_t        count;
    const char     *topics[TOPIC_CACHE_CAPACITY];
};

struct SubTopics
{
    void addSubTopic(const char *topic, uint8_t qos) {
        if (findTopic(topic) == -1 && count < TOPIC_CACHE_CAPACITY) {
            topics[count].topic = topic;
            topics[count].qos = qos;
            ++count;
        }
    }

    void addSubTopics(SubTopics &subTopics) {
        if (subTopics.count + count > TOPIC_CACHE_CAPACITY)
            return;
        for (uint16_t i = 0; i < subTopics.count; ++i) {
            if (findTopic(subTopics.topics[i].topic) == -1) {
                topics[count] = subTopics.topics[i];
                ++count;
            }
        }
    }

    void removeUnsubTopics(UnsubTopics &unsubTopics) {
        bool needCleanupNull = false;
        for (uint16_t i = 0; i < unsubTopics.count; ++i) {
            int index = findTopic(unsubTopics.topics[i]);
            if (index != -1) {
                topics[index].topic = NULL;
                needCleanupNull = true;
            }
        }
        if (needCleanupNull) cleanupNull();
    }

    void cleanupNull() {
        uint16_t searchCount = count;
        uint16_t j = 0;
        for (uint16_t i = 0; i < searchCount; ++i) {
            if (topics[i].topic == NULL) {
                --count;
                if (j == 0) j = i;
                while (++j < searchCount) {
                    if (topics[j].topic != NULL) {
                        topics[i].topic = topics[j].topic;
                        topics[i].qos = topics[j].qos;
                        topics[j].topic = NULL;
                        break;
                    }
                }
            }
        }
    }

    int findTopic(const char *topic) {
        for (uint16_t i = 0; i < count; ++i) {
            if (topics[i].topic != NULL && strcmp(topics[i].topic, topic) == 0)
                return i;
        }
        return -1;
    }

    void clear() {
        for (uint16_t i = 0; i < TOPIC_CACHE_CAPACITY; ++i) {
            topics[i].topic = NULL;
            topics[i].qos = 0;
        }
        count = 0;
    }

    uint16_t        count;
    MqttSubTopic    topics[TOPIC_CACHE_CAPACITY];
};


/////////////////////////////////////////////////////////////////////////////////////////
// ------ MqttClient class
/////////////////////////////////////////////////////////////////////////////////////////
#define MONGOOSE_MQTT_DEFAULT_POLL_SLEEP     0

class MqttClient : public MessagePubDelegate, public MqttClientDelegate
{
public:
    // constructor
    MqttClient();

    // MessagePubDelegate
    virtual void repubMessage(PoolMessage *message);

    // loop poll
    void poll(int sleepMilli = MONGOOSE_MQTT_DEFAULT_POLL_SLEEP) {
        mg_mgr_poll(&_manager, sleepMilli);
    }

    // config connection
    void setServerAddress(const char* serverAddress);
    void setAliveGuardInterval(time_t interval) { _aliveGuardInterval = interval; }
    time_t aliveGuardInterval() { return _aliveGuardInterval; }
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

    // connection
    void start();
    bool connected() { return _connected; }

    const SubTopics & topicsSubscribed();

    // MqttClientDelegate interface
    virtual void addSubTopic(const char *topic, uint8_t qos = 0);
    virtual void subscribeTopics();
    virtual bool hasTopicsToSubscribe() { return _topicsToSubscribe.count > 0; }

    virtual void addUnsubTopic(const char *topic);
    virtual void unsubscribeTopics();
    virtual bool hasTopicsToUnsubscribe() { return _topicsToUnsubscribe.count > 0; }

    virtual void publish(const char *topic,
                         const void *data,
                         size_t      len,
                         uint8_t     qos,
                         bool        retain = false,
                         bool        dup = false);
    virtual bool hasUnackPub();

    // for alive guard check task
    void aliveGuardCheck();

public:
    // for event handler
    void onConnect(struct mg_connection *nc);
    void onConnAck(struct mg_mqtt_message *msg);
    void onPubAck(struct mg_mqtt_message *msg);
    void onPubRec(struct mg_connection *nc, struct mg_mqtt_message *msg);
    void onPubComp(struct mg_mqtt_message *msg);
    void onSubAck(struct mg_mqtt_message *msg);
    void onUnsubAct(struct mg_mqtt_message *msg);
    void onRxPubMessage(struct mg_mqtt_message *msg);
    void onPingResp(struct mg_mqtt_message *msg);
    void onTimeout(struct mg_connection *nc);
    void onClose(struct mg_connection *nc);

protected:
    bool _makeConnection();
    void _closeProcess();

protected:
    // init and connection
    bool                                _inited;
    bool                                _connected;
    bool                                _subscribeImmediatelyOnConnected;
    TickType_t                          _reconnectTicksOnServerUnavailable;
    TickType_t                          _reconnectTicksOnDisconnection;
    time_t                              _aliveGuardInterval;
    time_t                              _recentActiveTime;
    const char                         *_serverAddress;

    // mqtt connection protocol
    const char                         *_clientId;
    struct mg_mgr                       _manager;
    struct mg_send_mqtt_handshake_opts  _handShakeOpt;

    // mqtt topics
    bool                                _subscribeInProgress;
    bool                                _unsubscribeInProgress;
    SubTopics                           _topicsSubscribed;
    SubTopics                           _topicsToSubscribe;
    UnsubTopics                         _topicsToUnsubscribe;

    // message publish pool
    MessagePubPool                      _msgPubPool;
};

#endif // _MQTT_CLIENT_H
