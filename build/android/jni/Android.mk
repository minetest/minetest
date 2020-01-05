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
LOCAL_SRC_FILES := deps/curl/lib/.libs/libcurl.a
include $(PREBUILT_STATIC_LIBRARY)

include $(CLEAR_VARS)
LOCAL_MODULE := freetype
LOCAL_SRC_FILES := deps/freetype2-android/Android/obj/local/$(TARGET_ARCH_ABI)/libfreetype2-static.a
include $(PREBUILT_STATIC_LIBRARY)

include $(CLEAR_VARS)
LOCAL_MODULE := iconv
LOCAL_SRC_FILES := deps/libiconv/lib/.libs/libiconv.so
include $(PREBUILT_SHARED_LIBRARY)

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
LOCAL_SRC_FILES := deps/openssl/libssl.a
include $(PREBUILT_STATIC_LIBRARY)

include $(CLEAR_VARS)
LOCAL_MODULE := crypto
LOCAL_SRC_FILES := deps/openssl/libcrypto.a
include $(PREBUILT_STATIC_LIBRARY)

include $(CLEAR_VARS)
LOCAL_MODULE := minetest

LOCAL_CPP_FEATURES += exceptions

ifdef GPROF
GPROF_DEF=-DGPROF
endif

LOCAL_CFLAGS := -D_IRR_ANDROID_PLATFORM_      \
		-DHAVE_TOUCHSCREENGUI         \
		-DENABLE_GLES=1               \
		-DUSE_CURL=1                  \
		-DUSE_SOUND=1                 \
		-DUSE_FREETYPE=1              \
		-DUSE_LEVELDB=$(HAVE_LEVELDB) \
		$(GPROF_DEF)                  \
		-pipe -fstrict-aliasing

ifndef NDEBUG
LOCAL_CFLAGS += -g -D_DEBUG -O0 -fno-omit-frame-pointer
else
LOCAL_CFLAGS += $(TARGET_CFLAGS_ADDON)
endif

ifdef GPROF
PROFILER_LIBS := android-ndk-profiler
LOCAL_CFLAGS += -pg
endif

# LOCAL_CFLAGS += -fsanitize=address
# LOCAL_LDFLAGS += -fsanitize=address

ifeq ($(TARGET_ABI),x86)
LOCAL_CFLAGS += -fno-stack-protector
endif

LOCAL_C_INCLUDES := \
		jni/src                                   \
		jni/src/script                            \
		jni/lib/gmp                               \
		jni/lib/lua/src                           \
		jni/lib/jsoncpp                           \
		jni/src/cguittfont                        \
		deps/irrlicht/include                     \
		deps/libiconv/include                     \
		deps/freetype2-android/include            \
		deps/curl/include                         \
		deps/openal-soft/jni/OpenAL/include       \
		deps/libvorbis-libogg-android/jni/include \
		deps/leveldb/include                      \
		deps/sqlite/

