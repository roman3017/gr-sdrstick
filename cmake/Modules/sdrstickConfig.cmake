INCLUDE(FindPkgConfig)
PKG_CHECK_MODULES(PC_SDRSTICK sdrstick)

FIND_PATH(
    SDRSTICK_INCLUDE_DIRS
    NAMES sdrstick/api.h
    HINTS $ENV{SDRSTICK_DIR}/include
        ${PC_SDRSTICK_INCLUDEDIR}
    PATHS ${CMAKE_INSTALL_PREFIX}/include
          /usr/local/include
          /usr/include
)

FIND_LIBRARY(
    SDRSTICK_LIBRARIES
    NAMES gnuradio-sdrstick
    HINTS $ENV{SDRSTICK_DIR}/lib
        ${PC_SDRSTICK_LIBDIR}
    PATHS ${CMAKE_INSTALL_PREFIX}/lib
          ${CMAKE_INSTALL_PREFIX}/lib64
          /usr/local/lib
          /usr/local/lib64
          /usr/lib
          /usr/lib64
)

INCLUDE(FindPackageHandleStandardArgs)
FIND_PACKAGE_HANDLE_STANDARD_ARGS(SDRSTICK DEFAULT_MSG SDRSTICK_LIBRARIES SDRSTICK_INCLUDE_DIRS)
MARK_AS_ADVANCED(SDRSTICK_LIBRARIES SDRSTICK_INCLUDE_DIRS)

