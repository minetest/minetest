#! /bin/bash -eu

cmake -B build -DCMAKE_BUILD_TYPE=Debug \
	-DCMAKE_EXPORT_COMPILE_COMMANDS=ON \
	-DRUN_IN_PLACE=TRUE \
	-G Ninja \
	-DBUILD_SERVER=TRUE
cmake --build build --target GenerateVersion
cmake --build build --target src/network/proto/remoteclient.capnp.h

./util/ci/run-clang-tidy.py \
	-clang-tidy-binary=$CLANG_TIDY -p build \
	-quiet -config="$(cat .clang-tidy)" \
	'src/.*'
