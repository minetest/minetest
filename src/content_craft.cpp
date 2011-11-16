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

/*
	items: actually *items[9]
	return value: allocates a new item, or returns NULL.
*/
InventoryItem *craft_get_result(InventoryItem **items, IGameDef *gamedef)
{
	INodeDefManager *ndef = gamedef->ndef();

	// Wood
	{
		ItemSpec specs[9];
		specs[0] = ItemSpec(ITEM_MATERIAL, LEGN(ndef, "CONTENT_TREE"));
		if(checkItemCombination(items, specs))
		{
			return new MaterialItem(gamedef, LEGN(ndef, "CONTENT_WOOD"), 4);
		}
	}

	// Stick
	{
		ItemSpec specs[9];
		specs[0] = ItemSpec(ITEM_MATERIAL, LEGN(ndef, "CONTENT_WOOD"));
		if(checkItemCombination(items, specs))
		{
			return new CraftItem(gamedef, "Stick", 4);
		}
	}

	// Fence
	{
		ItemSpec specs[9];
		specs[3] = ItemSpec(ITEM_CRAFT, "Stick");
		specs[4] = ItemSpec(ITEM_CRAFT, "Stick");
		specs[5] = ItemSpec(ITEM_CRAFT, "Stick");
		specs[6] = ItemSpec(ITEM_CRAFT, "Stick");
		specs[7] = ItemSpec(ITEM_CRAFT, "Stick");
		specs[8] = ItemSpec(ITEM_CRAFT, "Stick");
		if(checkItemCombination(items, specs))
		{
			return new MaterialItem(gamedef, LEGN(ndef, "CONTENT_FENCE"), 2);
		}
	}

	// Sign
	{
		ItemSpec specs[9];
		specs[0] = ItemSpec(ITEM_MATERIAL, LEGN(ndef, "CONTENT_WOOD"));
		specs[1] = ItemSpec(ITEM_MATERIAL, LEGN(ndef, "CONTENT_WOOD"));
		specs[2] = ItemSpec(ITEM_MATERIAL, LEGN(ndef, "CONTENT_WOOD"));
		specs[3] = ItemSpec(ITEM_MATERIAL, LEGN(ndef, "CONTENT_WOOD"));
		specs[4] = ItemSpec(ITEM_MATERIAL, LEGN(ndef, "CONTENT_WOOD"));
		specs[5] = ItemSpec(ITEM_MATERIAL, LEGN(ndef, "CONTENT_WOOD"));
		specs[7] = ItemSpec(ITEM_CRAFT, "Stick");
		if(checkItemCombination(items, specs))
		{
			//return new MapBlockObjectItem(gamedef, "Sign");
			return new MaterialItem(gamedef, LEGN(ndef, "CONTENT_SIGN_WALL"), 1);
		}
	}

	// Torch
	{
		ItemSpec specs[9];
		specs[0] = ItemSpec(ITEM_CRAFT, "lump_of_coal");
		specs[3] = ItemSpec(ITEM_CRAFT, "Stick");
		if(checkItemCombination(items, specs))
		{
			return new MaterialItem(gamedef, LEGN(ndef, "CONTENT_TORCH"), 4);
		}
	}

	// Wooden pick
	{
		ItemSpec specs[9];
		specs[0] = ItemSpec(ITEM_MATERIAL, LEGN(ndef, "CONTENT_WOOD"));
		specs[1] = ItemSpec(ITEM_MATERIAL, LEGN(ndef, "CONTENT_WOOD"));
		specs[2] = ItemSpec(ITEM_MATERIAL, LEGN(ndef, "CONTENT_WOOD"));
		specs[4] = ItemSpec(ITEM_CRAFT, "Stick");
		specs[7] = ItemSpec(ITEM_CRAFT, "Stick");
		if(checkItemCombination(items, specs))
		{
			return new ToolItem(gamedef, "WPick", 0);
		}
	}

	// Stone pick
	{
		ItemSpec specs[9];
		specs[0] = ItemSpec(ITEM_MATERIAL, LEGN(ndef, "CONTENT_COBBLE"));
		specs[1] = ItemSpec(ITEM_MATERIAL, LEGN(ndef, "CONTENT_COBBLE"));
		specs[2] = ItemSpec(ITEM_MATERIAL, LEGN(ndef, "CONTENT_COBBLE"));
		specs[4] = ItemSpec(ITEM_CRAFT, "Stick");
		specs[7] = ItemSpec(ITEM_CRAFT, "Stick");
		if(checkItemCombination(items, specs))
		{
			return new ToolItem(gamedef, "STPick", 0);
		}
	}

	// Steel pick
	{
		ItemSpec specs[9];
		specs[0] = ItemSpec(ITEM_CRAFT, "steel_ingot");
		specs[1] = ItemSpec(ITEM_CRAFT, "steel_ingot");
		specs[2] = ItemSpec(ITEM_CRAFT, "steel_ingot");
		specs[4] = ItemSpec(ITEM_CRAFT, "Stick");
		specs[7] = ItemSpec(ITEM_CRAFT, "Stick");
		if(checkItemCombination(items, specs))
		{
			return new ToolItem(gamedef, "SteelPick", 0);
		}
	}

	// Mese pick
	{
		ItemSpec specs[9];
		specs[0] = ItemSpec(ITEM_MATERIAL, LEGN(ndef, "CONTENT_MESE"));
		specs[1] = ItemSpec(ITEM_MATERIAL, LEGN(ndef, "CONTENT_MESE"));
		specs[2] = ItemSpec(ITEM_MATERIAL, LEGN(ndef, "CONTENT_MESE"));
		specs[4] = ItemSpec(ITEM_CRAFT, "Stick");
		specs[7] = ItemSpec(ITEM_CRAFT, "Stick");
		if(checkItemCombination(items, specs))
		{
			return new ToolItem(gamedef, "MesePick", 0);
		}
	}

	// Wooden shovel
	{
		ItemSpec specs[9];
		specs[1] = ItemSpec(ITEM_MATERIAL, LEGN(ndef, "CONTENT_WOOD"));
		specs[4] = ItemSpec(ITEM_CRAFT, "Stick");
		specs[7] = ItemSpec(ITEM_CRAFT, "Stick");
		if(checkItemCombination(items, specs))
		{
			return new ToolItem(gamedef, "WShovel", 0);
		}
	}

	// Stone shovel
	{
		ItemSpec specs[9];
		specs[1] = ItemSpec(ITEM_MATERIAL, LEGN(ndef, "CONTENT_COBBLE"));
		specs[4] = ItemSpec(ITEM_CRAFT, "Stick");
		specs[7] = ItemSpec(ITEM_CRAFT, "Stick");
		if(checkItemCombination(items, specs))
		{
			return new ToolItem(gamedef, "STShovel", 0);
		}
	}

	// Steel shovel
	{
		ItemSpec specs[9];
		specs[1] = ItemSpec(ITEM_CRAFT, "steel_ingot");
		specs[4] = ItemSpec(ITEM_CRAFT, "Stick");
		specs[7] = ItemSpec(ITEM_CRAFT, "Stick");
		if(checkItemCombination(items, specs))
		{
			return new ToolItem(gamedef, "SteelShovel", 0);
		}
	}

	// Wooden axe
	{
		ItemSpec specs[9];
		specs[0] = ItemSpec(ITEM_MATERIAL, LEGN(ndef, "CONTENT_WOOD"));
		specs[1] = ItemSpec(ITEM_MATERIAL, LEGN(ndef, "CONTENT_WOOD"));
		specs[3] = ItemSpec(ITEM_MATERIAL, LEGN(ndef, "CONTENT_WOOD"));
		specs[4] = ItemSpec(ITEM_CRAFT, "Stick");
		specs[7] = ItemSpec(ITEM_CRAFT, "Stick");
		if(checkItemCombination(items, specs))
		{
			return new ToolItem(gamedef, "WAxe", 0);
		}
	}

	// Stone axe
	{
		ItemSpec specs[9];
		specs[0] = ItemSpec(ITEM_MATERIAL, LEGN(ndef, "CONTENT_COBBLE"));
		specs[1] = ItemSpec(ITEM_MATERIAL, LEGN(ndef, "CONTENT_COBBLE"));
		specs[3] = ItemSpec(ITEM_MATERIAL, LEGN(ndef, "CONTENT_COBBLE"));
		specs[4] = ItemSpec(ITEM_CRAFT, "Stick");
		specs[7] = ItemSpec(ITEM_CRAFT, "Stick");
		if(checkItemCombination(items, specs))
		{
			return new ToolItem(gamedef, "STAxe", 0);
		}
	}

	// Steel axe
	{
		ItemSpec specs[9];
		specs[0] = ItemSpec(ITEM_CRAFT, "steel_ingot");
		specs[1] = ItemSpec(ITEM_CRAFT, "steel_ingot");
		specs[3] = ItemSpec(ITEM_CRAFT, "steel_ingot");
		specs[4] = ItemSpec(ITEM_CRAFT, "Stick");
		specs[7] = ItemSpec(ITEM_CRAFT, "Stick");
		if(checkItemCombination(items, specs))
		{
			return new ToolItem(gamedef, "SteelAxe", 0);
		}
	}

	// Wooden sword
	{
		ItemSpec specs[9];
		specs[1] = ItemSpec(ITEM_MATERIAL, LEGN(ndef, "CONTENT_WOOD"));
		specs[4] = ItemSpec(ITEM_MATERIAL, LEGN(ndef, "CONTENT_WOOD"));
		specs[7] = ItemSpec(ITEM_CRAFT, "Stick");
		if(checkItemCombination(items, specs))
		{
			return new ToolItem(gamedef, "WSword", 0);
		}
	}

	// Stone sword
	{
		ItemSpec specs[9];
		specs[1] = ItemSpec(ITEM_MATERIAL, LEGN(ndef, "CONTENT_COBBLE"));
		specs[4] = ItemSpec(ITEM_MATERIAL, LEGN(ndef, "CONTENT_COBBLE"));
		specs[7] = ItemSpec(ITEM_CRAFT, "Stick");
		if(checkItemCombination(items, specs))
		{
			return new ToolItem(gamedef, "STSword", 0);
		}
	}

	// Steel sword
	{
		ItemSpec specs[9];
		specs[1] = ItemSpec(ITEM_CRAFT, "steel_ingot");
		specs[4] = ItemSpec(ITEM_CRAFT, "steel_ingot");
		specs[7] = ItemSpec(ITEM_CRAFT, "Stick");
		if(checkItemCombination(items, specs))
		{
			return new ToolItem(gamedef, "SteelSword", 0);
		}
	}

	// Rail
	{
		ItemSpec specs[9];
		specs[0] = ItemSpec(ITEM_CRAFT, "steel_ingot");
		specs[1] = ItemSpec(ITEM_CRAFT, "Stick");
		specs[2] = ItemSpec(ITEM_CRAFT, "steel_ingot");
		specs[3] = ItemSpec(ITEM_CRAFT, "steel_ingot");
		specs[4] = ItemSpec(ITEM_CRAFT, "Stick");
		specs[5] = ItemSpec(ITEM_CRAFT, "steel_ingot");
		specs[6] = ItemSpec(ITEM_CRAFT, "steel_ingot");
		specs[7] = ItemSpec(ITEM_CRAFT, "Stick");
		specs[8] = ItemSpec(ITEM_CRAFT, "steel_ingot");
		if(checkItemCombination(items, specs))
		{
			return new MaterialItem(gamedef, LEGN(ndef, "CONTENT_RAIL"), 15);
		}
	}

	// Chest
	{
		ItemSpec specs[9];
		specs[0] = ItemSpec(ITEM_MATERIAL, LEGN(ndef, "CONTENT_WOOD"));
		specs[1] = ItemSpec(ITEM_MATERIAL, LEGN(ndef, "CONTENT_WOOD"));
		specs[2] = ItemSpec(ITEM_MATERIAL, LEGN(ndef, "CONTENT_WOOD"));
		specs[3] = ItemSpec(ITEM_MATERIAL, LEGN(ndef, "CONTENT_WOOD"));
		specs[5] = ItemSpec(ITEM_MATERIAL, LEGN(ndef, "CONTENT_WOOD"));
		specs[6] = ItemSpec(ITEM_MATERIAL, LEGN(ndef, "CONTENT_WOOD"));
		specs[7] = ItemSpec(ITEM_MATERIAL, LEGN(ndef, "CONTENT_WOOD"));
		specs[8] = ItemSpec(ITEM_MATERIAL, LEGN(ndef, "CONTENT_WOOD"));
		if(checkItemCombination(items, specs))
		{
			return new MaterialItem(gamedef, LEGN(ndef, "CONTENT_CHEST"), 1);
		}
	}

	// Locking Chest
	{
		ItemSpec specs[9];
		specs[0] = ItemSpec(ITEM_MATERIAL, LEGN(ndef, "CONTENT_WOOD"));
		specs[1] = ItemSpec(ITEM_MATERIAL, LEGN(ndef, "CONTENT_WOOD"));
		specs[2] = ItemSpec(ITEM_MATERIAL, LEGN(ndef, "CONTENT_WOOD"));
		specs[3] = ItemSpec(ITEM_MATERIAL, LEGN(ndef, "CONTENT_WOOD"));
		specs[4] = ItemSpec(ITEM_CRAFT, "steel_ingot");
		specs[5] = ItemSpec(ITEM_MATERIAL, LEGN(ndef, "CONTENT_WOOD"));
		specs[6] = ItemSpec(ITEM_MATERIAL, LEGN(ndef, "CONTENT_WOOD"));
		specs[7] = ItemSpec(ITEM_MATERIAL, LEGN(ndef, "CONTENT_WOOD"));
		specs[8] = ItemSpec(ITEM_MATERIAL, LEGN(ndef, "CONTENT_WOOD"));
		if(checkItemCombination(items, specs))
		{
			return new MaterialItem(gamedef, LEGN(ndef, "CONTENT_LOCKABLE_CHEST"), 1);
		}
	}

	// Furnace
	{
		ItemSpec specs[9];
		specs[0] = ItemSpec(ITEM_MATERIAL, LEGN(ndef, "CONTENT_COBBLE"));
		specs[1] = ItemSpec(ITEM_MATERIAL, LEGN(ndef, "CONTENT_COBBLE"));
		specs[2] = ItemSpec(ITEM_MATERIAL, LEGN(ndef, "CONTENT_COBBLE"));
		specs[3] = ItemSpec(ITEM_MATERIAL, LEGN(ndef, "CONTENT_COBBLE"));
		specs[5] = ItemSpec(ITEM_MATERIAL, LEGN(ndef, "CONTENT_COBBLE"));
		specs[6] = ItemSpec(ITEM_MATERIAL, LEGN(ndef, "CONTENT_COBBLE"));
		specs[7] = ItemSpec(ITEM_MATERIAL, LEGN(ndef, "CONTENT_COBBLE"));
		specs[8] = ItemSpec(ITEM_MATERIAL, LEGN(ndef, "CONTENT_COBBLE"));
		if(checkItemCombination(items, specs))
		{
			return new MaterialItem(gamedef, LEGN(ndef, "CONTENT_FURNACE"), 1);
		}
	}

	// Steel block
	{
		ItemSpec specs[9];
		specs[0] = ItemSpec(ITEM_CRAFT, "steel_ingot");
		specs[1] = ItemSpec(ITEM_CRAFT, "steel_ingot");
		specs[2] = ItemSpec(ITEM_CRAFT, "steel_ingot");
		specs[3] = ItemSpec(ITEM_CRAFT, "steel_ingot");
		specs[4] = ItemSpec(ITEM_CRAFT, "steel_ingot");
		specs[5] = ItemSpec(ITEM_CRAFT, "steel_ingot");
		specs[6] = ItemSpec(ITEM_CRAFT, "steel_ingot");
		specs[7] = ItemSpec(ITEM_CRAFT, "steel_ingot");
		specs[8] = ItemSpec(ITEM_CRAFT, "steel_ingot");
		if(checkItemCombination(items, specs))
		{
			return new MaterialItem(gamedef, LEGN(ndef, "CONTENT_STEEL"), 1);
		}
	}

	// Sandstone
	{
		ItemSpec specs[9];
		specs[3] = ItemSpec(ITEM_MATERIAL, LEGN(ndef, "CONTENT_SAND"));
		specs[4] = ItemSpec(ITEM_MATERIAL, LEGN(ndef, "CONTENT_SAND"));
		specs[6] = ItemSpec(ITEM_MATERIAL, LEGN(ndef, "CONTENT_SAND"));
		specs[7] = ItemSpec(ITEM_MATERIAL, LEGN(ndef, "CONTENT_SAND"));
		if(checkItemCombination(items, specs))
		{
			return new MaterialItem(gamedef, LEGN(ndef, "CONTENT_SANDSTONE"), 1);
		}
	}

	// Clay
	{
		ItemSpec specs[9];
		specs[3] = ItemSpec(ITEM_CRAFT, "lump_of_clay");
		specs[4] = ItemSpec(ITEM_CRAFT, "lump_of_clay");
		specs[6] = ItemSpec(ITEM_CRAFT, "lump_of_clay");
		specs[7] = ItemSpec(ITEM_CRAFT, "lump_of_clay");
		if(checkItemCombination(items, specs))
		{
			return new MaterialItem(gamedef, LEGN(ndef, "CONTENT_CLAY"), 1);
		}
	}

	// Brick
	{
		ItemSpec specs[9];
		specs[3] = ItemSpec(ITEM_CRAFT, "clay_brick");
		specs[4] = ItemSpec(ITEM_CRAFT, "clay_brick");
		specs[6] = ItemSpec(ITEM_CRAFT, "clay_brick");
		specs[7] = ItemSpec(ITEM_CRAFT, "clay_brick");
		if(checkItemCombination(items, specs))
		{
			return new MaterialItem(gamedef, LEGN(ndef, "CONTENT_BRICK"), 1);
		}
	}

	// Paper
	{
		ItemSpec specs[9];
		specs[3] = ItemSpec(ITEM_MATERIAL, LEGN(ndef, "CONTENT_PAPYRUS"));
		specs[4] = ItemSpec(ITEM_MATERIAL, LEGN(ndef, "CONTENT_PAPYRUS"));
		specs[5] = ItemSpec(ITEM_MATERIAL, LEGN(ndef, "CONTENT_PAPYRUS"));
		if(checkItemCombination(items, specs))
		{
			return new CraftItem(gamedef, "paper", 1);
		}
	}

	// Book
	{
		ItemSpec specs[9];
		specs[1] = ItemSpec(ITEM_CRAFT, "paper");
		specs[4] = ItemSpec(ITEM_CRAFT, "paper");
		specs[7] = ItemSpec(ITEM_CRAFT, "paper");
		if(checkItemCombination(items, specs))
		{
			return new CraftItem(gamedef, "book", 1);
		}
	}

	// Book shelf
	{
		ItemSpec specs[9];
		specs[0] = ItemSpec(ITEM_MATERIAL, LEGN(ndef, "CONTENT_WOOD"));
		specs[1] = ItemSpec(ITEM_MATERIAL, LEGN(ndef, "CONTENT_WOOD"));
		specs[2] = ItemSpec(ITEM_MATERIAL, LEGN(ndef, "CONTENT_WOOD"));
		specs[3] = ItemSpec(ITEM_CRAFT, "book");
		specs[4] = ItemSpec(ITEM_CRAFT, "book");
		specs[5] = ItemSpec(ITEM_CRAFT, "book");
		specs[6] = ItemSpec(ITEM_MATERIAL, LEGN(ndef, "CONTENT_WOOD"));
		specs[7] = ItemSpec(ITEM_MATERIAL, LEGN(ndef, "CONTENT_WOOD"));
		specs[8] = ItemSpec(ITEM_MATERIAL, LEGN(ndef, "CONTENT_WOOD"));
		if(checkItemCombination(items, specs))
		{
			return new MaterialItem(gamedef, LEGN(ndef, "CONTENT_BOOKSHELF"), 1);
		}
	}

	// Ladder
	{
		ItemSpec specs[9];
		specs[0] = ItemSpec(ITEM_CRAFT, "Stick");
		specs[2] = ItemSpec(ITEM_CRAFT, "Stick");
		specs[3] = ItemSpec(ITEM_CRAFT, "Stick");
		specs[4] = ItemSpec(ITEM_CRAFT, "Stick");
		specs[5] = ItemSpec(ITEM_CRAFT, "Stick");
		specs[6] = ItemSpec(ITEM_CRAFT, "Stick");
		specs[8] = ItemSpec(ITEM_CRAFT, "Stick");
		if(checkItemCombination(items, specs))
		{
			return new MaterialItem(gamedef, LEGN(ndef, "CONTENT_LADDER"), 1);
		}
	}
	
	// Iron Apple
	{
		ItemSpec specs[9];
		specs[1] = ItemSpec(ITEM_CRAFT, "steel_ingot");
		specs[3] = ItemSpec(ITEM_CRAFT, "steel_ingot");
		specs[4] = ItemSpec(ITEM_CRAFT, "apple");
		specs[5] = ItemSpec(ITEM_CRAFT, "steel_ingot");
		specs[7] = ItemSpec(ITEM_CRAFT, "steel_ingot");
		if(checkItemCombination(items, specs))
		{
			return new CraftItem(gamedef, "apple_iron", 1);
		}
	}

	return NULL;
}

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

