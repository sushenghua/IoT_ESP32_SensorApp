/*
 * MqttClient: Wrap the mongoose lib
 * Copyright (c) 2017 Shenghua Su
 *
 */

#include "MqttClient.h"

#include "Config.h"
#include "AppLog.h"
#include "Wifi.h"
#include "SNTP.h"
#include "System.h"


/////////////////////////////////////////////////////////////////////////////////////////
// ------ helper functions
/////////////////////////////////////////////////////////////////////////////////////////
void printTopics(const MqttSubTopic *topics, uint16_t count)
{
#ifdef LOG_MQTT_TOPIC_CHANGE
    for (uint16_t i = 0; i < count; ++i) {
        printf("[%d] %s\n", i, topics[i].topic);
    }
#endif
}

void printTopics(const char **topics, uint16_t count)
{
#ifdef LOG_MQTT_TOPIC_CHANGE
    for (uint16_t i = 0; i < count; ++i) {
        printf("[%d] %s\n", i, topics[i]);
    }
#endif
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
            mqttClient->onConnect(nc);
            break;

        case MG_EV_MQTT_CONNACK:
            mqttClient->onConnAck(msg);
            break;

        case MG_EV_MQTT_PUBACK:  // for QoS(1)
            mqttClient->onPubAck(msg);
            break;

        case MG_EV_MQTT_PUBREC:  // for QoS(2)
            mqttClient->onPubRec(nc, msg);
            break;

        case MG_EV_MQTT_PUBCOMP: // for QoS(2)
            mqttClient->onPubComp(msg);
            break;

        case MG_EV_MQTT_SUBACK:
            mqttClient->onSubAck(msg);
            break;

        case MG_EV_MQTT_UNSUBACK:
            mqttClient->onUnsubAct(msg);
            break;

        case MG_EV_MQTT_PUBLISH:
            mqttClient->onRxPubMessage(msg);
            break;

        case MG_EV_MQTT_PINGRESP:
            mqttClient->onPingResp(msg);
            break;

        case MG_EV_TIMER:
            mqttClient->onTimeout(nc);
            break;

        case MG_EV_CLOSE:
            mqttClient->onClose(nc);
            break;
    }  
}


/////////////////////////////////////////////////////////////////////////////////////////
// alive guard check task
/////////////////////////////////////////////////////////////////////////////////////////
#define ALIVE_GUARD_TASK_PRIORITY  3
static xSemaphoreHandle _closeProcessSemaphore = 0;
static TaskHandle_t _aliveGuardTaskHandle = 0;
static void alive_guard_task(void *pvParams)
{
    MqttClient *client = static_cast<MqttClient*>(pvParams);
    TickType_t delayTicks = client->aliveGuardInterval() * 1000;
    while (true) {
        client->aliveGuardCheck();
        vTaskDelay(delayTicks / portTICK_PERIOD_MS);
    }
}


/////////////////////////////////////////////////////////////////////////////////////////
// Message pub pool process loop task
/////////////////////////////////////////////////////////////////////////////////////////
#define MSG_POOL_TASK_PRIORITY     3
static TaskHandle_t _msgTaskHandle = 0;
static void msg_pool_task(void *pvParams)
{
    MessagePubPool *pool = static_cast<MessagePubPool*>(pvParams);
    while (true) {
        pool->processLoop();
        vTaskDelay(pool->loopInterval() / portTICK_PERIOD_MS);
    }
}


/////////////////////////////////////////////////////////////////////////////////////////
// MQTT over TSL
/////////////////////////////////////////////////////////////////////////////////////////
#if MG_ENABLE_SSL

#include "certs/mqtt_crt.h"
#include "certs/ski.h"
#include "Crypto.h"

// void print_test()
// {
//     printf("===> mqtt crt (%d) : \n%s\n", strlen(mqttCrt), mqttCrt);
//     printf("===> mqtt key (%d) : \n%s\n", strlen(mqttKey), mqttKey);
// }

#define CRT_CACHE_SIZE 2048
#define VSIZE 32
char            _crtCache[CRT_CACHE_SIZE];
size_t          _crtLength = 0;

