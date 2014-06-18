#!/bin/bash
set -e

dir="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
if [ $# -ne 1 ]; then
	echo "Usage: $0 <build directory>"
	exit 1
fi
builddir=$1
mkdir -p $builddir
builddir="$( cd "$builddir" && pwd )"
packagedir=$builddir/packages
libdir=$builddir/libs

toolchain_file=$dir/toolchain_mingw.cmake
irrlicht_version=1.8.1
ogg_version=1.2.1
vorbis_version=1.3.3
curl_version=7.18.0
gettext_version=0.14.4
freetype_version=2.3.5-1
luajit_version=2.0.1
leveldb_version=1.15

mkdir -p $packagedir
mkdir -p $libdir

cd $builddir

# Get stuff
[ -e $packagedir/irrlicht-$irrlicht_version.zip ] || wget http://sfan5.pf-control.de/irrlicht-$irrlicht_version-win32.zip \
	-c -O $packagedir/irrlicht-$irrlicht_version.zip
[ -e $packagedir/zlib125.zip ] || wget http://www.winimage.com/zLibDll/zlib125.zip \
	-c -O $packagedir/zlib125.zip
[ -e $packagedir/zlib125dll.zip ] || wget http://www.winimage.com/zLibDll/zlib125dll.zip \
	-c -O $packagedir/zlib125dll.zip
[ -e $packagedir/libogg-$ogg_version-dev.7z ] || wget http://sfan5.pf-control.de/libogg-$ogg_version-dev.7z \
	-c -O $packagedir/libogg-$ogg_version-dev.7z
[ -e $packagedir/libogg-$ogg_version-dll.7z ] || wget http://sfan5.pf-control.de/libogg-$ogg_version-dll.7z \
	-c -O $packagedir/libogg-$ogg_version-dll.7z
[ -e $packagedir/libvorbis-$vorbis_version-dev.7z ] || wget http://sfan5.pf-control.de/libvorbis-$vorbis_version-dev.7z \
	-c -O $packagedir/libvorbis-$vorbis_version-dev.7z
[ -e $packagedir/libvorbis-$vorbis_version-dll.7z ] || wget http://sfan5.pf-control.de/libvorbis-$vorbis_version-dll.7z \
	-c -O $packagedir/libvorbis-$vorbis_version-dll.7z
[ -e $packagedir/libcurl-$curl_version-win32-msvc.zip ] || wget http://curl.haxx.se/download/libcurl-$curl_version-win32-msvc.zip \
	-c -O $packagedir/libcurl-$curl_version-win32-msvc.zip
[ -e $packagedir/gettext-$gettext_version.zip ] || wget http://sfan5.pf-control.de/gettext-$gettext_version.zip \
	-c -O $packagedir/gettext-$gettext_version.zip
[ -e $packagedir/freetype-$freetype_version.zip ] || wget http://sfan5.pf-control.de/freetype-$freetype_version.zip \
    -c -O $packagedir/freetype-$freetype_version.zip
[ -e $packagedir/luajit-$luajit_version-static-win32.zip ] || wget http://sfan5.pf-control.de/luajit-$luajit_version-static-win32.zip \
	-c -O $packagedir/luajit-$luajit_version-static-win32.zip
[ -e $packagedir/libleveldb-$leveldb_version-win32.zip ] || wget http://sfan5.pf-control.de/libleveldb-$leveldb_version-win32.zip \
	-c -O $packagedir/libleveldb-$leveldb_version-win32.zip
[ -e $packagedir/openal_stripped.zip ] || wget http://minetest.ru/bin/openal_stripped.zip \
	-c -O $packagedir/openal_stripped.zip
[ -e $packagedir/mingwm10.dll ] || wget http://minetest.ru/bin/mingwm10.dll \
	-c -O $packagedir/mingwm10.dll

# Extract stuff
cd $libdir
[ -d irrlicht-$irrlicht_version ] || unzip -o $packagedir/irrlicht-$irrlicht_version.zip
[ -d zlib-1.2.5 ] || unzip -o $packagedir/zlib125.zip
[ -d zlib125dll ] || unzip -o $packagedir/zlib125dll.zip -d zlib125dll
[ -d libogg/include ] || 7z x -y -olibogg $packagedir/libogg-$ogg_version-dev.7z
[ -d libogg/bin ] || 7z x -y -olibogg $packagedir/libogg-$ogg_version-dll.7z
[ -d libvorbis/include ] || 7z x -y -olibvorbis $packagedir/libvorbis-$vorbis_version-dev.7z
[ -d libvorbis/bin ] || 7z x -y -olibvorbis $packagedir/libvorbis-$vorbis_version-dll.7z
[ -d libcurl ] || unzip -o $packagedir/libcurl-$curl_version-win32-msvc.zip -d libcurl
[ -d gettext ] || unzip -o $packagedir/gettext-$gettext_version.zip -d gettext
[ -d freetype ] || unzip -o $packagedir/freetype-$freetype_version.zip -d freetype
[ -d openal_stripped ] || unzip -o $packagedir/openal_stripped.zip
[ -d luajit ] || unzip -o $packagedir/luajit-$luajit_version-static-win32.zip -d luajit
[ -d leveldb ] || unzip -o $packagedir/libleveldb-$leveldb_version-win32.zip -d leveldb

# Get minetest
cd $builddir
[ -d minetest ] && (cd minetest && git pull) || (git clone https://github.com/minetest/minetest)
cd minetest
git_hash=`git show | head -c14 | tail -c7`

# Get minetest_game
cd games
[ -d minetest_game ] && (cd minetest_game && git pull) || (git clone https://github.com/minetest/minetest_game)
cd ../..

# Build the thing
cd minetest
[ -d build ] && rm -Rf build/
mkdir build
cd build
cmake .. \
	-DCMAKE_INSTALL_PREFIX=/tmp \
	-DVERSION_EXTRA=$git_hash \
	-DBUILD_CLIENT=1 -DBUILD_SERVER=0 \
	-DCMAKE_TOOLCHAIN_FILE=$toolchain_file \
	-DENABLE_SOUND=1 \
	-DENABLE_CURL=1 \
	-DENABLE_GETTEXT=1 \
	-DENABLE_FREETYPE=1 \
	-DENABLE_LEVELDB=1 \
	-DIRRLICHT_INCLUDE_DIR=$libdir/irrlicht-$irrlicht_version/include \
	-DIRRLICHT_LIBRARY=$libdir/irrlicht-$irrlicht_version/lib/Win32-gcc/libIrrlicht.dll.a \
	-DIRRLICHT_DLL=$libdir/irrlicht-$irrlicht_version/bin/Win32-gcc/Irrlicht.dll \
	-DZLIB_INCLUDE_DIR=$libdir/zlib-1.2.5 \
	-DZLIB_LIBRARIES=$libdir/zlib125dll/dll32/zlibwapi.lib \
	-DZLIB_DLL=$libdir/zlib125dll/dll32/zlibwapi.dll \
	-DLUA_INCLUDE_DIR=$libdir/luajit/include \
	-DLUA_LIBRARY=$libdir/luajit/libluajit.a \
	-DOGG_INCLUDE_DIR=$libdir/libogg/include \
	-DOGG_LIBRARY=$libdir/libogg/lib/libogg.dll.a \
	-DOGG_DLL=$libdir/libogg/bin/libogg-0.dll \
	-DVORBIS_INCLUDE_DIR=$libdir/libvorbis/include \
	-DVORBIS_LIBRARY=$libdir/libvorbis/lib/libvorbis.dll.a \
	-DVORBIS_DLL=$libdir/libvorbis/bin/libvorbis-0.dll \
	-DVORBISFILE_LIBRARY=$libdir/libvorbis/lib/libvorbisfile.dll.a \
	-DVORBISFILE_DLL=$libdir/libvorbis/bin/libvorbisfile-3.dll \
	-DOPENAL_INCLUDE_DIR=$libdir/openal_stripped/include \
	-DOPENAL_LIBRARY=$libdir/openal_stripped/lib/OpenAL32.lib \
	-DOPENAL_DLL=$libdir/openal_stripped/bin/OpenAL32.dll \
	-DMINGWM10_DLL=$packagedir/mingwm10.dll \
	-DCURL_DLL=$libdir/libcurl/libcurl.dll \
	-DCURL_INCLUDE_DIR=$libdir/libcurl/include \
	-DCURL_LIBRARY=$libdir/libcurl/libcurl.lib \
	-DCUSTOM_GETTEXT_PATH=$libdir/gettext \
	-DGETTEXT_MSGFMT=`which msgfmt` \
	-DGETTEXT_DLL=$libdir/gettext/bin/libintl3.dll \
	-DGETTEXT_ICONV_DLL=$libdir/gettext/bin/libiconv2.dll \
	-DGETTEXT_INCLUDE_DIR=$libdir/gettext/include \
	-DGETTEXT_LIBRARY=$libdir/gettext/lib/libintl.dll.a \
	-DFREETYPE_INCLUDE_DIR_freetype2=$libdir/freetype/include/freetype \
	-DFREETYPE_INCLUDE_DIR_ft2build=$libdir/freetype/include \
	-DFREETYPE_LIBRARY=$libdir/freetype/lib/freetype.lib \
	-DLEVELDB_INCLUDE_DIR=$libdir/leveldb/include \
	-DLEVELDB_LIBRARY=$libdir/leveldb/lib/libleveldb.dll.a

make package -j2

# EOF
