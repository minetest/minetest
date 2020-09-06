#! /bin/bash -e

mkdir cmakebuild
cd cmakebuild
cmake -DCMAKE_BUILD_TYPE=Debug \
	-DRUN_IN_PLACE=TRUE -DENABLE_GETTEXT=TRUE \
	-DBUILD_SERVER=TRUE ${CMAKE_FLAGS} ..
make -j2
