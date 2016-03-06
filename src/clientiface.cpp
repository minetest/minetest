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
#include "util/numeric.h"
#include "remoteplayer.h"
#include "settings.h"
#include "mapblock.h"
#include "network/connection.h"
#include "serverenvironment.h"
#include "map.h"
#include "emerge.h"
#include "content_sao.h"              // TODO this is used for cleanup of only
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

void RemoteClient::ResendBlockIfOnWire(v3s16 p)
{
	// if this block is on wire, mark it for sending again as soon as possible
	if (m_blocks_sending.find(p) != m_blocks_sending.end()) {
		SetBlockNotSent(p);
	}
}

void RemoteClient::GetNextBlocks (
		ServerEnvironment *env,
		EmergeManager * emerge,
		float dtime,
		std::vector<PrioritySortedBlockTransfer> &dest)
{
	DSTACK(FUNCTION_NAME);


	// Increment timers
	m_nothing_to_send_pause_timer -= dtime;
	m_nearest_unsent_reset_timer += dtime;

	if(m_nothing_to_send_pause_timer >= 0)
		return;

	RemotePlayer *player = env->getPlayer(peer_id);
	// This can happen sometimes; clients and players are not in perfect sync.
	if (player == NULL)
		return;

	PlayerSAO *sao = player->getPlayerSAO();
	if (sao == NULL)
		return;

	// Won't send anything if already sending
	if(m_blocks_sending.size() >= g_settings->getU16
			("max_simultaneous_block_sends_per_client"))
	{
		//infostream<<"Not sending any blocks, Queue full."<<std::endl;
		return;
	}

	v3f playerpos = sao->getBasePosition();
	v3f playerspeed = player->getSpeed();
	v3f playerspeeddir(0,0,0);
	if(playerspeed.getLength() > 1.0*BS)
		playerspeeddir = playerspeed / playerspeed.getLength();
	// Predict to next block
	v3f playerpos_predicted = playerpos + playerspeeddir*MAP_BLOCKSIZE*BS;

	v3s16 center_nodepos = floatToInt(playerpos_predicted, BS);

	v3s16 center = getNodeBlockPos(center_nodepos);

	// Camera position and direction
	v3f camera_pos = sao->getEyePosition();
	v3f camera_dir = v3f(0,0,1);
	camera_dir.rotateYZBy(sao->getPitch());
	camera_dir.rotateXZBy(sao->getYaw());

	/*infostream<<"camera_dir=("<<camera_dir.X<<","<<camera_dir.Y<<","
			<<camera_dir.Z<<")"<<std::endl;*/

	/*
		Get the starting value of the block finder radius.
	*/

	if(m_last_center != center)
	{
		m_nearest_unsent_d = 0;
		m_last_center = center;
	}

	/*infostream<<"m_nearest_unsent_reset_timer="
			<<m_nearest_unsent_reset_timer<<std::endl;*/

	// Reset periodically to workaround for some bugs or stuff
	if(m_nearest_unsent_reset_timer > 20.0)
	{
		m_nearest_unsent_reset_timer = 0;
		m_nearest_unsent_d = 0;
		//infostream<<"Resetting m_nearest_unsent_d for "
		//		<<server->getPlayerName(peer_id)<<std::endl;
	}

	//s16 last_nearest_unsent_d = m_nearest_unsent_d;
	s16 d_start = m_nearest_unsent_d;

	//infostream<<"d_start="<<d_start<<std::endl;

	u16 max_simul_sends_setting = g_settings->getU16
			("max_simultaneous_block_sends_per_client");
	u16 max_simul_sends_usually = max_simul_sends_setting;

	/*
		Check the time from last addNode/removeNode.

		Decrease send rate if player is building stuff.
	*/
	m_time_from_building += dtime;
	if(m_time_from_building < g_settings->getFloat(
				"full_block_send_enable_min_time_from_building"))
	{
		max_simul_sends_usually
			= LIMITED_MAX_SIMULTANEOUS_BLOCK_SENDS;
	}

	/*
		Number of blocks sending + number of blocks selected for sending
	*/
	u32 num_blocks_selected = m_blocks_sending.size();

	/*
		next time d will be continued from the d from which the nearest
		unsent block was found this time.

		This is because not necessarily any of the blocks found this
		time are actually sent.
	*/
	s32 new_nearest_unsent_d = -1;

	// get view range and camera fov from the client
	s16 wanted_range = sao->getWantedRange();
	float camera_fov = sao->getFov();
	// if FOV, wanted_range are not available (old client), fall back to old default
	if (wanted_range <= 0) wanted_range = 1000;
	if (camera_fov <= 0) camera_fov = (72.0*M_PI/180) * 4./3.;

	const s16 full_d_max = MYMIN(g_settings->getS16("max_block_send_distance"), wanted_range);
	const s16 d_opt = MYMIN(g_settings->getS16("block_send_optimize_distance"), wanted_range);
	const s16 d_blocks_in_sight = full_d_max * BS * MAP_BLOCKSIZE;
	//infostream << "Fov from client " << camera_fov << " full_d_max " << full_d_max << std::endl;

	s16 d_max = full_d_max;
	s16 d_max_gen = MYMIN(g_settings->getS16("max_block_generate_distance"), wanted_range);

	// Don't loop very much at a time
	s16 max_d_increment_at_time = 2;
	if(d_max > d_start + max_d_increment_at_time)
		d_max = d_start + max_d_increment_at_time;

	s32 nearest_emerged_d = -1;
	s32 nearest_emergefull_d = -1;
	s32 nearest_sent_d = -1;
	//bool queue_is_full = false;

	const v3s16 cam_pos_nodes = floatToInt(camera_pos, BS);
	const bool occ_cull = g_settings->getBool("server_side_occlusion_culling");

	s16 d;
	for(d = d_start; d <= d_max; d++) {
		/*
			Get the border/face dot coordinates of a "d-radiused"
			box
		*/
		std::vector<v3s16> list = FacePositionCache::getFacePositions(d);

		std::vector<v3s16>::iterator li;
		for(li = list.begin(); li != list.end(); ++li) {
			v3s16 p = *li + center;

			/*
				Send throttling
				- Don't allow too many simultaneous transfers
				- EXCEPT when the blocks are very close

				Also, don't send blocks that are already flying.
			*/

			// Start with the usual maximum
			u16 max_simul_dynamic = max_simul_sends_usually;

			// If block is very close, allow full maximum
			if(d <= BLOCK_SEND_DISABLE_LIMITS_MAX_D)
				max_simul_dynamic = max_simul_sends_setting;

			// Don't select too many blocks for sending
			if (num_blocks_selected >= max_simul_dynamic) {
				//queue_is_full = true;
				goto queue_full_break;
			}

			// Don't send blocks that are currently being transferred
			if (m_blocks_sending.find(p) != m_blocks_sending.end())
				continue;

			/*
				Do not go over max mapgen limit
			*/
			if (blockpos_over_max_limit(p))
				continue;

			// If this is true, inexistent block will be made from scratch
			bool generate = d <= d_max_gen;

			/*
				Don't generate or send if not in sight
				FIXME This only works if the client uses a small enough
				FOV setting. The default of 72 degrees is fine.
			*/

			f32 dist;
			if (!isBlockInSight(p, camera_pos, camera_dir, camera_fov, d_blocks_in_sight, &dist)) {
				continue;
			}

			/*
				Don't send already sent blocks
			*/
			{
				if(m_blocks_sent.find(p) != m_blocks_sent.end())
				{
					continue;
				}
			}

			/*
				Check if map has this block
			*/
			MapBlock *block = env->getMap().getBlockNoCreateNoEx(p);

			bool surely_not_found_on_disk = false;
			bool block_is_invalid = false;
			if(block != NULL)
			{
				// Reset usage timer, this block will be of use in the future.
				block->resetUsageTimer();

				// Block is dummy if data doesn't exist.
				// It means it has been not found from disk and not generated
				if(block->isDummy())
				{
					surely_not_found_on_disk = true;
				}

				if(block->isGenerated() == false)
					block_is_invalid = true;

				/*
					If block is not close, don't send it unless it is near
					ground level.

					Block is near ground level if night-time mesh
					differs from day-time mesh.
				*/
				if(d >= d_opt)
				{
					if(block->getDayNightDiff() == false)
						continue;
				}

				if (occ_cull && !block_is_invalid &&
						env->getMap().isBlockOccluded(block, cam_pos_nodes)) {
					continue;
				}
			}

			/*
				If block has been marked to not exist on disk (dummy)
				and generating new ones is not wanted, skip block.
			*/
			if(generate == false && surely_not_found_on_disk == true)
			{
				// get next one.
				continue;
			}

			/*
				Add inexistent block to emerge queue.
			*/
			if(block == NULL || surely_not_found_on_disk || block_is_invalid)
			{
				if (emerge->enqueueBlockEmerge(peer_id, p, generate)) {
					if (nearest_emerged_d == -1)
						nearest_emerged_d = d;
				} else {
					if (nearest_emergefull_d == -1)
						nearest_emergefull_d = d;
					goto queue_full_break;
				}

				// get next one.
				continue;
			}

			if(nearest_sent_d == -1)
				nearest_sent_d = d;

			/*
				Add block to send queue
			*/
			PrioritySortedBlockTransfer q((float)dist, p, peer_id);

			dest.push_back(q);

			num_blocks_selected += 1;
		}
	}
queue_full_break:

	// If nothing was found for sending and nothing was queued for
	// emerging, continue next time browsing from here
	if(nearest_emerged_d != -1){
		new_nearest_unsent_d = nearest_emerged_d;
	} else if(nearest_emergefull_d != -1){
		new_nearest_unsent_d = nearest_emergefull_d;
	} else {
		if(d > full_d_max){
			new_nearest_unsent_d = 0;
			m_nothing_to_send_pause_timer = 2.0;
		} else {
			if(nearest_sent_d != -1)
				new_nearest_unsent_d = nearest_sent_d;
			else
				new_nearest_unsent_d = d;
		}
	}

	if(new_nearest_unsent_d != -1)
		m_nearest_unsent_d = new_nearest_unsent_d;
}

