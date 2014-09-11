LOCAL_PATH := $(call my-dir)/..

#LOCAL_ADDRESS_SANITIZER:=true

include $(CLEAR_VARS)
LOCAL_MODULE := Irrlicht
LOCAL_SRC_FILES := deps/irrlicht/lib/Android/libIrrlicht.a
include $(PREBUILT_STATIC_LIBRARY)

ifeq ($(HAVE_LEVELDB), 1)
	include $(CLEAR_VARS)
	LOCAL_MODULE := LevelDB
	LOCAL_SRC_FILES := deps/leveldb/libleveldb.a
	include $(PREBUILT_STATIC_LIBRARY)
endif

include $(CLEAR_VARS)
LOCAL_MODULE := curl
LOCAL_SRC_FILES := deps/curl-7.35.0/lib/.libs/libcurl.a
include $(PREBUILT_STATIC_LIBRARY)

include $(CLEAR_VARS)
LOCAL_MODULE := freetype
LOCAL_SRC_FILES := deps/freetype2-android/Android/obj/local/$(TARGET_ARCH_ABI)/libfreetype2-static.a
include $(PREBUILT_STATIC_LIBRARY)

include $(CLEAR_VARS)
LOCAL_MODULE := openal
LOCAL_SRC_FILES := deps/openal-soft/libs/$(TARGET_LIBDIR)/libopenal.so
include $(PREBUILT_SHARED_LIBRARY)

include $(CLEAR_VARS)
LOCAL_MODULE := ogg
LOCAL_SRC_FILES := deps/libvorbis-libogg-android/libs/$(TARGET_LIBDIR)/libogg.so
include $(PREBUILT_SHARED_LIBRARY)

include $(CLEAR_VARS)
LOCAL_MODULE := vorbis
LOCAL_SRC_FILES := deps/libvorbis-libogg-android/libs/$(TARGET_LIBDIR)/libvorbis.so
include $(PREBUILT_SHARED_LIBRARY)

include $(CLEAR_VARS)
LOCAL_MODULE := ssl
LOCAL_SRC_FILES := deps/openssl-android/libs/$(TARGET_LIBDIR)/libssl.so
include $(PREBUILT_SHARED_LIBRARY)

include $(CLEAR_VARS)
LOCAL_MODULE := crypto
LOCAL_SRC_FILES := deps/openssl-android/libs/$(TARGET_LIBDIR)/libcrypto.so
include $(PREBUILT_SHARED_LIBRARY)


include $(CLEAR_VARS)
LOCAL_MODULE := minetest

LOCAL_CPP_FEATURES += exceptions

ifdef GPROF
GPROF_DEF=-DGPROF
endif

LOCAL_CFLAGS := -D_IRR_ANDROID_PLATFORM_      \
				-DHAVE_TOUCHSCREENGUI         \
				-DUSE_CURL=1                  \
				-DUSE_SOUND=1                 \
				-DUSE_FREETYPE=1              \
				-DUSE_LEVELDB=$(HAVE_LEVELDB) \
				$(GPROF_DEF)                  \
				-pipe -fstrict-aliasing

ifndef NDEBUG
LOCAL_CFLAGS += -g -D_DEBUG -O0 -fno-omit-frame-pointer
else
LOCAL_CFLAGS += -fexpensive-optimizations -O3
endif

ifdef GPROF
PROFILER_LIBS := android-ndk-profiler
LOCAL_CFLAGS += -pg
endif

# LOCAL_CFLAGS += -fsanitize=address
# LOCAL_LDFLAGS += -fsanitize=address

ifeq ($(TARGET_ARCH_ABI),x86)
LOCAL_CFLAGS += -fno-stack-protector
endif

LOCAL_C_INCLUDES :=                               \
		jni/src jni/src/sqlite                    \
		jni/src/script                            \
		jni/src/lua/src                           \
		jni/src/json                              \
		jni/src/cguittfont                        \
		deps/irrlicht/include                     \
		deps/freetype2-android/include            \
		deps/curl-7.35.0/include                  \
		deps/openal-soft/jni/OpenAL/include       \
		deps/libvorbis-libogg-android/jni/include \
		deps/leveldb/include                      \

