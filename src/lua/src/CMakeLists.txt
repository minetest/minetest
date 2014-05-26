set(LUA_CORE_SRC
	lapi.c
	lauxlib.c
	lbaselib.c
        lbitlib.c
	lcode.c
        lcorolib.c
        lctype.c
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

add_library(lua STATIC ${LUA_CORE_SRC})
target_link_libraries(lua ${LIBS})
set(LUA_STATIC_LIB lua)
set(LUA_LIBS lua)

set_target_properties(${LUA_LIBS} PROPERTIES
	CLEAN_DIRECT_OUTPUT 1
)

