cmake_minimum_required(VERSION 2.6)

project(minetest)

INCLUDE(CheckIncludeFiles)

# Add custom SemiDebug build mode
set(CMAKE_CXX_FLAGS_SEMIDEBUG "-O1 -g -Wall -Wabi" CACHE STRING
	"Flags used by the C++ compiler during semidebug builds."
	FORCE
)
set(CMAKE_C_FLAGS_SEMIDEBUG "-O1 -g -Wall -pedantic" CACHE STRING
	"Flags used by the C compiler during semidebug builds."
	FORCE
)
mark_as_advanced(
	CMAKE_CXX_FLAGS_SEMIDEBUG
	CMAKE_C_FLAGS_SEMIDEBUG
)
set(CMAKE_BUILD_TYPE "${CMAKE_BUILD_TYPE}" CACHE STRING
	"Choose the type of build. Options are: None Debug SemiDebug RelWithDebInfo MinSizeRel."
	FORCE
)


# Set some random things default to not being visible in the GUI
mark_as_advanced(EXECUTABLE_OUTPUT_PATH LIBRARY_OUTPUT_PATH)


if(NOT (BUILD_CLIENT OR BUILD_SERVER))
	message(WARNING "Neither BUILD_CLIENT nor BUILD_SERVER is set! Setting BUILD_SERVER=true")
	set(BUILD_SERVER TRUE)
endif()


option(ENABLE_CURL "Enable cURL support for fetching media" TRUE)
set(USE_CURL FALSE)

if(ENABLE_CURL)
	find_package(CURL)
	if (CURL_FOUND)
		message(STATUS "cURL support enabled.")
		set(USE_CURL TRUE)
	endif()
else()
	mark_as_advanced(CLEAR CURL_LIBRARY CURL_INCLUDE_DIR)
endif()

if(NOT USE_CURL)
	if(BUILD_CLIENT)
		message(WARNING "cURL is required to load the server list")
	endif()
	if(BUILD_SERVER)
		message(WARNING "cURL is required to announce to the server list")
	endif()
endif()


option(ENABLE_GETTEXT "Use GetText for internationalization" FALSE)
set(USE_GETTEXT FALSE)

if(ENABLE_GETTEXT)
	find_package(GettextLib)
	if(GETTEXT_FOUND)
		if(WIN32)
			message(STATUS "GetText library: ${GETTEXT_LIBRARY}")
			message(STATUS "GetText DLL: ${GETTEXT_DLL}")
			message(STATUS "GetText iconv DLL: ${GETTEXT_ICONV_DLL}")
		endif()
		set(USE_GETTEXT TRUE)
		message(STATUS "GetText enabled; locales found: ${GETTEXT_AVAILABLE_LOCALES}")
	endif(GETTEXT_FOUND)
else()
	mark_as_advanced(GETTEXT_ICONV_DLL GETTEXT_INCLUDE_DIR GETTEXT_LIBRARY GETTEXT_MSGFMT)
	message(STATUS "GetText disabled.")
endif()


option(ENABLE_SOUND "Enable sound" TRUE)
set(USE_SOUND FALSE)

if(BUILD_CLIENT AND ENABLE_SOUND)
	# Sound libraries
	find_package(OpenAL)
	find_package(Vorbis)
	if(NOT OPENAL_FOUND)
		message(STATUS "Sound enabled, but OpenAL not found!")
		mark_as_advanced(CLEAR OPENAL_LIBRARY OPENAL_INCLUDE_DIR)
	endif()
	if(NOT VORBIS_FOUND)
		message(STATUS "Sound enabled, but Vorbis libraries not found!")
		mark_as_advanced(CLEAR OGG_INCLUDE_DIR VORBIS_INCLUDE_DIR OGG_LIBRARY VORBIS_LIBRARY VORBISFILE_LIBRARY)
	endif()
	if(OPENAL_FOUND AND VORBIS_FOUND)
		set(USE_SOUND TRUE)
		message(STATUS "Sound enabled.")
	else()
		message(FATAL_ERROR "Sound enabled, but cannot be used.\n"
			"To continue, either fill in the required paths or disable sound. (-DENABLE_SOUND=0)")
	endif()
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


