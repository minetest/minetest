project(minetest)

INCLUDE(CheckTypeSize)
INCLUDE(CheckIncludeFiles)
INCLUDE(CheckLibraryExists)

check_type_size(int SIZEOF_INT BUILTIN_TYPES_ONLY LANGUAGE CXX)
if(SIZEOF_INT LESS 4)
	message(FATAL_ERROR "Minetest will not work with int less than 32 bits wide.")
endif()

check_type_size(size_t SIZEOF_SIZE_T LANGUAGE CXX)
if(SIZEOF_SIZE_T LESS 4)
	message(FATAL_ERROR "Minetest will not work with size_t less than 32 bits wide.")
endif()

# Add custom SemiDebug build mode
set(CMAKE_CXX_FLAGS_SEMIDEBUG "-O1 -g -Wall" CACHE STRING
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


option(ENABLE_GETTEXT "Use GetText for internationalization" ${BUILD_CLIENT})
set(USE_GETTEXT FALSE)

if(ENABLE_GETTEXT)
	find_package(GettextLib)
	if(GETTEXTLIB_FOUND)
		if(WIN32)
			message(STATUS "GetText library: ${GETTEXT_LIBRARY}")
			message(STATUS "GetText DLL(s): ${GETTEXT_DLL}")
		endif()
		set(USE_GETTEXT TRUE)
		message(STATUS "GetText enabled; locales found: ${GETTEXT_AVAILABLE_LOCALES}")
	endif(GETTEXTLIB_FOUND)
else()
	mark_as_advanced(GETTEXT_INCLUDE_DIR GETTEXT_LIBRARY GETTEXT_MSGFMT)
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

# TODO: this should be removed one day, we can enable it unconditionally
option(ENABLE_GLES "Enable extra support code for OpenGL ES" FALSE)
mark_as_advanced(ENABLE_GLES)

option(ENABLE_TOUCH "Enable Touchscreen support" FALSE)
if(ENABLE_TOUCH)
	add_definitions(-DHAVE_TOUCHSCREENGUI)
endif()

if(BUILD_CLIENT)
	find_package(Freetype REQUIRED)
endif()

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
	if(CMAKE_VERSION VERSION_LESS "3.20")
		find_package(PostgreSQL QUIET)
		# Before CMake 3.20 FindPostgreSQL.cmake always looked for server includes
		# but we don't need them, so continue anyway if only those are missing.
		if(PostgreSQL_INCLUDE_DIR AND PostgreSQL_LIBRARY)
			set(PostgreSQL_FOUND TRUE)
			set(PostgreSQL_INCLUDE_DIRS ${PostgreSQL_INCLUDE_DIR})
			set(PostgreSQL_LIBRARIES ${PostgreSQL_LIBRARY})
		endif()
	else()
		find_package(PostgreSQL)
	endif()

	if(PostgreSQL_FOUND)
		set(USE_POSTGRESQL TRUE)
		message(STATUS "PostgreSQL backend enabled")
		# This variable is case sensitive, don't try to change it to POSTGRESQL_INCLUDE_DIR
		message(STATUS "PostgreSQL includes: ${PostgreSQL_INCLUDE_DIRS}")
		include_directories(${PostgreSQL_INCLUDE_DIRS})
	else()
		message(STATUS "PostgreSQL not found!")
	endif()
endif(ENABLE_POSTGRESQL)

option(ENABLE_LEVELDB "Enable LevelDB backend" TRUE)
set(USE_LEVELDB FALSE)

if(ENABLE_LEVELDB)
	find_library(LEVELDB_LIBRARY NAMES leveldb libleveldb)
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


OPTION(ENABLE_PROMETHEUS "Enable prometheus client support" FALSE)
set(USE_PROMETHEUS FALSE)

if(ENABLE_PROMETHEUS)
	find_path(PROMETHEUS_CPP_INCLUDE_DIR NAMES prometheus/counter.h)
	find_library(PROMETHEUS_PULL_LIBRARY NAMES prometheus-cpp-pull)
	find_library(PROMETHEUS_CORE_LIBRARY NAMES prometheus-cpp-core)
	if(PROMETHEUS_CPP_INCLUDE_DIR AND PROMETHEUS_PULL_LIBRARY AND PROMETHEUS_CORE_LIBRARY)
		set(PROMETHEUS_LIBRARIES ${PROMETHEUS_PULL_LIBRARY} ${PROMETHEUS_CORE_LIBRARY})
		set(USE_PROMETHEUS TRUE)
		include_directories(${PROMETHEUS_CPP_INCLUDE_DIR})
	endif(PROMETHEUS_CPP_INCLUDE_DIR AND PROMETHEUS_PULL_LIBRARY AND PROMETHEUS_CORE_LIBRARY)
endif(ENABLE_PROMETHEUS)

if(USE_PROMETHEUS)
	message(STATUS "Prometheus client enabled.")
else(USE_PROMETHEUS)
	message(STATUS "Prometheus client disabled.")
endif(USE_PROMETHEUS)

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


find_package(ZLIB REQUIRED)
find_package(Zstd REQUIRED)


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

set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)
set(PLATFORM_LIBS Threads::Threads)

