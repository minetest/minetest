/*
Minetest-c55
Copyright (C) 2010-2011 celeron55, Perttu Ahola <celeron55@gmail.com>

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

#include "inventory.h"
#include "serialization.h"
#include "utility.h"
#include "debug.h"
#include <sstream>
#include "main.h" // For tsrc, g_toolmanager
#include "serverobject.h"
#include "content_mapnode.h"
#include "content_sao.h"
#include "environment.h"
#include "mapblock.h"
#include "player.h"
#include "log.h"
#include "nodedef.h"
#include "tooldef.h"
#include "craftitemdef.h"
#include "gamedef.h"
#include "scriptapi.h"
#include "strfnd.h"
#include "nameidmapping.h" // For loading legacy MaterialItems

/*
	InventoryItem
*/

InventoryItem::InventoryItem(IGameDef *gamedef, u16 count):
	m_gamedef(gamedef),
	m_count(count)
{
	assert(m_gamedef);
}

InventoryItem::~InventoryItem()
{
}

content_t content_translate_from_19_to_internal(content_t c_from)
{
	for(u32 i=0; i<sizeof(trans_table_19)/sizeof(trans_table_19[0]); i++)
	{
		if(trans_table_19[i][1] == c_from)
		{
			return trans_table_19[i][0];
		}
	}
	return c_from;
}

InventoryItem* InventoryItem::deSerialize(std::istream &is, IGameDef *gamedef)
{
	DSTACK(__FUNCTION_NAME);

	//is.imbue(std::locale("C"));
	// Read name
	std::string name;
	std::getline(is, name, ' ');
	
	if(name == "MaterialItem")
	{
		// u16 reads directly as a number (u8 doesn't)
		u16 material;
		is>>material;
		u16 count;
		is>>count;
		// Convert old materials
		if(material <= 0xff)
			material = content_translate_from_19_to_internal(material);
		if(material > MAX_CONTENT)
			throw SerializationError("Too large material number");
		return new MaterialItem(gamedef, material, count);
	}
	else if(name == "MaterialItem2")
	{
		u16 material;
		is>>material;
		u16 count;
		is>>count;
		if(material > MAX_CONTENT)
			throw SerializationError("Too large material number");
		return new MaterialItem(gamedef, material, count);
	}
	else if(name == "node" || name == "NodeItem" || name == "MaterialItem3")
	{
		std::string all;
		std::getline(is, all, '\n');
		std::string nodename;
		// First attempt to read inside ""
		Strfnd fnd(all);
		fnd.next("\"");
		// If didn't skip to end, we have ""s
		if(!fnd.atend()){
			nodename = fnd.next("\"");
		} else { // No luck, just read a word then
			fnd.start(all);
			nodename = fnd.next(" ");
		}
		fnd.skip_over(" ");
		u16 count = stoi(trim(fnd.next("")));
		if(count == 0)
			count = 1;
		return new MaterialItem(gamedef, nodename, count);
	}
	else if(name == "MBOItem")
	{
		std::string inventorystring;
		std::getline(is, inventorystring, '|');
		throw SerializationError("MBOItem not supported anymore");
	}
	else if(name == "craft" || name == "CraftItem")
	{
		std::string all;
		std::getline(is, all, '\n');
		std::string subname;
		// First attempt to read inside ""
		Strfnd fnd(all);
		fnd.next("\"");
		// If didn't skip to end, we have ""s
		if(!fnd.atend()){
			subname = fnd.next("\"");
		} else { // No luck, just read a word then
			fnd.start(all);
			subname = fnd.next(" ");
		}
		// Then read count
		fnd.skip_over(" ");
		u16 count = stoi(trim(fnd.next("")));
		if(count == 0)
			count = 1;
		return new CraftItem(gamedef, subname, count);
	}
	else if(name == "tool" || name == "ToolItem")
	{
		std::string all;
		std::getline(is, all, '\n');
		std::string toolname;
		// First attempt to read inside ""
		Strfnd fnd(all);
		fnd.next("\"");
		// If didn't skip to end, we have ""s
		if(!fnd.atend()){
			toolname = fnd.next("\"");
		} else { // No luck, just read a word then
			fnd.start(all);
			toolname = fnd.next(" ");
		}
		// Then read wear
		fnd.skip_over(" ");
		u16 wear = stoi(trim(fnd.next("")));
		return new ToolItem(gamedef, toolname, wear);
	}
	else
	{
		infostream<<"Unknown InventoryItem name=\""<<name<<"\""<<std::endl;
		throw SerializationError("Unknown InventoryItem name");
	}
}

