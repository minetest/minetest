#pragma once

struct ItemStack;
struct MoveAction;

namespace api
{
namespace server
{
/*
 * Inventory callback events
 */

class Inventory
{
public:
	// Return number of accepted items to be moved
	virtual int detached_inventory_AllowMove(
			const MoveAction &ma, int count, ServerActiveObject *player)
	{
		return 0;
	}
	// Return number of accepted items to be put
	virtual int detached_inventory_AllowPut(const MoveAction &ma,
			const ItemStack &stack, ServerActiveObject *player)
	{
		return 0;
	}
	// Return number of accepted items to be taken
	virtual int detached_inventory_AllowTake(const MoveAction &ma,
			const ItemStack &stack, ServerActiveObject *player)
	{
		return 0;
	}
	// Report moved items
	virtual void detached_inventory_OnMove(
			const MoveAction &ma, int count, ServerActiveObject *player)
	{
	}
	// Report put items
	virtual void detached_inventory_OnPut(const MoveAction &ma,
			const ItemStack &stack, ServerActiveObject *player)
	{
	}
	// Report taken items
	virtual void detached_inventory_OnTake(const MoveAction &ma,
			const ItemStack &stack, ServerActiveObject *player)
	{
	}
};
}
}