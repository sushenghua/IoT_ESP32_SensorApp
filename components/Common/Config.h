/*
 * config.h App config
 * Copyright (c) 2017 Shenghua Su
 *
 */

#ifndef _CONF_H_INCLUDED
#define _CONF_H_INCLUDED

// ------ DEBUG mode
// #define DEBUG_APP
#ifdef DEBUG_APP
  // ------ Module 1
  //#define DEBUG_MODULE_1

  // ------ Module 2
  //#define DEBUG_MODULE_2
#endif

// #define DEBUG_APP_OK
#define DEBUG_APP_ERR

// #define LOG_MQTT_TX
// #define LOG_MQTT_RX
#define LOG_MQTT_RETX
// #define LOG_MQTT_PING_PONG

// #define LOG_HTTP
// #define LOG_WEBSOCKET
// #define LOG_WEBSOCKET_MSG

#define LOG_ALERT
#define DEBUG_PN

// #define LOG_APPUPDATER

#endif // _CONF_H_INCLUDED
