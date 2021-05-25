# Locate LuaJIT library
# This module defines
#  LUAJIT_FOUND, if false, do not try to link to Lua
#  LUA_LIBRARY, where to find the lua library
#  LUA_INCLUDE_DIR, where to find lua.h
#
# This module is similar to FindLua51.cmake except that it finds LuaJit instead.

FIND_PATH(LUA_INCLUDE_DIR luajit.h
	HINTS
	$ENV{LUA_DIR}
	PATH_SUFFIXES include/luajit-2.1 include/luajit-2.0 include/luajit-5_1-2.1 include/luajit-5_1-2.0 include luajit
	PATHS
	~/Library/Frameworks
	/Library/Frameworks
	/sw # Fink
	/opt/local # DarwinPorts
	/opt/csw # Blastwave
	/opt
)

# Test if running on vcpkg toolchain
if(DEFINED VCPKG_TARGET_TRIPLET AND DEFINED VCPKG_APPLOCAL_DEPS)
	# On vcpkg luajit is 'lua51' and normal lua is 'lua'
	FIND_LIBRARY(LUA_LIBRARY
		NAMES lua51
		HINTS
		$ENV{LUA_DIR}
		PATH_SUFFIXES lib
	)
else()
	FIND_LIBRARY(LUA_LIBRARY
		NAMES luajit-5.1
		HINTS
		$ENV{LUA_DIR}
		PATH_SUFFIXES lib64 lib
		PATHS
		~/Library/Frameworks
		/Library/Frameworks
		/sw
		/opt/local
		/opt/csw
		/opt
	)
endif()

INCLUDE(FindPackageHandleStandardArgs)
# handle the QUIETLY and REQUIRED arguments and set LUAJIT_FOUND to TRUE if
# all listed variables exist
FIND_PACKAGE_HANDLE_STANDARD_ARGS(LuaJIT
	REQUIRED_VARS LUA_LIBRARY LUA_INCLUDE_DIR)

MARK_AS_ADVANCED(LUA_INCLUDE_DIR LUA_LIBRARY)
