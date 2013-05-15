add_subdirectory(common)
add_subdirectory(cpp_api)
add_subdirectory(lua_api)

set(SCRIPT_SRCS 
	${SCRIPT_COMMON_SRCS}
	${SCRIPT_CPP_API_SRCS}
	${SCRIPT_LUA_API_SRCS}
	PARENT_SCOPE)