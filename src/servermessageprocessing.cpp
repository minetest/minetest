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


#include "script/scripting_game.h"
#include "util/serialize.h"
#include "util/pointedthing.h"
#include "util/string.h"
#include "clientserver.h"
#include "server.h"
#include "player.h"
#include "log.h"
#include "main.h"
#include "settings.h"
#include "content_sao.h"
#include "base64.h"
#include "version.h"
#include "nodedef.h"
#include "rollback.h"
#include "emerge.h"
#include "tool.h"
#include "profiler.h"
#include "ban.h"

void Server::ProcessData(u8 *data, u32 datasize, u16 peer_id)
{
	DSTACK(__FUNCTION_NAME);
	// Environment is locked first.
	JMutexAutoLock envlock(m_env_mutex);
	JMutexAutoLock conlock(m_con_mutex);

	ScopeProfiler sp(g_profiler, "Server::ProcessData");

	std::string addr_s;
	try{
		Address address = m_con.GetPeerAddress(peer_id);
		addr_s = address.serializeString();

		// drop player if is ip is banned
		if(m_banmanager->isIpBanned(addr_s)){
			std::string ban_name = m_banmanager->getBanName(addr_s);
			infostream<<"Server: A banned client tried to connect from "
					<<addr_s<<"; banned name was "
					<<ban_name<<std::endl;
			// This actually doesn't seem to transfer to the client
			DenyAccess(peer_id, L"Your ip is banned. Banned name was "
					+narrow_to_wide(ban_name));
			m_con.DeletePeer(peer_id);
			return;
		}
	}
	catch(con::PeerNotFoundException &e)
	{
		infostream<<"Server::ProcessData(): Cancelling: peer "
				<<peer_id<<" not found"<<std::endl;
		return;
	}

	u8 peer_ser_ver = getClient(peer_id)->serialization_version;

	try
	{

	if(datasize < 2)
		return;

	ToServerCommand command = (ToServerCommand)readU16(&data[0]);


	switch(command) {
		case TOSERVER_INIT:
			processInit(data,datasize,peer_id,addr_s);
			return;
		break;
		case TOSERVER_INIT2:
			processInit2(data,datasize,peer_id,addr_s);
			return;
		break;
		default:
		break;
	}


	if(peer_ser_ver == SER_FMT_VER_INVALID)
	{
		infostream<<"Server::ProcessData(): Cancelling: Peer"
				" serialization format invalid or not initialized."
				" Skipping incoming command="<<command<<std::endl;
		return;
	}

	Player *player = m_env->getPlayer(peer_id);
	if(player == NULL){
		infostream<<"Server::ProcessData(): Cancelling: "
				"No player for peer_id="<<peer_id
				<<std::endl;
		return;
	}

	PlayerSAO *playersao = player->getPlayerSAO();
	if(playersao == NULL){
		infostream<<"Server::ProcessData(): Cancelling: "
				"No player object for peer_id="<<peer_id
				<<std::endl;
		return;
	}

	switch(command) {
		case TOSERVER_INIT: /* Fallthrough */
		case TOSERVER_INIT2:
			assert("Invalid command evaluation order!" == 0);
		break;

		case TOSERVER_PLAYERPOS:
			processPlayerPos(data,datasize,peer_id,player,playersao);
			return;
			break;

		case TOSERVER_GOTBLOCKS:
			processGotBlocks(data,datasize,peer_id);
			return;
			break;

		case TOSERVER_DELETEDBLOCKS:
			processDeletedBlocks(data,datasize,peer_id);
			return;
			break;

		case TOSERVER_CLICK_OBJECT: /* Fallthrough */
		case TOSERVER_CLICK_ACTIVEOBJECT:
		case TOSERVER_GROUND_ACTION:
		case TOSERVER_RELEASE:
		case TOSERVER_SIGNTEXT:
		case TOSERVER_SIGNNODETEXT:
			infostream << "Server: ignored deprecated command: " << command << std::endl;
			return;
		break;

		case TOSERVER_INVENTORY_ACTION:
			processInventoryAction(data,datasize,peer_id,player,playersao);
			return;
			break;

		case TOSERVER_CHAT_MESSAGE:
			processChatMessage(data,datasize,peer_id,player);
			return;
			break;

		case TOSERVER_DAMAGE:
			processDamage(data,datasize,peer_id,player,playersao);
			return;
			break;

		case TOSERVER_BREATH:
			processBreath(data,datasize,peer_id,playersao);
			return;
			break;

		case TOSERVER_PASSWORD:
			processPassword(data,datasize,peer_id,player,playersao);
			return;
			break;

		case TOSERVER_PLAYERITEM:
			processPlayeritem(data,datasize,peer_id,playersao);
			return;
			break;

		case TOSERVER_RESPAWN:
			if(player->hp != 0 || !g_settings->getBool("enable_damage")) return;

			RespawnPlayer(peer_id);

			actionstream<<player->getName()<<" respawns at "
					<<PP(player->getPosition()/BS)<<std::endl;
			return;
			break;

		case TOSERVER_REQUEST_MEDIA:
			processRequestMedia(data,datasize,peer_id);
			return;
			break;

		case TOSERVER_RECEIVED_MEDIA:
			getClient(peer_id)->definitions_sent = true;
			return;
			break;

		case TOSERVER_INTERACT:
			processInteract(data,datasize,peer_id,player,playersao);
			return;
			break;

		case TOSERVER_REMOVED_SOUNDS:
			processRemovedSounds(data,datasize,peer_id);
			return;
			break;

		case TOSERVER_NODEMETA_FIELDS:
			processNodemetaFields(data,datasize,peer_id,player,playersao);
			return;
			break;

		case TOSERVER_INVENTORY_FIELDS:
			processInventoryFields(data,datasize,peer_id,player,playersao);
			return;
			break;

		default:
			infostream<<"Server::ProcessData(): Ignoring "
					"unknown command "<<command<<std::endl;
			break;
	}


	} //try
	catch(SendFailedException &e)
	{
		errorstream<<"Server::ProcessData(): SendFailedException: "
				<<"what="<<e.what()
				<<std::endl;
	}
}

