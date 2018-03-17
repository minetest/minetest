cmake_minimum_required(VERSION 3.1)

project(minetest)
set(PROJECT_NAME_CAPITALIZED "Minetest")

set(CMAKE_C STANDARD 11)
set(CMAKE_CXX_STANDARD 11)
set(PROJECT_SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../src/)
set(PROJECT_BINARY_DIR ${CMAKE_CURRENT_SOURCE_DIR}/cmakebuild/)
set(OPENSSL_INCLUDE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/deps/openssl/include)
set(OPENSSL_LIBRARIES ${PROJECT_BINARY_DIR}/lib/ssl)

# Build options
set(BUILD_CLIENT 1)
set(USE_FREETYPE 1)
set(USE_CURL 1)
set(USE_SOUND 1)
set(_IRR_ANDROID_PLATFORM_1)
set(HAVE_TOUCHSCREENGUI 1)

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
	${CMAKE_CURRENT_SOURCE_DIR}/deps/openal/include
	${CMAKE_CURRENT_SOURCE_DIR}/deps/sqlite
	${CMAKE_CURRENT_SOURCE_DIR}/../lib/jsoncpp
	${CMAKE_CURRENT_SOURCE_DIR}/../lib/lua/src
	${PROJECT_SOURCE_DIR}
	${PROJECT_SOURCE_DIR}/script
	${PROJECT_BINARY_DIR}
	${PROJECT_BINARY_DIR}/lib)

# Library deps
add_subdirectory(${PROJECT_SOURCE_DIR}/../lib/jsoncpp ${PROJECT_BINARY_DIR}/lib/jsoncpp)
add_subdirectory(${PROJECT_SOURCE_DIR}/../lib/gmp ${PROJECT_BINARY_DIR}/lib/gmp)
add_subdirectory(${PROJECT_SOURCE_DIR}/../lib/lua ${PROJECT_BINARY_DIR}/lib/lua)
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/deps/openal-soft ${PROJECT_BINARY_DIR}/lib/openal)
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/deps/openssl ${PROJECT_BINARY_DIR}/lib/openssl)
#add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/deps/curl ${PROJECT_BINARY_DIR}/lib/curl)
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/deps/sqlite ${PROJECT_BINARY_DIR}/lib/sqlite)

# Our sources
add_subdirectory(${PROJECT_SOURCE_DIR}/client ${PROJECT_BINARY_DIR}/client)
add_subdirectory(${PROJECT_SOURCE_DIR}/database ${PROJECT_BINARY_DIR}/database)
add_subdirectory(${PROJECT_SOURCE_DIR}/gui ${PROJECT_BINARY_DIR}/gui)
add_subdirectory(${PROJECT_SOURCE_DIR}/irrlicht_changes ${PROJECT_BINARY_DIR}/irrlicht_changes)
add_subdirectory(${PROJECT_SOURCE_DIR}/mapgen ${PROJECT_BINARY_DIR}/mapgen)
add_subdirectory(${PROJECT_SOURCE_DIR}/network ${PROJECT_BINARY_DIR}/network)
add_subdirectory(${PROJECT_SOURCE_DIR}/script ${PROJECT_BINARY_DIR}/script)
add_subdirectory(${PROJECT_SOURCE_DIR}/threading ${PROJECT_BINARY_DIR}/server)
add_subdirectory(${PROJECT_SOURCE_DIR}/threading ${PROJECT_BINARY_DIR}/threading)
add_subdirectory(${PROJECT_SOURCE_DIR}/util ${PROJECT_BINARY_DIR}/util)

