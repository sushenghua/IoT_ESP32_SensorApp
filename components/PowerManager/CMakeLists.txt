idf_component_register( SRCS "PowerManager.cpp"
                        INCLUDE_DIRS "."
                        REQUIRES I2c
                        PRIV_REQUIRES Config Common Adc )