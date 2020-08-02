#
# Main Makefile. This is basically the same as a component makefile.
#
# (Uses default behaviour of compiling all source files in directory, adding 'include' to include path.)

# embed files from the "certs" directory as binary data symbols in the app
# COMPONENT_EMBED_TXTFILES := certs/ca.crt
# COMPONENT_EMBED_TXTFILES += certs/ca.strdiv
# COMPONENT_EMBED_TXTFILES += certs/ca.key

# COMPONENT_PRIV_INCLUDEDIRS=
# COMPONENT_ADD_LDFLAGS=
# COMPONENT_DEPENDS=
# COMPONENT_ADD_LINKER_DEPS=
# COMPONENT_SUBMODULES=

COMPONENT_ADD_INCLUDEDIRS := . mongoose certs
# COMPONENT_PRIV_INCLUDEDIRS :=
# COMPONENT_EXTRA_INCLUDES=
COMPONENT_SRCDIRS := . mongoose
# COMPONENT_OBJS=
# COMPONENT_EXTRA_CLEAN=
# COMPONENT_OWNBUILDTARGET=

# CFLAGS += -DMG_ENABLE_SSL -DMG_SSL_IF=MG_SSL_IF_MBEDTLS
CPPFLAGS += -DMG_ENABLE_SSL -DMG_SSL_IF=MG_SSL_IF_MBEDTLS -DMONGOOSE_ESP32_ADAPTION
# CPPFLAGS += -DMBEDTLS_KEY_EXCHANGE__SOME__PSK_ENABLED
# CPPFLAGS += -DMG_ENABLE_SSL -DMG_SSL_IF=MG_SSL_IF_MBEDTLS -DMONGOOSE_ESP32_ADAPTION -DMG_ENABLE_DEBUG
# CXXFLAGS += -DMG_ENABLE_SSL 