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

#ifndef METADATA_HEADER
#define METADATA_HEADER

#include "irr_v3d.h"
#include <iostream>
#include <vector>
#include "util/string.h"

/*
	NodeMetadata stores arbitary amounts of data for special blocks.
	Used for furnaces, chests and signs.

	There are two interaction methods: inventory menu and text input.
	Only one can be used for a single metadata, thus only inventory OR
	text input should exist in a metadata.
*/

class Inventory;
class IItemDefManager;

class Metadata
{
public:
	void clear();
	bool empty() const;

	// Generic key/value store
	const std::string &getString(const std::string &name, u16 recursion = 0) const;
	void setString(const std::string &name, const std::string &var);
	// Support variable names in values
	const std::string &resolveString(const std::string &str, u16 recursion = 0) const;
	const StringMap &getStrings() const
	{
		return m_stringvars;
	}
private:
	StringMap m_stringvars;
};

#endif
