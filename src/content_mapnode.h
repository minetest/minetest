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

#pragma once

#include "mapnode.h"

/*
	Legacy node definitions
*/

// Backwards compatibility for non-extended content types in v19
extern content_t trans_table_19[21][2];
MapNode mapnode_translate_to_internal(MapNode n_from, u8 version);

// Get legacy node name mapping for loading old blocks
class NameIdMapping;
void content_mapnode_get_name_id_mapping(NameIdMapping *nimap);
