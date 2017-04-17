/*
 * MessagePubPool: Wrap the mongoose lib
 * Copyright (c) 2017 Shenghua Su
 *
 */

#include "MessagePubPool.h"
#include "AppLog.h"

PoolMessage * MessagePubPool::_message(uint16_t msgId)
{
    std::map<uint16_t, int>::iterator it = _messageMap.find(msgId);
    if (it != _messageMap.end()) {
        return &_messageBuf[_messageMap[msgId]];
    }
    return NULL;
}

void MessagePubPool::_repubPoolMessage(uint16_t msgId)
{
    PoolMessage *message = _message(msgId);
    if (message && _delegate) {
        _delegate->repubMessage(message);
    }
}

void MessagePubPool::_initFreeSlotStack()
{
    for (int i = 0; i < PUB_POOL_CAPACITY; ++i) {
        _freeSlotStack[i] = i;
    }
    _stackTopIndex = 0;
}

inline bool MessagePubPool::_hasFreeSlot()
{
    return _stackTopIndex < PUB_POOL_CAPACITY - 1;
}

inline int MessagePubPool::_popFreeSlot()
{
    if (_stackTopIndex < PUB_POOL_CAPACITY - 1) {
        return _freeSlotStack[_stackTopIndex++];
    }
    return -1;
}

inline bool MessagePubPool::_pushFreeSlot(int slot)
{
    if (_stackTopIndex > 0) {
        _freeSlotStack[--_stackTopIndex] = slot;
        return true;
    }
    return false;
}

MessagePubPool::MessagePubPool(TickType_t loopInterval)
: _loopInterval(loopInterval)
, _delegate(NULL)
{
	_initFreeSlotStack();
}

void MessagePubPool::setPubDelegate(MessagePubDelegate *delegate)
{
    _delegate = delegate;
}

void MessagePubPool::processLoop()
{
	APP_LOGI("[MessagePubPool]", "porcess loop");
}

bool MessagePubPool::addMessage(uint16_t    msgId,
                                const char* topic,
                                const void* data,
                                size_t      len,
                                uint8_t     qos,
                                bool        retain)
{
    if (_messageMap.find(msgId) != _messageMap.end()) {
        return false;
    }

    if (_hasFreeSlot()) {
        int slot = _popFreeSlot();
        // add to map
        _messageMap[msgId] = slot;
        // save message
        _messageBuf[slot].msgId = msgId;
        _messageBuf[slot].topic = topic;
        _messageBuf[slot].data = data;
        _messageBuf[slot].length = len;
        _messageBuf[slot].qos = qos;
        _messageBuf[slot].retain = retain;
        _messageBuf[slot].pubCount = 1;
        _messageBuf[slot].life = 0;
        return true;
    }

    return false;
}

void MessagePubPool::drainPoolMessage(uint16_t msgId)
{
    std::map<uint16_t, int>::iterator it = _messageMap.find(msgId);
    if (it != _messageMap.end()) {
        int slot = _messageMap[msgId];
        _pushFreeSlot(slot);
        _messageMap.erase(it);
    }
}

void MessagePubPool::cleanPool()
{
    _messageMap.clear();
    _initFreeSlotStack();
}
