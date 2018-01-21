/*
Minetest
Copyright (C) 2013 celeron55, Perttu Ahola <celeron55@gmail.com>

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

#pragma once

enum ModifiedState
{
	// Has not been modified.
	MOD_STATE_CLEAN = 0,
	MOD_RESERVED1 = 1,
	// Has been modified, and will be saved when being unloaded.
	MOD_STATE_WRITE_AT_UNLOAD = 2,
	MOD_RESERVED3 = 3,
	// Has been modified, and will be saved as soon as possible.
	MOD_STATE_WRITE_NEEDED = 4,
	MOD_RESERVED5 = 5,
};
