#!/bin/bash -e
. util/travis/common.sh

needs_compile || exit 0

if [ -z "${CLANG_TIDY}" ]; then
	CLANG_TIDY=clang-tidy
fi

mkdir -p cmakebuild && cd cmakebuild
cmake -DCMAKE_BUILD_TYPE=Debug \
	-DCMAKE_EXPORT_COMPILE_COMMANDS=ON \
	-DRUN_IN_PLACE=TRUE \
	-DENABLE_GETTEXT=TRUE \
	-DENABLE_SOUND=FALSE \
	-DBUILD_SERVER=TRUE ..
make GenerateVersion
cd ..

echo "Performing clang-tidy checks..."
./util/travis/run-clang-tidy.py \
	-clang-tidy-binary=${CLANG_TIDY} -p cmakebuild \
	-quiet -config="$(cat .clang-tidy)" \
	'src/.*'

RET=$?
echo "Clang tidy returned $RET"
exit $RET
