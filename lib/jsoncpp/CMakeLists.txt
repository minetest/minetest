if(NOT MSVC)
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")
endif()

add_library(jsoncpp jsoncpp.cpp)
target_link_libraries(jsoncpp)

