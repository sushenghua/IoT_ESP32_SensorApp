#
# Main Makefile. This is basically the same as a component makefile.
#
# (Uses default behaviour of compiling all source files in directory, adding 'include' to include path.)


# COMPONENT_PRIV_INCLUDEDIRS=
# COMPONENT_ADD_LDFLAGS=
# COMPONENT_DEPENDS=
# COMPONENT_ADD_LINKER_DEPS=
# COMPONENT_SUBMODULES=

COMPONENT_ADD_INCLUDEDIRS := . Common CO2 HCHO PM TempHumid Orientation/MPU6050 Orientation
# COMPONENT_PRIV_INCLUDEDIRS :=
# COMPONENT_EXTRA_INCLUDES=
COMPONENT_SRCDIRS := Common CO2 PM Orientation/MPU6050 Orientation
# COMPONENT_OBJS=
# COMPONENT_EXTRA_CLEAN=
# COMPONENT_OWNBUILDTARGET=

# CFLAGS=
# CPPFLAGS=
# CXXFLAGS=
