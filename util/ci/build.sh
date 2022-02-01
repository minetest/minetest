#! /bin/bash -e

cmake -B build -DCMAKE_BUILD_TYPE=Debug \
	-DRUN_IN_PLACE=TRUE -DENABLE_GETTEXT=TRUE \
	-DBUILD_SERVER=TRUE ${CMAKE_FLAGS}
cmake --build build --parallel $(($(nproc) + 1))
