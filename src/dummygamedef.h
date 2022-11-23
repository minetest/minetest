/*
Minetest
Copyright (C) 2013 celeron55, Perttu Ahola <celeron55@gmail.com>
Copyright (C) 2022 TurkeyMcMac, Jude Melton-Houghton <jwmhjwmh@gmail.com>

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

#include "gamedef.h"
#include "itemdef.h"
#include "nodedef.h"
#include "craftdef.h"
#include "content/mods.h"
#include "database/database-dummy.h"

class DummyGameDef : public IGameDef {
public:
	DummyGameDef():
		m_itemdef(createItemDefManager()),
		m_nodedef(createNodeDefManager()),
		m_craftdef(createCraftDefManager()),
		m_mod_storage_database(new Database_Dummy())
	{
	}

	~DummyGameDef()
	{
		delete m_mod_storage_database;
		delete m_craftdef;
		delete m_nodedef;
		delete m_itemdef;
	}

	IItemDefManager *getItemDefManager() override { return m_itemdef; }
	const NodeDefManager *getNodeDefManager() override { return m_nodedef; }
	NodeDefManager* getWritableNodeDefManager() { return m_nodedef; }
	ICraftDefManager *getCraftDefManager() override { return m_craftdef; }

	u16 allocateUnknownNodeId(const std::string &name) override
	{
		return m_nodedef->allocateDummy(name);
	}

	const std::vector<ModSpec> &getMods() const override
	{
		static std::vector<ModSpec> emptymodspec;
		return emptymodspec;
	}
	const ModSpec* getModSpec(const std::string &modname) const override { return nullptr; }
	ModStorageDatabase *getModStorageDatabase() override { return m_mod_storage_database; }

	bool joinModChannel(const std::string &channel) override { return false; }
	bool leaveModChannel(const std::string &channel) override { return false; }
	bool sendModChannelMessage(const std::string &channel, const std::string &message) override
	{
		return false;
	}
	ModChannel *getModChannel(const std::string &channel) override { return nullptr; }

protected:
	IItemDefManager *m_itemdef = nullptr;
	NodeDefManager *m_nodedef = nullptr;
	ICraftDefManager *m_craftdef = nullptr;
	ModStorageDatabase *m_mod_storage_database = nullptr;
};
