
# Lua core source files.
set(LUA_CORE_SRC
	lapi.c
	lauxlib.c
	lbaselib.c
	lcode.c
	ldblib.c
	ldebug.c
	ldo.c
	ldump.c
	lfunc.c
	lgc.c
	linit.c
	liolib.c
	llex.c
	lmathlib.c
	lmem.c
	loadlib.c
	lobject.c
	lopcodes.c
	loslib.c
	lparser.c
	lstate.c
	lstring.c
	lstrlib.c
	ltable.c
	ltablib.c
	ltm.c
	lundump.c
	lvm.c
	lzio.c
)
set(LUA_LIB_HEADERS
	lua.h
	lualib.h
	lauxlib.h
	luaconf.h
)

include_directories(${CMAKE_CURRENT_SOURCE_DIR}
				${CMAKE_CURRENT_BINARY_DIR})

#
# Lua library.
#
if(BUILD_STATIC)
	add_library(lua STATIC ${LUA_CORE_SRC})
	add_target_properties(lua COMPILE_FLAGS "${COMMON_CFLAGS}")
	add_target_properties(lua LINK_FLAGS "${LD_FLAGS} ${COMMON_LDFLAGS}")
	target_link_libraries(lua ${LIBS})
	set(LUA_STATIC_LIB lua)
	set(LUA_LIBS lua)
else(BUILD_STATIC)
	add_library(lua_static STATIC ${LUA_CORE_SRC})
	add_target_properties(lua_static COMPILE_FLAGS "${COMMON_CFLAGS}")
	add_target_properties(lua_static LINK_FLAGS "${LD_FLAGS} ${COMMON_LDFLAGS}")
	target_link_libraries(lua_static ${LIBS})
	set(LUA_STATIC_LIB lua_static)

	add_library(lua SHARED ${LUA_CORE_SRC})
	add_target_properties(lua COMPILE_FLAGS "${COMMON_CFLAGS}")
	add_target_properties(lua LINK_FLAGS "${LD_FLAGS} ${COMMON_LDFLAGS}")
	target_link_libraries(lua ${LIBS})

	set(LUA_LIBS lua_static lua)
endif(BUILD_STATIC)
set_target_properties(${LUA_LIBS} PROPERTIES
	VERSION ${LUA_VERSION}
	SOVERSION ${LUA_SOVERSION}
	CLEAN_DIRECT_OUTPUT 1
)

#
# Lua compiler
#
add_executable(luac luac.c print.c)
add_target_properties(luac COMPILE_FLAGS "${COMMON_CFLAGS}")
add_target_properties(luac LINK_FLAGS "${LD_FLAGS} ${COMMON_LDFLAGS}")
target_link_libraries(luac ${LUA_STATIC_LIB} ${LIBS})

#
# Lua stand-alone interpreter
#
add_executable(lua.bin lua.c)
add_target_properties(lua.bin COMPILE_FLAGS "${COMMON_CFLAGS}")
add_target_properties(lua.bin LINK_FLAGS "${LD_FLAGS} ${COMMON_LDFLAGS}")
target_link_libraries(lua.bin lua ${LIBS})
# rename lub.bin to lua
set_target_properties(lua.bin PROPERTIES OUTPUT_NAME lua)

#
# install lua/luac & library.
#
install(TARGETS luac lua.bin ${LUA_LIBS}
				RUNTIME DESTINATION bin
				LIBRARY DESTINATION lib
				ARCHIVE DESTINATION lib)

install(FILES
	${LUA_LIB_HEADERS}
	DESTINATION include)