void _decryptCrt()
{
    uint32_t sih = 1027434326;
    uint32_t sil = 1027424597;

    unsigned char   v1[VSIZE];
    unsigned char   v2[VSIZE];
    unsigned char   _v1[VSIZE];
    unsigned char   _v2[VSIZE];

    size_t len = strlen(ski) / 2;
    memcpy(v1, ski,       len);
    memcpy(v2, ski + len, len);

    memcpy(v1 + len, &sih, 4); v1[len + 4] = '\0';
    memcpy(v2 + len, &sil, 4); v2[len + 4] = '\0';

    decode(v1, strlen((char*)v1), _v1, &len);
    decode(v2, strlen((char*)v2), _v2, &len);

#ifdef DEBUG_CRYPT
    ESP_LOG_BUFFER_HEXDUMP("v1", v1, len + 5, ESP_LOG_INFO);
    ESP_LOG_BUFFER_HEXDUMP("v2", v2, len + 5, ESP_LOG_INFO);
    APP_LOGC("[decryptCrt]", "_v1: %s", (char*)_v1);
    APP_LOGC("[decryptCrt]", "_v2: %s", (char*)_v2);
#endif

    decryptBase64(mqttCrt, strlen(mqttCrt), '\n', _v1, _v2, _crtCache, &_crtLength);

#ifdef DEBUG_CRYPT
    APP_LOGC("[decryptCrt]", "calculated crtCahe len: %d", _crtLength);
    // APP_LOGC("[decryptCrt]", "mqttKey len: %d, crtCahe len: %d", strlen(mqttKey), strlen(_crtCache));
    // APP_LOGC("[decryptCrt]", "decrypted crt: \n%s", _crtCache);
#endif
}

#endif


/////////////////////////////////////////////////////////////////////////////////////////
// ------ MqttClient class
/////////////////////////////////////////////////////////////////////////////////////////
// FreeRTOS semaphore
static xSemaphoreHandle _pubSemaphore = 0;

// --- default values
#define MQTT_KEEP_ALIVE_DEFAULT_VALUE                           60     // 60 seconds
#define MQTT_ALIVE_GUARD_REGULAR_INTERVAL_DEFAULT               (MQTT_KEEP_ALIVE_DEFAULT_VALUE / 2)
#define MQTT_ALIVE_GUARD_OFFLINE_INTERVAL_DEFAULT               5
#define MQTT_RECONNECT_DEFAULT_DELAY_TICKS                      10000   // 1 second
#define MQTT_SERVER_UNAVAILABLE_RECONNECT_DEFAULT_DELAY_TICKS   300000 // 5 minutes
// static const char* MQTT_SERVER_ADDR =                           "192.168.0.99:8883";
static const char* MQTT_SERVER_ADDR =                           "appsgenuine.com:8883";

