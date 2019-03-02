cmake_minimum_required(VERSION 2.6)

project(minetest)

INCLUDE(CheckIncludeFiles)
INCLUDE(CheckLibraryExists)

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


option(ENABLE_GETTEXT "Use GetText for internationalization" TRUE)
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
	endif()
endif(ENABLE_FREETYPE)

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

option(ENABLE_POSTGRESQL "Enable PostgreSQL backend" TRUE)
set(USE_POSTGRESQL FALSE)

if(ENABLE_POSTGRESQL)
	find_program(POSTGRESQL_CONFIG_EXECUTABLE pg_config DOC "pg_config")
	find_library(POSTGRESQL_LIBRARY pq)
	if(POSTGRESQL_CONFIG_EXECUTABLE)
		execute_process(COMMAND ${POSTGRESQL_CONFIG_EXECUTABLE} --includedir-server
			OUTPUT_VARIABLE POSTGRESQL_SERVER_INCLUDE_DIRS
			OUTPUT_STRIP_TRAILING_WHITESPACE)
		execute_process(COMMAND ${POSTGRESQL_CONFIG_EXECUTABLE}
			OUTPUT_VARIABLE POSTGRESQL_CLIENT_INCLUDE_DIRS
			OUTPUT_STRIP_TRAILING_WHITESPACE)
		# This variable is case sensitive for the cmake PostgreSQL module
		set(PostgreSQL_ADDITIONAL_SEARCH_PATHS ${POSTGRESQL_SERVER_INCLUDE_DIRS} ${POSTGRESQL_CLIENT_INCLUDE_DIRS})
	endif()

	find_package("PostgreSQL")

	if(POSTGRESQL_FOUND)
		set(USE_POSTGRESQL TRUE)
		message(STATUS "PostgreSQL backend enabled")
		# This variable is case sensitive, don't try to change it to POSTGRESQL_INCLUDE_DIR
		message(STATUS "PostgreSQL includes: ${PostgreSQL_INCLUDE_DIR}")
		include_directories(${PostgreSQL_INCLUDE_DIR})
	else()
		message(STATUS "PostgreSQL not found!")
	endif()
endif(ENABLE_POSTGRESQL)

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

# Haiku endian support
if(HAIKU)
	set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -D_BSD_SOURCE")
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -D_BSD_SOURCE")
endif()

# Use cmake_config.h
add_definitions(-DUSE_CMAKE_CONFIG_H)

if(WIN32)
	# Windows
	if(MSVC) # MSVC Specifics
		set(PLATFORM_LIBS dbghelp.lib ${PLATFORM_LIBS})
		# Surpress some useless warnings
		add_definitions ( /D "_CRT_SECURE_NO_DEPRECATE" /W1 )
		# Get M_PI to work
		add_definitions(/D "_USE_MATH_DEFINES")
		# Dont define min/max macros in minwindef.h
		add_definitions(/D "NOMINMAX")
	else() # Probably MinGW = GCC
		set(PLATFORM_LIBS "")
	endif()
	set(PLATFORM_LIBS ws2_32.lib version.lib shlwapi.lib ${PLATFORM_LIBS})

	# Zlib stuff
	set(ZLIB_INCLUDE_DIR "${PROJECT_SOURCE_DIR}/../../zlib/zlib-1.2.5"
			CACHE PATH "Zlib include directory")
	set(ZLIB_LIBRARIES "${PROJECT_SOURCE_DIR}/../../zlib125dll/dll32/zlibwapi.lib"
			CACHE FILEPATH "Path to zlib library (usually zlibwapi.lib)")
	set(ZLIB_DLL "${PROJECT_SOURCE_DIR}/../../zlib125dll/dll32/zlibwapi.dll"
			CACHE FILEPATH "Path to zlib DLL (for installation)")
	set(ZLIBWAPI_DLL "" CACHE FILEPATH "Path to zlibwapi DLL")
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
	if(USE_LUAJIT)
		set(LUA_DLL "" CACHE FILEPATH "Path to lua51.dll for installation (optional)")
	endif()
