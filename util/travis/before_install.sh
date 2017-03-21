#!/bin/bash -e
echo "Preparing for $TRAVIS_COMMIT_RANGE"

if [[ "$LINT" == "1" ]]; then
	curl http://apt.llvm.org/llvm-snapshot.gpg.key | sudo apt-key add -
	sudo add-apt-repository "deb http://apt.llvm.org/trusty/ llvm-toolchain-trusty-3.9 main"
	sudo apt-get -yq update
	sudo apt-get install clang-format-3.9
	exit 0
fi

. util/travis/common.sh
needs_compile || exit 0

if [[ $TRAVIS_OS_NAME == "linux" ]]; then
	sudo apt-get update
	sudo apt-get install p7zip-full $COMPILER
fi

if [[ $PLATFORM == "Unix" ]]; then
	if [[ $TRAVIS_OS_NAME == "linux" ]]; then
		sudo apt-get update
		sudo apt-get install libirrlicht-dev cmake libbz2-dev libpng12-dev \
			libjpeg-dev libxxf86vm-dev libgl1-mesa-dev libsqlite3-dev \
			libhiredis-dev libogg-dev libgmp-dev libvorbis-dev libopenal-dev \
			gettext libpq-dev postgresql-server-dev-all
		# Linking to LevelDB is broken, use a custom build
		wget http://minetest.kitsunemimi.pw/libleveldb-1.18-ubuntu12.04.7z
		sudo 7z x -o/usr libleveldb-1.18-ubuntu12.04.7z
		if [[ "$VALGRIND" == "1" ]]; then
			sudo apt-get install valgrind
		fi

	else
		brew update
		brew install freetype gettext hiredis irrlicht jpeg leveldb libogg libvorbis luajit
		#brew upgrade postgresql
	fi
elif [[ $PLATFORM == "Win32" ]]; then
	wget http://minetest.kitsunemimi.pw/mingw_w64_i686_ubuntu12.04_4.9.1.7z -O mingw.7z
	sed -e "s|%PREFIX%|i686-w64-mingw32|" \
		-e "s|%ROOTPATH%|/usr/i686-w64-mingw32|" \
		< util/travis/toolchain_mingw.cmake.in > util/buildbot/toolchain_mingw.cmake
	sudo 7z x -y -o/usr mingw.7z
elif [[ $PLATFORM == "Win64" ]]; then
	wget http://minetest.kitsunemimi.pw/mingw_w64_x86_64_ubuntu12.04_4.9.1.7z -O mingw.7z
	sed -e "s|%PREFIX%|x86_64-w64-mingw32|" \
		-e "s|%ROOTPATH%|/usr/x86_64-w64-mingw32|" \
		< util/travis/toolchain_mingw.cmake.in > util/buildbot/toolchain_mingw64.cmake
	sudo 7z x -y -o/usr mingw.7z
fi