option(ENABLE_GLES "Enable OpenGL ES support" FALSE)
mark_as_advanced(ENABLE_GLES)
if(ENABLE_GLES)
	find_package(OpenGLES2)
endif()


option(ENABLE_FREETYPE "Enable FreeType2 (TrueType fonts and basic unicode support)" TRUE)
set(USE_FREETYPE FALSE)

if(ENABLE_FREETYPE)
##
## Note: FindFreetype.cmake seems to have been fixed in recent versions of
## CMake. If issues persist, re-enable this workaround specificially for the
## failing platforms.
##
#	if(UNIX)
#		include(FindPkgConfig)
#		if(PKG_CONFIG_FOUND)
#			pkg_check_modules(FREETYPE QUIET freetype2)
#			if(FREETYPE_FOUND)
#				SET(FREETYPE_PKGCONFIG_FOUND TRUE)
#				SET(FREETYPE_LIBRARY ${FREETYPE_LIBRARIES})
#				# Because CMake is idiotic
#				string(REPLACE ";" " " FREETYPE_CFLAGS_STR ${FREETYPE_CFLAGS})
#				string(REPLACE ";" " " FREETYPE_LDFLAGS_STR ${FREETYPE_LDFLAGS})
#			endif(FREETYPE_FOUND)
#		endif(PKG_CONFIG_FOUND)
#	endif(UNIX)
#	if(NOT FREETYPE_FOUND)
#		find_package(Freetype)
#	endif()
	find_package(Freetype)
	if(FREETYPE_FOUND)
		message(STATUS "Freetype enabled.")
		set(USE_FREETYPE TRUE)
		set(CGUITTFONT_INCLUDE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/cguittfont")
		set(CGUITTFONT_LIBRARY cguittfont)
	endif()
endif(ENABLE_FREETYPE)

# LuaJIT
option(ENABLE_LUAJIT "Enable LuaJIT support" TRUE)
set(USE_LUAJIT FALSE)
if(ENABLE_LUAJIT)
	find_package(LuaJIT)
	if(LUAJIT_FOUND)
		set(USE_LUAJIT TRUE)
	endif(LUAJIT_FOUND)
else()
	message (STATUS "LuaJIT detection disabled! (ENABLE_LUAJIT=0)")
endif()
if(NOT USE_LUAJIT)
	message(STATUS "LuaJIT not found, using bundled Lua.")
	set(LUA_LIBRARY "lua")
	set(LUA_INCLUDE_DIR "${PROJECT_SOURCE_DIR}/lua/src")
	add_subdirectory(lua)
endif()

find_package(GMP REQUIRED)

option(ENABLE_CURSES "Enable ncurses console" TRUE)
set(USE_CURSES FALSE)

if(ENABLE_CURSES)
	find_package(Ncursesw)
	if(CURSES_FOUND)
		set(USE_CURSES TRUE)
		message(STATUS "ncurses console enabled.")
		include_directories(${CURSES_INCLUDE_DIRS})
	else()
		message(STATUS "ncurses not found!")
	endif()
endif(ENABLE_CURSES)

option(ENABLE_LEVELDB "Enable LevelDB backend" TRUE)
set(USE_LEVELDB FALSE)

if(ENABLE_LEVELDB)
	find_library(LEVELDB_LIBRARY leveldb)
	find_path(LEVELDB_INCLUDE_DIR db.h PATH_SUFFIXES leveldb)
	if(LEVELDB_LIBRARY AND LEVELDB_INCLUDE_DIR)
		set(USE_LEVELDB TRUE)
		message(STATUS "LevelDB backend enabled.")
		include_directories(${LEVELDB_INCLUDE_DIR})
	else()
		message(STATUS "LevelDB not found!")
	endif()
endif(ENABLE_LEVELDB)


OPTION(ENABLE_REDIS "Enable Redis backend" TRUE)
set(USE_REDIS FALSE)

if(ENABLE_REDIS)
	find_library(REDIS_LIBRARY hiredis)
	find_path(REDIS_INCLUDE_DIR hiredis.h PATH_SUFFIXES hiredis)
	if(REDIS_LIBRARY AND REDIS_INCLUDE_DIR)
		set(USE_REDIS TRUE)
		message(STATUS "Redis backend enabled.")
		include_directories(${REDIS_INCLUDE_DIR})
	else(REDIS_LIBRARY AND REDIS_INCLUDE_DIR)
		message(STATUS "Redis not found!")
	endif(REDIS_LIBRARY AND REDIS_INCLUDE_DIR)
endif(ENABLE_REDIS)


find_package(SQLite3 REQUIRED)
find_package(Json REQUIRED)

OPTION(ENABLE_SPATIAL "Enable SpatialIndex AreaStore backend" TRUE)
set(USE_SPATIAL FALSE)

if(ENABLE_SPATIAL)
	find_library(SPATIAL_LIBRARY spatialindex)
	find_path(SPATIAL_INCLUDE_DIR spatialindex/SpatialIndex.h)
	if(SPATIAL_LIBRARY AND SPATIAL_INCLUDE_DIR)
		set(USE_SPATIAL TRUE)
		message(STATUS "SpatialIndex AreaStore backend enabled.")
		include_directories(${SPATIAL_INCLUDE_DIR})
	else(SPATIAL_LIBRARY AND SPATIAL_INCLUDE_DIR)
		message(STATUS "SpatialIndex not found!")
	endif(SPATIAL_LIBRARY AND SPATIAL_INCLUDE_DIR)
endif(ENABLE_SPATIAL)


if(NOT MSVC)
	set(USE_GPROF FALSE CACHE BOOL "Use -pg flag for g++")
endif()

# Use cmake_config.h
add_definitions(-DUSE_CMAKE_CONFIG_H)

if(WIN32)
	# Windows
	if(MSVC) # MSVC Specifics
		set(PLATFORM_LIBS dbghelp.lib ${PLATFORM_LIBS})
		# Surpress some useless warnings
		add_definitions ( /D "_CRT_SECURE_NO_DEPRECATE" /W1 )
	else() # Probably MinGW = GCC
		set(PLATFORM_LIBS "")
	endif()
	set(PLATFORM_LIBS ws2_32.lib shlwapi.lib ${PLATFORM_LIBS})

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
	endif()
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
			find_library(CARBON_LIB Carbon)
			find_library(COCOA_LIB Cocoa)
			find_library(IOKIT_LIB IOKit)
			mark_as_advanced(
				CARBON_LIB
				COCOA_LIB
				IOKIT_LIB
			)
			SET(CLIENT_PLATFORM_LIBS ${CLIENT_PLATFORM_LIBS} ${CARBON_LIB} ${COCOA_LIB} ${IOKIT_LIB})
		endif(APPLE)
	endif(BUILD_CLIENT)
	find_package(ZLIB REQUIRED)
	set(PLATFORM_LIBS -lpthread ${CMAKE_DL_LIBS})
	if(APPLE)
		set(PLATFORM_LIBS "-framework CoreFoundation" ${PLATFORM_LIBS})
	else()
		set(PLATFORM_LIBS -lrt ${PLATFORM_LIBS})
	endif(APPLE)

	# This way Xxf86vm is found on OpenBSD too
	find_library(XXF86VM_LIBRARY Xxf86vm)
	mark_as_advanced(XXF86VM_LIBRARY)
	set(CLIENT_PLATFORM_LIBS ${CLIENT_PLATFORM_LIBS} ${XXF86VM_LIBRARY})

	# Prefer local iconv if installed
	find_library(ICONV_LIBRARY iconv)
	mark_as_advanced(ICONV_LIBRARY)
	if (ICONV_LIBRARY)
		set(PLATFORM_LIBS ${PLATFORM_LIBS} ${ICONV_LIBRARY})
	endif()
endif()

check_include_files(endian.h HAVE_ENDIAN_H)

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
	-D "DEVELOPMENT_BUILD=${DEVELOPMENT_BUILD}"
	-P "${CMAKE_SOURCE_DIR}/cmake/Modules/GenerateVersion.cmake"
	WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}")