else()
	# Unix probably
	if(BUILD_CLIENT)
		if(NOT HAIKU)
			find_package(X11 REQUIRED)
		endif(NOT HAIKU)

		set(OPENGL_GL_PREFERENCE "LEGACY" CACHE STRING
			"See CMake Policy CMP0072 for reference. GLVND is broken on some nvidia setups")
		set(OpenGL_GL_PREFERENCE ${OPENGL_GL_PREFERENCE})

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
		check_library_exists(rt clock_gettime "" HAVE_LIBRT)
		if (HAVE_LIBRT)
			set(PLATFORM_LIBS -lrt ${PLATFORM_LIBS})
		endif(HAVE_LIBRT)
	endif(APPLE)

	if(NOT HAIKU AND NOT APPLE)
	# This way Xxf86vm is found on OpenBSD too
		find_library(XXF86VM_LIBRARY Xxf86vm)
		mark_as_advanced(XXF86VM_LIBRARY)
		set(CLIENT_PLATFORM_LIBS ${CLIENT_PLATFORM_LIBS} ${XXF86VM_LIBRARY})
	endif(NOT HAIKU AND NOT APPLE)

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
add_subdirectory(content)
add_subdirectory(database)
add_subdirectory(gui)
add_subdirectory(mapgen)
add_subdirectory(network)
add_subdirectory(script)
add_subdirectory(unittest)
add_subdirectory(util)
add_subdirectory(irrlicht_changes)
add_subdirectory(server)

set(common_SRCS
	${database_SRCS}
	${mapgen_SRCS}
	${server_SRCS}
	${content_SRCS}
	ban.cpp
	chat.cpp
	clientiface.cpp
	collision.cpp
	content_mapnode.cpp
	content_nodemeta.cpp
	content_sao.cpp
	convert_json.cpp
	craftdef.cpp
	debug.cpp
	defaultsettings.cpp
	emerge.cpp
	environment.cpp
	face_position_cache.cpp
	filesys.cpp
	genericobject.cpp
	gettext.cpp
	httpfetch.cpp
	hud.cpp
	inventory.cpp
	inventorymanager.cpp
	itemdef.cpp
	itemstackmetadata.cpp
	light.cpp
	log.cpp
	main.cpp
	map.cpp
	map_settings_manager.cpp
	mapblock.cpp
	mapnode.cpp
	mapsector.cpp
	metadata.cpp
	modchannels.cpp
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
	raycast.cpp
	reflowscan.cpp
	remoteplayer.cpp
	rollback.cpp
	rollback_interface.cpp
	serialization.cpp
	server.cpp
	serverenvironment.cpp
	serverlist.cpp
	serverobject.cpp
	settings.cpp
	staticobject.cpp
	terminal_chat_console.cpp
	tileanimation.cpp
	tool.cpp
	translation.cpp
	version.cpp
	voxel.cpp
	voxelalgorithms.cpp
	hud.cpp
	${common_network_SRCS}
	${JTHREAD_SRCS}
	${common_SCRIPT_SRCS}
	${UTIL_SRCS}
	${UNITTEST_SRCS}
)


# This gives us the icon and file version information
if(WIN32)
	set(WINRESOURCE_FILE "${CMAKE_CURRENT_SOURCE_DIR}/../misc/winresource.rc")
	set(MINETEST_EXE_MANIFEST_FILE "${CMAKE_CURRENT_SOURCE_DIR}/../misc/minetest.exe.manifest")
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
		SET(extra_windows_SRCS ${CMAKE_CURRENT_BINARY_DIR}/winresource_rc.o)
	else(MINGW) # Probably MSVC
		set(extra_windows_SRCS ${WINRESOURCE_FILE} ${MINETEST_EXE_MANIFEST_FILE})
	endif(MINGW)
endif()


# Client sources
if (BUILD_CLIENT)
	add_subdirectory(client)
endif(BUILD_CLIENT)

set(client_SRCS
	${client_SRCS}
	${common_SRCS}
	${gui_SRCS}
	${client_network_SRCS}
	${client_irrlicht_changes_SRCS}
	${client_SCRIPT_SRCS}
	${UNITTEST_CLIENT_SRCS}
)
list(SORT client_SRCS)

# Server sources
set(server_SRCS
	${common_SRCS}
)
list(SORT server_SRCS)

