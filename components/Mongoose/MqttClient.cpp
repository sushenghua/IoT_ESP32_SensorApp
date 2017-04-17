/*
 * MqttClient: Wrap the mongoose lib
 * Copyright (c) 2017 Shenghua Su
 *
 */

#include "MqttClient.h"

#include "esp_system.h"
#include "AppLog.h"
#include "Wifi.h"

/////////////////////////////////////////////////////////////////////////////////////////
// ------ helper functions
/////////////////////////////////////////////////////////////////////////////////////////
void printTopics(const struct mg_mqtt_topic_expression *topics, uint16_t count)
{
    for (uint16_t i = 0; i < count; ++i) {
        printf("[%d] %s\n", i, topics->topic);
    }
}

void printTopics(const char **topics, uint16_t count)
{
    for (uint16_t i = 0; i < count; ++i) {
        printf("[%d] %s\n", i, topics[i]);
    }
}

static char MAC_ADDR[13] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0xFF };

static void initMacADDR()
{
    uint8_t macAddr[6];
    char *target = MAC_ADDR;
    esp_efuse_read_mac(macAddr);
    for (int i = 0; i < 6; ++i) {
        sprintf(target, "%02x", macAddr[i]);
        target += 2;
    }
    MAC_ADDR[12] = '\0';
}

static const char* macAddress()
{
    if (MAC_ADDR[12] == 0xFF)
        initMacADDR();
    return MAC_ADDR;
}

static uint16_t _mqttMsgId = 0;
inline static uint16_t createMsgId() { return _mqttMsgId++; }

/////////////////////////////////////////////////////////////////////////////////////////
// ------ mongoose mqtt event handler
/////////////////////////////////////////////////////////////////////////////////////////
void reconnectCountDelay(TickType_t ticks)
{
    int seconds = ticks / 1000;
    while (seconds > 0) {
        APP_LOGI("[MqttClient]", "reconnect in %d second%s", seconds, seconds > 1 ? "s" : "");
        vTaskDelay(1000/portTICK_RATE_MS);
        --seconds;
    }
}

static void mongoose_mqtt_event_handler(struct mg_connection *nc, int ev, void *p)
{
    struct mg_mqtt_message *msg = (struct mg_mqtt_message *) p;
    MqttClient *mqttClient = static_cast<MqttClient*>(nc->user_data);

    switch (ev) {
        case MG_EV_CONNECT:
            APP_LOGI("[MqttClient]", "try to connect to server: %s (client id: %s, nc: %p)",
                                     mqttClient->serverAddress(), mqttClient->clientId(), nc);
            mg_set_protocol_mqtt(nc);
            mg_send_mqtt_handshake_opt(nc, mqttClient->clientId(), mqttClient->handShakeOpt());
            break;

        case MG_EV_MQTT_CONNACK:
            if (msg->connack_ret_code == MG_EV_MQTT_CONNACK_ACCEPTED) {
                APP_LOGI("[MqttClient]", "connection established");
                mqttClient->setConnected(true);
                if (mqttClient->subscribeImmediatelyOnConnected()) {
                    mqttClient->subscribeTopics();
                }
            }
            else {
                APP_LOGE("[MqttClient]", "connection error: %d", msg->connack_ret_code);
                if (msg->connack_ret_code == MG_EV_MQTT_CONNACK_SERVER_UNAVAILABLE) {
                    reconnectCountDelay(mqttClient->reconnectTicksOnServerUnavailable());
                    mqttClient->makeConnection();
                }
            }
            break;

        case MG_EV_MQTT_SUBACK:
            APP_LOGI("[MqttClient]", "subscription acknowledged");
            mqttClient->setSubscribeSucceeded(true);
            break;

        case MG_EV_MQTT_PUBACK:  // for QoS(1)
            APP_LOGI("[MqttClient]", "message QoS(1) Pub acknowledged (msg_id: %d)", msg->message_id);
            mqttClient->onPubAck(msg->message_id);
            break;

        case MG_EV_MQTT_PUBREC:  // for QoS(2)
            APP_LOGI("[MqttClient]", "message QoS(2) Pub-Receive acknowledged (msg_id: %d)", msg->message_id);
            mg_mqtt_pubrel(nc, msg->message_id);
            break;

        case MG_EV_MQTT_PUBCOMP: // for QoS(2)
            APP_LOGI("[MqttClient]", "message QoS(2) Pub-Complete acknowledged (msg_id: %d)", msg->message_id);
            mqttClient->onPubAck(msg->message_id);
            break;

        case MG_EV_MQTT_PUBLISH:
            printf("Got incoming message (msg_id: %d) %.*s: %.*s\n", msg->message_id,
                   (int) msg->topic.len, msg->topic.p, (int) msg->payload.len, msg->payload.p);
            if (mqttClient->msgInterpreter()) {
                mqttClient->msgInterpreter()->interprete(msg->topic.p, msg->topic.len, msg->payload.p, msg->payload.len);
            }
            break;

        case MG_EV_MQTT_PINGRESP:
            break;

        case MG_EV_CLOSE: {
            APP_LOGI("[MqttClient]", "connection to server %s closed (nc: %p)", mqttClient->serverAddress(), nc);
            mqttClient->setConnected(false);
            mqttClient->setSubscribeSucceeded(false);
            reconnectCountDelay(mqttClient->reconnectTicksOnDisconnection());
            mqttClient->makeConnection();
            break;
        }
    }  
}


