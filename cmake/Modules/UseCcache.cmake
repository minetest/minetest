# Enable ccache if present and not already enabled system wide.
OPTION(SKIP_CCACHE
	"Skip enabling for ccache - no effect if ccache enabled system wide" OFF)
IF(NOT SKIP_CCACHE)
	FIND_PROGRAM(CCACHE_FOUND ccache)
	IF(CCACHE_FOUND)
		IF(NOT ("${CMAKE_CXX_COMPILER} ${CMAKE_C_COMPILER}" MATCHES ".*ccache.*"))
			set_property(GLOBAL PROPERTY RULE_LAUNCH_COMPILE ${CCACHE_FOUND})
			set_property(GLOBAL PROPERTY RULE_LAUNCH_LINK ${CCACHE_FOUND})
			MESSAGE(STATUS
        "Ccache enabled. (${CCACHE_FOUND} - enabling as compiler wrapper)")
		ELSE()
			MESSAGE(STATUS
        "Ccache enabled. (already in use as C and/or CXX compiler wrapper)")
		ENDIF()
	ENDIF(CCACHE_FOUND)
ENDIF(NOT SKIP_CCACHE)
