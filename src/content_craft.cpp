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

#include "content_craft.h"
#include "inventory.h"
#include "content_mapnode.h"
#include "player.h"
#include "mapnode.h" // For content_t
#include "gamedef.h"

void craft_set_creative_inventory(Player *player, IGameDef *gamedef)
{
	INodeDefManager *ndef = gamedef->ndef();

	player->resetInventory();
	
	// Give some good tools
	{
		InventoryItem *item = new ToolItem(gamedef, "MesePick", 0);
		void* r = player->inventory.addItem("main", item);
		assert(r == NULL);
	}
	{
		InventoryItem *item = new ToolItem(gamedef, "SteelPick", 0);
		void* r = player->inventory.addItem("main", item);
		assert(r == NULL);
	}
	{
		InventoryItem *item = new ToolItem(gamedef, "SteelAxe", 0);
		void* r = player->inventory.addItem("main", item);
		assert(r == NULL);
	}
	{
		InventoryItem *item = new ToolItem(gamedef, "SteelShovel", 0);
		void* r = player->inventory.addItem("main", item);
		assert(r == NULL);
	}

	/*
		Give materials
	*/
	
	// CONTENT_IGNORE-terminated list
	content_t material_items[] = {
		LEGN(ndef, "CONTENT_TORCH"),
		LEGN(ndef, "CONTENT_COBBLE"),
		LEGN(ndef, "CONTENT_MUD"),
		LEGN(ndef, "CONTENT_STONE"),
		LEGN(ndef, "CONTENT_SAND"),
		LEGN(ndef, "CONTENT_SANDSTONE"),
		LEGN(ndef, "CONTENT_CLAY"),
		LEGN(ndef, "CONTENT_BRICK"),
		LEGN(ndef, "CONTENT_TREE"),
		LEGN(ndef, "CONTENT_LEAVES"),
		LEGN(ndef, "CONTENT_CACTUS"),
		LEGN(ndef, "CONTENT_PAPYRUS"),
		LEGN(ndef, "CONTENT_BOOKSHELF"),
		LEGN(ndef, "CONTENT_GLASS"),
		LEGN(ndef, "CONTENT_FENCE"),
		LEGN(ndef, "CONTENT_RAIL"),
		LEGN(ndef, "CONTENT_MESE"),
		LEGN(ndef, "CONTENT_WATERSOURCE"),
		LEGN(ndef, "CONTENT_CLOUD"),
		LEGN(ndef, "CONTENT_CHEST"),
		LEGN(ndef, "CONTENT_FURNACE"),
		LEGN(ndef, "CONTENT_SIGN_WALL"),
		LEGN(ndef, "CONTENT_LAVASOURCE"),
		CONTENT_IGNORE
	};
	
	content_t *mip = material_items;
	for(u16 i=0; i<PLAYER_INVENTORY_SIZE; i++)
	{
		if(*mip == CONTENT_IGNORE)
			break;

		InventoryItem *item = new MaterialItem(gamedef, *mip, 1);
		player->inventory.addItem("main", item);

		mip++;
	}

#if 0
	assert(USEFUL_LEGN(ndef, "CONTENT_COUNT") <= PLAYER_INVENTORY_SIZE);
	
	// add torch first
	InventoryItem *item = new MaterialItem(gamedef, LEGN(ndef, "CONTENT_TORCH"), 1);
	player->inventory.addItem("main", item);
	
	// Then others
	for(u16 i=0; i<USEFUL_LEGN(ndef, "CONTENT_COUNT"); i++)
	{
		// Skip some materials
		if(i == LEGN(ndef, "CONTENT_WATER") || i == LEGN(ndef, "CONTENT_TORCH")
			|| i == LEGN(ndef, "CONTENT_COALSTONE"))
			continue;

		InventoryItem *item = new MaterialItem(gamedef, i, 1);
		player->inventory.addItem("main", item);
	}
#endif

	/*// Sign
	{
		InventoryItem *item = new MapBlockObjectItem(gamedef, "Sign Example text");
		void* r = player->inventory.addItem("main", item);
		assert(r == NULL);
	}*/
}

void craft_give_initial_stuff(Player *player, IGameDef *gamedef)
{
	INodeDefManager *ndef = gamedef->ndef();

	{
		InventoryItem *item = new ToolItem(gamedef, "SteelPick", 0);
		void* r = player->inventory.addItem("main", item);
		assert(r == NULL);
	}
	{
		InventoryItem *item = new MaterialItem(gamedef, LEGN(ndef, "CONTENT_TORCH"), 99);
		void* r = player->inventory.addItem("main", item);
		assert(r == NULL);
	}
	{
		InventoryItem *item = new ToolItem(gamedef, "SteelAxe", 0);
		void* r = player->inventory.addItem("main", item);
		assert(r == NULL);
	}
	{
		InventoryItem *item = new ToolItem(gamedef, "SteelShovel", 0);
		void* r = player->inventory.addItem("main", item);
		assert(r == NULL);
	}
	{
		InventoryItem *item = new MaterialItem(gamedef, LEGN(ndef, "CONTENT_COBBLE"), 99);
		void* r = player->inventory.addItem("main", item);
		assert(r == NULL);
	}
	/*{
		InventoryItem *item = new MaterialItem(gamedef, LEGN(ndef, "CONTENT_MESE"), 6);
		void* r = player->inventory.addItem("main", item);
		assert(r == NULL);
	}
	{
		InventoryItem *item = new MaterialItem(gamedef, LEGN(ndef, "CONTENT_COALSTONE"), 6);
		void* r = player->inventory.addItem("main", item);
		assert(r == NULL);
	}
	{
		InventoryItem *item = new MaterialItem(gamedef, LEGN(ndef, "CONTENT_WOOD"), 6);
		void* r = player->inventory.addItem("main", item);
		assert(r == NULL);
	}
	{
		InventoryItem *item = new CraftItem(gamedef, "Stick", 4);
		void* r = player->inventory.addItem("main", item);
		assert(r == NULL);
	}
	{
		InventoryItem *item = new ToolItem(gamedef, "WPick", 32000);
		void* r = player->inventory.addItem("main", item);
		assert(r == NULL);
	}
	{
		InventoryItem *item = new ToolItem(gamedef, "STPick", 32000);
		void* r = player->inventory.addItem("main", item);
		assert(r == NULL);
	}*/
	/*// and some signs
	for(u16 i=0; i<4; i++)
	{
		InventoryItem *item = new MapBlockObjectItem(gamedef, "Sign Example text");
		bool r = player->inventory.addItem("main", item);
		assert(r == true);
	}*/
	/*// Give some other stuff
	{
		InventoryItem *item = new MaterialItem(gamedef, LEGN(ndef, "CONTENT_TREE"), 999);
		bool r = player->inventory.addItem("main", item);
		assert(r == true);
	}*/
}