/////////////////////////////////////////////////////////////////////////////////////////
// Message pub pool process loop task
/////////////////////////////////////////////////////////////////////////////////////////
static TaskHandle_t _msgTaskHandle = 0;
static void msg_pool_task(void *pvParams)
{
    MessagePubPool *pool = static_cast<MessagePubPool*>(pvParams);
    while (true) {
        pool->processLoop();
        vTaskDelay(pool->loopInterval() / portTICK_PERIOD_MS);
    }
    // vTaskDelete(sntpTaskHandle);
}


/////////////////////////////////////////////////////////////////////////////////////////
// ------ MqttClient class
/////////////////////////////////////////////////////////////////////////////////////////
// FreeRTOS semaphore
static xSemaphoreHandle _pubSemaphore = 0;

// --- default values
#define MQTT_KEEP_ALIVE_DEFAULT_VALUE                           60     // 60 seconds
#define MQTT_RECONNECT_DEFAULT_DELAY_TICKS                      5000   // 1 second
#define MQTT_SERVER_UNAVAILABLE_RECONNECT_DEFAULT_DELAY_TICKS   300000 // 5 minutes
// static const char* MQTT_SERVER_ADDR =                           "192.168.0.99:8883";
static const char* MQTT_SERVER_ADDR =                           "123.57.0.158:8883";

MqttClient::MqttClient()
: _inited(false)
, _connected(false)
, _subscribeImmediatelyOnConnected(true)
, _reconnectTicksOnServerUnavailable(MQTT_SERVER_UNAVAILABLE_RECONNECT_DEFAULT_DELAY_TICKS)
, _reconnectTicksOnDisconnection(MQTT_RECONNECT_DEFAULT_DELAY_TICKS)
, _serverAddress(MQTT_SERVER_ADDR)
, _subscribeSucceeded(false)
, _clientId(macAddress())
, _topicCount(0)
, _unsubTopicCount(0)
, _msgInterpreter(NULL)
{
    _handShakeOpt.flags = 0;
    _handShakeOpt.keep_alive = MQTT_KEEP_ALIVE_DEFAULT_VALUE;
    _handShakeOpt.will_topic = NULL;
    _handShakeOpt.will_message = NULL;
    _handShakeOpt.user_name = NULL;
    _handShakeOpt.password = NULL;
}

void MqttClient::setServerAddress(const char* serverAddress)
{
    _serverAddress = serverAddress;
}

void MqttClient::setOnServerUnavailableReconnectDelay(TickType_t delayTicks)
{
    _reconnectTicksOnServerUnavailable = delayTicks;
}

void MqttClient::setOnDisconnectedReconnectDelay(TickType_t delayTicks)
{
    _reconnectTicksOnDisconnection = delayTicks;
}

void MqttClient::setSubscribeImmediateOnConnected(bool immediate)
{
    _subscribeImmediatelyOnConnected = immediate;
}

void MqttClient::setClientId(const char *clientId)
{
    _clientId = clientId;
}

void MqttClient::setCleanSession(bool cleanSenssion)
{
    if (cleanSenssion) {
        _handShakeOpt.flags |= MG_MQTT_CLEAN_SESSION;
    }
    else {
        _handShakeOpt.flags &= ~MG_MQTT_CLEAN_SESSION;
    }
}

void MqttClient::setUserPassword(const char *user, const char *password)
{
    _handShakeOpt.user_name = user;
    _handShakeOpt.password = password;
}

void MqttClient::setKeepAlive(uint16_t value)
{
    _handShakeOpt.keep_alive = value;
}

void MqttClient::setLastWill(const char* topic, uint8_t qos, const char* msg, bool retain)
{
    _handShakeOpt.will_topic = topic;
    _handShakeOpt.will_message = msg;
    if (retain) {
        _handShakeOpt.flags |= MG_MQTT_WILL_RETAIN;
    }
    else {
        _handShakeOpt.flags &= ~MG_MQTT_WILL_RETAIN;
    }
    MG_MQTT_SET_WILL_QOS(_handShakeOpt.flags, qos);
}