void RemoteClient::GotBlock(v3s16 p)
{
	if (m_blocks_modified.find(p) == m_blocks_modified.end()) {
		if (m_blocks_sending.find(p) != m_blocks_sending.end())
			m_blocks_sending.erase(p);
		else
			m_excess_gotblocks++;

		m_blocks_sent.insert(p);
	}
}

void RemoteClient::SentBlock(v3s16 p)
{
	if (m_blocks_modified.find(p) != m_blocks_modified.end())
		m_blocks_modified.erase(p);

	if(m_blocks_sending.find(p) == m_blocks_sending.end())
		m_blocks_sending[p] = 0.0;
	else
		infostream<<"RemoteClient::SentBlock(): Sent block"
				" already in m_blocks_sending"<<std::endl;
}

void RemoteClient::SetBlockNotSent(v3s16 p)
{
	m_nearest_unsent_d = 0;
	m_nothing_to_send_pause_timer = 0;

	if(m_blocks_sending.find(p) != m_blocks_sending.end())
		m_blocks_sending.erase(p);
	if(m_blocks_sent.find(p) != m_blocks_sent.end())
		m_blocks_sent.erase(p);
	m_blocks_modified.insert(p);
}

void RemoteClient::SetBlocksNotSent(std::map<v3s16, MapBlock*> &blocks)
{
	m_nearest_unsent_d = 0;
	m_nothing_to_send_pause_timer = 0;

	for(std::map<v3s16, MapBlock*>::iterator
			i = blocks.begin();
			i != blocks.end(); ++i)
	{
		v3s16 p = i->first;
		m_blocks_modified.insert(p);

		if(m_blocks_sending.find(p) != m_blocks_sending.end())
			m_blocks_sending.erase(p);
		if(m_blocks_sent.find(p) != m_blocks_sent.end())
			m_blocks_sent.erase(p);
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
		case CSE_InitLegacy:
			m_state = CS_AwaitingInit2;
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
			if ((chosen_mech == AUTH_MECHANISM_SRP)
					|| (chosen_mech == AUTH_MECHANISM_LEGACY_PASSWORD))
				srp_verifier_delete((SRPVerifier *) auth_data);
			chosen_mech = AUTH_MECHANISM_NONE;
			break;
		case CSE_Disconnect:
			m_state = CS_Disconnecting;
			break;
		case CSE_SetDenied:
			m_state = CS_Denied;
			if ((chosen_mech == AUTH_MECHANISM_SRP)
					|| (chosen_mech == AUTH_MECHANISM_LEGACY_PASSWORD))
				srp_verifier_delete((SRPVerifier *) auth_data);
			chosen_mech = AUTH_MECHANISM_NONE;
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
			if ((chosen_mech == AUTH_MECHANISM_SRP)
					|| (chosen_mech == AUTH_MECHANISM_LEGACY_PASSWORD))
				srp_verifier_delete((SRPVerifier *) auth_data);
			chosen_mech = AUTH_MECHANISM_NONE;
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

u32 RemoteClient::uptime()
{
	return porting::getTime(PRECISION_SECONDS) - m_connection_time;
}

ClientInterface::ClientInterface(con::Connection* con)
:
	m_con(con),
	m_env(NULL),
	m_print_info_timer(0.0)
{

}
ClientInterface::~ClientInterface()
{
	/*
		Delete clients
	*/
	{
		MutexAutoLock clientslock(m_clients_mutex);

		for (UNORDERED_MAP<u16, RemoteClient*>::iterator i = m_clients.begin();
			i != m_clients.end(); ++i) {
			// Delete client
			delete i->second;
		}
	}
}

std::vector<u16> ClientInterface::getClientIDs(ClientState min_state)
{
	std::vector<u16> reply;
	MutexAutoLock clientslock(m_clients_mutex);

	for(UNORDERED_MAP<u16, RemoteClient*>::iterator i = m_clients.begin();
		i != m_clients.end(); ++i) {
		if (i->second->getState() >= min_state)
			reply.push_back(i->second->peer_id);
	}

	return reply;
}

void ClientInterface::step(float dtime)
{
	m_print_info_timer += dtime;
	if(m_print_info_timer >= 30.0)
	{
		m_print_info_timer = 0.0;
		UpdatePlayerList();
	}
}

void ClientInterface::UpdatePlayerList()
{
	if (m_env != NULL) {
		std::vector<u16> clients = getClientIDs();
		m_clients_names.clear();


		if(!clients.empty())
			infostream<<"Players:"<<std::endl;

		for (std::vector<u16>::iterator i = clients.begin(); i != clients.end(); ++i) {
			RemotePlayer *player = m_env->getPlayer(*i);

			if (player == NULL)
				continue;

			infostream << "* " << player->getName() << "\t";

			{
				MutexAutoLock clientslock(m_clients_mutex);
				RemoteClient* client = lockedGetClientNoEx(*i);
				if(client != NULL)
					client->PrintInfo(infostream);
			}

			m_clients_names.push_back(player->getName());
		}
	}
}

void ClientInterface::send(u16 peer_id, u8 channelnum,
		NetworkPacket* pkt, bool reliable)
{
	m_con->Send(peer_id, channelnum, pkt, reliable);
}

void ClientInterface::sendToAll(NetworkPacket *pkt)
{
	MutexAutoLock clientslock(m_clients_mutex);
	for (UNORDERED_MAP<u16, RemoteClient*>::iterator i = m_clients.begin();
			i != m_clients.end(); ++i) {
		RemoteClient *client = i->second;

		if (client->net_proto_version != 0) {
			m_con->Send(client->peer_id,
					clientCommandFactoryTable[pkt->getCommand()].channel, pkt,
					clientCommandFactoryTable[pkt->getCommand()].reliable);
		}
	}
}

RemoteClient* ClientInterface::getClientNoEx(u16 peer_id, ClientState state_min)
{
	MutexAutoLock clientslock(m_clients_mutex);
	UNORDERED_MAP<u16, RemoteClient*>::iterator n = m_clients.find(peer_id);
	// The client may not exist; clients are immediately removed if their
	// access is denied, and this event occurs later then.
	if (n == m_clients.end())
		return NULL;

	if (n->second->getState() >= state_min)
		return n->second;
	else
		return NULL;
}

RemoteClient* ClientInterface::lockedGetClientNoEx(u16 peer_id, ClientState state_min)
{
	UNORDERED_MAP<u16, RemoteClient*>::iterator n = m_clients.find(peer_id);
	// The client may not exist; clients are immediately removed if their
	// access is denied, and this event occurs later then.
	if (n == m_clients.end())
		return NULL;

	if (n->second->getState() >= state_min)
		return n->second;
	else
		return NULL;
}

ClientState ClientInterface::getClientState(u16 peer_id)
{
	MutexAutoLock clientslock(m_clients_mutex);
	UNORDERED_MAP<u16, RemoteClient*>::iterator n = m_clients.find(peer_id);
	// The client may not exist; clients are immediately removed if their
	// access is denied, and this event occurs later then.
	if (n == m_clients.end())
		return CS_Invalid;

	return n->second->getState();
}

void ClientInterface::setPlayerName(u16 peer_id,std::string name)
{
	MutexAutoLock clientslock(m_clients_mutex);
	UNORDERED_MAP<u16, RemoteClient*>::iterator n = m_clients.find(peer_id);
	// The client may not exist; clients are immediately removed if their
	// access is denied, and this event occurs later then.
	if (n != m_clients.end())
		n->second->setName(name);
}

void ClientInterface::DeleteClient(u16 peer_id)
{
	MutexAutoLock conlock(m_clients_mutex);

	// Error check
	UNORDERED_MAP<u16, RemoteClient*>::iterator n = m_clients.find(peer_id);
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
	for(std::set<u16>::iterator
			i = client->m_known_objects.begin();
			i != client->m_known_objects.end(); ++i)
	{
		// Get object
		u16 id = *i;
		ServerActiveObject* obj = m_env->getActiveObject(id);

		if(obj && obj->m_known_by_count > 0)
			obj->m_known_by_count--;
	}

	// Delete client
	delete m_clients[peer_id];
	m_clients.erase(peer_id);
}

void ClientInterface::CreateClient(u16 peer_id)
{
	MutexAutoLock conlock(m_clients_mutex);

	// Error check
	UNORDERED_MAP<u16, RemoteClient*>::iterator n = m_clients.find(peer_id);
	// The client shouldn't already exist
	if (n != m_clients.end()) return;

	// Create client
	RemoteClient *client = new RemoteClient();
	client->peer_id = peer_id;
	m_clients[client->peer_id] = client;
}

void ClientInterface::event(u16 peer_id, ClientStateEvent event)
{
	{
		MutexAutoLock clientlock(m_clients_mutex);

		// Error check
		UNORDERED_MAP<u16, RemoteClient*>::iterator n = m_clients.find(peer_id);

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

u16 ClientInterface::getProtocolVersion(u16 peer_id)
{
	MutexAutoLock conlock(m_clients_mutex);

	// Error check
	UNORDERED_MAP<u16, RemoteClient*>::iterator n = m_clients.find(peer_id);

	// No client to get version
	if (n == m_clients.end())
		return 0;

	return n->second->net_proto_version;
}

void ClientInterface::setClientVersion(u16 peer_id, u8 major, u8 minor, u8 patch, std::string full)
{
	MutexAutoLock conlock(m_clients_mutex);

	// Error check
	UNORDERED_MAP<u16, RemoteClient*>::iterator n = m_clients.find(peer_id);

	// No client to set versions
	if (n == m_clients.end())
		return;

	n->second->setVersionInfo(major,minor,patch,full);
}
