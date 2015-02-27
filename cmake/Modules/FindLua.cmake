
option(ENABLE_LUAJIT "Enable LuaJIT support" TRUE)
mark_as_advanced(LUA_LIBRARY LUA_INCLUDE_DIR)
set(USE_LUAJIT FALSE)

if(ENABLE_LUAJIT)
	find_library(LUA_LIBRARY luajit
			NAMES luajit-5.1)
	find_path(LUA_INCLUDE_DIR luajit.h
		NAMES luajit.h
		PATH_SUFFIXES luajit-2.0)
	if(LUA_LIBRARY AND LUA_INCLUDE_DIR)
		set(USE_LUAJIT TRUE)
	endif()
else()
	message (STATUS "LuaJIT detection disabled! (ENABLE_LUAJIT=0)")
endif()

if(NOT USE_LUAJIT)
	message(STATUS "LuaJIT not found, using bundled Lua.")
	set(LUA_LIBRARY "lua")
	set(LUA_INCLUDE_DIR "${PROJECT_SOURCE_DIR}/lua/src")
	add_subdirectory(lua)
endif()