if(WIN32)
	# Windows
	if(MSVC) # MSVC Specifics
		set(PLATFORM_LIBS dbghelp.lib ${PLATFORM_LIBS})
		# Surpress some useless warnings
		add_definitions ( /D "_CRT_SECURE_NO_DEPRECATE" /W1 )
		# Get M_PI to work
		add_definitions(/D "_USE_MATH_DEFINES")
		# Don't define min/max macros in minwindef.h
		add_definitions(/D "NOMINMAX")
	endif()
	set(PLATFORM_LIBS ws2_32.lib version.lib shlwapi.lib winmm.lib ${PLATFORM_LIBS})

	set(EXTRA_DLL "" CACHE FILEPATH "Optional paths to additional DLLs that should be packaged")

	# DLLs are automatically copied to the output directory by vcpkg when VCPKG_APPLOCAL_DEPS=ON
	if(NOT VCPKG_APPLOCAL_DEPS)
		set(ZLIB_DLL "" CACHE FILEPATH "Path to Zlib DLL for installation (optional)")
		set(ZSTD_DLL "" CACHE FILEPATH "Path to Zstd DLL for installation (optional)")
		if(ENABLE_SOUND)
			set(OPENAL_DLL "" CACHE FILEPATH "Path to OpenAL32.dll for installation (optional)")
			set(OGG_DLL "" CACHE FILEPATH "Path to libogg.dll for installation (optional)")
			set(VORBIS_DLL "" CACHE FILEPATH "Path to Vorbis DLLs for installation (optional)")
		endif()
		if(USE_GETTEXT)
			set(GETTEXT_DLL "" CACHE FILEPATH "Path to Intl/Iconv DLLs for installation (optional)")
		endif()
		if(USE_LUAJIT)
			set(LUA_DLL "" CACHE FILEPATH "Path to luajit-5.1.dll for installation (optional)")
		endif()
	endif()
else()
	# Unix probably
	if(BUILD_CLIENT)
		if(NOT HAIKU AND NOT APPLE)
			find_package(X11 REQUIRED)
		endif(NOT HAIKU AND NOT APPLE)
	endif()

	set(PLATFORM_LIBS ${PLATFORM_LIBS} ${CMAKE_DL_LIBS})
	if(APPLE)
		set(PLATFORM_LIBS "-framework CoreFoundation" ${PLATFORM_LIBS})
	else()
		check_library_exists(rt clock_gettime "" HAVE_LIBRT)
		if (HAVE_LIBRT)
			set(PLATFORM_LIBS -lrt ${PLATFORM_LIBS})
		endif(HAVE_LIBRT)
	endif(APPLE)

	find_library(ICONV_LIBRARY iconv)
	mark_as_advanced(ICONV_LIBRARY)
	if (ICONV_LIBRARY)
		set(PLATFORM_LIBS ${PLATFORM_LIBS} ${ICONV_LIBRARY})
	endif()

	if (HAIKU)
		set(PLATFORM_LIBS ${PLATFORM_LIBS} network)
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
add_subdirectory(benchmark)
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
	convert_json.cpp
	craftdef.cpp
	debug.cpp
	defaultsettings.cpp
	emerge.cpp
	environment.cpp
	face_position_cache.cpp
	filesys.cpp
	gettext.cpp
	httpfetch.cpp
	hud.cpp
	inventory.cpp
	inventorymanager.cpp
	itemdef.cpp
	itemstackmetadata.cpp
	light.cpp
	lighting.cpp
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
	particles.cpp
	pathfinder.cpp
	player.cpp
	porting.cpp
	profiler.cpp
	raycast.cpp
	reflowscan.cpp
	remoteplayer.cpp
	rollback.cpp
	rollback_interface.cpp
	serialization.cpp
	server.cpp
	serverenvironment.cpp
	serverlist.cpp
	settings.cpp
	staticobject.cpp
	terminal_chat_console.cpp
	texture_override.cpp
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
)

