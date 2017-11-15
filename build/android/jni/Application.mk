# NDK_TOOLCHAIN_VERSION := clang3.8

APP_PLATFORM := android-14
APP_MODULES := minetest
APP_STL := c++_static

APP_CPPFLAGS += -fexceptions -std=c++11
APP_GNUSTL_FORCE_CPP_FEATURES := rtti