add_subdirectory(threading)
add_subdirectory(network)
add_subdirectory(script)
add_subdirectory(unittest)
add_subdirectory(util)

set(common_SRCS
	ban.cpp
	cavegen.cpp
	chat.cpp
	clientiface.cpp
	collision.cpp
	content_abm.cpp
	content_mapnode.cpp
	content_nodemeta.cpp
	content_sao.cpp
	convert_json.cpp
	craftdef.cpp
	database-dummy.cpp
	database-leveldb.cpp
	database-redis.cpp
	database-sqlite3.cpp
	database.cpp
	debug.cpp
	defaultsettings.cpp
	dungeongen.cpp
	emerge.cpp
	environment.cpp
	filesys.cpp
	genericobject.cpp
	gettext.cpp
	httpfetch.cpp
	inventory.cpp
	inventorymanager.cpp
	itemdef.cpp
	light.cpp
	log.cpp
	map.cpp
	mapblock.cpp
	mapgen.cpp
	mapgen_flat.cpp
	mapgen_fractal.cpp
	mapgen_singlenode.cpp
	mapgen_v5.cpp
	mapgen_v6.cpp
	mapgen_v7.cpp
	mapgen_valleys.cpp
	mapnode.cpp
	mapsector.cpp
	mg_biome.cpp
	mg_decoration.cpp
	mg_ore.cpp
	mg_schematic.cpp
	mods.cpp
	nameidmapping.cpp
	nodedef.cpp
	nodemetadata.cpp
	nodetimer.cpp
	noise.cpp
	objdef.cpp
	object_properties.cpp
	pathfinder.cpp
	player.cpp
	porting.cpp
	profiler.cpp
	quicktune.cpp
	rollback.cpp
	rollback_interface.cpp
	serialization.cpp
	server.cpp
	serverlist.cpp
	serverobject.cpp
	settings.cpp
	socket.cpp
	sound.cpp
	staticobject.cpp
	subgame.cpp
	terminal_chat_console.cpp
	tool.cpp
	treegen.cpp
	version.cpp
	voxel.cpp
	voxelalgorithms.cpp
	${common_network_SRCS}
	${JTHREAD_SRCS}
	${common_SCRIPT_SRCS}
	${UTIL_SRCS}
	${UNITTEST_SRCS}
)