if(BUILD_UNITTESTS)
	set(common_SRCS ${common_SRCS} ${UNITTEST_SRCS})
endif()

if(BUILD_BENCHMARKS)
	set(common_SRCS ${common_SRCS} ${BENCHMARK_SRCS})
endif()

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
			DEPENDS ${WINRESOURCE_FILE} ${MINETEST_EXE_MANIFEST_FILE})
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
)

if(BUILD_UNITTESTS)
	set(client_SRCS ${client_SRCS} ${UNITTEST_CLIENT_SRCS})
endif()

if(BUILD_BENCHMARKS)
	set(client_SRCS ${client_SRCS} ${BENCHMARK_CLIENT_SRCS})
endif()

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
	${PROJECT_SOURCE_DIR}/script
)
include_directories(SYSTEM
	${ZLIB_INCLUDE_DIR}
	${ZSTD_INCLUDE_DIR}
	${SQLITE3_INCLUDE_DIR}
	${LUA_INCLUDE_DIR}
	${GMP_INCLUDE_DIR}
	${JSON_INCLUDE_DIR}
	${LUA_BIT_INCLUDE_DIR}
)

if(USE_GETTEXT)
	include_directories(${GETTEXT_INCLUDE_DIR})
endif()

if(BUILD_CLIENT)
	include_directories(SYSTEM
		${FREETYPE_INCLUDE_DIRS}
		${SOUND_INCLUDE_DIRS}
		${X11_INCLUDE_DIR}
	)
endif()

if(USE_CURL)
	include_directories(${CURL_INCLUDE_DIR})
endif()


# When cross-compiling assume the user doesn't want to run the executable anyway,
# otherwise place it in <source dir>/bin/ since Minetest can only run from there.
if(NOT CMAKE_CROSSCOMPILING)
	set(EXECUTABLE_OUTPUT_PATH "${CMAKE_SOURCE_DIR}/bin")
endif()

