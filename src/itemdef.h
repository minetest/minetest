/*
Minetest
Copyright (C) 2010-2013 celeron55, Perttu Ahola <celeron55@gmail.com>
Copyright (C) 2013 Kahrl <kahrl@gmx.net>

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

#ifndef ITEMDEF_HEADER
#define ITEMDEF_HEADER

#include "irrlichttypes_extrabloated.h"
#include <string>
#include <iostream>
#include <set>
#include "itemgroup.h"
#include "sound.h"
class IGameDef;
struct ToolCapabilities;

enum ItemType
{
	ITEM_NONE,
	ITEM_NODE,
	ITEM_CRAFT,
	ITEM_TOOL,
};

/**
 *  Defines items that can be used by players and added to their inventories.
 *
 *  The hand item is defined with name = "" and is guarenteed to always have tool_capabilities.
 *  This is enforced by CItemDefManager::clear() and CItemDefManager::registerItem().
 *
 *  tool_capabilities may be NULL but if it is non-NULL, this struct owns it and will delete it in the destructor.
 *
 *  node_placement_prediction is the name of the node that will be placed immediately by the client. The server will
 *  update the precise end result a moment later. "" = no prediction.
 *  If an ITEM_NODE item is defined in a mod, it defaults to the name of the item. This is done in ModApiItemMod::l_register_item_raw().
 */
struct ItemDefinition
{
	///@name Basic item properties
	///@{

	ItemType type;
	/// "" = hand
	std::string name;
	/// Shown in the tooltip.
	std::string description;
	///@}

	///@name Visual properties
	///@{

	/// Optional for ITEM_NODE, Mandatory for ITEM_CRAFT and ITEM_TOOL.
	std::string inventory_image;
	/// If empty, the inventory_image or mesh (only ITEM_NODE) is used.
	std::string wield_image;
	v3f wield_scale;
	///@}

	///@name Item stack and interaction properties
	///@{

	s16 stack_max;
	bool usable;
	bool liquids_pointable;
	/// May be NULL. If non-NULL, deleted by destructor.
	ToolCapabilities *tool_capabilities;
	ItemGroupList groups;
	SimpleSoundSpec sound_place;
	f32 range;
	///@}

	std::string node_placement_prediction;

	///@name Some helpful methods
	///@{

	ItemDefinition();
	ItemDefinition(const ItemDefinition &def);
	ItemDefinition& operator=(const ItemDefinition &def);
	~ItemDefinition();
	void reset();
	void serialize(std::ostream &os, u16 protocol_version) const;
	void deSerialize(std::istream &is);
	///@}
private:
	void resetInitial();
};

/**
 *  @class CItemDefManager
 *
 *  This stores ItemDefinitions for all known items in the game.
 *
 *  Use the IItemDefManager and IWritableItemDefManager interfaces,
 *  do not use this class directly.
 *  Use createItemDefManager() to instantiate this class.
 *
 *  All functions that take a name also accept an alias.
 *
 *  getInventoryTexture() and getWieldMesh() act differently depending on
 *  if called from the main thread or called from another thread.
 *  If called from another thread, it queues up a job to get the inventory
 *  texture and wield mesh and wait up to 1 second for it to finish.  See
 *  CItemDefManager::processQueue and CItemDefManager::getClientCached.
 *
 *  Every job is cached (including the main thread jobs), so subsequent calls
 *  will just read from the cache.
 *  The cache key is the passed in parameter, so an alias will have a separate
 *  cached result from the actual name. Adding aliases or item definitions will
 *  not clear the cache.
 *
 *  getAll() returns all names and aliases.
 *
 *  get() returns the "unknown" item if the requested item isn't found.
 *
 *  registerItem() overwrites any existing item with the same name and
 *  removes an alias if it is equal to the new item's name.
 *
 *  registerAlias() will not overwrite an existing alias if it already exists.
 *
 *  clear() removes all registered items and aliases. It re-adds the
 *  built-in items, which are: hand, unknown, air, and ignore.
 *
 *  processQueue() handles the job requests from getInventoryTexture() and
 *  getWieldMesh(). It should only be run from the main thread.
 */

/**
 * @class IItemDefManager
 *
 * See CItemDefManager
 */
class IItemDefManager
{
public:
	IItemDefManager(){}
	virtual ~IItemDefManager(){}

	virtual const ItemDefinition& get(const std::string &name) const=0;
	virtual std::string getAlias(const std::string &name) const=0;
	virtual std::set<std::string> getAll() const=0;
	virtual bool isKnown(const std::string &name) const=0;
#ifndef SERVER
	virtual video::ITexture* getInventoryTexture(const std::string &name,
			IGameDef *gamedef) const=0;
	virtual scene::IMesh* getWieldMesh(const std::string &name,
		IGameDef *gamedef) const=0;
#endif

	virtual void serialize(std::ostream &os, u16 protocol_version)=0;
};

/**
 * @class IWritableItemDefManager
 *
 * See CItemDefManager
 */
class IWritableItemDefManager : public IItemDefManager
{
public:
	IWritableItemDefManager(){}
	virtual ~IWritableItemDefManager(){}

	virtual void clear()=0;
	virtual void registerItem(const ItemDefinition &def)=0;
	virtual void registerAlias(const std::string &name,
			const std::string &convert_to)=0;

	virtual void serialize(std::ostream &os, u16 protocol_version)=0;
	virtual void deSerialize(std::istream &is)=0;

	virtual void processQueue(IGameDef *gamedef)=0;
};

IWritableItemDefManager* createItemDefManager();

#endif