# This gives us the icon and file version information
if(WIN32)
	set(WINRESOURCE_FILE "${CMAKE_CURRENT_SOURCE_DIR}/../misc/winresource.rc")
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
if (BUILD_CLIENT)
	add_subdirectory(client)
endif(BUILD_CLIENT)

set(client_SRCS
	${client_SRCS}
	${common_SRCS}
	${sound_SRCS}
	${client_network_SRCS}
	camera.cpp
	client.cpp
	clientmap.cpp
	clientmedia.cpp
	clientobject.cpp
	clouds.cpp
	content_cao.cpp
	content_cso.cpp
	content_mapblock.cpp
	convert_json.cpp
	drawscene.cpp
	filecache.cpp
	fontengine.cpp
	game.cpp
	guiChatConsole.cpp
	guiEngine.cpp
	guiFileSelectMenu.cpp
	guiFormSpecMenu.cpp
	guiKeyChangeMenu.cpp
	guiPasswordChange.cpp
	guiscalingfilter.cpp
	guiTable.cpp
	guiVolumeChange.cpp
	hud.cpp
	imagefilters.cpp
	intlGUIEditBox.cpp
	keycode.cpp
	localplayer.cpp
	main.cpp
	mapblock_mesh.cpp
	mesh.cpp
	minimap.cpp
	particles.cpp
	shader.cpp
	sky.cpp
	wieldmesh.cpp
	${client_SCRIPT_SRCS}
)
list(SORT client_SRCS)

# Server sources
set(server_SRCS
	${common_SRCS}
	main.cpp
)
list(SORT server_SRCS)

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
	${GMP_INCLUDE_DIR}
	${JSON_INCLUDE_DIR}
	${PROJECT_SOURCE_DIR}/script
)


if(USE_FREETYPE)
	include_directories(${FREETYPE_INCLUDE_DIRS} ${CGUITTFONT_INCLUDE_DIR})
endif()

if(USE_CURL)
	include_directories(${CURL_INCLUDE_DIR})
endif()


set(EXECUTABLE_OUTPUT_PATH "${CMAKE_SOURCE_DIR}/bin")


