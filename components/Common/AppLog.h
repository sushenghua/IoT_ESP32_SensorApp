/*
 * AppLog.h wrap esp log
 * Copyright (c) 2017 Shenghua Su
 *
 */

#ifndef _APP_LOG_H
#define _APP_LOG_H

#define ENABLE_APP_LOG
#ifdef ENABLE_APP_LOG

#include "esp_log.h"

#define APP_LOGE( tag, format, ... )  ESP_LOGE(tag, format, ##__VA_ARGS__)
#define APP_LOGW( tag, format, ... )  ESP_LOGW(tag, format, ##__VA_ARGS__)
#define APP_LOGI( tag, format, ... )  ESP_LOGI(tag, format, ##__VA_ARGS__)
#define APP_LOGD( tag, format, ... )  ESP_LOGD(tag, format, ##__VA_ARGS__)
#define APP_LOGV( tag, format, ... )  ESP_LOGV(tag, format, ##__VA_ARGS__)

#else

#define APP_LOGE( tag, format, ... )
#define APP_LOGW( tag, format, ... )
#define APP_LOGI( tag, format, ... )
#define APP_LOGD( tag, format, ... )
#define APP_LOGV( tag, format, ... )

#endif // ENABLE_APP_LOG

#endif // _APP_LOG_H