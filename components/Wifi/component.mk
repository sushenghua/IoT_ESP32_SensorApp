#
# Main Makefile. This is basically the same as a component makefile.
#
# (Uses default behaviour of compiling all source files in directory, adding 'include' to include path.)


# embed files from the "certs" directory as binary data symbols in the app
# COMPONENT_EMBED_TXTFILES := wpa2_ca.pem
# COMPONENT_EMBED_TXTFILES += wpa2_client.crt
# COMPONENT_EMBED_TXTFILES += wpa2_client.key

# COMPONENT_PRIV_INCLUDEDIRS=
# COMPONENT_ADD_LDFLAGS=
# COMPONENT_DEPENDS=
# COMPONENT_ADD_LINKER_DEPS=
# COMPONENT_SUBMODULES=

COMPONENT_ADD_INCLUDEDIRS := .
# COMPONENT_PRIV_INCLUDEDIRS :=
# COMPONENT_EXTRA_INCLUDES=
# COMPONENT_SRCDIRS := 
# COMPONENT_OBJS=
# COMPONENT_EXTRA_CLEAN=
# COMPONENT_OWNBUILDTARGET=

# CFLAGS=
# CPPFLAGS=
# CXXFLAGS=
