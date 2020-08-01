idf_component_register( SRCS "System.cpp"
                        INCLUDE_DIRS "."
                        REQUIRES Sensor
                        PRIV_REQUIRES bootloader_support app_update Config Common Adc AppUpdater SNTP Buzzer Wifi I2c
                                      DisplayController DisplayDevice Input PowerManager Sensor CmdEngine MessageProtocol )