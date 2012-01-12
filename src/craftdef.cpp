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
#include <set>
#include "utility.h"
#include "gamedef.h"
#include "inventory.h"


// Deserialize an itemstring then return the name of the item
static std::string craftGetItemName(const std::string &itemstring, IGameDef *gamedef)
{
	ItemStack item;
	item.deSerialize(itemstring, gamedef->idef());
	return item.name;
}

// (mapcar craftGetItemName itemstrings)
static std::vector<std::string> craftGetItemNames(
		const std::vector<std::string> &itemstrings, IGameDef *gamedef)
{
	std::vector<std::string> result;
	for(std::vector<std::string>::const_iterator
			i = itemstrings.begin();
			i != itemstrings.end(); i++)
	{
		result.push_back(craftGetItemName(*i, gamedef));
	}
	return result;
}

// Get name of each item, and return them as a new list.
static std::vector<std::string> craftGetItemNames(
		const std::vector<ItemStack> &items, IGameDef *gamedef)
{
	std::vector<std::string> result;
	for(std::vector<ItemStack>::const_iterator
			i = items.begin();
			i != items.end(); i++)
	{
		result.push_back(i->name);
	}
	return result;
}

// Compute bounding rectangle given a matrix of items
// Returns false if every item is ""
static bool craftGetBounds(const std::vector<std::string> &items, unsigned int width,
		unsigned int &min_x, unsigned int &max_x,
		unsigned int &min_y, unsigned int &max_y)
{
	bool success = false;
	unsigned int x = 0;
	unsigned int y = 0;
	for(std::vector<std::string>::const_iterator
			i = items.begin();
			i != items.end(); i++)
	{
		if(*i != "")  // Is this an actual item?
		{
			if(!success)
			{
				// This is the first nonempty item
				min_x = max_x = x;
				min_y = max_y = y;
				success = true;
			}
			else
			{
				if(x < min_x) min_x = x;
				if(x > max_x) max_x = x;
				if(y < min_y) min_y = y;
				if(y > max_y) max_y = y;
			}
		}

		// Step coordinate
		x++;
		if(x == width)
		{
			x = 0;
			y++;
		}
	}
	return success;
}

// Convert a list of item names to a multiset
static std::multiset<std::string> craftMakeMultiset(const std::vector<std::string> &names)
{
	std::multiset<std::string> set;
	for(std::vector<std::string>::const_iterator
			i = names.begin();
			i != names.end(); i++)
	{
		if(*i != "")
			set.insert(*i);
	}
	return set;
}

// Removes 1 from each item stack
static void craftDecrementInput(CraftInput &input, IGameDef *gamedef)
{
	for(std::vector<ItemStack>::iterator
			i = input.items.begin();
			i != input.items.end(); i++)
	{
		if(i->count != 0)
			i->remove(1);
	}
}

// Removes 1 from each item stack with replacement support
// Example: if replacements contains the pair ("bucket:bucket_water", "bucket:bucket_empty"),
//   a water bucket will not be removed but replaced by an empty bucket.
static void craftDecrementOrReplaceInput(CraftInput &input,
		const CraftReplacements &replacements,
		IGameDef *gamedef)
{
	if(replacements.pairs.empty())
	{
		craftDecrementInput(input, gamedef);
		return;
	}

	// Make a copy of the replacements pair list
	std::vector<std::pair<std::string, std::string> > pairs = replacements.pairs;

	for(std::vector<ItemStack>::iterator
			i = input.items.begin();
			i != input.items.end(); i++)
	{
		if(i->count == 1)
		{
			// Find an appropriate replacement
			bool found_replacement = false;
			for(std::vector<std::pair<std::string, std::string> >::iterator
					j = pairs.begin();
					j != pairs.end(); j++)
			{
				ItemStack from_item;
				from_item.deSerialize(j->first, gamedef->idef());
				if(i->name == from_item.name)
				{
					i->deSerialize(j->second, gamedef->idef());
					found_replacement = true;
					pairs.erase(j);
					break;
				}
			}
			// No replacement was found, simply decrement count to zero
			if(!found_replacement)
				i->remove(1);
		}
		else if(i->count >= 2)
		{
			// Ignore replacements for items with count >= 2
			i->remove(1);
		}
	}
}

