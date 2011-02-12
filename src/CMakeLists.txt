project(minetest)
cmake_minimum_required( VERSION 2.6 )

if(RUN_IN_PLACE)
	add_definitions ( -DRUN_IN_PLACE )
endif(RUN_IN_PLACE)

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
	#set(CLIENT_PLATFORM_LIBS -lXxf86vm)
	# This way Xxf86vm is found on OpenBSD too
	find_library(XXF86VM_LIBRARY, Xxf86vm)
	set(CLIENT_PLATFORM_LIBS ${XXF86VM_LIBRARY})
endif()

configure_file(
	"${PROJECT_SOURCE_DIR}/cmake_config.h.in"
	"${PROJECT_BINARY_DIR}/cmake_config.h"
)

set(common_SRCS
	noise.cpp
	mineral.cpp
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
	map.cpp
	player.cpp
	utility.cpp
	test.cpp
)

set(minetest_SRCS
	${common_SRCS}
	guiMainMenu.cpp
	guiMessageMenu.cpp
	guiTextInputMenu.cpp
	guiInventoryMenu.cpp
	guiPauseMenu.cpp
	irrlichtwrapper.cpp
	client.cpp
	tile.cpp
	main.cpp
)

set(minetestserver_SRCS
	${common_SRCS}
	servermain.cpp
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

set(JTHREAD_LIBRARIES "jthread")
set(JTHREAD_SRCS "")

if(BUILD_CLIENT)
	add_executable(minetest ${minetest_SRCS} ${JTHREAD_SRCS})
	target_link_libraries(
		minetest
		${ZLIB_LIBRARIES}
		${IRRLICHT_LIBRARY}
		${OPENGL_LIBRARIES}
		${JPEG_LIBRARIES}
		${BZIP2_LIBRARIES}
		${PNG_LIBRARIES}
		${X11_LIBRARIES}
		${JTHREAD_LIBRARIES}
		${PLATFORM_LIBS}
		${CLIENT_PLATFORM_LIBS}
	)
endif(BUILD_CLIENT)

if(BUILD_SERVER)
	add_executable(minetestserver ${minetestserver_SRCS} ${JTHREAD_SRCS})
	target_link_libraries(
		minetestserver
		${ZLIB_LIBRARIES}
		${JTHREAD_LIBRARIES}
		${PLATFORM_LIBS}
	)
endif(BUILD_SERVER)

#
# Set some optimizations and tweaks
#

if(MSVC)
	# Visual Studio

	# EHa enables SEH exceptions (used for catching segfaults)
	set(CMAKE_CXX_FLAGS_RELEASE "/EHa /MD /O2 /Ob2 /Oi /Ot /Oy /GL /FD /MT /GS- /arch:SSE /fp:fast /D NDEBUG /D _HAS_ITERATOR_DEBUGGING=0 /TP")
	set(CMAKE_EXE_LINKER_FLAGS_RELEASE "/LTCG /NODEFAULTLIB:\"libcmtd.lib\"")

	# Debug build doesn't catch exceptions by itself
	# Add some optimizations because otherwise it's VERY slow
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
