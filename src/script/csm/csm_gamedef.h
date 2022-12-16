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

#pragma once

#include "gamedef.h"
#include "csm_message.h"

class IWritableItemDefManager;

class CSMGameDef : public IGameDef
{
public:
	CSMGameDef(FILE *from_controller, FILE *to_controller);
	~CSMGameDef();

	CSMRecvMsg recvEx()
	{
		return csm_recv_msg_ex(m_from_controller);
	}
	void sendEx(CSMMsgType type, size_t size = 0, const void *data = nullptr)
	{
		csm_send_msg_ex(m_to_controller, type, size, data);
	}

	IItemDefManager *getItemDefManager() override;
	const NodeDefManager *getNodeDefManager() override;
	ICraftDefManager *getCraftDefManager() override;
	IWritableItemDefManager *getWritableItemDefManager();
	NodeDefManager *getWritableNodeDefManager();

	u16 allocateUnknownNodeId(const std::string &name) override;

	const std::vector<ModSpec> &getMods() const override;
	const ModSpec *getModSpec(const std::string &modname) const override;
	ModStorageDatabase *getModStorageDatabase() override;

	bool joinModChannel(const std::string &channel) override;
	bool leaveModChannel(const std::string &channel) override;
	bool sendModChannelMessage(const std::string &channel,
			const std::string &message) override;
	ModChannel *getModChannel(const std::string &channel) override;

private:
	IWritableItemDefManager *m_itemdef;
	NodeDefManager *m_nodedef;
	FILE *m_from_controller;
	FILE *m_to_controller;
};
