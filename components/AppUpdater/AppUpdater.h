/*
 * AppUpdater app firmware updater
 * Copyright (c) 2016 Shenghua Su
 *
 */

#ifndef _APP_UPDATER_H
#define _APP_UPDATER_H

#include "MqttClientDelegate.h"

#include "esp_ota_ops.h"
#include "esp_partition.h"


class AppUpdater
{
public:
	// type
    enum UpdateState {
        UPDATE_STATE_IDLE,
        UPDATE_STATE_WAIT_VERSION_INFO,
        UPDATE_STATE_WAIT_DATA
    };
    typedef uint16_t VersionNoType;
public:
    AppUpdater();
    void init();
    void update();
    void updateLoop(const char* data, size_t dataLen);

protected:
    void _sendUpdateCmd();
    bool _beforeUpdateCheck();
    void _prepareUpdate();	
    void _onRxDataComplete();


protected:
    UpdateState              _state;
    const VersionNoType      _currentVersion;
    size_t                   _newVersionSize;
    size_t                   _writeCount;
    esp_ota_handle_t         _updateHandle;
    const esp_partition_t *  _updatePartition;
    MqttClientDelegate      *_delegate;
};

#endif // _APP_UPDATER_H
