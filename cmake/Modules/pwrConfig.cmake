INCLUDE(FindPkgConfig)
PKG_CHECK_MODULES(PC_PWR pwr)

FIND_PATH(
    PWR_INCLUDE_DIRS
    NAMES pwr/api.h
    HINTS $ENV{PWR_DIR}/include
        ${PC_PWR_INCLUDEDIR}
    PATHS ${CMAKE_INSTALL_PREFIX}/include
          /usr/local/include
          /usr/include
)

FIND_LIBRARY(
    PWR_LIBRARIES
    NAMES gnuradio-pwr
    HINTS $ENV{PWR_DIR}/lib
        ${PC_PWR_LIBDIR}
    PATHS ${CMAKE_INSTALL_PREFIX}/lib
          ${CMAKE_INSTALL_PREFIX}/lib64
          /usr/local/lib
          /usr/local/lib64
          /usr/lib
          /usr/lib64
)

INCLUDE(FindPackageHandleStandardArgs)
FIND_PACKAGE_HANDLE_STANDARD_ARGS(PWR DEFAULT_MSG PWR_LIBRARIES PWR_INCLUDE_DIRS)
MARK_AS_ADVANCED(PWR_LIBRARIES PWR_INCLUDE_DIRS)

