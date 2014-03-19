project(minetest)
cmake_minimum_required( VERSION 2.6 )

INCLUDE(CheckCSourceRuns)

# Set some random things default to not being visible in the GUI
mark_as_advanced(EXECUTABLE_OUTPUT_PATH LIBRARY_OUTPUT_PATH)
mark_as_advanced(SQLITE3_INCLUDE_DIR SQLITE3_LIBRARY)
mark_as_advanced(JSON_INCLUDE_DIR JSON_LIBRARY)

option(ENABLE_CURL "Enable cURL support for fetching media" 1)

if (NOT ENABLE_CURL)
	mark_as_advanced(CLEAR CURL_LIBRARY CURL_INCLUDE_DIR)
endif(NOT ENABLE_CURL)

if( ENABLE_CURL )
	find_package(CURL)
endif( ENABLE_CURL )
set(USE_CURL 0)
if (CURL_FOUND AND ENABLE_CURL)
	message(STATUS "cURL support enabled")
	set(USE_CURL 1)
endif(CURL_FOUND AND ENABLE_CURL)

# user-visible option to enable/disable gettext usage
OPTION(ENABLE_GETTEXT "Use GetText for internationalization" 0)

# this is only set to 1 if gettext is enabled _and_ available
set(USE_GETTEXT 0)

if(ENABLE_GETTEXT)
	find_package(GettextLib)
else()
	MARK_AS_ADVANCED(GETTEXT_ICONV_DLL GETTEXT_INCLUDE_DIR GETTEXT_LIBRARY GETTEXT_MSGFMT)
endif()

if(GETTEXT_FOUND AND ENABLE_GETTEXT)
	message(STATUS "gettext include path: ${GETTEXT_INCLUDE_DIR}")
	message(STATUS "gettext msgfmt path: ${GETTEXT_MSGFMT}")
	if(WIN32)
		message(STATUS "gettext library: ${GETTEXT_LIBRARY}")
		message(STATUS "gettext dll: ${GETTEXT_DLL}")
		message(STATUS "gettext iconv dll: ${GETTEXT_ICONV_DLL}")
	endif()
	set(USE_GETTEXT 1)
	message(STATUS "GetText enabled; locales found: ${GETTEXT_AVAILABLE_LOCALES}")
elseif(GETTEXT_FOUND AND NOT ENABLE_GETTEXT)
	MESSAGE(STATUS "GetText found but disabled;")
else(GETTEXT_FOUND AND ENABLE_GETTEXT)
	message(STATUS "GetText disabled")
endif(GETTEXT_FOUND AND ENABLE_GETTEXT)

# user visible option to enable/disable sound
OPTION(ENABLE_SOUND "Enable sound" ON)

# this is only set to 1 if sound is enabled _and_ available
set(USE_SOUND 0)
set(SOUND_PROBLEM 0)

if(ENABLE_SOUND AND BUILD_CLIENT)
	# Sound libraries
	find_package(OpenAL)
	find_package(Vorbis)
	if(NOT OPENAL_FOUND)
		message(STATUS "Sound enabled, but OpenAL not found!")
		set(SOUND_PROBLEM 1)
		MARK_AS_ADVANCED(CLEAR OPENAL_LIBRARY OPENAL_INCLUDE_DIR)
	endif()
	if(NOT VORBIS_FOUND)
		message(STATUS "Sound enabled, but Vorbis libraries not found!")
		set(SOUND_PROBLEM 1)
		MARK_AS_ADVANCED(CLEAR OGG_INCLUDE_DIR VORBIS_INCLUDE_DIR OGG_LIBRARY VORBIS_LIBRARY VORBISFILE_LIBRARY)
	endif()
	if(OPENAL_FOUND AND VORBIS_FOUND)
		set(USE_SOUND 1)
		message(STATUS "Sound enabled")
	endif()
endif(ENABLE_SOUND AND BUILD_CLIENT)

if(SOUND_PROBLEM)
	message(FATAL_ERROR "Sound enabled, but cannot be used.\n"
		"To continue, either fill in the required paths or disable sound. (-DENABLE_SOUND=0)")
