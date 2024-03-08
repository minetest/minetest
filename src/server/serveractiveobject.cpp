/*
Minetest
Copyright (C) 2010-2013 celeron55, Perttu Ahola <celeron55@gmail.com>

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

#include "serveractiveobject.h"
#include <algorithm>
#include <cstddef>
#include <cstdio>
#include <fstream>
#include <unordered_map>
#include "inventory.h"
#include "inventorymanager.h"
#include "constants.h" // BS
#include "log.h"

using SAO = ServerActiveObject;

SAO::ServerActiveObject(ServerEnvironment *env, v3f pos):
	ActiveObject(0),
	m_env(env),
	m_base_position(pos)
{
}

float SAO::getMinimumSavedMovement()
{
	return 2.0*BS;
}

ItemStack SAO::getWieldedItem(ItemStack *selected, ItemStack *hand) const
{
	*selected = ItemStack();
	if (hand)
		*hand = ItemStack();

	return ItemStack();
}

bool SAO::setWieldedItem(const ItemStack &item)
{
	return false;
}

std::string SAO::generateUpdateInfantCommand(u16 infant_id, u16 protocol_version)
{
	std::ostringstream os(std::ios::binary);
	// command
	writeU8(os, AO_CMD_SPAWN_INFANT);
	// parameters
	writeU16(os, infant_id);
	writeU8(os, getSendType());
	if (protocol_version < 38) {
		// Clients since 4aa9a66 so no longer need this data
		// Version 38 is the first bump after that commit.
		// See also: ClientEnvironment::addActiveObject
		os << serializeString32(getClientInitializationData(protocol_version));
	}
	return os.str();
}

void SAO::dumpAOMessagesToQueue(std::queue<ActiveObjectMessage> &queue)
{
	while (!m_messages_out.empty()) {
		queue.push(std::move(m_messages_out.front()));
		m_messages_out.pop();
	}
}

void SAO::markForRemoval()
{
	if (!m_pending_removal) {
		onMarkedForRemoval();
		m_pending_removal = true;
	}
}

void SAO::markForDeactivation()
{
	if (!m_pending_deactivation) {
		onMarkedForDeactivation();
		m_pending_deactivation = true;
	}
}

InventoryLocation SAO::getInventoryLocation() const
{
	return InventoryLocation();
}

void SAO::invalidateEffectiveObservers()
{
	m_effective_observers.reset();
}

const SAO::Observers &SAO::getEffectiveObservers()
{
	if (m_effective_observers) // cached
		return *m_effective_observers;

	auto parent = getParent();
	if (parent == nullptr)
		return *(m_effective_observers = m_observers);
	auto parent_observers = parent->getEffectiveObservers();
	if (!parent_observers) // parent is unmanaged
		return *(m_effective_observers = m_observers);
	if (!m_observers) // we are unmanaged
		return *(m_effective_observers = parent_observers);
	// Set intersection between parent_observers and m_observers
	m_effective_observers = std::unordered_set<std::string>();
	for (const auto &observer_name : *m_observers) {
		if (parent_observers->count(observer_name) > 0)
			(*m_effective_observers)->insert(observer_name);
	}
	return *m_effective_observers;
}

const SAO::Observers &SAO::recalculateEffectiveObservers()
{
	// Invalidate final observers for this object and all of its parents.
	for (auto obj = this; obj != nullptr; obj = obj->getParent())
		obj->invalidateEffectiveObservers();
	// getEffectiveObservers will now be forced to recalculate.
	return getEffectiveObservers();
}

bool SAO::isEffectivelyObservedBy(const std::string &player_name)
{
	auto effective_observers = getEffectiveObservers();
	return !effective_observers || effective_observers->count(player_name) > 0;
}
