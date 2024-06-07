cmake_minimum_required(VERSION 3.12)

project(tiniergltf
  VERSION 1.0.0
  LANGUAGES CXX
)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_library(tiniergltf OBJECT tiniergltf.hpp)
add_library(tiniergltf::tiniergltf ALIAS tiniergltf)
set_target_properties(tiniergltf PROPERTIES LINKER_LANGUAGE CXX)

target_include_directories(tiniergltf
  INTERFACE
    "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>"
	"${JSON_INCLUDE_DIR}" # Set in FindJson.cmake
  "${CMAKE_SOURCE_DIR}/src" # util/base64.h
)

target_link_libraries(tiniergltf)