void Server::processInit(u8* data, unsigned int datasize ,u16 peer_id,
									std::string& addr_s)
{
	DSTACK(__FUNCTION_NAME);
	// [0] u16 TOSERVER_INIT
	// [2] u8 SER_FMT_VER_HIGHEST_READ
	// [3] u8[20] player_name
	// [23] u8[28] password <--- can be sent without this, from old versions

	if(datasize < 2+1+PLAYERNAME_SIZE)
		return;

	// If net_proto_version is set, this client has already been handled
	if(getClient(peer_id)->net_proto_version != 0){
		verbosestream<<"Server: Ignoring multiple TOSERVER_INITs from "
				<<addr_s<<" (peer_id="<<peer_id<<")"<<std::endl;
		return;
	}

	verbosestream<<"Server: Got TOSERVER_INIT from "<<addr_s<<" (peer_id="
			<<peer_id<<")"<<std::endl;

	// Do not allow multiple players in simple singleplayer mode.
	// This isn't a perfect way to do it, but will suffice for now.
	if(m_simple_singleplayer_mode && m_clients.size() > 1){
		infostream<<"Server: Not allowing another client ("<<addr_s
				<<") to connect in simple singleplayer mode"<<std::endl;
		DenyAccess(peer_id, L"Running in simple singleplayer mode.");
		return;
	}

	// First byte after command is maximum supported
	// serialization version
	u8 client_max = data[2];
	u8 our_max = SER_FMT_VER_HIGHEST_READ;
	// Use the highest version supported by both
	u8 deployed = std::min(client_max, our_max);
	// If it's lower than the lowest supported, give up.
	if(deployed < SER_FMT_VER_LOWEST)
		deployed = SER_FMT_VER_INVALID;

	//peer->serialization_version = deployed;
	getClient(peer_id)->pending_serialization_version = deployed;

	if(deployed == SER_FMT_VER_INVALID)
	{
		actionstream<<"Server: A mismatched client tried to connect from "
				<<addr_s<<std::endl;
		infostream<<"Server: Cannot negotiate serialization version with "
				<<addr_s<<std::endl;
		DenyAccess(peer_id, std::wstring(
				L"Your client's version is not supported.\n"
				L"Server version is ")
				+ narrow_to_wide(minetest_version_simple) + L"."
		);
		return;
	}

	/*
		Read and check network protocol version
	*/

	u16 min_net_proto_version = 0;
	if(datasize >= 2+1+PLAYERNAME_SIZE+PASSWORD_SIZE+2)
		min_net_proto_version = readU16(&data[2+1+PLAYERNAME_SIZE+PASSWORD_SIZE]);

	// Use same version as minimum and maximum if maximum version field
	// doesn't exist (backwards compatibility)
	u16 max_net_proto_version = min_net_proto_version;
	if(datasize >= 2+1+PLAYERNAME_SIZE+PASSWORD_SIZE+2+2)
		max_net_proto_version = readU16(&data[2+1+PLAYERNAME_SIZE+PASSWORD_SIZE+2]);

	// Start with client's maximum version
	u16 net_proto_version = max_net_proto_version;

	// Figure out a working version if it is possible at all
	if(max_net_proto_version >= SERVER_PROTOCOL_VERSION_MIN ||
			min_net_proto_version <= SERVER_PROTOCOL_VERSION_MAX)
	{
		// If maximum is larger than our maximum, go with our maximum
		if(max_net_proto_version > SERVER_PROTOCOL_VERSION_MAX)
			net_proto_version = SERVER_PROTOCOL_VERSION_MAX;
		// Else go with client's maximum
		else
			net_proto_version = max_net_proto_version;
	}

	verbosestream<<"Server: "<<addr_s<<": Protocol version: min: "
			<<min_net_proto_version<<", max: "<<max_net_proto_version
			<<", chosen: "<<net_proto_version<<std::endl;

	getClient(peer_id)->net_proto_version = net_proto_version;

	if(net_proto_version < SERVER_PROTOCOL_VERSION_MIN ||
			net_proto_version > SERVER_PROTOCOL_VERSION_MAX)
	{
		actionstream<<"Server: A mismatched client tried to connect from "
				<<addr_s<<std::endl;
		DenyAccess(peer_id, std::wstring(
				L"Your client's version is not supported.\n"
				L"Server version is ")
				+ narrow_to_wide(minetest_version_simple) + L",\n"
				+ L"server's PROTOCOL_VERSION is "
				+ narrow_to_wide(itos(SERVER_PROTOCOL_VERSION_MIN))
				+ L"..."
				+ narrow_to_wide(itos(SERVER_PROTOCOL_VERSION_MAX))
				+ L", client's PROTOCOL_VERSION is "
				+ narrow_to_wide(itos(min_net_proto_version))
				+ L"..."
				+ narrow_to_wide(itos(max_net_proto_version))
		);
		return;
	}

	if(g_settings->getBool("strict_protocol_version_checking"))
	{
		if(net_proto_version != LATEST_PROTOCOL_VERSION)
		{
			actionstream<<"Server: A mismatched (strict) client tried to "
					<<"connect from "<<addr_s<<std::endl;
			DenyAccess(peer_id, std::wstring(
					L"Your client's version is not supported.\n"
					L"Server version is ")
					+ narrow_to_wide(minetest_version_simple) + L",\n"
					+ L"server's PROTOCOL_VERSION (strict) is "
					+ narrow_to_wide(itos(LATEST_PROTOCOL_VERSION))
					+ L", client's PROTOCOL_VERSION is "
					+ narrow_to_wide(itos(min_net_proto_version))
					+ L"..."
					+ narrow_to_wide(itos(max_net_proto_version))
			);
			return;
		}
	}

	/*
		Set up player
	*/

	// Get player name
	char playername[PLAYERNAME_SIZE];
	for(u32 i=0; i<PLAYERNAME_SIZE-1; i++)
	{
		playername[i] = data[3+i];
	}
	playername[PLAYERNAME_SIZE-1] = 0;

	if(playername[0]=='\0')
	{
		actionstream<<"Server: Player with an empty name "
				<<"tried to connect from "<<addr_s<<std::endl;
		DenyAccess(peer_id, L"Empty name");
		return;
	}

	if(string_allowed(playername, PLAYERNAME_ALLOWED_CHARS)==false)
	{
		actionstream<<"Server: Player with an invalid name "
				<<"tried to connect from "<<addr_s<<std::endl;
		DenyAccess(peer_id, L"Name contains unallowed characters");
		return;
	}

	if(!isSingleplayer() && strcasecmp(playername, "singleplayer") == 0)
	{
		actionstream<<"Server: Player with the name \"singleplayer\" "
				<<"tried to connect from "<<addr_s<<std::endl;
		DenyAccess(peer_id, L"Name is not allowed");
		return;
	}

	{
		std::string reason;
		if(m_script->on_prejoinplayer(playername, addr_s, reason))
		{
			actionstream<<"Server: Player with the name \""<<playername<<"\" "
					<<"tried to connect from "<<addr_s<<" "
					<<"but it was disallowed for the following reason: "
					<<reason<<std::endl;
			DenyAccess(peer_id, narrow_to_wide(reason.c_str()));
			return;
		}
	}

	infostream<<"Server: New connection: \""<<playername<<"\" from "
			<<addr_s<<" (peer_id="<<peer_id<<")"<<std::endl;

	// Get password
	char given_password[PASSWORD_SIZE];
	if(datasize < 2+1+PLAYERNAME_SIZE+PASSWORD_SIZE)
	{
		// old version - assume blank password
		given_password[0] = 0;
	}
	else
	{
		for(u32 i=0; i<PASSWORD_SIZE-1; i++)
		{
			given_password[i] = data[23+i];
		}
		given_password[PASSWORD_SIZE-1] = 0;
	}

	if(!base64_is_valid(given_password)){
		actionstream<<"Server: "<<playername
				<<" supplied invalid password hash"<<std::endl;
		DenyAccess(peer_id, L"Invalid password hash");
		return;
	}

	// Enforce user limit.
	// Don't enforce for users that have some admin right
	if(m_clients.size() >= g_settings->getU16("max_users") &&
			!checkPriv(playername, "server") &&
			!checkPriv(playername, "ban") &&
			!checkPriv(playername, "privs") &&
			!checkPriv(playername, "password") &&
			playername != g_settings->get("name"))
	{
		actionstream<<"Server: "<<playername<<" tried to join, but there"
				<<" are already max_users="
				<<g_settings->getU16("max_users")<<" players."<<std::endl;
		DenyAccess(peer_id, L"Too many users.");
		return;
	}

	std::string checkpwd; // Password hash to check against
	bool has_auth = m_script->getAuth(playername, &checkpwd, NULL);

	// If no authentication info exists for user, create it
	if(!has_auth){
		if(!isSingleplayer() &&
				g_settings->getBool("disallow_empty_password") &&
				std::string(given_password) == ""){
			actionstream<<"Server: "<<playername
					<<" supplied empty password"<<std::endl;
			DenyAccess(peer_id, L"Empty passwords are "
					L"disallowed. Set a password and try again.");
			return;
		}
		std::wstring raw_default_password =
			narrow_to_wide(g_settings->get("default_password"));
		std::string initial_password =
			translatePassword(playername, raw_default_password);

		// If default_password is empty, allow any initial password
		if (raw_default_password.length() == 0)
			initial_password = given_password;

		m_script->createAuth(playername, initial_password);
	}

	has_auth = m_script->getAuth(playername, &checkpwd, NULL);

	if(!has_auth){
		actionstream<<"Server: "<<playername<<" cannot be authenticated"
				<<" (auth handler does not work?)"<<std::endl;
		DenyAccess(peer_id, L"Not allowed to login");
		return;
	}

	if(given_password != checkpwd){
		actionstream<<"Server: "<<playername<<" supplied wrong password"
				<<std::endl;
		DenyAccess(peer_id, L"Wrong password");
		return;
	}

	// Get player
	PlayerSAO *playersao = emergePlayer(playername, peer_id);

	// If failed, cancel
	if(playersao == NULL)
	{
		RemotePlayer *player =
				static_cast<RemotePlayer*>(m_env->getPlayer(playername));
		if(player && player->peer_id != 0){
			errorstream<<"Server: "<<playername<<": Failed to emerge player"
					<<" (player allocated to an another client)"<<std::endl;
			DenyAccess(peer_id, L"Another client is connected with this "
					L"name. If your client closed unexpectedly, try again in "
					L"a minute.");
		} else {
			errorstream<<"Server: "<<playername<<": Failed to emerge player"
					<<std::endl;
			DenyAccess(peer_id, L"Could not allocate player.");
		}
		return;
	}

	/*
		Answer with a TOCLIENT_INIT
	*/
	{
		SharedBuffer<u8> reply(2+1+6+8+4);
		writeU16(&reply[0], TOCLIENT_INIT);
		writeU8(&reply[2], deployed);
		writeV3S16(&reply[2+1], floatToInt(playersao->getPlayer()->getPosition()+v3f(0,BS/2,0), BS));
		writeU64(&reply[2+1+6], m_env->getServerMap().getSeed());
		writeF1000(&reply[2+1+6+8], g_settings->getFloat("dedicated_server_step"));

		// Send as reliable
		m_con.Send(peer_id, 0, reply, true);
	}

	/*
		Send complete position information
	*/
	SendMovePlayer(peer_id);

	return;
}

