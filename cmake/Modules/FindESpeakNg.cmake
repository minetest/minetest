mark_as_advanced(ESPEAKNG_DATA_DIR ESPEAKNG_INCLUDE_DIR ESPEAKNG_LIBRARY)

find_path(ESPEAKNG_DATA_DIR en_dict)

find_path(ESPEAKNG_INCLUDE_DIR espeak-ng/espeak_ng.h)

find_library(ESPEAKNG_LIBRARY NAMES espeak-ng)

if(ESPEAKNG_DATA_DIR)
	add_custom_target(espeak-data-copy
		COMMAND "${CMAKE_COMMAND}"
			-E copy_directory "${ESPEAKNG_DATA_DIR}/" "${CMAKE_SOURCE_DIR}/client/espeak-ng-data"
		COMMENT
			"Providing espeak-ng voice data"
	)
	install(DIRECTORY   "${ESPEAKNG_DATA_DIR}/"
	        DESTINATION "client/espeak-ng-data"
	)
endif()

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(ESPEAKNG DEFAULT_MSG ESPEAKNG_DATA_DIR ESPEAKNG_INCLUDE_DIR ESPEAKNG_LIBRARY)
