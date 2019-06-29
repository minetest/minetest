# Macro for setting up precompiled headers. Usage:
#
#   add_precompiled_header(target header.h [FORCEINCLUDE])
#
# MSVC: A source file with the same name as the header must exist and
# be included in the target (E.g. header.cpp).
#
# MSVC: Add FORCEINCLUDE to automatically include the precompiled
# header file from every source file.
#
# GCC: The precompiled header is always automatically included from
# every header file.
FUNCTION(ADD_PRECOMPILED_HEADER _targetName _input)
  GET_FILENAME_COMPONENT(_inputWe ${_input} NAME_WE)
  SET(pch_source ${_inputWe}.cpp)
  SET(FORCEINCLUDE OFF)
  FOREACH(arg ${ARGN})
    IF("${arg}" STREQUAL "FORCEINCLUDE")
      SET(FORCEINCLUDE ON)
    ELSE("${arg}" STREQUAL "FORCEINCLUDE")
      LIST(APPEND _extra_incl_dirs ${arg}) # significant on Linux only
    ENDIF("${arg}" STREQUAL "FORCEINCLUDE")
  ENDFOREACH(arg)

  IF(MSVC)
    GET_TARGET_PROPERTY(sources ${_targetName} SOURCES)
    SET(_sourceFound FALSE)
    FOREACH(_source ${sources})
		  SET(PCH_COMPILE_FLAGS "")
		  IF(_source MATCHES \\.\(cc|cxx|cpp\)$)
		GET_FILENAME_COMPONENT(_sourceWe ${_source} NAME_WE)
		IF(_sourceWe STREQUAL ${_inputWe})
		  SET(PCH_COMPILE_FLAGS "${PCH_COMPILE_FLAGS} /Yc${_input}")
		  SET(_sourceFound TRUE)
		ELSE(_sourceWe STREQUAL ${_inputWe})
		  SET(PCH_COMPILE_FLAGS "${PCH_COMPILE_FLAGS} /Yu${_input}")
		  IF(FORCEINCLUDE)
			SET(PCH_COMPILE_FLAGS "${PCH_COMPILE_FLAGS} /FI${_input}")
		  ENDIF(FORCEINCLUDE)
		ENDIF(_sourceWe STREQUAL ${_inputWe})
		SET_SOURCE_FILES_PROPERTIES(${_source} PROPERTIES COMPILE_FLAGS "${PCH_COMPILE_FLAGS}")
		  ENDIF(_source MATCHES \\.\(cc|cxx|cpp\)$)
    ENDFOREACH()
    IF(NOT _sourceFound)
      MESSAGE(FATAL_ERROR "A source file for ${_input} was not found. Required for MSVC builds.")
    ENDIF(NOT _sourceFound)
  ENDIF(MSVC)

  IF(CMAKE_COMPILER_IS_GNUCXX)
    GET_FILENAME_COMPONENT(_name ${_input} NAME)
    SET(_source "${CMAKE_CURRENT_SOURCE_DIR}/${_input}")
    SET(_output "${CMAKE_CURRENT_BINARY_DIR}/${_name}.gch")
    
    STRING(TOUPPER "CMAKE_CXX_FLAGS_${CMAKE_BUILD_TYPE}" _flags_var_name)
    SET(_compiler_FLAGS ${CMAKE_CXX_FLAGS} ${${_flags_var_name}})
    
    GET_TARGET_PROPERTY(_include_directories ${_targetName} INCLUDE_DIRECTORIES)
    FOREACH(item ${_include_directories})
      string(SUBSTRING ${item} 0 1 _item)
      string(COMPARE EQUAL "$" ${_item} _item)
      if (NOT ${_item}) # do not take into account not evaluated generator expressions! (things like $<X:Y>)
      # Generaor expressions are evaluated at build stage, not in config stage which is really sad! https://stackoverflow.com/questions/35695152
          LIST(APPEND _compiler_FLAGS "-I${item}")
      endif()
    ENDFOREACH(item)
    FOREACH(item ${_extra_incl_dirs})
      LIST(APPEND _compiler_FLAGS "-I${item}")
    ENDFOREACH(item)

    GET_TARGET_PROPERTY(_compile_definitions ${_targetName} COMPILE_DEFINITIONS)
    if (NOT ${_compile_definitions} STREQUAL "_compile_definitions-NOTFOUND")
    FOREACH(item ${_compile_definitions})
      LIST(APPEND _compiler_FLAGS "-D${item}")
    ENDFOREACH(item)
    endif()

    GET_TARGET_PROPERTY(_compile_options ${_targetName} COMPILE_OPTIONS)
    if (NOT ${_compile_options} STREQUAL "_compile_options-NOTFOUND")
    FOREACH(item ${_compile_options})
      string(REGEX MATCH "RELEASE>" _regex_res "${item}")
      string(LENGTH "${_regex_res}" _len)
      if ((${CMAKE_BUILD_TYPE} STREQUAL "Release") AND (${_len} GREATER 0))
          string(SUBSTRING ${item} 17 -1 _just_option)
          string(REGEX MATCH "-[A-Za-z\-]+" _just_option ${_just_option})
          list(APPEND _compiler_FLAGS "${_just_option}")
      endif()
      if (${_len} GREATER 0)
          continue()
      endif()

      string(REGEX MATCH "DEBUG>" _regex_res "${item}")
      string(LENGTH "${_regex_res}" _len)
      if ((${CMAKE_BUILD_TYPE} STREQUAL "Debug") AND (${_len} GREATER 0))
          string(SUBSTRING ${item} 15 -1 _just_option)
          string(REGEX MATCH "-[A-Za-z\-]+" _just_option ${_just_option})
          list(APPEND _compiler_FLAGS "${_just_option}")
      endif()
      if (${_len} GREATER 0)
          continue()
      endif()

      list(APPEND _compiler_FLAGS "${item}")
    ENDFOREACH(item)
    endif()

    get_target_property(_cxx_std ${_targetName} CXX_STANDARD)

    SEPARATE_ARGUMENTS(_compiler_FLAGS)
    #STRING(REPLACE ";" "\t" _compiler_FLAGS "${_compiler_FLAGS}")
    MESSAGE("${CMAKE_CXX_COMPILER} -DPCHCOMPILE ${_compiler_FLAGS} -x c++-header -std=gnu++${_cxx_std} -o ${_output} ${_source}")
    ADD_CUSTOM_COMMAND(
      OUTPUT ${_output}
      COMMAND ${CMAKE_CXX_COMPILER} ${_compiler_FLAGS} -std=gnu++${_cxx_std} -x c++-header -c ${_source} -o ${_output}
      DEPENDS ${_source} ${IRRLICHT_HEADERS}
    )
    ADD_CUSTOM_TARGET(${_targetName}_gch DEPENDS ${_output})
    ADD_DEPENDENCIES(${_targetName} ${_targetName}_gch)
    target_compile_options(${_targetName} PRIVATE "$<$<COMPILE_LANGUAGE:CXX>:-include;${CMAKE_CURRENT_BINARY_DIR}/${_name};-Winvalid-pch>")
  ENDIF(CMAKE_COMPILER_IS_GNUCXX)
ENDFUNCTION()
