/*
Minetest-c55
Copyright (C) 2012 celeron55, Perttu Ahola <celeron55@gmail.com>

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

#ifndef ITEMGROUP_HEADER
#define ITEMGROUP_HEADER

#include "irrlichttypes_extrabloated.h"
#include <string>
#include <map>

typedef std::map<std::string, int> ItemGroupList;

static inline int itemgroup_get(const ItemGroupList &groups,
		const std::string &name)
{
	std::map<std::string, int>::const_iterator i = groups.find(name);
	if(i == groups.end())
		return 0;
	return i->second;
}

#endif