LOCAL_SRC_FILES := \
		jni/src/ban.cpp                           \
		jni/src/chat.cpp                          \
		jni/src/client/activeobjectmgr.cpp        \
		jni/src/client/camera.cpp                 \
		jni/src/client/client.cpp                 \
		jni/src/client/clientenvironment.cpp      \
		jni/src/client/clientlauncher.cpp         \
		jni/src/client/clientmap.cpp              \
		jni/src/client/clientmedia.cpp            \
		jni/src/client/clientobject.cpp           \
		jni/src/client/clouds.cpp                 \
		jni/src/client/content_cao.cpp            \
		jni/src/client/content_cso.cpp            \
		jni/src/client/content_mapblock.cpp       \
		jni/src/client/filecache.cpp              \
		jni/src/client/fontengine.cpp             \
		jni/src/client/game.cpp                   \
		jni/src/client/gameui.cpp                 \
		jni/src/client/guiscalingfilter.cpp       \
		jni/src/client/hud.cpp                    \
		jni/src/clientiface.cpp                   \
		jni/src/client/imagefilters.cpp           \
		jni/src/client/inputhandler.cpp           \
		jni/src/client/joystick_controller.cpp    \
		jni/src/client/keycode.cpp                \
		jni/src/client/localplayer.cpp            \
		jni/src/client/mapblock_mesh.cpp          \
		jni/src/client/mesh.cpp                   \
		jni/src/client/meshgen/collector.cpp      \
		jni/src/client/mesh_generator_thread.cpp  \
		jni/src/client/minimap.cpp                \
		jni/src/client/particles.cpp              \
		jni/src/client/render/anaglyph.cpp        \
		jni/src/client/render/core.cpp            \
		jni/src/client/render/factory.cpp         \
		jni/src/client/renderingengine.cpp        \
		jni/src/client/render/interlaced.cpp      \
		jni/src/client/render/pageflip.cpp        \
		jni/src/client/render/plain.cpp           \
		jni/src/client/render/sidebyside.cpp      \
		jni/src/client/render/stereo.cpp          \
		jni/src/client/shader.cpp                 \
		jni/src/client/sky.cpp                    \
		jni/src/client/sound.cpp                  \
		jni/src/client/sound_openal.cpp           \
		jni/src/client/tile.cpp                   \
		jni/src/client/wieldmesh.cpp              \
		jni/src/collision.cpp                     \
		jni/src/content/content.cpp               \
		jni/src/content_mapnode.cpp               \
		jni/src/content/mods.cpp                  \
		jni/src/content_nodemeta.cpp              \
		jni/src/content/packages.cpp              \
		jni/src/content_sao.cpp                   \
		jni/src/content/subgames.cpp              \
		jni/src/convert_json.cpp                  \
		jni/src/craftdef.cpp                      \
		jni/src/database/database.cpp             \
		jni/src/database/database-dummy.cpp       \
		jni/src/database/database-files.cpp       \
		jni/src/database/database-leveldb.cpp     \
		jni/src/database/database-sqlite3.cpp     \
		jni/src/debug.cpp                         \
		jni/src/defaultsettings.cpp               \
		jni/src/emerge.cpp                        \
		jni/src/environment.cpp                   \
		jni/src/face_position_cache.cpp           \
		jni/src/filesys.cpp                       \
		jni/src/genericobject.cpp                 \
		jni/src/gettext.cpp                       \
		jni/src/gui/guiBackgroundImage.cpp        \
		jni/src/gui/guiBox.cpp                    \
		jni/src/gui/guiButton.cpp                 \
		jni/src/gui/guiButtonImage.cpp            \
		jni/src/gui/guiButtonItemImage.cpp        \
		jni/src/gui/guiChatConsole.cpp            \
		jni/src/gui/guiConfirmRegistration.cpp    \
		jni/src/gui/guiEditBoxWithScrollbar.cpp   \
		jni/src/gui/guiEngine.cpp                 \
		jni/src/gui/guiFormSpecMenu.cpp           \
		jni/src/gui/guiHyperText.cpp              \
		jni/src/gui/guiInventoryList.cpp          \
		jni/src/gui/guiItemImage.cpp              \
		jni/src/gui/guiKeyChangeMenu.cpp          \
		jni/src/gui/guiPasswordChange.cpp         \
		jni/src/gui/guiPathSelectMenu.cpp         \
		jni/src/gui/guiScrollBar.cpp              \
		jni/src/gui/guiSkin.cpp                   \
		jni/src/gui/guiTable.cpp                  \
		jni/src/gui/guiVolumeChange.cpp           \
		jni/src/gui/intlGUIEditBox.cpp            \
		jni/src/gui/modalMenu.cpp                 \
		jni/src/gui/profilergraph.cpp             \
		jni/src/gui/touchscreengui.cpp            \
		jni/src/httpfetch.cpp                     \
		jni/src/hud.cpp                           \
		jni/src/inventory.cpp                     \
		jni/src/inventorymanager.cpp              \
		jni/src/irrlicht_changes/CGUITTFont.cpp   \
		jni/src/irrlicht_changes/static_text.cpp  \
		jni/src/itemdef.cpp                       \
		jni/src/itemstackmetadata.cpp             \
		jni/src/light.cpp                         \
		jni/src/log.cpp                           \
		jni/src/main.cpp                          \
		jni/src/mapblock.cpp                      \
		jni/src/map.cpp                           \
		jni/src/mapgen/cavegen.cpp                \
		jni/src/mapgen/dungeongen.cpp             \
		jni/src/mapgen/mapgen_carpathian.cpp      \
		jni/src/mapgen/mapgen.cpp                 \
		jni/src/mapgen/mapgen_flat.cpp            \
		jni/src/mapgen/mapgen_fractal.cpp         \
		jni/src/mapgen/mapgen_singlenode.cpp      \
		jni/src/mapgen/mapgen_v5.cpp              \
		jni/src/mapgen/mapgen_v6.cpp              \
		jni/src/mapgen/mapgen_v7.cpp              \
		jni/src/mapgen/mapgen_valleys.cpp         \
		jni/src/mapgen/mg_biome.cpp               \
		jni/src/mapgen/mg_decoration.cpp          \
		jni/src/mapgen/mg_ore.cpp                 \
		jni/src/mapgen/mg_schematic.cpp           \
		jni/src/mapgen/treegen.cpp                \
		jni/src/mapnode.cpp                       \
		jni/src/mapsector.cpp                     \
		jni/src/map_settings_manager.cpp          \
		jni/src/metadata.cpp                      \
		jni/src/modchannels.cpp                   \
		jni/src/nameidmapping.cpp                 \
		jni/src/nodedef.cpp                       \
		jni/src/nodemetadata.cpp                  \
		jni/src/nodetimer.cpp                     \
		jni/src/noise.cpp                         \
		jni/src/objdef.cpp                        \
		jni/src/object_properties.cpp             \
		jni/src/pathfinder.cpp                    \
		jni/src/player.cpp                        \
		jni/src/porting_android.cpp               \
		jni/src/porting.cpp                       \
		jni/src/profiler.cpp                      \
		jni/src/raycast.cpp                       \
		jni/src/reflowscan.cpp                    \
		jni/src/remoteplayer.cpp                  \
		jni/src/rollback.cpp                      \
		jni/src/rollback_interface.cpp            \
		jni/src/serialization.cpp                 \
		jni/src/server/activeobjectmgr.cpp        \
		jni/src/server.cpp                        \
		jni/src/serverenvironment.cpp             \
		jni/src/serverlist.cpp                    \
		jni/src/server/mods.cpp                   \
		jni/src/serverobject.cpp                  \
		jni/src/settings.cpp                      \
		jni/src/staticobject.cpp                  \
		jni/src/tileanimation.cpp                 \
		jni/src/tool.cpp                          \
		jni/src/translation.cpp                   \
		jni/src/unittest/test_authdatabase.cpp    \
		jni/src/unittest/test_collision.cpp       \
		jni/src/unittest/test_compression.cpp     \
		jni/src/unittest/test_connection.cpp      \
		jni/src/unittest/test.cpp                 \
		jni/src/unittest/test_filepath.cpp        \
		jni/src/unittest/test_gameui.cpp          \
		jni/src/unittest/test_inventory.cpp       \
		jni/src/unittest/test_mapnode.cpp         \
		jni/src/unittest/test_map_settings_manager.cpp \
		jni/src/unittest/test_nodedef.cpp         \
		jni/src/unittest/test_noderesolver.cpp    \
		jni/src/unittest/test_noise.cpp           \
		jni/src/unittest/test_objdef.cpp          \
		jni/src/unittest/test_profiler.cpp        \
		jni/src/unittest/test_random.cpp          \
		jni/src/unittest/test_schematic.cpp       \
		jni/src/unittest/test_serialization.cpp   \
		jni/src/unittest/test_settings.cpp        \
		jni/src/unittest/test_socket.cpp          \
		jni/src/unittest/test_utilities.cpp       \
		jni/src/unittest/test_voxelalgorithms.cpp \
		jni/src/unittest/test_voxelmanipulator.cpp \
		jni/src/util/areastore.cpp                \
		jni/src/util/auth.cpp                     \
		jni/src/util/base64.cpp                   \
		jni/src/util/directiontables.cpp          \
		jni/src/util/enriched_string.cpp          \
		jni/src/util/ieee_float.cpp               \
		jni/src/util/numeric.cpp                  \
		jni/src/util/pointedthing.cpp             \
		jni/src/util/quicktune.cpp                \
		jni/src/util/serialize.cpp                \
		jni/src/util/sha1.cpp                     \
		jni/src/util/srp.cpp                      \
		jni/src/util/string.cpp                   \
		jni/src/util/timetaker.cpp                \
		jni/src/version.cpp                       \
		jni/src/voxelalgorithms.cpp               \
		jni/src/voxel.cpp


