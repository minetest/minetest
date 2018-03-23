#!/bin/bash -e
. util/travis/common.sh

needs_compile || exit 0

if [ -z "${CLANG_TIDY}" ]; then
	CLANG_TIDY=clang-tidy
fi

files_to_analyze="$(find src/ -name '*.cpp' -or -name '*.h')"

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
./util/travis/run-clang-tidy.py -clang-tidy-binary=${CLANG_TIDY} -p cmakebuild \
	-checks='-*,modernize-use-emplace,modernize-avoid-bind,performance-*' \
	-warningsaserrors='-*,modernize-use-emplace,performance-type-promotion-in-math-fn,performance-faster-string-find,performance-implicit-cast-in-loop' \
	-no-command-on-stdout -quiet \
	files 'src/.*'
RET=$?
echo "Clang tidy returned $RET"
exit $RET