// Dump an itemstring matrix
static std::string craftDumpMatrix(const std::vector<std::string> &items,
		unsigned int width)
{
	std::ostringstream os(std::ios::binary);
	os<<"{ ";
	unsigned int x = 0;
	for(std::vector<std::string>::const_iterator
			i = items.begin();
			i != items.end(); i++, x++)
	{
		if(x == width)
		{
			os<<"; ";
			x = 0;
		}
		else if(x != 0)
		{
			os<<",";
		}
		os<<"\""<<(*i)<<"\"";
	}
	os<<" }";
	return os.str();
}

// Dump an item matrix
std::string craftDumpMatrix(const std::vector<ItemStack> &items,
		unsigned int width)
{
	std::ostringstream os(std::ios::binary);
	os<<"{ ";
	unsigned int x = 0;
	for(std::vector<ItemStack>::const_iterator
			i = items.begin();
			i != items.end(); i++, x++)
	{
		if(x == width)
		{
			os<<"; ";
			x = 0;
		}
		else if(x != 0)
		{
			os<<",";
		}
		os<<"\""<<(i->getItemString())<<"\"";
	}
	os<<" }";
	return os.str();
}


/*
	CraftInput
*/

std::string CraftInput::dump() const
{
	std::ostringstream os(std::ios::binary);
	os<<"(method="<<((int)method)<<", items="<<craftDumpMatrix(items, width)<<")";
	return os.str();
}

/*
	CraftOutput
*/

std::string CraftOutput::dump() const
{
	std::ostringstream os(std::ios::binary);
	os<<"(item=\""<<item<<"\", time="<<time<<")";
	return os.str();
}

/*
	CraftReplacements
*/
std::string CraftReplacements::dump() const
{
	std::ostringstream os(std::ios::binary);
	os<<"{";
	const char *sep = "";
	for(std::vector<std::pair<std::string, std::string> >::const_iterator
			i = pairs.begin();
			i != pairs.end(); i++)
	{
		os<<sep<<"\""<<(i->first)<<"\"=>\""<<(i->second)<<"\"";
		sep = ",";
	}
	os<<"}";
	return os.str();
}


/*
	CraftDefinition
*/

void CraftDefinition::serialize(std::ostream &os) const
{
	writeU8(os, 1); // version
	os<<serializeString(getName());
	serializeBody(os);
}

CraftDefinition* CraftDefinition::deSerialize(std::istream &is)
{
	int version = readU8(is);
	if(version != 1) throw SerializationError(
			"unsupported CraftDefinition version");
	std::string name = deSerializeString(is);
	CraftDefinition *def = NULL;
	if(name == "shaped")
	{
		def = new CraftDefinitionShaped;
	}
	else if(name == "shapeless")
	{
		def = new CraftDefinitionShapeless;
	}
	else if(name == "toolrepair")
	{
		def = new CraftDefinitionToolRepair;
	}
	else if(name == "cooking")
	{
		def = new CraftDefinitionCooking;
	}
	else if(name == "fuel")
	{
		def = new CraftDefinitionFuel;
	}
	else
	{
		infostream<<"Unknown CraftDefinition name=\""<<name<<"\""<<std::endl;
                throw SerializationError("Unknown CraftDefinition name");
	}
	def->deSerializeBody(is, version);
	return def;
}

/*
	CraftDefinitionShaped
*/

std::string CraftDefinitionShaped::getName() const
{
	return "shaped";
}

