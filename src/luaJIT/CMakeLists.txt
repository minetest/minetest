#
# LuaJIT 2.0.x
#
cmake_minimum_required(VERSION 2.4 FATAL_ERROR)

project(luaJIT C)

set(COMMON_CFLAGS)
set(COMMON_LDFLAGS)
set(LIBS)

include_directories ( ${CMAKE_CURRENT_BINARY_DIR} dynasm src src/host )

set (LJVM_MODE elfasm)
if(APPLE)
	set(DEFAULT_POSIX TRUE)
	set(DEFAULT_DLOPEN ON)
	set (LJVM_MODE machasm)
	# use this on Mac OS X 10.3-
	option(LUAJIT_USE_MACOSX "Mac OS X 10.3-" OFF)
elseif(CYGWIN)
	set(DEFAULT_POSIX TRUE)
elseif(UNIX)
	set(DEFAULT_POSIX TRUE)
elseif(WIN32)
	set(LUAJIT_WIN TRUE)
	set(COMMON_CFLAGS "${COMMON_CFLAGS} -DLUA_BUILD_AS_DLL")
	set (LJVM_MODE coffasm)
else()
	set(DEFAULT_ANSI TRUE)
endif()

if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
	set(COMMON_LDFLAGS "${COMMON_LDFLAGS} -Wl,-E -lm")
	set(DEFAULT_DLOPEN ON)
endif()

if(WIN32)
	#set(BUILD_STATIC OFF)
	set(BUILD_STATIC ON)
else()
	#option(BUILD_STATIC "build static library" ON)
	set(BUILD_STATIC ON)
endif()

if(DEFAULT_DLOPEN)
	option(LUAJIT_USE_DLOPEN "Enable dlopen support." ON)
else()
	option(LUAJIT_USE_DLOPEN "Enable dlopen support." OFF)
endif()
mark_as_advanced(LUAJIT_USE_DLOPEN)

if(DEFAULT_POSIX)
	list ( APPEND LIBS m )
else()
endif()

if(DEFAULT_ANSI)
	option(LUAJIT_ANSI "Disable non-ansi features." ON)
else()
	option(LUAJIT_ANSI "Disable non-ansi features." OFF)
endif()
mark_as_advanced(LUAJIT_ANSI)

option ( LUAJIT_DISABLE_FFI "Disable FFI." OFF )
option ( LUAJIT_DISABLE_JIT "Disable JIT." OFF )

#
# Lua version
#
set(LUAJIT_VERSION_MAJOR 2)
set(LUAJIT_VERSION_MINOR 0)
set(LUAJIT_VERSION_PATCH 1)
set(LUAJIT_VERSION
				"${LUAJIT_VERSION_MAJOR}.${LUAJIT_VERSION_MINOR}.${LUAJIT_VERSION_PATCH}")
set(LUAJIT_SOVERSION
				"${LUAJIT_VERSION_MAJOR}")

#
# libs & cflags
#

# For "Mac OS X 10.3-"
if(LUAJIT_USE_MACOSX)
	set(COMMON_CFLAGS "${COMMON_CFLAGS} -DLUA_USE_MACOSX")
	set(LUAJIT_USE_DLOPEN FALSE)
endif(LUAJIT_USE_MACOSX)

if(LUAJIT_USE_DLOPEN)
	set(COMMON_CFLAGS "${COMMON_CFLAGS} -DLUA_USE_DLOPEN")
	if(NOT APPLE)
		set(COMMON_LDFLAGS "${COMMON_LDFLAGS} -ldl ")
	endif(NOT APPLE)
endif(LUAJIT_USE_DLOPEN)

if(LUAJIT_ANSI)
	set(COMMON_CFLAGS "${COMMON_CFLAGS} -DLUA_ANSI")
endif(LUAJIT_ANSI)

#
# COMMON_CFLAGS has no effect without this line
#
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${COMMON_CFLAGS}")

set(CMAKE_EXE_LINKERS_FLAG "${CMAKE_EXE_LINKERS_FLAG} ${COMMON_LDFLAGS}")

#
# standard flags to use for each build type.
#
if(CMAKE_COMPILER_IS_GNUCC)
	set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -pipe -Wall -Wextra -Wshadow -W -pedantic -std=gnu99")
	set(CMAKE_C_FLAGS_RELEASE        "${CMAKE_C_FLAGS_RELEASE}     -O2")
	set(CMAKE_C_FLAGS_DEBUG          "${CMAKE_C_FLAGS_DEBUG}       -O0 -g")
	set(CMAKE_C_FLAGS_PROFILE        "${CMAKE_C_FLAGS_PROFILE}     -O1 -g")
	set(CMAKE_C_FLAGS_RELWITHDEBINFO "${CMAKE_C_FLAGS_WITHDEBINFO} -O2 -g")
endif(CMAKE_COMPILER_IS_GNUCC)

set(LUAJIT_LIB_SRC
	src/lib_base.c
	src/lib_bit.c
	src/lib_debug.c
	src/lib_ffi.c
	src/lib_io.c
	src/lib_jit.c
	src/lib_math.c
	src/lib_os.c
	src/lib_package.c
	src/lib_string.c
	src/lib_table.c
)

# Build minilua

add_executable ( minilua src/host/minilua.c )
target_link_libraries ( minilua ${LIBS} )

# Dynasm
set ( DASM ${CMAKE_CURRENT_SOURCE_DIR}/dynasm/dynasm.lua )
set ( DASM_T ${CMAKE_CURRENT_SOURCE_DIR}/src/host/buildvm_arch.h )
set ( DASM_DASC ${CMAKE_CURRENT_SOURCE_DIR}/src/vm_x86.dasc )

