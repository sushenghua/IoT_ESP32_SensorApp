/*
 * Wifi: Wrap the esp wifi
 * Copyright (c) 2017 Shenghua Su
 *
 */

#ifndef _WIFI_H
#define _WIFI_H

#include "esp_wifi_types.h"


/////////////////////////////////////////////////////////////////////////////////////////
// Wifi config
/////////////////////////////////////////////////////////////////////////////////////////
// ------ AP
#define WIFI_DEFAULT_AP_MAX_CONNECTION 4

// ------ EAP
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

// ------ host info
#define HOST_NAME_MAX_LEN              32

// ------ EAP config structure
struct WifiEapConfig {
    bool                     enabled;
    EapMode                  eapMode;
    char                     eapId[EAP_ID_MAX_LEN];
    char                     eapUsername[EAP_USERNAME_MAX_LEN];
    char                     eapPassword[EAP_PASSWORD_MAX_LEN];
};

struct WifiConfig {
    wifi_mode_t              mode;
    wifi_config_t            apConfig;
    wifi_config_t            staConfig;
    WifiEapConfig            eapConfig;
    char                     hostName[HOST_NAME_MAX_LEN+1];
};


/////////////////////////////////////////////////////////////////////////////////////////
// Wifi class
/////////////////////////////////////////////////////////////////////////////////////////
#undef connect // mongoose lib define 'connect' macro

class Wifi
{
public:
    // singleton
    static Wifi * instance();

public:
    // constructor
    Wifi();

    // config
    void setDefaultConfig();
    void setWifiMode(wifi_mode_t mode);
    bool setStaConfig(const char *ssid, const char *passwd);
    bool setApConfig(const char      *ssid,
                     const char      *passwd,
                     wifi_auth_mode_t authmode = WIFI_AUTH_WPA_WPA2_PSK,
                     uint8_t          maxConnection = WIFI_DEFAULT_AP_MAX_CONNECTION,
                     uint8_t          ssidHidden = 0); // default 0: not hidden

#ifdef ENABLE_EAP
public:
    void enableEap(bool enabled = true) { _config.eapConfig.enabled = enabled; }
    bool setEapConfig(const char    *eapId,
                      const char    *username,
                      const char    *password,
                      EapMode        mode = EAP_PEAP);
protected:
    void _initEap();
#endif

public:
    bool setHostName(const char* hostname);
    void setAutoReconnect(bool autoreconnect = true) { _autoreconnect = autoreconnect; }

    // init, deinit
    void init();
    void deinit();

    // start, stop, connect, disconnect, status
    void start(bool waitConnected = false);
    void stop();
    void connect(bool autoreconnect = true);
    void disconnect();
    void waitConnected();
    bool connected();

    // storage load, save
    bool loadConfig();
    bool saveConfig();

public:
    // event handler
    void onScanDone();
    void onStaStart();
    void onStaStop();
    void onStaConnected();
    void onStaDisconnected();
    void onStaGotIp();

protected:
    // flags
    bool                     _initialized;
    bool                     _started;
    bool                     _connected;
    bool                     _autoreconnect;
    // config
    WifiConfig               _config;
};

#endif // _WIFI_H
