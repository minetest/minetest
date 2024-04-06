cmake_minimum_required(VERSION 3.12)

project(Irrlicht LANGUAGES CXX)

message(STATUS "*** Building IrrlichtMt ***")

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if(NOT CMAKE_BUILD_TYPE)
	set(CMAKE_BUILD_TYPE Release CACHE STRING "Build type: Debug or Release" FORCE)
endif()

# FIXME: tests need to be moved to MT if we want to keep them

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")
#enable_testing()
add_subdirectory(src)
#add_subdirectory(test)

#option(BUILD_EXAMPLES "Build example applications" FALSE)
#if(BUILD_EXAMPLES)
#	add_subdirectory(examples)
#endif()