InventoryItem* InventoryItem::deSerialize(const std::string &str,
		IGameDef *gamedef)
{
	std::istringstream is(str, std::ios_base::binary);
	return deSerialize(is, gamedef);
}

std::string InventoryItem::getItemString() {
	// Get item string
	std::ostringstream os(std::ios_base::binary);
	serialize(os);
	return os.str();
}

bool InventoryItem::dropOrPlace(ServerEnvironment *env,
		ServerActiveObject *dropper,
		v3f pos, bool place, s16 count)
{
	/*
		Ensure that the block is loaded so that the item
		can properly be added to the static list too
	*/
	v3s16 blockpos = getNodeBlockPos(floatToInt(pos, BS));
	MapBlock *block = env->getMap().emergeBlock(blockpos, false);
	if(block==NULL)
	{
		infostream<<"InventoryItem::dropOrPlace(): FAIL: block not found: "
				<<blockpos.X<<","<<blockpos.Y<<","<<blockpos.Z
				<<std::endl;
		return false;
	}

	/*
		Take specified number of items,
		but limit to getDropCount().
	*/
	s16 dropcount = getDropCount();
	if(count < 0 || count > dropcount)
		count = dropcount;
	if(count < 0 || count > getCount())
		count = getCount();
	if(count > 0)
	{
		/*
			Create an ItemSAO
		*/
		pos.Y -= BS*0.25; // let it drop a bit
		// Randomize a bit
		//pos.X += BS*0.2*(float)myrand_range(-1000,1000)/1000.0;
		//pos.Z += BS*0.2*(float)myrand_range(-1000,1000)/1000.0;
		// Create object
		ServerActiveObject *obj = new ItemSAO(env, pos, getItemString());
		// Add the object to the environment
		env->addActiveObject(obj);
		infostream<<"Dropped item"<<std::endl;

		setCount(getCount() - count);
	}

	return getCount() < 1; // delete the item?
}

/*
	MaterialItem
*/

MaterialItem::MaterialItem(IGameDef *gamedef, std::string nodename, u16 count):
	InventoryItem(gamedef, count)
{
	if(nodename == "")
		nodename = "unknown_block";

	// Convert directly to the correct name through aliases
	m_nodename = gamedef->ndef()->getAlias(nodename);
}
// Legacy constructor
MaterialItem::MaterialItem(IGameDef *gamedef, content_t content, u16 count):
	InventoryItem(gamedef, count)
{
	NameIdMapping legacy_nimap;
	content_mapnode_get_name_id_mapping(&legacy_nimap);
	std::string nodename;
	legacy_nimap.getName(content, nodename);
	if(nodename == "")
		nodename = "unknown_block";
	m_nodename = nodename;
}

#ifndef SERVER
video::ITexture * MaterialItem::getImage() const
{
	return m_gamedef->getNodeDefManager()->get(m_nodename).inventory_texture;
}
#endif

bool MaterialItem::isCookable() const
{
	INodeDefManager *ndef = m_gamedef->ndef();
	const ContentFeatures &f = ndef->get(m_nodename);
	return (f.cookresult_item != "");
}

InventoryItem *MaterialItem::createCookResult() const
{
	INodeDefManager *ndef = m_gamedef->ndef();
	const ContentFeatures &f = ndef->get(m_nodename);
	std::istringstream is(f.cookresult_item, std::ios::binary);
	return InventoryItem::deSerialize(is, m_gamedef);
}

float MaterialItem::getCookTime() const
{
	INodeDefManager *ndef = m_gamedef->ndef();
	const ContentFeatures &f = ndef->get(m_nodename);
	return f.furnace_cooktime;
}

float MaterialItem::getBurnTime() const
{
	INodeDefManager *ndef = m_gamedef->ndef();
	const ContentFeatures &f = ndef->get(m_nodename);
	return f.furnace_burntime;
}

content_t MaterialItem::getMaterial() const
{
	INodeDefManager *ndef = m_gamedef->ndef();
	content_t id = CONTENT_IGNORE;
	ndef->getId(m_nodename, id);
	return id;
}

/*
	ToolItem
*/

