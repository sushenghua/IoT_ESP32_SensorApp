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

inline size_t stringLen(const char *str)
{
    return strlen((const char*)str);
}

inline void stringAssign(char *target, const char *str, size_t len)
{
    strncpy((char *)target, (const char*)str, len);
}

Wifi::Wifi()
: _initialized(false)
, _started(false)
, _mode(WIFI_MODE_STA)
, _eapEnabled(false)
, _eapMode(EAP_PEAP)
{
}

void Wifi::setWifiMode(wifi_mode_t mode)
{
    if (!_initialized)
        _mode = mode;
}

bool Wifi::setStaConfig(const char *ssid, const char *passwd)
{
    if (_initialized) return false;

    size_t ssidLen = stringLen(ssid);
    size_t ssidMaxLen = sizeof(_apConfig.sta.ssid);
    size_t passwdLen = stringLen(passwd);
    size_t passwdMaxLen = sizeof(_apConfig.sta.password);
    
    if (ssidLen > ssidMaxLen || passwdLen > passwdMaxLen)
        return false;

    stringAssign((char*)_staConfig.sta.ssid, ssid, ssidLen);
    stringAssign((char*)_staConfig.sta.password, passwd, passwdLen);
    
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
    size_t ssidMaxLen = sizeof(_apConfig.ap.ssid);
    size_t passwdLen = stringLen(passwd);
    size_t passwdMaxLen = sizeof(_apConfig.ap.password);

    if (ssidLen > ssidMaxLen || passwdLen > passwdMaxLen)
        return false;

    stringAssign((char*)_apConfig.ap.ssid, ssid, ssidLen);
    stringAssign((char*)_apConfig.ap.password, passwd, passwdLen);

    _apConfig.ap.ssid_len = ssidLen;
    _apConfig.ap.authmode = authmode;
    _apConfig.ap.max_connection = maxConnection;
    _apConfig.ap.ssid_hidden = ssidHidden;

    return true;
}

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

#ifdef ENABLE_EAP

bool Wifi::setEapConfig(EapMode     mode,
                        const char *eapId,
                        const char *username,
                        const char *passwd)
{
    if (_initialized) return false;

    size_t eapIdLen = stringLen(eapId);
    size_t usernameLen = stringLen(username);
    size_t passwdLen = stringLen(passwd);

    if (eapIdLen > EAP_ID_MAX_LEN || usernameLen > EAP_USERNAME_MAX_LEN || passwdLen > EAP_PASSWORD_MAX_LEN)
        return false;

    _eapMode = mode;
    stringAssign(_eapId, eapId, eapIdLen);
    stringAssign(_eapUsername, username, usernameLen);
    stringAssign(_eapPassword, passwd, passwdLen);

    return true;
}

void Wifi::_initEap()
{
    if (_eapEnabled) {
        unsigned int ca_pem_bytes = ca_pem_end - ca_pem_start;
        unsigned int client_crt_bytes = client_crt_end - client_crt_start;
        unsigned int client_key_bytes = client_key_end - client_key_start;

        ESP_ERROR_CHECK( esp_wifi_sta_wpa2_ent_set_ca_cert(ca_pem_start, ca_pem_bytes) );
        ESP_ERROR_CHECK( esp_wifi_sta_wpa2_ent_set_cert_key(client_crt_start, client_crt_bytes,
                                                            client_key_start, client_key_bytes, NULL, 0) );

        ESP_ERROR_CHECK( esp_wifi_sta_wpa2_ent_set_identity((uint8_t*)_eapId, stringLen(_eapId)) );
        if (_eapMode == EAP_PEAP || _eapMode == EAP_TTLS) {
            ESP_ERROR_CHECK( esp_wifi_sta_wpa2_ent_set_username((uint8_t*)_eapUsername, stringLen(_eapUsername)) );
            ESP_ERROR_CHECK( esp_wifi_sta_wpa2_ent_set_password((uint8_t*)_eapPassword, stringLen(_eapPassword)) );
        }
        ESP_ERROR_CHECK( esp_wifi_sta_wpa2_ent_enable() );
    }
}

#endif

void Wifi::init()
{
    if (_initialized) return;

    ESP_LOGI("[Wifi]", "init with mode %d", _mode);

    tcpip_adapter_init();

    wifiEventGroup = xEventGroupCreate();
    ESP_ERROR_CHECK( esp_event_loop_init(wifi_app_event_handler, NULL) );

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK( esp_wifi_init(&cfg) );
    ESP_ERROR_CHECK( esp_wifi_set_storage(WIFI_STORAGE_RAM) );

    ESP_ERROR_CHECK( esp_wifi_set_mode(_mode) );

    if (_mode == WIFI_MODE_APSTA || _mode == WIFI_MODE_STA)
        ESP_ERROR_CHECK( esp_wifi_set_config(ESP_IF_WIFI_STA, &_staConfig) );
    if (_mode == WIFI_MODE_APSTA || _mode == WIFI_MODE_AP)
        ESP_ERROR_CHECK( esp_wifi_set_config(ESP_IF_WIFI_AP, &_apConfig) );

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

void Wifi::start()
{
    if (!_started) {
        ESP_ERROR_CHECK( esp_wifi_start() );
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