void Server::processInit2(u8* data, unsigned int datasize, u16 peer_id,
								std::string& addr_s)
{
	DSTACK(__FUNCTION_NAME);
	verbosestream<<"Server: Got TOSERVER_INIT2 from "
			<<peer_id<<std::endl;

	Player *player = m_env->getPlayer(peer_id);
	if(!player){
		verbosestream<<"Server: TOSERVER_INIT2: "
				<<"Player not found; ignoring."<<std::endl;
		return;
	}

	RemoteClient *client = getClient(peer_id);
	client->serialization_version =
			getClient(peer_id)->pending_serialization_version;

	/*
		Send some initialization data
	*/

	infostream<<"Server: Sending content to "
			<<getPlayerName(peer_id)<<std::endl;

	// Send player movement settings
	SendMovement(m_con, peer_id);

	// Send item definitions
	SendItemDef(m_con, peer_id, m_itemdef, client->net_proto_version);

	// Send node definitions
	SendNodeDef(m_con, peer_id, m_nodedef, client->net_proto_version);

	// Send media announcement
	sendMediaAnnouncement(peer_id);

	// Send privileges
	SendPlayerPrivileges(peer_id);

	// Send inventory formspec
	SendPlayerInventoryFormspec(peer_id);

	// Send inventory
	UpdateCrafting(peer_id);
	SendInventory(peer_id);

	// Send HP
	if(g_settings->getBool("enable_damage"))
		SendPlayerHP(peer_id);

	// Send Breath
	SendPlayerBreath(peer_id);

	// Send detached inventories
	sendDetachedInventories(peer_id);

	// Show death screen if necessary
	if(player->hp == 0)
		SendDeathscreen(m_con, peer_id, false, v3f(0,0,0));

	// Send time of day
	{
		u16 time = m_env->getTimeOfDay();
		float time_speed = g_settings->getFloat("time_speed");
		SendTimeOfDay(peer_id, time, time_speed);
	}

	// Note things in chat if not in simple singleplayer mode
	if(!m_simple_singleplayer_mode)
	{
		// Send information about server to player in chat
		SendChatMessage(peer_id, getStatusString());

		// Send information about joining in chat
		{
			std::wstring name = L"unknown";
			Player *player = m_env->getPlayer(peer_id);
			if(player != NULL)
				name = narrow_to_wide(player->getName());

			std::wstring message;
			message += L"*** ";
			message += name;
			message += L" joined the game.";
			BroadcastChatMessage(message);
		}
	}

	// Warnings about protocol version can be issued here
	if(getClient(peer_id)->net_proto_version < LATEST_PROTOCOL_VERSION)
	{
		SendChatMessage(peer_id, L"# Server: WARNING: YOUR CLIENT'S "
				L"VERSION MAY NOT BE FULLY COMPATIBLE WITH THIS SERVER!");
	}

	/*
		Print out action
	*/
	{
		std::ostringstream os(std::ios_base::binary);
		for(std::map<u16, RemoteClient*>::iterator
			i = m_clients.begin();
			i != m_clients.end(); ++i)
		{
			RemoteClient *client = i->second;
			assert(client->peer_id == i->first);
			if(client->serialization_version == SER_FMT_VER_INVALID)
				continue;
			// Get player
			Player *player = m_env->getPlayer(client->peer_id);
			if(!player)
				continue;
			// Get name of player
			os<<player->getName()<<" ";
		}

		actionstream<<player->getName()<<" ["<<addr_s<<"] "<<"joins game. List of players: "
				<<os.str()<<std::endl;
	}

	return;
}

