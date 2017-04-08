/*
 * Wifi: Wrap the esp wifi
 * Copyright (c) 2017 Shenghua Su
 *
 */

#include "Wifi.h"

#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"
#include "esp_wifi.h"
#include "esp_wpa2.h"
#include "esp_event_loop.h"
#include "esp_log.h"
#include "esp_system.h"
#include "nvs_flash.h"
#include "tcpip_adapter.h"

/////////////////////////////////////////////////////////////////////////////////////////
// ------ enterprise AP
#ifdef ENABLE_EAP
/* CA cert, taken from wpa2_ca.pem
   Client cert, taken from wpa2_client.crt
   Client key, taken from wpa2_client.key

   The PEM, CRT and KEY file were provided by the person or organization
   who configured the AP with wpa2 enterprise.

   To embed it in the app binary, the PEM, CRT and KEY file is named
   in the component.mk COMPONENT_EMBED_TXTFILES variable.
*/
extern uint8_t ca_pem_start[] asm("_binary_wpa2_ca_pem_start");
extern uint8_t ca_pem_end[]   asm("_binary_wpa2_ca_pem_end");
extern uint8_t client_crt_start[] asm("_binary_wpa2_client_crt_start");
extern uint8_t client_crt_end[]   asm("_binary_wpa2_client_crt_end");
extern uint8_t client_key_start[] asm("_binary_wpa2_client_key_start");
extern uint8_t client_key_end[]   asm("_binary_wpa2_client_key_end");

#endif // ENABLE_EAP

/////////////////////////////////////////////////////////////////////////////////////////
// ------ helper
inline size_t stringLen(const char *str)
{
    return strlen((const char*)str);
}

inline void stringAssign(char *target, const char *str, size_t len)
{
    strncpy((char *)target, (const char*)str, len);
}

/////////////////////////////////////////////////////////////////////////////////////////
// ------ contructor
Wifi::Wifi()
: _initialized(false)
, _started(false)
{
    // set default config value
    _config.mode = WIFI_MODE_STA;
    _config.eapConfig.enabled = false;
    _config.eapConfig.eapMode = EAP_PEAP;
}

/////////////////////////////////////////////////////////////////////////////////////////
// ------ config
void Wifi::setWifiMode(wifi_mode_t mode)
{
    if (!_initialized)
        _config.mode = mode;
}

bool Wifi::setStaConfig(const char *ssid, const char *passwd)
{
    if (_initialized) return false;

    size_t ssidLen = stringLen(ssid);
    size_t ssidMaxLen = sizeof(_config.apConfig.sta.ssid);
    size_t passwdLen = stringLen(passwd);
    size_t passwdMaxLen = sizeof(_config.apConfig.sta.password);
    
    if (ssidLen > ssidMaxLen || passwdLen > passwdMaxLen)
        return false;

    stringAssign((char*)_config.staConfig.sta.ssid, ssid, ssidLen);
    stringAssign((char*)_config.staConfig.sta.password, passwd, passwdLen);
    
    return true;
}

bool Wifi::setApConfig(const char      *ssid,
                       const char      *passwd,
                       wifi_auth_mode_t authmode,
                       uint8_t          maxConnection,
                       uint8_t          ssidHidden)
{
    if (_initialized) return false;

    size_t ssidLen = stringLen(ssid);
    size_t ssidMaxLen = sizeof(_config.apConfig.ap.ssid);
    size_t passwdLen = stringLen(passwd);
    size_t passwdMaxLen = sizeof(_config.apConfig.ap.password);

    if (ssidLen > ssidMaxLen || passwdLen > passwdMaxLen)
        return false;

    stringAssign((char*)_config.apConfig.ap.ssid, ssid, ssidLen);
    stringAssign((char*)_config.apConfig.ap.password, passwd, passwdLen);

    _config.apConfig.ap.ssid_len = ssidLen;
    _config.apConfig.ap.authmode = authmode;
    _config.apConfig.ap.max_connection = maxConnection;
    _config.apConfig.ap.ssid_hidden = ssidHidden;

    return true;
}


#ifdef ENABLE_EAP

bool Wifi::setEapConfig(const char *eapId,
                        const char *username,
                        const char *password,
                        EapMode     mode)
{
    if (_initialized) return false;

    size_t eapIdLen = stringLen(eapId);
    size_t usernameLen = stringLen(username);
    size_t passwdLen = stringLen(password);

    if (eapIdLen > EAP_ID_MAX_LEN ||
        usernameLen > EAP_USERNAME_MAX_LEN ||
        passwdLen < 8 ||
        passwdLen > EAP_PASSWORD_MAX_LEN)
        return false;

    _config.eapConfig.eapMode = mode;
    stringAssign(_config.eapConfig.eapId, eapId, eapIdLen);
    stringAssign(_config.eapConfig.eapUsername, username, usernameLen);
    stringAssign(_config.eapConfig.eapPassword, password, passwdLen);

    return true;
}

