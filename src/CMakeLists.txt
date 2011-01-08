project(minetest)
cmake_minimum_required( VERSION 2.6 )
set ( CMAKE_BUILD_TYPE Debug )
add_definitions ( -Wall -DRUN_IN_PLACE -O2)
find_package(ZLIB REQUIRED)
find_package(X11 REQUIRED)
find_package(OpenGL REQUIRED)
find_package(JPEG REQUIRED)
find_package(BZip2 REQUIRED)

if( UNIX )
  #set( platform_SRCS some_necessary_linux_file.cpp )
else( UNIX )
  #windows
  #set( platform_SRCS dllmain.cpp stdafx.cpp )
endif( UNIX )

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

include_directories(
	${ZLIB_INCLUDE_DIR}
	${IRRLICHT_INCLUDE_DIR}
	"${PROJECT_SOURCE_DIR}/jthread"
)

set(EXECUTABLE_OUTPUT_PATH ../bin)

add_executable(minetest ${minetest_SRCS})

target_link_libraries(
	minetest
	${ZLIB_LIBRARIES}
	${IRRLICHT_LIBRARY}
	${OPENGL_LIBRARIES}
	${JPEG_LIBRARIES}
	${BZIP2_LIBRARIES}
	jthread
)

add_subdirectory(jthread)

#END
