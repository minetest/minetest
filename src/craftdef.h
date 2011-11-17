/*
Minetest-c55
Copyright (C) 2011 celeron55, Perttu Ahola <celeron55@gmail.com>

This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License along
with this program; if not, write to the Free Software Foundation, Inc.,
51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
*/

#ifndef CRAFTDEF_HEADER
#define CRAFTDEF_HEADER

#include <string>
#include <iostream>
#include <vector>
class IGameDef;
class InventoryItem;

struct CraftPointerInput
{
	unsigned int width;
	std::vector<InventoryItem*> items;

	CraftPointerInput(unsigned int width_, const std::vector<InventoryItem*> &items_):
		width(width_),
		items(items_)
	{}
	CraftPointerInput():
		width(0)
	{}
	~CraftPointerInput();
	unsigned int height() const{
		return (items.size() + width - 1) / width;
	}
};

struct CraftInput
{
	unsigned int width;
	std::vector<std::string> items;

	CraftInput(unsigned int width_, const std::vector<std::string> &items_):
		width(width_),
		items(items_)
	{}
	CraftInput():
		width(0)
	{}
	unsigned int height() const{
		return (items.size() + width - 1) / width;
	}
	std::string dump() const;
};

struct CraftDefinition
{
	std::string output;
	CraftInput input;

	CraftDefinition(){}
	CraftDefinition(const std::string &output_, unsigned int width_,
			const std::vector<std::string> &input_):
		output(output_),
		input(width_, input_)
	{}
	
	std::string dump() const;
	void serialize(std::ostream &os) const;
	void deSerialize(std::istream &is);
};

class ICraftDefManager
{
public:
	ICraftDefManager(){}
	virtual ~ICraftDefManager(){}
	virtual InventoryItem* getCraftResult(const CraftPointerInput &input_cpi,
			IGameDef *gamedef) const=0;
	
	virtual void serialize(std::ostream &os)=0;
};

class IWritableCraftDefManager : public ICraftDefManager
{
public:
	IWritableCraftDefManager(){}
	virtual ~IWritableCraftDefManager(){}
	virtual InventoryItem* getCraftResult(const CraftPointerInput &input_cpi,
			IGameDef *gamedef) const=0;
			
	virtual void registerCraft(const CraftDefinition &def)=0;
	virtual void clear()=0;

	virtual void serialize(std::ostream &os)=0;
	virtual void deSerialize(std::istream &is)=0;
};

IWritableCraftDefManager* createCraftDefManager();

#endif