void Wifi::_initEap()
{
    if (_config.eapConfig.enabled) {
        unsigned int ca_pem_bytes = ca_pem_end - ca_pem_start;
        unsigned int client_crt_bytes = client_crt_end - client_crt_start;
        unsigned int client_key_bytes = client_key_end - client_key_start;

        ESP_ERROR_CHECK( esp_wifi_sta_wpa2_ent_set_ca_cert(ca_pem_start, ca_pem_bytes) );
        ESP_ERROR_CHECK( esp_wifi_sta_wpa2_ent_set_cert_key(client_crt_start, client_crt_bytes,
                                                            client_key_start, client_key_bytes, NULL, 0) );

        ESP_ERROR_CHECK( esp_wifi_sta_wpa2_ent_set_identity((uint8_t*)_config.eapConfig.eapId,
                                                             stringLen(_config.eapConfig.eapId)) );
        if (_config.eapConfig.eapMode == EAP_PEAP || _config.eapConfig.eapMode == EAP_TTLS) {
            ESP_ERROR_CHECK( esp_wifi_sta_wpa2_ent_set_username((uint8_t*)_config.eapConfig.eapUsername,
                                                                 stringLen(_config.eapConfig.eapUsername)) );
            ESP_ERROR_CHECK( esp_wifi_sta_wpa2_ent_set_password((uint8_t*)_config.eapConfig.eapPassword,
                                                                 stringLen(_config.eapConfig.eapPassword)) );
        }
        ESP_ERROR_CHECK( esp_wifi_sta_wpa2_ent_enable() );
    }
}

#endif

bool Wifi::setHostName(const char* hostname)
{
    size_t len = stringLen(hostname);
    if (len > HOST_NAME_MAX_LEN || len == 0) return false;

    stringAssign(_config.hostName, hostname, len);
    if ( _started && (_config.mode == WIFI_MODE_APSTA || _config.mode == WIFI_MODE_STA) ) {
        ESP_ERROR_CHECK( tcpip_adapter_set_hostname(TCPIP_ADAPTER_IF_STA, _config.hostName) );
    }

    return true;
}

/////////////////////////////////////////////////////////////////////////////////////////
// ------ wifi init and event handler
/* FreeRTOS event group to signal when we are connected & ready to make a request */
static EventGroupHandle_t wifiEventGroup;

/* The event group allows multiple bits for each event,
   but we only care about one event - are we connected
   to the AP with an IP? */
const int CONNECTED_BIT = BIT0;

static esp_err_t wifi_app_event_handler(void *ctx, system_event_t *event)
{
    switch (event->event_id) {
        case SYSTEM_EVENT_STA_START:
ESP_LOGI("[Wifi]", "connect event");
            ESP_ERROR_CHECK( esp_wifi_connect() );
            break;
        case SYSTEM_EVENT_STA_GOT_IP:
ESP_LOGI("[Wifi]", "got ip event");
            xEventGroupSetBits(wifiEventGroup, CONNECTED_BIT);
            break;
        case SYSTEM_EVENT_STA_DISCONNECTED:
ESP_LOGI("[Wifi]", "disconnected event");
            ESP_ERROR_CHECK( esp_wifi_connect() );
            xEventGroupClearBits(wifiEventGroup, CONNECTED_BIT);
            break;
        default:
            break;
    }
    return ESP_OK;
}

void Wifi::init()
{
    if (_initialized) return;

    ESP_LOGI("[Wifi]", "init with mode %d", _config.mode);

    tcpip_adapter_init();

    wifiEventGroup = xEventGroupCreate();
    ESP_ERROR_CHECK( esp_event_loop_init(wifi_app_event_handler, NULL) );

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK( esp_wifi_init(&cfg) );
    ESP_ERROR_CHECK( esp_wifi_set_storage(WIFI_STORAGE_RAM) );

    ESP_ERROR_CHECK( esp_wifi_set_mode(_config.mode) );

    if (_config.mode == WIFI_MODE_APSTA || _config.mode == WIFI_MODE_STA)
        ESP_ERROR_CHECK( esp_wifi_set_config(ESP_IF_WIFI_STA, &_config.staConfig) );
    if (_config.mode == WIFI_MODE_APSTA || _config.mode == WIFI_MODE_AP)
        ESP_ERROR_CHECK( esp_wifi_set_config(ESP_IF_WIFI_AP, &_config.apConfig) );

#ifdef ENABLE_EAP
    _initEap();
#endif

    _initialized = true;
}

void Wifi::deinit()
{
    if (_initialized) {
        ESP_ERROR_CHECK( esp_wifi_deinit() );
        _initialized = false;
    }
}

