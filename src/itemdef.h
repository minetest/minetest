/*
Minetest-c55
Copyright (C) 2010-2011 celeron55, Perttu Ahola <celeron55@gmail.com>
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

#ifndef ITEMDEF_HEADER
#define ITEMDEF_HEADER

#include "common_irrlicht.h"
#include <string>
#include <iostream>
#include <set>
class IGameDef;
struct ToolDiggingProperties;

/*
	Base item definition
*/

enum ItemType
{
	ITEM_NONE,
	ITEM_NODE,
	ITEM_CRAFT,
	ITEM_TOOL,
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

	/*
		Item stack and interaction properties
	*/
	s16 stack_max;
	bool usable;
	bool liquids_pointable;
	// May be NULL. If non-NULL, deleted by destructor
	ToolDiggingProperties *tool_digging_properties;

	/*
		Cached stuff
	*/
#ifndef SERVER
	video::ITexture *inventory_texture;
	scene::IMesh *wield_mesh;
#endif

	/*
		Some helpful methods
	*/
	ItemDefinition();
	ItemDefinition(const ItemDefinition &def);
	ItemDefinition& operator=(const ItemDefinition &def);
	~ItemDefinition();
	void reset();
	void serialize(std::ostream &os) const;
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

	virtual void serialize(std::ostream &os)=0;
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

	/*
		Update inventory textures and wield meshes to latest
		return values of ITextureSource and INodeDefManager.
		Call after updating the texture atlas of a texture source.
	*/
	virtual void updateTexturesAndMeshes(IGameDef *gamedef)=0;

	virtual void serialize(std::ostream &os)=0;
	virtual void deSerialize(std::istream &is)=0;
};

IWritableItemDefManager* createItemDefManager();

#endif
