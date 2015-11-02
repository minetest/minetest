/*
Minetest
Copyright (C) 2010-2014 celeron55, Perttu Ahola <celeron55@gmail.com>

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

#include <sstream>
#include "clientiface.h"
#include "network/connection.h"
#include "network/serveropcodes.h"
#include "remoteplayer.h"
#include "settings.h"
#include "mapblock.h"
#include "serverenvironment.h"
#include "map.h"
#include "emerge.h"
#include "server/player_sao.h"
#include "log.h"
#include "network/serveropcodes.h"
#include "util/srp.h"

const char *ClientInterface::statenames[] = {
	"Invalid",
	"Disconnecting",
	"Denied",
	"Created",
	"AwaitingInit2",
	"HelloSent",
	"InitDone",
	"DefinitionsSent",
	"Active",
	"SudoMode",
};

std::string ClientInterface::state2Name(ClientState state)
{
	return statenames[state];
}

/*WMSSuggestion RemoteClient::getNextBlock(
		EmergeManager *emerge, ServerFarMap *far_map)*/
WMSSuggestion RemoteClient::getNextBlock(EmergeManager *emerge)
{
	// If the client has not indicated it supports the new algorithm, fill in
	// autosend parameters and things should work fine
	if (m_fallback_autosend_active)
	{
		s16 radius_map = g_settings->getS16("max_block_send_distance");
		s16 radius_far = 0; // Old client does not understand FarBlocks
		float far_weight = 3.0f; // Whatever non-zero
		float fov = 1.72f; // Assume something (radians)
		u32 max_total_mapblocks = 5000; // 5000 is the default client-side setting
		u32 max_total_farblocks = 0;

		// One version of Minetest supports wanted_range and camera_fov via
		// PlayerSAO. Try to use them.
		RemotePlayer *player = m_env->getPlayer(peer_id);
		PlayerSAO *sao = player ? player->getPlayerSAO() : NULL;
		if (sao) {
			// Get view range and camera fov from the client
			s16 wanted_range = sao->getWantedRange();
			float camera_fov = sao->getFov();

			// Update our values to these ones, if they are set
			if (wanted_range > 0 && camera_fov > 0) {
				radius_map = wanted_range;
				fov = camera_fov;
			}
		}

		m_autosend.setParameters(radius_map, radius_far, far_weight, fov,
				max_total_mapblocks, max_total_farblocks);

		// Continue normally.
	}

	/*
		Autosend

		NOTE: All auto-sent stuff is considered higher priority than custom
		transfers. If the client wants to get custom stuff quickly, it has to
		disable autosend.
	*/
	WMSSuggestion wmss = m_autosend.getNextBlock(emerge);
	WantedMapSend wms = wmss.wms;
	if (wms.type != WMST_INVALID)
		return wmss;

	/*
		Handle map send queue as set by the client for custom map transfers

		// TODO: This needs rework
	*/
	for (size_t i=0; i<m_map_send_queue.size(); i++) {
		const WantedMapSend &wms = m_map_send_queue[i];
		if (wms.type == WMST_MAPBLOCK) {
			/*verbosestream << "Server: Client "<<peer_id<<" wants MapBlock ("
					<<wms.p.X<<","<<wms.p.Y<<","<<wms.p.Z<<")"
					<< std::endl;*/

			// Do not go over-limit
			if (blockpos_over_max_limit(wms.p))
				continue;

			// Don't send blocks that are currently being transferred
			if (m_blocks_sending.find(wms) != m_blocks_sending.end())
				continue;

			// Don't send blocks that have already been sent, unless it has been
			// updated
			if (m_mapblocks_sent.find(wms.p) != m_mapblocks_sent.end()) {
				if (m_blocks_updated_since_last_send.count(wms) == 0)
					continue;
			}

			// If the MapBlock is not loaded, it will be queued to be loaded or
			// generated. Otherwise it will be added to 'dest'.

			const bool allow_generate = true;

			MapBlock *block = m_env->getMap().getBlockNoCreateNoEx(wms.p);

			bool surely_not_found_on_disk = false;
			bool emerge_required = false;
			if(block != NULL)
			{
				// Reset usage timer, this block will be of use in the future.
				block->resetUsageTimer();

				// Block is dummy if data doesn't exist.
				// It means it has been not found from disk and not generated
				if(block->isDummy())
					surely_not_found_on_disk = true;

				if(block->isGenerated() == false && allow_generate)
					emerge_required = true;

				// This check is disabled because it mis-guesses sea floors to
				// not be worth transferring to the client, while they are.
				/*if (d >= 4 && block->getDayNightDiff() == false)
					continue;*/
			}

			if(allow_generate == false && surely_not_found_on_disk == true) {
				// NOTE: If we wanted to avoid generating new blocks based on
				// some criterion, that check woould go here and we would call
				// 'continue' if the block should not be generated.

				// TODO: There needs to be some new way or limiting which
				// positions are allowed to be generated, because we aren't
				// going to look up the player's position and compare it with
				// max_mapblock_generate_distance anymore. Maybe a configured set
				// of allowed areas, or maybe a callback to Lua.

				// NOTE: There may need to be a way to tell the client that this
				// block will not be transferred to it no matter how nicely it
				// asks. Otherwise the requested send queue is going to get
				// filled up by these and less important blocks further away
				// that happen to have been already generated will not transfer.
			}

			/*
				If block does not exist, add it to the emerge queue.
			*/
			if(block == NULL || surely_not_found_on_disk || emerge_required)
			{
				bool allow_generate = true;
				if (!emerge->enqueueBlockEmerge(peer_id, wms.p, allow_generate)) {
					// EmergeThread's queue is full; maybe it's not full on the
					// next time this is called.
				}

				// This block is not available now; hopefully it appears on some
				// next call to this function.
				continue;
			}

			// The block is available
			return wms;
		}
		if (wms.type == WMST_FARBLOCK) {
			// Not supported; always discard
			continue;
		}
	}

	return WantedMapSend();
}