void Server::processPlayerPos(u8* data, unsigned int datasize, u16 peer_id,
								Player *player, PlayerSAO *playersao)
{
	DSTACK(__FUNCTION_NAME);
	if(datasize < 2+12+12+4+4)
		return;

	u32 start = 0;
	v3s32 ps = readV3S32(&data[start+2]);
	v3s32 ss = readV3S32(&data[start+2+12]);
	f32 pitch = (f32)readS32(&data[2+12+12]) / 100.0;
	f32 yaw = (f32)readS32(&data[2+12+12+4]) / 100.0;
	u32 keyPressed = 0;
	if(datasize >= 2+12+12+4+4+4)
		keyPressed = (u32)readU32(&data[2+12+12+4+4]);
	v3f position((f32)ps.X/100., (f32)ps.Y/100., (f32)ps.Z/100.);
	v3f speed((f32)ss.X/100., (f32)ss.Y/100., (f32)ss.Z/100.);
	pitch = wrapDegrees(pitch);
	yaw = wrapDegrees(yaw);

	player->setPosition(position);
	player->setSpeed(speed);
	player->setPitch(pitch);
	player->setYaw(yaw);
	player->keyPressed=keyPressed;
	player->control.up = (bool)(keyPressed&1);
	player->control.down = (bool)(keyPressed&2);
	player->control.left = (bool)(keyPressed&4);
	player->control.right = (bool)(keyPressed&8);
	player->control.jump = (bool)(keyPressed&16);
	player->control.aux1 = (bool)(keyPressed&32);
	player->control.sneak = (bool)(keyPressed&64);
	player->control.LMB = (bool)(keyPressed&128);
	player->control.RMB = (bool)(keyPressed&256);

	bool cheated = playersao->checkMovementCheat();
	if(cheated){
		// Call callbacks
		m_script->on_cheat(playersao, "moved_too_fast");
	}

	/*infostream<<"Server::ProcessData(): Moved player "<<peer_id<<" to "
														<<"("<<position.X<<","<<position.Y<<","<<position.Z<<")"
														<<" pitch="<<pitch<<" yaw="<<yaw<<std::endl;*/
}

void Server::processGotBlocks(u8* data, unsigned int datasize, u16 peer_id)
{
	DSTACK(__FUNCTION_NAME);
	if(datasize < 2+1)
		return;

	/*
		[0] u16 command
		[2] u8 count
		[3] v3s16 pos_0
		[3+6] v3s16 pos_1
		...
	*/

	u16 count = data[2];
	for(u16 i=0; i<count; i++)
	{
		if((s16)datasize < 2+1+(i+1)*6)
			throw con::InvalidIncomingDataException
				("GOTBLOCKS length is too short");
		v3s16 p = readV3S16(&data[2+1+i*6]);
		/*infostream<<"Server: GOTBLOCKS ("
				<<p.X<<","<<p.Y<<","<<p.Z<<")"<<std::endl;*/
		RemoteClient *client = getClient(peer_id);
		client->GotBlock(p);
	}
}

void Server::processDeletedBlocks(u8* data, unsigned int datasize, u16 peer_id)
{
	DSTACK(__FUNCTION_NAME);
	if(datasize < 2+1)
		return;

	/*
		[0] u16 command
		[2] u8 count
		[3] v3s16 pos_0
		[3+6] v3s16 pos_1
		...
	*/

	u16 count = data[2];
	for(u16 i=0; i<count; i++)
	{
		if((s16)datasize < 2+1+(i+1)*6)
			throw con::InvalidIncomingDataException
				("DELETEDBLOCKS length is too short");
		v3s16 p = readV3S16(&data[2+1+i*6]);
		/*infostream<<"Server: DELETEDBLOCKS ("
				<<p.X<<","<<p.Y<<","<<p.Z<<")"<<std::endl;*/
		RemoteClient *client = getClient(peer_id);
		client->SetBlockNotSent(p);
	}
}