endif()
if(USE_SOUND)
	set(sound_SRCS sound_openal.cpp)
	set(SOUND_INCLUDE_DIRS
		${OPENAL_INCLUDE_DIR}
		${VORBIS_INCLUDE_DIR}
		${OGG_INCLUDE_DIR}
		)
	set(SOUND_LIBRARIES
		${OPENAL_LIBRARY}
		${VORBIS_LIBRARIES}
		)
endif()

option(ENABLE_FREETYPE "Enable freetype2 (truetype fonts and basic unicode support)" OFF)
set(USE_FREETYPE 0)
if(ENABLE_FREETYPE)
	set(USE_FREETYPE 1)
endif(ENABLE_FREETYPE)

if(NOT MSVC)
	set(USE_GPROF 0 CACHE BOOL "Use -pg flag for g++")
endif()

# Use cmake_config.h
add_definitions ( -DUSE_CMAKE_CONFIG_H )

if(WIN32)
	# Windows
	if(MSVC) # MSVC Specifics
		# Surpress some useless warnings
		add_definitions ( /D "_CRT_SECURE_NO_DEPRECATE" /W1 )
	else() # Probably MinGW = GCC
		set(PLATFORM_LIBS ws2_32.lib)
	endif()
	# Zlib stuff
	set(ZLIB_INCLUDE_DIR "${PROJECT_SOURCE_DIR}/../../zlib/zlib-1.2.5"
			CACHE PATH "Zlib include directory")
	set(ZLIB_LIBRARIES "${PROJECT_SOURCE_DIR}/../../zlib125dll/dll32/zlibwapi.lib"
			CACHE FILEPATH "Path to zlibwapi.lib")
	set(ZLIB_DLL "${PROJECT_SOURCE_DIR}/../../zlib125dll/dll32/zlibwapi.dll"
			CACHE FILEPATH "Path to zlibwapi.dll (for installation)")
	set(IRRLICHT_SOURCE_DIR "${PROJECT_SOURCE_DIR}/../../irrlicht-1.7.2"
			CACHE PATH "irrlicht dir")
	if(USE_FREETYPE)
		set(FREETYPE_INCLUDE_DIR_ft2build "${PROJECT_SOURCE_DIR}/../../freetype2/include/"
				CACHE PATH "freetype include dir")
		set(FREETYPE_INCLUDE_DIR_freetype2 "${PROJECT_SOURCE_DIR}/../../freetype2/include/freetype"
				CACHE PATH "freetype include dir")
		set(FREETYPE_LIBRARY "${PROJECT_SOURCE_DIR}/../../freetype2/objs/win32/vc2005/freetype247.lib"
				CACHE FILEPATH "Path to freetype247.lib")
	endif(USE_FREETYPE)
	set(MINGWM10_DLL ""
			CACHE FILEPATH "Path to mingwm10.dll (for installation)")
	if(ENABLE_SOUND)
		set(OPENAL_DLL "" CACHE FILEPATH "Path to OpenAL32.dll for installation (optional)")
		set(OGG_DLL "" CACHE FILEPATH "Path to libogg.dll for installation (optional)")
		set(VORBIS_DLL "" CACHE FILEPATH "Path to libvorbis.dll for installation (optional)")
		set(VORBISFILE_DLL "" CACHE FILEPATH "Path to libvorbisfile.dll for installation (optional)")
	endif()
else()
	# Unix probably
	if(BUILD_CLIENT)
		find_package(X11 REQUIRED)
		find_package(OpenGL REQUIRED)
		find_package(JPEG REQUIRED)
		find_package(BZip2 REQUIRED)
		find_package(PNG REQUIRED)
		if(APPLE)
			FIND_LIBRARY(CARBON_LIB Carbon)
			FIND_LIBRARY(COCOA_LIB Cocoa)
			FIND_LIBRARY(IOKIT_LIB IOKit)
			mark_as_advanced(
				CARBON_LIB
				COCOA_LIB
				IOKIT_LIB
			)
			SET(CLIENT_PLATFORM_LIBS ${CLIENT_PLATFORM_LIBS} ${CARBON_LIB} ${COCOA_LIB} ${IOKIT_LIB})
		endif(APPLE)
	endif(BUILD_CLIENT)
	find_package(ZLIB REQUIRED)
	set(PLATFORM_LIBS -lpthread -lrt ${CMAKE_DL_LIBS})
	#set(CLIENT_PLATFORM_LIBS -lXxf86vm)
	# This way Xxf86vm is found on OpenBSD too
	find_library(XXF86VM_LIBRARY Xxf86vm)
	mark_as_advanced(XXF86VM_LIBRARY)
	set(CLIENT_PLATFORM_LIBS ${CLIENT_PLATFORM_LIBS} ${XXF86VM_LIBRARY})
