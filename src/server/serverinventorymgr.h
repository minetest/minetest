/*
Minetest
Copyright (C) 2010-2020 Minetest core development team

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

#include "inventorymanager.h"
#include <cassert>
#include <functional>
#include <memory>
#include <unordered_map>

class IItemDefManager;
class ServerEnvironment;

class ServerInventoryManager : public InventoryManager
{
public:
	ServerInventoryManager();
	virtual ~ServerInventoryManager() = default;

	void setEnv(ServerEnvironment *env)
	{
		assert(!m_env);
		m_env = env;
	}

	// virtual: Overwritten by MockInventoryManager for the unittests
	virtual Inventory *getInventory(const InventoryLocation &loc);
	virtual void setInventoryModified(const InventoryLocation &loc);

	// Creates or resets inventory
	Inventory *createDetachedInventory(const std::string &name, IItemDefManager *idef,
			const std::string &player = "");
	bool removeDetachedInventory(const std::string &name);
	bool checkDetachedInventoryAccess(const InventoryLocation &loc, const std::string &player) const;

	void sendDetachedInventories(const std::string &peer_name, bool incremental,
			std::function<void(const std::string &, Inventory *)> apply_cb);

protected:
	struct DetachedInventory
	{
		std::unique_ptr<Inventory> inventory;
		std::string owner;
	};

	ServerEnvironment *m_env = nullptr;

	std::unordered_map<std::string, DetachedInventory> m_detached_inventories;
};