void Server::processInventoryAction(u8* data, unsigned int datasize, u16 peer_id,
						Player *player, PlayerSAO *playersao)
{
	DSTACK(__FUNCTION_NAME);
	// Strip command and create a stream
	std::string datastring((char*)&data[2], datasize-2);
	verbosestream<<"TOSERVER_INVENTORY_ACTION: data="<<datastring<<std::endl;
	std::istringstream is(datastring, std::ios_base::binary);
	// Create an action
	InventoryAction *a = InventoryAction::deSerialize(is);
	if(a == NULL)
	{
		infostream<<"TOSERVER_INVENTORY_ACTION: "
				<<"InventoryAction::deSerialize() returned NULL"
				<<std::endl;
		return;
	}

	// If something goes wrong, this player is to blame
	RollbackScopeActor rollback_scope(m_rollback,
			std::string("player:")+player->getName());

	/*
		Note: Always set inventory not sent, to repair cases
		where the client made a bad prediction.
	*/

	/*
		Handle restrictions and special cases of the move action
	*/
	if(a->getType() == IACTION_MOVE)
	{
		IMoveAction *ma = (IMoveAction*)a;

		ma->from_inv.applyCurrentPlayer(player->getName());
		ma->to_inv.applyCurrentPlayer(player->getName());

		setInventoryModified(ma->from_inv);
		setInventoryModified(ma->to_inv);

		bool from_inv_is_current_player =
			(ma->from_inv.type == InventoryLocation::PLAYER) &&
			(ma->from_inv.name == player->getName());

		bool to_inv_is_current_player =
			(ma->to_inv.type == InventoryLocation::PLAYER) &&
			(ma->to_inv.name == player->getName());

		/*
			Disable moving items out of craftpreview
		*/
		if(ma->from_list == "craftpreview")
		{
			infostream<<"Ignoring IMoveAction from "
					<<(ma->from_inv.dump())<<":"<<ma->from_list
					<<" to "<<(ma->to_inv.dump())<<":"<<ma->to_list
					<<" because src is "<<ma->from_list<<std::endl;
			delete a;
			return;
		}

		/*
			Disable moving items into craftresult and craftpreview
		*/
		if(ma->to_list == "craftpreview" || ma->to_list == "craftresult")
		{
			infostream<<"Ignoring IMoveAction from "
					<<(ma->from_inv.dump())<<":"<<ma->from_list
					<<" to "<<(ma->to_inv.dump())<<":"<<ma->to_list
					<<" because dst is "<<ma->to_list<<std::endl;
			delete a;
			return;
		}

		// Disallow moving items in elsewhere than player's inventory
		// if not allowed to interact
		if(!checkPriv(player->getName(), "interact") &&
				(!from_inv_is_current_player ||
				!to_inv_is_current_player))
		{
			infostream<<"Cannot move outside of player's inventory: "
					<<"No interact privilege"<<std::endl;
			delete a;
			return;
		}
	}
	/*
		Handle restrictions and special cases of the drop action
	*/
	else if(a->getType() == IACTION_DROP)
	{
		IDropAction *da = (IDropAction*)a;

		da->from_inv.applyCurrentPlayer(player->getName());

		setInventoryModified(da->from_inv);

		/*
			Disable dropping items out of craftpreview
		*/
		if(da->from_list == "craftpreview")
		{
			infostream<<"Ignoring IDropAction from "
					<<(da->from_inv.dump())<<":"<<da->from_list
					<<" because src is "<<da->from_list<<std::endl;
			delete a;
			return;
		}

		// Disallow dropping items if not allowed to interact
		if(!checkPriv(player->getName(), "interact"))
		{
			delete a;
			return;
		}
	}
	/*
		Handle restrictions and special cases of the craft action
	*/
	else if(a->getType() == IACTION_CRAFT)
	{
		ICraftAction *ca = (ICraftAction*)a;

		ca->craft_inv.applyCurrentPlayer(player->getName());

		setInventoryModified(ca->craft_inv);

		//bool craft_inv_is_current_player =
		//	(ca->craft_inv.type == InventoryLocation::PLAYER) &&
		//	(ca->craft_inv.name == player->getName());

		// Disallow crafting if not allowed to interact
		if(!checkPriv(player->getName(), "interact"))
		{
			infostream<<"Cannot craft: "
					<<"No interact privilege"<<std::endl;
			delete a;
			return;
		}
	}

	// Do the action
	a->apply(this, playersao, this);
	// Eat the action
	delete a;
}

void Server::processChatMessage(u8* data, unsigned int datasize, u16 peer_id,
							Player *player)
{
	DSTACK(__FUNCTION_NAME);
	/*
		u16 command
		u16 length
		wstring message
	*/
	u8 buf[6];
	std::string datastring((char*)&data[2], datasize-2);
	std::istringstream is(datastring, std::ios_base::binary);

	// Read stuff
	is.read((char*)buf, 2);
	u16 len = readU16(buf);

	std::wstring message;
	for(u16 i=0; i<len; i++)
	{
		is.read((char*)buf, 2);
		message += (wchar_t)readU16(buf);
	}

	// If something goes wrong, this player is to blame
	RollbackScopeActor rollback_scope(m_rollback,
			std::string("player:")+player->getName());

	// Get player name of this client
	std::wstring name = narrow_to_wide(player->getName());

	// Run script hook
	bool ate = m_script->on_chat_message(player->getName(),
			wide_to_narrow(message));
	// If script ate the message, don't proceed
	if(ate)
		return;

	// Line to send to players
	std::wstring line;
	// Whether to send to the player that sent the line
	bool send_to_sender = false;
	// Whether to send to other players
	bool send_to_others = false;

	// Commands are implemented in Lua, so only catch invalid
	// commands that were not "eaten" and send an error back
	if(message[0] == L'/')
	{
		message = message.substr(1);
		send_to_sender = true;
		if(message.length() == 0)
			line += L"-!- Empty command";
		else
			line += L"-!- Invalid command: " + str_split(message, L' ')[0];
	}
	else
	{
		if(checkPriv(player->getName(), "shout")){
			line += L"<";
			line += name;
			line += L"> ";
			line += message;
			send_to_others = true;
		} else {
			line += L"-!- You don't have permission to shout.";
			send_to_sender = true;
		}
	}

	if(line != L"")
	{
		if(send_to_others)
			actionstream<<"CHAT: "<<wide_to_narrow(line)<<std::endl;

		/*
			Send the message to clients
		*/
		for(std::map<u16, RemoteClient*>::iterator
			i = m_clients.begin();
			i != m_clients.end(); ++i)
		{
			// Get client and check that it is valid
			RemoteClient *client = i->second;
			assert(client->peer_id == i->first);
			if(client->serialization_version == SER_FMT_VER_INVALID)
				continue;

			// Filter recipient
			bool sender_selected = (peer_id == client->peer_id);
			if(sender_selected == true && send_to_sender == false)
				continue;
			if(sender_selected == false && send_to_others == false)
				continue;

			SendChatMessage(client->peer_id, line);
		}
	}
}