endif()

find_package(Sqlite3 REQUIRED)
find_package(Json REQUIRED)

option(ENABLE_GLES "Enable OpenGL ES support" 0)
mark_as_advanced(ENABLE_GLES)
if(ENABLE_GLES)
	find_package(OpenGLES2)
endif(ENABLE_GLES)

if(USE_FREETYPE)
	if(UNIX)
		include(FindPkgConfig)
		if(PKG_CONFIG_FOUND)
			pkg_check_modules(FREETYPE QUIET freetype2)
			if(FREETYPE_FOUND)
				SET(FREETYPE_PKGCONFIG_FOUND TRUE)
				SET(FREETYPE_LIBRARY ${FREETYPE_LIBRARIES})
				# because cmake is idiotic
				string(REPLACE ";" " " FREETYPE_CFLAGS_STR ${FREETYPE_CFLAGS})
				string(REPLACE ";" " " FREETYPE_LDFLAGS_STR ${FREETYPE_LDFLAGS})
			endif(FREETYPE_FOUND)
		endif(PKG_CONFIG_FOUND)
	endif(UNIX)
	if(NOT FREETYPE_FOUND)
		find_package(Freetype REQUIRED)
	endif(NOT FREETYPE_FOUND)
	set(CGUITTFONT_INCLUDE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/cguittfont")
	set(CGUITTFONT_LIBRARY cguittfont)
endif(USE_FREETYPE)

if (NOT DISABLE_LUAJIT)
	find_library(LUA_LIBRARY luajit
			NAMES luajit-5.1)
	find_path(LUA_INCLUDE_DIR luajit.h
		NAMES luajit.h
		PATH_SUFFIXES luajit-2.0)
	message (STATUS "LuaJIT library: ${LUA_LIBRARY}")
	message (STATUS "LuaJIT headers: ${LUA_INCLUDE_DIR}")
else (NOT ${DISABLE_LUAJIT} MATCHES "1")
	message (STATUS "LuaJIT detection disabled! (DISABLE_LUAJIT=1)")
	set(LUA_LIBRARY "")
	set(LUA_INCLUDE_DIR "")
endif (NOT DISABLE_LUAJIT)

set(USE_LUAJIT 0)
if(LUA_LIBRARY AND LUA_INCLUDE_DIR)
	message (STATUS "LuaJIT found, checking for broken versions...")
	if(CMAKE_CROSSCOMPILING)
		message(WARNING "Cross-compiling enabled, assuming LuaJIT is not broken")
		set(VALID_LUAJIT_VERSION 1)
	else(CMAKE_CROSSCOMPILING)
		set(BACKUP_REQUIRED_INCS CMAKE_REQUIRED_INCLUDES)
		set(CMAKE_REQUIRED_INCLUDES "${CMAKE_REQUIRED_INCLUDES} ${LUA_INCLUDE_DIR}")
		CHECK_C_SOURCE_RUNS("
			#include <luajit.h>
			#include <stdio.h>
			#include <string.h>

			#define ARRAYSIZE(a) (sizeof(a) / sizeof((a)[0]))

			static char *broken_luajit_versions[] = {
					\"LuaJIT 2.0.0-beta7\",
					\"LuaJIT 2.0.0-beta6\",
					\"LuaJIT 2.0.0-beta5\",
					\"LuaJIT 2.0.0-beta4\",
					\"LuaJIT 2.0.0-beta3\",
					\"LuaJIT 2.0.0-beta2\",
					\"LuaJIT 2.0.0-beta1\"
			};

			int main(int argc, char *argv[]) {
				unsigned int i;
				for (i = 0; i < ARRAYSIZE(broken_luajit_versions); i++) {
					if (strcmp(LUAJIT_VERSION, broken_luajit_versions[i]) == 0) {
						return 1;
					}
				}
				return 0;
			}
			"
			VALID_LUAJIT_VERSION)
		set(CMAKE_REQUIRED_INCLUDES BACKUP_REQUIRED_INCS)
	endif(CMAKE_CROSSCOMPILING)
	if (VALID_LUAJIT_VERSION)
		message (STATUS "LuaJIT version ok")
		set(USE_LUAJIT 1)
	else (VALID_LUAJIT_VERSION)
		message (STATUS "LuaJIT versions till 2.0.0beta7 known to be broken, update to at least beta8")
		set(USE_LUAJIT 0)
	endif (VALID_LUAJIT_VERSION)
endif (LUA_LIBRARY AND LUA_INCLUDE_DIR)

if(NOT USE_LUAJIT)
	message (STATUS "LuaJIT not found, using bundled Lua.")
	set(LUA_INCLUDE_DIR "${PROJECT_SOURCE_DIR}/lua/src")
	set(LUA_LIBRARY "lua")
	add_subdirectory(lua)
endif(NOT USE_LUAJIT)

mark_as_advanced(LUA_LIBRARY)
mark_as_advanced(LUA_INCLUDE_DIR)

set(USE_LEVELDB 0)

OPTION(ENABLE_LEVELDB "Enable LevelDB backend")

if(ENABLE_LEVELDB)
	find_library(LEVELDB_LIBRARY leveldb)
	find_path(LEVELDB_INCLUDE_DIR db.h PATH_SUFFIXES leveldb)
	message (STATUS "LevelDB library: ${LEVELDB_LIBRARY}")
	message (STATUS "LevelDB headers: ${LEVELDB_INCLUDE_DIR}")
	if(LEVELDB_LIBRARY AND LEVELDB_INCLUDE_DIR)
		set(USE_LEVELDB 1)
		message(STATUS "LevelDB backend enabled")
		include_directories(${LEVELDB_INCLUDE_DIR})
	else(LEVELDB_LIBRARY AND LEVELDB_INCLUDE_DIR)
		set(USE_LEVELDB 0)
		message(STATUS "LevelDB not found!")
	endif(LEVELDB_LIBRARY AND LEVELDB_INCLUDE_DIR)
endif(ENABLE_LEVELDB)

set(USE_REDIS 0)

OPTION(ENABLE_REDIS "Enable redis backend" 1)

if(ENABLE_REDIS)
	find_library(REDIS_LIBRARY hiredis)
	find_path(REDIS_INCLUDE_DIR hiredis.h PATH_SUFFIXES hiredis)
	message(STATUS "redis library: ${REDIS_LIBRARY}")
	message(STATUS "redis headers: ${REDIS_INCLUDE_DIR}")
	if(REDIS_LIBRARY AND REDIS_INCLUDE_DIR)
		set(USE_REDIS 1)
		message(STATUS "redis backend enabled")
		include_directories(${REDIS_INCLUDE_DIR})
	else(REDIS_LIBRARY AND REDIS_INCLUDE_DIR)
		set(USE_REDIS 0)
		message(STATUS "redis not found!")
	endif(REDIS_LIBRARY AND REDIS_INCLUDE_DIR)
endif(ENABLE_REDIS)

configure_file(
	"${PROJECT_SOURCE_DIR}/cmake_config.h.in"
	"${PROJECT_BINARY_DIR}/cmake_config.h"
)

# Add a target that always rebuilds cmake_config_githash.h
add_custom_target(GenerateVersion
	COMMAND ${CMAKE_COMMAND}
	-D "GENERATE_VERSION_SOURCE_DIR=${CMAKE_CURRENT_SOURCE_DIR}"
	-D "GENERATE_VERSION_BINARY_DIR=${CMAKE_CURRENT_BINARY_DIR}"
	-D "VERSION_STRING=${VERSION_STRING}"
	-D "VERSION_EXTRA=${VERSION_EXTRA}"
	-P "${CMAKE_SOURCE_DIR}/cmake/Modules/GenerateVersion.cmake"
	WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}")

add_subdirectory(jthread)
add_subdirectory(script)
add_subdirectory(util)

set(common_SRCS
	version.cpp
	rollback_interface.cpp
	rollback.cpp
	genericobject.cpp
	voxelalgorithms.cpp
	sound.cpp
	quicktune.cpp
	subgame.cpp
	inventorymanager.cpp
	mods.cpp
	content_abm.cpp
	craftdef.cpp
	nameidmapping.cpp
	itemdef.cpp
	nodedef.cpp
	object_properties.cpp
	log.cpp
	content_sao.cpp
	emerge.cpp
	mapgen.cpp
	mapgen_v6.cpp
	mapgen_v7.cpp
	mapgen_indev.cpp
	mapgen_singlenode.cpp
	mapgen_math.cpp
	treegen.cpp
	dungeongen.cpp
	cavegen.cpp
	content_nodemeta.cpp
	content_mapnode.cpp
	collision.cpp
	nodemetadata.cpp
	nodetimer.cpp
	serverobject.cpp
	noise.cpp
	porting.cpp
	tool.cpp
	defaultsettings.cpp
	mapnode.cpp
	voxel.cpp
	inventory.cpp
	debug.cpp
	serialization.cpp
	light.cpp
	filesys.cpp
	connection.cpp
	environment.cpp
	server.cpp
	clientiface.cpp
	socket.cpp
	mapblock.cpp
	mapsector.cpp
	map.cpp
	database.cpp
	database-dummy.cpp
	database-leveldb.cpp
	database-sqlite3.cpp
	database-redis.cpp
	player.cpp
	test.cpp
	sha1.cpp
	base64.cpp
	ban.cpp
	biome.cpp
	staticobject.cpp
	serverlist.cpp
	pathfinder.cpp
	convert_json.cpp
	gettext.cpp
	httpfetch.cpp
	${JTHREAD_SRCS}
	${common_SCRIPT_SRCS}
	${UTIL_SRCS}
)

# This gives us the icon and file version information
if(WIN32)
	set(WINRESOURCE_FILE ${CMAKE_CURRENT_SOURCE_DIR}/../misc/winresource.rc)
	if(MINGW)
		if(NOT CMAKE_RC_COMPILER)
			set(CMAKE_RC_COMPILER "windres.exe")
		endif()
		ADD_CUSTOM_COMMAND(OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/winresource_rc.o
			COMMAND ${CMAKE_RC_COMPILER} -I${CMAKE_CURRENT_SOURCE_DIR} -I${CMAKE_CURRENT_BINARY_DIR}
			-i${WINRESOURCE_FILE}
			-o ${CMAKE_CURRENT_BINARY_DIR}/winresource_rc.o
			WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
			DEPENDS ${WINRESOURCE_FILE})
		SET(common_SRCS ${common_SRCS} ${CMAKE_CURRENT_BINARY_DIR}/winresource_rc.o)
	else(MINGW) # Probably MSVC
		set(common_SRCS ${common_SRCS} ${WINRESOURCE_FILE})
	endif(MINGW)
endif()

# Client sources
set(minetest_SRCS
	${common_SRCS}
	${sound_SRCS}
	localplayer.cpp
	sky.cpp
	clientmap.cpp
	content_cso.cpp
	content_mapblock.cpp
	content_cao.cpp
	mesh.cpp
	mapblock_mesh.cpp
	keycode.cpp
	camera.cpp
	clouds.cpp
	particles.cpp
	clientobject.cpp
	chat.cpp
	hud.cpp
	guiKeyChangeMenu.cpp
	guiTextInputMenu.cpp
	guiFormSpecMenu.cpp
	guiTable.cpp
	guiPasswordChange.cpp
	guiVolumeChange.cpp
	guiChatConsole.cpp
	client.cpp
	clientmedia.cpp
	filecache.cpp
	tile.cpp
	shader.cpp
	game.cpp
	main.cpp
	guiEngine.cpp
	guiFileSelectMenu.cpp
	convert_json.cpp
	${minetest_SCRIPT_SRCS}
)

if(USE_FREETYPE)
	set(minetest_SRCS
		${minetest_SRCS}
		intlGUIEditBox.cpp
	)
endif(USE_FREETYPE)

list(SORT minetest_SRCS)

# Server sources
set(minetestserver_SRCS
	${common_SRCS}
	main.cpp
)
list(SORT minetestserver_SRCS)

include_directories(
	${PROJECT_BINARY_DIR}
	${PROJECT_SOURCE_DIR}
	${IRRLICHT_INCLUDE_DIR}
	${ZLIB_INCLUDE_DIR}
	${CMAKE_BUILD_TYPE}
	${PNG_INCLUDE_DIR}
	${GETTEXT_INCLUDE_DIR}
	${SOUND_INCLUDE_DIRS}
	${SQLITE3_INCLUDE_DIR}
	${LUA_INCLUDE_DIR}
	${JSON_INCLUDE_DIR}
	${PROJECT_SOURCE_DIR}/script
)

if(USE_FREETYPE)
	include_directories(
		${FREETYPE_INCLUDE_DIRS}
		${CGUITTFONT_INCLUDE_DIR}
	)
endif(USE_FREETYPE)

if(USE_CURL)
	include_directories(
		${CURL_INCLUDE_DIR}
	)
endif(USE_CURL)

set(EXECUTABLE_OUTPUT_PATH "${CMAKE_SOURCE_DIR}/bin")

if(BUILD_CLIENT)
	add_executable(${PROJECT_NAME} ${minetest_SRCS})
	add_dependencies(${PROJECT_NAME} GenerateVersion)
	target_link_libraries(
		${PROJECT_NAME}
		${ZLIB_LIBRARIES}
		${IRRLICHT_LIBRARY}
		${OPENGL_LIBRARIES}
		${JPEG_LIBRARIES}
		${BZIP2_LIBRARIES}
		${PNG_LIBRARIES}
		${X11_LIBRARIES}
		${GETTEXT_LIBRARY}
		${SOUND_LIBRARIES}
		${SQLITE3_LIBRARY}
		${LUA_LIBRARY}
		${JSON_LIBRARY}
		${OPENGLES2_LIBRARIES}
		${PLATFORM_LIBS}
		${CLIENT_PLATFORM_LIBS}
	)
	if(USE_CURL)
		target_link_libraries(
			${PROJECT_NAME}
			${CURL_LIBRARY}
		)
	endif(USE_CURL)
	if(USE_FREETYPE)
		if(FREETYPE_PKGCONFIG_FOUND)
			set_target_properties(${PROJECT_NAME}
				PROPERTIES
				COMPILE_FLAGS "${FREETYPE_CFLAGS_STR}"
			)
		endif(FREETYPE_PKGCONFIG_FOUND)
		target_link_libraries(
			${PROJECT_NAME}
			${FREETYPE_LIBRARY}
			${CGUITTFONT_LIBRARY}
		)
	endif(USE_FREETYPE)
	if (USE_LEVELDB)
		target_link_libraries(${PROJECT_NAME} ${LEVELDB_LIBRARY})
	endif(USE_LEVELDB)
	if (USE_REDIS)
		target_link_libraries(${PROJECT_NAME} ${REDIS_LIBRARY})
	endif(USE_REDIS)
endif(BUILD_CLIENT)

if(BUILD_SERVER)
	add_executable(${PROJECT_NAME}server ${minetestserver_SRCS})
	add_dependencies(${PROJECT_NAME}server GenerateVersion)
	target_link_libraries(
		${PROJECT_NAME}server
		${ZLIB_LIBRARIES}
		${SQLITE3_LIBRARY}
		${JSON_LIBRARY}
		${GETTEXT_LIBRARY}
		${LUA_LIBRARY}
		${PLATFORM_LIBS}
	)
	if (USE_LEVELDB)
		target_link_libraries(${PROJECT_NAME}server ${LEVELDB_LIBRARY})
	endif(USE_LEVELDB)
	if (USE_REDIS)
		target_link_libraries(${PROJECT_NAME}server ${REDIS_LIBRARY})
	endif(USE_REDIS)
	if(USE_CURL)
		target_link_libraries(
			${PROJECT_NAME}server
			${CURL_LIBRARY}
		)
	endif(USE_CURL)
endif(BUILD_SERVER)


#
# Set some optimizations and tweaks
#

include(CheckCXXCompilerFlag)

if(MSVC)
	# Visual Studio

	# EHa enables SEH exceptions (used for catching segfaults)
	set(CMAKE_CXX_FLAGS_RELEASE "/EHa /O2 /Ob2 /Oi /Ot /Oy /GL /FD /MT /GS- /arch:SSE /fp:fast /D NDEBUG /D _HAS_ITERATOR_DEBUGGING=0 /TP")
	#set(CMAKE_EXE_LINKER_FLAGS_RELEASE "/LTCG /NODEFAULTLIB:\"libcmtd.lib\" /NODEFAULTLIB:\"libcmt.lib\"")
	set(CMAKE_EXE_LINKER_FLAGS_RELEASE "/LTCG")

	# Debug build doesn't catch exceptions by itself
	# Add some optimizations because otherwise it's VERY slow
	set(CMAKE_CXX_FLAGS_DEBUG "/MDd /Zi /Ob0 /Od /RTC1")

	# Flags for C files (sqlite)
	# /MT = Link statically with standard library stuff
	set(CMAKE_C_FLAGS_RELEASE "/O2 /Ob2 /MT")

	if(BUILD_SERVER)
		set_target_properties(${PROJECT_NAME}server PROPERTIES
				COMPILE_DEFINITIONS "SERVER")
	endif(BUILD_SERVER)

else()
	# Probably GCC

	if(WARN_ALL)
		set(RELEASE_WARNING_FLAGS "-Wall")
	else()
		set(RELEASE_WARNING_FLAGS "")
	endif()

	if(NOT APPLE AND NOT "${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang")
		CHECK_CXX_COMPILER_FLAG("-Wno-unused-but-set-variable" HAS_UNUSED_BUT_SET_VARIABLE_WARNING)
		if(HAS_UNUSED_BUT_SET_VARIABLE_WARNING)
			set(WARNING_FLAGS "${WARNING_FLAGS} -Wno-unused-but-set-variable")
		endif(HAS_UNUSED_BUT_SET_VARIABLE_WARNING)
	endif()

	if(MINGW)
		set(OTHER_FLAGS "-mthreads -fexceptions")
	endif()

	if(APPLE)
		set(CMAKE_OSX_ARCHITECTURES i386 CACHE STRING "do not build for 64-bit" FORCE)
		set(ARCH i386)
	endif()

	set(CMAKE_CXX_FLAGS_RELEASE "-DNDEBUG ${RELEASE_WARNING_FLAGS} ${WARNING_FLAGS} ${OTHER_FLAGS} -O3 -ffast-math -Wall -fomit-frame-pointer -pipe -funroll-loops")
	set(CMAKE_CXX_FLAGS_DEBUG "-g -O1 -Wall ${WARNING_FLAGS} ${OTHER_FLAGS}")

	if(USE_GPROF)
		set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -pg")
	endif()

	if(BUILD_SERVER)
		set_target_properties(${PROJECT_NAME}server PROPERTIES
				COMPILE_DEFINITIONS "SERVER")
	endif(BUILD_SERVER)

endif()

#MESSAGE(STATUS "CMAKE_CXX_FLAGS_RELEASE=${CMAKE_CXX_FLAGS_RELEASE}")
#MESSAGE(STATUS "CMAKE_CXX_FLAGS_DEBUG=${CMAKE_CXX_FLAGS_DEBUG}")

#
# Installation
#
if(WIN32)
	if(MINGWM10_DLL)
		install(FILES ${MINGWM10_DLL} DESTINATION ${BINDIR})
	endif()
	if(DEFINED ZLIB_DLL)
		install(FILES ${ZLIB_DLL} DESTINATION ${BINDIR})
	endif()
	if(USE_SOUND)
		if(OPENAL_DLL)
			install(FILES ${OPENAL_DLL} DESTINATION ${BINDIR})
		endif()
		if(OGG_DLL)
			install(FILES ${OGG_DLL} DESTINATION ${BINDIR})
		endif()
		if(VORBIS_DLL)
			install(FILES ${VORBIS_DLL} DESTINATION ${BINDIR})
		endif()
		if(VORBISFILE_DLL)
			install(FILES ${VORBISFILE_DLL} DESTINATION ${BINDIR})
		endif()
	endif()
	if(CURL_DLL)
		install(FILES ${CURL_DLL} DESTINATION ${BINDIR})
	endif()
endif()

if(BUILD_CLIENT)
	install(TARGETS ${PROJECT_NAME} DESTINATION ${BINDIR})

	if(USE_GETTEXT)
		foreach(LOCALE ${GETTEXT_AVAILABLE_LOCALES})
			set_mo_paths(MO_BUILD_PATH MO_DEST_PATH ${LOCALE})
			set(MO_BUILD_PATH "${MO_BUILD_PATH}/${PROJECT_NAME}.mo")
			install(FILES ${MO_BUILD_PATH} DESTINATION ${MO_DEST_PATH})
		endforeach(LOCALE ${GETTEXT_AVAILABLE_LOCALES})
	endif()

	if(WIN32)
		if(DEFINED IRRLICHT_DLL)
			install(FILES ${IRRLICHT_DLL} DESTINATION ${BINDIR})
		endif()
		if(USE_GETTEXT)
			if(DEFINED GETTEXT_DLL)
				install(FILES ${GETTEXT_DLL} DESTINATION ${BINDIR})
			endif()
			if(DEFINED GETTEXT_ICONV_DLL)
				install(FILES ${GETTEXT_ICONV_DLL} DESTINATION ${BINDIR})
			endif()
		endif(USE_GETTEXT)
	endif()
endif(BUILD_CLIENT)

if(BUILD_SERVER)
	install(TARGETS ${PROJECT_NAME}server DESTINATION ${BINDIR})
endif(BUILD_SERVER)

if (USE_GETTEXT)
	set(MO_FILES)

	foreach(LOCALE ${GETTEXT_AVAILABLE_LOCALES})
		set(PO_FILE_PATH "${GETTEXT_PO_PATH}/${LOCALE}/minetest.po")
		set_mo_paths(MO_BUILD_PATH MO_DEST_PATH ${LOCALE})
		set(MO_FILE_PATH "${MO_BUILD_PATH}/${PROJECT_NAME}.mo")

		add_custom_command(OUTPUT ${MO_BUILD_PATH}
			COMMAND ${CMAKE_COMMAND} -E make_directory ${MO_BUILD_PATH}
			COMMENT "mo-update [${LOCALE}]: Creating locale directory.")

		add_custom_command(
			OUTPUT ${MO_FILE_PATH}
			COMMAND ${GETTEXT_MSGFMT} -o ${MO_FILE_PATH} ${PO_FILE_PATH}
			DEPENDS ${MO_BUILD_PATH} ${PO_FILE_PATH}
			WORKING_DIRECTORY "${GETTEXT_PO_PATH}/${LOCALE}"
			COMMENT "mo-update [${LOCALE}]: Creating mo file."
			)

		set(MO_FILES ${MO_FILES} ${MO_FILE_PATH})
	endforeach(LOCALE ${GETTEXT_AVAILABLE_LOCALES})

	add_custom_target(translations ALL COMMENT "mo update" DEPENDS ${MO_FILES})
endif(USE_GETTEXT)

# Subdirectories

if (SQLITE3_FOUND)
else (SQLITE3_FOUND)
	add_subdirectory(sqlite)
endif (SQLITE3_FOUND)

if (BUILD_CLIENT AND USE_FREETYPE)
	add_subdirectory(cguittfont)
endif (BUILD_CLIENT AND USE_FREETYPE)

if (JSON_FOUND)
else (JSON_FOUND)
	add_subdirectory(json)
endif (JSON_FOUND)

#end
