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

#ifndef S_NODEMETA_H_
#define S_NODEMETA_H_

#include "cpp_api/s_base.h"
#include "cpp_api/s_item.h"
#include "irr_v3d.h"

struct ItemStack;

class ScriptApiNodemeta
		: virtual public ScriptApiBase,
		  public ScriptApiItem
{
public:
	ScriptApiNodemeta();
	virtual ~ScriptApiNodemeta();

	// Return number of accepted items to be moved
	int nodemeta_inventory_AllowMove(v3s16 p,
			const std::string &from_list, int from_index,
			const std::string &to_list, int to_index,
			int count, ServerActiveObject *player);
	// Return number of accepted items to be put
	int nodemeta_inventory_AllowPut(v3s16 p,
			const std::string &listname, int index, ItemStack &stack,
			ServerActiveObject *player);
	// Return number of accepted items to be taken
	int nodemeta_inventory_AllowTake(v3s16 p,
			const std::string &listname, int index, ItemStack &stack,
			ServerActiveObject *player);
	// Report moved items
	void nodemeta_inventory_OnMove(v3s16 p,
			const std::string &from_list, int from_index,
			const std::string &to_list, int to_index,
			int count, ServerActiveObject *player);
	// Report put items
	void nodemeta_inventory_OnPut(v3s16 p,
			const std::string &listname, int index, ItemStack &stack,
			ServerActiveObject *player);
	// Report taken items
	void nodemeta_inventory_OnTake(v3s16 p,
			const std::string &listname, int index, ItemStack &stack,
			ServerActiveObject *player);
private:

};

#endif /* S_NODEMETA_H_ */
