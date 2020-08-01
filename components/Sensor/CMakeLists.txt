idf_component_register( SRC_DIRS "Common" "CO2" "PM" "TempHumid" "TSL2561" "Orientation/MPU6050" "Orientation" "SensorDataPacker"
                        INCLUDE_DIRS "Common" "CO2" "HCHO" "PM" "TempHumid" "TSL2561" "Orientation" "SensorDataPacker"
                        REQUIRES Uart DisplayController
                        PRIV_REQUIRES Config Common I2c Application )