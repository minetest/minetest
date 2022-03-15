#pragma once

struct MoveAction;
class ServerActiveObject;

namespace api
{
namespace server
{

/*
 * NodeMeta callback events
 */

class NodeMeta
{
public:
	// Return number of accepted items to be moved
	virtual int nodemeta_inventory_AllowMove(
			const MoveAction &ma, int count, ServerActiveObject *player)
	{
		return 0;
	}
	// Return number of accepted items to be put
	virtual int nodemeta_inventory_AllowPut(const MoveAction &ma,
			const ItemStack &stack, ServerActiveObject *player)
	{
		return 0;
	}
	// Return number of accepted items to be taken
	virtual int nodemeta_inventory_AllowTake(const MoveAction &ma,
			const ItemStack &stack, ServerActiveObject *player)
	{
		return 0;
	}
	// Report moved items
	virtual void nodemeta_inventory_OnMove(
			const MoveAction &ma, int count, ServerActiveObject *player)
	{
	}
	// Report put items
	virtual void nodemeta_inventory_OnPut(const MoveAction &ma,
			const ItemStack &stack, ServerActiveObject *player)
	{
	}
	// Report taken items
	virtual void nodemeta_inventory_OnTake(const MoveAction &ma,
			const ItemStack &stack, ServerActiveObject *player)
	{
	}
};

}
}