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

#include "craftitemdef.h"
#include "irrlichttypes.h"
#include "log.h"
#include <sstream>
#include "utility.h"
#include <map>

CraftItemDefinition::CraftItemDefinition():
	imagename(""),
	cookresult_item(""),
	furnace_cooktime(3.0),
	furnace_burntime(-1.0),
	usable(false),
	liquids_pointable(false),
	dropcount(-1),
	stack_max(99)
{}

std::string CraftItemDefinition::dump()
{
	std::ostringstream os(std::ios::binary);
	os<<"imagename="<<imagename;
	os<<", cookresult_item="<<cookresult_item;
	os<<", furnace_cooktime="<<furnace_cooktime;
	os<<", furnace_burntime="<<furnace_burntime;
	os<<", usable="<<usable;
	os<<", liquids_pointable="<<liquids_pointable;
	os<<", dropcount="<<dropcount;
	os<<", stack_max="<<stack_max;
	return os.str();
}

void CraftItemDefinition::serialize(std::ostream &os)
{
	writeU8(os, 0); // version
	os<<serializeString(imagename);
	os<<serializeString(cookresult_item);
	writeF1000(os, furnace_cooktime);
	writeF1000(os, furnace_burntime);
	writeU8(os, usable);
	writeU8(os, liquids_pointable);
	writeS16(os, dropcount);
	writeS16(os, stack_max);
}

void CraftItemDefinition::deSerialize(std::istream &is)
{
	int version = readU8(is);
	if(version != 0) throw SerializationError(
			"unsupported CraftItemDefinition version");
	imagename = deSerializeString(is);
	cookresult_item = deSerializeString(is);
	furnace_cooktime = readF1000(is);
	furnace_burntime = readF1000(is);
	usable = readU8(is);
	liquids_pointable = readU8(is);
	dropcount = readS16(is);
	stack_max = readS16(is);
}

class CCraftItemDefManager: public IWritableCraftItemDefManager
{
public:
	virtual ~CCraftItemDefManager()
	{
		clear();
	}
	virtual const CraftItemDefinition* getCraftItemDefinition(const std::string &itemname_) const
	{
		// Convert name according to possible alias
		std::string itemname = getAlias(itemname_);
		// Get the definition
		core::map<std::string, CraftItemDefinition*>::Node *n;
		n = m_item_definitions.find(itemname);
		if(n == NULL)
			return NULL;
		return n->getValue();
	}
	virtual std::string getImagename(const std::string &itemname) const
	{
		const CraftItemDefinition *def = getCraftItemDefinition(itemname);
		if(def == NULL)
			return "";
		return def->imagename;
	}
	virtual std::string getAlias(const std::string &name) const
	{
		std::map<std::string, std::string>::const_iterator i;
		i = m_aliases.find(name);
		if(i != m_aliases.end())
			return i->second;
		return name;
	}
	virtual bool registerCraftItem(std::string itemname, const CraftItemDefinition &def)
	{
		infostream<<"registerCraftItem: registering CraftItem \""<<itemname<<"\""<<std::endl;
		m_item_definitions[itemname] = new CraftItemDefinition(def);

		// Remove conflicting alias if it exists
		bool alias_removed = (m_aliases.erase(itemname) != 0);
		if(alias_removed)
			infostream<<"cidef: erased alias "<<itemname
					<<" because item was defined"<<std::endl;
		
		return true;
	}
	virtual void clear()
	{
		for(core::map<std::string, CraftItemDefinition*>::Iterator
				i = m_item_definitions.getIterator();
				i.atEnd() == false; i++){
			delete i.getNode()->getValue();
		}
		m_item_definitions.clear();
		m_aliases.clear();
	}
	virtual void setAlias(const std::string &name,
			const std::string &convert_to)
	{
		if(getCraftItemDefinition(name) != NULL){
			infostream<<"nidef: not setting alias "<<name<<" -> "<<convert_to
					<<": "<<name<<" is already defined"<<std::endl;
			return;
		}
		infostream<<"nidef: setting alias "<<name<<" -> "<<convert_to
				<<std::endl;
		m_aliases[name] = convert_to;
	}
	virtual void serialize(std::ostream &os)
	{
		writeU8(os, 0); // version
		u16 count = m_item_definitions.size();
		writeU16(os, count);
		for(core::map<std::string, CraftItemDefinition*>::Iterator
				i = m_item_definitions.getIterator();
				i.atEnd() == false; i++){
			std::string name = i.getNode()->getKey();
			CraftItemDefinition *def = i.getNode()->getValue();
			// Serialize name
			os<<serializeString(name);
			// Serialize CraftItemDefinition and write wrapped in a string
			std::ostringstream tmp_os(std::ios::binary);
			def->serialize(tmp_os);
			os<<serializeString(tmp_os.str());
		}

		writeU16(os, m_aliases.size());
		for(std::map<std::string, std::string>::const_iterator
				i = m_aliases.begin(); i != m_aliases.end(); i++)
		{
			os<<serializeString(i->first);
			os<<serializeString(i->second);
		}
	}
	virtual void deSerialize(std::istream &is)
	{
		// Clear everything
		clear();
		// Deserialize
		int version = readU8(is);
		if(version != 0) throw SerializationError(
				"unsupported CraftItemDefManager version");
		u16 count = readU16(is);
		for(u16 i=0; i<count; i++){
			// Deserialize name
			std::string name = deSerializeString(is);
			// Deserialize a string and grab a CraftItemDefinition from it
			std::istringstream tmp_is(deSerializeString(is), std::ios::binary);
			CraftItemDefinition def;
			def.deSerialize(tmp_is);
			// Register
			registerCraftItem(name, def);
		}

		u16 num_aliases = readU16(is);
		if(!is.eof()){
			for(u16 i=0; i<num_aliases; i++){
				std::string name = deSerializeString(is);
				std::string convert_to = deSerializeString(is);
				m_aliases[name] = convert_to;
			}
		}
	}
private:
	// Key is name
	core::map<std::string, CraftItemDefinition*> m_item_definitions;
	// Aliases
	std::map<std::string, std::string> m_aliases;
};

IWritableCraftItemDefManager* createCraftItemDefManager()
{
	return new CCraftItemDefManager();
}
