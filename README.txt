Minetest
========

An InfiniMiner/Minecraft inspired game.

Copyright (c) 2010-2017 Perttu Ahola <celeron55@gmail.com>
and contributors (see source file comments and the version control log)

In case you downloaded the source code:
---------------------------------------
If you downloaded the Minetest Engine source code in which this file is
contained, you probably want to download the minetest_game project too:
  https://github.com/minetest/minetest_game/
See the README.txt in it.

Further documentation
----------------------
- Website: http://minetest.net/
- Wiki: http://wiki.minetest.net/
- Developer wiki: http://dev.minetest.net/
- Forum: http://forum.minetest.net/
- Github: https://github.com/minetest/minetest/
- doc/ directory of source distribution

This game is not finished
--------------------------
- Don't expect it to work as well as a finished game will.
- Please report any bugs. When doing that, debug.txt is useful.

Default controls
-----------------
- Move mouse: Look around
- W, A, S, D: Move
- Space: Jump/move up
- Shift: Sneak/move down
- Q: Drop itemstack
- Shift + Q: Drop single item
- Left mouse button: Dig/punch/take item
- Right mouse button: Place/use
- Shift + right mouse button: Build (without using)
- I: Inventory menu
- Mouse wheel: Select item
- 0-9: Select item
- Z: Zoom (needs zoom privilege)
- T: Chat
- /: Command

- Esc: Pause menu/abort/exit (pauses only singleplayer game)
- R: Enable/disable full range view
- +: Increase view range
- -: Decrease view range
- K: Enable/disable fly mode (needs fly privilege)
- J: Enable/disable fast mode (needs fast privilege)
- H: Enable/disable noclip mode (needs noclip privilege)

- F1:  Hide/show HUD
- F2:  Hide/show chat
- F3:  Disable/enable fog
- F4:  Disable/enable camera update (Mapblocks are not updated anymore when disabled, disabled in release builds)
- F5:  Cycle through debug info screens
- F6:  Cycle through profiler info screens
- F7:  Cycle through camera modes
- F8:  Toggle cinematic mode
- F9:  Cycle through minimap modes
- Shift + F9: Change minimap orientation
- F10: Show/hide console
- F12: Take screenshot
- P: Write stack traces into debug.txt

Most controls are settable in the configuration file, see the section below.

Paths
------
$bin   - Compiled binaries
$share - Distributed read-only data
$user  - User-created modifiable data

Windows .zip / RUN_IN_PLACE source:
$bin   = bin
$share = .
$user  = .

Linux installed:
$bin   = /usr/bin
$share = /usr/share/minetest
$user  = ~/.minetest

macOS:
$bin   = Contents/MacOS
$share = Contents/Resources
$user  = Contents/User OR ~/Library/Application Support/minetest

World directory
----------------
- Worlds can be found as separate folders in:
    $user/worlds/

Configuration file:
-------------------
- Default location:
    $user/minetest.conf
- It is created by Minetest when it is ran the first time.
- A specific file can be specified on the command line:
    --config <path-to-file>
- A run-in-place build will look for the configuration file in
    $location_of_exe/../minetest.conf and also $location_of_exe/../../minetest.conf

Command-line options:
---------------------
- Use --help

Compiling on GNU/Linux:
-----------------------

Install dependencies. Here's an example for Debian/Ubuntu:
$ sudo apt-get install build-essential libirrlicht-dev cmake libbz2-dev libpng-dev libjpeg-dev libxxf86vm-dev libgl1-mesa-dev libsqlite3-dev libogg-dev libvorbis-dev libopenal-dev libcurl4-gnutls-dev libfreetype6-dev zlib1g-dev libgmp-dev libjsoncpp-dev

For Fedora users:
$ sudo dnf install make automake gcc gcc-c++ kernel-devel cmake libcurl* openal* libvorbis* libXxf86vm-devel libogg-devel freetype-devel mesa-libGL-devel zlib-devel jsoncpp-devel irrlicht-devel bzip2-libs gmp-devel sqlite-devel luajit-devel leveldb-devel ncurses-devel doxygen spatialindex-devel bzip2-devel

You can install git for easily keeping your copy up to date.
If you don’t want git, read below on how to get the source without git.
This is an example for installing git on Debian/Ubuntu:
$ sudo apt-get install git

For Fedora users:
$ sudo dnf install git

Download source (this is the URL to the latest of source repository, which might not work at all times) using git:
$ git clone --depth 1 https://github.com/minetest/minetest.git
$ cd minetest

