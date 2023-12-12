/*
Minetest
Copyright (C) 2022 sfan5 <sfan5@live.de>

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

#include "cpp_api/s_base.h"

struct BlockMakeData;

/*
 * Note that this is the class defining the functions called inside the emerge
 * Lua state, not the server one.
 */

class ScriptApiMapgen : virtual public ScriptApiBase
{
public:

	void on_mods_loaded();
	void on_shutdown();

	// Called after generating a piece of map before writing it to the map
	void on_generated(BlockMakeData *bmdata);
};
