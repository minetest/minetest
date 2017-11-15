APP_PLATFORM := ${APP_PLATFORM}
APP_ABI := ${TARGET_ABI}
APP_STL := gnustl_static
NDK_TOOLCHAIN_VERSION := 4.9
APP_DEPRECATED_HEADERS := true
APP_MODULES := Irrlicht

APP_CLAFGS += -mfloat-abi=softfp -mfpu=vfpv3 -O3
APP_CPPFLAGS += -fexceptions
