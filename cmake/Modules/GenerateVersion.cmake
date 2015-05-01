# Always run during 'make'

if(VERSION_EXTRA)
	set(VERSION_GITHASH "${VERSION_STRING}")
else()
	execute_process(COMMAND git describe --tag --dirty
		WORKING_DIRECTORY "${GENERATE_VERSION_SOURCE_DIR}"
		OUTPUT_VARIABLE VERSION_GITHASH OUTPUT_STRIP_TRAILING_WHITESPACE
		ERROR_QUIET)

	if(VERSION_GITHASH)
		message(STATUS "*** Detected Git version ${VERSION_GITHASH} ***")
	else()
		execute_process(COMMAND git describe --always --tag --dirty
			WORKING_DIRECTORY "${GENERATE_VERSION_SOURCE_DIR}"
			OUTPUT_VARIABLE VERSION_GITHASH OUTPUT_STRIP_TRAILING_WHITESPACE
			ERROR_QUIET)
		if(VERSION_GITHASH)
			set(VERSION_GITHASH "${VERSION_STRING}-${VERSION_GITHASH}")
			message(STATUS "*** Detected shallow Git version ${VERSION_GITHASH} ***")
		else()
			set(VERSION_GITHASH "${VERSION_STRING}")
		endif()
	endif()
endif()

configure_file(
	${GENERATE_VERSION_SOURCE_DIR}/cmake_config_githash.h.in
	${GENERATE_VERSION_BINARY_DIR}/cmake_config_githash.h)

