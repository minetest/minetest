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
#include "serverremoteplayer.h"

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
	m_nodename = nodename;
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

/*
	InventoryAction
*/

InventoryAction * InventoryAction::deSerialize(std::istream &is)
{
	std::string type;
	std::getline(is, type, ' ');

	InventoryAction *a = NULL;

	if(type == "Move")
	{
		a = new IMoveAction(is);
	}
	else if(type == "Drop")
	{
		a = new IDropAction(is);
	}

	return a;
}

static std::string describeC(const struct InventoryContext *c)
{
	if(c->current_player == NULL)
		return "current_player=NULL";
	else
		return std::string("current_player=") + c->current_player->getName();
}

IMoveAction::IMoveAction(std::istream &is)
{
	std::string ts;

	std::getline(is, ts, ' ');
	count = stoi(ts);

	std::getline(is, from_inv, ' ');

	std::getline(is, from_list, ' ');

	std::getline(is, ts, ' ');
	from_i = stoi(ts);

	std::getline(is, to_inv, ' ');

	std::getline(is, to_list, ' ');

	std::getline(is, ts, ' ');
	to_i = stoi(ts);
}

void IMoveAction::apply(InventoryContext *c, InventoryManager *mgr,
		ServerEnvironment *env)
{
	Inventory *inv_from = mgr->getInventory(c, from_inv);
	Inventory *inv_to = mgr->getInventory(c, to_inv);
	
	if(!inv_from){
		infostream<<"IMoveAction::apply(): FAIL: source inventory not found: "
				<<"context=["<<describeC(c)<<"], from_inv=\""<<from_inv<<"\""
				<<", to_inv=\""<<to_inv<<"\""<<std::endl;
		return;
	}
	if(!inv_to){
		infostream<<"IMoveAction::apply(): FAIL: destination inventory not found: "
				"context=["<<describeC(c)<<"], from_inv=\""<<from_inv<<"\""
				<<", to_inv=\""<<to_inv<<"\""<<std::endl;
		return;
	}

	InventoryList *list_from = inv_from->getList(from_list);
	InventoryList *list_to = inv_to->getList(to_list);

	/*
		If a list doesn't exist or the source item doesn't exist
	*/
	if(!list_from){
		infostream<<"IMoveAction::apply(): FAIL: source list not found: "
				<<"context=["<<describeC(c)<<"], from_inv=\""<<from_inv<<"\""
				<<", from_list=\""<<from_list<<"\""<<std::endl;
		return;
	}
	if(!list_to){
		infostream<<"IMoveAction::apply(): FAIL: destination list not found: "
				<<"context=["<<describeC(c)<<"], to_inv=\""<<to_inv<<"\""
				<<", to_list=\""<<to_list<<"\""<<std::endl;
		return;
	}
	if(list_from->getItem(from_i) == NULL)
	{
		infostream<<"IMoveAction::apply(): FAIL: source item not found: "
				<<"context=["<<describeC(c)<<"], from_inv=\""<<from_inv<<"\""
				<<", from_list=\""<<from_list<<"\""
				<<" from_i="<<from_i<<std::endl;
		return;
	}
	/*
		If the source and the destination slots are the same
	*/
	if(inv_from == inv_to && list_from == list_to && from_i == to_i)
	{
		infostream<<"IMoveAction::apply(): FAIL: source and destination slots "
				<<"are the same: inv=\""<<from_inv<<"\" list=\""<<from_list
				<<"\" i="<<from_i<<std::endl;
		return;
	}
	
	// Take item from source list
	InventoryItem *item1 = NULL;
	if(count == 0)
		item1 = list_from->changeItem(from_i, NULL);
	else
		item1 = list_from->takeItem(from_i, count);

	// Try to add the item to destination list
	InventoryItem *olditem = item1;
	item1 = list_to->addItem(to_i, item1);

	// If something is returned, the item was not fully added
	if(item1 != NULL)
	{
		// If olditem is returned, nothing was added.
		bool nothing_added = (item1 == olditem);
		
		// If something else is returned, part of the item was left unadded.
		// Add the other part back to the source item
		list_from->addItem(from_i, item1);

		// If olditem is returned, nothing was added.
		// Swap the items
		if(nothing_added)
		{
			// Take item from source list
			item1 = list_from->changeItem(from_i, NULL);
			// Adding was not possible, swap the items.
			InventoryItem *item2 = list_to->changeItem(to_i, item1);
			// Put item from destination list to the source list
			list_from->changeItem(from_i, item2);
		}
	}

	mgr->inventoryModified(c, from_inv);
	if(from_inv != to_inv)
		mgr->inventoryModified(c, to_inv);
	
	infostream<<"IMoveAction::apply(): moved at "
			<<"["<<describeC(c)<<"]"
			<<" from inv=\""<<from_inv<<"\""
			<<" list=\""<<from_list<<"\""
			<<" i="<<from_i
			<<" to inv=\""<<to_inv<<"\""
			<<" list=\""<<to_list<<"\""
			<<" i="<<to_i
			<<std::endl;
}