ToolItem::ToolItem(IGameDef *gamedef, std::string toolname, u16 wear):
	InventoryItem(gamedef, 1)
{
	// Convert directly to the correct name through aliases
	m_toolname = gamedef->tdef()->getAlias(toolname);
	
	m_wear = wear;
}

std::string ToolItem::getImageBasename() const
{
	return m_gamedef->getToolDefManager()->getImagename(m_toolname);
}

#ifndef SERVER
video::ITexture * ToolItem::getImage() const
{
	ITextureSource *tsrc = m_gamedef->tsrc();

	std::string basename = getImageBasename();
	
	/*
		Calculate a progress value with sane amount of
		maximum states
	*/
	u32 maxprogress = 30;
	u32 toolprogress = (65535-m_wear)/(65535/maxprogress);
	
	float value_f = (float)toolprogress / (float)maxprogress;
	std::ostringstream os;
	os<<basename<<"^[progressbar"<<value_f;

	return tsrc->getTextureRaw(os.str());
}

video::ITexture * ToolItem::getImageRaw() const
{
	ITextureSource *tsrc = m_gamedef->tsrc();
	
	return tsrc->getTextureRaw(getImageBasename());
}
#endif

bool ToolItem::isKnown() const
{
	IToolDefManager *tdef = m_gamedef->tdef();
	const ToolDefinition *def = tdef->getToolDefinition(m_toolname);
	return (def != NULL);
}

/*
	CraftItem
*/

CraftItem::CraftItem(IGameDef *gamedef, std::string subname, u16 count):
	InventoryItem(gamedef, count)
{
	// Convert directly to the correct name through aliases
	m_subname = gamedef->cidef()->getAlias(subname);
}

#ifndef SERVER
video::ITexture * CraftItem::getImage() const
{
	ICraftItemDefManager *cidef = m_gamedef->cidef();
	ITextureSource *tsrc = m_gamedef->tsrc();
	std::string imagename = cidef->getImagename(m_subname);
	return tsrc->getTextureRaw(imagename);
}
#endif

bool CraftItem::isKnown() const
{
	ICraftItemDefManager *cidef = m_gamedef->cidef();
	const CraftItemDefinition *def = cidef->getCraftItemDefinition(m_subname);
	return (def != NULL);
}

u16 CraftItem::getStackMax() const
{
	ICraftItemDefManager *cidef = m_gamedef->cidef();
	const CraftItemDefinition *def = cidef->getCraftItemDefinition(m_subname);
	if(def == NULL)
		return InventoryItem::getStackMax();
	return def->stack_max;
}

bool CraftItem::isUsable() const
{
	ICraftItemDefManager *cidef = m_gamedef->cidef();
	const CraftItemDefinition *def = cidef->getCraftItemDefinition(m_subname);
	return def != NULL && def->usable;
}

bool CraftItem::isCookable() const
{
	ICraftItemDefManager *cidef = m_gamedef->cidef();
	const CraftItemDefinition *def = cidef->getCraftItemDefinition(m_subname);
	return def != NULL && def->cookresult_item != "";
}

InventoryItem *CraftItem::createCookResult() const
{
	ICraftItemDefManager *cidef = m_gamedef->cidef();
	const CraftItemDefinition *def = cidef->getCraftItemDefinition(m_subname);
	if(def == NULL)
		return InventoryItem::createCookResult();
	std::istringstream is(def->cookresult_item, std::ios::binary);
	return InventoryItem::deSerialize(is, m_gamedef);
}

float CraftItem::getCookTime() const
{
	ICraftItemDefManager *cidef = m_gamedef->cidef();
	const CraftItemDefinition *def = cidef->getCraftItemDefinition(m_subname);
	if (def == NULL)
		return InventoryItem::getCookTime();
	return def->furnace_cooktime;
}

float CraftItem::getBurnTime() const
{
	ICraftItemDefManager *cidef = m_gamedef->cidef();
	const CraftItemDefinition *def = cidef->getCraftItemDefinition(m_subname);
	if (def == NULL)
		return InventoryItem::getBurnTime();
	return def->furnace_burntime;
}

s16 CraftItem::getDropCount() const
{
	// Special cases
	ICraftItemDefManager *cidef = m_gamedef->cidef();
	const CraftItemDefinition *def = cidef->getCraftItemDefinition(m_subname);
	if(def != NULL && def->dropcount >= 0)
		return def->dropcount;
	// Default
	return InventoryItem::getDropCount();
}