if(BUILD_CLIENT)
	add_executable(${PROJECT_NAME} ${client_SRCS} ${extra_windows_SRCS})
	add_dependencies(${PROJECT_NAME} GenerateVersion)
	target_link_libraries(
		${PROJECT_NAME}
		${ZLIB_LIBRARIES}
		IrrlichtMt::IrrlichtMt
		${ZSTD_LIBRARY}
		${X11_LIBRARIES}
		${SOUND_LIBRARIES}
		${SQLITE3_LIBRARY}
		${LUA_LIBRARY}
		${GMP_LIBRARY}
		${JSON_LIBRARY}
		${LUA_BIT_LIBRARY}
		${FREETYPE_LIBRARY}
		${PLATFORM_LIBS}
	)
	if(NOT USE_LUAJIT)
		set_target_properties(${PROJECT_NAME} PROPERTIES
			# This is necessary for dynamic Lua modules
			# to work when Lua is statically linked (issue #10806)
			ENABLE_EXPORTS 1
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
	if(FREETYPE_PKGCONFIG_FOUND)
		set_target_properties(${PROJECT_NAME}
			PROPERTIES
			COMPILE_FLAGS "${FREETYPE_CFLAGS_STR}"
	)
	endif()
	if (USE_CURSES)
		target_link_libraries(${PROJECT_NAME} ${CURSES_LIBRARIES})
	endif()
	if (USE_POSTGRESQL)
		target_link_libraries(${PROJECT_NAME} ${PostgreSQL_LIBRARIES})
	endif()
	if (USE_LEVELDB)
		target_link_libraries(${PROJECT_NAME} ${LEVELDB_LIBRARY})
	endif()
	if (USE_REDIS)
		target_link_libraries(${PROJECT_NAME} ${REDIS_LIBRARY})
	endif()
	if (USE_PROMETHEUS)
		target_link_libraries(${PROJECT_NAME} ${PROMETHEUS_LIBRARIES})
	endif()
	if (USE_SPATIAL)
		target_link_libraries(${PROJECT_NAME} ${SPATIAL_LIBRARY})
	endif()
	if(BUILD_BENCHMARKS)
		target_link_libraries(${PROJECT_NAME} catch2)
	endif()
endif(BUILD_CLIENT)


if(BUILD_SERVER)
	add_executable(${PROJECT_NAME}server ${server_SRCS} ${extra_windows_SRCS})
	add_dependencies(${PROJECT_NAME}server GenerateVersion)

	get_target_property(
		IRRLICHT_INCLUDES IrrlichtMt::IrrlichtMt INTERFACE_INCLUDE_DIRECTORIES)
	# Doesn't work without PRIVATE/PUBLIC/INTERFACE mode specified.
	target_include_directories(${PROJECT_NAME}server PRIVATE ${IRRLICHT_INCLUDES})
	target_link_libraries(
		${PROJECT_NAME}server
		${ZLIB_LIBRARIES}
		${ZSTD_LIBRARY}
		${SQLITE3_LIBRARY}
		${JSON_LIBRARY}
		${LUA_LIBRARY}
		${LUA_BIT_LIBRARY}
		${GMP_LIBRARY}
		${PLATFORM_LIBS}
	)
	set_target_properties(${PROJECT_NAME}server PROPERTIES
		COMPILE_DEFINITIONS "SERVER")
	if(NOT USE_LUAJIT)
		set_target_properties(${PROJECT_NAME}server PROPERTIES
			# This is necessary for dynamic Lua modules
			# to work when Lua is statically linked (issue #10806)
			ENABLE_EXPORTS 1
		)
	endif()

	if (USE_GETTEXT)
		target_link_libraries(${PROJECT_NAME}server ${GETTEXT_LIBRARY})
	endif()
	if (USE_CURSES)
		target_link_libraries(${PROJECT_NAME}server ${CURSES_LIBRARIES})
	endif()
	if (USE_POSTGRESQL)
		target_link_libraries(${PROJECT_NAME}server ${PostgreSQL_LIBRARIES})
	endif()
	if (USE_LEVELDB)
		target_link_libraries(${PROJECT_NAME}server ${LEVELDB_LIBRARY})
	endif()
	if (USE_REDIS)
		target_link_libraries(${PROJECT_NAME}server ${REDIS_LIBRARY})
	endif()
	if (USE_PROMETHEUS)
		target_link_libraries(${PROJECT_NAME}server ${PROMETHEUS_LIBRARIES})
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
	if(BUILD_BENCHMARKS)
		target_link_libraries(${PROJECT_NAME}server catch2)
	endif()
endif(BUILD_SERVER)

# Blacklisted locales that don't work.
# see issue #4638
set(GETTEXT_BLACKLISTED_LOCALES
	ar
	dv
	he
	hi
	kn
	ms_Arab
	th
)

option(APPLY_LOCALE_BLACKLIST "Use a blacklist to avoid known broken locales" TRUE)

if (GETTEXTLIB_FOUND AND APPLY_LOCALE_BLACKLIST)
	set(GETTEXT_USED_LOCALES "")
	foreach(LOCALE ${GETTEXT_AVAILABLE_LOCALES})
		if (NOT "${LOCALE}" IN_LIST GETTEXT_BLACKLISTED_LOCALES)
			list(APPEND GETTEXT_USED_LOCALES ${LOCALE})
		endif()
	endforeach()
	message(STATUS "Locale blacklist applied; Locales used: ${GETTEXT_USED_LOCALES}")
elseif (GETTEXTLIB_FOUND)
	set(GETTEXT_USED_LOCALES ${GETTEXT_AVAILABLE_LOCALES})
endif()

# Set some optimizations and tweaks

include(CheckCSourceCompiles)
include(CheckSymbolExists)

set(CMAKE_REQUIRED_INCLUDES ${LUA_INCLUDE_DIR})
if(USE_LUAJIT)
	set(CMAKE_REQUIRED_LIBRARIES ${LUA_LIBRARY})
	# LuaJIT provides exactly zero ways to determine how recent it is (the version
	# is unchanged since 2017), however it happens that string buffers were added
	# after the changes which we care about so that works as an indicator.
	# (https://github.com/LuaJIT/LuaJIT/commit/4c6b669 March 2021)
	unset(HAVE_RECENT_LJ CACHE)
	check_symbol_exists(luaopen_string_buffer "lualib.h" HAVE_RECENT_LJ)
	if(NOT HAVE_RECENT_LJ)
		string(CONCAT explanation_msg
			"You are using a relatively old version of LuaJIT. We recommend "
			"running a recent version (from git) as older ones are known not "
			"to build/work correctly in all cases.\n"
			"THIS APPLIES ESPECIALLY ON macOS OR Linux/aarch64!")
		message(WARNING ${explanation_msg})
	endif()
elseif(NOT MSVC)
	set(CMAKE_REQUIRED_LIBRARIES "")
	unset(HAVE_ATCCALL CACHE)
	# Note: we need to check the function without having the library
	#       available for linking, so check_symbol_exists won't work.
	# Incidentally this doesn't seem to work on MSVC...
	check_c_source_compiles("#include <lua.h>\nint main(){return sizeof(lua_atccall);}" HAVE_ATCCALL)
	if(NOT HAVE_ATCCALL)
		string(CONCAT explanation_msg
			"It looks like you're trying to build Minetest using a system-wide "
			"Lua installation. This is no longer supported because PUC Lua "
			"cannot interoperate with C++ correctly. Read src/unittest/test_lua.cpp "
			" for technical details.")
		message(FATAL_ERROR ${explanation_msg})
	endif()
endif()

if(MSVC)
	# Visual Studio
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /D WIN32_LEAN_AND_MEAN")
	# EHa enables SEH exceptions (used for catching segfaults)
	set(CMAKE_CXX_FLAGS_RELEASE "/EHa /Ox /MD /GS- /Zi /fp:fast /D NDEBUG /D _HAS_ITERATOR_DEBUGGING=0")
	if(CMAKE_SIZEOF_VOID_P EQUAL 4)
		set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} /arch:SSE")
	endif()

	set(CMAKE_EXE_LINKER_FLAGS_RELEASE "/INCREMENTAL:NO /DEBUG /OPT:REF /OPT:ICF /SUBSYSTEM:WINDOWS /ENTRY:mainCRTStartup")

	set(CMAKE_CXX_FLAGS_SEMIDEBUG "/MDd /Zi /Ob0 /O1 /RTC1")

	# Debug build doesn't catch exceptions by itself
	# Add some optimizations because otherwise it's VERY slow
	set(CMAKE_CXX_FLAGS_DEBUG "/MDd /Zi /Ob0 /Od /RTC1")

	# Flags for C files (sqlite)
	# /MD = dynamically link to MSVCRxxx.dll
	set(CMAKE_C_FLAGS_RELEASE "/O2 /Ob2 /MD")

	# Flags that cannot be shared between cl and clang-cl
	# https://clang.llvm.org/docs/UsersManual.html#clang-cl
	if(CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
		set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fuse-ld=lld")

		# Disable pragma-pack warning
		set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -Wno-pragma-pack")
	else()
		set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /MP")
		set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} /TP /FD /GL")
		set(CMAKE_EXE_LINKER_FLAGS_RELEASE "${CMAKE_EXE_LINKER_FLAGS_RELEASE} /LTCG")
	endif()