bool CraftDefinitionShaped::check(const CraftInput &input, IGameDef *gamedef) const
{
	if(input.method != CRAFT_METHOD_NORMAL)
		return false;

	// Get input item matrix
	std::vector<std::string> inp_names = craftGetItemNames(input.items, gamedef);
	unsigned int inp_width = input.width;
	if(inp_width == 0)
		return false;
	while(inp_names.size() % inp_width != 0)
		inp_names.push_back("");

	// Get input bounds
	unsigned int inp_min_x=0, inp_max_x=0, inp_min_y=0, inp_max_y=0;
	if(!craftGetBounds(inp_names, inp_width, inp_min_x, inp_max_x, inp_min_y, inp_max_y))
		return false;  // it was empty

	// Get recipe item matrix
	std::vector<std::string> rec_names = craftGetItemNames(recipe, gamedef);
	unsigned int rec_width = width;
	if(rec_width == 0)
		return false;
	while(rec_names.size() % rec_width != 0)
		rec_names.push_back("");

	// Get recipe bounds
	unsigned int rec_min_x=0, rec_max_x=0, rec_min_y=0, rec_max_y=0;
	if(!craftGetBounds(rec_names, rec_width, rec_min_x, rec_max_x, rec_min_y, rec_max_y))
		return false;  // it was empty

	// Different sizes?
	if(inp_max_x - inp_min_x != rec_max_x - rec_min_x)
		return false;
	if(inp_max_y - inp_min_y != rec_max_y - rec_min_y)
		return false;

	// Verify that all item names in the bounding box are equal
	unsigned int w = inp_max_x - inp_min_x + 1;
	unsigned int h = inp_max_y - inp_min_y + 1;
	for(unsigned int y=0; y<h; y++)
	for(unsigned int x=0; x<w; x++)
	{
		unsigned int inp_x = inp_min_x + x;
		unsigned int inp_y = inp_min_y + y;
		unsigned int rec_x = rec_min_x + x;
		unsigned int rec_y = rec_min_y + y;

		if(
			inp_names[inp_y * inp_width + inp_x] !=
			rec_names[rec_y * rec_width + rec_x]
		){
			return false;
		}
	}

	return true;
}

CraftOutput CraftDefinitionShaped::getOutput(const CraftInput &input, IGameDef *gamedef) const
{
	return CraftOutput(output, 0);
}

void CraftDefinitionShaped::decrementInput(CraftInput &input, IGameDef *gamedef) const
{
	craftDecrementOrReplaceInput(input, replacements, gamedef);
}

std::string CraftDefinitionShaped::dump() const
{
	std::ostringstream os(std::ios::binary);
	os<<"(shaped, output=\""<<output
		<<"\", recipe="<<craftDumpMatrix(recipe, width)
		<<", replacements="<<replacements.dump()<<")";
	return os.str();
}

void CraftDefinitionShaped::serializeBody(std::ostream &os) const
{
	os<<serializeString(output);
	writeU16(os, width);
	writeU16(os, recipe.size());
	for(u32 i=0; i<recipe.size(); i++)
		os<<serializeString(recipe[i]);
	writeU16(os, replacements.pairs.size());
	for(u32 i=0; i<replacements.pairs.size(); i++)
	{
		os<<serializeString(replacements.pairs[i].first);
		os<<serializeString(replacements.pairs[i].second);
	}
}

void CraftDefinitionShaped::deSerializeBody(std::istream &is, int version)
{
	if(version != 1) throw SerializationError(
			"unsupported CraftDefinitionShaped version");
	output = deSerializeString(is);
	width = readU16(is);
	recipe.clear();
	u32 count = readU16(is);
	for(u32 i=0; i<count; i++)
		recipe.push_back(deSerializeString(is));
	replacements.pairs.clear();
	count = readU16(is);
	for(u32 i=0; i<count; i++)
	{
		std::string first = deSerializeString(is);
		std::string second = deSerializeString(is);
		replacements.pairs.push_back(std::make_pair(first, second));
	}
}

/*
	CraftDefinitionShapeless
*/

std::string CraftDefinitionShapeless::getName() const
{
	return "shapeless";
}

