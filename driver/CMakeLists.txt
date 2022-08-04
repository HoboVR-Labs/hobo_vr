cmake_minimum_required(VERSION 3.16)


project(hobo_vr)

set(TARGET_NAME driver_hobovr)
set(CMAKE_CXX_STANDARD 17)
# Prefix all shared libraries with nothing.
set(CMAKE_SHARED_LIBRARY_PREFIX "")
# Prefix all static libraries with nothing.
set(CMAKE_STATIC_LIBRARY_PREFIX "")

IF (EXISTS $ENV{USERPROFILE}/vcpkg)
  include($ENV{USERPROFILE}/vcpkg/scripts/buildsystems/vcpkg.cmake)
  # find_path(SYSTEM_INCLUDE_DIR openvr_driver.h)
  # include_directories(${SYSTEM_INCLUDE_DIR})
endif()

# Set a default build type if none was specified
if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
  message("Setting build type to 'RelWithDebInfo' as none was specified.")
  set(CMAKE_BUILD_TYPE RelWithDebInfo CACHE STRING "Choose the type of build." FORCE)
  # Set the possible values of build type for cmake-gui
  set_property(CACHE CMAKE_BUILD_TYPE PROPERTY STRINGS "Debug" "Release"
    "MinSizeRel" "RelWithDebInfo")
endif()


if (MSVC)
    # all warnings as errors
    add_compile_options(/W4 /WX)
else()
    # all warnings as errors
    add_compile_options(-Wall -Wextra -pedantic -Werror -Wno-missing-braces)
endif()

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O2")

# options
option(INSTALL_X86_DRIVER "Overrides the platform architecture on install, will use bin/{platform}{arch}" OFF)


# Check if 32 or 64 bit system or a 32 bit override.
set(SIZEOF_VOIDP ${CMAKE_SIZEOF_VOID_P})
if(CMAKE_SIZEOF_VOID_P EQUAL 8 AND NOT INSTALL_X86_DRIVER)
  set(HOBOVR_PROCESSOR_ARCH "64")
else()
  set(HOBOVR_PROCESSOR_ARCH "32")
endif()

message(STATUS "Compilation set for ${HOBOVR_PROCESSOR_ARCH}bit architectures.")

# Get platform.
if(${CMAKE_SYSTEM_NAME} MATCHES "Linux")
  set(PLATFORM_NAME "linux")
  set(PLATFORM_SCRIPT_EXT "sh")
  add_definitions(-DLINUX)

  if(HOBOVR_PROCESSOR_ARCH MATCHES "64")
      add_definitions(-DLINUX64)
  endif()
elseif(${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
  message(FATAL_ERROR "OSX is not supported")

elseif(${CMAKE_SYSTEM_NAME} MATCHES "Windows")
  set(PLATFORM_NAME "win")
  set(PLATFORM_SCRIPT_EXT "bat")
  add_definitions(-DWIN)

  # Binaries path for thirdparties are not generics so we try to guess their suffixes.
  set(WINDOWS_PATH_SUFFIXES win${HOBOVR_PROCESSOR_ARCH} Win${HOBOVR_PROCESSOR_ARCH} x${HOBOVR_PROCESSOR_ARCH})
endif()

# set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${hobo_vr_SOURCE_DIR}/../hobovr/bin/${PLATFORM_NAME}${HOBOVR_PROCESSOR_ARCH})

# add_subdirectory(openvr)

set(BUILD_SHARED ON)

# OpenVR API prep
## OpenVR API path
find_library(OPENVR_LIBRARIES
  NAMES
    openvr_api
  PATHS
    # /usr/local/lib
    # /usr/lib
    # lib
    ${hobo_vr_SOURCE_DIR}/openvr/lib
    ${hobo_vr_SOURCE_DIR}/openvr/bin
  PATH_SUFFIXES
    "osx${HOBOVR_PROCESSOR_ARCH}"
    "linux${HOBOVR_PROCESSOR_ARCH}"
    ${WINDOWS_PATH_SUFFIXES}
  NO_DEFAULT_PATH
  NO_CMAKE_FIND_ROOT_PATH
)
set(OPENVR_INCLUDE_DIR ${hobo_vr_SOURCE_DIR}/openvr/headers)

message(NOTICE "openvr libs: ${OPENVR_LIBRARIES}")
message(NOTICE "openvr headers: ${OPENVR_INCLUDE_DIR}")

# shroom dep
add_subdirectory(shroom)
set(SHROOM_INCLUDE_DIR "shroom/include/")

# lazy-sockets dep
add_subdirectory("lazy-sockets")
set(LAZY_SOCKETS_INCLUDE_DIR "lazy-sockets/src/")

# example gaze master plugin
add_subdirectory("gaze_master_plugins")

set(Boost_USE_STATIC_LIBS ON)
find_package(Boost REQUIRED)

# actual build instructions
include_directories(
  ${OPENVR_INCLUDE_DIR}
  ${Boost_INCLUDE_DIR}
  ${SHROOM_INCLUDE_DIR}
  ${LAZY_SOCKETS_INCLUDE_DIR}
  "src/plugins/"
  "src/ref/"
  "src/"
)

file(GLOB driver_hobovr_src
  "src/*.cpp"
  "src/*.h"
  "src/plugins/gaze_master_plugin_interface.h"
  "src/ref/hobovr*.cpp"
  "src/ref/util.cpp"
  "src/ref/*.h"
) # grab all source files

add_library(${TARGET_NAME} SHARED
  ${driver_hobovr_src}
)

target_link_libraries(${TARGET_NAME}
  "${OPENVR_LIBRARIES}"
  shoom
  lazy_sockets
  -lstdc++fs
  ${Boost_DLL_LIBRARY}
  ${CMAKE_DL_LIBS}
)

message(NOTICE "boost_root: ${BOOST_ROOT}|")

install(
  TARGETS
    ${TARGET_NAME}
  DESTINATION
    ${hobo_vr_SOURCE_DIR}/../hobovr/bin/${PLATFORM_NAME}${HOBOVR_PROCESSOR_ARCH}
)

install(
  FILES
    ../scripts/${PLATFORM_NAME}/register_driver.${PLATFORM_SCRIPT_EXT}
    ../scripts/${PLATFORM_NAME}/unregister_driver.${PLATFORM_SCRIPT_EXT}
  DESTINATION
    ${hobo_vr_SOURCE_DIR}/../hobovr/
)



# message(NOTICE "Driver will be installed to:\n${hobo_vr_SOURCE_DIR}/../hobovr/bin/${PLATFORM_NAME}${HOBOVR_PROCESSOR_ARCH}")
# message(WARNING ${CMAKE_SYSTEM_PROCESSOR})
