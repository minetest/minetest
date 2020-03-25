#!/bin/bash -e

echo "Preparing for $TRAVIS_COMMIT_RANGE"

. util/travis/common.sh

if [[ ! -z "${CLANG_FORMAT}" ]]; then
	exit 0
fi

needs_compile || exit 0

if [[ $PLATFORM == "Unix" ]] || [[ ! -z "${CLANG_TIDY}" ]]; then
	if [[ $TRAVIS_OS_NAME == "linux" ]] || [[ ! -z "${CLANG_TIDY}" ]]; then
		install_linux_deps
	else
		install_macosx_deps
	fi
elif [[ $PLATFORM == "Win32" ]]; then
	wget http://minetest.kitsunemimi.pw/mingw-w64-i686_9.2.0_ubuntu18.04.tar.xz -O mingw.tar.xz
	# buildwin32.sh detects the installed toolchain automatically
	sudo tar -xaf mingw.tar.xz -C /usr
elif [[ $PLATFORM == "Win64" ]]; then
	wget http://minetest.kitsunemimi.pw/mingw-w64-x86_64_9.2.0_ubuntu18.04.tar.xz -O mingw.tar.xz
	sed -e "s|%PREFIX%|x86_64-w64-mingw32|" \
		-e "s|%ROOTPATH%|/usr/x86_64-w64-mingw32|" \
		< util/travis/toolchain_mingw.cmake.in > util/buildbot/toolchain_mingw64.cmake
	sudo tar -xaf mingw.tar.xz -C /usr
fi
