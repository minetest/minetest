#pragma once

class InventoryList;
struct InventoryLocation;
struct ItemStack;
struct PointedThing;

namespace api
{
namespace server
{
/*
 * Item callback events
 */

class Item
{
public:
	virtual bool item_OnDrop(ItemStack &item, ServerActiveObject *dropper, v3f pos)
	{
		return false;
	}
	virtual bool item_OnPlace(ItemStack &item, ServerActiveObject *placer,
			const PointedThing &pointed)
	{
		return false;
	}
	virtual bool item_OnUse(ItemStack &item, ServerActiveObject *user,
			const PointedThing &pointed)
	{
		return false;
	}
	virtual bool item_OnSecondaryUse(ItemStack &item, ServerActiveObject *user,
			const PointedThing &pointed)
	{
		return false;
	}
	virtual bool item_OnCraft(ItemStack &item, ServerActiveObject *user,
			const InventoryList *old_craft_grid,
			const InventoryLocation &craft_inv)
	{
		return false;
	}
	virtual bool item_CraftPredict(ItemStack &item, ServerActiveObject *user,
			const InventoryList *old_craft_grid,
			const InventoryLocation &craft_inv)
	{
		return false;
	}
};

}
}