void Server::processDamage(u8* data, unsigned int datasize, u16 peer_id,
						Player *player, PlayerSAO *playersao)
{
	DSTACK(__FUNCTION_NAME);
	std::string datastring((char*)&data[2], datasize-2);
	std::istringstream is(datastring, std::ios_base::binary);
	u8 damage = readU8(is);

	if(g_settings->getBool("enable_damage"))
	{
		actionstream<<player->getName()<<" damaged by "
				<<(int)damage<<" hp at "<<PP(player->getPosition()/BS)
				<<std::endl;

		playersao->setHP(playersao->getHP() - damage);

		if(playersao->getHP() == 0 && playersao->m_hp_not_sent)
			DiePlayer(peer_id);

		if(playersao->m_hp_not_sent)
			SendPlayerHP(peer_id);
	}
}

void Server::processBreath(u8* data, unsigned int datasize, u16 peer_id,
						PlayerSAO *playersao)
{
	DSTACK(__FUNCTION_NAME);
	std::string datastring((char*)&data[2], datasize-2);
	std::istringstream is(datastring, std::ios_base::binary);
	u16 breath = readU16(is);
	playersao->setBreath(breath);
}

void Server::processPassword(u8* data, unsigned int datasize, u16 peer_id,
						Player *player, PlayerSAO *playersao)
{
	DSTACK(__FUNCTION_NAME);
	/*
		[0] u16 TOSERVER_PASSWORD
		[2] u8[28] old password
		[30] u8[28] new password
	*/

	if(datasize != 2+PASSWORD_SIZE*2)
		return;
	/*char password[PASSWORD_SIZE];
	for(u32 i=0; i<PASSWORD_SIZE-1; i++)
		password[i] = data[2+i];
	password[PASSWORD_SIZE-1] = 0;*/
	std::string oldpwd;
	for(u32 i=0; i<PASSWORD_SIZE-1; i++)
	{
		char c = data[2+i];
		if(c == 0)
			break;
		oldpwd += c;
	}
	std::string newpwd;
	for(u32 i=0; i<PASSWORD_SIZE-1; i++)
	{
		char c = data[2+PASSWORD_SIZE+i];
		if(c == 0)
			break;
		newpwd += c;
	}

	if(!base64_is_valid(newpwd)){
		infostream<<"Server: "<<player->getName()<<" supplied invalid password hash"<<std::endl;
		// Wrong old password supplied!!
		SendChatMessage(peer_id, L"Invalid new password hash supplied. Password NOT changed.");
		return;
	}

	infostream<<"Server: Client requests a password change from "
			<<"'"<<oldpwd<<"' to '"<<newpwd<<"'"<<std::endl;

	std::string playername = player->getName();

	std::string checkpwd;
	m_script->getAuth(playername, &checkpwd, NULL);

	if(oldpwd != checkpwd)
	{
		infostream<<"Server: invalid old password"<<std::endl;
		// Wrong old password supplied!!
		SendChatMessage(peer_id, L"Invalid old password supplied. Password NOT changed.");
		return;
	}

	bool success = m_script->setPassword(playername, newpwd);
	if(success){
		actionstream<<player->getName()<<" changes password"<<std::endl;
		SendChatMessage(peer_id, L"Password change successful.");
	} else {
		actionstream<<player->getName()<<" tries to change password but "
				<<"it fails"<<std::endl;
		SendChatMessage(peer_id, L"Password change failed or inavailable.");
	}
}

void Server::processRequestMedia(u8* data, unsigned int datasize, u16 peer_id)
{
	DSTACK(__FUNCTION_NAME);
	std::string datastring((char*)&data[2], datasize-2);
	std::istringstream is(datastring, std::ios_base::binary);

	std::list<std::string> tosend;
	u16 numfiles = readU16(is);

	infostream<<"Sending "<<numfiles<<" files to "
			<<getPlayerName(peer_id)<<std::endl;
	verbosestream<<"TOSERVER_REQUEST_MEDIA: "<<std::endl;

	for(int i = 0; i < numfiles; i++) {
		std::string name = deSerializeString(is);
		tosend.push_back(name);
		verbosestream<<"TOSERVER_REQUEST_MEDIA: requested file "
				<<name<<std::endl;
	}

	sendRequestedMedia(peer_id, tosend);

	// Now the client should know about everything
	// (definitions and files)
	getClient(peer_id)->definitions_sent = true;
}

