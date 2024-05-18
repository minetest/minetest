#!/bin/bash -e

# Use ccache if it is available
extra_args=()
command -v ccache >/dev/null && extra_args+=(-DCMAKE_{C,CXX}_COMPILER_LAUNCHER=ccache)

cmake -B build \
	"${extra_args[@]}" \
	-DCMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE:-Debug} \
	-DENABLE_LTO=FALSE \
	-DRUN_IN_PLACE=TRUE \
	-DENABLE_GETTEXT=${CMAKE_ENABLE_GETTEXT:-TRUE} \
	-DBUILD_SERVER=${CMAKE_BUILD_SERVER:-TRUE} \
	${CMAKE_FLAGS}

cmake --build build --parallel $(($(nproc) + 1))
