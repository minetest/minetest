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

#include "modchannels.h"
#include <algorithm>

bool ModChannel::register_consumer(u16 peer_id)
{
	// ignore if peer_id already joined
	if (std::find(m_client_consumers.begin(), m_client_consumers.end(), peer_id) !=
			m_client_consumers.end())
		return false;

	m_client_consumers.push_back(peer_id);
	return true;
}

bool ModChannel::remove_consumer(u16 peer_id)
{
	bool found = false;
	auto peer_removal_fct = [peer_id, &found](u16 p) {
		if (p == peer_id)
			found = true;

		return p == peer_id;
	};

	m_client_consumers.erase(
			std::remove_if(m_client_consumers.begin(),
					m_client_consumers.end(), peer_removal_fct),
			m_client_consumers.end());

	return found;
}

bool ModChannelMgr::channel_registered(const std::string &channel) const
{
	return m_registered_channels.find(channel) != m_registered_channels.end();
}

void ModChannelMgr::register_channel(const std::string &channel)
{
	m_registered_channels[channel] = std::unique_ptr<ModChannel>(new ModChannel());
}

bool ModChannelMgr::remove_channel(const std::string &channel)
{
	if (!channel_registered(channel)) {
		return false;
	}

	m_registered_channels.erase(channel);
	return true;
}

bool ModChannelMgr::join_channel(const std::string &channel, u16 peer_id)
{
	if (!channel_registered(channel))
		register_channel(channel);

	return m_registered_channels[channel]->register_consumer(peer_id);
}

bool ModChannelMgr::leave_channel(const std::string &channel, u16 peer_id)
{
	if (!channel_registered(channel))
		return false;

	return m_registered_channels[channel]->remove_consumer(peer_id);
}

void ModChannelMgr::leave_all_channels(u16 peer_id)
{
	for (auto &channel_it : m_registered_channels)
		channel_it.second->remove_consumer(peer_id);
}

static std::vector<u16> empty_channel_list;
const std::vector<u16> &ModChannelMgr::get_channel_peers(const std::string &channel) const
{
	const auto &channel_it = m_registered_channels.find(channel);
	if (channel_it == m_registered_channels.end())
		return empty_channel_list;

	return channel_it->second->get_channel_peers();
}
