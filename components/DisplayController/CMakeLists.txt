idf_component_register( SRCS "DisplayController.cpp" "SensorDisplayController.cpp"
                        INCLUDE_DIRS "."
                        REQUIRES DisplayDevice Sensor
                        PRIV_REQUIRES  Common SNTP Application QRCode )