cmake_minimum_required(VERSION 2.8)
project(test)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++0x")

set(SOURCE_FILES ../RTree.h ../rtree_detail.h ../config.h ../bbox.h ../allocator.h ../indexable.h ../QuadTree.h ../quad_tree_detail.h custom_allocator.h)

include_directories("${CMAKE_SOURCE_DIR}/../")

add_executable(thst_test ${SOURCE_FILES} main.cpp)
