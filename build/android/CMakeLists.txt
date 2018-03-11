cmake_minimum_required(VERSION 3.1)

project(minetest)
set(PROJECT_NAME_CAPITALIZED "Minetest")

set(CMAKE_CXX_STANDARD 11)
set(PROJECT_SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../../src/)
set(PROJECT_BINARY_DIR ${CMAKE_CURRENT_SOURCE_DIR}/cmakebuild/)
set(BUILD_CLIENT 1)

message(WARN "Ndk ${ANDROID_NDK}")
if (${ANDROID_ABI} STREQUAL "x86_64")
	include_directories(${ANDROID_SYSROOT}/usr/include/x86_64-linux-android)
elseif (${ANDROID_ABI} STREQUAL "x86")
	include_directories(${ANDROID_SYSROOT}/usr/include/i686-linux-android)
elseif (${ANDROID_ABI} STREQUAL "arm64-v8a")
	include_directories(${ANDROID_SYSROOT}/usr/include/aarch64-linux-android)
elseif (${ANDROID_ABI} STREQUAL "armeabi-v7a")
	include_directories(${ANDROID_SYSROOT}/usr/include/arm-linux-androideabi)
endif()

include_directories(
	${ANDROID_NDK}/sources/android/native_app_glue
	${CMAKE_CURRENT_SOURCE_DIR}/deps/irrlicht/include
	${CMAKE_CURRENT_SOURCE_DIR}/deps/gmp
	${CMAKE_CURRENT_SOURCE_DIR}/deps/libiconv/include
	${CMAKE_CURRENT_SOURCE_DIR}/../../lib/jsoncpp
	${CMAKE_CURRENT_SOURCE_DIR}/../../lib/lua/src
	${PROJECT_SOURCE_DIR}
	${PROJECT_SOURCE_DIR}/script
	${PROJECT_BINARY_DIR})

add_subdirectory(${PROJECT_SOURCE_DIR}/client ${PROJECT_BINARY_DIR}/client)
add_subdirectory(${PROJECT_SOURCE_DIR}/database ${PROJECT_BINARY_DIR}/database)
add_subdirectory(${PROJECT_SOURCE_DIR}/gui ${PROJECT_BINARY_DIR}/gui)
add_subdirectory(${PROJECT_SOURCE_DIR}/irrlicht_changes ${PROJECT_BINARY_DIR}/irrlicht_changes)
add_subdirectory(${PROJECT_SOURCE_DIR}/mapgen ${PROJECT_BINARY_DIR}/mapgen)
add_subdirectory(${PROJECT_SOURCE_DIR}/network ${PROJECT_BINARY_DIR}/network)
add_subdirectory(${PROJECT_SOURCE_DIR}/script ${PROJECT_BINARY_DIR}/script)
add_subdirectory(${PROJECT_SOURCE_DIR}/threading ${PROJECT_BINARY_DIR}/threading)
add_subdirectory(${PROJECT_SOURCE_DIR}/util ${PROJECT_BINARY_DIR}/util)

configure_file(
	"${PROJECT_SOURCE_DIR}/cmake_config.h.in"
	"${PROJECT_BINARY_DIR}/cmake_config.h"
)

add_library(minetest
	${client_network_SRCS}
	${client_SCRIPT_SRCS}
	${common_network_SRCS}
	${common_SCRIPT_SRCS}
	${JTHREAD_SRCS}
	${UTIL_SRCS})

target_link_libraries(minetest)
