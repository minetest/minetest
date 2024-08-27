#!/bin/bash -e

download_macos_archive() {
	wget -O $1 $2
	checksum=$(shasum -a 256 $1)
	if [[ "$checksum" != "$3  $1" ]]; then
		echo "Downloaded file $1 has unexpected checksum $checksum."
		exit 1
	fi
}

download_macos_sdk() {
	osx=$1

	sysroot="/Library/Developer/CommandLineTools/SDKs/MacOSX${osx}.sdk"
	sdkfile="MacOSX${osx}.sdk"

	dir=$(pwd)

	mkdir deps
	cd deps

	echo "Downloading $sdkfile..."
	if [[ "$osx" == "10.14" ]]; then
		download_macos_archive SDK.tar.bz2 \
				https://github.com/alexey-lysiuk/macos-sdk/releases/download/10.14/MacOSX10.14.tar.bz2 \
				e0a9747ae9838aeac70430c005f9757587aa055c690383d91165cf7b4a80401d
		tar -xf SDK.tar.bz2
	elif [[ "$osx" == "10.15" ]]; then
		download_macos_archive SDK.tar.bz2 \
				https://github.com/alexey-lysiuk/macos-sdk/releases/download/10.15/MacOSX10.15.tar.bz2 \
				bb548125ebf5cf4ae6c80d226f40ad39e155564ca78bc00554a2e84074d0180e
		tar -xf SDK.tar.bz2
	elif [[ "$osx" == "11" ]]; then
		download_macos_archive SDK.tar.bz2 \
				https://github.com/alexey-lysiuk/macos-sdk/releases/download/11.3/MacOSX11.3.tar.bz2 \
				d6604578f4ee3090d1c3efce1e5c336ecfd7be345d046c729189d631ea3b8ec6
		tar -xf SDK.tar.bz2
		ln -s MacOSX11.3.sdk MacOSX11.sdk
	else
		echo "This SDK target is not supported."
		exit 1
	fi

	cd $dir
}

download_macos_deps() {
	brew install cmake nasm wget m4 autoconf automake libtool

	cd deps

	echo "Downloading sources..."
	download_macos_archive gettext.tar.gz \
			https://ftp.gnu.org/gnu/gettext/gettext-0.22.5.tar.gz \
			ec1705b1e969b83a9f073144ec806151db88127f5e40fe5a94cb6c8fa48996a0
	download_macos_archive freetype.tar.xz \
			https://downloads.sourceforge.net/project/freetype/freetype2/2.13.3/freetype-2.13.3.tar.xz \
			0550350666d427c74daeb85d5ac7bb353acba5f76956395995311a9c6f063289
	download_macos_archive gmp.tar.xz \
			https://ftp.gnu.org/gnu/gmp/gmp-6.3.0.tar.xz \
			a3c2b80201b89e68616f4ad30bc66aee4927c3ce50e33929ca819d5c43538898
	download_macos_archive libjpeg-turbo.tar.gz \
			https://github.com/libjpeg-turbo/libjpeg-turbo/releases/download/3.0.3/libjpeg-turbo-3.0.3.tar.gz \
			343e789069fc7afbcdfe44dbba7dbbf45afa98a15150e079a38e60e44578865d
	download_macos_archive jsoncpp.tar.gz \
			https://github.com/open-source-parsers/jsoncpp/archive/refs/tags/1.9.5.tar.gz \
			f409856e5920c18d0c2fb85276e24ee607d2a09b5e7d5f0a371368903c275da2
	download_macos_archive leveldb.tar.gz \
			https://github.com/google/leveldb/archive/refs/tags/1.23.tar.gz \
			9a37f8a6174f09bd622bc723b55881dc541cd50747cbd08831c2a82d620f6d76
	download_macos_archive libogg.tar.gz \
			https://ftp.osuosl.org/pub/xiph/releases/ogg/libogg-1.3.5.tar.gz \
			0eb4b4b9420a0f51db142ba3f9c64b333f826532dc0f48c6410ae51f4799b664
	download_macos_archive libpng.tar.xz \
			https://downloads.sourceforge.net/project/libpng/libpng16/1.6.43/libpng-1.6.43.tar.xz \
			6a5ca0652392a2d7c9db2ae5b40210843c0bbc081cbd410825ab00cc59f14a6c
	download_macos_archive libvorbis.tar.gz \
			https://github.com/sfence/libvorbis/archive/refs/tags/v1.3.7_macos_apple_silicon.tar.gz \
			61dd22715136f13317326ea60f9c1345529fbc1bf84cab99d6b7a165bf86a609
	download_macos_archive luajit.tar.gz \
			https://github.com/LuaJIT/LuaJIT/archive/f725e44cda8f359869bf8f92ce71787ddca45618.tar.gz \
			2b5514bd6a6573cb6111b43d013e952cbaf46762d14ebe26c872ddb80b5a84e0
	download_macos_archive zstd.tar.gz \
			https://github.com/facebook/zstd/archive/refs/tags/v1.5.6.tar.gz \
			30f35f71c1203369dc979ecde0400ffea93c27391bfd2ac5a9715d2173d92ff7
	# leveldb depends
	download_macos_archive snappy.tar.gz \
			https://github.com/google/snappy/archive/refs/tags/1.2.1.tar.gz \
			736aeb64d86566d2236ddffa2865ee5d7a82d26c9016b36218fcc27ea4f09f86

	cd ..
}

