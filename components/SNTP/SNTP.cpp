/*
 * SNTP: Wrap esp32 c sntp
 * Copyright (c) 2017 Shenghua Su
 *
 */

#include "SNTP.h"
#include "esp_log.h"
#include "Wifi.h"
#include <string.h>

bool       SNTP::_inited = false;
time_t     SNTP::_timeNow = 0;
struct tm  SNTP::_timeInfo;

void SNTP::init(uint8_t operationMode) {
    if (!_inited) {
        ESP_LOGI("[SNTP]", "init SNTP");
        Wifi::waitConnected(); // block wait
        sntp_setoperatingmode(operationMode);
        sntp_setservername(0, (char*)("pool.ntp.org"));
        sntp_init();
        _inited = true;
    }
}

void SNTP::stop()
{
    if (_inited) {
        sntp_stop();
    }
}

bool SNTP::sync(int trials)
{
    // wait for time to be set
    _timeNow = 0;
    memset(&_timeInfo, 0, sizeof(_timeInfo));
    int retry = 0;
    while(_timeInfo.tm_year < (2016 - 1900) && ++retry <= trials) {
        ESP_LOGI("[SNTP]", "waiting for system time to be set... (%d/%d)", retry, trials);
        vTaskDelay(2000 / portTICK_PERIOD_MS);
        time(&_timeNow);
        localtime_r(&_timeNow, &_timeInfo);
    }
    return _timeInfo.tm_year > (2016 - 1900);
}

void SNTP::setTimezone(const char* zone)
{
    setenv("TZ", zone, 1);
    tzset();
}

void SNTP::test()
{
    time_t now;
    char strftime_buf[64];

    now = timeNow();
    // Set timezone to Eastern Standard Time and print local time
    setTimezone("EST5EDT,M3.2.0/2,M11.1.0");
    strftime(strftime_buf, sizeof(strftime_buf), "%c", &timeInfo(now));
    ESP_LOGI("[SNTP]", "The current date/time in New York is: %s", strftime_buf);

    // Set timezone to China Standard Time
    setTimezone("CST-8CDT-9,M4.2.0/2,M9.2.0/3");
    strftime(strftime_buf, sizeof(strftime_buf), "%c", &timeInfo(now));
    ESP_LOGI("[SNTP]", "The current date/time in Shanghai is: %s", strftime_buf);
}
