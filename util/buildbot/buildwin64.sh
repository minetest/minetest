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

# Test which win64 compiler is present
command -v x86_64-w64-mingw32-gcc >/dev/null &&
	compiler=x86_64-w64-mingw32-gcc
command -v x86_64-w64-mingw32-gcc-posix >/dev/null &&
	compiler=x86_64-w64-mingw32-gcc-posix

if [ -z "$compiler" ]; then
	echo "Unable to determine which MinGW compiler to use"
	exit 1
fi
toolchain_file=$topdir/toolchain_${compiler/-gcc/}.cmake
echo "Using $toolchain_file"

find_runtime_dlls x86_64-w64-mingw32

# Get stuff
irrlicht_version=$(cat $topdir/../../misc/irrlichtmt_tag.txt)

mkdir -p $libdir

# 'ucrt' just points to rebuilt versions after a toolchain change

cd $libdir
libhost="http://minetest.kitsunemimi.pw"
download "https://github.com/minetest/irrlicht/releases/download/$irrlicht_version/win64-ucrt.zip" irrlicht-$irrlicht_version-win64.zip
download "$libhost/zlib-$zlib_version-win64.zip"
download "$libhost/ucrt/zstd-$zstd_version-win64.zip"
download "$libhost/ucrt/libogg-$ogg_version-win64.zip"
download "$libhost/ucrt/libvorbis-$vorbis_version-win64.zip"
download "$libhost/curl-$curl_version-win64.zip"
download "$libhost/ucrt/gettext-$gettext_version-win64.zip"
download "$libhost/freetype-$freetype_version-win64.zip"
download "$libhost/sqlite3-$sqlite3_version-win64.zip"
download "$libhost/luajit-$luajit_version-win64.zip"
download "$libhost/ucrt/libleveldb-$leveldb_version-win64.zip" leveldb-$leveldb_version-win64.zip
download "$libhost/openal-soft-$openal_version-win64.zip"

# Set source dir, downloading Minetest as needed
get_sources

# Build the thing
cd $builddir
[ -d build ] && rm -rf build

cmake_args=(
	-DCMAKE_TOOLCHAIN_FILE=$toolchain_file
	-DCMAKE_INSTALL_PREFIX=/tmp
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
