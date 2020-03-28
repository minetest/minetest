#!/bin/bash -e

set_linux_compiler_env() {
	if [[ "${COMPILER}" == "gcc-6" ]]; then
		export CC=gcc-6
		export CXX=g++-6
	elif [[ "${COMPILER}" == "gcc-8" ]]; then
		export CC=gcc-8
		export CXX=g++-8
	elif [[ "${COMPILER}" == "clang-3.9" ]]; then
		export CC=clang-3.9
		export CXX=clang++-3.9
	elif [[ "${COMPILER}" == "clang-9" ]]; then
		export CC=clang-9
		export CXX=clang++-9
	fi
}

# Linux build only
install_linux_deps() {
	sudo apt-get update
	sudo apt-get install libirrlicht-dev cmake libbz2-dev libpng-dev \
		libjpeg-dev libxxf86vm-dev libgl1-mesa-dev libsqlite3-dev \
		libhiredis-dev libogg-dev libgmp-dev libvorbis-dev libopenal-dev \
		gettext libpq-dev postgresql-server-dev-all libleveldb-dev
}

# Mac OSX build only
install_macosx_deps() {
	brew update
	brew install freetype gettext hiredis irrlicht leveldb libogg libvorbis luajit
	if brew ls | grep -q jpeg; then
		brew upgrade jpeg
	else
		brew install jpeg
	fi
	#brew upgrade postgresql
}

# Relative to git-repository root:
TRIGGER_COMPILE_PATHS="src/.*\.(c|cpp|h)|CMakeLists.txt|cmake/Modules/|util/travis/|util/buildbot/"

needs_compile() {
	RANGE="$TRAVIS_COMMIT_RANGE"
	if [[ "$(git diff --name-only $RANGE -- 2>/dev/null)" == "" ]]; then
		RANGE="$TRAVIS_COMMIT^...$TRAVIS_COMMIT"
		echo "Fixed range: $RANGE"
	fi
	git diff --name-only $RANGE -- | egrep -q "^($TRIGGER_COMPILE_PATHS)"
}

