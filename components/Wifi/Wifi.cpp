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
#include "esp_system.h"
#include "tcpip_adapter.h"

#include "AppLog.h"
#include "System.h"

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
  target[0] = '\0';
  strncat((char *)target, (const char*)str, len);
}

/////////////////////////////////////////////////////////////////////////////////////////
// ------ static shared instance
static Wifi _wifiInstance;

Wifi * Wifi::instance()
{
  return &_wifiInstance;
}

/////////////////////////////////////////////////////////////////////////////////////////
// ------ contructor
Wifi::Wifi()
: _initialized(false)
, _started(false)
, _connected(false)
, _autoreconnect(true)
, _apStaConnectionCount(0)
, _nextAltApIndex(0)
, _connectionFailCount(0)
, _altApsConnectionFailRound(0)
{
  // set default config value
  _config.mode = WIFI_MODE_AP;
  _config.powerSaveType = WIFI_PS_MODEM;
#ifdef ENABLE_EAP
  _config.eapConfig.enabled = false;
  _config.eapConfig.eapMode = EAP_PEAP;
#endif
  _config.altApsHead = 0;
  _config.altApCount = 0;
}

/////////////////////////////////////////////////////////////////////////////////////////
// ------ config
void Wifi::setDefaultConfig()
{
  char apName[32];
  sprintf(apName, "%s_%.*s", "AQStation", 8, System::instance()->uid());

  setWifiMode(WIFI_MODE_APSTA);
  setStaConfig("ssid", "ssidpasswd", false, false);
  setApConfig(apName, "aqstation");
  setHostName(apName);
#ifdef ENABLE_EAP
  enableEap(false);
  setEapConfig("eapid", "eapusername", "eapuserpassword");
#endif
  _config.altApsHead = 0;
  _config.altApCount = 0;
  _nextAltApIndex = 0;
}

void Wifi::setWifiMode(wifi_mode_t mode)
{
  if (!_initialized)
    _config.mode = mode;
}

void Wifi::setPowerSaveEnabled(bool enabled)
{
  _config.powerSaveType = enabled ? WIFI_PS_MODEM : WIFI_PS_NONE;
  esp_wifi_set_ps(_config.powerSaveType);
}

bool _setConfSsidPass(uint8_t *ssidT, const char *ssidS, size_t ssidMaxLen,
                      uint8_t *passT, const char *passS, size_t passMaxLen)
{
  size_t ssidLen = stringLen(ssidS);
  size_t passLen = stringLen(passS);

  if (ssidLen > ssidMaxLen || passLen > passMaxLen) return false;

  stringAssign((char*)ssidT, ssidS, ssidLen);
  stringAssign((char*)passT, passS, passLen);
  return true;
}

bool Wifi::setStaConfig(const char *ssid, const char *passwd, bool forceOverride, bool append)
{
  if (!forceOverride && _initialized) return false;

  if (append)
    appendAltApConnectionSsidPassword(ssid, passwd);

  return _setConfSsidPass(_config.staConfig.sta.ssid, ssid,
                          sizeof(_config.staConfig.sta.ssid),
                          _config.staConfig.sta.password, passwd,
                          sizeof(_config.staConfig.sta.password));
}

bool Wifi::setApConfig(const char      *ssid,
                       const char      *passwd,
                       bool             forceOverride,
                       wifi_auth_mode_t authmode,
                       uint8_t          maxConnection,
                       uint8_t          ssidHidden)
{
  if (!forceOverride && _initialized) return false;

  if ( _setConfSsidPass(_config.apConfig.ap.ssid, ssid,
                        sizeof(_config.apConfig.ap.ssid),
                        _config.apConfig.ap.password, passwd,
                        sizeof(_config.apConfig.ap.password)) ) {

    _config.apConfig.ap.ssid_len = stringLen(ssid);
    _config.apConfig.ap.authmode = authmode;
    _config.apConfig.ap.max_connection = maxConnection;
    _config.apConfig.ap.ssid_hidden = ssidHidden;
    return true;
  }

  return false;
}

bool Wifi::setAltApConnectionSsidPassword(const char *ssid, const char *passwd, uint8_t index)
{
  if (index >= ALTERNATIVE_AP_LIST_SIZE) return false;

  index += _config.altApsHead;
  index = index % ALTERNATIVE_AP_LIST_SIZE;

    return _setConfSsidPass(_config.altAps[index].ssid, ssid,
                            sizeof(_config.staConfig.sta.ssid),
                            _config.altAps[index].password, passwd,
                            sizeof(_config.staConfig.sta.password));
}

