/*
Minetest
Copyright (C) 2017 nerzhul, Loic Blot <loic.blot@unix-experience.fr>

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

#include <unordered_map>
#include <string>
#include <vector>
#include <memory>
#include "irrlichttypes.h"

class ModChannel
{
public:
	ModChannel() = default;
	~ModChannel() = default;

	bool register_consumer(u16 peer_id);
	bool remove_consumer(u16 peer_id);
	const std::vector<u16> &get_channel_peers() const { return m_client_consumers; }

private:
	std::vector<u16> m_client_consumers;
};

enum ModChannelSignal : u8
{
	MODCHANNEL_SIGNAL_JOIN_OK,
	MODCHANNEL_SIGNAL_JOIN_FAILURE,
	MODCHANNEL_SIGNAL_LEAVE_OK,
	MODCHANNEL_SIGNAL_LEAVE_FAILURE,
	MODCHANNEL_SIGNAL_CHANNEL_NOT_REGISTERED,
};

class ModChannelMgr
{
public:
	ModChannelMgr() = default;
	~ModChannelMgr() = default;

	void register_channel(const std::string &channel);
	bool remove_channel(const std::string &channel);
	bool join_channel(const std::string &channel, u16 peer_id);
	bool leave_channel(const std::string &channel, u16 peer_id);
	bool channel_registered(const std::string &channel) const;
	void leave_all_channels(u16 peer_id);
	const std::vector<u16> &get_channel_peers(const std::string &channel) const;

private:
	std::unordered_map<std::string, std::unique_ptr<ModChannel>>
			m_registered_channels;
};
