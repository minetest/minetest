/*
Minetest
Copyright (C) 2010-2013 celeron55, Perttu Ahola <celeron55@gmail.com>

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
*/

#ifndef BASICMACROS_HEADER
#define BASICMACROS_HEADER

#include <algorithm>

#define ARRLEN(x) (sizeof(x) / sizeof((x)[0]))

#define MYMIN(a, b) ((a) < (b) ? (a) : (b))

#define MYMAX(a, b) ((a) > (b) ? (a) : (b))

#define CONTAINS(c, v) (std::find((c).begin(), (c).end(), (v)) != (c).end())

#endif