void RemoteClient::cycleAutosendAlgorithm(float dtime)
{
	m_time_from_building += dtime;
	m_autosend.cycle(dtime, m_env);
}

void RemoteClient::GotBlock(const WantedMapSend &wms)
{
	if (m_blocks_sending.find(wms) != m_blocks_sending.end()) {
		if (wms.type == WMST_MAPBLOCK)
			m_mapblocks_sent[wms.p] = m_blocks_sending[wms];
		else if (wms.type == WMST_MAPBLOCK)
			m_farblocks_sent[wms.p] = m_blocks_sending[wms];
		m_blocks_sending.erase(wms);
	} else {
		m_excess_gotblocks++;
	}
}

void RemoteClient::SendingBlock(const WantedMapSend &wms)
{
	assert(!m_blocks_sending.count(wms) ||
			m_blocks_updated_while_sending.count(wms) &&
			"Block shouldn't be re-sent if it is being sent and was not"
			" updated while being sent");

	m_blocks_sending[wms] = time(NULL);
	m_blocks_updated_since_last_send.erase(wms);
	m_blocks_updated_while_sending.erase(wms);
}

void RemoteClient::SetBlockUpdated(const WantedMapSend &wms)
{
	//dstream<<"SetBlockUpdated: "<<wms.describe()<<std::endl;

	// The client does not have a recent version of this block so it has to be
	// considered "not sent". This function is also called when the client
	// deletes the block from memory.
	if (wms.type == WMST_MAPBLOCK)
		m_mapblocks_sent.erase(wms.p);
	else if (wms.type == WMST_FARBLOCK)
		m_farblocks_sent.erase(wms.p);

	// Reset autosend's search radius if it's a MapBlock
	if (wms.type == WMST_MAPBLOCK) {
		m_autosend.resetMapblockSearchRadius(wms.p);
	}

	m_blocks_updated_since_last_send.insert(wms);

	if (m_blocks_sending.count(wms)) {
		m_blocks_updated_while_sending.insert(wms);
	}
}