# intentionally kept out (we already build openssl itself): jni/src/util/sha256.c

# Network
LOCAL_SRC_FILES += \
		jni/src/network/address.cpp               \
		jni/src/network/connection.cpp            \
		jni/src/network/networkpacket.cpp         \
		jni/src/network/clientopcodes.cpp         \
		jni/src/network/clientpackethandler.cpp   \
		jni/src/network/connectionthreads.cpp     \
		jni/src/network/serveropcodes.cpp         \
		jni/src/network/serverpackethandler.cpp   \
		jni/src/network/socket.cpp                \

# lua api
LOCAL_SRC_FILES += \
		jni/src/script/common/c_content.cpp       \
		jni/src/script/common/c_converter.cpp     \
		jni/src/script/common/c_internal.cpp      \
		jni/src/script/common/c_types.cpp         \
		jni/src/script/common/helper.cpp          \
		jni/src/script/cpp_api/s_async.cpp        \
		jni/src/script/cpp_api/s_base.cpp         \
		jni/src/script/cpp_api/s_client.cpp       \
		jni/src/script/cpp_api/s_entity.cpp       \
		jni/src/script/cpp_api/s_env.cpp          \
		jni/src/script/cpp_api/s_inventory.cpp    \
		jni/src/script/cpp_api/s_item.cpp         \
		jni/src/script/cpp_api/s_mainmenu.cpp     \
		jni/src/script/cpp_api/s_modchannels.cpp  \
		jni/src/script/cpp_api/s_node.cpp         \
		jni/src/script/cpp_api/s_nodemeta.cpp     \
		jni/src/script/cpp_api/s_player.cpp       \
		jni/src/script/cpp_api/s_security.cpp     \
		jni/src/script/cpp_api/s_server.cpp       \
		jni/src/script/lua_api/l_areastore.cpp    \
		jni/src/script/lua_api/l_auth.cpp         \
		jni/src/script/lua_api/l_base.cpp         \
		jni/src/script/lua_api/l_camera.cpp       \
		jni/src/script/lua_api/l_client.cpp       \
		jni/src/script/lua_api/l_craft.cpp        \
		jni/src/script/lua_api/l_env.cpp          \
		jni/src/script/lua_api/l_inventory.cpp    \
		jni/src/script/lua_api/l_item.cpp         \
		jni/src/script/lua_api/l_itemstackmeta.cpp\
		jni/src/script/lua_api/l_localplayer.cpp  \
		jni/src/script/lua_api/l_mainmenu.cpp     \
		jni/src/script/lua_api/l_mapgen.cpp       \
		jni/src/script/lua_api/l_metadata.cpp     \
		jni/src/script/lua_api/l_minimap.cpp      \
		jni/src/script/lua_api/l_modchannels.cpp  \
		jni/src/script/lua_api/l_nodemeta.cpp     \
		jni/src/script/lua_api/l_nodetimer.cpp    \
		jni/src/script/lua_api/l_noise.cpp        \
		jni/src/script/lua_api/l_object.cpp       \
		jni/src/script/lua_api/l_playermeta.cpp   \
		jni/src/script/lua_api/l_particles.cpp    \
		jni/src/script/lua_api/l_particles_local.cpp\
		jni/src/script/lua_api/l_rollback.cpp     \
		jni/src/script/lua_api/l_server.cpp       \
		jni/src/script/lua_api/l_settings.cpp     \
		jni/src/script/lua_api/l_sound.cpp        \
		jni/src/script/lua_api/l_http.cpp         \
		jni/src/script/lua_api/l_storage.cpp      \
		jni/src/script/lua_api/l_util.cpp         \
		jni/src/script/lua_api/l_vmanip.cpp       \
		jni/src/script/scripting_client.cpp       \
		jni/src/script/scripting_server.cpp       \
		jni/src/script/scripting_mainmenu.cpp

