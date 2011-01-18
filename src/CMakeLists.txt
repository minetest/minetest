project(minetest)
cmake_minimum_required( VERSION 2.6 )

if(RUN_IN_PLACE)
	add_definitions ( -DRUN_IN_PLACE )
endif(RUN_IN_PLACE)

# Use cmake_config.h
add_definitions ( -DUSE_CMAKE_CONFIG_H )

if(WIN32)
	# Windows
	# Surpress some warnings
	add_definitions ( /D "_CRT_SECURE_NO_DEPRECATE" /W1 )
	# Zlib stuff
	set(ZLIB_INCLUDE_DIR "${PROJECT_SOURCE_DIR}/../../zlib/zlib-1.2.5"
			CACHE PATH "Zlib include directory")
	set(ZLIB_LIBRARIES "${PROJECT_SOURCE_DIR}/../../zlib125dll/dll32/zlibwapi.lib"
			CACHE PATH "Path to zlibwapi.lib")
	set(ZLIB_DLL "${PROJECT_SOURCE_DIR}/../../zlib125dll/dll32/zlibwapi.dll"
			CACHE PATH "Path to zlibwapi.dll (for installation)")
else()
	# Unix probably
	if(BUILD_CLIENT)
		find_package(X11 REQUIRED)
		find_package(OpenGL REQUIRED)
		find_package(JPEG REQUIRED)
		find_package(BZip2 REQUIRED)
		find_package(PNG REQUIRED)
	endif(BUILD_CLIENT)
	find_package(ZLIB REQUIRED)
	set(PLATFORM_LIBS -lpthread)
	set(CLIENT_PLATFORM_LIBS -lXxf86vm)
endif()

configure_file(
	"${PROJECT_SOURCE_DIR}/cmake_config.h.in"
	"${PROJECT_BINARY_DIR}/cmake_config.h"
)

set(minetest_SRCS
	porting.cpp
	guiMessageMenu.cpp
	materials.cpp
	guiTextInputMenu.cpp
	guiInventoryMenu.cpp
	irrlichtwrapper.cpp
	guiPauseMenu.cpp
	defaultsettings.cpp
	mapnode.cpp
	tile.cpp
	voxel.cpp
	mapblockobject.cpp
	inventory.cpp
	debug.cpp
	serialization.cpp
	light.cpp
	filesys.cpp
	connection.cpp
	environment.cpp
	client.cpp
	server.cpp
	socket.cpp
	mapblock.cpp
	mapsector.cpp
	heightmap.cpp
	map.cpp
	player.cpp
	utility.cpp
	main.cpp
	test.cpp
)

set(minetestserver_SRCS
	porting.cpp
	materials.cpp
	defaultsettings.cpp
	mapnode.cpp
	voxel.cpp
	mapblockobject.cpp
	inventory.cpp
	debug.cpp
	serialization.cpp
	light.cpp
	filesys.cpp
	connection.cpp
	environment.cpp
	server.cpp
	socket.cpp
	mapblock.cpp
	mapsector.cpp
	heightmap.cpp
	map.cpp
	player.cpp
	utility.cpp
	servermain.cpp
	test.cpp
)

include_directories(
	${PROJECT_BINARY_DIR}
	${IRRLICHT_INCLUDE_DIR}
	${ZLIB_INCLUDE_DIR}
	${CMAKE_BUILD_TYPE}
	"${PROJECT_SOURCE_DIR}/jthread"
	${PNG_INCLUDE_DIR}
)

set(EXECUTABLE_OUTPUT_PATH ../bin)

if(BUILD_CLIENT)
	add_executable(minetest ${minetest_SRCS})
	target_link_libraries(
		minetest
		${ZLIB_LIBRARIES}
		${IRRLICHT_LIBRARY}
		${OPENGL_LIBRARIES}
		${JPEG_LIBRARIES}
		${BZIP2_LIBRARIES}
		${PNG_LIBRARIES}
		${X11_LIBRARIES}
		jthread
		${PLATFORM_LIBS}
		${CLIENT_PLATFORM_LIBS}
	)
endif(BUILD_CLIENT)

if(BUILD_SERVER)
	add_executable(minetestserver ${minetestserver_SRCS})
	target_link_libraries(
		minetestserver
		${ZLIB_LIBRARIES}
		jthread
		${PLATFORM_LIBS}
	)
endif(BUILD_SERVER)

#
# Set some optimizations and tweaks
#

if(MSVC)
	# Visual Studio

	# EHa enables SEH exceptions (used for catching segfaults)
	set(CMAKE_CXX_FLAGS_RELEASE "/EHa /MD /O2 /Ob2 /Oi /Ot /Oy /GL /FD /MT /GS- /arch:SSE /fp:fast /D NDEBUG")
	set(CMAKE_CXX_FLAGS_DEBUG "/MDd /Zi /Ob0 /Od /RTC1")
	
	if(BUILD_SERVER)
		set_target_properties(minetestserver PROPERTIES
				COMPILE_FLAGS "/D SERVER")
	endif(BUILD_SERVER)

else()
	# Probably GCC
	
	if(WARN_ALL)
		set(WARNING_FLAGS "-Wall")
	else()
		set(WARNING_FLAGS "")
	endif()

	set(CMAKE_CXX_FLAGS_RELEASE "-DNDEBUG ${WARNING_FLAGS} -O3 -ffast-math -Wall -fomit-frame-pointer -pipe -funroll-loops")
	set(CMAKE_CXX_FLAGS_DEBUG "-g -O1 -Wall")
	
	if(BUILD_SERVER)
		set_target_properties(minetestserver PROPERTIES
				COMPILE_FLAGS "-DSERVER")
	endif(BUILD_SERVER)

endif()

#MESSAGE(STATUS "CMAKE_CXX_FLAGS_RELEASE=${CMAKE_CXX_FLAGS_RELEASE}")
#MESSAGE(STATUS "CMAKE_CXX_FLAGS_DEBUG=${CMAKE_CXX_FLAGS_DEBUG}")

#
# Installation
#

if(BUILD_CLIENT)
	install(TARGETS minetest DESTINATION ${BINDIR})

	file(GLOB images "${CMAKE_CURRENT_SOURCE_DIR}/../data/*.png")

	install(FILES ${images} DESTINATION ${DATADIR})

	if(WIN32)
		if(DEFINED IRRLICHT_DLL)
			install(FILES ${IRRLICHT_DLL} DESTINATION ${BINDIR})
		endif()
		if(DEFINED ZLIB_DLL)
			install(FILES ${ZLIB_DLL} DESTINATION ${BINDIR})
		endif()
	endif()
endif(BUILD_CLIENT)

if(BUILD_SERVER)
	install(TARGETS minetestserver DESTINATION ${BINDIR})
endif(BUILD_SERVER)

# Subdirectories

add_subdirectory(jthread)

#end
