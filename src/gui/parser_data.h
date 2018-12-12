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

#include <unordered_map>

#include "irr_v2d.h"
#include "rect.h"
#include "guiTable.h"

// These macros were moved here for convenience, feel free to move them

#define MY_CHECKPOS(a,b)													\
	if (v_pos.size() != 2) {												\
		errorstream<< "Invalid pos for element " << a << "specified: \""	\
			<< parts[b] << "\"" << std::endl;								\
			return;															\
	}

#define MY_CHECKGEOM(a,b)													\
	if (v_geom.size() != 2) {												\
		errorstream<< "Invalid pos for element " << a << "specified: \""	\
			<< parts[b] << "\"" << std::endl;								\
			return;															\
	}

// Used for parsing in FormSpecMenu and Widgets
struct parserData {
	bool explicit_size;
	v2f invsize;
	v2s32 size;
	v2f32 offset;
	v2f32 anchor;
	core::rect<s32> rect;
	v2s32 basepos;
	v2u32 screensize;
	std::string focused_fieldname;
	GUITable::TableOptions table_options;
	GUITable::TableColumns table_columns;
	// used to restore table selection/scroll/treeview state
	std::unordered_map<std::string, GUITable::DynamicData> table_dyndata;
};

