# build options

OS := $(shell uname)

# compile with GPROF
# GPROF = 1

# build for build platform
APP_PLATFORM = android-9

ANDR_ROOT = $(shell pwd)
PROJ_ROOT = $(shell realpath $(ANDR_ROOT)/../..)
APP_ROOT = $(ANDR_ROOT)/src/main

GAMES_TO_COPY = minetest_game
MODS_TO_COPY =


VERSION_MAJOR := $(shell cat $(PROJ_ROOT)/CMakeLists.txt | \
	grep ^set\(VERSION_MAJOR\ | sed 's/)/ /' | cut -f2 -d' ')
VERSION_MINOR := $(shell cat $(PROJ_ROOT)/CMakeLists.txt | \
	grep ^set\(VERSION_MINOR\ | sed 's/)/ /' | cut -f2 -d' ')
VERSION_PATCH := $(shell cat $(PROJ_ROOT)/CMakeLists.txt | \
	grep ^set\(VERSION_PATCH\ | sed 's/)/ /' | cut -f2 -d' ')

################################################################################
# toolchain config for arm new processors
################################################################################
TARGET_HOST = arm-linux
TARGET_ABI = armeabi-v7a
TARGET_LIBDIR = armeabi-v7a
TARGET_TOOLCHAIN = arm-linux-androideabi-
TARGET_CFLAGS_ADDON = -mfloat-abi=softfp -mfpu=vfpv3 -O3
TARGET_CXXFLAGS_ADDON = $(TARGET_CFLAGS_ADDON)
TARGET_ARCH = armv7
CROSS_PREFIX = arm-linux-androideabi-
COMPILER_VERSION = 4.9
HAVE_LEVELDB = 1

################################################################################
# toolchain config for little endian mips
################################################################################
#TARGET_HOST = mipsel-linux
#TARGET_ABI = mips
#TARGET_LIBDIR = mips
#TARGET_TOOLCHAIN = mipsel-linux-android-
#TARGET_ARCH = mips32
#CROSS_PREFIX = mipsel-linux-android-
#COMPILER_VERSION = 4.9
#HAVE_LEVELDB = 0

################################################################################
# toolchain config for x86
################################################################################
#TARGET_HOST = x86-linux
#TARGET_ABI = x86
#TARGET_LIBDIR = x86
#TARGET_TOOLCHAIN = x86-
#CROSS_PREFIX = i686-linux-android-
#TARGET_ARCH = x86
#COMPILER_VERSION = 4.9
#HAVE_LEVELDB = 1

################################################################################
ASSETS_TIMESTAMP = deps/assets_timestamp

LEVELDB_DIR = $(ANDR_ROOT)/deps/leveldb/
LEVELDB_LIB = $(LEVELDB_DIR)libleveldb.a
LEVELDB_TIMESTAMP = $(LEVELDB_DIR)/timestamp
LEVELDB_TIMESTAMP_INT = $(ANDR_ROOT)/deps/leveldb_timestamp
LEVELDB_URL_GIT = https://github.com/google/leveldb
LEVELDB_COMMIT = 2d0320a458d0e6a20fff46d5f80b18bfdcce7018

OPENAL_DIR = $(ANDR_ROOT)/deps/openal-soft/
OPENAL_LIB = $(OPENAL_DIR)libs/$(TARGET_ABI)/libopenal.so
OPENAL_TIMESTAMP = $(OPENAL_DIR)/timestamp
OPENAL_TIMESTAMP_INT = $(ANDR_ROOT)/deps/openal_timestamp
OPENAL_URL_GIT = https://github.com/apportable/openal-soft

OGG_DIR = $(ANDR_ROOT)/deps/libvorbis-libogg-android/
OGG_LIB = $(OGG_DIR)libs/$(TARGET_ABI)/libogg.so
VORBIS_LIB = $(OGG_DIR)libs/$(TARGET_ABI)/libogg.so
OGG_TIMESTAMP = $(OGG_DIR)timestamp
OGG_TIMESTAMP_INT = $(ANDR_ROOT)/deps/ogg_timestamp
OGG_URL_GIT = https://github.com/vincentjames501/libvorbis-libogg-android

IRRLICHT_REVISION = 5122
IRRLICHT_DIR = $(ANDR_ROOT)/deps/irrlicht/
IRRLICHT_LIB = $(IRRLICHT_DIR)lib/Android/libIrrlicht.a
IRRLICHT_TIMESTAMP = $(IRRLICHT_DIR)timestamp
IRRLICHT_TIMESTAMP_INT = $(ANDR_ROOT)/deps/irrlicht_timestamp
IRRLICHT_URL_SVN = https://svn.code.sf.net/p/irrlicht/code/branches/ogl-es@$(IRRLICHT_REVISION)

OPENSSL_VERSION = 1.0.2j
OPENSSL_BASEDIR = openssl-$(OPENSSL_VERSION)
OPENSSL_DIR = $(ANDR_ROOT)/deps/$(OPENSSL_BASEDIR)/
OPENSSL_LIB = $(OPENSSL_DIR)/libssl.so.1.0.0
OPENSSL_TIMESTAMP = $(OPENSSL_DIR)timestamp
OPENSSL_TIMESTAMP_INT = $(ANDR_ROOT)/deps/openssl_timestamp
OPENSSL_URL = https://www.openssl.org/source/openssl-$(OPENSSL_VERSION).tar.gz

CURL_VERSION = 7.52.0
CURL_DIR = $(ANDR_ROOT)/deps/curl-$(CURL_VERSION)
CURL_LIB = $(CURL_DIR)/lib/.libs/libcurl.a
CURL_TIMESTAMP = $(CURL_DIR)/timestamp
CURL_TIMESTAMP_INT = $(ANDR_ROOT)/deps/curl_timestamp
CURL_URL_HTTP = https://curl.haxx.se/download/curl-${CURL_VERSION}.tar.bz2

GMP_VERSION = 6.1.2
GMP_DIR = $(ANDR_ROOT)/deps/gmp-$(GMP_VERSION)
GMP_LIB = $(GMP_DIR)/usr/lib/libgmp.so
GMP_TIMESTAMP = $(GMP_DIR)/timestamp
GMP_TIMESTAMP_INT = $(ANDR_ROOT)/deps/gmp_timestamp
GMP_URL_HTTP = https://gmplib.org/download/gmp/gmp-$(GMP_VERSION).tar.bz2

FREETYPE_DIR = $(ANDR_ROOT)/deps/freetype2-android/
FREETYPE_LIB = $(FREETYPE_DIR)/Android/obj/local/$(TARGET_ABI)/libfreetype2-static.a
FREETYPE_TIMESTAMP = $(FREETYPE_DIR)timestamp
FREETYPE_TIMESTAMP_INT = $(ANDR_ROOT)/deps/freetype_timestamp
FREETYPE_URL_GIT = https://github.com/cdave1/freetype2-android

ICONV_VERSION = 1.14
ICONV_DIR = $(ANDR_ROOT)/deps/libiconv/
ICONV_LIB = $(ICONV_DIR)/lib/.libs/libiconv.so
ICONV_TIMESTAMP = $(ICONV_DIR)timestamp
ICONV_TIMESTAMP_INT = $(ANDR_ROOT)/deps/iconv_timestamp
ICONV_URL_HTTP = https://ftp.gnu.org/pub/gnu/libiconv/libiconv-$(ICONV_VERSION).tar.gz

SQLITE3_FOLDER = sqlite-amalgamation-3150200
SQLITE3_URL = https://www.sqlite.org/2016/$(SQLITE3_FOLDER).zip

ANDROID_SDK = $(shell grep '^sdk\.dir' local.properties | sed 's/^.*=[[:space:]]*//')
ANDROID_NDK = $(shell grep '^ndk\.dir' local.properties | sed 's/^.*=[[:space:]]*//')
NDK_MODULE_PATH = $(ANDROID_NDK)/toolchains

#use interim target variable to switch leveldb on or off
ifeq ($(HAVE_LEVELDB),1)
	LEVELDB_TARGET = $(LEVELDB_LIB)
endif

.PHONY : debug release reconfig delconfig \
	leveldb_download clean_leveldb leveldb\
	irrlicht_download clean_irrlicht irrlicht \
	clean_assets assets sqlite3_download \
	freetype_download clean_freetype freetype \
	apk clean_apk \
	clean_all clean prep_srcdir \
	install_debug install_release envpaths all \
	$(ASSETS_TIMESTAMP) $(LEVELDB_TIMESTAMP) \
	$(OPENAL_TIMESTAMP) $(OGG_TIMESTAMP) \
	$(IRRLICHT_TIMESTAMP) $(CURL_TIMESTAMP) \
	$(OPENSSL_TIMESTAMP) \
	$(ANDR_ROOT)/jni/src/android_version.h \
	$(ANDR_ROOT)/jni/src/android_version_githash.h

debug : local.properties
	export NDEBUG=;                                                            \
	export BUILD_TYPE=debug;                                                   \
	$(MAKE) apk

all : debug release

release : local.properties
	@export NDEBUG=1;                                                          \
	export BUILD_TYPE=release;                                                 \
	$(MAKE) apk

reconfig: delconfig
	@$(MAKE) local.properties

delconfig:
	$(RM) local.properties

local.properties:
	@echo "Please specify path of ANDROID NDK";                                \
	echo "e.g. $$HOME/Android/ndk-r11c/";                                  \
	read ANDROID_NDK ;                                                         \
	if [ ! -d $$ANDROID_NDK ] ; then                                           \
		echo "$$ANDROID_NDK is not a valid folder";                            \
		exit 1;                                                                \
	fi;                                                                        \
	echo "ndk.dir = $$ANDROID_NDK" > local.properties;                         \
	echo "Please specify path of ANDROID SDK";                                 \
	echo "e.g. $$HOME/Android/sdk/";                                       \
	read SDKFLDR ;                                                             \
	if [ ! -d $$SDKFLDR ] ; then                                               \
		echo "$$SDKFLDR is not a valid folder";                                \
		exit 1;                                                                \
	fi;                                                                        \
	echo "sdk.dir = $$SDKFLDR" >> local.properties;


$(OPENAL_TIMESTAMP) : openal_download
	@LAST_MODIF=$$(find ${OPENAL_DIR} -type f -printf '%T@ %p\n' | sort -n | tail -1 | cut -f2- -d" "); \
	if [ $$(basename $$LAST_MODIF) != "timestamp" ] ; then                     \
		touch ${OPENAL_TIMESTAMP};                                            \
	fi

openal_download :
	@if [ ! -d ${OPENAL_DIR} ] ; then                                          \
		echo "openal sources missing, downloading...";                         \
		mkdir -p ${ANDR_ROOT}/deps;                                            \
		cd ${ANDR_ROOT}/deps ;                                                 \
		git clone ${OPENAL_URL_GIT} || exit 1;                                 \
	fi

openal : $(OPENAL_LIB)

$(OPENAL_LIB): $(OPENAL_TIMESTAMP)
	+ @REFRESH=0;                                                                \
	if [ ! -e ${OPENAL_TIMESTAMP_INT} ] ; then                                 \
		REFRESH=1;                                                             \
	fi;                                                                        \
	if [ ${OPENAL_TIMESTAMP} -nt ${OPENAL_TIMESTAMP_INT} ] ; then              \
		REFRESH=1;                                                             \
	fi;                                                                        \
	if [ $$REFRESH -ne 0 ] ; then                                              \
	echo "changed timestamp for openal detected building...";                  \
	cd ${OPENAL_DIR};                                                          \
	${ANDROID_NDK}/ndk-build NDEBUG=${NDEBUG}                                  \
		NDK_MODULE_PATH=${NDK_MODULE_PATH} APP_ABI=${TARGET_ABI}               \
		TARGET_ARCH_ABI=${TARGET_ABI} APP_PLATFORM=${APP_PLATFORM}             \
		PRIVATE_CC=${NDK_MODULE_PATH}/${TARGET_TOOLCHAIN}${COMPILER_VERSION}/prebuilt/linux-x86_64/bin/${TARGET_TOOLCHAIN}gcc \
		PRIVATE_CXX=${NDK_MODULE_PATH}/${TARGET_TOOLCHAIN}${COMPILER_VERSION}/prebuilt/linux-x86_64/bin/${TARGET_TOOLCHAIN}g++ \
		TARGET_CFLAGS+="${TARGET_CFLAGS_ADDON}"                                \
		TARGET_LDFLAGS+="${TARGET_LDFLAGS_ADDON}"                              \
		TARGET_CXXFLAGS+="${TARGET_CXXFLAGS_ADDON}" || exit 1;                 \
	touch ${OPENAL_TIMESTAMP};                                                 \
	touch ${OPENAL_TIMESTAMP_INT};                                             \
	else                                                                       \
		echo "nothing to be done for openal";                                  \
	fi

clean_openal :
	$(RM) -rf ${OPENAL_DIR}

$(OGG_TIMESTAMP) : ogg_download
	@LAST_MODIF=$$(find ${OGG_DIR} -type f -printf '%T@ %p\n' | sort -n | tail -1 | cut -f2- -d" "); \
	if [ $$(basename $$LAST_MODIF) != "timestamp" ] ; then                     \
		touch ${OGG_TIMESTAMP};                                                \
	fi

ogg_download :
	@if [ ! -d ${OGG_DIR} ] ; then                                             \
		echo "ogg sources missing, downloading...";                            \
		mkdir -p ${ANDR_ROOT}/deps;                                            \
		cd ${ANDR_ROOT}/deps ;                                                 \
		git clone ${OGG_URL_GIT}|| exit 1;                                     \
		cd libvorbis-libogg-android ;                                          \
		patch -p1 < ${ANDR_ROOT}/patches/libvorbis-libogg-fpu.patch || exit 1; \
		sed -i 's-:-?-' jni/Application.mk;                                    \
	fi

ogg : $(OGG_LIB)

$(OGG_LIB): $(OGG_TIMESTAMP)
	+ @REFRESH=0;                                                                \
	if [ ! -e ${OGG_TIMESTAMP_INT} ] ; then                                    \
		echo "${OGG_TIMESTAMP_INT} doesn't exist";                             \
		REFRESH=1;                                                             \
	fi;                                                                        \
	if [ ${OGG_TIMESTAMP} -nt ${OGG_TIMESTAMP_INT} ] ; then                    \
		REFRESH=1;                                                             \
	fi;                                                                        \
	if [ $$REFRESH -ne 0 ] ; then                                              \
	echo "changed timestamp for ogg detected building...";                     \
	cd ${OGG_DIR};                                                             \
	${ANDROID_NDK}/ndk-build NDEBUG=${NDEBUG}                                  \
		NDK_MODULE_PATH=${NDK_MODULE_PATH}                                     \
		APP_ABI=${TARGET_ABI} APP_PLATFORM=${APP_PLATFORM}                     \
		PRIVATE_CC=${NDK_MODULE_PATH}/${TARGET_TOOLCHAIN}${COMPILER_VERSION}/prebuilt/linux-x86_64/bin/${TARGET_TOOLCHAIN}gcc \
		PRIVATE_CXX=${NDK_MODULE_PATH}/${TARGET_TOOLCHAIN}${COMPILER_VERSION}/prebuilt/linux-x86_64/bin/${TARGET_TOOLCHAIN}g++ \
		TARGET_CFLAGS+="${TARGET_CFLAGS_ADDON}"                                \
		TARGET_LDFLAGS+="${TARGET_LDFLAGS_ADDON}"                              \
		TARGET_CXXFLAGS+="${TARGET_CXXFLAGS_ADDON}" || exit 1;                 \
	touch ${OGG_TIMESTAMP};                                                    \
	touch ${OGG_TIMESTAMP_INT};                                                \
	else                                                                       \
		echo "nothing to be done for libogg/libvorbis";                        \
	fi

clean_ogg :
	$(RM) -rf ${OGG_DIR}

$(OPENSSL_TIMESTAMP) : openssl_download
	@LAST_MODIF=$$(find ${OPENSSL_DIR} -type f -printf '%T@ %p\n' | sort -n | tail -1 | cut -f2- -d" "); \
	if [ $$(basename $$LAST_MODIF) != "timestamp" ] ; then                     \
		touch ${OPENSSL_TIMESTAMP};                                            \
	fi

openssl_download :
	@if [ ! -d ${OPENSSL_DIR} ] ; then                                         \
		echo "openssl sources missing, downloading...";                        \
		mkdir -p ${ANDR_ROOT}/deps;                                            \
		cd ${ANDR_ROOT}/deps ;                                                 \
		wget ${OPENSSL_URL} || exit 1;                                         \
		tar -xzf ${OPENSSL_BASEDIR}.tar.gz;                                    \
		cd ${OPENSSL_BASEDIR};                                                 \
		patch -p1 < ${ANDR_ROOT}/patches/openssl_arch.patch;                   \
	fi

openssl : $(OPENSSL_LIB)

$(OPENSSL_LIB): $(OPENSSL_TIMESTAMP) $(GMP_LIB)
	@REFRESH=0;                                                                \
	if [ ! -e ${OPENSSL_TIMESTAMP_INT} ] ; then                                \
		echo "${OPENSSL_TIMESTAMP_INT} doesn't exist";                         \
		REFRESH=1;                                                             \
	fi;                                                                        \
	if [ ${OPENSSL_TIMESTAMP} -nt ${OPENSSL_TIMESTAMP_INT} ] ; then            \
		REFRESH=1;                                                             \
	fi;                                                                        \
	if [ $$REFRESH -ne 0 ] ; then                                              \
	echo "changed timestamp for openssl detected building...";                 \
	cd ${OPENSSL_DIR};                                                         \
	ln -s ${OPENSSL_DIR} ../openssl;                                           \
	export TOOLCHAIN=/tmp/ndk-${TARGET_HOST}-openssl;                          \
	${ANDROID_NDK}/build/tools/make-standalone-toolchain.sh                    \
		--toolchain=${TARGET_TOOLCHAIN}${COMPILER_VERSION}                     \
		--platform=android-9                                                   \
		--install-dir=$${TOOLCHAIN};                                           \
	export PATH="$${TOOLCHAIN}/bin:$${PATH}";                                  \
	CC=${CROSS_PREFIX}gcc ./Configure enable-gmp -DL_ENDIAN -I${GMP_DIR} -L${GMP_DIR}/usr/lib android-${TARGET_ARCH};\
	CC=${CROSS_PREFIX}gcc ANDROID_DEV=/tmp/ndk-${TARGET_HOST} make depend;     \
	CC=${CROSS_PREFIX}gcc ANDROID_DEV=/tmp/ndk-${TARGET_HOST} make build_libs; \
	touch ${OPENSSL_TIMESTAMP};                                                \
	touch ${OPENSSL_TIMESTAMP_INT};                                            \
	$(RM) -rf $${TOOLCHAIN};                                                   \
	else                                                                       \
		echo "nothing to be done for openssl";                        \
	fi

clean_openssl :
	$(RM) -rf ${OPENSSL_DIR};                                                  \
	$(RM) -rf $(ANDR_ROOT)/deps/${OPENSSL_BASEDIR}.tar.gz;                     \
	$(RM) -rf $(ANDR_ROOT)/deps/openssl

$(LEVELDB_TIMESTAMP) : leveldb_download
	@LAST_MODIF=$$(find ${LEVELDB_DIR} -type f -printf '%T@ %p\n' | sort -n | tail -1 | cut -f2- -d" "); \
	if [ $$(basename $$LAST_MODIF) != "timestamp" ] ; then                     \
		touch ${LEVELDB_TIMESTAMP};                                            \
	fi

leveldb_download :
	@if [ ! -d ${LEVELDB_DIR} ] ; then                                         \
		echo "leveldb sources missing, downloading...";                        \
		mkdir -p ${ANDR_ROOT}/deps;                                            \
		cd ${ANDR_ROOT}/deps ;                                                 \
		git clone ${LEVELDB_URL_GIT} || exit 1;                                \
		cd ${LEVELDB_DIR} || exit 1;                                           \
		git checkout ${LEVELDB_COMMIT} || exit 1;                              \
	fi

leveldb : $(LEVELDB_LIB)

$(LEVELDB_LIB): $(LEVELDB_TIMESTAMP)
	@REFRESH=0;                                                                \
	if [ ! -e ${LEVELDB_TIMESTAMP_INT} ] ; then                                \
		REFRESH=1;                                                             \
	fi;                                                                        \
	if [ ${LEVELDB_TIMESTAMP} -nt ${LEVELDB_TIMESTAMP_INT} ] ; then            \
		REFRESH=1;                                                             \
	fi;                                                                        \
	if [ $$REFRESH -ne 0 ] ; then                                              \
	echo "changed timestamp for leveldb detected building...";                 \
	cd deps/leveldb;                                                           \
	export CROSS_PREFIX=${CROSS_PREFIX};                                       \
	export TOOLCHAIN=/tmp/ndk-${TARGET_HOST}-leveldb;                          \
	${ANDROID_NDK}/build/tools/make-standalone-toolchain.sh                    \
		--toolchain=${TARGET_TOOLCHAIN}${COMPILER_VERSION}                     \
		--platform=android-9                                                   \
		--install-dir=$${TOOLCHAIN};                                           \
	export PATH="$${TOOLCHAIN}/bin:$${PATH}";                                  \
	export CC=${CROSS_PREFIX}gcc;                                              \
	export CXX=${CROSS_PREFIX}g++;                                             \
	export CFLAGS="$${CFLAGS} ${TARGET_CFLAGS_ADDON}";                         \
	export CPPFLAGS="$${CPPFLAGS} ${TARGET_CXXFLAGS_ADDON}";                     \
	export LDFLAGS="$${LDFLAGS} ${TARGET_LDFLAGS_ADDON}";                      \
	export TARGET_OS=OS_ANDROID_CROSSCOMPILE;                                  \
	$(MAKE) || exit 1;                                                      \
	touch ${LEVELDB_TIMESTAMP};                                                \
	touch ${LEVELDB_TIMESTAMP_INT};                                            \
	$(RM) -rf $${TOOLCHAIN};                                                   \
	else                                                                       \
		echo "nothing to be done for leveldb";                                 \
	fi

clean_leveldb :
	$(RM) -rf deps/leveldb

$(FREETYPE_TIMESTAMP) : freetype_download
	@LAST_MODIF=$$(find ${FREETYPE_DIR} -type f -printf '%T@ %p\n' | sort -n | tail -1 | cut -f2- -d" "); \
	if [ $$(basename $$LAST_MODIF) != "timestamp" ] ; then                     \
		touch ${FREETYPE_TIMESTAMP};                                           \
	fi

freetype_download :
	@if [ ! -d ${FREETYPE_DIR} ] ; then                                        \
		echo "freetype sources missing, downloading...";                       \
		mkdir -p ${ANDR_ROOT}/deps;                                            \
		cd deps;                                                               \
		git clone ${FREETYPE_URL_GIT} || exit 1;                               \
	fi

freetype : $(FREETYPE_LIB)

$(FREETYPE_LIB) : $(FREETYPE_TIMESTAMP)
	+ @REFRESH=0;                                                                \
	if [ ! -e ${FREETYPE_TIMESTAMP_INT} ] ; then                               \
		REFRESH=1;                                                             \
	fi;                                                                        \
	if [ ! -e ${FREETYPE_LIB} ] ; then                                         \
		REFRESH=1;                                                             \
	fi;                                                                        \
	if [ ${FREETYPE_TIMESTAMP} -nt ${FREETYPE_TIMESTAMP_INT} ] ; then          \
		REFRESH=1;                                                             \
	fi;                                                                        \
	if [ $$REFRESH -ne 0 ] ; then                                              \
	mkdir -p ${FREETYPE_DIR};                                                  \
	echo "changed timestamp for freetype detected building...";                \
	cd ${FREETYPE_DIR}/Android/jni;                                            \
	${ANDROID_NDK}/ndk-build NDEBUG=${NDEBUG}                                  \
		NDK_MODULE_PATH=${NDK_MODULE_PATH}                                     \
		APP_PLATFORM=${APP_PLATFORM} APP_ABI=${TARGET_ABI}                     \
		PRIVATE_CC=${NDK_MODULE_PATH}/${TARGET_TOOLCHAIN}${COMPILER_VERSION}/prebuilt/linux-x86_64/bin/${TARGET_TOOLCHAIN}gcc \
		PRIVATE_CXX=${NDK_MODULE_PATH}/${TARGET_TOOLCHAIN}${COMPILER_VERSION}/prebuilt/linux-x86_64/bin/${TARGET_TOOLCHAIN}g++ \
		TARGET_CFLAGS+="${TARGET_CFLAGS_ADDON}"                                \
		TARGET_LDFLAGS+="${TARGET_LDFLAGS_ADDON}"                              \
		TARGET_CXXFLAGS+="${TARGET_CXXFLAGS_ADDON}" || exit 1;                 \
	touch ${FREETYPE_TIMESTAMP};                                               \
	touch ${FREETYPE_TIMESTAMP_INT};                                           \
	else                                                                       \
		echo "nothing to be done for freetype";                                \
	fi

clean_freetype :
	$(RM) -rf ${FREETYPE_DIR}

$(ICONV_TIMESTAMP) : iconv_download
	@LAST_MODIF=$$(find ${ICONV_DIR} -type f -printf '%T@ %p\n' | sort -n | tail -1 | cut -f2- -d" "); \
	if [ $$(basename $$LAST_MODIF) != "timestamp" ] ; then                     \
		touch ${ICONV_TIMESTAMP};                                           \
	fi

iconv_download :
	@if [ ! -d ${ICONV_DIR} ] ; then                                           \
		echo "iconv sources missing, downloading...";                      \
		mkdir -p ${ANDR_ROOT}/deps;                                        \
		cd ${ANDR_ROOT}/deps;                                              \
		wget ${ICONV_URL_HTTP} || exit 1;                                  \
		tar -xzf libiconv-${ICONV_VERSION}.tar.gz || exit 1;               \
		rm libiconv-${ICONV_VERSION}.tar.gz;                               \
		ln -s libiconv-${ICONV_VERSION} libiconv;                          \
		cd ${ICONV_DIR};                                                   \
		patch -p1 < ${ANDR_ROOT}/patches/libiconv_android.patch;           \
		patch -p1 < ${ANDR_ROOT}/patches/libiconv_stdio.patch;             \
	fi

iconv : $(ICONV_LIB)

$(ICONV_LIB) : $(ICONV_TIMESTAMP)
	@REFRESH=0;                                                                \
	if [ ! -e ${ICONV_TIMESTAMP_INT} ] ; then                                  \
		REFRESH=1;                                                         \
	fi;                                                                        \
	if [ ! -e ${ICONV_LIB} ] ; then                                            \
		REFRESH=1;                                                         \
	fi;                                                                        \
	if [ ${ICONV_TIMESTAMP} -nt ${ICONV_TIMESTAMP_INT} ] ; then                \
		REFRESH=1;                                                         \
	fi;                                                                        \
	if [ $$REFRESH -ne 0 ] ; then                                              \
	mkdir -p ${ICONV_DIR};                                                     \
	echo "changed timestamp for iconv detected building...";                   \
	cd ${ICONV_DIR};                                                           \
	\
	export TOOLCHAIN=/tmp/ndk-${TARGET_HOST}-iconv;                            \
	${ANDROID_NDK}/build/tools/make-standalone-toolchain.sh                    \
		--toolchain=${TARGET_TOOLCHAIN}${COMPILER_VERSION}                     \
		--platform=android-9                                                   \
		--install-dir=$${TOOLCHAIN};                                           \
	export PATH="$${TOOLCHAIN}/bin:$${PATH}";                                  \
	export CC=${CROSS_PREFIX}gcc;                                              \
	export CXX=${CROSS_PREFIX}g++;                                             \
	export TARGET_OS=OS_ANDROID_CROSSCOMPILE;                                  \
	./configure --host=${TARGET_HOST} || exit 1;                               \
	sed -i 's/LIBICONV_VERSION_INFO) /LIBICONV_VERSION_INFO) -avoid-version /g' lib/Makefile; \
	grep "iconv_LDFLAGS" src/Makefile;                                         \
	$(MAKE) -s || exit 1;                                                      \
	touch ${ICONV_TIMESTAMP};                                                  \
	touch ${ICONV_TIMESTAMP_INT};                                              \
	rm -rf ${TOOLCHAIN};                                                       \
	else                                                                       \
		echo "nothing to be done for iconv";                               \
	fi

clean_iconv :
	$(RM) -rf ${ICONV_DIR}

#Note: Texturehack patch is required for gpu's not supporting color format
#      correctly. Known bad GPU:
#      -geforce on emulator
#      -Vivante Corporation GC1000 core (e.g. Galaxy Tab 3)

irrlicht_download :
	@if [ ! -d "deps/irrlicht" ] ; then                                        \
		echo "irrlicht sources missing, downloading...";                       \
		mkdir -p ${ANDR_ROOT}/deps;                                            \
		cd deps;                                                               \
		svn co ${IRRLICHT_URL_SVN} irrlicht || exit 1;                         \
		cd irrlicht;                                                           \
		patch -p1 < ${ANDR_ROOT}/patches/irrlicht-touchcount.patch || exit 1;  \
		patch -p1 < ${ANDR_ROOT}/patches/irrlicht-back_button.patch || exit 1; \
		patch -p1 < ${ANDR_ROOT}/patches/irrlicht-texturehack.patch || exit 1; \
	fi

$(IRRLICHT_TIMESTAMP) : irrlicht_download
	@LAST_MODIF=$$(find ${IRRLICHT_DIR} -type f -printf '%T@ %p\n' | sort -n | tail -1 | cut -f2- -d" "); \
	if [ $$(basename $$LAST_MODIF) != "timestamp" ] ; then                     \
		touch ${IRRLICHT_TIMESTAMP};                                           \
	fi

irrlicht : $(IRRLICHT_LIB)

$(IRRLICHT_LIB): $(IRRLICHT_TIMESTAMP) $(FREETYPE_LIB)
	+ @REFRESH=0;                                                                \
	if [ ! -e ${IRRLICHT_TIMESTAMP_INT} ] ; then                               \
		REFRESH=1;                                                             \
	fi;                                                                        \
	if [ ! -e ${IRRLICHT_LIB} ] ; then                                         \
		REFRESH=1;                                                             \
	fi;                                                                        \
	if [ ${IRRLICHT_TIMESTAMP} -nt ${IRRLICHT_TIMESTAMP_INT} ] ; then          \
		REFRESH=1;                                                             \
	fi;                                                                        \
	if [ $$REFRESH -ne 0 ] ; then                                              \
	mkdir -p ${IRRLICHT_DIR};                                                  \
	echo "changed timestamp for irrlicht detected building...";                \
	cd deps/irrlicht/source/Irrlicht/Android;                                  \
	${ANDROID_NDK}/ndk-build NDEBUG=${NDEBUG}                                  \
		NDK_MODULE_PATH=${NDK_MODULE_PATH}                                     \
		APP_ABI=${TARGET_ABI} APP_PLATFORM=${APP_PLATFORM}                     \
		PRIVATE_CC=${NDK_MODULE_PATH}/${TARGET_TOOLCHAIN}${COMPILER_VERSION}/prebuilt/linux-x86_64/bin/${TARGET_TOOLCHAIN}gcc \
		PRIVATE_CXX=${NDK_MODULE_PATH}/${TARGET_TOOLCHAIN}${COMPILER_VERSION}/prebuilt/linux-x86_64/bin/${TARGET_TOOLCHAIN}g++ \
		TARGET_CFLAGS+="${TARGET_CFLAGS_ADDON}"                                \
		TARGET_LDFLAGS+="${TARGET_LDFLAGS_ADDON}"                              \
		TARGET_CXXFLAGS+="${TARGET_CXXFLAGS_ADDON}" || exit 1;                 \
	touch ${IRRLICHT_TIMESTAMP};                                               \
	touch ${IRRLICHT_TIMESTAMP_INT};                                           \
	else                                                                       \
		echo "nothing to be done for irrlicht";                                \
	fi

clean_irrlicht :
	$(RM) -rf deps/irrlicht

$(CURL_TIMESTAMP) : curl_download
	@LAST_MODIF=$$(find ${CURL_DIR} -type f -printf '%T@ %p\n' | sort -n | tail -1 | cut -f2- -d" "); \
	if [ $$(basename $$LAST_MODIF) != "timestamp" ] ; then                     \
		touch ${CURL_TIMESTAMP};                                               \
	fi

curl_download :
	@if [ ! -d "deps/curl-${CURL_VERSION}" ] ; then                            \
		echo "curl sources missing, downloading...";                           \
		mkdir -p ${ANDR_ROOT}/deps;                                            \
		cd deps;                                                               \
		wget ${CURL_URL_HTTP} || exit 1;                                       \
		tar -xjf curl-${CURL_VERSION}.tar.bz2 || exit 1;                       \
		rm curl-${CURL_VERSION}.tar.bz2;                                       \
		ln -s curl-${CURL_VERSION} curl;                                       \
	fi

curl : $(CURL_LIB)

$(CURL_LIB): $(CURL_TIMESTAMP) $(OPENSSL_LIB)
	@REFRESH=0;                                                                \
	if [ ! -e ${CURL_TIMESTAMP_INT} ] ; then                                   \
		REFRESH=1;                                                             \
	fi;                                                                        \
	if [ ! -e ${CURL_LIB} ] ; then                                             \
		REFRESH=1;                                                             \
	fi;                                                                        \
	if [ ${CURL_TIMESTAMP} -nt ${CURL_TIMESTAMP_INT} ] ; then                  \
		REFRESH=1;                                                             \
	fi;                                                                        \
	if [ $$REFRESH -ne 0 ] ; then                                              \
	mkdir -p ${CURL_DIR};                                                      \
	echo "changed timestamp for curl detected building...";                    \
	cd deps/curl-${CURL_VERSION};                                              \
	export CROSS_PREFIX=${CROSS_PREFIX};                                       \
	export TOOLCHAIN=/tmp/ndk-${TARGET_HOST}-curl;                             \
	${ANDROID_NDK}/build/tools/make-standalone-toolchain.sh                    \
		--toolchain=${TARGET_TOOLCHAIN}${COMPILER_VERSION}                     \
		--platform=android-9                                                   \
		--install-dir=$${TOOLCHAIN};                                           \
	export PATH="$${TOOLCHAIN}/bin:$${PATH}";                                  \
	export CC=${CROSS_PREFIX}gcc;                                              \
	export CXX=${CROSS_PREFIX}g++;                                             \
	export TARGET_OS=OS_ANDROID_CROSSCOMPILE;                                  \
	export CPPFLAGS="$${CPPFLAGS} -I${OPENSSL_DIR}/include ${TARGET_CFLAGS_ADDON}"; \
	export CFLAGS="$${CFLAGS} ${TARGET_CFLAGS_ADDON}";                         \
	export LDFLAGS="$${LDFLAGS} -L${OPENSSL_DIR} ${TARGET_LDFLAGS_ADDON}";     \
	./configure --host=${TARGET_HOST} --disable-shared --enable-static --with-ssl;  \
	$(MAKE) -s || exit 1;                                                      \
	touch ${CURL_TIMESTAMP};                                                   \
	touch ${CURL_TIMESTAMP_INT};                                               \
	$(RM) -rf $${TOOLCHAIN};                                                   \
	else                                                                       \
		echo "nothing to be done for curl";                                    \
	fi

clean_curl :
	$(RM) -rf deps/curl-${CURL_VERSION}                                        \
	$(RM) -f deps/curl

$(GMP_TIMESTAMP) : gmp_download
	@LAST_MODIF=$$(find ${GMP_DIR} -type f -printf '%T@ %p\n' | sort -n | tail -1 | cut -f2- -d" "); \
	if [ $$(basename $$LAST_MODIF) != "timestamp" ] ; then                     \
		touch ${GMP_TIMESTAMP};                                            \
	fi

gmp_download :
	@if [ ! -d "${GMP_DIR}" ] ; then                                           \
		echo "gmp sources missing, downloading...";                        \
		mkdir -p ${ANDR_ROOT}/deps;                                        \
		cd deps;                                                           \
		wget ${GMP_URL_HTTP} || exit 1;                                    \
		tar -xjf gmp-${GMP_VERSION}.tar.bz2 || exit 1;                     \
		rm gmp-${GMP_VERSION}.tar.bz2;                                     \
		ln -s gmp-${GMP_VERSION} gmp;                                      \
	fi

gmp : $(GMP_LIB)

$(GMP_LIB): $(GMP_TIMESTAMP)
	@REFRESH=0;                                                                \
	if [ ! -e ${GMP_TIMESTAMP_INT} ] ; then                                    \
		REFRESH=1;                                                         \
	fi;                                                                        \
	if [ ! -e ${GMP_LIB} ] ; then                                              \
		REFRESH=1;                                                         \
	fi;                                                                        \
	if [ ${GMP_TIMESTAMP} -nt ${GMP_TIMESTAMP_INT} ] ; then                    \
		REFRESH=1;                                                         \
	fi;                                                                        \
	if [ $$REFRESH -ne 0 ] ; then                                              \
	mkdir -p ${GMP_DIR};                                                       \
	echo "changed timestamp for gmp detected building...";                     \
	cd deps/gmp-${GMP_VERSION};                                                \
	export CROSS_PREFIX=${CROSS_PREFIX};                                       \
	export TOOLCHAIN=/tmp/ndk-${TARGET_HOST}-gmp;                              \
	${ANDROID_NDK}/build/tools/make-standalone-toolchain.sh                    \
		--toolchain=${TARGET_TOOLCHAIN}${COMPILER_VERSION}                     \
		--platform=android-9                                                   \
		--install-dir=$${TOOLCHAIN};                                           \
	export PATH="$${TOOLCHAIN}/bin:$${PATH}";                                  \
	export CC=${CROSS_PREFIX}gcc;                                              \
	export CXX=${CROSS_PREFIX}g++;                                             \
	export LIBGMP_LDFLAGS="-avoid-version";                                    \
	export LIBGMPXX_LDFLAGS="-avoid-version";                                  \
	./configure --disable-static --host=${TARGET_HOST} --prefix=/usr;          \
	$(MAKE) install DESTDIR=/${GMP_DIR} || exit 1;                             \
	touch ${GMP_TIMESTAMP};                                                    \
	touch ${GMP_TIMESTAMP_INT};                                                \
	$(RM) -rf $${TOOLCHAIN};                                                   \
	else                                                                       \
		echo "nothing to be done for gmp";                                 \
	fi

clean_gmp:
	$(RM) -rf deps/gmp-${GMP_VERSION}                                          \
	$(RM) -f deps/gmp

sqlite3_download: deps/${SQLITE3_FOLDER}/sqlite3.c

deps/${SQLITE3_FOLDER}/sqlite3.c :
	cd deps;                                                                    \
	wget ${SQLITE3_URL};                                                        \
	unzip ${SQLITE3_FOLDER}.zip;                                                \
	ln -s ${SQLITE3_FOLDER} sqlite;                                             \
	cd ${SQLITE3_FOLDER};                                                       \
	patch sqlite3.c < ${ANDR_ROOT}/patches/sqlite3-readonly-fix.patch

clean_sqlite3:
	cd deps && $(RM) -rf ${SQLITE3_FOLDER} && $(RM) -f ${SQLITE3_FOLDER}.zip && \
	$(RM) -f sqlite

$(ASSETS_TIMESTAMP) : $(IRRLICHT_LIB)
	@mkdir -p ${ANDR_ROOT}/deps;                                                \
	for DIRNAME in {builtin,client,doc,fonts,games,mods,po,textures}; do        \
		LAST_MODIF=$$(find ${PROJ_ROOT}/${DIRNAME} -type f -printf '%T@ %p\n' | sort -n | tail -1 | cut -f2- -d" "); \
		if [ $$(basename $$LAST_MODIF) != "timestamp" ]; then               \
			touch ${PROJ_ROOT}/${DIRNAME}/timestamp;                   \
			touch ${ASSETS_TIMESTAMP};                                  \
			echo ${DIRNAME} changed $$LAST_MODIF;                       \
		fi;                                                                 \
	done;                                                                       \
	LAST_MODIF=$$(find ${IRRLICHT_DIR}/media -type f -printf '%T@ %p\n' | sort -n | tail -1 | cut -f2- -d" "); \
	if [ $$(basename $$LAST_MODIF) != "timestamp" ] ; then                     \
		touch ${IRRLICHT_DIR}/media/timestamp;                                 \
		touch ${ASSETS_TIMESTAMP};                                             \
	fi;                                                                        \
	if [ ${PROJ_ROOT}/minetest.conf.example -nt ${ASSETS_TIMESTAMP} ] ; then   \
		echo "conf changed";                                                   \
		touch ${ASSETS_TIMESTAMP};                                             \
	fi;                                                                        \
	if [ ${PROJ_ROOT}/README.txt -nt ${ASSETS_TIMESTAMP} ] ; then              \
		touch ${ASSETS_TIMESTAMP};                                             \
	fi;                                                                        \
	if [ ! -e $(ASSETS_TIMESTAMP) ] ; then                                     \
		touch $(ASSETS_TIMESTAMP);                                             \
	fi

assets : $(ASSETS_TIMESTAMP)
	@REFRESH=0;                                                                \
	if [ ! -e ${ASSETS_TIMESTAMP}.old ] ; then                                 \
		REFRESH=1;                                                             \
	fi;                                                                        \
	if [ ${ASSETS_TIMESTAMP} -nt ${ASSETS_TIMESTAMP}.old ] ; then              \
		REFRESH=1;                                                             \
	fi;                                                                        \
	if [ ! -d ${APP_ROOT}/assets ] ; then                                      \
		REFRESH=1;                                                             \
	fi;                                                                        \
	if [ $$REFRESH -ne 0 ] ; then                                              \
	echo "assets changed, refreshing...";                                      \
	$(MAKE) clean_assets;                                                      \
	mkdir -p ${APP_ROOT}/assets/Minetest;                                      \
	cp ${PROJ_ROOT}/minetest.conf.example ${APP_ROOT}/assets/Minetest;         \
	cp ${PROJ_ROOT}/README.txt ${APP_ROOT}/assets/Minetest;                    \
	cp -r ${PROJ_ROOT}/builtin ${APP_ROOT}/assets/Minetest;                    \
	mkdir -p ${APP_ROOT}/assets/Minetest/client;                                  \
	cp -r ${PROJ_ROOT}/client/shaders ${APP_ROOT}/assets/Minetest/client;      \
	cp ${PROJ_ROOT}/doc/lgpl-2.1.txt ${APP_ROOT}/assets/Minetest/LICENSE.txt;  \
	mkdir -p ${APP_ROOT}/assets/Minetest/fonts;                                   \
	cp -r ${PROJ_ROOT}/fonts/*.ttf ${APP_ROOT}/assets/Minetest/fonts/;         \
	mkdir -p ${APP_ROOT}/assets/Minetest/games;                                   \
	for game in ${GAMES_TO_COPY}; do                                           \
	    cp -r ${PROJ_ROOT}/games/$$game ${APP_ROOT}/assets/Minetest/games/;    \
	done;                                                                      \
	mkdir -p ${APP_ROOT}/assets/Minetest/mods;                                    \
	for mod in ${MODS_TO_COPY}; do                                             \
	    cp -r ${PROJ_ROOT}/mods/$$mod ${APP_ROOT}/assets/Minetest/mods/;       \
	done;                                                                      \
	cp -r ${PROJ_ROOT}/po ${APP_ROOT}/assets/Minetest;                         \
	cp -r ${PROJ_ROOT}/textures ${APP_ROOT}/assets/Minetest;                   \
	mkdir -p ${APP_ROOT}/assets/Minetest/media;                                \
	cp -r ${IRRLICHT_DIR}/media/Shaders ${APP_ROOT}/assets/Minetest/media;     \
	cd ${APP_ROOT}/assets ||  exit 1;                                          \
	find . -name "timestamp" -exec rm {} \; ;                                  \
	find . -name "*.blend" -exec rm {} \; ;                                    \
	find . -name "*~" -exec rm {} \; ;                                         \
	find . -type d -path "*.git" -exec rm -rf {} \; ;                          \
	find . -type d -path "*.svn" -exec rm -rf {} \; ;                          \
	find . -type f -path "*.gitignore" -exec rm -rf {} \; ;                    \
	ls -R | grep ":$$" | sed -e 's/:$$//' -e 's/\.//' -e 's/^\///' > "index.txt"; \
	find -L Minetest > filelist.txt;                                             \
	cp ${ANDR_ROOT}/${ASSETS_TIMESTAMP} ${ANDR_ROOT}/${ASSETS_TIMESTAMP}.old;  \
	else                                                                       \
		echo "nothing to be done for assets";                                  \
	fi

clean_assets :
	@$(RM) -r assets

apk: local.properties assets $(ICONV_LIB) $(IRRLICHT_LIB) $(CURL_LIB) $(GMP_LIB) $(LEVELDB_TARGET)       \
	$(OPENAL_LIB) $(OGG_LIB) prep_srcdir $(ANDR_ROOT)/jni/src/android_version.h    \
	$(ANDR_ROOT)/jni/src/android_version_githash.h sqlite3_download
	+ @${ANDROID_NDK}/ndk-build NDK_MODULE_PATH=${NDK_MODULE_PATH}             \
		GPROF=${GPROF} APP_ABI=${TARGET_ABI} HAVE_LEVELDB=${HAVE_LEVELDB}      \
		APP_PLATFORM=${APP_PLATFORM}                                           \
		PRIVATE_CC=${NDK_MODULE_PATH}/${TARGET_TOOLCHAIN}${COMPILER_VERSION}/prebuilt/linux-x86_64/bin/${TARGET_TOOLCHAIN}gcc \
		PRIVATE_CXX=${NDK_MODULE_PATH}/${TARGET_TOOLCHAIN}${COMPILER_VERSION}/prebuilt/linux-x86_64/bin/${TARGET_TOOLCHAIN}g++ \
		TARGET_LIBDIR=${TARGET_LIBDIR}                                         \
		TARGET_CFLAGS+="${TARGET_CFLAGS_ADDON}"                                \
		TARGET_LDFLAGS+="${TARGET_LDFLAGS_ADDON}"                              \
		TARGET_CXXFLAGS+="${TARGET_CXXFLAGS_ADDON}" || exit 1;                 \
	if [ ! -e ${APP_ROOT}/jniLibs ]; then                                      \
		ln -s ${ANDR_ROOT}/libs ${APP_ROOT}/jniLibs || exit 1;                 \
	fi;                                                                        \
	export VERSION_STR="${VERSION_MAJOR}.${VERSION_MINOR}.${VERSION_PATCH}" && \
	export BUILD_TYPE_C=$$(echo "$${BUILD_TYPE}" | sed 's/./\U&/') &&          \
	./gradlew assemble$$BUILD_TYPE_C &&                                           \
	echo "APK stored at: build/outputs/apk/Minetest-$$BUILD_TYPE.apk" &&       \
	echo "You can install it with \`make install_$$BUILD_TYPE\`"

# These Intentionally doesn't depend on their respective build steps,
# because it takes a while to verify that everything's up-to-date.
install_debug:
	${ANDROID_SDK}/platform-tools/adb install -r build/outputs/apk/Minetest-debug.apk

install_release:
	${ANDROID_SDK}/platform-tools/adb install -r build/outputs/apk/Minetest-release.apk

prep_srcdir :
	@if [ ! -e ${ANDR_ROOT}/jni/src ]; then                                    \
		ln -s ${PROJ_ROOT}/src ${ANDR_ROOT}/jni/src;                           \
	fi

clean_apk :
	./gradlew clean

clean_all :
	@$(MAKE) clean_apk;                                                        \
	$(MAKE) clean_assets clean_iconv clean_irrlicht clean_leveldb clean_curl   \
	clean_openssl clean_openal clean_ogg clean_gmp;                            \
	sleep 1;                                                                   \
	$(RM) -r gen libs obj deps bin Debug and_env

$(ANDR_ROOT)/jni/src/android_version_githash.h : prep_srcdir
	@export VERSION_FILE=${ANDR_ROOT}/jni/src/android_version_githash.h;        \
	export VERSION_FILE_NEW=$${VERSION_FILE}.new;                              \
	{                                                                          \
	echo "#ifndef ANDROID_MT_VERSION_GITHASH_H";                               \
	echo "#define ANDROID_MT_VERSION_GITHASH_H";                               \
	export GITHASH=$$(git rev-parse --short=8 HEAD);                           \
	export VERSION_STR="${VERSION_MAJOR}.${VERSION_MINOR}.${VERSION_PATCH}";   \
	echo "#define VERSION_GITHASH \"$$VERSION_STR-$$GITHASH-Android\"";        \
	echo "#endif";                                                             \
	} > "$${VERSION_FILE_NEW}";                                                \
	if ! cmp -s $${VERSION_FILE} $${VERSION_FILE_NEW}; then                    \
		echo "android_version_githash.h changed, updating...";             \
		mv "$${VERSION_FILE_NEW}" "$${VERSION_FILE}";                      \
	else                                                                       \
		rm "$${VERSION_FILE_NEW}";                                         \
	fi


$(ANDR_ROOT)/jni/src/android_version.h : prep_srcdir
	@export VERSION_FILE=${ANDR_ROOT}/jni/src/android_version.h;                \
	export VERSION_FILE_NEW=$${VERSION_FILE}.new;                              \
	{                                                                          \
	echo "#ifndef ANDROID_MT_VERSION_H";                                       \
	echo "#define ANDROID_MT_VERSION_H";                                       \
	echo "#define VERSION_MAJOR ${VERSION_MAJOR}";                             \
	echo "#define VERSION_MINOR ${VERSION_MINOR}";                             \
	echo "#define VERSION_PATCH ${VERSION_PATCH}";                             \
	echo "#define VERSION_STRING STR(VERSION_MAJOR) \".\" STR(VERSION_MINOR) \
	\".\" STR(VERSION_PATCH)";                                                 \
	echo "#endif";                                                             \
	} > $${VERSION_FILE_NEW};                                                  \
	if ! cmp -s $${VERSION_FILE} $${VERSION_FILE_NEW}; then                    \
		echo "android_version.h changed, updating...";                     \
		mv "$${VERSION_FILE_NEW}" "$${VERSION_FILE}";                      \
	else                                                                       \
		rm "$${VERSION_FILE_NEW}";                                         \
	fi

clean : clean_apk clean_assets