MqttClient::MqttClient()
: _inited(false)
, _connected(false)
, _subscribeImmediatelyOnConnected(true)
, _reconnectTicksOnServerUnavailable(MQTT_SERVER_UNAVAILABLE_RECONNECT_DEFAULT_DELAY_TICKS)
, _reconnectTicksOnDisconnection(MQTT_RECONNECT_DEFAULT_DELAY_TICKS)
, _aliveGuardInterval(MQTT_ALIVE_GUARD_REGULAR_INTERVAL_DEFAULT)
, _serverAddress(MQTT_SERVER_ADDR)
, _clientId(System::instance()->uid())
{
    // init hand shake option
    _handShakeOpt.flags = 0;
    _handShakeOpt.keep_alive = MQTT_KEEP_ALIVE_DEFAULT_VALUE;
    _handShakeOpt.will_topic = NULL;
    _handShakeOpt.will_message = NULL;
    _handShakeOpt.user_name = NULL;
    _handShakeOpt.password = NULL;

    // init topic cache
    _topicsSubscribed.clear();
    _topicsToSubscribe.clear();
    _topicsToUnsubscribe.clear();

    // message publish pool
    _msgPubPool.setPubDelegate(this);

#if MG_ENABLE_SSL
    // crt
    _decryptCrt();
#endif
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
        // APP_LOGI("[MqttClient]", "init client (Mongoose version: %s, Free RAM: %d bytes)",
        //                           MG_VERSION, esp_get_free_heap_size());
        APP_LOGI("[MqttClient]", "init, free RAM: %d bytes", esp_get_free_heap_size());
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

bool MqttClient::_makeConnection()
{
    if (_inited) {
        Wifi::instance()->waitConnected(); // block wait wifi
        // set connect opts
        struct mg_connect_opts opts;
        memset(&opts, 0, sizeof(opts));
        opts.user_data = static_cast<void*>(this);
#if MG_ENABLE_SSL
        // opts.ssl_cert = mqttCrt;
        // opts.ssl_key = mqttKey;
        opts.ssl_ca_cert = _crtCache;
#endif
        // create connection
        struct mg_connection *nc;
        // APP_LOGI("[MqttClient]", "try to connect to server: %s", _serverAddress);
        APP_LOGI("[MqttClient]", "try to connect to server: ***");
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
    // SNTP::waitSynced();    // block wait time sync
    _makeConnection();
    _pubSemaphore = xSemaphoreCreateMutex();
    _closeProcessSemaphore = xSemaphoreCreateMutex();
    xTaskCreate(&alive_guard_task, "alive_guard_task", 4096, this, ALIVE_GUARD_TASK_PRIORITY, &_aliveGuardTaskHandle);
    xTaskCreate(&msg_pool_task, "msg_pool_task", 4096, &_msgPubPool, MSG_POOL_TASK_PRIORITY, &_msgTaskHandle);
}

const SubTopics & MqttClient::topicsSubscribed()
{
    return _topicsSubscribed;
}

void MqttClient::addSubTopic(const char *topic, uint8_t qos)
{
    _topicsToSubscribe.addSubTopic(topic, qos);
}

void MqttClient::subscribeTopics()
{
    if (_connected && _topicsToSubscribe.count > 0) {
        APP_LOGI("[MqttClient]", "subscribe to topics:");
        printTopics(_topicsToSubscribe.topics, _topicsToSubscribe.count);
        mg_mqtt_subscribe(_manager.active_connections,
                          _topicsToSubscribe.topics,
                          _topicsToSubscribe.count,
                          createMsgId());
    }
}

void MqttClient::addUnsubTopic(const char *topic)
{
    _topicsToUnsubscribe.addUnsubTopic(topic);
}

void MqttClient::unsubscribeTopics()
{
    if (_connected && _topicsToUnsubscribe.count > 0) {
        APP_LOGI("[MqttClient]", "unsubscribe to topics:");
        printTopics(_topicsToUnsubscribe.topics, _topicsToUnsubscribe.count);
        mg_mqtt_unsubscribe(_manager.active_connections,
                            const_cast<char**>(_topicsToUnsubscribe.topics),
                            _topicsToUnsubscribe.count,
                            createMsgId());
    }
}

#define MSG_PUB_SEMAPHORE_TAKE_WAIT_TICKS  5000   /// TickType_t

void MqttClient::publish(const char *topic, const void *data, size_t len, uint8_t qos, bool retain, bool dup)
{
    if (!_connected) return;
    if (xSemaphoreTake(_pubSemaphore, MSG_PUB_SEMAPHORE_TAKE_WAIT_TICKS)) {
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
#ifdef LOG_MQTT_TX
        APP_LOGC("[MqttClient]", "pub message (msg_id: %d, qos: %d) %s: %.*s", msgId, qos, topic,
                 len, (const char*)data);
#endif
    }
}

bool MqttClient::hasUnackPub()
{
    APP_LOGC("[MqttClient]", "pool message count: %d", _msgPubPool.poolMessageCount());
    return _msgPubPool.poolMessageCount() > 0;
}

void MqttClient::repubMessage(PoolMessage *message)
{
    if (!_connected) return;
    if (xSemaphoreTake(_pubSemaphore, MSG_PUB_SEMAPHORE_TAKE_WAIT_TICKS)) {
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
#ifdef LOG_MQTT_RETX
        APP_LOGE("[MqttClient]", "repub message (msg_id: %d) %s: %.*s", message->msgId,
                 message->topic, message->length, (const char*)message->data);
#endif
    }
}

void MqttClient::onConnect(struct mg_connection *nc)
{
    // APP_LOGI("[MqttClient]", "... connecting to server: %s (client id: %s, nc: %p)",
    //                          _serverAddress, _clientId, nc);
    APP_LOGI("[MqttClient]", "... connecting to server: *** (client id: uid, nc: %p)", nc);
    mg_set_timer(nc, 0);        // Clear connect timer
    mg_set_protocol_mqtt(nc);
    mg_send_mqtt_handshake_opt(nc, _clientId, _handShakeOpt);
}

void MqttClient::onConnAck(struct mg_mqtt_message *msg)
{
    if (msg->connack_ret_code == MG_EV_MQTT_CONNACK_ACCEPTED) {
        APP_LOGI("[MqttClient]", "connection established");
        _connected = true;
        if (_subscribeImmediatelyOnConnected) {
            subscribeTopics();
        }
        _recentActiveTime = time(NULL);
    }
    else {
        APP_LOGE("[MqttClient]", "connection error: %d", msg->connack_ret_code);
        if (msg->connack_ret_code == MG_EV_MQTT_CONNACK_SERVER_UNAVAILABLE) {
            reconnectCountDelay(_reconnectTicksOnServerUnavailable);
            _makeConnection();
        }
    }
}

void MqttClient::onPubAck(struct mg_mqtt_message *msg)
{
    // APP_LOGI("[MqttClient]", "message QoS(1) Pub acknowledged (msg_id: %d)", msg->message_id);
    _msgPubPool.drainPoolMessage(msg->message_id);
    _recentActiveTime = time(NULL);
}

void MqttClient::onPubRec(struct mg_connection *nc, struct mg_mqtt_message *msg)
{
    // APP_LOGI("[MqttClient]", "message QoS(2) Pub-Receive acknowledged (msg_id: %d)", msg->message_id);
    mg_mqtt_pubrel(nc, msg->message_id);
    _recentActiveTime = time(NULL);
}

void MqttClient::onPubComp(struct mg_mqtt_message *msg)
{
    // APP_LOGI("[MqttClient]", "message QoS(2) Pub-Complete acknowledged (msg_id: %d)", msg->message_id);
    onPubAck(msg);
    _recentActiveTime = time(NULL);
}

void MqttClient::onSubAck(struct mg_mqtt_message *msg)
{
    APP_LOGI("[MqttClient]", "subscription acknowledged");
    _topicsSubscribed.addSubTopics(_topicsToSubscribe);
    _topicsToSubscribe.clear();
    APP_LOGI("[MqttClient]", "all subscribed topics:");
    printTopics(_topicsSubscribed.topics, _topicsSubscribed.count);
    _recentActiveTime = time(NULL);
}

void MqttClient::onUnsubAct(struct mg_mqtt_message *msg)
{
    APP_LOGI("[MqttClient]", "unsubscription acknowledged");
    _topicsSubscribed.removeUnsubTopics(_topicsToUnsubscribe);
    _topicsToUnsubscribe.clear();
    _recentActiveTime = time(NULL);
}

void MqttClient::onRxPubMessage(struct mg_mqtt_message *msg)
{
#ifdef LOG_MQTT_RX
    printf("\n");
    APP_LOGC("[MqttClient]", "got incoming message (msg_id: %d) %.*s: %.*s", msg->message_id,
             (int) msg->topic.len, msg->topic.p, (int) msg->payload.len, msg->payload.p);
#endif
    if (_msgInterpreter) {
        _msgInterpreter->interpreteMqttMsg(msg->topic.p, msg->topic.len, msg->payload.p, msg->payload.len);
    }
    _recentActiveTime = time(NULL);
}

void MqttClient::onPingResp(struct mg_mqtt_message *msg)
{
#ifdef LOG_MQTT_PING_PONG
    APP_LOGI("[MqttClient]", "got ping response");
#endif
    _recentActiveTime = time(NULL);
}

void MqttClient::onTimeout(struct mg_connection *nc)
{
    APP_LOGI("[MqttClient]", "connection timeout");
    nc->flags |= MG_F_CLOSE_IMMEDIATELY;
}

#define CLOSE_SEMAPHORE_TAKE_WAIT_TICKS    1000

void MqttClient::onClose(struct mg_connection *nc)
{
    // APP_LOGI("[MqttClient]", "connection to server %s closed (nc: %p) (alive nc: %p)", _serverAddress, nc, _manager.active_connections);
    APP_LOGI("[MqttClient]", "connection to server *** closed (nc: %p) (alive nc: %p)", nc, _manager.active_connections);

    if (_manager.active_connections && _manager.active_connections != nc) return;

    if (xSemaphoreTake(_closeProcessSemaphore, CLOSE_SEMAPHORE_TAKE_WAIT_TICKS)) {
        _closeProcess();
        xSemaphoreGive(_closeProcessSemaphore);
    }
}

void MqttClient::_closeProcess()
{
    _connected = false;
    //vTaskDelete(_aliveGuardTaskHandle);
    reconnectCountDelay(_reconnectTicksOnDisconnection);
    _makeConnection();
}

void MqttClient::aliveGuardCheck()
{
    // APP_LOGI("[MqttClient]", "alive guard check");
    if (_connected) {
        time_t timeNow = time(NULL);
        if (timeNow - _recentActiveTime > _handShakeOpt.keep_alive) {
            // server does not response, connection dead
            APP_LOGI("[MqttClient]", "client has received no response for a while, connection considered to be lost");
            if (xSemaphoreTake(_closeProcessSemaphore, CLOSE_SEMAPHORE_TAKE_WAIT_TICKS)) {
                if (_connected) {
                    // mg_mqtt_disconnect(_manager.active_connections);
                    mg_set_timer(_manager.active_connections, mg_time() + 1);
                    // _manager.active_connections->flags |= MG_F_SEND_AND_CLOSE;
                }
                xSemaphoreGive(_closeProcessSemaphore);
            }
        }
        else if (_connected && timeNow - _recentActiveTime > _aliveGuardInterval) {
#ifdef LOG_MQTT_PING_PONG
            APP_LOGI("[MqttClient]", "no activity recently, ping to server");
#endif
            mg_mqtt_ping(_manager.active_connections);
        }
    }
}