else()
	# GCC or compatible compilers such as Clang
	set(WARNING_FLAGS "-Wall -Wextra")
	set(WARNING_FLAGS "${WARNING_FLAGS} -Wno-unused-parameter -Wno-implicit-fallthrough")
	if(WARN_ALL)
		set(RELEASE_WARNING_FLAGS "${WARNING_FLAGS}")
	else()
		set(RELEASE_WARNING_FLAGS "")
	endif()

	if(MINGW)
		set(OTHER_FLAGS "${OTHER_FLAGS} -mthreads -fexceptions")
		set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DWIN32_LEAN_AND_MEAN")
	endif()

	# Use a safe subset of flags to speed up math calculations:
	# - we don't need errno or math exceptions
	# - we don't deal with signed zero
	set(MATH_FLAGS "-fno-math-errno -fno-trapping-math -fno-signed-zeros")

	# Enable SSE for floating point math on 32-bit x86 by default
	# reasoning see minetest issue #11810 and https://gcc.gnu.org/wiki/FloatingPointMath
	if(CMAKE_SIZEOF_VOID_P EQUAL 4)
		check_c_source_compiles("#ifndef __i686__\n#error\n#endif\nint main(){}" IS_I686)
		if(IS_I686)
			message(STATUS "Detected Intel x86: using SSE instead of x87 FPU")
			set(OTHER_FLAGS "${OTHER_FLAGS} -mfpmath=sse -msse")
		endif()
	endif()

	set(CMAKE_CXX_FLAGS_RELEASE "-DNDEBUG ${RELEASE_WARNING_FLAGS} ${OTHER_FLAGS} -pipe -funroll-loops -O3 -fomit-frame-pointer")
	if(CMAKE_SYSTEM_NAME STREQUAL "Linux"
			AND CMAKE_CXX_COMPILER_ID STREQUAL "Clang"
			AND CMAKE_CXX_COMPILER_VERSION MATCHES "^9\\.")
		# Clang 9 has broken -ffast-math on glibc
	else()
		set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} ${MATH_FLAGS}")
	endif()
	set(CMAKE_CXX_FLAGS_RELWITHDEBINFO "${CMAKE_CXX_FLAGS_RELEASE} -g")
	set(CMAKE_CXX_FLAGS_SEMIDEBUG "-g -O1 ${WARNING_FLAGS} ${OTHER_FLAGS}")
	set(CMAKE_CXX_FLAGS_DEBUG "-g -O0 ${WARNING_FLAGS} ${OTHER_FLAGS}")

	if(UNIX)
		# enable assertions for libstdc++ or libc++
		set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -Wp,-D_GLIBCXX_ASSERTIONS -Wp,-D_LIBCPP_ENABLE_ASSERTIONS=1")
	endif()
	if(USE_GPROF)
		set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -pg")
	endif()

	if(MINGW)
		set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -mwindows")
	endif()
