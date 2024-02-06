cmake_minimum_required(VERSION 3.9)
project(vamos-monitors LANGUAGES C VERSION 0.1)

set(CMAKE_VERBOSE_MAKEFILE 1)
set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)

find_package(vamos-buffers REQUIRED)

if(NOT CMAKE_BUILD_TYPE)
	message(STATUS "Build type not set. Setting buffer's build type.")
	set(CMAKE_BUILD_TYPE ${vamos-buffers_BUILD_TYPE} CACHE STRING "" FORCE)
endif()

if (vamos-buffers_OPTION_ENABLE_IPO)
	message(STATUS "Enabling IPO")
        set(CMAKE_INTERPROCEDURAL_OPTIMIZATION TRUE)
endif()

include_directories(${vamos_buffers_INCLUDE_DIRS})

get_filename_component(vamos-buffers_DIR ${vamos-buffers_DIR} ABSOLUTE)
get_target_property(vamos-buffers_vamos-buffers-lib vamos-buffers-lib LOCATION)
get_target_property(vamos-buffers_vamos-buffers-ringbuf vamos-buffers-ringbuf LOCATION)
get_target_property(vamos-buffers_vamos-buffers-shmbuf vamos-buffers-shmbuf LOCATION)
get_target_property(vamos-buffers_vamos-buffers-streams vamos-buffers-streams LOCATION)
get_filename_component(vamos-buffers_LIBRARIES_DIRS_lib ${vamos-buffers_vamos-buffers-lib} DIRECTORY)
get_filename_component(vamos-buffers_LIBRARIES_DIRS_core ${vamos-buffers_vamos-buffers-ringbuf} DIRECTORY)
get_filename_component(vamos-buffers_LIBRARIES_DIRS_shmbuf ${vamos-buffers_vamos-buffers-shmbuf} DIRECTORY)
get_filename_component(vamos-buffers_LIBRARIES_DIRS_streams ${vamos-buffers_vamos-buffers-streams} DIRECTORY)

add_library(vamos-monitors-utils STATIC utils.c)

add_executable(monitor-generic monitor-generic.c)
target_link_libraries(monitor-generic PRIVATE vamos-buffers-shamon vamos-buffers-monitor vamos-monitors-utils pthread)

add_executable(monitor-regex monitor-regex.c)
target_link_libraries(monitor-regex PRIVATE vamos-buffers-shamon vamos-buffers-monitor vamos-monitors-utils pthread)

add_executable(monitor-drregex monitor-drregex.c)
target_link_libraries(monitor-drregex PRIVATE vamos-buffers-shamon vamos-buffers-monitor vamos-monitors-utils pthread)

add_executable(monitor-consume monitor-consume.c)
target_link_libraries(monitor-consume PRIVATE vamos-buffers-shamon vamos-buffers-monitor vamos-monitors-utils pthread)