bool CraftItem::areLiquidsPointable() const
{
	ICraftItemDefManager *cidef = m_gamedef->cidef();
	const CraftItemDefinition *def = cidef->getCraftItemDefinition(m_subname);
	return def != NULL && def->liquids_pointable;
}

bool CraftItem::dropOrPlace(ServerEnvironment *env,
		ServerActiveObject *dropper,
		v3f pos, bool place, s16 count)
{
	if(count == 0)
		return false;

	bool callback_exists = false;
	bool result = false;

	if(place)
	{
		result = scriptapi_craftitem_on_place_on_ground(
				env->getLua(),
				m_subname.c_str(), dropper, pos,
				callback_exists);
	}

	// note: on_drop is fallback for on_place_on_ground

	if(!callback_exists)
	{
		result = scriptapi_craftitem_on_drop(
				env->getLua(),
				m_subname.c_str(), dropper, pos,
				callback_exists);
	}

	if(callback_exists)
	{
		// If the callback returned true, drop one item
		if(result)
			setCount(getCount() - 1);
		return getCount() < 1;
	}
	else
	{
		// If neither on_place_on_ground (if place==true)
		// nor on_drop exists, call the base implementation
		return InventoryItem::dropOrPlace(env, dropper, pos, place, count);
	}
}

bool CraftItem::use(ServerEnvironment *env,
		ServerActiveObject *user,
		const PointedThing& pointed)
{
	bool callback_exists = false;
	bool result = false;

	result = scriptapi_craftitem_on_use(
			env->getLua(),
			m_subname.c_str(), user, pointed,
			callback_exists);

	if(callback_exists)
	{
		// If the callback returned true, drop one item
		if(result)
			setCount(getCount() - 1);
		return getCount() < 1;
	}
	else
	{
		// If neither on_place_on_ground (if place==true)
		// nor on_drop exists, call the base implementation
		return InventoryItem::use(env, user, pointed);
	}
}

/*
	Inventory
*/

InventoryList::InventoryList(std::string name, u32 size)
{
	m_name = name;
	m_size = size;
	clearItems();
	//m_dirty = false;
}

InventoryList::~InventoryList()
{
	for(u32 i=0; i<m_items.size(); i++)
	{
		if(m_items[i])
			delete m_items[i];
	}
}

void InventoryList::clearItems()
{
	for(u32 i=0; i<m_items.size(); i++)
	{
		if(m_items[i])
			delete m_items[i];
	}

	m_items.clear();

	for(u32 i=0; i<m_size; i++)
	{
		m_items.push_back(NULL);
	}

	//setDirty(true);
}

void InventoryList::setSize(u32 newsize)
{
	if(newsize < m_items.size()){
		for(u32 i=newsize; i<m_items.size(); i++){
			if(m_items[i])
				delete m_items[i];
		}
		m_items.erase(newsize, m_items.size() - newsize);
	} else {
		for(u32 i=m_items.size(); i<newsize; i++){
			m_items.push_back(NULL);
		}
	}
	m_size = newsize;
}

void InventoryList::serialize(std::ostream &os) const
{
	//os.imbue(std::locale("C"));
	
	for(u32 i=0; i<m_items.size(); i++)
	{
		InventoryItem *item = m_items[i];
		if(item != NULL)
		{
			os<<"Item ";
			item->serialize(os);
		}
		else
		{
			os<<"Empty";
		}
		os<<"\n";
	}

	os<<"EndInventoryList\n";
}

void InventoryList::deSerialize(std::istream &is, IGameDef *gamedef)
{
	//is.imbue(std::locale("C"));

	clearItems();
	u32 item_i = 0;

	for(;;)
	{
		std::string line;
		std::getline(is, line, '\n');

		std::istringstream iss(line);
		//iss.imbue(std::locale("C"));

		std::string name;
		std::getline(iss, name, ' ');

		if(name == "EndInventoryList")
		{
			break;
		}
		// This is a temporary backwards compatibility fix
		else if(name == "end")
		{
			break;
		}
		else if(name == "Item")
		{
			if(item_i > getSize() - 1)
				throw SerializationError("too many items");
			InventoryItem *item = InventoryItem::deSerialize(iss, gamedef);
			m_items[item_i++] = item;
		}
		else if(name == "Empty")
		{
			if(item_i > getSize() - 1)
				throw SerializationError("too many items");
			m_items[item_i++] = NULL;
		}
		else
		{
			throw SerializationError("Unknown inventory identifier");
		}
	}
}

