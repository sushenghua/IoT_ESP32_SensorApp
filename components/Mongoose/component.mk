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

# CFLAGS += -DMG_ENABLE_SSL=1
# CPPFLAGS += -DMG_ENABLE_SSL
# CXXFLAGS += -DMG_ENABLE_SSL