void RemoteClient::SetMapBlockUpdated(v3s16 p)
{
	SetBlockUpdated(WantedMapSend(WMST_MAPBLOCK, p));

	// Also set the corresponding FarBlock not sent
	v3s16 farblock_p = getContainerPos(p, FMP_SCALE);
	SetBlockUpdated(WantedMapSend(WMST_FARBLOCK, farblock_p));

	/*dstream<<"RemoteClient: now not sent: MB"<<"("<<p.X<<","<<p.Y<<","<<p.Z<<")"
			<<" FB"<<"("<<farblock_p.X<<","<<farblock_p.Y<<","<<farblock_p.Z<<")"
			<<std::endl;*/
}

void RemoteClient::SetMapBlocksUpdated(std::map<v3s16, MapBlock*> &blocks)
{
	for(std::map<v3s16, MapBlock*>::iterator
			i = blocks.begin();
			i != blocks.end(); ++i)
	{
		v3s16 p = i->first;
		SetMapBlockUpdated(p);
	}
}

void RemoteClient::ResendBlockIfOnWire(const WantedMapSend &wms)
{
	// if this block is on wire, mark it for sending again as soon as possible
	if (m_blocks_sending.find(wms) != m_blocks_sending.end()) {
		SetBlockUpdated(wms);
	}
}

void RemoteClient::notifyEvent(ClientStateEvent event)
{
	std::ostringstream myerror;
	switch (m_state)
	{
	case CS_Invalid:
		//intentionally do nothing
		break;
	case CS_Created:
		switch (event) {
		case CSE_Hello:
			m_state = CS_HelloSent;
			break;
		case CSE_Disconnect:
			m_state = CS_Disconnecting;
			break;
		case CSE_SetDenied:
			m_state = CS_Denied;
			break;
		/* GotInit2 SetDefinitionsSent SetMediaSent */
		default:
			myerror << "Created: Invalid client state transition! " << event;
			throw ClientStateError(myerror.str());
		}
		break;
	case CS_Denied:
		/* don't do anything if in denied state */
		break;
	case CS_HelloSent:
		switch(event)
		{
		case CSE_AuthAccept:
			m_state = CS_AwaitingInit2;
			resetChosenMech();
			break;
		case CSE_Disconnect:
			m_state = CS_Disconnecting;
			break;
		case CSE_SetDenied:
			m_state = CS_Denied;
			resetChosenMech();
			break;
		default:
			myerror << "HelloSent: Invalid client state transition! " << event;
			throw ClientStateError(myerror.str());
		}
		break;
	case CS_AwaitingInit2:
		switch(event)
		{
		case CSE_GotInit2:
			confirmSerializationVersion();
			m_state = CS_InitDone;
			break;
		case CSE_Disconnect:
			m_state = CS_Disconnecting;
			break;
		case CSE_SetDenied:
			m_state = CS_Denied;
			break;

		/* Init SetDefinitionsSent SetMediaSent */
		default:
			myerror << "InitSent: Invalid client state transition! " << event;
			throw ClientStateError(myerror.str());
		}
		break;

	case CS_InitDone:
		switch(event)
		{
		case CSE_SetDefinitionsSent:
			m_state = CS_DefinitionsSent;
			break;
		case CSE_Disconnect:
			m_state = CS_Disconnecting;
			break;
		case CSE_SetDenied:
			m_state = CS_Denied;
			break;

		/* Init GotInit2 SetMediaSent */
		default:
			myerror << "InitDone: Invalid client state transition! " << event;
			throw ClientStateError(myerror.str());
		}
		break;
	case CS_DefinitionsSent:
		switch(event)
		{
		case CSE_SetClientReady:
			m_state = CS_Active;
			break;
		case CSE_Disconnect:
			m_state = CS_Disconnecting;
			break;
		case CSE_SetDenied:
			m_state = CS_Denied;
			break;
		/* Init GotInit2 SetDefinitionsSent */
		default:
			myerror << "DefinitionsSent: Invalid client state transition! " << event;
			throw ClientStateError(myerror.str());
		}
		break;
	case CS_Active:
		switch(event)
		{
		case CSE_SetDenied:
			m_state = CS_Denied;
			break;
		case CSE_Disconnect:
			m_state = CS_Disconnecting;
			break;
		case CSE_SudoSuccess:
			m_state = CS_SudoMode;
			resetChosenMech();
			break;
		/* Init GotInit2 SetDefinitionsSent SetMediaSent SetDenied */
		default:
			myerror << "Active: Invalid client state transition! " << event;
			throw ClientStateError(myerror.str());
			break;
		}
		break;
	case CS_SudoMode:
		switch(event)
		{
		case CSE_SetDenied:
			m_state = CS_Denied;
			break;
		case CSE_Disconnect:
			m_state = CS_Disconnecting;
			break;
		case CSE_SudoLeave:
			m_state = CS_Active;
			break;
		default:
			myerror << "Active: Invalid client state transition! " << event;
			throw ClientStateError(myerror.str());
			break;
		}
		break;
	case CS_Disconnecting:
		/* we are already disconnecting */
		break;
	}
}