Download minetest_game (otherwise only the "Minimal development test" game is available) using git:
$ git clone --depth 1 https://github.com/minetest/minetest_game.git games/minetest_game

Download source, without using git:
$ wget https://github.com/minetest/minetest/archive/master.tar.gz
$ tar xf master.tar.gz
$ cd minetest-master

Download minetest_game, without using git:
$ cd games/
$ wget https://github.com/minetest/minetest_game/archive/master.tar.gz
$ tar xf master.tar.gz
$ mv minetest_game-master minetest_game
$ cd ..

Build a version that runs directly from the source directory:
$ cmake . -DRUN_IN_PLACE=TRUE
$ make -j <number of processors>

Run it:
$ ./bin/minetest

- Use cmake . -LH to see all CMake options and their current state
- If you want to install it system-wide (or are making a distribution package),
  you will want to use -DRUN_IN_PLACE=FALSE
- You can build a bare server by specifying -DBUILD_SERVER=TRUE
- You can disable the client build by specifying -DBUILD_CLIENT=FALSE
- You can select between Release and Debug build by -DCMAKE_BUILD_TYPE=<Debug or Release>
  - Debug build is slower, but gives much more useful output in a debugger
- If you build a bare server, you don't need to have Irrlicht installed.
  In that case use -DIRRLICHT_SOURCE_DIR=/the/irrlicht/source

CMake options
-------------
General options:

BUILD_CLIENT        - Build Minetest client
BUILD_SERVER        - Build Minetest server
CMAKE_BUILD_TYPE    - Type of build (Release vs. Debug)
    Release         - Release build
    Debug           - Debug build
    SemiDebug       - Partially optimized debug build
    RelWithDebInfo  - Release build with Debug information
    MinSizeRel      - Release build with -Os passed to compiler to make executable as small as possible
ENABLE_CURL         - Build with cURL; Enables use of online mod repo, public serverlist and remote media fetching via http
ENABLE_CURSES       - Build with (n)curses; Enables a server side terminal (command line option: --terminal)
ENABLE_FREETYPE     - Build with FreeType2; Allows using TTF fonts
ENABLE_GETTEXT      - Build with Gettext; Allows using translations
ENABLE_GLES         - Search for Open GLES headers & libraries and use them
ENABLE_LEVELDB      - Build with LevelDB; Enables use of LevelDB map backend
ENABLE_POSTGRESQL   - Build with libpq; Enables use of PostgreSQL map backend (PostgreSQL 9.5 or greater recommended)
ENABLE_REDIS        - Build with libhiredis; Enables use of Redis map backend
ENABLE_SPATIAL      - Build with LibSpatial; Speeds up AreaStores
ENABLE_SOUND        - Build with OpenAL, libogg & libvorbis; in-game Sounds
ENABLE_LUAJIT       - Build with LuaJIT (much faster than non-JIT Lua)
ENABLE_SYSTEM_GMP   - Use GMP from system (much faster than bundled mini-gmp)
RUN_IN_PLACE        - Create a portable install (worlds, settings etc. in current directory)
USE_GPROF           - Enable profiling using GProf
VERSION_EXTRA       - Text to append to version (e.g. VERSION_EXTRA=foobar -> Minetest 0.4.9-foobar)

Library specific options:

BZIP2_INCLUDE_DIR               - Linux only; directory where bzlib.h is located
BZIP2_LIBRARY                   - Linux only; path to libbz2.a/libbz2.so
CURL_DLL                        - Only if building with cURL on Windows; path to libcurl.dll
CURL_INCLUDE_DIR                - Only if building with cURL; directory where curl.h is located
CURL_LIBRARY                    - Only if building with cURL; path to libcurl.a/libcurl.so/libcurl.lib
EGL_INCLUDE_DIR                 - Only if building with GLES; directory that contains egl.h
EGL_LIBRARY                     - Only if building with GLES; path to libEGL.a/libEGL.so
FREETYPE_INCLUDE_DIR_freetype2  - Only if building with Freetype2; directory that contains an freetype directory with files such as ftimage.h in it
FREETYPE_INCLUDE_DIR_ft2build   - Only if building with Freetype2; directory that contains ft2build.h
FREETYPE_LIBRARY                - Only if building with Freetype2; path to libfreetype.a/libfreetype.so/freetype.lib
FREETYPE_DLL                    - Only if building with Freetype2 on Windows; path to libfreetype.dll
GETTEXT_DLL                     - Only when building with Gettext on Windows; path to libintl3.dll
GETTEXT_ICONV_DLL               - Only when building with Gettext on Windows; path to libiconv2.dll
GETTEXT_INCLUDE_DIR             - Only when building with Gettext; directory that contains iconv.h
GETTEXT_LIBRARY                 - Only when building with Gettext on Windows; path to libintl.dll.a
GETTEXT_MSGFMT                  - Only when building with Gettext; path to msgfmt/msgfmt.exe
IRRLICHT_DLL                    - Only on Windows; path to Irrlicht.dll
IRRLICHT_INCLUDE_DIR            - Directory that contains IrrCompileConfig.h
IRRLICHT_LIBRARY                - Path to libIrrlicht.a/libIrrlicht.so/libIrrlicht.dll.a/Irrlicht.lib
LEVELDB_INCLUDE_DIR             - Only when building with LevelDB; directory that contains db.h
LEVELDB_LIBRARY                 - Only when building with LevelDB; path to libleveldb.a/libleveldb.so/libleveldb.dll.a
LEVELDB_DLL                     - Only when building with LevelDB on Windows; path to libleveldb.dll
PostgreSQL_INCLUDE_DIR          - Only when building with PostgreSQL; directory that contains libpq-fe.h
POSTGRESQL_LIBRARY              - Only when building with PostgreSQL; path to libpq.a/libpq.so
REDIS_INCLUDE_DIR               - Only when building with Redis; directory that contains hiredis.h
REDIS_LIBRARY                   - Only when building with Redis; path to libhiredis.a/libhiredis.so
SPATIAL_INCLUDE_DIR             - Only when building with LibSpatial; directory that contains spatialindex/SpatialIndex.h
SPATIAL_LIBRARY                 - Only when building with LibSpatial; path to libspatialindex_c.so/spatialindex-32.lib
LUA_INCLUDE_DIR                 - Only if you want to use LuaJIT; directory where luajit.h is located
LUA_LIBRARY                     - Only if you want to use LuaJIT; path to libluajit.a/libluajit.so
MINGWM10_DLL                    - Only if compiling with MinGW; path to mingwm10.dll
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
VORBISFILE_DLL                  - Only if building with sound on Windows; path to libvorbisfile-3.dll
VORBISFILE_LIBRARY              - Only if building with sound; path to libvorbisfile.a/libvorbisfile.so/libvorbisfile.dll.a
VORBIS_DLL                      - Only if building with sound on Windows; path to libvorbis-0.dll
VORBIS_INCLUDE_DIR              - Only if building with sound; directory that contains a directory vorbis with vorbisenc.h inside
VORBIS_LIBRARY                  - Only if building with sound; path to libvorbis.a/libvorbis.so/libvorbis.dll.a
XXF86VM_LIBRARY                 - Only on Linux; path to libXXf86vm.a/libXXf86vm.so
ZLIB_DLL                        - Only on Windows; path to zlib1.dll
ZLIBWAPI_DLL                    - Only on Windows; path to zlibwapi.dll
ZLIB_INCLUDE_DIR                - Directory that contains zlib.h
ZLIB_LIBRARY                    - Path to libz.a/libz.so/zlibwapi.lib

Compiling on Windows:
---------------------
- This section is outdated. In addition to what is described here:
  - In addition to minetest, you need to download minetest_game.
  - If you wish to have sound support, you need libogg, libvorbis and libopenal

- You need:
	* CMake:
		http://www.cmake.org/cmake/resources/software.html
	* MinGW or Visual Studio
		http://www.mingw.org/
		http://msdn.microsoft.com/en-us/vstudio/default
	* Irrlicht SDK 1.7:
		http://irrlicht.sourceforge.net/downloads.html
	* Zlib headers (zlib125.zip)
		http://www.winimage.com/zLibDll/index.html
	* Zlib library (zlibwapi.lib and zlibwapi.dll from zlib125dll.zip):
		http://www.winimage.com/zLibDll/index.html
	* SQLite3 headers and library
		https://www.sqlite.org/download.html
	* Optional: gettext library and tools:
		http://gnuwin32.sourceforge.net/downlinks/gettext.php
		- This is used for other UI languages. Feel free to leave it out.
	* And, of course, Minetest:
		http://minetest.net/download