bool Wifi::appendAltApConnectionSsidPassword(const char *ssid, const char *passwd)
{
  // circular move ahead
  uint8_t index = (ALTERNATIVE_AP_LIST_SIZE + _config.altApsHead - 1) % ALTERNATIVE_AP_LIST_SIZE;

  if ( _setConfSsidPass(_config.altAps[index].ssid, ssid,
                        sizeof(_config.staConfig.sta.ssid),
                        _config.altAps[index].password, passwd,
                        sizeof(_config.staConfig.sta.password)) ) {

    _config.altApsHead = index;
    _nextAltApIndex = 0; // update RAM next Alt index, point to head (head + 0)

    if (_config.altApCount < ALTERNATIVE_AP_LIST_SIZE)
      ++_config.altApCount;

    return true;
  }

  return false;
}

void Wifi::loadNextAltSsidPassword()
{
  if (_config.altApCount > 0) {
    if (_config.mode == WIFI_MODE_APSTA || _config.mode == WIFI_MODE_STA) {
      uint8_t index = (_config.altApsHead + _nextAltApIndex) % ALTERNATIVE_AP_LIST_SIZE;
      APP_LOGC("[Wifi]", "load alt ssid: %s, pass: %s",
               (const char*)_config.altAps[index].ssid,
               (const char*)_config.altAps[index].password);
      _setConfSsidPass(_config.staConfig.sta.ssid,
                       (const char*)_config.altAps[index].ssid,
                       sizeof(_config.staConfig.sta.ssid),
                       _config.staConfig.sta.password,
                       (const char*)_config.altAps[index].password,
                       sizeof(_config.staConfig.sta.password));

      ESP_ERROR_CHECK( esp_wifi_set_config(ESP_IF_WIFI_STA, &_config.staConfig) );
      _nextAltApIndex = (_nextAltApIndex + 1) % _config.altApCount;
    }
  }
}

void Wifi::clearAltApConnectionSsidPassword()
{
  _config.altApCount = 1; // mark only head valid
}

