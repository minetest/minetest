#!/bin/bash -e

# NOTE: this code is mostly copied from minetest_android_deps
# <https://github.com/minetest/minetest_android_deps>

png_ver=1.6.40
jpeg_ver=3.0.1

download () {
	get_tar_archive libpng "https://download.sourceforge.net/libpng/libpng-${png_ver}.tar.gz"
	get_tar_archive libjpeg "https://download.sourceforge.net/libjpeg-turbo/libjpeg-turbo-${jpeg_ver}.tar.gz"
}

build () {
	# Build libjpg and libpng first because Irrlicht needs them
	mkdir -p libpng
	pushd libpng
	$srcdir/libpng/configure --host=$CROSS_PREFIX
	make && make DESTDIR=$PWD install
	popd

	mkdir -p libjpeg
	pushd libjpeg
	cmake $srcdir/libjpeg "${CMAKE_FLAGS[@]}" -DENABLE_SHARED=OFF
	make && make DESTDIR=$PWD install
	popd

	local libpng=$PWD/libpng/usr/local/lib/libpng.a
	local libjpeg=$(echo $PWD/libjpeg/opt/libjpeg-turbo/lib*/libjpeg.a)
	cmake $srcdir/irrlicht "${CMAKE_FLAGS[@]}" \
		-DBUILD_SHARED_LIBS=OFF \
		-DPNG_LIBRARY=$libpng \
		-DPNG_PNG_INCLUDE_DIR=$(dirname "$libpng")/../include \
		-DJPEG_LIBRARY=$libjpeg \
		-DJPEG_INCLUDE_DIR=$(dirname "$libjpeg")/../include
	make

	cp -p lib/Android/libIrrlichtMt.a $libpng $libjpeg $pkgdir/
	cp -a $srcdir/irrlicht/include $pkgdir/include
	cp -a $srcdir/irrlicht/media/Shaders $pkgdir/Shaders
}

get_tar_archive () {
	# $1: folder to extract to, $2: URL
	local filename="${2##*/}"
	[ -d "$1" ] && return 0
	wget -c "$2" -O "$filename"
	mkdir -p "$1"
	tar -xaf "$filename" -C "$1" --strip-components=1
	rm "$filename"
}

_setup_toolchain () {
	local toolchain=$(echo "$ANDROID_NDK"/toolchains/llvm/prebuilt/*)
	if [ ! -d "$toolchain" ]; then
		echo "Android NDK path not specified or incorrect"; return 1
	fi
	export PATH="$toolchain/bin:$ANDROID_NDK:$PATH"

	unset CFLAGS CPPFLAGS CXXFLAGS

	TARGET_ABI="$1"
	API=21
	if [ "$TARGET_ABI" == armeabi-v7a ]; then
		CROSS_PREFIX=armv7a-linux-androideabi
		CFLAGS="-mthumb"
		CXXFLAGS="-mthumb"
	elif [ "$TARGET_ABI" == arm64-v8a ]; then
		CROSS_PREFIX=aarch64-linux-android
	elif [ "$TARGET_ABI" == x86 ]; then
		CROSS_PREFIX=i686-linux-android
		CFLAGS="-mssse3 -mfpmath=sse"
		CXXFLAGS="-mssse3 -mfpmath=sse"
	elif [ "$TARGET_ABI" == x86_64 ]; then
		CROSS_PREFIX=x86_64-linux-android
	else
		echo "Invalid ABI given"; return 1
	fi
	export CC=$CROSS_PREFIX$API-clang
	export CXX=$CROSS_PREFIX$API-clang++
	export AR=llvm-ar
	export RANLIB=llvm-ranlib
	export CFLAGS="-fPIC ${CFLAGS}"
	export CXXFLAGS="-fPIC ${CXXFLAGS}"

	CMAKE_FLAGS=(
		"-DCMAKE_TOOLCHAIN_FILE=$ANDROID_NDK/build/cmake/android.toolchain.cmake"
		"-DANDROID_ABI=$TARGET_ABI" "-DANDROID_NATIVE_API_LEVEL=$API"
		"-DCMAKE_BUILD_TYPE=Release"
	)

	# make sure pkg-config doesn't interfere
	export PKG_CONFIG=/bin/false

	export MAKEFLAGS="-j$(nproc)"
}

_run_build () {
	local abi=$1
	irrdir=$PWD

	mkdir -p $RUNNER_TEMP/src
	cd $RUNNER_TEMP/src
	srcdir=$PWD
	[ -d irrlicht ] || ln -s $irrdir irrlicht
	download

	builddir=$RUNNER_TEMP/build/irrlicht-$abi
	pkgdir=$RUNNER_TEMP/pkg/$abi/Irrlicht
	rm -rf "$pkgdir"
	mkdir -p "$builddir" "$pkgdir"

	cd "$builddir"
	build
}

if [ $# -lt 1 ]; then
	echo "Usage: ci-build-android.sh <ABI>"
	exit 1
fi

_setup_toolchain $1
_run_build $1