set(common_SRCS
	${database_SRCS}
	${mapgen_SRCS}
	${server_SRCS}
	${PROJECT_SOURCE_DIR}/ban.cpp
	${PROJECT_SOURCE_DIR}/chat.cpp
	${PROJECT_SOURCE_DIR}/clientiface.cpp
	${PROJECT_SOURCE_DIR}/collision.cpp
	${PROJECT_SOURCE_DIR}/content_mapnode.cpp
	${PROJECT_SOURCE_DIR}/content_nodemeta.cpp
	${PROJECT_SOURCE_DIR}/content_sao.cpp
	${PROJECT_SOURCE_DIR}/convert_json.cpp
	${PROJECT_SOURCE_DIR}/craftdef.cpp
	${PROJECT_SOURCE_DIR}/debug.cpp
	${PROJECT_SOURCE_DIR}/defaultsettings.cpp
	${PROJECT_SOURCE_DIR}/emerge.cpp
	${PROJECT_SOURCE_DIR}/environment.cpp
	${PROJECT_SOURCE_DIR}/face_position_cache.cpp
	${PROJECT_SOURCE_DIR}/filesys.cpp
	${PROJECT_SOURCE_DIR}/genericobject.cpp
	${PROJECT_SOURCE_DIR}/gettext.cpp
	${PROJECT_SOURCE_DIR}/httpfetch.cpp
	${PROJECT_SOURCE_DIR}/inventory.cpp
	${PROJECT_SOURCE_DIR}/inventorymanager.cpp
	${PROJECT_SOURCE_DIR}/itemdef.cpp
	${PROJECT_SOURCE_DIR}/itemstackmetadata.cpp
	${PROJECT_SOURCE_DIR}/light.cpp
	${PROJECT_SOURCE_DIR}/log.cpp
	${PROJECT_SOURCE_DIR}/map.cpp
	${PROJECT_SOURCE_DIR}/map_settings_manager.cpp
	${PROJECT_SOURCE_DIR}/mapblock.cpp
	${PROJECT_SOURCE_DIR}/mapnode.cpp
	${PROJECT_SOURCE_DIR}/mapsector.cpp
	${PROJECT_SOURCE_DIR}/metadata.cpp
	${PROJECT_SOURCE_DIR}/modchannels.cpp
	${PROJECT_SOURCE_DIR}/mods.cpp
	${PROJECT_SOURCE_DIR}/nameidmapping.cpp
	${PROJECT_SOURCE_DIR}/nodedef.cpp
	${PROJECT_SOURCE_DIR}/nodemetadata.cpp
	${PROJECT_SOURCE_DIR}/nodetimer.cpp
	${PROJECT_SOURCE_DIR}/noise.cpp
	${PROJECT_SOURCE_DIR}/objdef.cpp
	${PROJECT_SOURCE_DIR}/object_properties.cpp
	${PROJECT_SOURCE_DIR}/pathfinder.cpp
	${PROJECT_SOURCE_DIR}/player.cpp
	${PROJECT_SOURCE_DIR}/porting.cpp
	${PROJECT_SOURCE_DIR}/profiler.cpp
	${PROJECT_SOURCE_DIR}/quicktune.cpp
	${PROJECT_SOURCE_DIR}/raycast.cpp
	${PROJECT_SOURCE_DIR}/reflowscan.cpp
	${PROJECT_SOURCE_DIR}/remoteplayer.cpp
	${PROJECT_SOURCE_DIR}/rollback.cpp
	${PROJECT_SOURCE_DIR}/rollback_interface.cpp
	${PROJECT_SOURCE_DIR}/serialization.cpp
	${PROJECT_SOURCE_DIR}/server.cpp
	${PROJECT_SOURCE_DIR}/serverenvironment.cpp
	${PROJECT_SOURCE_DIR}/serverlist.cpp
	${PROJECT_SOURCE_DIR}/serverobject.cpp
	${PROJECT_SOURCE_DIR}/settings.cpp
	${PROJECT_SOURCE_DIR}/sound.cpp
	${PROJECT_SOURCE_DIR}/staticobject.cpp
	${PROJECT_SOURCE_DIR}/subgame.cpp
	${PROJECT_SOURCE_DIR}/terminal_chat_console.cpp
	${PROJECT_SOURCE_DIR}/tileanimation.cpp
	${PROJECT_SOURCE_DIR}/tool.cpp
	${PROJECT_SOURCE_DIR}/translation.cpp
	${PROJECT_SOURCE_DIR}/version.cpp
	${PROJECT_SOURCE_DIR}/voxel.cpp
	${PROJECT_SOURCE_DIR}/voxelalgorithms.cpp
	${PROJECT_SOURCE_DIR}/hud.cpp
	${common_network_SRCS}
	${JTHREAD_SRCS}
	${common_SCRIPT_SRCS}
	${UTIL_SRCS})

set(client_SRCS
	${client_SRCS}
	${common_SRCS}
	${gui_SRCS}
	${sound_SRCS}
	${client_network_SRCS}
	${client_irrlicht_changes_SRCS}
	${PROJECT_SOURCE_DIR}/camera.cpp
	${PROJECT_SOURCE_DIR}/client.cpp
	${PROJECT_SOURCE_DIR}/clientenvironment.cpp
	${PROJECT_SOURCE_DIR}/clientmap.cpp
	${PROJECT_SOURCE_DIR}/clientmedia.cpp
	${PROJECT_SOURCE_DIR}/clientobject.cpp
	${PROJECT_SOURCE_DIR}/clouds.cpp
	${PROJECT_SOURCE_DIR}/content_cao.cpp
	${PROJECT_SOURCE_DIR}/content_cso.cpp
	${PROJECT_SOURCE_DIR}/content_mapblock.cpp
	${PROJECT_SOURCE_DIR}/convert_json.cpp
	${PROJECT_SOURCE_DIR}/filecache.cpp
	${PROJECT_SOURCE_DIR}/fontengine.cpp
	${PROJECT_SOURCE_DIR}/game.cpp
	${PROJECT_SOURCE_DIR}/guiscalingfilter.cpp
	${PROJECT_SOURCE_DIR}/hud.cpp
	${PROJECT_SOURCE_DIR}/imagefilters.cpp
	${PROJECT_SOURCE_DIR}/keycode.cpp
	${PROJECT_SOURCE_DIR}/localplayer.cpp
	${PROJECT_SOURCE_DIR}/main.cpp
	${PROJECT_SOURCE_DIR}/mapblock_mesh.cpp
	${PROJECT_SOURCE_DIR}/mesh.cpp
	${PROJECT_SOURCE_DIR}/mesh_generator_thread.cpp
	${PROJECT_SOURCE_DIR}/minimap.cpp
	${PROJECT_SOURCE_DIR}/particles.cpp
	${PROJECT_SOURCE_DIR}/shader.cpp
	${PROJECT_SOURCE_DIR}/sky.cpp
	${PROJECT_SOURCE_DIR}/wieldmesh.cpp
	${client_SCRIPT_SRCS})
list(SORT client_SRCS)

configure_file(
	"${PROJECT_SOURCE_DIR}/cmake_config.h.in"
	"${PROJECT_BINARY_DIR}/cmake_config.h"
)

add_library(minetest SHARED ${client_SRCS})
add_dependencies(minetest crypto ssl jsoncpp gmp OpenAL lua sqlite3)

target_link_libraries(minetest crypto ssl gmp openal jsoncpp lua sqlite3)
