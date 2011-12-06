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

#include "craftdef.h"

#include "irrlichttypes.h"
#include "log.h"
#include <sstream>
#include "utility.h"
#include "gamedef.h"
#include "inventory.h"
#include "inventorymanager.h" // checkItemCombination

CraftPointerInput::~CraftPointerInput()
{
	for(u32 i=0; i<items.size(); i++)
		delete items[i];
}

CraftPointerInput createPointerInput(const CraftInput &ci, IGameDef *gamedef)
{
	std::vector<InventoryItem*> items;
	for(u32 i=0; i<ci.items.size(); i++){
		InventoryItem *item = NULL;
		if(ci.items[i] != ""){
			std::istringstream iss(ci.items[i], std::ios::binary);
			item = InventoryItem::deSerialize(iss, gamedef);
		}
		items.push_back(item);
	}
	return CraftPointerInput(ci.width, items);
}

CraftInput createInput(const CraftPointerInput &cpi)
{
	std::vector<std::string> items;
	for(u32 i=0; i<cpi.items.size(); i++){
		if(cpi.items[i] == NULL)
			items.push_back("");
		else{
			std::ostringstream oss(std::ios::binary);
			cpi.items[i]->serialize(oss);
			items.push_back(oss.str());
		}
	}
	return CraftInput(cpi.width, items);
}

std::string CraftInput::dump() const
{
	std::ostringstream os(std::ios::binary);
	os<<"(width="<<width<<"){";
	for(u32 i=0; i<items.size(); i++)
		os<<"\""<<items[i]<<"\",";
	os<<"}";
	return os.str();
}

std::string CraftDefinition::dump() const
{
	std::ostringstream os(std::ios::binary);
	os<<"{output=\""<<output<<"\", input={";
	for(u32 i=0; i<input.items.size(); i++)
		os<<"\""<<input.items[i]<<"\",";
	os<<"}, (input.width="<<input.width<<")}";
	return os.str();
}

void CraftDefinition::serialize(std::ostream &os) const
{
	writeU8(os, 0); // version
	os<<serializeString(output);
	writeU8(os, input.width);
	writeU16(os, input.items.size());
	for(u32 i=0; i<input.items.size(); i++)
		os<<serializeString(input.items[i]);
}

void CraftDefinition::deSerialize(std::istream &is)
{
	int version = readU8(is);
	if(version != 0) throw SerializationError(
			"unsupported CraftDefinition version");
	output = deSerializeString(is);
	input.width = readU8(is);
	u32 count = readU16(is);
	for(u32 i=0; i<count; i++)
		input.items.push_back(deSerializeString(is));
}

class CCraftDefManager: public IWritableCraftDefManager
{
public:
	virtual ~CCraftDefManager()
	{
		clear();
	}
	virtual InventoryItem* getCraftResult(const CraftPointerInput &input_cpi,
			IGameDef *gamedef) const
	{
		if(input_cpi.width > 3){
			errorstream<<"getCraftResult(): ERROR: "
					<<"input_cpi.width > 3; Failing to craft."<<std::endl;
			return NULL;
		}
		InventoryItem *input_items[9];
		for(u32 y=0; y<3; y++)
		for(u32 x=0; x<3; x++)
		{
			u32 i=y*3+x;
			if(x >= input_cpi.width || y >= input_cpi.height())
				input_items[i] = NULL;
			else
				input_items[i] = input_cpi.items[y*input_cpi.width+x];
		}
		for(core::list<CraftDefinition*>::ConstIterator
				i = m_craft_definitions.begin();
				i != m_craft_definitions.end(); i++)
		{
			CraftDefinition *def = *i;

			/*infostream<<"Checking "<<createInput(input_cpi).dump()<<std::endl
					<<" against "<<def->input.dump()
					<<" (output=\""<<def->output<<"\")"<<std::endl;*/

			try {
				CraftPointerInput spec_cpi = createPointerInput(def->input, gamedef);
				if(spec_cpi.width > 3){
					errorstream<<"getCraftResult: ERROR: "
							<<"spec_cpi.width > 3 in recipe "
							<<def->dump()<<std::endl;
					continue;
				}
				InventoryItem *spec_items[9];
				for(u32 y=0; y<3; y++)
				for(u32 x=0; x<3; x++)
				{
					u32 i=y*3+x;
					if(x >= spec_cpi.width || y >= spec_cpi.height())
						spec_items[i] = NULL;
					else
						spec_items[i] = spec_cpi.items[y*spec_cpi.width+x];
				}

				bool match = checkItemCombination(input_items, spec_items);

				if(match){
					std::istringstream iss(def->output, std::ios::binary);
					return InventoryItem::deSerialize(iss, gamedef);
				}
			}
			catch(SerializationError &e)
			{
				errorstream<<"getCraftResult: ERROR: "
						<<"Serialization error in recipe "
						<<def->dump()<<std::endl;
				// then go on with the next craft definition
			}
		}
		return NULL;
	}
	virtual void registerCraft(const CraftDefinition &def)
	{
		infostream<<"registerCraft: registering craft definition: "
				<<def.dump()<<std::endl;
		if(def.input.width > 3 || def.input.height() > 3){
			errorstream<<"registerCraft: input size is larger than 3x3,"
					<<" ignoring"<<std::endl;
			return;
		}
		m_craft_definitions.push_back(new CraftDefinition(def));
	}
	virtual void clear()
	{
		for(core::list<CraftDefinition*>::Iterator
				i = m_craft_definitions.begin();
				i != m_craft_definitions.end(); i++){
			delete *i;
		}
		m_craft_definitions.clear();
	}
	virtual void serialize(std::ostream &os)
	{
		writeU8(os, 0); // version
		u16 count = m_craft_definitions.size();
		writeU16(os, count);
		for(core::list<CraftDefinition*>::Iterator
				i = m_craft_definitions.begin();
				i != m_craft_definitions.end(); i++){
			CraftDefinition *def = *i;
			// Serialize wrapped in a string
			std::ostringstream tmp_os(std::ios::binary);
			def->serialize(tmp_os);
			os<<serializeString(tmp_os.str());
		}
	}
	virtual void deSerialize(std::istream &is)
	{
		// Clear everything
		clear();
		// Deserialize
		int version = readU8(is);
		if(version != 0) throw SerializationError(
				"unsupported CraftDefManager version");
		u16 count = readU16(is);
		for(u16 i=0; i<count; i++){
			// Deserialize a string and grab a CraftDefinition from it
			std::istringstream tmp_is(deSerializeString(is), std::ios::binary);
			CraftDefinition def;
			def.deSerialize(tmp_is);
			// Register
			registerCraft(def);
		}
	}
private:
	core::list<CraftDefinition*> m_craft_definitions;
};

IWritableCraftDefManager* createCraftDefManager()
{
	return new CCraftDefManager();
}

