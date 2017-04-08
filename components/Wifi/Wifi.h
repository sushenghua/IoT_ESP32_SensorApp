/*
 * Wifi: Wrap the esp wifi
 * Copyright (c) 2017 Shenghua Su
 *
 */

#ifndef _WIFI_H
#define _WIFI_H

#include "esp_wifi_types.h"

// AP
#define WIFI_DEFAULT_AP_MAX_CONNECTION 4

// EAP
#define ENABLE_EAP
#ifdef ENABLE_EAP
#define EAP_ID_MAX_LEN                 127
#define EAP_USERNAME_MAX_LEN           32
#define EAP_PASSWORD_MAX_LEN           64
enum EapMode {
    EAP_NULL   = 0,
    EAP_PEAP,
    EAP_TTLS
};
#endif

// host info
#define HOST_NAME_MAX_LEN              32

// Wifi class
class Wifi
{
public:
    // constructor
    Wifi();

    // config
    void setWifiMode(wifi_mode_t mode);
    bool setStaConfig(const char *ssid, const char *passwd);
    bool setApConfig(const char      *ssid,
                     const char      *passwd,
                     wifi_auth_mode_t authmode = WIFI_AUTH_WPA_WPA2_PSK,
                     uint8_t          maxConnection = WIFI_DEFAULT_AP_MAX_CONNECTION,
                     uint8_t          ssidHidden = 0); // default 0: not hidden

#ifdef ENABLE_EAP
    void enableEap(bool enabled = true) { _eapEnabled = enabled; }
    bool setEapConfig(EapMode        mode,
                      const char    *eapId,
                      const char    *eapUsername,
                      const char    *passwd);
#endif

    bool setHostName(const char* hostname);

    // storage load, save
    void loadConfig();
    void saveConfig();

    // init, deinit
    void init();
    void deinit();

    // start, stop
    void start();
    void stop();

protected:
    // init
    bool                     _initialized;
    bool                     _started;
    // wifi mode and config
    wifi_mode_t              _mode;
    wifi_config_t            _apConfig;
    wifi_config_t            _staConfig;

#ifdef ENABLE_EAP
    void _initEap();
    // EAP
    bool                     _eapEnabled;
    EapMode                  _eapMode;
    char                     _eapId[EAP_ID_MAX_LEN];
    char                     _eapUsername[EAP_USERNAME_MAX_LEN];
    char                     _eapPassword[EAP_PASSWORD_MAX_LEN];
#endif
    // Host info
    char                     _hostName[HOST_NAME_MAX_LEN+1];
};

#endif // _WIFI_H