IDropAction::IDropAction(std::istream &is)
{
	std::string ts;

	std::getline(is, ts, ' ');
	count = stoi(ts);

	std::getline(is, from_inv, ' ');

	std::getline(is, from_list, ' ');

	std::getline(is, ts, ' ');
	from_i = stoi(ts);
}

void IDropAction::apply(InventoryContext *c, InventoryManager *mgr,
		ServerEnvironment *env)
{
	if(c->current_player == NULL){
		infostream<<"IDropAction::apply(): FAIL: current_player is NULL"<<std::endl;
		return;
	}

	// Do NOT cast directly to ServerActiveObject*, it breaks
	// because of multiple inheritance.
	ServerActiveObject *dropper =
		static_cast<ServerActiveObject*>(
		static_cast<ServerRemotePlayer*>(
			c->current_player
		));

	Inventory *inv_from = mgr->getInventory(c, from_inv);
	
	if(!inv_from){
		infostream<<"IDropAction::apply(): FAIL: source inventory not found: "
				<<"context=["<<describeC(c)<<"], from_inv=\""<<from_inv<<"\""<<std::endl;
		return;
	}

	InventoryList *list_from = inv_from->getList(from_list);

	/*
		If a list doesn't exist or the source item doesn't exist
	*/
	if(!list_from){
		infostream<<"IDropAction::apply(): FAIL: source list not found: "
				<<"context=["<<describeC(c)<<"], from_inv=\""<<from_inv<<"\""
				<<", from_list=\""<<from_list<<"\""<<std::endl;
		return;
	}
	InventoryItem *item = list_from->getItem(from_i);
	if(item == NULL)
	{
		infostream<<"IDropAction::apply(): FAIL: source item not found: "
				<<"context=["<<describeC(c)<<"], from_inv=\""<<from_inv<<"\""
				<<", from_list=\""<<from_list<<"\""
				<<" from_i="<<from_i<<std::endl;
		return;
	}

	v3f pos = dropper->getBasePosition();
	pos.Y += 0.5*BS;

	s16 count2 = count;
	if(count2 == 0)
		count2 = -1;

	/*
		Drop the item
	*/
	bool remove = item->dropOrPlace(env, dropper, pos, false, count2);
	if(remove)
		list_from->deleteItem(from_i);

	mgr->inventoryModified(c, from_inv);

	infostream<<"IDropAction::apply(): dropped "
			<<"["<<describeC(c)<<"]"
			<<" from inv=\""<<from_inv<<"\""
			<<" list=\""<<from_list<<"\""
			<<" i="<<from_i
			<<std::endl;
}

/*
	Craft checking system
*/

bool ItemSpec::checkItem(const InventoryItem *item) const
{
	if(type == ITEM_NONE)
	{
		// Has to be no item
		if(item != NULL)
			return false;
		return true;
	}
	
	// There should be an item
	if(item == NULL)
		return false;

	std::string itemname = item->getName();

	if(type == ITEM_MATERIAL)
	{
		if(itemname != "MaterialItem")
			return false;
		MaterialItem *mitem = (MaterialItem*)item;
		if(num != 65535){
			if(mitem->getMaterial() != num)
				return false;
		} else {
			if(mitem->getNodeName() != name)
				return false;
		}
	}
	else if(type == ITEM_CRAFT)
	{
		if(itemname != "CraftItem")
			return false;
		CraftItem *mitem = (CraftItem*)item;
		if(mitem->getSubName() != name)
			return false;
	}
	else if(type == ITEM_TOOL)
	{
		// Not supported yet
		assert(0);
	}
	else if(type == ITEM_MBO)
	{
		// Not supported yet
		assert(0);
	}
	else
	{
		// Not supported yet
		assert(0);
	}
	return true;
}

