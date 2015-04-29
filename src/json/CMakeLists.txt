if(MSVC)
	set(CMAKE_CXX_FLAGS_RELEASE "/MT")
endif()

add_library(jsoncpp jsoncpp.cpp)
target_link_libraries(jsoncpp)

