set(libserver_SRCS
	${CMAKE_CURRENT_SOURCE_DIR}/libserver.cpp
)

add_library(${PROJECT_NAME}api SHARED ${libserver_SRCS})