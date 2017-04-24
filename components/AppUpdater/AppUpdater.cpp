/*
 * AppUpdater app firmware updater
 * Copyright (c) 2016 Shenghua Su
 *
 */

#include "AppUpdater.h"

#include <string.h>

#include "Wifi.h"
#include "System.h"
#include "AppLog.h"

#define TAG      "[AppUpdater]"
#define VERSION  0x0100
#define APP_UPDATE_TOPIC             "api/update"

static char _updateDrxDataTopic[32];   // device rx
static char _updateDtxDataTopic[32];   // device tx


AppUpdater::AppUpdater()
: _state(UPDATE_STATE_IDLE)
, _currentVersion(VERSION)
, _rxTopicLen(0)
, _newVersionSize(0)
, _writeCount(0)
, _updateHandle(0)
, _delegate(NULL)
{}

void AppUpdater::init()
{
    size_t index = 0;
    size_t slen = 0;

    slen = strlen(APP_UPDATE_TOPIC);
    memcpy(_updateDrxDataTopic, APP_UPDATE_TOPIC, slen);
    index += slen;

    _updateDrxDataTopic[index] = '/';
    index += 1;

    slen = strlen(System::macAddress());
    memcpy(_updateDrxDataTopic + index, System::macAddress(), slen);
    index += slen;

    memcpy(_updateDtxDataTopic, _updateDrxDataTopic, index);

    memcpy(_updateDrxDataTopic + index, "/drx", 4);
    memcpy(_updateDtxDataTopic + index, "/dtx", 4);
    index += 4;

    _rxTopicLen = index;

    _updateDrxDataTopic[index] = '\0';
    _updateDtxDataTopic[index] = '\0';

    _state = UPDATE_STATE_IDLE;
}

size_t AppUpdater::updateRxTopicLen()
{
	return _rxTopicLen;
}

const char* AppUpdater::updateRxTopic()
{
    return _updateDrxDataTopic;
}

void AppUpdater::_sendUpdateCmd()
{
    if (_delegate) {
        _delegate->addSubTopic(_updateDrxDataTopic);
        _delegate->subscribeTopics();
        _delegate->publish(APP_UPDATE_TOPIC, System::macAddress(), strlen(System::macAddress()), 1);
    }
}

bool AppUpdater::_beforeUpdateCheck()
{
    APP_LOGI(TAG, "checking partition ...");

    const esp_partition_t *configured = esp_ota_get_boot_partition();
    const esp_partition_t *running = esp_ota_get_running_partition();

    if (configured != running) {
        APP_LOGE(TAG, "running partition is different from boot partition, cannot execute update operation");
        return false;
    }
    else {
        APP_LOGI(TAG, "running partition type %d subtype %d (offset 0x%08x)",
                      configured->type, configured->subtype, configured->address);
        return true;
    }
}

void AppUpdater::_onRxDataComplete()
{
    if (_delegate) {
        _delegate->addUnsubTopic(_updateDrxDataTopic);
        _delegate->unsubscribeTopics();
    }

    APP_LOGI(TAG, "total Write binary data length : %d", _writeCount);

    bool succeeded = true;

    if (esp_ota_end(_updateHandle) != ESP_OK) {
        APP_LOGE(TAG, "esp_ota_end failed!");
        // task_fatal_error();
        succeeded = false;
    }

    esp_err_t err = esp_ota_set_boot_partition(_updatePartition);
    if (err != ESP_OK) {
        APP_LOGE(TAG, "esp_ota_set_boot_partition failed! err=0x%x", err);
        // task_fatal_error();
        succeeded = false;
    }

    _state = UPDATE_STATE_IDLE;

    if (succeeded) {
        APP_LOGE(TAG, "update completed, prepare to restart system");
        System::restart();
    }
}

void AppUpdater::_prepareUpdate()
{
    _updatePartition = esp_ota_get_next_update_partition(NULL);
    APP_LOGI(TAG, "Writing to partition subtype %d at offset 0x%x",
                  _updatePartition->subtype, _updatePartition->address);
    if (_updatePartition == NULL) {
        ESP_LOGE(TAG, "cannot get the update partition");
    }

    esp_err_t err = esp_ota_begin(_updatePartition, OTA_SIZE_UNKNOWN, &_updateHandle);
    if (err == ESP_OK) {
        _writeCount = 0;
        APP_LOGI(TAG, "esp_ota_begin succeeded");
    } else {
        ESP_LOGE(TAG, "esp_ota_begin failed, error=%d", err);
        _state = UPDATE_STATE_IDLE;
    }
}

void AppUpdater::updateLoop(const char* data, size_t dataLen)
{
    switch (_state) {
        case UPDATE_STATE_WAIT_VERSION_INFO: {
            VersionNoType newVersion = *((VersionNoType *)data);
            if (newVersion > _currentVersion) {
                _newVersionSize = *((size_t *)(data + sizeof(VersionNoType)));
                _prepareUpdate();
            }
            else {
                APP_LOGI(TAG, "already the latest version");
                _state = UPDATE_STATE_IDLE;
            }
            break;
        }

        case UPDATE_STATE_WAIT_DATA: {
            size_t dataIndex = *((size_t*)data);
            if (dataIndex != _writeCount) {
                APP_LOGE(TAG, "received data index mismatched with requested");
                break;
            }
            const void *dataBlock = data + sizeof(size_t);
            size_t blockSize = dataLen - sizeof(size_t);
            esp_err_t err = esp_ota_write(_updateHandle, dataBlock, blockSize);
            if (err == ESP_OK) {
                _writeCount += blockSize;
                APP_LOGI(TAG, "write data complete %d%%", int(_writeCount/(float)_newVersionSize) * 100);
                if (_writeCount == _newVersionSize) {
                    _onRxDataComplete();
                }
                else {
                	_delegate->publish(_updateDtxDataTopic, &_writeCount, sizeof(_writeCount), 1);
                }
            } else {
                APP_LOGE(TAG, "esp_ota_write failed! err=0x%x", err);
                if (esp_ota_end(_updateHandle) != ESP_OK) {
                    APP_LOGE(TAG, "esp_ota_end failed!");
                    // should exit and restart ?
                }
                _state = UPDATE_STATE_IDLE;
            }
            break;
        }

        case UPDATE_STATE_IDLE:
            break;

        default:
            break;
    }
}

void AppUpdater::update()
{
    if (!_beforeUpdateCheck()) return;

    Wifi::waitConnected();

    _state = UPDATE_STATE_WAIT_VERSION_INFO;

    _sendUpdateCmd();
}
