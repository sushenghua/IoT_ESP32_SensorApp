/*
 * System.h system function
 * Copyright (c) 2017 Shenghua Su
 *
 */

#ifndef _SYSTEM_H_
#define _SYSTEM_H_

class System
{
public:
    // type define
    enum State {
        Uninitialized,
        Initializing,
        Running,
        Restarting,
        Error
    };
    enum ConfigMode {
        HTTPServerMode,
        MQTTClientMode
    };

public:
    // singleton
    static System * instance();

public:
    System();
    void init();
    const char* uid();
    const char* macAddress();
    const char* idfVersion();
    const char* firmwareVersion();

    void setConfigMode(ConfigMode mode);
    void restart();
    bool restarting();

private:
    bool _loadConfig();
    bool _saveConfig();
    void _launchTasks();

private:
    State        _state;
    ConfigMode   _mode;
};

#endif // _SYSTEM_H_
