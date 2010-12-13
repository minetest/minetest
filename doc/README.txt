Minetest-c55
---------------

Copyright (c) 2010 Perttu Ahola <celeron55@gmail.com>

An InfiniMiner/Minecraft inspired game.

This is a development version:
- Don't expect it to work as well as a finished game will.
- Please report any bugs to me. That way I can fix them to the next release.
	- debug.txt is very useful when the game crashes.

Public servers:
	kray.dy.fi :30000 (friend's server - usually in creative mode)
	celer.oni.biz :30000 (main development server)
- Both of these have very limited bandwidth and the game will become laggy
  with 4-5 players.
- If you want to run a server, I can list you on my website and in here.

Features, as of now:
- Almost Infinite Map (limited to +-31000 blocks in any direction at the moment)
    - Minecraft alpha has a height restriction of 128 blocks
- Map Generator capable of taking advantage of the infinite map

Controls:
- WASD+mouse: Move
- Mouse L: Dig
- Mouse R: Place block
- Mouse Wheel: Change item
- F: Change item
- R: Toggle full view range

Configuration file:
- An optional configuration file can be used. See minetest.conf.example.
- Path to file can be passed as a parameter to the executable.
- If not given as a parameter, these are checked, in order:
	../minetest.conf
	../../minetest.conf

Running on Windows:
- The working directory should be ./bin

Running on GNU/Linux:
- fasttest is a linux binary compiled on a recent Arch Linux installation.
  It should run on most recent GNU/Linux distributions.
- Browse to the game ./bin directory and type:
    LD_LIBRARY_PATH=. ./fasttest
- If it doesn't work, use wine. I aim at 100% compatibility with wine.

Compiling on GNU/Linux:
- You need:
* Irrlicht:
	http://downloads.sourceforge.net/irrlicht/irrlicht-1.7.2.zip
* JThread:
	http://research.edm.uhasselt.be/~jori/page/index.php?n=CS.Jthread
* zlib:
	- Get the -dev package from your package manager.
- Irrlicht and JThread are very likely not to be found from your distro's
  repository.
- Compiling each of them should be fairly unproblematic, though.

Compiling on Windows:
- You need Irrlicht, JThread and zlib, see above
- Be sure to
  #define JMUTEX_CRITICALSECTION
  in jmutex.h before compiling it. Otherwise mutexes will be very slow.

License of Minetest-c55
-----------------------

Minetest-c55
Copyright (C) 2010 celeron55, Perttu Ahola <celeron55@gmail.com>

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