void RemoteClient::resetChosenMech()
{
	if (auth_data) {
		srp_verifier_delete((SRPVerifier *) auth_data);
		auth_data = nullptr;
	}
	chosen_mech = AUTH_MECHANISM_NONE;
}

u64 RemoteClient::uptime() const
{
	return porting::getTimeS() - m_connection_time;
}

ClientInterface::ClientInterface(const std::shared_ptr<con::Connection> & con)
:
	m_con(con),
	m_env(NULL),
	m_print_info_timer(0.0f)
{

}
ClientInterface::~ClientInterface()
{
	/*
		Delete clients
	*/
	{
		RecursiveMutexAutoLock clientslock(m_clients_mutex);

		for (auto &client_it : m_clients) {
			// Delete client
			delete client_it.second;
		}
	}
}

std::vector<session_t> ClientInterface::getClientIDs(ClientState min_state)
{
	std::vector<session_t> reply;
	RecursiveMutexAutoLock clientslock(m_clients_mutex);

	for (const auto &m_client : m_clients) {
		if (m_client.second->getState() >= min_state)
			reply.push_back(m_client.second->peer_id);
	}

	return reply;
}

void ClientInterface::markBlockposAsNotSent(const v3s16 &pos)
{
	RecursiveMutexAutoLock clientslock(m_clients_mutex);
	for (const auto &client : m_clients) {
		if (client.second->getState() >= CS_Active)
			client.second->SetMapBlockUpdated(pos);
	}
}

/**
 * Verify if user limit was reached.
 * User limit count all clients from HelloSent state (MT protocol user) to Active state
 * @return true if user limit was reached
 */
bool ClientInterface::isUserLimitReached()
{
	return getClientIDs(CS_HelloSent).size() >= g_settings->getU16("max_users");
}

void ClientInterface::step(float dtime)
{
	m_print_info_timer += dtime;
	if (m_print_info_timer >= 30.0f) {
		m_print_info_timer = 0.0f;
		UpdatePlayerList();
	}
}

void ClientInterface::UpdatePlayerList()
{
	if (m_env) {
		std::vector<session_t> clients = getClientIDs();
		m_clients_names.clear();

		if (!clients.empty())
			infostream<<"Players:"<<std::endl;

		for (session_t i : clients) {
			RemotePlayer *player = m_env->getPlayer(i);

			if (player == NULL)
				continue;

			infostream << "* " << player->getName() << "\t";

			{
				RecursiveMutexAutoLock clientslock(m_clients_mutex);
				RemoteClient* client = lockedGetClientNoEx(i);
				if (client)
					client->PrintInfo(infostream);
			}

			m_clients_names.emplace_back(player->getName());
		}
	}
}

void ClientInterface::send(session_t peer_id, u8 channelnum,
		NetworkPacket *pkt, bool reliable)
{
	m_con->Send(peer_id, channelnum, pkt, reliable);
}

void ClientInterface::sendToAll(NetworkPacket *pkt)
{
	RecursiveMutexAutoLock clientslock(m_clients_mutex);
	for (auto &client_it : m_clients) {
		RemoteClient *client = client_it.second;

		if (client->net_proto_version != 0) {
			m_con->Send(client->peer_id,
					clientCommandFactoryTable[pkt->getCommand()].channel, pkt,
					clientCommandFactoryTable[pkt->getCommand()].reliable);
		}
	}
}

