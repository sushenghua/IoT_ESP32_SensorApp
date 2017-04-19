/*
 * System.h system function
 * Copyright (c) 2017 Shenghua Su
 *
 */

#include "System.h"
#include "esp_system.h"
#include <string.h>

static char MAC_ADDR[13] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0xFF };

static void initMacADDR()
{
    uint8_t macAddr[6];
    char *target = MAC_ADDR;
    esp_efuse_read_mac(macAddr);
    for (int i = 0; i < 6; ++i) {
        sprintf(target, "%02x", macAddr[i]);
        target += 2;
    }
    MAC_ADDR[12] = '\0';
}

const char* System::macAddress()
{
    if (MAC_ADDR[12] == 0xFF)
        initMacADDR();
    return MAC_ADDR;
}

const char* System::idfVersion()
{
	return esp_get_idf_version();
}

const char* System::firmwareVersion()
{
	return "1.0";
}

static bool _sysRestartInProgress = false;

void System::restart()
{
	_sysRestartInProgress = true;
	esp_restart();
}

bool System::restartInProgress()
{
	return _sysRestartInProgress;
}