void Wifi::getAltApConnectionSsidPassword(SsidPasswd *&list, uint8_t &head, uint8_t &count)
{
  list = _config.altAps;
  head = _config.altApsHead;
  count = _config.altApCount;
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

// host name can be changed at Framework level:
// esp-idf/components/lwip/port/netif/ethernetif.c:213:  netif->hostname = "espressif";
// esp-idf/components/lwip/port/netif/wlanif.c:202:  netif->hostname = "espressif";
bool Wifi::setHostName(const char* hostname)
{
  size_t len = stringLen(hostname);
  if (len > HOST_NAME_MAX_LEN || len == 0) return false;

  stringAssign(_config.hostName, hostname, len);
  if ( _started ) {
    if (_config.mode == WIFI_MODE_APSTA || _config.mode == WIFI_MODE_STA) {
      ESP_ERROR_CHECK( tcpip_adapter_set_hostname(TCPIP_ADAPTER_IF_STA, _config.hostName) );
    }
    if (_config.mode == WIFI_MODE_APSTA || _config.mode == WIFI_MODE_AP) {
      ESP_ERROR_CHECK( tcpip_adapter_set_hostname(TCPIP_ADAPTER_IF_AP, _config.hostName) );
    }
  }

  return true;
}

const char * Wifi::getHostName()
{
  const char *hostname = NULL;
  if (_config.mode == WIFI_MODE_STA || _config.mode == WIFI_MODE_APSTA) {
    ESP_ERROR_CHECK( tcpip_adapter_get_hostname(TCPIP_ADAPTER_IF_STA, &hostname) );
  }
  else if (_config.mode == WIFI_MODE_AP) {
    ESP_ERROR_CHECK( tcpip_adapter_get_hostname(TCPIP_ADAPTER_IF_AP, &hostname) );
  }
  return hostname;
}

const char * Wifi::staSsid()
{
  return (const char *)_config.staConfig.sta.ssid;
}

const char * Wifi::staPassword()
{
  return (const char *)_config.staConfig.sta.password;
}

const char * Wifi::apSsid()
{
  return (const char *)_config.apConfig.ap.ssid;
}

const char * Wifi::apPassword()
{
  return (const char *)_config.apConfig.ap.password;
}

/////////////////////////////////////////////////////////////////////////////////////////
// ------ wifi init and event handler
/* FreeRTOS event group to signal when we are connected & ready to make a request */
static EventGroupHandle_t wifiEventGroup;

/* The event group allows multiple bits for each event,
   but we only care about one event - are we connected
   to the AP with an IP? */
static const int CONNECTED_BIT = BIT0;

static esp_err_t wifi_app_event_handler(void *ctx, system_event_t *event)
{
  switch (event->event_id) {

    case SYSTEM_EVENT_STA_START:
      Wifi::instance()->onStaStart();
      break;

    case SYSTEM_EVENT_STA_GOT_IP:
      Wifi::instance()->onStaGotIp();
      break;

    case SYSTEM_EVENT_STA_DISCONNECTED:
      Wifi::instance()->onStaDisconnected(event->event_info);
      break;

    case SYSTEM_EVENT_AP_START:
      Wifi::instance()->onApStart();
      break;

    case SYSTEM_EVENT_AP_STACONNECTED:
      Wifi::instance()->onApStaConnected();
      break;

    case SYSTEM_EVENT_AP_STADISCONNECTED:
      Wifi::instance()->onApStaDisconnected(event->event_info);
      break;

    default:
      break;
  }
  return ESP_OK;
}

/////////////////////////////////////////////////////////////////////////////////////////
// ------ event handler
void Wifi::onScanDone()
{
}

void Wifi::onStaStart()
{
  APP_LOGI("[Wifi]", "try to connect to %s", _config.staConfig.sta.ssid);
  ESP_ERROR_CHECK( esp_wifi_connect() );
}

void Wifi::onStaGotIp()
{
  APP_LOGI("[Wifi]", "connected, got ip");
  xEventGroupSetBits(wifiEventGroup, CONNECTED_BIT);
  _connectionFailCount = 0;
  _altApsConnectionFailRound = 0;
  _connected = true;

  if ( (_config.mode == WIFI_MODE_APSTA || _config.mode == WIFI_MODE_STA) && _config.hostName[0] != '\0') {
    ESP_ERROR_CHECK( tcpip_adapter_set_hostname(TCPIP_ADAPTER_IF_STA, _config.hostName) );
  }
}

void Wifi::onStaStop()
{
}

void Wifi::onStaConnected()
{
}

#define TRY_OTHER_AP_AFTER_FAIL_COUNT  3
#define MAX_ALT_APS_TRY_ROUND          4

void Wifi::onStaDisconnected(system_event_info_t &info)
{
  APP_LOGI("[Wifi]", "disconnected, code: %d", info.disconnected.reason);
  xEventGroupClearBits(wifiEventGroup, CONNECTED_BIT);
  _connected = false;
  bool tryReconnect = true;
  switch(info.disconnected.reason) {
    case WIFI_REASON_4WAY_HANDSHAKE_TIMEOUT:
    case WIFI_REASON_AUTH_FAIL:
    case WIFI_REASON_NO_AP_FOUND:
      ++_connectionFailCount;
      break;
    case WIFI_REASON_ASSOC_LEAVE:
      tryReconnect = false;
      break;
    default:
      break;
  }

  if (!tryReconnect) return;

  if (_connectionFailCount >= TRY_OTHER_AP_AFTER_FAIL_COUNT) {
    if (_nextAltApIndex == _config.altApCount - 1) ++_altApsConnectionFailRound;
    if (_altApsConnectionFailRound < MAX_ALT_APS_TRY_ROUND) {
      loadNextAltSsidPassword();
      _connectionFailCount = 0;
    }
  }
  if (!System::instance()->restarting() && _altApsConnectionFailRound < MAX_ALT_APS_TRY_ROUND
      && _autoreconnect) {
    if (_started) {
      // ESP_ERROR_CHECK( esp_wifi_connect() );
      esp_err_t ret = esp_wifi_connect();
      if (ret != ESP_OK) {
        if (ret == ESP_ERR_WIFI_NOT_INIT)
          APP_LOGE("[Wifi]", "esp_wifi_connect err: ESP_ERR_WIFI_NOT_INIT");
        else if (ret == ESP_ERR_WIFI_NOT_STARTED)
          APP_LOGE("[Wifi]", "esp_wifi_connect err: ESP_ERR_WIFI_NOT_START");
      }
    }
  }
}

void Wifi::onApStart()
{
  APP_LOGI("[Wifi]", "AP start");
  if ( (_config.mode == WIFI_MODE_AP) && _config.hostName[0] != '\0') {
    ESP_ERROR_CHECK( tcpip_adapter_set_hostname(TCPIP_ADAPTER_IF_AP, _config.hostName) );
  }
}

void Wifi::onApStaConnected()
{
  ++_apStaConnectionCount;
  // APP_LOGC("[Wifi]", "++_apStaConnectionCount %d", _apStaConnectionCount);
}

void Wifi::onApStaDisconnected(system_event_info_t &info)
{
  _apStaConnectionCount = 0;
  // APP_LOGC("[Wifi]", "--_apStaConnectionCount %d", _apStaConnectionCount);
}

void Wifi::waitConnected()
{
  xEventGroupWaitBits(wifiEventGroup, CONNECTED_BIT, false, true, portMAX_DELAY);
}

bool Wifi::connected()
{
  return _connected;
}

bool Wifi::apStaConnected()
{
  return _apStaConnectionCount > 0;
}

bool Wifi::started()
{
  return _started;
}

/////////////////////////////////////////////////////////////////////////////////////////
// ------ connect, disconnect
void Wifi::connect(bool autoreconnect)
{
  _autoreconnect = autoreconnect;
  if (_started && !_connected) {
    ESP_ERROR_CHECK( esp_wifi_connect() );
  }
}

void Wifi::disconnect()
{
  if (_started && _connected) {
    _autoreconnect = false;
    ESP_ERROR_CHECK( esp_wifi_disconnect() );
  }
}

/////////////////////////////////////////////////////////////////////////////////////////
// ------ init, deinit
void Wifi::init()
{
  if (_initialized) return;

  tcpip_adapter_init();

  if (_config.mode == WIFI_MODE_APSTA || _config.mode == WIFI_MODE_AP) {
    tcpip_adapter_ip_info_t info = { 0, 0, 0};
    IP4_ADDR(&info.ip, 192, 168, 4, 1);
    IP4_ADDR(&info.gw, 192, 168, 4, 1);
    IP4_ADDR(&info.netmask, 255, 255, 255, 0);
    ESP_ERROR_CHECK(tcpip_adapter_dhcps_stop(TCPIP_ADAPTER_IF_AP));
    ESP_ERROR_CHECK(tcpip_adapter_set_ip_info(TCPIP_ADAPTER_IF_AP, &info));
    ESP_ERROR_CHECK(tcpip_adapter_dhcps_start(TCPIP_ADAPTER_IF_AP));
  }

  wifiEventGroup = xEventGroupCreate();
  ESP_ERROR_CHECK( esp_event_loop_init(wifi_app_event_handler, NULL) );

  wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
  ESP_ERROR_CHECK( esp_wifi_init(&cfg) );
  ESP_ERROR_CHECK( esp_wifi_set_storage(WIFI_STORAGE_RAM) );

  esp_err_t ret = esp_wifi_set_ps(_config.powerSaveType);
  APP_LOGI("[Wifi]", "init with power save support: %s", ret != ESP_ERR_NOT_SUPPORTED ? "Yes" : "No");

  APP_LOGI("[Wifi]", "init with mode %d", _config.mode);
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
// ------ start, stop
void Wifi::start(bool waitConnected)
{
  if (!_started) {
    _nextAltApIndex = 0;
    _connectionFailCount = 0;
    _altApsConnectionFailRound = 0;
    APP_LOGC("[Wifi]", "start wifi");
    ESP_ERROR_CHECK( esp_wifi_start() );
    if (waitConnected) this->waitConnected();
    // if ( (_config.mode == WIFI_MODE_APSTA || _config.mode == WIFI_MODE_STA) && _config.hostName[0] != '\0') {
    //   if (!waitConnected) this->waitConnected();  // wait connect anyway
    //   ESP_ERROR_CHECK( tcpip_adapter_set_hostname(TCPIP_ADAPTER_IF_STA, _config.hostName) );
    // }
    _started = true;
  }
}

void Wifi::stop()
{
  if (_started) {
    APP_LOGC("[Wifi]", "stop wifi");
    if (_connected) {
      ESP_ERROR_CHECK( esp_wifi_disconnect() );
    }
    while (_connected) vTaskDelay(100/portTICK_RATE_MS);
    ESP_ERROR_CHECK( esp_wifi_stop() );
    _started = false;
  }
}

/////////////////////////////////////////////////////////////////////////////////////////
// ------ load, save config
#include "NvsFlash.h"
#define WIFI_STORAGE_NAMESPACE          "wifi"
#define WIFI_CONFIG_TAG                 "wifiConf"
#define WIFI_CONFIG_SAVE_COUNT_TAG      "wifiConfSC"  // wifi config save count

bool Wifi::loadConfig()
{
  bool succeeded = NvsFlash::loadData(WIFI_CONFIG_TAG, &_config, sizeof(_config), WIFI_CONFIG_SAVE_COUNT_TAG);
  if (succeeded) _nextAltApIndex = 0; // point to _config.altApsHead (head + nextIndex)
  return succeeded;
}

bool Wifi::saveConfig()
{
  return NvsFlash::saveData(WIFI_CONFIG_TAG, &_config, sizeof(_config), WIFI_CONFIG_SAVE_COUNT_TAG);
}