set ( DASM_FLAGS -LN )

if(WIN32)
	set ( DASM_FLAGS ${DASM_FLAGS} -D WIN )
endif(WIN32)
if ("${CMAKE_SIZEOF_VOID_P}" EQUAL "8")
	set ( DASM_FLAGS ${DASM_FLAGS} -D P64 )
endif()
if(NOT LUAJIT_DISABLE_FFI)
	set ( DASM_FLAGS ${DASM_FLAGS} -D FFI )
endif(NOT LUAJIT_DISABLE_FFI)
if(NOT LUAJIT_DISABLE_JIT)
	set ( DASM_FLAGS ${DASM_FLAGS} -D JIT )
endif(NOT LUAJIT_DISABLE_JIT)

# Generate buildvm arch header

add_custom_command(OUTPUT ${DASM_T}
  COMMAND minilua ${DASM} ${DASM_FLAGS} -o ${DASM_T} ${DASM_DASC}
  WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/dynasm
  DEPENDS minilua
)

# Buildvm

set(BUILDVM_SRC
	src/host/buildvm.c
	src/host/buildvm_asm.c
	src/host/buildvm_fold.c
	src/host/buildvm_lib.c
	src/host/buildvm_peobj.c
)

add_executable ( buildvm ${BUILDVM_SRC} ${DASM_T} )

macro(add_buildvm_target _target _mode)
  add_custom_command(OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/${_target}
    COMMAND buildvm ARGS -m ${_mode} -o ${CMAKE_CURRENT_BINARY_DIR}/${_target} ${ARGN}
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    DEPENDS buildvm ${ARGN}
  )
endmacro(add_buildvm_target)

if (MSVC)
  add_buildvm_target ( lj_vm.obj peobj )
  set (LJ_VM_SRC ${CMAKE_CURRENT_BINARY_DIR}/lj_vm.obj)
else ()
  add_buildvm_target ( lj_vm.s ${LJVM_MODE} )
  set (LJ_VM_SRC ${CMAKE_CURRENT_BINARY_DIR}/lj_vm.s)
endif ()

add_buildvm_target ( lj_bcdef.h     bcdef   ${LUAJIT_LIB_SRC} )
add_buildvm_target ( lj_ffdef.h     ffdef   ${LUAJIT_LIB_SRC} )
add_buildvm_target ( lj_libdef.h    libdef  ${LUAJIT_LIB_SRC} )
add_buildvm_target ( lj_recdef.h    recdef  ${LUAJIT_LIB_SRC} )
add_buildvm_target ( jit/vmdef.lua  libvm   ${LUAJIT_LIB_SRC} )
add_buildvm_target ( lj_folddef.h   folddef src/lj_opt_fold.c )

set(LUAJIT_LIB_HEADERS
	${CMAKE_CURRENT_BINARY_DIR}/lj_ffdef.h
	${CMAKE_CURRENT_BINARY_DIR}/lj_bcdef.h
	${CMAKE_CURRENT_BINARY_DIR}/lj_libdef.h
	${CMAKE_CURRENT_BINARY_DIR}/lj_recdef.h
	${CMAKE_CURRENT_BINARY_DIR}/lj_folddef.h
)

set(LUAJIT_LIB_SRC ${LUAJIT_LIB_SRC} src/lib_aux.c src/lib_init.c)

set(LUAJIT_CORE_SRC
	src/lj_alloc.c
	src/lj_api.c
	src/lj_asm.c
	src/lj_bc.c
	src/lj_bcread.c
	src/lj_bcwrite.c
	src/lj_carith.c
	src/lj_ccall.c
	src/lj_ccallback.c
	src/lj_cconv.c
	src/lj_cdata.c
	src/lj_char.c
	src/lj_clib.c
	src/lj_cparse.c
	src/lj_crecord.c
	src/lj_ctype.c
	src/lj_debug.c
	src/lj_dispatch.c
	src/lj_err.c
	src/lj_ffrecord.c
	src/lj_func.c
	src/lj_gc.c
	src/lj_gdbjit.c
	src/lj_ir.c
	src/lj_lex.c
	src/lj_lib.c
	src/lj_load.c
	src/lj_mcode.c
	src/lj_meta.c
	src/lj_obj.c
	src/lj_opt_dce.c
	src/lj_opt_fold.c
	src/lj_opt_loop.c
	src/lj_opt_mem.c
	src/lj_opt_narrow.c
	src/lj_opt_sink.c
	src/lj_opt_split.c
	src/lj_parse.c
	src/lj_record.c
	src/lj_snap.c
	src/lj_state.c
	src/lj_str.c
	src/lj_strscan.c
	src/lj_tab.c
	src/lj_trace.c
	src/lj_udata.c
	src/lj_vmevent.c
	src/lj_vmmath.c
)

#
# Lua library.
#
add_library(luajit STATIC ${LUAJIT_LIB_SRC} ${LUAJIT_CORE_SRC} ${LJ_VM_SRC} ${LUAJIT_LIB_HEADERS})
target_link_libraries(luajit ${LIBS})
set(LUAJIT_STATIC_LIB luajit)
set(LUAJIT_LIBS luajit)

set_target_properties(${LUAJIT_LIBS} PROPERTIES
	VERSION ${LUAJIT_VERSION}
	SOVERSION ${LUAJIT_SOVERSION}
	CLEAN_DIRECT_OUTPUT 1
)
