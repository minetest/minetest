Minetest
========

![Build Status](https://github.com/minetest/minetest/workflows/build/badge.svg)
[![Translation status](https://hosted.weblate.org/widgets/minetest/-/svg-badge.svg)](https://hosted.weblate.org/engage/minetest/?utm_source=widget)
[![License](https://img.shields.io/badge/license-LGPLv2.1%2B-blue.svg)](https://www.gnu.org/licenses/old-licenses/lgpl-2.1.en.html)

Minetest is a free open-source voxel game engine with easy modding and game creation.

Copyright (C) 2010-2020 Perttu Ahola <celeron55@gmail.com>
and contributors (see source file comments and the version control log)

In case you downloaded the source code
--------------------------------------
If you downloaded the Minetest Engine source code in which this file is
contained, you probably want to download the [Minetest Game](https://github.com/minetest/minetest_game/)
project too. See its README.txt for more information.

Table of Contents
------------------

1. [Further Documentation](#further-documentation)
2. [Default Controls](#default-controls)
3. [Paths](#paths)
4. [Configuration File](#configuration-file)
5. [Command-line Options](#command-line-options)
6. [Compiling](#compiling)
7. [Docker](#docker)
8. [Version Scheme](#version-scheme)


Further documentation
----------------------
- Website: https://minetest.net/
- Wiki: https://wiki.minetest.net/
- Developer wiki: https://dev.minetest.net/
- Forum: https://forum.minetest.net/
- GitHub: https://github.com/minetest/minetest/
- [doc/](doc/) directory of source distribution

Default controls
----------------
All controls are re-bindable using settings.
Some can be changed in the key config dialog in the settings tab.

| Button                        | Action                                                         |
|-------------------------------|----------------------------------------------------------------|
| Move mouse                    | Look around                                                    |
| W, A, S, D                    | Move                                                           |
| Space                         | Jump/move up                                                   |
| Shift                         | Sneak/move down                                                |
| Q                             | Drop itemstack                                                 |
| Shift + Q                     | Drop single item                                               |
| Left mouse button             | Dig/punch/take item                                            |
| Right mouse button            | Place/use                                                      |
| Shift + right mouse button    | Build (without using)                                          |
| I                             | Inventory menu                                                 |
| Mouse wheel                   | Select item                                                    |
| 0-9                           | Select item                                                    |
| Z                             | Zoom (needs zoom privilege)                                    |
| T                             | Chat                                                           |
| /                             | Command                                                        |
| Esc                           | Pause menu/abort/exit (pauses only singleplayer game)          |
| R                             | Enable/disable full range view                                 |
| +                             | Increase view range                                            |
| -                             | Decrease view range                                            |
| K                             | Enable/disable fly mode (needs fly privilege)                  |
| P                             | Enable/disable pitch move mode                                 |
| J                             | Enable/disable fast mode (needs fast privilege)                |
| H                             | Enable/disable noclip mode (needs noclip privilege)            |
| E                             | Aux1 (Move fast in fast mode. Games may add special features)  |
| C                             | Cycle through camera modes                                     |
| V                             | Cycle through minimap modes                                    |
| Shift + V                     | Change minimap orientation                                     |
| F1                            | Hide/show HUD                                                  |
| F2                            | Hide/show chat                                                 |
| F3                            | Disable/enable fog                                             |
| F4                            | Disable/enable camera update (Mapblocks are not updated anymore when disabled, disabled in release builds)  |
| F5                            | Cycle through debug information screens                        |
| F6                            | Cycle through profiler info screens                            |
| F10                           | Show/hide console                                              |
| F12                           | Take screenshot                                                |

Paths
-----
Locations:

* `bin`   - Compiled binaries
* `share` - Distributed read-only data
* `user`  - User-created modifiable data

Where each location is on each platform:

* Windows .zip / RUN_IN_PLACE source:
    * `bin`   = `bin`
    * `share` = `.`
    * `user`  = `.`
* Windows installed:
    * `bin`   = `C:\Program Files\Minetest\bin (Depends on the install location)`
    * `share` = `C:\Program Files\Minetest (Depends on the install location)`
    * `user`  = `%APPDATA%\Minetest`
* Linux installed:
    * `bin`   = `/usr/bin`
    * `share` = `/usr/share/minetest`
    * `user`  = `~/.minetest`
* macOS:
    * `bin`   = `Contents/MacOS`
    * `share` = `Contents/Resources`
    * `user`  = `Contents/User OR ~/Library/Application Support/minetest`

Worlds can be found as separate folders in: `user/worlds/`

Configuration file
------------------
- Default location:
    `user/minetest.conf`
- This file is created by closing Minetest for the first time.
- A specific file can be specified on the command line:
    `--config <path-to-file>`
- A run-in-place build will look for the configuration file in
    `location_of_exe/../minetest.conf` and also `location_of_exe/../../minetest.conf`

Command-line options
--------------------
- Use `--help`

Compiling
---------
### Compiling on GNU/Linux

#### Dependencies

| Dependency | Version | Commentary |
|------------|---------|------------|
| GCC        | 5.1+    | or Clang 3.5+ |
| CMake      | 3.5+    |            |
| IrrlichtMt | -       | Custom version of Irrlicht, see https://github.com/minetest/irrlicht |
| Freetype   | 2.0+    |            |
| SQLite3    | 3+      |            |
| Zstd       | 1.0+    |            |
| LuaJIT     | 2.0+    | Bundled Lua 5.1 is used if not present |
| GMP        | 5.0.0+  | Bundled mini-GMP is used if not present |
| JsonCPP    | 1.0.0+  | Bundled JsonCPP is used if not present |

For Debian/Ubuntu users:

    sudo apt install g++ make libc6-dev cmake libpng-dev libjpeg-dev libxxf86vm-dev libgl1-mesa-dev libsqlite3-dev libogg-dev libvorbis-dev libopenal-dev libcurl4-gnutls-dev libfreetype6-dev zlib1g-dev libgmp-dev libjsoncpp-dev libzstd-dev libluajit-5.1-dev

For Fedora users:

    sudo dnf install make automake gcc gcc-c++ kernel-devel cmake libcurl-devel openal-soft-devel libvorbis-devel libXxf86vm-devel libogg-devel freetype-devel mesa-libGL-devel zlib-devel jsoncpp-devel gmp-devel sqlite-devel luajit-devel leveldb-devel ncurses-devel spatialindex-devel libzstd-devel

For Arch users:

    sudo pacman -S base-devel libcurl-gnutls cmake libxxf86vm libpng sqlite libogg libvorbis openal freetype2 jsoncpp gmp luajit leveldb ncurses zstd

For Alpine users:

    sudo apk add build-base cmake libpng-dev jpeg-dev libxxf86vm-dev mesa-dev sqlite-dev libogg-dev libvorbis-dev openal-soft-dev curl-dev freetype-dev zlib-dev gmp-dev jsoncpp-dev luajit-dev zstd-dev

#### Download

You can install Git for easily keeping your copy up to date.
If you don’t want Git, read below on how to get the source without Git.
This is an example for installing Git on Debian/Ubuntu:

    sudo apt install git

For Fedora users:

    sudo dnf install git

Download source (this is the URL to the latest of source repository, which might not work at all times) using Git:

    git clone --depth 1 https://github.com/minetest/minetest.git
    cd minetest

Download minetest_game (otherwise only the "Development Test" game is available) using Git:

    git clone --depth 1 https://github.com/minetest/minetest_game.git games/minetest_game

Download IrrlichtMt to `lib/irrlichtmt`, it will be used to satisfy the IrrlichtMt dependency that way:

    git clone --depth 1 https://github.com/minetest/irrlicht.git lib/irrlichtmt

Download source, without using Git:

    wget https://github.com/minetest/minetest/archive/master.tar.gz
    tar xf master.tar.gz
    cd minetest-master

Download minetest_game, without using Git:

    cd games/
    wget https://github.com/minetest/minetest_game/archive/master.tar.gz
    tar xf master.tar.gz
    mv minetest_game-master minetest_game
    cd ..

Download IrrlichtMt, without using Git:

    cd lib/
    wget https://github.com/minetest/irrlicht/archive/master.tar.gz
    tar xf master.tar.gz
    mv irrlicht-master irrlichtmt
    cd ..

#### Build

Build a version that runs directly from the source directory:

    cmake . -DRUN_IN_PLACE=TRUE
    make -j$(nproc)

Run it:

    ./bin/minetest

- Use `cmake . -LH` to see all CMake options and their current state.
- If you want to install it system-wide (or are making a distribution package),
  you will want to use `-DRUN_IN_PLACE=FALSE`.
- You can build a bare server by specifying `-DBUILD_SERVER=TRUE`.
- You can disable the client build by specifying `-DBUILD_CLIENT=FALSE`.
- You can select between Release and Debug build by `-DCMAKE_BUILD_TYPE=<Debug or Release>`.
  - Debug build is slower, but gives much more useful output in a debugger.
- If you build a bare server you don't need to have the Irrlicht or IrrlichtMt library installed.
  - In that case use `-DIRRLICHT_INCLUDE_DIR=/some/where/irrlicht/include`.

- Minetest will use the IrrlichtMt package that is found first, given by the following order:
  1. Specified `IRRLICHTMT_BUILD_DIR` CMake variable
  2. `${PROJECT_SOURCE_DIR}/lib/irrlichtmt` (if existent)
  3. Installation of IrrlichtMt in the system-specific library paths
  4. For server builds with disabled `BUILD_CLIENT` variable, the headers from `IRRLICHT_INCLUDE_DIR` will be used.
  - NOTE: Changing the IrrlichtMt build directory (includes system installs) requires regenerating the CMake cache (`rm CMakeCache.txt`)

### CMake options

General options and their default values:

    BUILD_CLIENT=TRUE          - Build Minetest client
    BUILD_SERVER=FALSE         - Build Minetest server
    BUILD_UNITTESTS=TRUE       - Build unittest sources
    CMAKE_BUILD_TYPE=Release   - Type of build (Release vs. Debug)
        Release                - Release build
        Debug                  - Debug build
        SemiDebug              - Partially optimized debug build
        RelWithDebInfo         - Release build with debug information
        MinSizeRel             - Release build with -Os passed to compiler to make executable as small as possible
    ENABLE_CURL=ON             - Build with cURL; Enables use of online mod repo, public serverlist and remote media fetching via http
    ENABLE_CURSES=ON           - Build with (n)curses; Enables a server side terminal (command line option: --terminal)
    ENABLE_GETTEXT=ON          - Build with Gettext; Allows using translations
    ENABLE_GLES=OFF            - Build for OpenGL ES instead of OpenGL (requires support by IrrlichtMt)
    ENABLE_LEVELDB=ON          - Build with LevelDB; Enables use of LevelDB map backend
    ENABLE_POSTGRESQL=ON       - Build with libpq; Enables use of PostgreSQL map backend (PostgreSQL 9.5 or greater recommended)
    ENABLE_REDIS=ON            - Build with libhiredis; Enables use of Redis map backend
    ENABLE_SPATIAL=ON          - Build with LibSpatial; Speeds up AreaStores
    ENABLE_SOUND=ON            - Build with OpenAL, libogg & libvorbis; in-game sounds
    ENABLE_LUAJIT=ON           - Build with LuaJIT (much faster than non-JIT Lua)
    ENABLE_PROMETHEUS=OFF      - Build with Prometheus metrics exporter (listens on tcp/30000 by default)
    ENABLE_SYSTEM_GMP=ON       - Use GMP from system (much faster than bundled mini-gmp)
    ENABLE_SYSTEM_JSONCPP=ON   - Use JsonCPP from system
    OPENGL_GL_PREFERENCE=LEGACY - Linux client build only; See CMake Policy CMP0072 for reference
    RUN_IN_PLACE=FALSE         - Create a portable install (worlds, settings etc. in current directory)
    USE_GPROF=FALSE            - Enable profiling using GProf
    VERSION_EXTRA=             - Text to append to version (e.g. VERSION_EXTRA=foobar -> Minetest 0.4.9-foobar)
    ENABLE_TOUCH=FALSE         - Enable Touchscreen support (requires support by IrrlichtMt)

Library specific options:

    CURL_DLL                        - Only if building with cURL on Windows; path to libcurl.dll
    CURL_INCLUDE_DIR                - Only if building with cURL; directory where curl.h is located
    CURL_LIBRARY                    - Only if building with cURL; path to libcurl.a/libcurl.so/libcurl.lib
    EGL_INCLUDE_DIR                 - Only if building with GLES; directory that contains egl.h
    EGL_LIBRARY                     - Only if building with GLES; path to libEGL.a/libEGL.so
    EXTRA_DLL                       - Only on Windows; optional paths to additional DLLs that should be packaged
    FREETYPE_INCLUDE_DIR_freetype2  - Directory that contains files such as ftimage.h
    FREETYPE_INCLUDE_DIR_ft2build   - Directory that contains ft2build.h
    FREETYPE_LIBRARY                - Path to libfreetype.a/libfreetype.so/freetype.lib
    FREETYPE_DLL                    - Only on Windows; path to libfreetype-6.dll
    GETTEXT_DLL                     - Only when building with gettext on Windows; paths to libintl + libiconv DLLs
    GETTEXT_INCLUDE_DIR             - Only when building with gettext; directory that contains iconv.h
    GETTEXT_LIBRARY                 - Only when building with gettext on Windows; path to libintl.dll.a
    GETTEXT_MSGFMT                  - Only when building with gettext; path to msgfmt/msgfmt.exe
    IRRLICHT_DLL                    - Only on Windows; path to IrrlichtMt.dll
    IRRLICHT_INCLUDE_DIR            - Directory that contains IrrCompileConfig.h (usable for server build only)
    LEVELDB_INCLUDE_DIR             - Only when building with LevelDB; directory that contains db.h
    LEVELDB_LIBRARY                 - Only when building with LevelDB; path to libleveldb.a/libleveldb.so/libleveldb.dll.a
    LEVELDB_DLL                     - Only when building with LevelDB on Windows; path to libleveldb.dll
    PostgreSQL_INCLUDE_DIR          - Only when building with PostgreSQL; directory that contains libpq-fe.h
    PostgreSQL_LIBRARY              - Only when building with PostgreSQL; path to libpq.a/libpq.so/libpq.lib
    REDIS_INCLUDE_DIR               - Only when building with Redis; directory that contains hiredis.h
    REDIS_LIBRARY                   - Only when building with Redis; path to libhiredis.a/libhiredis.so
    SPATIAL_INCLUDE_DIR             - Only when building with LibSpatial; directory that contains spatialindex/SpatialIndex.h
    SPATIAL_LIBRARY                 - Only when building with LibSpatial; path to libspatialindex_c.so/spatialindex-32.lib
    LUA_INCLUDE_DIR                 - Only if you want to use LuaJIT; directory where luajit.h is located
    LUA_LIBRARY                     - Only if you want to use LuaJIT; path to libluajit.a/libluajit.so
    OGG_DLL                         - Only if building with sound on Windows; path to libogg.dll
    OGG_INCLUDE_DIR                 - Only if building with sound; directory that contains an ogg directory which contains ogg.h
    OGG_LIBRARY                     - Only if building with sound; path to libogg.a/libogg.so/libogg.dll.a
    OPENAL_DLL                      - Only if building with sound on Windows; path to OpenAL32.dll
    OPENAL_INCLUDE_DIR              - Only if building with sound; directory where al.h is located
    OPENAL_LIBRARY                  - Only if building with sound; path to libopenal.a/libopenal.so/OpenAL32.lib
    OPENGLES2_INCLUDE_DIR           - Only if building with GLES; directory that contains gl2.h
    OPENGLES2_LIBRARY               - Only if building with GLES; path to libGLESv2.a/libGLESv2.so
    SQLITE3_INCLUDE_DIR             - Directory that contains sqlite3.h
    SQLITE3_LIBRARY                 - Path to libsqlite3.a/libsqlite3.so/sqlite3.lib
    VORBISFILE_LIBRARY              - Only if building with sound; path to libvorbisfile.a/libvorbisfile.so/libvorbisfile.dll.a
    VORBIS_DLL                      - Only if building with sound on Windows; paths to vorbis DLLs
    VORBIS_INCLUDE_DIR              - Only if building with sound; directory that contains a directory vorbis with vorbisenc.h inside
    VORBIS_LIBRARY                  - Only if building with sound; path to libvorbis.a/libvorbis.so/libvorbis.dll.a
    XXF86VM_LIBRARY                 - Only on Linux; path to libXXf86vm.a/libXXf86vm.so
    ZLIB_DLL                        - Only on Windows; path to zlib1.dll
    ZLIB_INCLUDE_DIR                - Directory that contains zlib.h
    ZLIB_LIBRARY                    - Path to libz.a/libz.so/zlib.lib
    ZSTD_DLL                        - Only on Windows; path to libzstd.dll
    ZSTD_INCLUDE_DIR                - Directory that contains zstd.h
    ZSTD_LIBRARY                    - Path to libzstd.a/libzstd.so/ztd.lib

### Compiling on Windows using MSVC

### Requirements

- [Visual Studio 2015 or newer](https://visualstudio.microsoft.com)
- [CMake](https://cmake.org/download/)
- [vcpkg](https://github.com/Microsoft/vcpkg)
- [Git](https://git-scm.com/downloads)

### Compiling and installing the dependencies

It is highly recommended to use vcpkg as package manager.

After you successfully built vcpkg you can easily install the required libraries:
```powershell
vcpkg install zlib zstd curl[winssl] openal-soft libvorbis libogg libjpeg-turbo sqlite3 freetype luajit gmp jsoncpp opengl-registry --triplet x64-windows
```

- **Don't forget about IrrlichtMt.** The easiest way is to clone it to `lib/irrlichtmt` as described in the Linux section.
- `curl` is optional, but required to read the serverlist, `curl[winssl]` is required to use the content store.
- `openal-soft`, `libvorbis` and `libogg` are optional, but required to use sound.
- `luajit` is optional, it replaces the integrated Lua interpreter with a faster just-in-time interpreter.
- `gmp` and `jsoncpp` are optional, otherwise the bundled versions will be compiled

There are other optional libraries, but they are not tested if they can build and link correctly.

Use `--triplet` to specify the target triplet, e.g. `x64-windows` or `x86-windows`.

### Compile Minetest

#### a) Using the vcpkg toolchain and CMake GUI
1. Start up the CMake GUI
2. Select **Browse Source...** and select DIR/minetest
3. Select **Browse Build...** and select DIR/minetest-build
4. Select **Configure**
5. Choose the right visual Studio version and target platform. It has to match the version of the installed dependencies
6. Choose **Specify toolchain file for cross-compiling**
7. Click **Next**
8. Select the vcpkg toolchain file e.g. `D:/vcpkg/scripts/buildsystems/vcpkg.cmake`
9. Click Finish
10. Wait until cmake have generated the cash file
11. If there are any errors, solve them and hit **Configure**
12. Click **Generate**
13. Click **Open Project**
14. Compile Minetest inside Visual studio.

#### b) Using the vcpkg toolchain and the commandline

Run the following script in PowerShell:

```powershell
cmake . -G"Visual Studio 15 2017 Win64" -DCMAKE_TOOLCHAIN_FILE=D:/vcpkg/scripts/buildsystems/vcpkg.cmake -DCMAKE_BUILD_TYPE=Release -DENABLE_GETTEXT=OFF -DENABLE_CURSES=OFF
cmake --build . --config Release
```
Make sure that the right compiler is selected and the path to the vcpkg toolchain is correct.

### Windows Installer using WiX Toolset

Requirements:
* [Visual Studio 2017](https://visualstudio.microsoft.com/)
* [WiX Toolset](https://wixtoolset.org/)

In the Visual Studio 2017 Installer select **Optional Features -> WiX Toolset**.

Build the binaries as described above, but make sure you unselect `RUN_IN_PLACE`.

Open the generated project file with Visual Studio. Right-click **Package** and choose **Generate**.
It may take some minutes to generate the installer.

### Compiling on MacOS

#### Requirements
- [Homebrew](https://brew.sh/)
- [Git](https://git-scm.com/downloads)

Install dependencies with homebrew:

```
brew install cmake freetype gettext gmp hiredis jpeg jsoncpp leveldb libogg libpng libvorbis luajit zstd
```

#### Download

Download source (this is the URL to the latest of source repository, which might not work at all times) using Git:

```bash
git clone --depth 1 https://github.com/minetest/minetest.git
cd minetest
```

Download minetest_game (otherwise only the "Development Test" game is available) using Git:

```
git clone --depth 1 https://github.com/minetest/minetest_game.git games/minetest_game
```

Download Minetest's fork of Irrlicht:

```
git clone --depth 1 https://github.com/minetest/irrlicht.git lib/irrlichtmt
```

#### Build

```bash
mkdir cmakebuild
cd cmakebuild

cmake .. \
    -DCMAKE_OSX_DEPLOYMENT_TARGET=10.14 \
    -DCMAKE_FIND_FRAMEWORK=LAST \
    -DCMAKE_INSTALL_PREFIX=../build/macos/ \
    -DRUN_IN_PLACE=FALSE -DENABLE_GETTEXT=TRUE

make -j$(nproc)
make install
```

#### Run

```
open ./build/macos/minetest.app
```

Docker
------
We provide Minetest server Docker images using the GitLab mirror registry.

Images are built on each commit and available using the following tag scheme:

* `registry.gitlab.com/minetest/minetest/server:latest` (latest build)
* `registry.gitlab.com/minetest/minetest/server:<branch/tag>` (current branch or current tag)
* `registry.gitlab.com/minetest/minetest/server:<commit-id>` (current commit id)

If you want to test it on a Docker server you can easily run:

	sudo docker run registry.gitlab.com/minetest/minetest/server:<docker tag>

If you want to use it in a production environment you should use volumes bound to the Docker host
to persist data and modify the configuration:

	sudo docker create -v /home/minetest/data/:/var/lib/minetest/ -v /home/minetest/conf/:/etc/minetest/ registry.gitlab.com/minetest/minetest/server:master

Data will be written to `/home/minetest/data` on the host, and configuration will be read from `/home/minetest/conf/minetest.conf`.

**Note:** If you don't understand the previous commands please read the official Docker documentation before use.

You can also host your Minetest server inside a Kubernetes cluster. See our example implementation in [`misc/kubernetes.yml`](misc/kubernetes.yml).


Version scheme
--------------
We use `major.minor.patch` since 5.0.0-dev. Prior to that we used `0.major.minor`.

- Major is incremented when the release contains breaking changes, all other
numbers are set to 0.
- Minor is incremented when the release contains new non-breaking features,
patch is set to 0.
- Patch is incremented when the release only contains bugfixes and very
minor/trivial features considered necessary.

Since 5.0.0-dev and 0.4.17-dev, the dev notation refers to the next release,
i.e.: 5.0.0-dev is the development version leading to 5.0.0.
Prior to that we used `previous_version-dev`.