InventoryList::InventoryList(const InventoryList &other)
{
	/*
		Do this so that the items get cloned. Otherwise the pointers
		in the array will just get copied.
	*/
	*this = other;
}

InventoryList & InventoryList::operator = (const InventoryList &other)
{
	m_name = other.m_name;
	m_size = other.m_size;
	clearItems();
	for(u32 i=0; i<other.m_items.size(); i++)
	{
		InventoryItem *item = other.m_items[i];
		if(item != NULL)
		{
			m_items[i] = item->clone();
		}
	}
	//setDirty(true);

	return *this;
}

const std::string &InventoryList::getName() const
{
	return m_name;
}

u32 InventoryList::getSize()
{
	return m_items.size();
}

u32 InventoryList::getUsedSlots()
{
	u32 num = 0;
	for(u32 i=0; i<m_items.size(); i++)
	{
		InventoryItem *item = m_items[i];
		if(item != NULL)
			num++;
	}
	return num;
}

u32 InventoryList::getFreeSlots()
{
	return getSize() - getUsedSlots();
}

const InventoryItem * InventoryList::getItem(u32 i) const
{
	if(i >= m_items.size())
		return NULL;
	return m_items[i];
}

InventoryItem * InventoryList::getItem(u32 i)
{
	if(i >= m_items.size())
		return NULL;
	return m_items[i];
}

InventoryItem * InventoryList::changeItem(u32 i, InventoryItem *newitem)
{
	if(i >= m_items.size())
		return newitem;

	InventoryItem *olditem = m_items[i];
	m_items[i] = newitem;
	//setDirty(true);
	return olditem;
}

void InventoryList::deleteItem(u32 i)
{
	assert(i < m_items.size());
	InventoryItem *item = changeItem(i, NULL);
	if(item)
		delete item;
}

InventoryItem * InventoryList::addItem(InventoryItem *newitem)
{
	if(newitem == NULL)
		return NULL;
	
	/*
		First try to find if it could be added to some existing items
	*/
	for(u32 i=0; i<m_items.size(); i++)
	{
		// Ignore empty slots
		if(m_items[i] == NULL)
			continue;
		// Try adding
		newitem = addItem(i, newitem);
		if(newitem == NULL)
			return NULL; // All was eaten
	}

	/*
		Then try to add it to empty slots
	*/
	for(u32 i=0; i<m_items.size(); i++)
	{
		// Ignore unempty slots
		if(m_items[i] != NULL)
			continue;
		// Try adding
		newitem = addItem(i, newitem);
		if(newitem == NULL)
			return NULL; // All was eaten
	}

	// Return leftover
	return newitem;
}

InventoryItem * InventoryList::addItem(u32 i, InventoryItem *newitem)
{
	if(newitem == NULL)
		return NULL;
	if(i >= m_items.size())
		return newitem;
	
	//setDirty(true);
	
	// If it is an empty position, it's an easy job.
	InventoryItem *to_item = getItem(i);
	if(to_item == NULL)
	{
		m_items[i] = newitem;
		return NULL;
	}
	
	// If not addable, return the item
	if(newitem->addableTo(to_item) == false)
		return newitem;
	
	// If the item fits fully in the slot, add counter and delete it
	if(newitem->getCount() <= to_item->freeSpace())
	{
		to_item->add(newitem->getCount());
		delete newitem;
		return NULL;
	}
	// Else the item does not fit fully. Add all that fits and return
	// the rest.
	else
	{
		u16 freespace = to_item->freeSpace();
		to_item->add(freespace);
		newitem->remove(freespace);
		return newitem;
	}
}

bool InventoryList::itemFits(const u32 i, const InventoryItem *newitem)
{
	// If it is an empty position, it's an easy job.
	const InventoryItem *to_item = getItem(i);
	if(to_item == NULL)
	{
		return true;
	}
	
	// If not addable, fail
	if(newitem->addableTo(to_item) == false)
		return false;
	
	// If the item fits fully in the slot, pass
	if(newitem->getCount() <= to_item->freeSpace())
	{
		return true;
	}

	return false;
}

bool InventoryList::roomForItem(const InventoryItem *item)
{
	for(u32 i=0; i<m_items.size(); i++)
		if(itemFits(i, item))
			return true;
	return false;
}

