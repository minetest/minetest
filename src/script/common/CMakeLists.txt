# Used by server and client
set(common_SCRIPT_COMMON_SRCS
	${CMAKE_CURRENT_SOURCE_DIR}/c_content.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/c_converter.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/c_types.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/c_internal.cpp
	PARENT_SCOPE)

# Used by client only
set(minetest_SCRIPT_COMMON_SRCS
	PARENT_SCOPE)
