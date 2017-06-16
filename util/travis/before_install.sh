#!/bin/bash -e

echo "Preparing for $TRAVIS_COMMIT_RANGE"

. util/travis/common.sh

needs_compile || exit 0

if [[ "${LINT}" == "1" ]]; then
	# We need to build astyle ourselves since Debian includes an outdated version.
	# Fourtunately, astyle is very small and should build in less than ten seconds.
	wget https://sourceforge.net/projects/astyle/files/astyle/astyle%203.0.1/astyle_3.0.1_linux.tar.gz -O astyle.tgz
	tar -xzf astyle.tgz
	cd astyle/build/gcc
	make -j2 release
	sudo make install
elif [[ $PLATFORM == "Unix" ]]; then
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