bool CraftDefinitionShapeless::check(const CraftInput &input, IGameDef *gamedef) const
{
	if(input.method != CRAFT_METHOD_NORMAL)
		return false;

	// Get input item multiset
	std::vector<std::string> inp_names = craftGetItemNames(input.items, gamedef);
	std::multiset<std::string> inp_names_multiset = craftMakeMultiset(inp_names);

	// Get recipe item multiset
	std::vector<std::string> rec_names = craftGetItemNames(recipe, gamedef);
	std::multiset<std::string> rec_names_multiset = craftMakeMultiset(rec_names);

	// Recipe is matched when the multisets coincide
	return inp_names_multiset == rec_names_multiset;
}

CraftOutput CraftDefinitionShapeless::getOutput(const CraftInput &input, IGameDef *gamedef) const
{
	return CraftOutput(output, 0);
}

void CraftDefinitionShapeless::decrementInput(CraftInput &input, IGameDef *gamedef) const
{
	craftDecrementOrReplaceInput(input, replacements, gamedef);
}

std::string CraftDefinitionShapeless::dump() const
{
	std::ostringstream os(std::ios::binary);
	os<<"(shapeless, output=\""<<output
		<<"\", recipe="<<craftDumpMatrix(recipe, recipe.size())
		<<", replacements="<<replacements.dump()<<")";
	return os.str();
}

void CraftDefinitionShapeless::serializeBody(std::ostream &os) const
{
	os<<serializeString(output);
	writeU16(os, recipe.size());
	for(u32 i=0; i<recipe.size(); i++)
		os<<serializeString(recipe[i]);
	writeU16(os, replacements.pairs.size());
	for(u32 i=0; i<replacements.pairs.size(); i++)
	{
		os<<serializeString(replacements.pairs[i].first);
		os<<serializeString(replacements.pairs[i].second);
	}
}

void CraftDefinitionShapeless::deSerializeBody(std::istream &is, int version)
{
	if(version != 1) throw SerializationError(
			"unsupported CraftDefinitionShapeless version");
	output = deSerializeString(is);
	recipe.clear();
	u32 count = readU16(is);
	for(u32 i=0; i<count; i++)
		recipe.push_back(deSerializeString(is));
	replacements.pairs.clear();
	count = readU16(is);
	for(u32 i=0; i<count; i++)
	{
		std::string first = deSerializeString(is);
		std::string second = deSerializeString(is);
		replacements.pairs.push_back(std::make_pair(first, second));
	}
}

/*
	CraftDefinitionToolRepair
*/

static ItemStack craftToolRepair(
		const ItemStack &item1,
		const ItemStack &item2,
		float additional_wear,
		IGameDef *gamedef)
{
	IItemDefManager *idef = gamedef->idef();
	if(item1.count != 1 || item2.count != 1 || item1.name != item2.name
			|| idef->get(item1.name).type != ITEM_TOOL
			|| idef->get(item2.name).type != ITEM_TOOL)
	{
		// Failure
		return ItemStack();
	}

	s32 item1_uses = 65536 - (u32) item1.wear;
	s32 item2_uses = 65536 - (u32) item2.wear;
	s32 new_uses = item1_uses + item2_uses;
	s32 new_wear = 65536 - new_uses + floor(additional_wear * 65536 + 0.5);
	if(new_wear >= 65536)
		return ItemStack();
	if(new_wear < 0)
		new_wear = 0;

	ItemStack repaired = item1;
	repaired.wear = new_wear;
	return repaired;
}

std::string CraftDefinitionToolRepair::getName() const
{
	return "toolrepair";
}

bool CraftDefinitionToolRepair::check(const CraftInput &input, IGameDef *gamedef) const
{
	if(input.method != CRAFT_METHOD_NORMAL)
		return false;

	ItemStack item1;
	ItemStack item2;
	for(std::vector<ItemStack>::const_iterator
			i = input.items.begin();
			i != input.items.end(); i++)
	{
		if(!i->empty())
		{
			if(item1.empty())
				item1 = *i;
			else if(item2.empty())
				item2 = *i;
			else
				return false;
		}
	}
	ItemStack repaired = craftToolRepair(item1, item2, additional_wear, gamedef);
	return !repaired.empty();
}