bool InventoryList::roomForCookedItem(const InventoryItem *item)
{
	if(!item)
		return false;
	const InventoryItem *cook = item->createCookResult();
	if(!cook)
		return false;
	bool room = roomForItem(cook);
	delete cook;
	return room;
}

InventoryItem * InventoryList::takeItem(u32 i, u32 count)
{
	if(count == 0)
		return NULL;
	
	//setDirty(true);

	InventoryItem *item = getItem(i);
	// If it is an empty position, return NULL
	if(item == NULL)
		return NULL;
	
	if(count >= item->getCount())
	{
		// Get the item by swapping NULL to its place
		return changeItem(i, NULL);
	}
	else
	{
		InventoryItem *item2 = item->clone();
		item->remove(count);
		item2->setCount(count);
		return item2;
	}
	
	return false;
}

void InventoryList::decrementMaterials(u16 count)
{
	for(u32 i=0; i<m_items.size(); i++)
	{
		InventoryItem *item = takeItem(i, count);
		if(item)
			delete item;
	}
}

void InventoryList::print(std::ostream &o)
{
	o<<"InventoryList:"<<std::endl;
	for(u32 i=0; i<m_items.size(); i++)
	{
		InventoryItem *item = m_items[i];
		if(item != NULL)
		{
			o<<i<<": ";
			item->serialize(o);
			o<<"\n";
		}
	}
}

/*
	Inventory
*/

Inventory::~Inventory()
{
	clear();
}

void Inventory::clear()
{
	for(u32 i=0; i<m_lists.size(); i++)
	{
		delete m_lists[i];
	}
	m_lists.clear();
}

Inventory::Inventory()
{
}

Inventory::Inventory(const Inventory &other)
{
	*this = other;
}

Inventory & Inventory::operator = (const Inventory &other)
{
	clear();
	for(u32 i=0; i<other.m_lists.size(); i++)
	{
		m_lists.push_back(new InventoryList(*other.m_lists[i]));
	}
	return *this;
}

void Inventory::serialize(std::ostream &os) const
{
	for(u32 i=0; i<m_lists.size(); i++)
	{
		InventoryList *list = m_lists[i];
		os<<"List "<<list->getName()<<" "<<list->getSize()<<"\n";
		list->serialize(os);
	}

	os<<"EndInventory\n";
}

void Inventory::deSerialize(std::istream &is, IGameDef *gamedef)
{
	clear();

	for(;;)
	{
		std::string line;
		std::getline(is, line, '\n');

		std::istringstream iss(line);

		std::string name;
		std::getline(iss, name, ' ');

		if(name == "EndInventory")
		{
			break;
		}
		// This is a temporary backwards compatibility fix
		else if(name == "end")
		{
			break;
		}
		else if(name == "List")
		{
			std::string listname;
			u32 listsize;

			std::getline(iss, listname, ' ');
			iss>>listsize;

			InventoryList *list = new InventoryList(listname, listsize);
			list->deSerialize(is, gamedef);

			m_lists.push_back(list);
		}
		else
		{
			throw SerializationError("Unknown inventory identifier");
		}
	}
}

InventoryList * Inventory::addList(const std::string &name, u32 size)
{
	s32 i = getListIndex(name);
	if(i != -1)
	{
		if(m_lists[i]->getSize() != size)
		{
			delete m_lists[i];
			m_lists[i] = new InventoryList(name, size);
		}
		return m_lists[i];
	}
	else
	{
		m_lists.push_back(new InventoryList(name, size));
		return m_lists.getLast();
	}
}

InventoryList * Inventory::getList(const std::string &name)
{
	s32 i = getListIndex(name);
	if(i == -1)
		return NULL;
	return m_lists[i];
}

bool Inventory::deleteList(const std::string &name)
{
	s32 i = getListIndex(name);
	if(i == -1)
		return false;
	delete m_lists[i];
	m_lists.erase(i);
	return true;
}

const InventoryList * Inventory::getList(const std::string &name) const
{
	s32 i = getListIndex(name);
	if(i == -1)
		return NULL;
	return m_lists[i];
}

const s32 Inventory::getListIndex(const std::string &name) const
{
	for(u32 i=0; i<m_lists.size(); i++)
	{
		if(m_lists[i]->getName() == name)
			return i;
	}
	return -1;
}

//END