untar_macos_deps() {
	cd deps

	echo "Unarchiving sources..."
	tar -xf libpng.tar.xz
	tar -xf gettext.tar.gz
	tar -xf freetype.tar.xz
	tar -xf gmp.tar.xz
	tar -xf libjpeg-turbo.tar.gz
	tar -xf jsoncpp.tar.gz
	tar -xf leveldb.tar.gz
	tar -xf libogg.tar.gz
	tar -xf libvorbis.tar.gz
	tar -xf luajit.tar.gz
	tar -xf zstd.tar.gz

	tar -xf snappy.tar.gz

	cd ..
}

compile_macos_deps() {
	arch=$1
	osx=$2

	cd deps
	mkdir install

	dir=$(pwd)

	sysroot="$dir/MacOSX${osx}.sdk"
	if [ ! -d "$sysroot" ]; then
		echo "Requested sysroot SDK does not found MacOSX${osx}.sdk"
		exit 1
	fi

	includecxx=""
	if [[ ! -d "$sysroot/usr/include/c++/v1" ]]; then
		includecxx="-I$(xcrun --show-sdk-path)/usr/include/c++/v1"
	fi

	export MACOSX_DEPLOYMENT_TARGET=$osx
	export MACOS_DEPLOYMENT_TARGET=$osx
	export CMAKE_PREFIX_PATH=$dir/install
	# needs to be exported for LuaJIT compilation
	export CPPFLAGS="-arch $arch"
	export CC="clang -arch $arch"
	#export CFLAGS="-arch $arch"
	export CXX="clang++ -arch $arch -isysroot $sysroot $includecxx"
	#export CXXFLAGS="-arch $arch -isysroot $sysroot"
	export LDFLAGS="-arch $arch"
  export SDKROOT=$sysroot
	hostdarwin="--host=${arch}-apple-darwin"
	hostdmacos="--host=${arch}-apple-macos${osx}"

	# libpng
	cd libpng-*
	echo "Configuring libpng..."
	./configure "--prefix=$dir/install" $hostdarwin
	echo "Building libpng..."
	make -j$(sysctl -n hw.logicalcpu)
	make check
	make install
	cd $dir

	# freetype
	cd freetype-*
	echo "Configuring freetype..."
	./configure "--prefix=${dir}/install" "LIBPNG_LIBS=-L${dir}/install/lib -lpng" \
							"LIBPNG_CFLAGS=-I${dir}/install/include" \
							--with-harfbuzz=no --with-brotli=no $hostdarwin
	echo "Building freetype..."
	make -j$(sysctl -n hw.logicalcpu)
	make install
	cd $dir

	# gettext
	cd gettext-*
	echo "Configuring gettext..."
	./configure "--prefix=$dir/install" --disable-silent-rules --with-included-glib \
							--with-included-libcroco --with-included-libunistring \
							--with-included-libxml --with-emacs --disable-java --disable-csharp \
							--without-git --without-cvs --without-xz --with-included-gettext
	echo "Building gettext..."
	make -j$(sysctl -n hw.logicalcpu)
	make install
	cd $dir

	# gmp
	cd gmp-*
	echo "Configuring gmp..."
	# different Cellar location on Intel and Arm MacOS
	#./configure "--prefix=$dir/install" --with-pic --enable-cxx \
	#							M4=/usr/local/Cellar/m4/1.4.19/bin/m4
	./configure "--prefix=$dir/install" --with-pic \
							M4=/opt/homebrew/Cellar/m4/1.4.19/bin/m4 $hostdarwin
	echo "Building gmp..."
	make -j$(sysctl -n hw.logicalcpu)
	make check
	make install
	cd $dir

	# libjpeg-turbo
	cd libjpeg-turbo-*
	echo "Configuring libjpeg-turbo..."
	cmake . "-DCMAKE_INSTALL_PREFIX:PATH=$dir/install" \
					-DCMAKE_OSX_ARCHITECTURES=$arch \
					-DCMAKE_INSTALL_NAME_DIR=$dir/install/lib
	echo "Building libjpeg-turbo..."
	make -j$(sysctl -n hw.logicalcpu)
	make install "PREFIX=$dir/install"
	cd $dir

	# jsoncpp
	cd jsoncpp-*
	mkdir build
	cd build
	echo "Configuring jsoncpp..."
	cmake .. "-DCMAKE_INSTALL_PREFIX:PATH=$dir/install" \
					-DCMAKE_OSX_ARCHITECTURES=$arch \
					-DCMAKE_INSTALL_NAME_DIR=$dir/install/lib
	echo "Building jsoncpp..."
	make -j$(sysctl -n hw.logicalcpu)
	make install
	cd $dir

	# snappy
	cd snappy-*
	echo "Configuring snappy..."
	cmake . "-DCMAKE_INSTALL_PREFIX:PATH=$dir/install" -DSNAPPY_BUILD_TESTS=OFF \
					-DSNAPPY_BUILD_BENCHMARKS=OFF -DCMAKE_OSX_ARCHITECTURES=$arch \
					-DCMAKE_INSTALL_NAME_DIR=$dir/install/lib
	echo "Building snappy..."
	make -j$(sysctl -n hw.logicalcpu)
	make install
	cd $dir

	# leveldb
	cd leveldb-*
	echo "Configuring leveldb..."
	cmake . "-DCMAKE_INSTALL_PREFIX:PATH=$dir/install" -DLEVELDB_BUILD_TESTS=OFF \
					-DLEVELDB_BUILD_BENCHMARKS=OFF -DCMAKE_BUILD_WITH_INSTALL_RPATH=ON \
					-DBUILD_SHARED_LIBS=ON -DCMAKE_OSX_ARCHITECTURES=$arch \
					-DCMAKE_INSTALL_NAME_DIR=$dir/install/lib
	echo "Building leveldb..."
	make -j$(sysctl -n hw.logicalcpu)
	make install
	cd $dir

	# libogg
	cd libogg-*
	echo "Configuring libogg..."
	./configure "--prefix=$dir/install" $hostmacos
	echo "Building libogg..."
	make -j$(sysctl -n hw.logicalcpu)
	make install
	cd $dir

	# libvorbis
	cd libvorbis-*
	echo "Configuring libvorbis..."
	./autogen.sh
	./configure "--prefix=$dir/install"	"--with-ogg=${dir}/install" $hostdarwin
	echo "Building libvorbis..."
	make -j$(sysctl -n hw.logicalcpu)
	make install
	cd $dir

	# luajit
	cd LuaJIT-*
	echo "Building LuaJIT..."
	jit_flags="-arch $arch -isysroot $sysroot"
	make -j$(sysctl -n hw.logicalcpu) "PREFIX=$dir/install" \
				"CFLAGS=$jit_flags" "HOST_CFLAGS=$jit_flags" "TARGET_CFLAGS=$jit_flags"
	make install "PREFIX=$dir/install" \
				"CFLAGS=$jit_flags" "HOST_CFLAGS=$jit_flags" "TARGET_CFLAGS=$jit_flags"
	cd $dir

	# zstd
	cd zstd-*
	cd build/cmake
	echo "Configuring zstd..."
	cmake . "-DCMAKE_INSTALL_PREFIX:PATH=$dir/install" -DCMAKE_OSX_ARCHITECTURES=$arch \
					-DCMAKE_INSTALL_NAME_DIR=$dir/install/lib
	echo "Building zstd..."
	make -j$(sysctl -n hw.logicalcpu)
	make install
	cd $dir

	cd ..
}
