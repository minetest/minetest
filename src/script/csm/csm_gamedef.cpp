/*
Minetest
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

#include "csm_gamedef.h"
#include "itemdef.h"
#include "nodedef.h"
#include "content/mods.h"

CSMGameDef::CSMGameDef():
	m_itemdef(createItemDefManager()),
	m_nodedef(createNodeDefManager())
{
}

CSMGameDef::~CSMGameDef()
{
	delete m_nodedef;
	delete m_itemdef;
}

IItemDefManager *CSMGameDef::getItemDefManager()
{
	return m_itemdef;
}

const NodeDefManager *CSMGameDef::getNodeDefManager()
{
	return m_nodedef;
}

ICraftDefManager *CSMGameDef::getCraftDefManager()
{
	return nullptr;
}

IWritableItemDefManager *CSMGameDef::getWritableItemDefManager()
{
	return m_itemdef;
}

NodeDefManager *CSMGameDef::getWritableNodeDefManager()
{
	return m_nodedef;
}

u16 CSMGameDef::allocateUnknownNodeId(const std::string &name)
{
	return CONTENT_IGNORE;
}

const std::vector<ModSpec> &CSMGameDef::getMods() const
{
	static const std::vector<ModSpec> mods;
	return mods;
}

const ModSpec *CSMGameDef::getModSpec(const std::string &modname) const
{
	return nullptr;
}

ModStorageDatabase *CSMGameDef::getModStorageDatabase()
{
	return nullptr;
}

bool CSMGameDef::joinModChannel(const std::string &channel)
{
	return false;
}

bool CSMGameDef::leaveModChannel(const std::string &channel)
{
	return false;
}

bool CSMGameDef::sendModChannelMessage(const std::string &channel,
			const std::string &message)
{
	return false;
}

ModChannel *CSMGameDef::getModChannel(const std::string &channel)
{
	return nullptr;
}
