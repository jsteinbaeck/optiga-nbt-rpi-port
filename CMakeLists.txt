# Version support
cmake_minimum_required(VERSION 3.14)

# project configuration
project(
  optiga-nbt-rpi-port
  VERSION 1.0.0
  DESCRIPTION "OPTIGA Authenticate NBT Raspberry Pi port"
  LANGUAGES C)


# Compiler settings
set(CMAKE_C_STANDARD 99)

# Disable building documentation
set(BUILD_DOCUMENTATION OFF)

# Set the source files
set(SOURCES
	"${CMAKE_CURRENT_SOURCE_DIR}/timer-rpi/src/timer-rpi.c"
	"${CMAKE_CURRENT_SOURCE_DIR}/logger-printf/src/logger-printf.c"
	"${CMAKE_CURRENT_SOURCE_DIR}/logger-printf/src/logger-printf.h"
	"${CMAKE_CURRENT_SOURCE_DIR}/i2c-rpi/src/i2c-rpi.c"
	"${CMAKE_CURRENT_SOURCE_DIR}/i2c-rpi/src/i2c-rpi.h"
)

set(HEADERS
	"${CMAKE_CURRENT_SOURCE_DIR}/logger-printf/include/infineon/logger-printf.h"
	"${CMAKE_CURRENT_SOURCE_DIR}/i2c-rpi/include/infineon/i2c-rpi.h"
)

# ##############################################################################
# Library
# ##############################################################################
add_library(${PROJECT_NAME} ${SOURCES} ${HEADERS})
add_library(Infineon::${PROJECT_NAME} ALIAS ${PROJECT_NAME})

target_include_directories(
  ${PROJECT_NAME}
  PUBLIC "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/i2c-rpi/include>"
  PUBLIC "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/logger-printf/include>"
         "$<INSTALL_INTERFACE:include>")


# Add installation configuration

# ##############################################################################
# Installation
# ##############################################################################
include(CMakePackageConfigHelpers)
include(GNUInstallDirs)

# Main library
install(
  TARGETS ${PROJECT_NAME}
  EXPORT ${PROJECT_NAME}-targets
  RUNTIME DESTINATION "${CMAKE_INSTALL_BINDIR}"
  LIBRARY DESTINATION "${CMAKE_INSTALL_LIBDIR}"
  ARCHIVE DESTINATION "${CMAKE_INSTALL_LIBDIR}")
install(DIRECTORY i2c-rpi/include/infineon DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}")
install(DIRECTORY logger-printf/include/infineon DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}")

# TODO: Add optiga-nbt-lib-c as dependency.

# TODO: CMake files for find_package()

# CMake targets for stuff like target_link_libraries()
install(
  EXPORT ${PROJECT_NAME}-targets
  FILE ${PROJECT_NAME}-targets.cmake
  NAMESPACE Infineon::
  DESTINATION "${CMAKE_INSTALL_LIBDIR}/${PROJECT_NAME}")
export(
  EXPORT ${PROJECT_NAME}-targets
  FILE "${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}-targets.cmake"
  NAMESPACE Infineon::)
export(PACKAGE ${PROJECT_NAME})