CraftOutput CraftDefinitionToolRepair::getOutput(const CraftInput &input, IGameDef *gamedef) const
{
	ItemStack item1;
	ItemStack item2;
	for(std::vector<ItemStack>::const_iterator
			i = input.items.begin();
			i != input.items.end(); i++)
	{
		if(!i->empty())
		{
			if(item1.empty())
				item1 = *i;
			else if(item2.empty())
				item2 = *i;
		}
	}
	ItemStack repaired = craftToolRepair(item1, item2, additional_wear, gamedef);
	return CraftOutput(repaired.getItemString(), 0);
}

void CraftDefinitionToolRepair::decrementInput(CraftInput &input, IGameDef *gamedef) const
{
	craftDecrementInput(input, gamedef);
}

std::string CraftDefinitionToolRepair::dump() const
{
	std::ostringstream os(std::ios::binary);
	os<<"(toolrepair, additional_wear="<<additional_wear<<")";
	return os.str();
}

void CraftDefinitionToolRepair::serializeBody(std::ostream &os) const
{
	writeF1000(os, additional_wear);
}

void CraftDefinitionToolRepair::deSerializeBody(std::istream &is, int version)
{
	if(version != 1) throw SerializationError(
			"unsupported CraftDefinitionToolRepair version");
	additional_wear = readF1000(is);
}

/*
	CraftDefinitionCooking
*/

std::string CraftDefinitionCooking::getName() const
{
	return "cooking";
}

bool CraftDefinitionCooking::check(const CraftInput &input, IGameDef *gamedef) const
{
	if(input.method != CRAFT_METHOD_COOKING)
		return false;

	// Get input item multiset
	std::vector<std::string> inp_names = craftGetItemNames(input.items, gamedef);
	std::multiset<std::string> inp_names_multiset = craftMakeMultiset(inp_names);

	// Get recipe item multiset
	std::multiset<std::string> rec_names_multiset;
	rec_names_multiset.insert(craftGetItemName(recipe, gamedef));

	// Recipe is matched when the multisets coincide
	return inp_names_multiset == rec_names_multiset;
}

CraftOutput CraftDefinitionCooking::getOutput(const CraftInput &input, IGameDef *gamedef) const
{
	return CraftOutput(output, cooktime);
}

void CraftDefinitionCooking::decrementInput(CraftInput &input, IGameDef *gamedef) const
{
	craftDecrementInput(input, gamedef);
}

std::string CraftDefinitionCooking::dump() const
{
	std::ostringstream os(std::ios::binary);
	os<<"(cooking, output=\""<<output
		<<"\", recipe=\""<<recipe
		<<"\", cooktime="<<cooktime<<")";
	return os.str();
}

void CraftDefinitionCooking::serializeBody(std::ostream &os) const
{
	os<<serializeString(output);
	os<<serializeString(recipe);
	writeF1000(os, cooktime);
}

void CraftDefinitionCooking::deSerializeBody(std::istream &is, int version)
{
	if(version != 1) throw SerializationError(
			"unsupported CraftDefinitionCooking version");
	output = deSerializeString(is);
	recipe = deSerializeString(is);
	cooktime = readF1000(is);
}

/*
	CraftDefinitionFuel
*/

std::string CraftDefinitionFuel::getName() const
{
	return "fuel";
}

bool CraftDefinitionFuel::check(const CraftInput &input, IGameDef *gamedef) const
{
	if(input.method != CRAFT_METHOD_FUEL)
		return false;

	// Get input item multiset
	std::vector<std::string> inp_names = craftGetItemNames(input.items, gamedef);
	std::multiset<std::string> inp_names_multiset = craftMakeMultiset(inp_names);

	// Get recipe item multiset
	std::multiset<std::string> rec_names_multiset;
	rec_names_multiset.insert(craftGetItemName(recipe, gamedef));

	// Recipe is matched when the multisets coincide
	return inp_names_multiset == rec_names_multiset;
}

