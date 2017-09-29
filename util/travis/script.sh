#!/bin/bash -e
. util/travis/common.sh
. util/travis/lint.sh

needs_compile || exit 0

if [[ "$LINT" == "1" ]]; then
	# Lint with exit CI
	perform_lint
	exit 0
fi

set_linux_compiler_env

if [[ ${PLATFORM} == "Unix" ]]; then
	mkdir -p travisbuild
	cd travisbuild || exit 1

	CMAKE_FLAGS=''

	if [[ ${TRAVIS_OS_NAME} == "osx" ]]; then
		CMAKE_FLAGS+=' -DCUSTOM_GETTEXT_PATH=/usr/local/opt/gettext'
	fi

	cmake -DCMAKE_BUILD_TYPE=Debug \
		-DRUN_IN_PLACE=TRUE \
		-DENABLE_GETTEXT=TRUE \
		-DBUILD_SERVER=TRUE \
		${CMAKE_FLAGS} ..
	make -j2

	echo "Running unit tests."
	CMD="../bin/minetest --run-unittests"
	if [[ "${VALGRIND}" == "1" ]]; then
		valgrind --leak-check=full --leak-check-heuristics=all --undef-value-errors=no --error-exitcode=9 ${CMD} && exit 0
	else
		${CMD} && exit 0
	fi

elif [[ $PLATFORM == Win* ]]; then
	[[ $CC == "clang" ]] && exit 1 # Not supposed to happen
	# We need to have our build directory outside of the minetest directory because
	#  CMake will otherwise get very very confused with symlinks and complain that
	#  something is not a subdirectory of something even if it actually is.
	# e.g.:
	# /home/travis/minetest/minetest/travisbuild/minetest
	# \/  \/  \/
	# /home/travis/minetest/minetest/travisbuild/minetest/travisbuild/minetest
	# \/  \/  \/
	# /home/travis/minetest/minetest/travisbuild/minetest/travisbuild/minetest/travisbuild/minetest
	# You get the idea.
	OLDDIR=$(pwd)
	cd ..
	export EXISTING_MINETEST_DIR=$OLDDIR
	export NO_MINETEST_GAME=1
	if [[ $PLATFORM == "Win32" ]]; then
		"$OLDDIR/util/buildbot/buildwin32.sh" travisbuild && exit 0
	elif [[ $PLATFORM == "Win64" ]]; then
		"$OLDDIR/util/buildbot/buildwin64.sh" travisbuild && exit 0
	fi
else
	echo "Unknown platform \"${PLATFORM}\"."
	exit 1
fi