endif()


# Installation

if(WIN32)
	if(EXTRA_DLL)
		install(FILES ${EXTRA_DLL} DESTINATION ${BINDIR})
	endif()
	if(VCPKG_APPLOCAL_DEPS)
		# Collect the dll's from the output path
		install(DIRECTORY ${EXECUTABLE_OUTPUT_PATH}/Release/
				DESTINATION ${BINDIR}
				CONFIGURATIONS Release
				FILES_MATCHING PATTERN "*.dll")
		install(DIRECTORY ${EXECUTABLE_OUTPUT_PATH}/Debug/
				DESTINATION ${BINDIR}
				CONFIGURATIONS Debug
				FILES_MATCHING PATTERN "*.dll")
		install(DIRECTORY ${EXECUTABLE_OUTPUT_PATH}/RelWithDebInfo/
				DESTINATION ${BINDIR}
				CONFIGURATIONS RelWithDebInfo
				FILES_MATCHING PATTERN "*.dll")
		install(DIRECTORY ${EXECUTABLE_OUTPUT_PATH}/MinSizeRel/
				DESTINATION ${BINDIR}
				CONFIGURATIONS MinSizeRel
				FILES_MATCHING PATTERN "*.dll")
	else()
		# Use the old-style way to install dll's
		if(BUILD_CLIENT AND USE_SOUND)
			if(OPENAL_DLL)
				install(FILES ${OPENAL_DLL} DESTINATION ${BINDIR})
			endif()
			if(OGG_DLL)
				install(FILES ${OGG_DLL} DESTINATION ${BINDIR})
			endif()
			if(VORBIS_DLL)
				install(FILES ${VORBIS_DLL} DESTINATION ${BINDIR})
			endif()
		endif()
		if(CURL_DLL)
			install(FILES ${CURL_DLL} DESTINATION ${BINDIR})
		endif()
		if(ZLIB_DLL)
			install(FILES ${ZLIB_DLL} DESTINATION ${BINDIR})
		endif()
		if(ZSTD_DLL)
			install(FILES ${ZSTD_DLL} DESTINATION ${BINDIR})
		endif()
		if(BUILD_CLIENT AND FREETYPE_DLL)
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
		if(BUILD_CLIENT AND USE_GETTEXT AND GETTEXT_DLL)
			install(FILES ${GETTEXT_DLL} DESTINATION ${BINDIR})
		endif()
	endif()

	if(BUILD_CLIENT AND IRRLICHT_DLL)
		install(FILES ${IRRLICHT_DLL} DESTINATION ${BINDIR})
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

	install(DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/../fonts" DESTINATION "${SHAREDIR}"
			FILES_MATCHING PATTERN "*.ttf" PATTERN "*.txt")
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
