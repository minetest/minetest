#!/bin/bash -e

if [ $CC = "clang" ]; then
	export PATH="/usr/bin/:$PATH"
	sudo sh -c 'echo "deb http://ppa.launchpad.net/eudoxos/llvm-3.1/ubuntu precise main" >> /etc/apt/sources.list'
	sudo apt-key adv --keyserver pool.sks-keyservers.net --recv-keys 92DE8183
	sudo apt-get update
	sudo apt-get install llvm-3.1
	sudo apt-get install clang
fi
if [ $WINDOWS = "no" ]; then
	sudo apt-get install libirrlicht-dev cmake libbz2-dev libpng12-dev \
	libjpeg8-dev libxxf86vm-dev libgl1-mesa-dev libsqlite3-dev libogg-dev \
	libvorbis-dev libopenal-dev gettext
else
	sudo apt-get install p7zip-full
	if [ $WINDOWS = "32" ]; then
		wget http://sfan5.pf-control.de/mingw_w64_i686_ubuntu12.04_4.9.1.7z -O mingw.7z
		sed -e "s|%PREFIX%|i686-w64-mingw32|" \
			-e "s|%ROOTPATH%|/usr/i686-w64-mingw32|" \
			< util/travis/toolchain_mingw.cmake.in > util/buildbot/toolchain_mingw.cmake
	elif [ $WINDOWS = "64" ]; then
		wget http://sfan5.pf-control.de/mingw_w64_x86_64_ubuntu12.04_4.9.1.7z -O mingw.7z
		sed -e "s|%PREFIX%|x86_64-w64-mingw32|" \
			-e "s|%ROOTPATH%|/usr/x86_64-w64-mingw32|" \
			< util/travis/toolchain_mingw.cmake.in > util/buildbot/toolchain_mingw64.cmake
	fi
	sudo 7z x -y -o/usr mingw.7z
fi