if(BUILD_CLIENT)
	add_executable(${PROJECT_NAME} ${client_SRCS})
	add_dependencies(${PROJECT_NAME} GenerateVersion)
	set(client_LIBS
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
		${GMP_LIBRARY}
		${JSON_LIBRARY}
		${OPENGLES2_LIBRARIES}
		${PLATFORM_LIBS}
		${CLIENT_PLATFORM_LIBS}
	)
	if(APPLE)
		target_link_libraries(
			${client_LIBS}
			${ICONV_LIBRARY}
		)
	else()
		target_link_libraries(
			${client_LIBS}
		)
	endif()
	if(USE_CURL)
		target_link_libraries(
			${PROJECT_NAME}
			${CURL_LIBRARY}
		)
	endif()
	if(USE_FREETYPE)
		if(FREETYPE_PKGCONFIG_FOUND)
			set_target_properties(${PROJECT_NAME}
				PROPERTIES
				COMPILE_FLAGS "${FREETYPE_CFLAGS_STR}"
			)
		endif()
		target_link_libraries(
			${PROJECT_NAME}
			${FREETYPE_LIBRARY}
			${CGUITTFONT_LIBRARY}
		)
	endif()
	if (USE_CURSES)
		target_link_libraries(${PROJECT_NAME} ${CURSES_LIBRARIES})
	endif()
	if (USE_LEVELDB)
		target_link_libraries(${PROJECT_NAME} ${LEVELDB_LIBRARY})
	endif()
	if (USE_REDIS)
		target_link_libraries(${PROJECT_NAME} ${REDIS_LIBRARY})
	endif()
	if (USE_SPATIAL)
		target_link_libraries(${PROJECT_NAME} ${SPATIAL_LIBRARY})
	endif()
endif(BUILD_CLIENT)


if(BUILD_SERVER)
	add_executable(${PROJECT_NAME}server ${server_SRCS})
	add_dependencies(${PROJECT_NAME}server GenerateVersion)
	target_link_libraries(
		${PROJECT_NAME}server
		${ZLIB_LIBRARIES}
		${SQLITE3_LIBRARY}
		${JSON_LIBRARY}
		${GETTEXT_LIBRARY}
		${LUA_LIBRARY}
		${GMP_LIBRARY}
		${PLATFORM_LIBS}
	)
	set_target_properties(${PROJECT_NAME}server PROPERTIES
			COMPILE_DEFINITIONS "SERVER")
	if (USE_CURSES)
		target_link_libraries(${PROJECT_NAME}server ${CURSES_LIBRARIES})
	endif()
	if (USE_LEVELDB)
		target_link_libraries(${PROJECT_NAME}server ${LEVELDB_LIBRARY})
	endif()
	if (USE_REDIS)
		target_link_libraries(${PROJECT_NAME}server ${REDIS_LIBRARY})
	endif()
	if (USE_SPATIAL)
		target_link_libraries(${PROJECT_NAME}server ${SPATIAL_LIBRARY})
	endif()
	if(USE_CURL)
		target_link_libraries(
			${PROJECT_NAME}server
			${CURL_LIBRARY}
		)
	endif()
endif(BUILD_SERVER)


# Set some optimizations and tweaks

include(CheckCXXCompilerFlag)

if(MSVC)
	# Visual Studio

	# EHa enables SEH exceptions (used for catching segfaults)
	set(CMAKE_CXX_FLAGS_RELEASE "/EHa /Ox /Ob2 /Oi /Ot /Oy /GL /FD /MT /GS- /arch:SSE /fp:fast /D NDEBUG /D _HAS_ITERATOR_DEBUGGING=0 /TP")
	#set(CMAKE_EXE_LINKER_FLAGS_RELEASE "/LTCG /NODEFAULTLIB:\"libcmtd.lib\" /NODEFAULTLIB:\"libcmt.lib\"")
	set(CMAKE_EXE_LINKER_FLAGS_RELEASE "/LTCG")

	set(CMAKE_CXX_FLAGS_SEMIDEBUG "/MDd /Zi /Ob0 /O1 /RTC1")

	# Debug build doesn't catch exceptions by itself
	# Add some optimizations because otherwise it's VERY slow
	set(CMAKE_CXX_FLAGS_DEBUG "/MDd /Zi /Ob0 /Od /RTC1")

	# Flags for C files (sqlite)
	# /MT = Link statically with standard library stuff
	set(CMAKE_C_FLAGS_RELEASE "/O2 /Ob2 /MT")
