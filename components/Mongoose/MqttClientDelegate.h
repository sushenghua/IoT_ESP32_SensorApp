/*
 * MqttClientDelegate: interface for communication with MQTT Client object
 * Copyright (c) 2017 Shenghua Su
 *
 */

#ifndef _MQTT_CLIENT_DELEGATE_H
#define _MQTT_CLIENT_DELEGATE_H

#include <stdint.h>

class MqttClientDelegate
{
public:
    // virtual functions
    virtual void setMessageInterpreter(MqttMessageInterpreter *interpreter) = 0;
    virtual void addSubTopic(const char *topic, uint8_t qos = 0) = 0;
    virtual void subscribeTopics() = 0;

    virtual void addUnsubTopic(const char *topic) = 0;
    virtual void unsubscribeTopics() = 0;

    virtual void publish(const char *topic,
                         const void *data,
                         size_t      len,
                         uint8_t     qos,
                         bool        retain = false,
                         bool        dup = false) = 0;
};

#endif // _MQTT_CLIENT_H