CraftOutput CraftDefinitionFuel::getOutput(const CraftInput &input, IGameDef *gamedef) const
{
	return CraftOutput("", burntime);
}

void CraftDefinitionFuel::decrementInput(CraftInput &input, IGameDef *gamedef) const
{
	craftDecrementInput(input, gamedef);
}

std::string CraftDefinitionFuel::dump() const
{
	std::ostringstream os(std::ios::binary);
	os<<"(fuel, recipe=\""<<recipe
		<<"\", burntime="<<burntime<<")";
	return os.str();
}

void CraftDefinitionFuel::serializeBody(std::ostream &os) const
{
	os<<serializeString(recipe);
	writeF1000(os, burntime);
}

void CraftDefinitionFuel::deSerializeBody(std::istream &is, int version)
{
	if(version != 1) throw SerializationError(
			"unsupported CraftDefinitionFuel version");
	recipe = deSerializeString(is);
	burntime = readF1000(is);
}

/*
	Craft definition manager
*/

class CCraftDefManager: public IWritableCraftDefManager
{
public:
	virtual ~CCraftDefManager()
	{
		clear();
	}
	virtual bool getCraftResult(CraftInput &input, CraftOutput &output,
			bool decrementInput, IGameDef *gamedef) const
	{
		output.item = "";
		output.time = 0;

		// If all input items are empty, abort.
		bool all_empty = true;
		for(std::vector<ItemStack>::const_iterator
				i = input.items.begin();
				i != input.items.end(); i++)
		{
			if(!i->empty())
			{
				all_empty = false;
				break;
			}
		}
		if(all_empty)
			return false;

		// Walk crafting definitions from back to front, so that later
		// definitions can override earlier ones.
		for(std::vector<CraftDefinition*>::const_reverse_iterator
				i = m_craft_definitions.rbegin();
				i != m_craft_definitions.rend(); i++)
		{
			CraftDefinition *def = *i;

			/*infostream<<"Checking "<<input.dump()<<std::endl
					<<" against "<<def->dump()<<std::endl;*/

			try {
				if(def->check(input, gamedef))
				{
					// Get output, then decrement input (if requested)
					output = def->getOutput(input, gamedef);
					if(decrementInput)
						def->decrementInput(input, gamedef);
					return true;
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
		return false;
	}
	virtual std::string dump() const
	{
		std::ostringstream os(std::ios::binary);
		os<<"Crafting definitions:\n";
		for(std::vector<CraftDefinition*>::const_iterator
				i = m_craft_definitions.begin();
				i != m_craft_definitions.end(); i++)
		{
			os<<(*i)->dump()<<"\n";
		}
		return os.str();
	}
	virtual void registerCraft(CraftDefinition *def)
	{
		infostream<<"registerCraft: registering craft definition: "
				<<def->dump()<<std::endl;
		m_craft_definitions.push_back(def);
	}
	virtual void clear()
	{
		for(std::vector<CraftDefinition*>::iterator
				i = m_craft_definitions.begin();
				i != m_craft_definitions.end(); i++){
			delete *i;
		}
		m_craft_definitions.clear();
	}
	virtual void serialize(std::ostream &os) const
	{
		writeU8(os, 0); // version
		u16 count = m_craft_definitions.size();
		writeU16(os, count);
		for(std::vector<CraftDefinition*>::const_iterator
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
			CraftDefinition *def = CraftDefinition::deSerialize(tmp_is);
			// Register
			registerCraft(def);
		}
	}
private:
	std::vector<CraftDefinition*> m_craft_definitions;
};

IWritableCraftDefManager* createCraftDefManager()
{
	return new CCraftDefManager();
}

