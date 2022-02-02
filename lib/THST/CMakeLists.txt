cmake_minimum_required(VERSION 3.5..3.18)

# version range does not set policies until version 3.12
if(${CMAKE_VERSION} VERSION_LESS 3.12)
	cmake_policy(VERSION ${CMAKE_MAJOR_VERSION}.${CMAKE_MINOR_VERSION})
endif()

project(THST
	DESCRIPTION "Templated hierarchical spatial trees designed for high performance and hierarchical spatial partitioning use cases."
	LANGUAGES CXX
)

add_library(THST INTERFACE)
add_library(THST::THST ALIAS THST)

target_include_directories(THST INTERFACE ${PROJECT_SOURCE_DIR})

if(CMAKE_VERSION VERSION_GREATER 3.7)
	target_compile_features(THST INTERFACE cxx_std_11)
endif()

if(CMAKE_PROJECT_NAME STREQUAL PROJECT_NAME)
	add_subdirectory(tests)
endif()