void MqttClient::init()
{
    if (!_inited) {
        APP_LOGI("[MqttClient]", "init client (MqttClient Versino: %s, Free RAM: %d bytes)",
                                   MG_VERSION, esp_get_free_heap_size());
        mg_mgr_init(&_manager, NULL);
        _inited = true;
    }
}

void MqttClient::deinit()
{
    if (_inited) {
        APP_LOGI("[MqttClient]", "deinit client");
        mg_mgr_free(&_manager);
        _inited = false;
    }
}

bool MqttClient::makeConnection()
{
    if (_inited) {
        Wifi::waitConnected(); // block wait
        // set connect opts
        struct mg_connect_opts opts;
        memset(&opts, 0, sizeof(opts));
        opts.user_data = static_cast<void*>(this);
        // create connection
        struct mg_connection *nc;
        nc = mg_connect_opt(&_manager, _serverAddress, mongoose_mqtt_event_handler, opts);
        if (nc == NULL) {
            APP_LOGE("[MqttClient]", "connect to server failed");
            return false;
        }
        return true;
    }
    APP_LOGE("[MqttClient]", "must be inited before connection");
    return false;
}

void MqttClient::start()
{
    _pubSemaphore = xSemaphoreCreateMutex();
    xTaskCreate(&msg_pool_task, "msg_pool_task", 4096, &_msgPubPool, 4, &_msgTaskHandle);
    makeConnection();
}

void MqttClient::clearSubTopics()
{
    // memset(_topics, 0, sizeof(_topics));
    _topicCount = 0;
}

void MqttClient::addSubTopic(const char *topic, uint8_t qos)
{
    if (_topicCount < TOPIC_CACHE_SIZE) {
        _topics[_topicCount].topic = topic;
        _topics[_topicCount].qos = qos;
        ++_topicCount;
    }
}

void MqttClient::subscribeTopics()
{
    if (_connected && !_subscribeSucceeded) {
        APP_LOGI("[MqttClient]", "subscribe to topics:");
        printTopics(_topics, _topicCount);
        mg_mqtt_subscribe(_manager.active_connections, _topics, _topicCount, createMsgId());
    }
}

void MqttClient::addUnsubTopic(const char *topic)
{
    if (_unsubTopicCount < TOPIC_CACHE_SIZE) {
        _unsubTopics[_unsubTopicCount] = topic;
        ++_unsubTopicCount;
    }
}

void MqttClient::unsubscribeTopics()
{
    if (_connected && _subscribeSucceeded) {
        APP_LOGI("[MqttClient]", "unsubscribe to topics:");
        printTopics(_unsubTopics, _unsubTopicCount);
        mg_mqtt_unsubscribe(_manager.active_connections, const_cast<char**>(_unsubTopics), _topicCount, createMsgId());
    }
}

void MqttClient::onPubAck(uint16_t msgId)
{
    _msgPubPool.drainPoolMessage(msgId);
}

#define SEMAPHORE_TAKE_WAIT_TICKS  5000   /// TickType_t

void MqttClient::publish(const char *topic, const void *data, size_t len, uint8_t qos, bool retain, bool dup)
{
    if (!_connected) return;
    if (xSemaphoreTake(_pubSemaphore, SEMAPHORE_TAKE_WAIT_TICKS)) {
        uint16_t msgId = qos > 0 ? createMsgId() : 0;
        int flag = 0;
        if (retain) flag |= MG_MQTT_RETAIN;
        if (dup) flag |= MG_MQTT_DUP;
        MG_MQTT_SET_QOS(flag, qos);
        mg_mqtt_publish(_manager.active_connections, topic, msgId, flag, data, len);
        if (qos > 0) {
            _msgPubPool.addMessage(msgId, topic, data, len, qos, retain);
        }
        xSemaphoreGive(_pubSemaphore);
    }
}

void MqttClient::repubMessage(PoolMessage *message)
{
    if (!_connected) return;
    if (xSemaphoreTake(_pubSemaphore, SEMAPHORE_TAKE_WAIT_TICKS)) {
        int flag = MG_MQTT_DUP;
        if (message->retain) flag |= MG_MQTT_RETAIN;
        MG_MQTT_SET_QOS(flag, message->qos);
        mg_mqtt_publish(_manager.active_connections,
                        message->topic,
                        message->msgId,
                        flag,
                        message->data,
                        message->length);
        message->pubCount++;
        xSemaphoreGive(_pubSemaphore);
    }
}
