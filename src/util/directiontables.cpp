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

#include "directiontables.h"

const v3pos_t g_6dirs[6] =
{
	// +right, +top, +back
	v3pos_t( 0, 0, 1), // back
	v3pos_t( 0, 1, 0), // top
	v3pos_t( 1, 0, 0), // right
	v3pos_t( 0, 0,-1), // front
	v3pos_t( 0,-1, 0), // bottom
	v3pos_t(-1, 0, 0) // left
};

const v3bpos_t g_6dirs_b[6] =
{
	// +right, +top, +back
	v3bpos_t( 0, 0, 1), // back
	v3bpos_t( 0, 1, 0), // top
	v3bpos_t( 1, 0, 0), // right
	v3bpos_t( 0, 0,-1), // front
	v3bpos_t( 0,-1, 0), // bottom
	v3bpos_t(-1, 0, 0) // left
};

const v3pos_t g_7dirs[7] =
{
	v3pos_t(0,0,1), // back
	v3pos_t(0,1,0), // top
	v3pos_t(1,0,0), // right
	v3pos_t(0,0,-1), // front
	v3pos_t(0,-1,0), // bottom
	v3pos_t(-1,0,0), // left
	v3pos_t(0,0,0), // self
};

const v3pos_t g_26dirs[26] =
{
	// +right, +top, +back
	v3pos_t( 0, 0, 1), // back
	v3pos_t( 0, 1, 0), // top
	v3pos_t( 1, 0, 0), // right
	v3pos_t( 0, 0,-1), // front
	v3pos_t( 0,-1, 0), // bottom
	v3pos_t(-1, 0, 0), // left
	// 6
	v3pos_t(-1, 1, 0), // top left
	v3pos_t( 1, 1, 0), // top right
	v3pos_t( 0, 1, 1), // top back
	v3pos_t( 0, 1,-1), // top front
	v3pos_t(-1, 0, 1), // back left
	v3pos_t( 1, 0, 1), // back right
	v3pos_t(-1, 0,-1), // front left
	v3pos_t( 1, 0,-1), // front right
	v3pos_t(-1,-1, 0), // bottom left
	v3pos_t( 1,-1, 0), // bottom right
	v3pos_t( 0,-1, 1), // bottom back
	v3pos_t( 0,-1,-1), // bottom front
	// 18
	v3pos_t(-1, 1, 1), // top back-left
	v3pos_t( 1, 1, 1), // top back-right
	v3pos_t(-1, 1,-1), // top front-left
	v3pos_t( 1, 1,-1), // top front-right
	v3pos_t(-1,-1, 1), // bottom back-left
	v3pos_t( 1,-1, 1), // bottom back-right
	v3pos_t(-1,-1,-1), // bottom front-left
	v3pos_t( 1,-1,-1), // bottom front-right
	// 26
};

const v3pos_t g_27dirs[27] =
{
	// +right, +top, +back
	v3pos_t( 0, 0, 1), // back
	v3pos_t( 0, 1, 0), // top
	v3pos_t( 1, 0, 0), // right
	v3pos_t( 0, 0,-1), // front
	v3pos_t( 0,-1, 0), // bottom
	v3pos_t(-1, 0, 0), // left
	// 6
	v3pos_t(-1, 1, 0), // top left
	v3pos_t( 1, 1, 0), // top right
	v3pos_t( 0, 1, 1), // top back
	v3pos_t( 0, 1,-1), // top front
	v3pos_t(-1, 0, 1), // back left
	v3pos_t( 1, 0, 1), // back right
	v3pos_t(-1, 0,-1), // front left
	v3pos_t( 1, 0,-1), // front right
	v3pos_t(-1,-1, 0), // bottom left
	v3pos_t( 1,-1, 0), // bottom right
	v3pos_t( 0,-1, 1), // bottom back
	v3pos_t( 0,-1,-1), // bottom front
	// 18
	v3pos_t(-1, 1, 1), // top back-left
	v3pos_t( 1, 1, 1), // top back-right
	v3pos_t(-1, 1,-1), // top front-left
	v3pos_t( 1, 1,-1), // top front-right
	v3pos_t(-1,-1, 1), // bottom back-left
	v3pos_t( 1,-1, 1), // bottom back-right
	v3pos_t(-1,-1,-1), // bottom front-left
	v3pos_t( 1,-1,-1), // bottom front-right
	// 26
	v3pos_t(0,0,0),
};

const u8 wallmounted_to_facedir[6] = {
	20,
	0,
	16 + 1,
	12 + 3,
	8,
	4 + 2
};
