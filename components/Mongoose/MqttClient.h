/*
 * MqttClient: Wrap the mongoose lib
 * Copyright (c) 2017 Shenghua Su
 *
 */

#ifndef _MQTT_CLIENT_H
#define _MQTT_CLIENT_H

#include "mongoose.h"
#include "MqttMessageInterpreter.h"
#include "MessagePubPool.h"
#include <string.h>


/////////////////////////////////////////////////////////////////////////////////////////
// ------ SubTopicCache, UnsubTopicCache
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
            if (strcmp(topics[i].topic, topic) == 0)
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
#define MONGOOSE_DEFAULT_POLL_SLEEP     1000   // 1 second

class MqttClient : public MessagePubDelegate
{
public:
    // constructor
    MqttClient();

    // MessagePubDelegate
    virtual void repubMessage(PoolMessage *message);

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
    bool makeConnection();

    const SubTopics & topicsSubscribed();
    void addSubTopic(const char *topic, uint8_t qos = 0);
    void subscribeTopics();

    void addUnsubTopic(const char *topic);
    void unsubscribeTopics();

    void publish(const char *topic,
                 const void *data,
                 size_t      len,
                 uint8_t     qos,
                 bool        retain = false,
                 bool        dup = false);

    // message interpreter
    void setMessageInterpreter(MqttMessageInterpreter *interpreter) { _msgInterpreter = interpreter; }

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
    void onClose(struct mg_connection *nc);

protected:
    // init and connection
    bool                                _inited;
    bool                                _connected;
    bool                                _subscribeImmediatelyOnConnected;
    TickType_t                          _reconnectTicksOnServerUnavailable;
    TickType_t                          _reconnectTicksOnDisconnection;
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

    // message interpreter
    MqttMessageInterpreter             *_msgInterpreter;

    // message publish pool
    MessagePubPool                      _msgPubPool;
};

#endif // _MQTT_CLIENT_H
