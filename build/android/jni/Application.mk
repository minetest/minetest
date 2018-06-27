APP_PLATFORM := ${APP_PLATFORM}
APP_ABI := ${TARGET_ABI}
APP_STL := c++_shared
APP_MODULES := minetest
ifndef NDEBUG
APP_OPTIM := debug
endif

APP_CPPFLAGS += -fexceptions -std=c++11 -frtti