else()
	# Probably GCC
	if(APPLE)
		SET( CMAKE_EXE_LINKER_FLAGS  "${CMAKE_EXE_LINKER_FLAGS} -pagezero_size 10000 -image_base 100000000" )
	endif()
	if(WARN_ALL)
		set(RELEASE_WARNING_FLAGS "-Wall")
	else()
		set(RELEASE_WARNING_FLAGS "")
	endif()

	if("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang")
		# clang does not understand __extern_always_inline but libc headers use it
		set(OTHER_FLAGS "${OTHER_FLAGS} \"-D__extern_always_inline=extern __always_inline\"")
		set(OTHER_FLAGS "${OTHER_FLAGS} -Wsign-compare")
	endif()

	if(MINGW)
		set(OTHER_FLAGS "-mthreads -fexceptions")
	endif()

	set(CMAKE_CXX_FLAGS_RELEASE "-DNDEBUG ${RELEASE_WARNING_FLAGS} ${WARNING_FLAGS} ${OTHER_FLAGS} -ffast-math -Wall -pipe -funroll-loops")
	if(APPLE)
		set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -Os")
	else()
		set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -O3 -fomit-frame-pointer")
	endif(APPLE)
	set(CMAKE_CXX_FLAGS_SEMIDEBUG "-g -O1 -Wall -Wabi ${WARNING_FLAGS} ${OTHER_FLAGS}")
	set(CMAKE_CXX_FLAGS_DEBUG "-g -O0 -Wall -Wabi ${WARNING_FLAGS} ${OTHER_FLAGS}")

	if(USE_GPROF)
		set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -pg")
	endif()
endif()


# Installation

if(WIN32)
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
	if(ZLIB_DLL)
		install(FILES ${ZLIB_DLL} DESTINATION ${BINDIR})
	endif()
	if(ZLIBWAPI_DLL)
		install(FILES ${ZLIBWAPI_DLL} DESTINATION ${BINDIR})
	endif()
	if(FREETYPE_DLL)
		install(FILES ${FREETYPE_DLL} DESTINATION ${BINDIR})
	endif()
	if(SQLITE3_DLL)
		install(FILES ${SQLITE3_DLL} DESTINATION ${BINDIR})
	endif()
	if(LEVELDB_DLL)
		install(FILES ${LEVELDB_DLL} DESTINATION ${BINDIR})
	endif()
endif()

if(BUILD_CLIENT)
	install(TARGETS ${PROJECT_NAME}
		RUNTIME DESTINATION ${BINDIR}
		LIBRARY DESTINATION ${BINDIR}
		ARCHIVE DESTINATION ${BINDIR}
		BUNDLE DESTINATION .
	)

	if(APPLE)
		install(CODE "
			set(BU_CHMOD_BUNDLE_ITEMS ON)
			include(BundleUtilities)
			fixup_bundle(\"\${CMAKE_INSTALL_PREFIX}/${BUNDLE_PATH}\" \"\" \"\${CMAKE_INSTALL_PREFIX}/${BINDIR}\")
		" COMPONENT Runtime)
	endif()

	if(USE_GETTEXT)
		foreach(LOCALE ${GETTEXT_AVAILABLE_LOCALES})
			set_mo_paths(MO_BUILD_PATH MO_DEST_PATH ${LOCALE})
			set(MO_BUILD_PATH "${MO_BUILD_PATH}/${PROJECT_NAME}.mo")
			install(FILES ${MO_BUILD_PATH} DESTINATION ${MO_DEST_PATH})
		endforeach()
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
		endif()
	endif()
endif(BUILD_CLIENT)

if(BUILD_SERVER)
	install(TARGETS ${PROJECT_NAME}server DESTINATION ${BINDIR})
endif()

if (USE_GETTEXT)
	set(MO_FILES)

	foreach(LOCALE ${GETTEXT_AVAILABLE_LOCALES})
		set(PO_FILE_PATH "${GETTEXT_PO_PATH}/${LOCALE}/${PROJECT_NAME}.po")
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
	endforeach()

	add_custom_target(translations ALL COMMENT "mo update" DEPENDS ${MO_FILES})
endif()


# Subdirectories

if (BUILD_CLIENT AND USE_FREETYPE)
	add_subdirectory(cguittfont)
endif()

