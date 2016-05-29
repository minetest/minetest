set(SRCS
	${CMAKE_CURRENT_SOURCE_DIR}/clientlauncher.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/tile.cpp
)

if (USE_MUMBLE)
	list(APPEND SRCS ${CMAKE_CURRENT_SOURCE_DIR}/mumble.cpp)
endif()

# Export client_SRCS to parent
set(client_SRCS ${SRCS} PARENT_SCOPE)

