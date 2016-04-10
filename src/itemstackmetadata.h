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

#ifndef ITEMSTACKMETADATA_HEADER
#define ITEMSTACKMETADATA_HEADER

#include "irr_v3d.h"
#include <iostream>
#include <vector>
#include "util/string.h"

/*
	ItemStackMetadata stores arbitary amounts of data for items as a key-value store.
*/

class Inventory;
class IItemDefManager;

class ItemStackMetadata
{
public:
	ItemStackMetadata():
		m_stringvars()
	{
	};
	~ItemStackMetadata(){};

	std::string serialize() const;
	void deSerialize(std::string &in);

	void clear();
	bool isEmpty() const;
	// Generic key/value store
	std::string getString(const std::string &name, unsigned short recursion = 0) const;
	void setString(const std::string &name, const std::string &var);
	// Support variable names in values
	std::string resolveString(const std::string &str, unsigned short recursion = 0) const;
	StringMap getStrings(){
			return m_stringvars;
	};
private:
	StringMap m_stringvars;
};

#endif

