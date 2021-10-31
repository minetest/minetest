APP_PLATFORM := ${APP_PLATFORM}
APP_ABI := ${TARGET_ABI}
APP_STL := c++_shared
NDK_TOOLCHAIN_VERSION := clang
APP_SHORT_COMMANDS := true
APP_MODULES := Minetest

APP_CPPFLAGS := -O2 -fvisibility=hidden

ifeq ($(APP_ABI),armeabi-v7a)
APP_CPPFLAGS += -mfloat-abi=softfp -mfpu=vfpv3-d16 -mthumb
endif

ifeq ($(APP_ABI),x86)
APP_CPPFLAGS += -mssse3 -mfpmath=sse -funroll-loops
endif

ifndef NDEBUG
APP_CPPFLAGS := -g -Og -fno-omit-frame-pointer
endif

APP_CFLAGS   := $(APP_CPPFLAGS) -Wno-inconsistent-missing-override -Wno-parentheses-equality
APP_CXXFLAGS := $(APP_CPPFLAGS) -fexceptions -frtti -std=gnu++17
APP_LDFLAGS  := -Wl,--no-warn-mismatch,--gc-sections,--icf=safe

ifeq ($(APP_ABI),arm64-v8a)
APP_LDFLAGS  := -Wl,--no-warn-mismatch,--gc-sections
endif

ifndef NDEBUG
APP_LDFLAGS  :=
endif
