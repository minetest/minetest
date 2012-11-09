/*
Minetest-c55
Copyright (C) 2010-2011 celeron55, Perttu Ahola <celeron55@gmail.com>

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

#ifndef NODEMETADATA_HEADER
#define NODEMETADATA_HEADER

#include "irrlichttypes_bloated.h"
#include <string>
#include <iostream>
#include <map>

/*
	NodeMetadata stores arbitary amounts of data for special blocks.
	Used for furnaces, chests and signs.

	There are two interaction methods: inventory menu and text input.
	Only one can be used for a single metadata, thus only inventory OR
	text input should exist in a metadata.
*/

class Inventory;
class IGameDef;

class NodeMetadata
{
public:
	NodeMetadata(IGameDef *gamedef);
	~NodeMetadata();
	
	void serialize(std::ostream &os) const;
	void deSerialize(std::istream &is);
	
	void clear();

	// Generic key/value store
	std::string getString(const std::string &name) const
	{
		std::map<std::string, std::string>::const_iterator i;
		i = m_stringvars.find(name);
		if(i == m_stringvars.end())
			return "";
		return resolveString(i->second);
	}
	void setString(const std::string &name, const std::string &var)
	{
		if(var.empty())
			m_stringvars.erase(name);
		else
			m_stringvars[name] = var;
	}
	// support variable names in values
	std::string resolveString(const std::string &str) const
	{
		if(str.substr(0,2) == "${" && str[str.length()-1] == '}')
			return resolveString(getString(str.substr(2,str.length()-3)));
		return str;
	}
	std::map<std::string, std::string> getStrings() const
	{
		return m_stringvars;
	}

	// The inventory
	Inventory* getInventory()
	{
		return m_inventory;
	}

private:
	std::map<std::string, std::string> m_stringvars;
	Inventory *m_inventory;
};


/*
	List of metadata of all the nodes of a block
*/

class NodeMetadataList
{
public:
	~NodeMetadataList();

	void serialize(std::ostream &os) const;
	void deSerialize(std::istream &is, IGameDef *gamedef);
	
	// Get pointer to data
	NodeMetadata* get(v3s16 p);
	// Deletes data
	void remove(v3s16 p);
	// Deletes old data and sets a new one
	void set(v3s16 p, NodeMetadata *d);
	// Deletes all
	void clear();
	
private:
	std::map<v3s16, NodeMetadata*> m_data;
};

#endif

