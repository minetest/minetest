
#ADD_SUBDIRECTORY(host)

# Lua core source files.
set(LUAJIT_CORE_SRC
	lib_base.c
	lib_math.c
	lib_bit.c
	lib_string.c
	lib_table.c
	lib_io.c
	lib_os.c
	lib_package.c
	lib_debug.c
	lib_jit.c
	lib_ffi.c
)

include_directories(${CMAKE_CURRENT_SOURCE_DIR}
				${CMAKE_CURRENT_BINARY_DIR} dynasm src src/host )

#
# Lua library.
#
add_library(luajit STATIC ${LUAJIT_CORE_SRC} ${LUAJIT_LIB_HEADERS})
target_link_libraries(lua ${LIBS})
set(LUA_STATIC_LIB lua)
set(LUA_LIBS lua)

set_target_properties(${LUA_LIBS} PROPERTIES
	VERSION ${LUA_VERSION}
	SOVERSION ${LUA_SOVERSION}
	CLEAN_DIRECT_OUTPUT 1
)

# Install library
#install(TARGETS ${LUA_LIBS}
#				RUNTIME DESTINATION bin
#				LIBRARY DESTINATION lib
#				ARCHIVE DESTINATION lib)

