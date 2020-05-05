#! /bin/bash -eu

mkdir -p cmakebuild
cd cmakebuild
cmake -DCMAKE_BUILD_TYPE=Debug \
	-DCMAKE_EXPORT_COMPILE_COMMANDS=ON \
	-DRUN_IN_PLACE=TRUE \
	-DENABLE_{GETTEXT,SOUND}=FALSE \
	-DBUILD_SERVER=TRUE ..
make GenerateVersion

cd ..

./util/ci/run-clang-tidy.py \
	-clang-tidy-binary=clang-tidy-9 -p cmakebuild \
	-quiet -config="$(cat .clang-tidy)" \
	'src/.*'
