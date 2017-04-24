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

#define REQUIRE_VERIFY_BIT_CMD       SIZE_MAX

#define UPDATE_RX_DATA_BLOCK_SIZE    2048

static char _updateDrxDataTopic[32];   // device rx
static char _updateDtxDataTopic[32];   // device tx

#define DEBUG_APP_UPDATER
#ifdef  DEBUG_APP_UPDATER

esp_err_t debug_esp_ota_begin(const esp_partition_t* partition, size_t image_size, esp_ota_handle_t* out_handle)
{
    return ESP_OK;
}

esp_err_t debug_esp_ota_write(esp_ota_handle_t handle, const void* data, size_t size)
{
    return ESP_OK;
}

esp_err_t debug_esp_ota_end(esp_ota_handle_t handle)
{
    return ESP_OK;
}

#define ESP_OTA_BEGIN    debug_esp_ota_begin
#define ESP_OTA_END      debug_esp_ota_end
#define ESP_OTA_WRITE    debug_esp_ota_write

#else

#define ESP_OTA_BEGIN    esp_ota_begin
#define ESP_OTA_END      esp_ota_end
#define ESP_OTA_WRITE    esp_ota_write

#endif


AppUpdater::AppUpdater()
: _state(UPDATE_STATE_IDLE)
, _currentVersion(VERSION)
, _rxTopicLen(0)
, _newVersionSize(0)
, _updateHandle(0)
, _delegate(NULL)
{
    _writeFlag.index = 0;
    _writeFlag.amount = 0;
}

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

    bool succeeded = true;

    if (ESP_OTA_END(_updateHandle) != ESP_OK) {
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

    esp_err_t err = ESP_OTA_BEGIN(_updatePartition, OTA_SIZE_UNKNOWN, &_updateHandle);
    if (err == ESP_OK) {
        _writeFlag.index = 0;
        _writeFlag.amount = UPDATE_RX_DATA_BLOCK_SIZE;
        APP_LOGI(TAG, "esp_ota_begin succeeded");
    } else {
        ESP_LOGE(TAG, "esp_ota_begin failed, error: %d", err);
        _state = UPDATE_STATE_IDLE;
    }
}

bool AppUpdater::_verifyData(const char *verifyBits, size_t length)
{
    return true;
}

void AppUpdater::updateLoop(const char* data, size_t dataLen)
{
    APP_LOGI(TAG, "update loop");
    switch (_state) {
        case UPDATE_STATE_WAIT_VERSION_INFO: {
            VersionNoType newVersion = *((VersionNoType *)data);
            if (newVersion > _currentVersion) {
                _newVersionSize = *((size_t *)(data + sizeof(VersionNoType)));
                _prepareUpdate();
                _delegate->publish(_updateDtxDataTopic, &_writeFlag, sizeof(_writeFlag), 1);
            }
            else {
                APP_LOGI(TAG, "already the latest version");
                _state = UPDATE_STATE_IDLE;
            }
            break;
        }

        case UPDATE_STATE_WAIT_DATA: {
            size_t dataIndex = *((size_t*)data);
            if (dataIndex != _writeFlag.index) {
                APP_LOGE(TAG, "received data index mismatched with requested");
                break;
            }
            const void *dataBlock = data + sizeof(size_t);
            size_t blockSize = dataLen - sizeof(size_t);
            if (_writeFlag.index + blockSize > _newVersionSize) {
                APP_LOGE(TAG, "received data size mismatched with the new version size");
                _state = UPDATE_STATE_IDLE;
                break;
            }
            esp_err_t err = ESP_OTA_WRITE(_updateHandle, dataBlock, blockSize);
            if (err == ESP_OK) {
                _writeFlag.index += blockSize;
                APP_LOGI(TAG, "write data complete %d%%", int(_writeFlag.index/(float)_newVersionSize) * 100);
                if (_writeFlag.index == _newVersionSize) {
                    APP_LOGI(TAG, "total write binary data length : %d", _newVersionSize);
                	_writeFlag.index = REQUIRE_VERIFY_BIT_CMD;
                	_writeFlag.amount = REQUIRE_VERIFY_BIT_CMD;
                    _delegate->publish(_updateDtxDataTopic, &_writeFlag, sizeof(_writeFlag), 1);
                }
                else {
                    _delegate->publish(_updateDtxDataTopic, &_writeFlag, sizeof(_writeFlag), 1);
                }
            } else {
                APP_LOGE(TAG, "esp_ota_write failed! err: 0x%x", err);
                if (ESP_OTA_END(_updateHandle) != ESP_OK) {
                    APP_LOGE(TAG, "esp_ota_end failed");
                    // should exit and restart ?
                }
                _state = UPDATE_STATE_IDLE;
            }
            break;
        }

        case UPDATE_STATE_WAIT_VERIFY_BITS:
            if (_verifyData(data, dataLen)) {
                _onRxDataComplete();
            }
            break;

        case UPDATE_STATE_IDLE:
            break;

        default:
            break;
    }
}

void AppUpdater::update()
{
    APP_LOGI(TAG, "receive update command");
    if (!_beforeUpdateCheck()) return;

    Wifi::waitConnected();

    _state = UPDATE_STATE_WAIT_VERSION_INFO;

    _sendUpdateCmd();
}