RemoteClient* ClientInterface::getClientNoEx(session_t peer_id, ClientState state_min)
{
	RecursiveMutexAutoLock clientslock(m_clients_mutex);
	RemoteClientMap::const_iterator n = m_clients.find(peer_id);
	// The client may not exist; clients are immediately removed if their
	// access is denied, and this event occurs later then.
	if (n == m_clients.end())
		return NULL;

	if (n->second->getState() >= state_min)
		return n->second;

	return NULL;
}

RemoteClient* ClientInterface::lockedGetClientNoEx(session_t peer_id, ClientState state_min)
{
	RemoteClientMap::const_iterator n = m_clients.find(peer_id);
	// The client may not exist; clients are immediately removed if their
	// access is denied, and this event occurs later then.
	if (n == m_clients.end())
		return NULL;

	if (n->second->getState() >= state_min)
		return n->second;

	return NULL;
}

ClientState ClientInterface::getClientState(session_t peer_id)
{
	RecursiveMutexAutoLock clientslock(m_clients_mutex);
	RemoteClientMap::const_iterator n = m_clients.find(peer_id);
	// The client may not exist; clients are immediately removed if their
	// access is denied, and this event occurs later then.
	if (n == m_clients.end())
		return CS_Invalid;

	return n->second->getState();
}

void ClientInterface::setPlayerName(session_t peer_id, const std::string &name)
{
	RecursiveMutexAutoLock clientslock(m_clients_mutex);
	RemoteClientMap::iterator n = m_clients.find(peer_id);
	// The client may not exist; clients are immediately removed if their
	// access is denied, and this event occurs later then.
	if (n != m_clients.end())
		n->second->setName(name);
}

void ClientInterface::DeleteClient(session_t peer_id)
{
	RecursiveMutexAutoLock conlock(m_clients_mutex);

	// Error check
	RemoteClientMap::iterator n = m_clients.find(peer_id);
	// The client may not exist; clients are immediately removed if their
	// access is denied, and this event occurs later then.
	if (n == m_clients.end())
		return;

	/*
		Mark objects to be not known by the client
	*/
	//TODO this should be done by client destructor!!!
	RemoteClient *client = n->second;
	// Handle objects
	for (u16 id : client->m_known_objects) {
		// Get object
		ServerActiveObject* obj = m_env->getActiveObject(id);

		if(obj && obj->m_known_by_count > 0)
			obj->m_known_by_count--;
	}

	// Delete client
	delete m_clients[peer_id];
	m_clients.erase(peer_id);
}

void ClientInterface::CreateClient(session_t peer_id)
{
	RecursiveMutexAutoLock conlock(m_clients_mutex);

	// Error check
	RemoteClientMap::iterator n = m_clients.find(peer_id);
	// The client shouldn't already exist
	if (n != m_clients.end()) return;

	// Create client
	RemoteClient *client = new RemoteClient(m_env);
	client->peer_id = peer_id;
	m_clients[client->peer_id] = client;
}

void ClientInterface::event(session_t peer_id, ClientStateEvent event)
{
	{
		RecursiveMutexAutoLock clientlock(m_clients_mutex);

		// Error check
		RemoteClientMap::iterator n = m_clients.find(peer_id);

		// No client to deliver event
		if (n == m_clients.end())
			return;
		n->second->notifyEvent(event);
	}

	if ((event == CSE_SetClientReady) ||
		(event == CSE_Disconnect)     ||
		(event == CSE_SetDenied))
	{
		UpdatePlayerList();
	}
}

u16 ClientInterface::getProtocolVersion(session_t peer_id)
{
	RecursiveMutexAutoLock conlock(m_clients_mutex);

	// Error check
	RemoteClientMap::iterator n = m_clients.find(peer_id);

	// No client to get version
	if (n == m_clients.end())
		return 0;

	return n->second->net_proto_version;
}

void ClientInterface::setClientVersion(session_t peer_id, u8 major, u8 minor, u8 patch,
		const std::string &full)
{
	RecursiveMutexAutoLock conlock(m_clients_mutex);

	// Error check
	RemoteClientMap::iterator n = m_clients.find(peer_id);

	// No client to set versions
	if (n == m_clients.end())
		return;

	n->second->setVersionInfo(major, minor, patch, full);
}
