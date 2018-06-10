APP_PLATFORM := ${APP_PLATFORM}
APP_ABI := ${TARGET_ABI}
APP_STL := c++_static
APP_DEPRECATED_HEADERS := true
APP_MODULES := Irrlicht

APP_CFLAGS += -mfloat-abi=softfp -mfpu=vfpv3 -O3
APP_CPPFLAGS += -fexceptions
