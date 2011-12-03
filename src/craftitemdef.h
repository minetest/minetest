/*
Minetest-c55
Copyright (C) 2011 celeron55, Perttu Ahola <celeron55@gmail.com>
Copyright (C) 2011 Kahrl <kahrl@gmx.net>

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

#ifndef CRAFTITEMDEF_HEADER
#define CRAFTITEMDEF_HEADER

#include "common_irrlicht.h"
#include <string>
#include <iostream>

struct CraftItemDefinition
{
	std::string imagename;
	std::string cookresult_item;
	float furnace_cooktime;
	float furnace_burntime;
	bool usable;
	bool liquids_pointable;
	s16 dropcount;
	s16 stack_max;

	CraftItemDefinition();
	std::string dump();
	void serialize(std::ostream &os);
	void deSerialize(std::istream &is);
};

class ICraftItemDefManager
{
public:
	ICraftItemDefManager(){}
	virtual ~ICraftItemDefManager(){}
	virtual const CraftItemDefinition* getCraftItemDefinition(const std::string &itemname) const=0;
	virtual std::string getImagename(const std::string &itemname) const =0;
	virtual std::string getAlias(const std::string &name) const =0;

	virtual void serialize(std::ostream &os)=0;
};

class IWritableCraftItemDefManager : public ICraftItemDefManager
{
public:
	IWritableCraftItemDefManager(){}
	virtual ~IWritableCraftItemDefManager(){}
	virtual const CraftItemDefinition* getCraftItemDefinition(const std::string &itemname) const=0;
	virtual std::string getImagename(const std::string &itemname) const =0;

	virtual bool registerCraftItem(std::string itemname, const CraftItemDefinition &def)=0;
	virtual void clear()=0;
	// Set an alias so that entries named <name> will load as <convert_to>.
	// Alias is not set if <name> has already been defined.
	// Alias will be removed if <name> is defined at a later point of time.
	virtual void setAlias(const std::string &name,
			const std::string &convert_to)=0;

	virtual void serialize(std::ostream &os)=0;
	virtual void deSerialize(std::istream &is)=0;
};

IWritableCraftItemDefManager* createCraftItemDefManager();

#endif
