idf_component_register( SRC_DIRS "." "mongoose"
                        INCLUDE_DIRS "."
                        PRIV_REQUIRES  mbedtls Config Common Wifi Application SNTP )

target_compile_options(${COMPONENT_LIB} PRIVATE -Wno-unused-variable -DMG_ENABLE_SSL -DMG_SSL_IF=MG_SSL_IF_MBEDTLS -DMONGOOSE_ESP32_ADAPTION)