void Server::processInteract(u8* data, unsigned int datasize, u16 peer_id,
						Player *player, PlayerSAO *playersao)
{
	DSTACK(__FUNCTION_NAME);
	std::string datastring((char*)&data[2], datasize-2);
	std::istringstream is(datastring, std::ios_base::binary);

	/*
		[0] u16 command
		[2] u8 action
		[3] u16 item
		[5] u32 length of the next item
		[9] serialized PointedThing
		actions:
		0: start digging (from undersurface) or use
		1: stop digging (all parameters ignored)
		2: digging completed
		3: place block or item (to abovesurface)
		4: use item
	*/
	u8 action = readU8(is);
	u16 item_i = readU16(is);
	std::istringstream tmp_is(deSerializeLongString(is), std::ios::binary);
	PointedThing pointed;
	pointed.deSerialize(tmp_is);

	verbosestream<<"TOSERVER_INTERACT: action="<<(int)action<<", item="
			<<item_i<<", pointed="<<pointed.dump()<<std::endl;

	if(player->hp == 0)
	{
		verbosestream<<"TOSERVER_INTERACT: "<<player->getName()
			<<" tried to interact, but is dead!"<<std::endl;
		return;
	}

	v3f player_pos = playersao->getLastGoodPosition();

	// Update wielded item
	playersao->setWieldIndex(item_i);

	// Get pointed to node (undefined if not POINTEDTYPE_NODE)
	v3s16 p_under = pointed.node_undersurface;
	v3s16 p_above = pointed.node_abovesurface;

	// Get pointed to object (NULL if not POINTEDTYPE_OBJECT)
	ServerActiveObject *pointed_object = NULL;
	if(pointed.type == POINTEDTHING_OBJECT)
	{
		pointed_object = m_env->getActiveObject(pointed.object_id);
		if(pointed_object == NULL)
		{
			verbosestream<<"TOSERVER_INTERACT: "
				"pointed object is NULL"<<std::endl;
			return;
		}

	}

	v3f pointed_pos_under = player_pos;
	v3f pointed_pos_above = player_pos;
	if(pointed.type == POINTEDTHING_NODE)
	{
		pointed_pos_under = intToFloat(p_under, BS);
		pointed_pos_above = intToFloat(p_above, BS);
	}
	else if(pointed.type == POINTEDTHING_OBJECT)
	{
		pointed_pos_under = pointed_object->getBasePosition();
		pointed_pos_above = pointed_pos_under;
	}

	/*
		Check that target is reasonably close
		(only when digging or placing things)
	*/
	if(action == 0 || action == 2 || action == 3)
	{
		float d = player_pos.getDistanceFrom(pointed_pos_under);
		float max_d = BS * 14; // Just some large enough value
		if(d > max_d){
			actionstream<<"Player "<<player->getName()
					<<" tried to access "<<pointed.dump()
					<<" from too far: "
					<<"d="<<d<<", max_d="<<max_d
					<<". ignoring."<<std::endl;
			// Re-send block to revert change on client-side
			RemoteClient *client = getClient(peer_id);
			v3s16 blockpos = getNodeBlockPos(floatToInt(pointed_pos_under, BS));
			client->SetBlockNotSent(blockpos);
			// Call callbacks
			m_script->on_cheat(playersao, "interacted_too_far");
			// Do nothing else
			return;
		}
	}

	/*
		Make sure the player is allowed to do it
	*/
	if(!checkPriv(player->getName(), "interact"))
	{
		actionstream<<player->getName()<<" attempted to interact with "
				<<pointed.dump()<<" without 'interact' privilege"
				<<std::endl;
		// Re-send block to revert change on client-side
		RemoteClient *client = getClient(peer_id);
		// Digging completed -> under
		if(action == 2){
			v3s16 blockpos = getNodeBlockPos(floatToInt(pointed_pos_under, BS));
			client->SetBlockNotSent(blockpos);
		}
		// Placement -> above
		if(action == 3){
			v3s16 blockpos = getNodeBlockPos(floatToInt(pointed_pos_above, BS));
			client->SetBlockNotSent(blockpos);
		}
		return;
	}

	/*
		If something goes wrong, this player is to blame
	*/
	RollbackScopeActor rollback_scope(m_rollback,
			std::string("player:")+player->getName());

	/*
		0: start digging or punch object
	*/
	if(action == 0)
	{
		if(pointed.type == POINTEDTHING_NODE)
		{
			/*
				NOTE: This can be used in the future to check if
				somebody is cheating, by checking the timing.
			*/
			MapNode n(CONTENT_IGNORE);
			try
			{
				n = m_env->getMap().getNode(p_under);
			}
			catch(InvalidPositionException &e)
			{
				infostream<<"Server: Not punching: Node not found."
						<<" Adding block to emerge queue."
						<<std::endl;
				m_emerge->enqueueBlockEmerge(peer_id, getNodeBlockPos(p_above), false);
			}
			if(n.getContent() != CONTENT_IGNORE)
				m_script->node_on_punch(p_under, n, playersao);
			// Cheat prevention
			playersao->noCheatDigStart(p_under);
		}
		else if(pointed.type == POINTEDTHING_OBJECT)
		{
			// Skip if object has been removed
			if(pointed_object->m_removed)
				return;

			actionstream<<player->getName()<<" punches object "
					<<pointed.object_id<<": "
					<<pointed_object->getDescription()<<std::endl;

			ItemStack punchitem = playersao->getWieldedItem();
			ToolCapabilities toolcap =
					punchitem.getToolCapabilities(m_itemdef);
			v3f dir = (pointed_object->getBasePosition() -
					(player->getPosition() + player->getEyeOffset())
						).normalize();
			float time_from_last_punch =
				playersao->resetTimeFromLastPunch();
			pointed_object->punch(dir, &toolcap, playersao,
					time_from_last_punch);
		}

	} // action == 0

	/*
		1: stop digging
	*/
	else if(action == 1)
	{
	} // action == 1

	/*
		2: Digging completed
	*/
	else if(action == 2)
	{
		// Only digging of nodes
		if(pointed.type == POINTEDTHING_NODE)
		{
			MapNode n(CONTENT_IGNORE);
			try
			{
				n = m_env->getMap().getNode(p_under);
			}
			catch(InvalidPositionException &e)
			{
				infostream<<"Server: Not finishing digging: Node not found."
						<<" Adding block to emerge queue."
						<<std::endl;
				m_emerge->enqueueBlockEmerge(peer_id, getNodeBlockPos(p_above), false);
			}

			/* Cheat prevention */
			bool is_valid_dig = true;
			if(!isSingleplayer() && !g_settings->getBool("disable_anticheat"))
			{
				v3s16 nocheat_p = playersao->getNoCheatDigPos();
				float nocheat_t = playersao->getNoCheatDigTime();
				playersao->noCheatDigEnd();
				// If player didn't start digging this, ignore dig
				if(nocheat_p != p_under){
					infostream<<"Server: NoCheat: "<<player->getName()
							<<" started digging "
							<<PP(nocheat_p)<<" and completed digging "
							<<PP(p_under)<<"; not digging."<<std::endl;
					is_valid_dig = false;
					// Call callbacks
					m_script->on_cheat(playersao, "finished_unknown_dig");
				}
				// Get player's wielded item
				ItemStack playeritem;
				InventoryList *mlist = playersao->getInventory()->getList("main");
				if(mlist != NULL)
					playeritem = mlist->getItem(playersao->getWieldIndex());
				ToolCapabilities playeritem_toolcap =
						playeritem.getToolCapabilities(m_itemdef);
				// Get diggability and expected digging time
				DigParams params = getDigParams(m_nodedef->get(n).groups,
						&playeritem_toolcap);
				// If can't dig, try hand
				if(!params.diggable){
					const ItemDefinition &hand = m_itemdef->get("");
					const ToolCapabilities *tp = hand.tool_capabilities;
					if(tp)
						params = getDigParams(m_nodedef->get(n).groups, tp);
				}
				// If can't dig, ignore dig
				if(!params.diggable){
					infostream<<"Server: NoCheat: "<<player->getName()
							<<" completed digging "<<PP(p_under)
							<<", which is not diggable with tool. not digging."
							<<std::endl;
					is_valid_dig = false;
					// Call callbacks
					m_script->on_cheat(playersao, "dug_unbreakable");
				}
				// Check digging time
				// If already invalidated, we don't have to
				if(!is_valid_dig){
					// Well not our problem then
				}
				// Clean and long dig
				else if(params.time > 2.0 && nocheat_t * 1.2 > params.time){
					// All is good, but grab time from pool; don't care if
					// it's actually available
					playersao->getDigPool().grab(params.time);
				}
				// Short or laggy dig
				// Try getting the time from pool
				else if(playersao->getDigPool().grab(params.time)){
					// All is good
				}
				// Dig not possible
				else{
					infostream<<"Server: NoCheat: "<<player->getName()
							<<" completed digging "<<PP(p_under)
							<<"too fast; not digging."<<std::endl;
					is_valid_dig = false;
					// Call callbacks
					m_script->on_cheat(playersao, "dug_too_fast");
				}
			}

			/* Actually dig node */

			if(is_valid_dig && n.getContent() != CONTENT_IGNORE)
				m_script->node_on_dig(p_under, n, playersao);

			// Send unusual result (that is, node not being removed)
			if(m_env->getMap().getNodeNoEx(p_under).getContent() != CONTENT_AIR)
			{
				// Re-send block to revert change on client-side
				RemoteClient *client = getClient(peer_id);
				v3s16 blockpos = getNodeBlockPos(floatToInt(pointed_pos_under, BS));
				client->SetBlockNotSent(blockpos);
			}
		}
	} // action == 2

	/*
		3: place block or right-click object
	*/
	else if(action == 3)
	{
		ItemStack item = playersao->getWieldedItem();

		// Reset build time counter
		if(pointed.type == POINTEDTHING_NODE &&
				item.getDefinition(m_itemdef).type == ITEM_NODE)
			getClient(peer_id)->m_time_from_building = 0.0;

		if(pointed.type == POINTEDTHING_OBJECT)
		{
			// Right click object

			// Skip if object has been removed
			if(pointed_object->m_removed)
				return;

			actionstream<<player->getName()<<" right-clicks object "
					<<pointed.object_id<<": "
					<<pointed_object->getDescription()<<std::endl;

			// Do stuff
			pointed_object->rightClick(playersao);
		}
		else if(m_script->item_OnPlace(
				item, playersao, pointed))
		{
			// Placement was handled in lua

			// Apply returned ItemStack
			playersao->setWieldedItem(item);
		}

		// If item has node placement prediction, always send the
		// blocks to make sure the client knows what exactly happened
		if(item.getDefinition(m_itemdef).node_placement_prediction != ""){
			RemoteClient *client = getClient(peer_id);
			v3s16 blockpos = getNodeBlockPos(floatToInt(pointed_pos_above, BS));
			client->SetBlockNotSent(blockpos);
			v3s16 blockpos2 = getNodeBlockPos(floatToInt(pointed_pos_under, BS));
			if(blockpos2 != blockpos){
				client->SetBlockNotSent(blockpos2);
			}
		}
	} // action == 3

	/*
		4: use
	*/
	else if(action == 4)
	{
		ItemStack item = playersao->getWieldedItem();

		actionstream<<player->getName()<<" uses "<<item.name
				<<", pointing at "<<pointed.dump()<<std::endl;

		if(m_script->item_OnUse(
				item, playersao, pointed))
		{
			// Apply returned ItemStack
			playersao->setWieldedItem(item);
		}

	} // action == 4


	/*
		Catch invalid actions
	*/
	else
	{
		infostream<<"WARNING: Server: Invalid action "
				<<action<<std::endl;
	}
}