LOCAL_SRC_FILES :=                                \
		jni/src/ban.cpp                           \
		jni/src/base64.cpp                        \
		jni/src/biome.cpp                         \
		jni/src/camera.cpp                        \
		jni/src/cavegen.cpp                       \
		jni/src/chat.cpp                          \
		jni/src/client.cpp                        \
		jni/src/clientiface.cpp                   \
		jni/src/clientmap.cpp                     \
		jni/src/clientmedia.cpp                   \
		jni/src/clientobject.cpp                  \
		jni/src/clouds.cpp                        \
		jni/src/collision.cpp                     \
		jni/src/connection.cpp                    \
		jni/src/content_abm.cpp                   \
		jni/src/content_cao.cpp                   \
		jni/src/content_cso.cpp                   \
		jni/src/content_mapblock.cpp              \
		jni/src/content_mapnode.cpp               \
		jni/src/content_nodemeta.cpp              \
		jni/src/content_sao.cpp                   \
		jni/src/convert_json.cpp                  \
		jni/src/craftdef.cpp                      \
		jni/src/database-dummy.cpp                \
		jni/src/database-sqlite3.cpp              \
		jni/src/database.cpp                      \
		jni/src/debug.cpp                         \
		jni/src/defaultsettings.cpp               \
		jni/src/drawscene.cpp                     \
		jni/src/dungeongen.cpp                    \
		jni/src/emerge.cpp                        \
		jni/src/environment.cpp                   \
		jni/src/filecache.cpp                     \
		jni/src/filesys.cpp                       \
		jni/src/game.cpp                          \
		jni/src/genericobject.cpp                 \
		jni/src/gettext.cpp                       \
		jni/src/guiChatConsole.cpp                \
		jni/src/guiEngine.cpp                     \
		jni/src/guiFileSelectMenu.cpp             \
		jni/src/guiFormSpecMenu.cpp               \
		jni/src/guiKeyChangeMenu.cpp              \
		jni/src/guiPasswordChange.cpp             \
		jni/src/guiTable.cpp                      \
		jni/src/guiVolumeChange.cpp               \
		jni/src/httpfetch.cpp                     \
		jni/src/hud.cpp                           \
		jni/src/inventory.cpp                     \
		jni/src/inventorymanager.cpp              \
		jni/src/itemdef.cpp                       \
		jni/src/keycode.cpp                       \
		jni/src/light.cpp                         \
		jni/src/localplayer.cpp                   \
		jni/src/log.cpp                           \
		jni/src/main.cpp                          \
		jni/src/map.cpp                           \
		jni/src/mapblock.cpp                      \
		jni/src/mapblock_mesh.cpp                 \
		jni/src/mapgen.cpp                        \
		jni/src/mapgen_singlenode.cpp             \
		jni/src/mapgen_v6.cpp                     \
		jni/src/mapgen_v7.cpp                     \
		jni/src/mapnode.cpp                       \
		jni/src/mapsector.cpp                     \
		jni/src/mesh.cpp                          \
		jni/src/mods.cpp                          \
		jni/src/nameidmapping.cpp                 \
		jni/src/nodedef.cpp                       \
		jni/src/nodemetadata.cpp                  \
		jni/src/nodetimer.cpp                     \
		jni/src/noise.cpp                         \
		jni/src/object_properties.cpp             \
		jni/src/particles.cpp                     \
		jni/src/pathfinder.cpp                    \
		jni/src/player.cpp                        \
		jni/src/porting_android.cpp               \
		jni/src/porting.cpp                       \
		jni/src/quicktune.cpp                     \
		jni/src/rollback.cpp                      \
		jni/src/rollback_interface.cpp            \
		jni/src/serialization.cpp                 \
		jni/src/server.cpp                        \
		jni/src/serverlist.cpp                    \
		jni/src/serverobject.cpp                  \
		jni/src/sha1.cpp                          \
		jni/src/shader.cpp                        \
		jni/src/sky.cpp                           \
		jni/src/socket.cpp                        \
		jni/src/sound.cpp                         \
		jni/src/sound_openal.cpp                  \
		jni/src/staticobject.cpp                  \
		jni/src/subgame.cpp                       \
		jni/src/test.cpp                          \
		jni/src/tile.cpp                          \
		jni/src/tool.cpp                          \
		jni/src/treegen.cpp                       \
		jni/src/version.cpp                       \
		jni/src/voxel.cpp                         \
		jni/src/voxelalgorithms.cpp               \
		jni/src/util/directiontables.cpp          \
		jni/src/util/numeric.cpp                  \
		jni/src/util/pointedthing.cpp             \
		jni/src/util/serialize.cpp                \
		jni/src/util/string.cpp                   \
		jni/src/util/timetaker.cpp                \
		jni/src/touchscreengui.cpp                \
 		jni/src/database-leveldb.cpp              \
 		jni/src/settings.cpp

