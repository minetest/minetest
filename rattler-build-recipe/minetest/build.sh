#!/usr/bin/env bash

set -o errexit
set -o nounset
set -o pipefail
set -o xtrace

rm -rf build

cmake -B build \
    -S . \
	-G "Ninja" \
    -LAH \
    -DCMAKE_PREFIX_PATH=${PREFIX} \
    -DCMAKE_INSTALL_PREFIX=${PREFIX} \
	-DCMAKE_BUILD_TYPE="Release" \
	-DBUILD_SERVER=FALSE \
	-DENABLE_GETTEXT=TRUE \
    -DENABLE_SOUND=FALSE \
    -DUSE_SDL2=ON \
    -DBUILD_UNITTESTS=FALSE

cmake --build build
cmake --install build
