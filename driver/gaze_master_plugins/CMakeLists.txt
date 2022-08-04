cmake_minimum_required(VERSION 3.16)

project(gaze_master_example_plugin)

set(Boost_USE_STATIC_LIBS ON)
find_package(Boost REQUIRED)

add_library(gaze_logger SHARED
	"../src/plugins/gaze_master_plugin_interface.h"
	"src/plugin.cpp"
)

target_include_directories(gaze_logger PRIVATE
	"../src/plugins"
	"../src/"
	${Boost_INCLUDE_DIRS}
)

target_link_libraries(gaze_logger PRIVATE
	${Boost_DLL_LIBRARY}
	${CMAKE_DL_LIBS}
)

install(
  TARGETS
    gaze_logger
  DESTINATION
    ${hobo_vr_SOURCE_DIR}/../hobovr/bin/${PLATFORM_NAME}${HOBOVR_PROCESSOR_ARCH}
)