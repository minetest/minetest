#! /bin/bash -e

mkdir build
cd build
cmake -DCMAKE_BUILD_TYPE=Debug \
	-DRUN_IN_PLACE=TRUE -DENABLE_GETTEXT=TRUE \
	-DBUILD_SERVER=TRUE ${CMAKE_FLAGS} ..
make -j$(($(nproc) + 1))
