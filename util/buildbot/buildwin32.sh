#!/bin/bash
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
openal_stripped_file=$dir/openal_stripped.zip
mingwm10_dll_file=$dir/mingwm10.dll
irrlicht_version=1.7.2
ogg_version=1.2.1
vorbis_version=1.3.2

# unzip -l $openal_stripped_file:
#        0  2012-04-03 00:25   openal_stripped/
#        0  2012-04-03 00:25   openal_stripped/bin/
#   109080  2012-04-03 00:03   openal_stripped/bin/OpenAL32.dll
#        0  2012-04-03 00:29   openal_stripped/include/
#    26443  2006-12-08 17:30   openal_stripped/include/al.h
#     8282  2007-04-13 17:02   openal_stripped/include/alc.h
#        0  2012-04-03 00:07   openal_stripped/lib/
#    20552  2005-08-02 08:53   openal_stripped/lib/OpenAL32.lib

mkdir -p $packagedir
mkdir -p $libdir

cd $builddir

# Get stuff
wget http://downloads.sourceforge.net/irrlicht/irrlicht-$irrlicht_version.zip \
	-c -O $packagedir/irrlicht-$irrlicht_version.zip || exit 1
wget http://www.winimage.com/zLibDll/zlib125.zip \
	-c -O $packagedir/zlib125.zip || exit 1
wget http://www.winimage.com/zLibDll/zlib125dll.zip \
	-c -O $packagedir/zlib125dll.zip || exit 1
wget http://switch.dl.sourceforge.net/project/winlibs/libogg/libogg-$ogg_version-dev.7z \
	-c -O $packagedir/libogg-$ogg_version-dev.7z || exit 1
wget http://switch.dl.sourceforge.net/project/winlibs/libogg/libogg-$ogg_version-dll.7z \
	-c -O $packagedir/libogg-$ogg_version-dll.7z || exit 1
wget http://switch.dl.sourceforge.net/project/winlibs/libvorbis/libvorbis-$vorbis_version-dev.7z \
	-c -O $packagedir/libvorbis-$vorbis_version-dev.7z || exit 1
wget http://switch.dl.sourceforge.net/project/winlibs/libvorbis/libvorbis-$vorbis_version-dll.7z \
	-c -O $packagedir/libvorbis-$vorbis_version-dll.7z || exit 1
wget http://github.com/celeron55/minetest/zipball/master \
	-c -O $packagedir/minetest.zip || exit 1
cp $openal_stripped_file $packagedir/openal_stripped.zip || exit 1
cp $mingwm10_dll_file $packagedir/mingwm10.dll || exit 1
wget http://github.com/celeron55/minetest_game/zipball/master \
	-c -O $packagedir/minetest_game.zip || exit 1

# Figure out some path names from the packages
minetestdirname=`unzip -l $packagedir/minetest.zip | head -n 7 | tail -n 1 | sed -e 's/^[^c]*//' -e 's/\/.*$//'`
minetestdir=$builddir/$minetestdirname || exit 1
git_hash=`echo $minetestdirname | sed -e 's/celeron55-minetest-//'`
minetest_gamedirname=`unzip -l $packagedir/minetest_game.zip | head -n 7 | tail -n 1 | sed -e 's/^[^c]*//' -e 's/\/.*$//'`

# Extract stuff
cd $libdir || exit 1
unzip -o $packagedir/irrlicht-$irrlicht_version.zip || exit 1
unzip -o $packagedir/zlib125.zip || exit 1
unzip -o $packagedir/zlib125dll.zip -d zlib125dll || exit 1
7z x -y -olibogg $packagedir/libogg-$ogg_version-dev.7z || exit 1
7z x -y -olibogg $packagedir/libogg-$ogg_version-dll.7z || exit 1
7z x -y -olibvorbis $packagedir/libvorbis-$vorbis_version-dev.7z || exit 1
7z x -y -olibvorbis $packagedir/libvorbis-$vorbis_version-dll.7z || exit 1
unzip -o $packagedir/openal_stripped.zip || exit 1
cd $builddir || exit 1
unzip -o $packagedir/minetest.zip || exit 1

# Symlink minetestdir
rm -rf $builddir/minetest
ln -s $minetestdir $builddir/minetest

# Extract minetest_game
cd $minetestdir/games || exit 1
rm -rf minetest_game || exit 1
unzip -o $packagedir/minetest_game.zip || exit 1
minetest_gamedir=$minetestdir/games/$minetest_gamedirname || exit 1
mv $minetest_gamedir $minetestdir/games/minetest_game || exit 1

# Build the thing
cd $minetestdir || exit 1
mkdir -p build || exit 1
cd build || exit 1
cmake $minetestdir -DCMAKE_TOOLCHAIN_FILE=$toolchain_file -DENABLE_SOUND=1 \
	-DIRRLICHT_INCLUDE_DIR=$libdir/irrlicht-$irrlicht_version/include \
	-DIRRLICHT_LIBRARY=$libdir/irrlicht-$irrlicht_version/lib/Win32-gcc/libIrrlicht.dll.a \
	-DIRRLICHT_DLL=$libdir/irrlicht-$irrlicht_version/bin/Win32-gcc/Irrlicht.dll \
	-DZLIB_INCLUDE_DIR=$libdir/zlib-1.2.5 \
	-DZLIB_LIBRARIES=$libdir/zlib125dll/dll32/zlibwapi.lib \
	-DZLIB_DLL=$libdir/zlib125dll/dll32/zlibwapi.dll \
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
	-DCMAKE_INSTALL_PREFIX=/tmp \
	-DVERSION_EXTRA=$git_hash \
	|| exit 1
make -j2 package || exit 1

#pubdir=/home/celeron55/public_html/random/`date +%Y-%m` || exit 1
#mkdir -p $pubdir || exit 1
#cp *autobuild*.zip $pubdir/ || exit 1

# EOF