- Steps:
	- Select a directory called DIR hereafter in which you will operate.
	- Make sure you have CMake and a compiler installed.
	- Download all the other stuff to DIR and extract them into there.
	  ("extract here", not "extract to packagename/")
	  NOTE: zlib125dll.zip needs to be extracted into zlib125dll
	  NOTE: You need to extract sqlite3.h & sqlite3ext.h from sqlite3 source
	      and sqlite3.dll & sqlite3.def from sqlite3 precompiled binaries
	      into "sqlite3" directory, and generate sqlite3.lib using command
	      "LIB /DEF:sqlite3.def /OUT:sqlite3.lib"
	- All those packages contain a nice base directory in them, which
	  should end up being the direct subdirectories of DIR.
	- You will end up with a directory structure like this (+=dir, -=file):
	-----------------
	+ DIR
		- zlib-1.2.5.tar.gz
		- zlib125dll.zip
		- irrlicht-1.8.3.zip
		- sqlite-amalgamation-3130000.zip (SQLite3 headers)
		- sqlite-dll-win32-x86-3130000.zip (SQLite3 library for 32bit system)
		- 110214175330.zip (or whatever, this is the minetest source)
		+ zlib-1.2.5
			- zlib.h
			+ win32
			...
		+ zlib125dll
			- readme.txt
			+ dll32
			...
		+ irrlicht-1.8.3
			+ lib
			+ include
			...
		+ sqlite3
			sqlite3.h
			sqlite3ext.h
			sqlite3.lib
			sqlite3.dll
		+ gettext (optional)
			+bin
			+include
			+lib
		+ minetest
			+ src
			+ doc
			- CMakeLists.txt
			...
	-----------------
	- Start up the CMake GUI
	- Select "Browse Source..." and select DIR/minetest
	- Now, if using MSVC:
		- Select "Browse Build..." and select DIR/minetest-build
	- Else if using MinGW:
		- Select "Browse Build..." and select DIR/minetest
	- Select "Configure"
	- Select your compiler
	- It will warn about missing stuff, ignore that at this point. (later don't)
	- Make sure the configuration is as follows
	  (note that the versions may differ for you):
	-----------------
	BUILD_CLIENT             [X]
	BUILD_SERVER             [ ]
	CMAKE_BUILD_TYPE         Release
	CMAKE_INSTALL_PREFIX     DIR/minetest-install
	IRRLICHT_SOURCE_DIR      DIR/irrlicht-1.8.3
	RUN_IN_PLACE             [X]
	WARN_ALL                 [ ]
	ZLIB_DLL                 DIR/zlib125dll/dll32/zlibwapi.dll
	ZLIB_INCLUDE_DIR         DIR/zlib-1.2.5
	ZLIB_LIBRARIES           DIR/zlib125dll/dll32/zlibwapi.lib
	GETTEXT_BIN_DIR          DIR/gettext/bin
	GETTEXT_INCLUDE_DIR      DIR/gettext/include
	GETTEXT_LIBRARIES        DIR/gettext/lib/intl.lib
	GETTEXT_MSGFMT           DIR/gettext/bin/msgfmt
	-----------------
	- If CMake complains it couldn't find SQLITE3, choose "Advanced" box on the
	  right top corner, then specify the location of SQLITE3_INCLUDE_DIR and
	  SQLITE3_LIBRARY manually.
	- If you want to build 64-bit minetest, you will need to build 64-bit version
	  of irrlicht engine manually, as only 32-bit pre-built library is provided.
	- Hit "Configure"
	- Hit "Configure" once again 8)
	- If something is still coloured red, you have a problem.
	- Hit "Generate"
	If using MSVC:
		- Open the generated minetest.sln
		- The project defaults to the "Debug" configuration. Make very sure to
		  select "Release", unless you want to debug some stuff (it's slower
		  and might not even work at all)
		- Build the ALL_BUILD project
		- Build the INSTALL project
		- You should now have a working game with the executable in
			DIR/minetest-install/bin/minetest.exe
		- Additionally you may create a zip package by building the PACKAGE
		  project.
	If using MinGW:
		- Using the command line, browse to the build directory and run 'make'
		  (or mingw32-make or whatever it happens to be)
		- You may need to copy some of the downloaded DLLs into bin/, see what
		  running the produced executable tells you it doesn't have.
		- You should now have a working game with the executable in
			DIR/minetest/bin/minetest.exe

Windows releases of minetest are built using a bat script like this:
--------------------------------------------------------------------

set sourcedir=%CD%
set installpath="C:\tmp\minetest_install"
set irrlichtpath="C:\tmp\irrlicht-1.7.2"