void Server::processPlayeritem(u8* data, unsigned int datasize, u16 peer_id,
						PlayerSAO *playersao)
{
	DSTACK(__FUNCTION_NAME);
	if (datasize < 2+2)
		return;

	u16 item = readU16(&data[2]);
	playersao->setWieldIndex(item);
}

void Server::processRemovedSounds(u8* data, unsigned int datasize, u16 peer_id)
{
	DSTACK(__FUNCTION_NAME);
	std::string datastring((char*)&data[2], datasize-2);
	std::istringstream is(datastring, std::ios_base::binary);

	int num = readU16(is);
	for(int k=0; k<num; k++){
		s32 id = readS32(is);
		std::map<s32, ServerPlayingSound>::iterator i =
				m_playing_sounds.find(id);
		if(i == m_playing_sounds.end())
			continue;
		ServerPlayingSound &psound = i->second;
		psound.clients.erase(peer_id);
		if(psound.clients.size() == 0)
			m_playing_sounds.erase(i++);
	}
}

void Server::processNodemetaFields(u8* data, unsigned int datasize, u16 peer_id,
						Player* player, PlayerSAO *playersao)
{
	DSTACK(__FUNCTION_NAME);
	std::string datastring((char*)&data[2], datasize-2);
	std::istringstream is(datastring, std::ios_base::binary);

	v3s16 p = readV3S16(is);
	std::string formname = deSerializeString(is);
	int num = readU16(is);
	std::map<std::string, std::string> fields;
	for(int k=0; k<num; k++){
		std::string fieldname = deSerializeString(is);
		std::string fieldvalue = deSerializeLongString(is);
		fields[fieldname] = fieldvalue;
	}

	// If something goes wrong, this player is to blame
	RollbackScopeActor rollback_scope(m_rollback,
			std::string("player:")+player->getName());

	// Check the target node for rollback data; leave others unnoticed
	RollbackNode rn_old(&m_env->getMap(), p, this);

	m_script->node_on_receive_fields(p, formname, fields,playersao);

	// Report rollback data
	RollbackNode rn_new(&m_env->getMap(), p, this);
	if(rollback() && rn_new != rn_old){
		RollbackAction action;
		action.setSetNode(p, rn_old, rn_new);
		rollback()->reportAction(action);
	}
}

void Server::processInventoryFields(u8* data, unsigned int datasize, u16 peer_id,
						Player* player, PlayerSAO *playersao)
{
	DSTACK(__FUNCTION_NAME);
	std::string datastring((char*)&data[2], datasize-2);
	std::istringstream is(datastring, std::ios_base::binary);

	std::string formname = deSerializeString(is);
	int num = readU16(is);
	std::map<std::string, std::string> fields;
	for(int k=0; k<num; k++){
		std::string fieldname = deSerializeString(is);
		std::string fieldvalue = deSerializeLongString(is);
		fields[fieldname] = fieldvalue;
	}

	m_script->on_playerReceiveFields(playersao, formname, fields);
}