# Avoid source_group on broken CMake version.
# see issue #7074 #7075
if (CMAKE_VERSION VERSION_GREATER 3.8.1)
	source_group(TREE ${PROJECT_SOURCE_DIR} PREFIX "Source Files" FILES ${client_SRCS})
	source_group(TREE ${PROJECT_SOURCE_DIR} PREFIX "Source Files" FILES ${server_SRCS})
endif()

include_directories(
	${PROJECT_BINARY_DIR}
	${PROJECT_SOURCE_DIR}
	${IRRLICHT_INCLUDE_DIR}
	${ZLIB_INCLUDE_DIR}
	${CMAKE_BUILD_TYPE}
	${PNG_INCLUDE_DIR}
	${SOUND_INCLUDE_DIRS}
	${SQLITE3_INCLUDE_DIR}
	${LUA_INCLUDE_DIR}
	${GMP_INCLUDE_DIR}
	${JSON_INCLUDE_DIR}
	${X11_INCLUDE_DIR}
	${PROJECT_SOURCE_DIR}/script
)

if(USE_GETTEXT)
	include_directories(${GETTEXT_INCLUDE_DIR})
endif()

if(USE_FREETYPE)
	include_directories(${FREETYPE_INCLUDE_DIRS})
endif()

if(USE_CURL)
	include_directories(${CURL_INCLUDE_DIR})
endif()


set(EXECUTABLE_OUTPUT_PATH "${CMAKE_SOURCE_DIR}/bin")


if(BUILD_CLIENT)
	add_executable(${PROJECT_NAME} ${client_SRCS} ${extra_windows_SRCS})
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
	if(USE_GETTEXT)
		target_link_libraries(
			${PROJECT_NAME}
			${GETTEXT_LIBRARY}
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
		)
	endif()
	if (USE_CURSES)
		target_link_libraries(${PROJECT_NAME} ${CURSES_LIBRARIES})
	endif()
	if (USE_POSTGRESQL)
		target_link_libraries(${PROJECT_NAME} ${POSTGRESQL_LIBRARY})
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
	add_executable(${PROJECT_NAME}server ${server_SRCS} ${extra_windows_SRCS})
	add_dependencies(${PROJECT_NAME}server GenerateVersion)
	target_link_libraries(
		${PROJECT_NAME}server
		${ZLIB_LIBRARIES}
		${SQLITE3_LIBRARY}
		${JSON_LIBRARY}
		${LUA_LIBRARY}
		${GMP_LIBRARY}
		${PLATFORM_LIBS}
	)
	set_target_properties(${PROJECT_NAME}server PROPERTIES
			COMPILE_DEFINITIONS "SERVER")
	if (USE_GETTEXT)
		target_link_libraries(${PROJECT_NAME}server ${GETTEXT_LIBRARY})
	endif()
	if (USE_CURSES)
		target_link_libraries(${PROJECT_NAME}server ${CURSES_LIBRARIES})
	endif()
	if (USE_POSTGRESQL)
		target_link_libraries(${PROJECT_NAME}server ${POSTGRESQL_LIBRARY})
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

# Blacklisted locales that don't work.
# see issue #4638
set(GETTEXT_BLACKLISTED_LOCALES
	be
	he
	ko
	ky
	zh_CN
	zh_TW
)

option(APPLY_LOCALE_BLACKLIST "Use a blacklist to avoid broken locales" TRUE)

if (GETTEXT_FOUND AND APPLY_LOCALE_BLACKLIST)
	set(GETTEXT_USED_LOCALES "")
	foreach(LOCALE ${GETTEXT_AVAILABLE_LOCALES})
		if (NOT ";${GETTEXT_BLACKLISTED_LOCALES};" MATCHES ";${LOCALE};")
			list(APPEND GETTEXT_USED_LOCALES ${LOCALE})
		endif()
	endforeach()
	message(STATUS "Locale blacklist applied; Locales used: ${GETTEXT_USED_LOCALES}")
endif()

# Set some optimizations and tweaks

include(CheckCXXCompilerFlag)

