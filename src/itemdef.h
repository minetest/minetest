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
#include <map>
#include "itemgroup.h"
#include "sound.h"
#include "util/container.h"
#include "util/thread.h"

#ifndef SERVER
#include "client/tile.h"
#endif

class IGameDef;
class INodeDefManager;
struct ToolCapabilities;

/*
	Base item definition
*/

enum ItemType
{
	ITEM_NONE,
	ITEM_NODE,
	ITEM_CRAFT,
	ITEM_TOOL
};

struct ItemDefinition
{
	/*
		Basic item properties
	*/
	ItemType type;
	std::string name; // "" = hand
	std::string description; // Shown in tooltip.

	/*
		Visual properties
	*/
	std::string inventory_image; // Optional for nodes, mandatory for tools/craftitems
	std::string wield_image; // If empty, inventory_image or mesh (only nodes) is used
	v3f wield_scale;
	std::string meshname;    // name of internal mesh (or meshfile to use TBD)
	std::string meshtexture; // meshtexture

	/*
		Item stack and interaction properties
	*/
	s16 stack_max;
	bool usable;
	bool liquids_pointable;
	// May be NULL. If non-NULL, deleted by destructor
	ToolCapabilities *tool_capabilities;
	ItemGroupList groups;
	SimpleSoundSpec sound_place;
	SimpleSoundSpec sound_place_failed;
	f32 range;

	// Client shall immediately place this node when player places the item.
	// Server will update the precise end result a moment later.
	// "" = no prediction
	std::string node_placement_prediction;

	/*
		Some helpful methods
	*/
	ItemDefinition();
	ItemDefinition(const ItemDefinition &def);
	ItemDefinition& operator=(const ItemDefinition &def);
	~ItemDefinition();
	void reset();
	void serialize(std::ostream &os, u16 protocol_version) const;
	void deSerialize(std::istream &is);
private:
	void resetInitial();
};

class IItemDefManager
{
public:
	IItemDefManager(){}
	virtual ~IItemDefManager(){}

	// Get item definition
	virtual const ItemDefinition& get(const std::string &name) const=0;
	// Get alias definition
	virtual std::string getAlias(const std::string &name) const=0;
	// Get set of all defined item names and aliases
	virtual std::set<std::string> getAll() const=0;
	// Check if item is known
	virtual bool isKnown(const std::string &name) const=0;
#ifndef SERVER
	// Get item inventory texture
	virtual video::ITexture* getInventoryTexture(const std::string &name,
			IGameDef *gamedef) const=0;
	// Get item wield mesh
	virtual scene::IMesh* getWieldMesh(const std::string &name,
		IGameDef *gamedef) const=0;
#endif

	virtual void serialize(std::ostream &os, u16 protocol_version)=0;
};

class IWritableItemDefManager : public IItemDefManager
{
public:
	IWritableItemDefManager(){}
	virtual ~IWritableItemDefManager(){}

	// Get item definition
	virtual const ItemDefinition& get(const std::string &name) const=0;
	// Get alias definition
	virtual std::string getAlias(const std::string &name) const=0;
	// Get set of all defined item names and aliases
	virtual std::set<std::string> getAll() const=0;
	// Check if item is known
	virtual bool isKnown(const std::string &name) const=0;
#ifndef SERVER
	// Get item inventory texture
	virtual video::ITexture* getInventoryTexture(const std::string &name,
			IGameDef *gamedef) const=0;
	// Get item wield mesh
	virtual scene::IMesh* getWieldMesh(const std::string &name,
		IGameDef *gamedef) const=0;
#endif

	// Remove all registered item and node definitions and aliases
	// Then re-add the builtin item definitions
	virtual void clear()=0;
	// Register item definition
	virtual void registerItem(const ItemDefinition &def)=0;
	// Set an alias so that items named <name> will load as <convert_to>.
	// Alias is not set if <name> has already been defined.
	// Alias will be removed if <name> is defined at a later point of time.
	virtual void registerAlias(const std::string &name,
			const std::string &convert_to)=0;

	virtual void serialize(std::ostream &os, u16 protocol_version)=0;
	virtual void deSerialize(std::istream &is)=0;

	// Do stuff asked by threads that can only be done in the main thread
	virtual void processQueue(IGameDef *gamedef)=0;
};


class CItemDefManager: public IWritableItemDefManager
{
public:
	CItemDefManager();
	virtual ~CItemDefManager();
	virtual const ItemDefinition& get(const std::string &name_) const;
	virtual std::string getAlias(const std::string &name) const;
	virtual std::set<std::string> getAll() const;
	virtual bool isKnown(const std::string &name_) const;

#ifndef SERVER
	// Get item inventory texture
	virtual video::ITexture* getInventoryTexture(const std::string &name,
			IGameDef *gamedef) const;

	// Get item wield mesh
	virtual scene::IMesh* getWieldMesh(const std::string &name,
			IGameDef *gamedef) const;
#endif
	void clear();

	virtual void registerItem(const ItemDefinition &def);
	virtual void registerAlias(const std::string &name,
			const std::string &convert_to);
	void serialize(std::ostream &os, u16 protocol_version);
	void deSerialize(std::istream &is);

	void processQueue(IGameDef *gamedef);

private:

#ifndef SERVER
	struct ClientCached
	{
		video::ITexture *inventory_texture;
		scene::IMesh *wield_mesh;

		ClientCached();
	};

	void createNodeItemTexture(const std::string& name,
			const ItemDefinition& def, INodeDefManager* nodedef,
			ClientCached* cc, IGameDef* gamedef, ITextureSource* tsrc) const;

	void createMeshItemTexture(const std::string& name,
			const ItemDefinition& def, INodeDefManager* nodedef,
			ClientCached* cc, IGameDef* gamedef, ITextureSource* tsrc) const;

	void renderMeshToTexture(const ItemDefinition& def, scene::IMesh* mesh,
			ClientCached* cc, ITextureSource* tsrc) const;

	ClientCached* createClientCachedDirect(const std::string &name,
			IGameDef *gamedef) const;

	ClientCached* getClientCached(const std::string &name,
			IGameDef *gamedef) const;

	// The id of the thread that is allowed to use irrlicht directly
	threadid_t m_main_thread;

	// A reference to this can be returned when nothing is found, to avoid NULLs
	mutable ClientCached m_dummy_clientcached;

	// Cached textures and meshes
	mutable MutexedMap<std::string, ClientCached*> m_clientcached;

	// Queued clientcached fetches (to be processed by the main thread)
	mutable RequestQueue<std::string, ClientCached*, u8, u8> m_get_clientcached_queue;
#endif

	// Key is name
	std::map<std::string, ItemDefinition*> m_item_definitions;

	// Aliases
	std::map<std::string, std::string> m_aliases;
};

IWritableItemDefManager* createItemDefManager();

#endif
