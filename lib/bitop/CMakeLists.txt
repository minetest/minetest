add_library(bitop STATIC bit.cpp)
target_link_libraries(bitop)

include_directories(${LUA_INCLUDE_DIR})
