#!/bin/bash
set -e

topdir="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
if [ $# -ne 1 ]; then
	echo "Usage: $0 <build directory>"
	exit 1
fi
builddir=$1
mkdir -p $builddir
builddir="$( cd "$builddir" && pwd )"
libdir=$builddir/libs

source $topdir/common.sh

# Test which win32 compiler is present
command -v i686-w64-mingw32-gcc >/dev/null &&
	compiler=i686-w64-mingw32-gcc
command -v i686-w64-mingw32-gcc-posix >/dev/null &&
	compiler=i686-w64-mingw32-gcc-posix

if [ -z "$compiler" ]; then
	echo "Unable to determine which MinGW compiler to use"
	exit 1
fi
toolchain_file=$topdir/toolchain_${compiler/-gcc/}.cmake
echo "Using $toolchain_file"

find_runtime_dlls i686-w64-mingw32

# Get stuff
irrlicht_version=$(cat $topdir/../../misc/irrlichtmt_tag.txt)

mkdir -p $libdir

# 'dw2' just points to rebuilt versions after a toolchain change
# this distinction should be gotten rid of next time

cd $libdir
download "https://github.com/minetest/irrlicht/releases/download/$irrlicht_version/win32.zip" irrlicht-$irrlicht_version.zip
download "http://minetest.kitsunemimi.pw/zlib-$zlib_version-win32.zip"
download "http://minetest.kitsunemimi.pw/zstd-$zstd_version-win32.zip"
download "http://minetest.kitsunemimi.pw/libogg-$ogg_version-win32.zip"
download "http://minetest.kitsunemimi.pw/dw2/libvorbis-$vorbis_version-win32.zip"
download "http://minetest.kitsunemimi.pw/curl-$curl_version-win32.zip"
download "http://minetest.kitsunemimi.pw/gettext-$gettext_version-win32.zip"
download "http://minetest.kitsunemimi.pw/freetype-$freetype_version-win32.zip"
download "http://minetest.kitsunemimi.pw/sqlite3-$sqlite3_version-win32.zip"
download "http://minetest.kitsunemimi.pw/luajit-$luajit_version-win32.zip"
download "http://minetest.kitsunemimi.pw/dw2/libleveldb-$leveldb_version-win32.zip" leveldb-$leveldb_version.zip
download "http://minetest.kitsunemimi.pw/openal-soft-$openal_version-win32.zip"

# Set source dir, downloading Minetest as needed
get_sources

git_hash=$(cd $sourcedir && git rev-parse --short HEAD)

# Build the thing
cd $builddir
[ -d build ] && rm -rf build

cmake_args=(
	-DCMAKE_TOOLCHAIN_FILE=$toolchain_file
	-DCMAKE_INSTALL_PREFIX=/tmp
	-DVERSION_EXTRA=$git_hash
	-DBUILD_CLIENT=1 -DBUILD_SERVER=0
	-DEXTRA_DLL="$runtime_dlls"

	-DENABLE_SOUND=1
	-DENABLE_CURL=1
	-DENABLE_GETTEXT=1
	-DENABLE_LEVELDB=1
)
add_cmake_libs
cmake -S $sourcedir -B build "${cmake_args[@]}"

cmake --build build -j$(nproc)

[ -z "$NO_PACKAGE" ] && cmake --build build --target package

exit 0
# EOF
