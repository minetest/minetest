/*
Minetest
Copyright (C) 2013 celeron55, Perttu Ahola <celeron55@gmail.com>

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

#pragma once

#include "cpp_api/s_base.h"
#include "irr_v3d.h"
#include "util/Optional.h"

struct PointedThing;
struct ItemStack;
class ServerActiveObject;
struct ItemDefinition;
class LuaItemStack;
class ModApiItemMod;
class InventoryList;
struct InventoryLocation;

class ScriptApiItem
: virtual public ScriptApiBase
{
public:
	/*
	 * Functions with Optional<ItemStack> are for callbacks where Lua may
	 * want to prevent the engine from modifying the inventory after it's done.
	 * This has a longer backstory where on_use may need to empty the player's
	 * inventory without the engine interfering (see issue #6546).
	 */

	bool item_OnDrop(ItemStack &item,
			ServerActiveObject *dropper, v3f pos);
	bool item_OnPlace(Optional<ItemStack> &item,
			ServerActiveObject *placer, const PointedThing &pointed);
	bool item_OnUse(Optional<ItemStack> &item,
			ServerActiveObject *user, const PointedThing &pointed);
	bool item_OnSecondaryUse(Optional<ItemStack> &item,
			ServerActiveObject *user, const PointedThing &pointed);
	bool item_OnCraft(ItemStack &item, ServerActiveObject *user,
			const InventoryList *old_craft_grid, const InventoryLocation &craft_inv);
	bool item_CraftPredict(ItemStack &item, ServerActiveObject *user,
			const InventoryList *old_craft_grid, const InventoryLocation &craft_inv);

protected:
	friend class LuaItemStack;
	friend class ModApiItemMod;

	bool getItemCallback(const char *name, const char *callbackname, const v3s16 *p = nullptr);
	/*!
	 * Pushes a `pointed_thing` tabe to the stack.
	 * \param hitpoint If true, the exact pointing location is also pushed
	 */
	void pushPointedThing(const PointedThing &pointed, bool hitpoint = false);

};