set builddir=%sourcedir%\bvc10
mkdir %builddir%
pushd %builddir%
cmake %sourcedir% -G "Visual Studio 10" -DIRRLICHT_SOURCE_DIR=%irrlichtpath% -DRUN_IN_PLACE=TRUE -DCMAKE_INSTALL_PREFIX=%installpath%
if %errorlevel% neq 0 goto fail
"C:\WINDOWS\Microsoft.NET\Framework\v4.0.30319\MSBuild.exe" ALL_BUILD.vcxproj /p:Configuration=Release
if %errorlevel% neq 0 goto fail
"C:\WINDOWS\Microsoft.NET\Framework\v4.0.30319\MSBuild.exe" INSTALL.vcxproj /p:Configuration=Release
if %errorlevel% neq 0 goto fail
"C:\WINDOWS\Microsoft.NET\Framework\v4.0.30319\MSBuild.exe" PACKAGE.vcxproj /p:Configuration=Release
if %errorlevel% neq 0 goto fail
popd
echo Finished.
exit /b 0

:fail
popd
echo Failed.
exit /b 1

License of Minetest textures and sounds
---------------------------------------

This applies to textures and sounds contained in the main Minetest
distribution.

Attribution-ShareAlike 3.0 Unported (CC BY-SA 3.0)
http://creativecommons.org/licenses/by-sa/3.0/

Authors of media files
-----------------------
Everything not listed in here:
Copyright (C) 2010-2012 celeron55, Perttu Ahola <celeron55@gmail.com>

ShadowNinja:
  textures/base/pack/smoke_puff.png

Paramat:
  textures/base/pack/menu_header.png

erlehmann:
  misc/minetest-icon-24x24.png
  misc/minetest-icon.ico
  misc/minetest.svg
  textures/base/pack/logo.png

License of Minetest source code
-------------------------------

Minetest
Copyright (C) 2010-2017 celeron55, Perttu Ahola <celeron55@gmail.com>

This program is free software; you can redistribute it and/or modify
it under the terms of the GNU Lesser General Public License as published by
the Free Software Foundation; either version 2.1 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU Lesser General Public License for more details.

You should have received a copy of the GNU Lesser General Public License along
with this program; if not, write to the Free Software Foundation, Inc.,
51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.

Irrlicht
---------------

This program uses the Irrlicht Engine. http://irrlicht.sourceforge.net/

 The Irrlicht Engine License

Copyright © 2002-2005 Nikolaus Gebhardt

This software is provided 'as-is', without any express or implied
warranty. In no event will the authors be held liable for any damages
arising from the use of this software.

Permission is granted to anyone to use this software for any purpose,
including commercial applications, and to alter it and redistribute
it freely, subject to the following restrictions:

   1. The origin of this software must not be misrepresented; you
      must not claim that you wrote the original software. If you use
	  this software in a product, an acknowledgment in the product
	  documentation would be appreciated but is not required.
   2. Altered source versions must be plainly marked as such, and must
      not be misrepresented as being the original software.
   3. This notice may not be removed or altered from any source
      distribution.


JThread
---------------

This program uses the JThread library. License for JThread follows:

Copyright (c) 2000-2006  Jori Liesenborgs (jori.liesenborgs@gmail.com)

Permission is hereby granted, free of charge, to any person obtaining a
copy of this software and associated documentation files (the "Software"),
to deal in the Software without restriction, including without limitation
the rights to use, copy, modify, merge, publish, distribute, sublicense,
and/or sell copies of the Software, and to permit persons to whom the
Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included
in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
IN THE SOFTWARE.

Lua
---------------

Lua is licensed under the terms of the MIT license reproduced below.
This means that Lua is free software and can be used for both academic
and commercial purposes at absolutely no cost.

For details and rationale, see https://www.lua.org/license.html .

Copyright (C) 1994-2008 Lua.org, PUC-Rio.

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
THE SOFTWARE.

Fonts
---------------

Bitstream Vera Fonts Copyright:

  Copyright (c) 2003 by Bitstream, Inc. All Rights Reserved. Bitstream Vera is
  a trademark of Bitstream, Inc.

Arimo - Apache License, version 2.0
  Digitized data copyright (c) 2010-2012 Google Corporation.

Cousine - Apache License, version 2.0
  Digitized data copyright (c) 2010-2012 Google Corporation.

DroidSansFallBackFull:

  Copyright (C) 2008 The Android Open Source Project

  Licensed under the Apache License, Version 2.0 (the "License");
  you may not use this file except in compliance with the License.
  You may obtain a copy of the License at

     http://www.apache.org/licenses/LICENSE-2.0

  Unless required by applicable law or agreed to in writing, software
  distributed under the License is distributed on an "AS IS" BASIS,
  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  See the License for the specific language governing permissions and
  limitations under the License.
