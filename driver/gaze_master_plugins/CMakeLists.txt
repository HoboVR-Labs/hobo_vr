cmake_minimum_required(VERSION 3.16)

project(gaze_master_example_plugin)

add_library(gaze_logger SHARED
	"../src/plugins/gaze_master_plugin_interface.h"
	"src/plugin.cpp"
)

target_include_directories(gaze_logger PRIVATE
	"../src/plugins"
	"../src/"
)

install(
  TARGETS
    gaze_logger
  DESTINATION
    ${hobo_vr_SOURCE_DIR}/../hobovr/bin/${PLATFORM_NAME}${HOBOVR_PROCESSOR_ARCH}
)