# lua api
LOCAL_SRC_FILES +=                                \
		jni/src/script/common/c_content.cpp       \
		jni/src/script/common/c_converter.cpp     \
		jni/src/script/common/c_internal.cpp      \
		jni/src/script/common/c_types.cpp         \
		jni/src/script/cpp_api/s_base.cpp         \
		jni/src/script/cpp_api/s_entity.cpp       \
		jni/src/script/cpp_api/s_env.cpp          \
		jni/src/script/cpp_api/s_inventory.cpp    \
		jni/src/script/cpp_api/s_item.cpp         \
		jni/src/script/cpp_api/s_mainmenu.cpp     \
		jni/src/script/cpp_api/s_node.cpp         \
		jni/src/script/cpp_api/s_nodemeta.cpp     \
		jni/src/script/cpp_api/s_player.cpp       \
		jni/src/script/cpp_api/s_server.cpp       \
		jni/src/script/cpp_api/s_async.cpp        \
		jni/src/script/lua_api/l_base.cpp         \
		jni/src/script/lua_api/l_craft.cpp        \
		jni/src/script/lua_api/l_env.cpp          \
		jni/src/script/lua_api/l_inventory.cpp    \
		jni/src/script/lua_api/l_item.cpp         \
		jni/src/script/lua_api/l_mainmenu.cpp     \
		jni/src/script/lua_api/l_mapgen.cpp       \
		jni/src/script/lua_api/l_nodemeta.cpp     \
		jni/src/script/lua_api/l_nodetimer.cpp    \
		jni/src/script/lua_api/l_noise.cpp        \
		jni/src/script/lua_api/l_object.cpp       \
		jni/src/script/lua_api/l_particles.cpp    \
		jni/src/script/lua_api/l_rollback.cpp     \
		jni/src/script/lua_api/l_server.cpp       \
		jni/src/script/lua_api/l_settings.cpp     \
		jni/src/script/lua_api/l_util.cpp         \
		jni/src/script/lua_api/l_vmanip.cpp       \
		jni/src/script/scripting_game.cpp         \
		jni/src/script/scripting_mainmenu.cpp
		
#freetype2 support
LOCAL_SRC_FILES +=                                \
		jni/src/cguittfont/xCGUITTFont.cpp

# lua
LOCAL_SRC_FILES +=                                \
		jni/src/lua/src/lapi.c                    \
		jni/src/lua/src/lauxlib.c                 \
		jni/src/lua/src/lbaselib.c                \
		jni/src/lua/src/lcode.c                   \
		jni/src/lua/src/ldblib.c                  \
		jni/src/lua/src/ldebug.c                  \
		jni/src/lua/src/ldo.c                     \
		jni/src/lua/src/ldump.c                   \
		jni/src/lua/src/lfunc.c                   \
		jni/src/lua/src/lgc.c                     \
		jni/src/lua/src/linit.c                   \
		jni/src/lua/src/liolib.c                  \
		jni/src/lua/src/llex.c                    \
		jni/src/lua/src/lmathlib.c                \
		jni/src/lua/src/lmem.c                    \
		jni/src/lua/src/loadlib.c                 \
		jni/src/lua/src/lobject.c                 \
		jni/src/lua/src/lopcodes.c                \
		jni/src/lua/src/loslib.c                  \
		jni/src/lua/src/lparser.c                 \
		jni/src/lua/src/lstate.c                  \
		jni/src/lua/src/lstring.c                 \
		jni/src/lua/src/lstrlib.c                 \
		jni/src/lua/src/ltable.c                  \
		jni/src/lua/src/ltablib.c                 \
		jni/src/lua/src/ltm.c                     \
		jni/src/lua/src/lundump.c                 \
		jni/src/lua/src/lvm.c                     \
		jni/src/lua/src/lzio.c                    \
		jni/src/lua/src/print.c

# sqlite
LOCAL_SRC_FILES += jni/src/sqlite/sqlite3.c

# jthread
LOCAL_SRC_FILES +=                                \
		jni/src/jthread/pthread/jevent.cpp        \
		jni/src/jthread/pthread/jmutex.cpp        \
		jni/src/jthread/pthread/jsemaphore.cpp    \
		jni/src/jthread/pthread/jthread.cpp

# json
LOCAL_SRC_FILES += jni/src/json/jsoncpp.cpp

LOCAL_SHARED_LIBRARIES := openal ogg vorbis ssl crypto
LOCAL_STATIC_LIBRARIES := Irrlicht freetype curl android_native_app_glue $(PROFILER_LIBS)

ifeq ($(HAVE_LEVELDB), 1)
	LOCAL_STATIC_LIBRARIES += LevelDB
endif
LOCAL_LDLIBS := -lEGL -llog -lGLESv1_CM -lGLESv2 -lz -landroid

include $(BUILD_SHARED_LIBRARY)

# at the end of Android.mk
ifdef GPROF
$(call import-module,android-ndk-profiler)
endif
$(call import-module,android/native_app_glue)
