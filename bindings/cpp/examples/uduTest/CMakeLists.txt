cmake_minimum_required(VERSION 3.16)


project(test_poser)

set(TARGET_NAME test_poser)
set(CMAKE_CXX_STANDARD 17)
# Prefix all shared libraries with nothing.
set(CMAKE_SHARED_LIBRARY_PREFIX "")
# Prefix all static libraries with nothing.
set(CMAKE_STATIC_LIBRARY_PREFIX "")

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

# lazy-sockets dep
add_subdirectory("lazy-sockets")
set(LAZY_SOCKETS_INCLUDE_DIR "lazy-sockets/src/")


include_directories(
	"../../../../driver/src/"
  ${LAZY_SOCKETS_INCLUDE_DIR}
)

file(GLOB poser_src
  "./*.cpp"
  "./*.h"
)

add_executable(${TARGET_NAME}
	${poser_src}
)

target_link_libraries(${TARGET_NAME}
  lazy_sockets
  ${CMAKE_DL_LIBS}
)