if(MSVC)
	# Visual Studio
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /D WIN32_LEAN_AND_MEAN /MP")
	# EHa enables SEH exceptions (used for catching segfaults)
	set(CMAKE_CXX_FLAGS_RELEASE "/EHa /Ox /GL /FD /MD /GS- /Zi /fp:fast /D NDEBUG /D _HAS_ITERATOR_DEBUGGING=0 /TP")
	if(CMAKE_SIZEOF_VOID_P EQUAL 4)
		set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} /arch:SSE")
	endif()
	#set(CMAKE_EXE_LINKER_FLAGS_RELEASE "/LTCG /NODEFAULTLIB:\"libcmtd.lib\" /NODEFAULTLIB:\"libcmt.lib\"")
	set(CMAKE_EXE_LINKER_FLAGS_RELEASE "/LTCG /INCREMENTAL:NO /DEBUG /OPT:REF /OPT:ICF")


	set(CMAKE_EXE_LINKER_FLAGS_RELEASE "${CMAKE_EXE_LINKER_FLAGS_RELEASE} /SUBSYSTEM:WINDOWS /ENTRY:mainCRTStartup")


	set(CMAKE_CXX_FLAGS_SEMIDEBUG "/MDd /Zi /Ob0 /O1 /RTC1")

	# Debug build doesn't catch exceptions by itself
	# Add some optimizations because otherwise it's VERY slow
	set(CMAKE_CXX_FLAGS_DEBUG "/MDd /Zi /Ob0 /Od /RTC1")

	# Flags for C files (sqlite)
	# /MD = dynamically link to MSVCRxxx.dll
	set(CMAKE_C_FLAGS_RELEASE "/O2 /Ob2 /MD")
else()
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")
	# Probably GCC
	if(APPLE)
		SET(CMAKE_EXE_LINKER_FLAGS  "${CMAKE_EXE_LINKER_FLAGS} -pagezero_size 10000 -image_base 100000000" )
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

	if(WIN32 AND NOT ZLIBWAPI_DLL AND CMAKE_SIZEOF_VOID_P EQUAL 4)
		set(OTHER_FLAGS "${OTHER_FLAGS} -DWIN32_NO_ZLIB_WINAPI")
		message(WARNING "Defaulting to cdecl for zlib on win32 because ZLIBWAPI_DLL"
			" isn't set, ensure that ZLIBWAPI_DLL is set if you want stdcall.")
	endif()

	if(MINGW)
		set(OTHER_FLAGS "${OTHER_FLAGS} -mthreads -fexceptions")
		set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DWIN32_LEAN_AND_MEAN")
	endif()

	set(CMAKE_CXX_FLAGS_RELEASE "-DNDEBUG ${RELEASE_WARNING_FLAGS} ${WARNING_FLAGS} ${OTHER_FLAGS} -Wall -pipe -funroll-loops")
	if(CMAKE_SYSTEM_NAME MATCHES "(Darwin|BSD|DragonFly)")
		set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -Os")
	else()
		set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -O3 -ffast-math -fomit-frame-pointer")
	endif(CMAKE_SYSTEM_NAME MATCHES "(Darwin|BSD|DragonFly)")
	set(CMAKE_CXX_FLAGS_SEMIDEBUG "-g -O1 -Wall -Wabi ${WARNING_FLAGS} ${OTHER_FLAGS}")
	set(CMAKE_CXX_FLAGS_DEBUG "-g -O0 -Wall -Wabi ${WARNING_FLAGS} ${OTHER_FLAGS}")

	if(USE_GPROF)
		set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -pg")
	endif()

	if(MINGW)
		set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -mwindows")
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
	if(LUA_DLL)
		install(FILES ${LUA_DLL} DESTINATION ${BINDIR})
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
		foreach(LOCALE ${GETTEXT_USED_LOCALES})
			set_mo_paths(MO_BUILD_PATH MO_DEST_PATH ${LOCALE})
			set(MO_BUILD_PATH "${MO_BUILD_PATH}/${PROJECT_NAME}.mo")
			install(FILES ${MO_BUILD_PATH} DESTINATION ${MO_DEST_PATH})
		endforeach()
	endif()

	if(USE_FREETYPE)
		install(DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/../fonts" DESTINATION "${SHAREDIR}"
				FILES_MATCHING PATTERN "*.ttf" PATTERN "*.txt")
	else()
		install(DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/../fonts" DESTINATION "${SHAREDIR}"
				FILES_MATCHING PATTERN "*.png" PATTERN "*.xml")
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

	foreach(LOCALE ${GETTEXT_USED_LOCALES})
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
