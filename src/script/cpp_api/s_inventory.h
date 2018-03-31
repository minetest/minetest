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

struct MoveAction;
struct ItemStack;

class ScriptApiDetached
		: virtual public ScriptApiBase
{
public:
	/* Detached inventory callbacks */
	// Return number of accepted items to be moved
	int detached_inventory_AllowMove(
			const MoveAction &ma, int count,
			ServerActiveObject *player);
	// Return number of accepted items to be put
	int detached_inventory_AllowPut(
			const MoveAction &ma, const ItemStack &stack,
			ServerActiveObject *player);
	// Return number of accepted items to be taken
	int detached_inventory_AllowTake(
			const MoveAction &ma, const ItemStack &stack,
			ServerActiveObject *player);
	// Report moved items
	void detached_inventory_OnMove(
			const MoveAction &ma, int count,
			ServerActiveObject *player);
	// Report put items
	void detached_inventory_OnPut(
			const MoveAction &ma, const ItemStack &stack,
			ServerActiveObject *player);
	// Report taken items
	void detached_inventory_OnTake(
			const MoveAction &ma, const ItemStack &stack,
			ServerActiveObject *player);
private:
	bool getDetachedInventoryCallback(
			const std::string &name, const char *callbackname);
};
