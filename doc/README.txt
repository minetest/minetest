Minetest-c55
---------------

Copyright (c) 2010 Perttu Ahola <celeron55@gmail.com>

An InfiniMiner/Minecraft inspired game.

NOTE: This file is somewhat outdated most of the time.

This is a development version:
- Don't expect it to work as well as a finished game will.
- Please report any bugs to me. That way I can fix them to the next release.
	- debug.txt is useful when the game crashes.

Public servers:
	kray.dy.fi :30000 (friend's server)
	celeron.55.lt :30000 (my own server)

Controls:
- See the in-game pause menu

Map directory:
- Map is stored in a directory, which can be removed to generate a new map.
- There is na command-line option for it: --map-dir
- As default, it is located in:
		../map
- Otherwise something like this:
	Windows: C:\Documents and Settings\user\Application Data\minetest\map
	Linux: ~/.minetest/map
	OS X: ~/Library/Application Support/map

Configuration file:
- An optional configuration file can be used. See minetest.conf.example.
- Path to file can be passed as a parameter to the executable:
	--config <path-to-file>
- Defaults:
	- If built with -DRUN_IN_PLACE:
		../minetest.conf
		../../minetest.conf
	- Otherwise something like this:
		Windows: C:\Documents and Settings\user\Application Data\minetest\minetest.conf
		Linux: ~/.minetest/minetest.conf
		OS X: ~/Library/Application Support/minetest.conf

Command-line options:
- Use --help

Compiling on GNU/Linux:

- You need:
	* CMake
	* Irrlicht
	* Zlib
- You can probably find these in your distro's package repository.
- Building has been tested to work flawlessly on many systems.

- Check possible options:
	$ cd whatever/minetest
	$ cmake . -LH

- A system-wide install:
	$ cd whatever/minetest
	$ cmake . -DCMAKE_INSTALL_PREFIX=/usr/local
	$ make -j2
	$ sudo make install

	$ minetest

- Install to home directory:
	$ cd whatever/minetest
	$ cmake . -DCMAKE_INSTALL_PREFIX=~/minetest_install
	$ make -j2
	$ make install

	$ ~/minetest_install/bin/minetest

- For running in the source directory:
	$ cd whatever/minetest
	$ cmake . -DRUN_IN_PLACE
	$ make -j2

	$ ./bin/minetest

Compiling on Windows:
- You need CMake, Irrlicht, Zlib and Visual Studio or MinGW
- NOTE: Probably it will not work easily and you will need to fix some stuff.
- Steps:
	- Start up the CMake GUI
	- Select your compiler
	- Hit "Configure"
	- Set up some options and paths
	- Hit "Configure"
	- Hit "Generate"
	- VC: Open the generated .sln and build it
	- MinGW: Browse to the build directory and run 'make'

License of Minetest-c55
-----------------------

Minetest-c55
Copyright (C) 2010-2011 celeron55, Perttu Ahola <celeron55@gmail.com>

This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License along
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