/////////////////////////////////////////////////////////////////////////////////////////
// ------ laod, save config
#include "nvs.h"
#define STORAGE_NAMESPACE               "storage"
#define WIFI_CONFIG_TAG                 "wifiConf"
#define WIFI_CONFIG_SAVE_COUNT_TAG      "wifiConfSC"  // wifi config save count

bool Wifi::loadConfig()
{
    bool succeeded = false;
    bool nvsOpened = false;

    nvs_handle nvsHandle;
    esp_err_t err;

    do {
        // open nvs
        err = nvs_open(STORAGE_NAMESPACE, NVS_READONLY, &nvsHandle);
        if (err != ESP_OK) {
            ESP_LOGE("[Wifi]", "loadConfig open nvs failed %d", err);
            break;
        }
        nvsOpened = true;

        // read wifi save count
        uint16_t wifiConfigSaveCount = 0; // value will default to 0, if not set yet in NVS
        err = nvs_get_u16(nvsHandle, WIFI_CONFIG_SAVE_COUNT_TAG, &wifiConfigSaveCount);
        if (err != ESP_OK && err != ESP_ERR_NVS_NOT_FOUND) {
            ESP_LOGE("[Wifi]", "loadConfig read \"save-count\" failed %d", err);
            break;
        }

        // read wifi config
        if (wifiConfigSaveCount > 0) {
            size_t requiredSize = 0;  // value will default to 0, if not set yet in NVS
            err = nvs_get_blob(nvsHandle, WIFI_CONFIG_TAG, NULL, &requiredSize);
            if (err != ESP_OK && err != ESP_ERR_NVS_NOT_FOUND) {
                ESP_LOGE("[Wifi]", "loadConfig read \"wifi-config-size\" failed %d", err);
                break;
            }
            if (requiredSize != sizeof(_config)) {
                ESP_LOGE("[Wifi]", "loadConfig read \"wifi-config-size\" got unexpected value");
                break;
            }
            // read previously saved config
            err = nvs_get_blob(nvsHandle, WIFI_CONFIG_TAG, &_config, &requiredSize);
            if (err != ESP_OK) {
                ESP_LOGE("[Wifi]", "loadConfig read \"wifi-config-content\" failed %d", err);
                break;
            }
            succeeded = true;
        }
    } while(false);

    // close nvs
    if (nvsOpened) nvs_close(nvsHandle);

    return succeeded;
}

bool Wifi::saveConfig()
{
    bool succeeded = false;
    bool nvsOpened = false;

    nvs_handle nvsHandle;
    esp_err_t err;

    do {
        // open nvs
        err = nvs_open(STORAGE_NAMESPACE, NVS_READWRITE, &nvsHandle);
        if (err != ESP_OK) {
            ESP_LOGE("[Wifi]", "saveConfig open nvs failed %d", err);
            break;
        }
        nvsOpened = true;

        // read wifi save count
        uint16_t wifiConfigSaveCount = 0; // value will default to 0, if not set yet in NVS
        err = nvs_get_u16(nvsHandle, WIFI_CONFIG_SAVE_COUNT_TAG, &wifiConfigSaveCount);
        if (err != ESP_OK && err != ESP_ERR_NVS_NOT_FOUND) {
            ESP_LOGE("[Wifi]", "saveConfig read \"save-count\" failed %d", err);
            break;
        }

        // write wifi config
        err = nvs_set_blob(nvsHandle, WIFI_CONFIG_TAG, &_config, sizeof(_config));
        if (err != ESP_OK) {
            ESP_LOGE("[Wifi]", "saveConfig write \"wifi-config\" failed %d", err);
            break;
        }

        // write save count
        wifiConfigSaveCount++;
        err = nvs_set_u16(nvsHandle, WIFI_CONFIG_SAVE_COUNT_TAG, wifiConfigSaveCount);
        if (err != ESP_OK) {
            ESP_LOGE("[Wifi]", "saveConfig write \"save-count\" failed %d", err);
            break;
        }

        // commit written value.
        err = nvs_commit(nvsHandle);
        if (err != ESP_OK) {
            ESP_LOGE("[Wifi]", "saveConfig commit failed %d", err);
            break;
        }
        succeeded = true;

    } while(false);

    // close nvs
    if (nvsOpened) nvs_close(nvsHandle);

    return succeeded;
}

/////////////////////////////////////////////////////////////////////////////////////////
// ------ start, stop
void Wifi::start()
{
    if (!_started) {
        ESP_ERROR_CHECK( esp_wifi_start() );
        if ( (_config.mode == WIFI_MODE_APSTA || _config.mode == WIFI_MODE_STA) &&_config.hostName[0] != '\0') {
            ESP_ERROR_CHECK( tcpip_adapter_set_hostname(TCPIP_ADAPTER_IF_STA, _config.hostName) );
        }
        _started = true;
    }
}

void Wifi::stop()
{
    if (_started) {
        ESP_ERROR_CHECK( esp_wifi_stop() );
        _started = false;
    }
}
