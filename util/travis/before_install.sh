#!/bin/bash -e

echo "Preparing for $TRAVIS_COMMIT_RANGE"

. util/travis/common.sh

if [[ "${LINT}" == "1" ]]; then
	exit 0
fi

needs_compile || exit 0

if [[ $PLATFORM == "Unix" ]]; then
	if [[ $TRAVIS_OS_NAME == "linux" ]]; then
		install_linux_deps
	else
		install_macosx_deps
	fi
elif [[ $PLATFORM == "Win32" ]]; then
	sudo apt-get update
	sudo apt-get install p7zip-full
	wget http://minetest.kitsunemimi.pw/mingw-w64-i686_7.1.1_ubuntu14.04.7z -O mingw.7z
	sed -e "s|%PREFIX%|i686-w64-mingw32|" \
		-e "s|%ROOTPATH%|/usr/i686-w64-mingw32|" \
		< util/travis/toolchain_mingw.cmake.in > util/buildbot/toolchain_mingw.cmake
	sudo 7z x -y -o/usr mingw.7z
elif [[ $PLATFORM == "Win64" ]]; then
	sudo apt-get update
	sudo apt-get install p7zip-full
	wget http://minetest.kitsunemimi.pw/mingw-w64-x86_64_7.1.1_ubuntu14.04.7z -O mingw.7z
	sed -e "s|%PREFIX%|x86_64-w64-mingw32|" \
		-e "s|%ROOTPATH%|/usr/x86_64-w64-mingw32|" \
		< util/travis/toolchain_mingw.cmake.in > util/buildbot/toolchain_mingw64.cmake
	sudo 7z x -y -o/usr mingw.7z
fi
