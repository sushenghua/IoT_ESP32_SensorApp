idf_component_register( SRC_DIRS "Common" "ILI9341"
                        INCLUDE_DIRS "Common" "ILI9341"
                        REQUIRES Spi
                        PRIV_REQUIRES LedController )