bool checkItemCombination(InventoryItem const * const *items, const ItemSpec *specs)
{
	u16 items_min_x = 100;
	u16 items_max_x = 100;
	u16 items_min_y = 100;
	u16 items_max_y = 100;
	for(u16 y=0; y<3; y++)
	for(u16 x=0; x<3; x++)
	{
		if(items[y*3 + x] == NULL)
			continue;
		if(items_min_x == 100 || x < items_min_x)
			items_min_x = x;
		if(items_min_y == 100 || y < items_min_y)
			items_min_y = y;
		if(items_max_x == 100 || x > items_max_x)
			items_max_x = x;
		if(items_max_y == 100 || y > items_max_y)
			items_max_y = y;
	}
	// No items at all, just return false
	if(items_min_x == 100)
		return false;
	
	u16 items_w = items_max_x - items_min_x + 1;
	u16 items_h = items_max_y - items_min_y + 1;

	u16 specs_min_x = 100;
	u16 specs_max_x = 100;
	u16 specs_min_y = 100;
	u16 specs_max_y = 100;
	for(u16 y=0; y<3; y++)
	for(u16 x=0; x<3; x++)
	{
		if(specs[y*3 + x].type == ITEM_NONE)
			continue;
		if(specs_min_x == 100 || x < specs_min_x)
			specs_min_x = x;
		if(specs_min_y == 100 || y < specs_min_y)
			specs_min_y = y;
		if(specs_max_x == 100 || x > specs_max_x)
			specs_max_x = x;
		if(specs_max_y == 100 || y > specs_max_y)
			specs_max_y = y;
	}
	// No specs at all, just return false
	if(specs_min_x == 100)
		return false;

	u16 specs_w = specs_max_x - specs_min_x + 1;
	u16 specs_h = specs_max_y - specs_min_y + 1;

	// Different sizes
	if(items_w != specs_w || items_h != specs_h)
		return false;

	for(u16 y=0; y<specs_h; y++)
	for(u16 x=0; x<specs_w; x++)
	{
		u16 items_x = items_min_x + x;
		u16 items_y = items_min_y + y;
		u16 specs_x = specs_min_x + x;
		u16 specs_y = specs_min_y + y;
		const InventoryItem *item = items[items_y * 3 + items_x];
		const ItemSpec &spec = specs[specs_y * 3 + specs_x];

		if(spec.checkItem(item) == false)
			return false;
	}

	return true;
}

bool checkItemCombination(const InventoryItem * const * items,
		const InventoryItem * const * specs)
{
	u16 items_min_x = 100;
	u16 items_max_x = 100;
	u16 items_min_y = 100;
	u16 items_max_y = 100;
	for(u16 y=0; y<3; y++)
	for(u16 x=0; x<3; x++)
	{
		if(items[y*3 + x] == NULL)
			continue;
		if(items_min_x == 100 || x < items_min_x)
			items_min_x = x;
		if(items_min_y == 100 || y < items_min_y)
			items_min_y = y;
		if(items_max_x == 100 || x > items_max_x)
			items_max_x = x;
		if(items_max_y == 100 || y > items_max_y)
			items_max_y = y;
	}
	// No items at all, just return false
	if(items_min_x == 100)
		return false;
	
	u16 items_w = items_max_x - items_min_x + 1;
	u16 items_h = items_max_y - items_min_y + 1;

	u16 specs_min_x = 100;
	u16 specs_max_x = 100;
	u16 specs_min_y = 100;
	u16 specs_max_y = 100;
	for(u16 y=0; y<3; y++)
	for(u16 x=0; x<3; x++)
	{
		if(specs[y*3 + x] == NULL)
			continue;
		if(specs_min_x == 100 || x < specs_min_x)
			specs_min_x = x;
		if(specs_min_y == 100 || y < specs_min_y)
			specs_min_y = y;
		if(specs_max_x == 100 || x > specs_max_x)
			specs_max_x = x;
		if(specs_max_y == 100 || y > specs_max_y)
			specs_max_y = y;
	}
	// No specs at all, just return false
	if(specs_min_x == 100)
		return false;

	u16 specs_w = specs_max_x - specs_min_x + 1;
	u16 specs_h = specs_max_y - specs_min_y + 1;

	// Different sizes
	if(items_w != specs_w || items_h != specs_h)
		return false;

	for(u16 y=0; y<specs_h; y++)
	for(u16 x=0; x<specs_w; x++)
	{
		u16 items_x = items_min_x + x;
		u16 items_y = items_min_y + y;
		u16 specs_x = specs_min_x + x;
		u16 specs_y = specs_min_y + y;
		const InventoryItem *item = items[items_y * 3 + items_x];
		const InventoryItem *spec = specs[specs_y * 3 + specs_x];
		
		if(item == NULL && spec == NULL)
			continue;
		if(item == NULL && spec != NULL)
			return false;
		if(item != NULL && spec == NULL)
			return false;
		if(!spec->isSubsetOf(item))
			return false;
	}

	return true;
}

//END