#freetype2 support
#LOCAL_SRC_FILES += jni/src/cguittfont/xCGUITTFont.cpp

# GMP
LOCAL_SRC_FILES += jni/lib/gmp/mini-gmp.c

# Lua
LOCAL_SRC_FILES += \
		jni/lib/lua/src/lapi.c                    \
		jni/lib/lua/src/lauxlib.c                 \
		jni/lib/lua/src/lbaselib.c                \
		jni/lib/lua/src/lcode.c                   \
		jni/lib/lua/src/ldblib.c                  \
		jni/lib/lua/src/ldebug.c                  \
		jni/lib/lua/src/ldo.c                     \
		jni/lib/lua/src/ldump.c                   \
		jni/lib/lua/src/lfunc.c                   \
		jni/lib/lua/src/lgc.c                     \
		jni/lib/lua/src/linit.c                   \
		jni/lib/lua/src/liolib.c                  \
		jni/lib/lua/src/llex.c                    \
		jni/lib/lua/src/lmathlib.c                \
		jni/lib/lua/src/lmem.c                    \
		jni/lib/lua/src/loadlib.c                 \
		jni/lib/lua/src/lobject.c                 \
		jni/lib/lua/src/lopcodes.c                \
		jni/lib/lua/src/loslib.c                  \
		jni/lib/lua/src/lparser.c                 \
		jni/lib/lua/src/lstate.c                  \
		jni/lib/lua/src/lstring.c                 \
		jni/lib/lua/src/lstrlib.c                 \
		jni/lib/lua/src/ltable.c                  \
		jni/lib/lua/src/ltablib.c                 \
		jni/lib/lua/src/ltm.c                     \
		jni/lib/lua/src/lundump.c                 \
		jni/lib/lua/src/lvm.c                     \
		jni/lib/lua/src/lzio.c                    \
		jni/lib/lua/src/print.c

# SQLite3
LOCAL_SRC_FILES += deps/sqlite/sqlite3.c

# Threading
LOCAL_SRC_FILES += \
		jni/src/threading/event.cpp \
		jni/src/threading/semaphore.cpp \
		jni/src/threading/thread.cpp

# JSONCPP
LOCAL_SRC_FILES += jni/lib/jsoncpp/jsoncpp.cpp

LOCAL_SHARED_LIBRARIES := iconv openal ogg vorbis
LOCAL_STATIC_LIBRARIES := Irrlicht freetype curl ssl crypto android_native_app_glue $(PROFILER_LIBS)

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
