/*
Minetest
Copyright (C) 2015 nerzhul, Loic Blot <loic.blot@unix-experience.fr>

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

#include "chatmessage.h"
#include "server.h"
#include "log.h"
#include "content_sao.h"
#include "emerge.h"
#include "mapblock.h"
#include "modchannels.h"
#include "nodedef.h"
#include "remoteplayer.h"
#include "rollback_interface.h"
#include "scripting_server.h"
#include "settings.h"
#include "tool.h"
#include "version.h"
#include "network/connection.h"
#include "network/networkprotocol.h"
#include "network/serveropcodes.h"
#include "util/auth.h"
#include "util/base64.h"
#include "util/pointedthing.h"
#include "util/serialize.h"
#include "util/srp.h"

void Server::handleCommand_Deprecated(NetworkPacket* pkt)
{
	infostream << "Server: " << toServerCommandTable[pkt->getCommand()].name
		<< " not supported anymore" << std::endl;
}

void Server::handleCommand_Init(NetworkPacket* pkt)
{

	if(pkt->getSize() < 1)
		return;

	RemoteClient* client = getClient(pkt->getPeerId(), CS_Created);

	std::string addr_s;
	try {
		Address address = getPeerAddress(pkt->getPeerId());
		addr_s = address.serializeString();
	}
	catch (con::PeerNotFoundException &e) {
		/*
		 * no peer for this packet found
		 * most common reason is peer timeout, e.g. peer didn't
		 * respond for some time, your server was overloaded or
		 * things like that.
		 */
		infostream << "Server::ProcessData(): Canceling: peer "
				<< pkt->getPeerId() << " not found" << std::endl;
		return;
	}

	// If net_proto_version is set, this client has already been handled
	if (client->getState() > CS_Created) {
		verbosestream << "Server: Ignoring multiple TOSERVER_INITs from "
				<< addr_s << " (peer_id=" << pkt->getPeerId() << ")" << std::endl;
		return;
	}

	verbosestream << "Server: Got TOSERVER_INIT from " << addr_s << " (peer_id="
			<< pkt->getPeerId() << ")" << std::endl;

	// Do not allow multiple players in simple singleplayer mode.
	// This isn't a perfect way to do it, but will suffice for now
	if (m_simple_singleplayer_mode && m_clients.getClientIDs().size() > 1) {
		infostream << "Server: Not allowing another client (" << addr_s
				<< ") to connect in simple singleplayer mode" << std::endl;
		DenyAccess(pkt->getPeerId(), SERVER_ACCESSDENIED_SINGLEPLAYER);
		return;
	}

	// First byte after command is maximum supported
	// serialization version
	u8 client_max;
	u16 supp_compr_modes;
	u16 min_net_proto_version = 0;
	u16 max_net_proto_version;
	std::string playerName;

	*pkt >> client_max >> supp_compr_modes >> min_net_proto_version
			>> max_net_proto_version >> playerName;

	u8 our_max = SER_FMT_VER_HIGHEST_READ;
	// Use the highest version supported by both
	u8 depl_serial_v = std::min(client_max, our_max);
	// If it's lower than the lowest supported, give up.
	if (depl_serial_v < SER_FMT_VER_LOWEST_READ)
		depl_serial_v = SER_FMT_VER_INVALID;

	if (depl_serial_v == SER_FMT_VER_INVALID) {
		actionstream << "Server: A mismatched client tried to connect from "
				<< addr_s << std::endl;
		infostream<<"Server: Cannot negotiate serialization version with "
				<< addr_s << std::endl;
		DenyAccess(pkt->getPeerId(), SERVER_ACCESSDENIED_WRONG_VERSION);
		return;
	}

	client->setPendingSerializationVersion(depl_serial_v);

	/*
		Read and check network protocol version
	*/

	u16 net_proto_version = 0;

	// Figure out a working version if it is possible at all
	if (max_net_proto_version >= SERVER_PROTOCOL_VERSION_MIN ||
			min_net_proto_version <= SERVER_PROTOCOL_VERSION_MAX) {
		// If maximum is larger than our maximum, go with our maximum
		if (max_net_proto_version > SERVER_PROTOCOL_VERSION_MAX)
			net_proto_version = SERVER_PROTOCOL_VERSION_MAX;
		// Else go with client's maximum
		else
			net_proto_version = max_net_proto_version;
	}

	verbosestream << "Server: " << addr_s << ": Protocol version: min: "
			<< min_net_proto_version << ", max: " << max_net_proto_version
			<< ", chosen: " << net_proto_version << std::endl;

	client->net_proto_version = net_proto_version;

	if ((g_settings->getBool("strict_protocol_version_checking") &&
			net_proto_version != LATEST_PROTOCOL_VERSION) ||
			net_proto_version < SERVER_PROTOCOL_VERSION_MIN ||
			net_proto_version > SERVER_PROTOCOL_VERSION_MAX) {
		actionstream << "Server: A mismatched client tried to connect from "
				<< addr_s << std::endl;
		DenyAccess(pkt->getPeerId(), SERVER_ACCESSDENIED_WRONG_VERSION);
		return;
	}

	/*
		Validate player name
	*/
	const char* playername = playerName.c_str();

	size_t pns = playerName.size();
	if (pns == 0 || pns > PLAYERNAME_SIZE) {
		actionstream << "Server: Player with "
			<< ((pns > PLAYERNAME_SIZE) ? "a too long" : "an empty")
			<< " name tried to connect from " << addr_s << std::endl;
		DenyAccess(pkt->getPeerId(), SERVER_ACCESSDENIED_WRONG_NAME);
		return;
	}

	if (!string_allowed(playerName, PLAYERNAME_ALLOWED_CHARS)) {
		actionstream << "Server: Player with an invalid name "
				<< "tried to connect from " << addr_s << std::endl;
		DenyAccess(pkt->getPeerId(), SERVER_ACCESSDENIED_WRONG_CHARS_IN_NAME);
		return;
	}

	m_clients.setPlayerName(pkt->getPeerId(), playername);
	//TODO (later) case insensitivity

	std::string legacyPlayerNameCasing = playerName;

	if (!isSingleplayer() && strcasecmp(playername, "singleplayer") == 0) {
		actionstream << "Server: Player with the name \"singleplayer\" "
				<< "tried to connect from " << addr_s << std::endl;
		DenyAccess(pkt->getPeerId(), SERVER_ACCESSDENIED_WRONG_NAME);
		return;
	}

	{
		std::string reason;
		if (m_script->on_prejoinplayer(playername, addr_s, &reason)) {
			actionstream << "Server: Player with the name \"" << playerName << "\" "
					<< "tried to connect from " << addr_s << " "
					<< "but it was disallowed for the following reason: "
					<< reason << std::endl;
			DenyAccess(pkt->getPeerId(), SERVER_ACCESSDENIED_CUSTOM_STRING, reason);
			return;
		}
	}

	infostream << "Server: New connection: \"" << playerName << "\" from "
			<< addr_s << " (peer_id=" << pkt->getPeerId() << ")" << std::endl;

	// Enforce user limit.
	// Don't enforce for users that have some admin right or mod permits it.
	if (m_clients.isUserLimitReached() &&
			playername != g_settings->get("name") &&
			!m_script->can_bypass_userlimit(playername, addr_s)) {
		actionstream << "Server: " << playername << " tried to join from "
				<< addr_s << ", but there" << " are already max_users="
				<< g_settings->getU16("max_users") << " players." << std::endl;
		DenyAccess(pkt->getPeerId(), SERVER_ACCESSDENIED_TOO_MANY_USERS);
		return;
	}

	/*
		Compose auth methods for answer
	*/
	std::string encpwd; // encrypted Password field for the user
	bool has_auth = m_script->getAuth(playername, &encpwd, NULL);
	u32 auth_mechs = 0;

	client->chosen_mech = AUTH_MECHANISM_NONE;

	if (has_auth) {
		std::vector<std::string> pwd_components = str_split(encpwd, '#');
		if (pwd_components.size() == 4) {
			if (pwd_components[1] == "1") { // 1 means srp
				auth_mechs |= AUTH_MECHANISM_SRP;
				client->enc_pwd = encpwd;
			} else {
				actionstream << "User " << playername
					<< " tried to log in, but password field"
					<< " was invalid (unknown mechcode)." << std::endl;
				DenyAccess(pkt->getPeerId(), SERVER_ACCESSDENIED_SERVER_FAIL);
				return;
			}
		} else if (base64_is_valid(encpwd)) {
			auth_mechs |= AUTH_MECHANISM_LEGACY_PASSWORD;
			client->enc_pwd = encpwd;
		} else {
			actionstream << "User " << playername
				<< " tried to log in, but password field"
				<< " was invalid (invalid base64)." << std::endl;
			DenyAccess(pkt->getPeerId(), SERVER_ACCESSDENIED_SERVER_FAIL);
			return;
		}
	} else {
		std::string default_password = g_settings->get("default_password");
		if (default_password.length() == 0) {
			auth_mechs |= AUTH_MECHANISM_FIRST_SRP;
		} else {
			// Take care of default passwords.
			client->enc_pwd = get_encoded_srp_verifier(playerName, default_password);
			auth_mechs |= AUTH_MECHANISM_SRP;
			// Allocate player in db, but only on successful login.
			client->create_player_on_auth_success = true;
		}
	}

	/*
		Answer with a TOCLIENT_HELLO
	*/

	verbosestream << "Sending TOCLIENT_HELLO with auth method field: "
		<< auth_mechs << std::endl;

	NetworkPacket resp_pkt(TOCLIENT_HELLO, 1 + 4
		+ legacyPlayerNameCasing.size(), pkt->getPeerId());

	u16 depl_compress_mode = NETPROTO_COMPRESSION_NONE;
	resp_pkt << depl_serial_v << depl_compress_mode << net_proto_version
		<< auth_mechs << legacyPlayerNameCasing;

	Send(&resp_pkt);

	client->allowed_auth_mechs = auth_mechs;
	client->setDeployedCompressionMode(depl_compress_mode);

	m_clients.event(pkt->getPeerId(), CSE_Hello);
}

void Server::handleCommand_Init2(NetworkPacket* pkt)
{
	verbosestream << "Server: Got TOSERVER_INIT2 from "
			<< pkt->getPeerId() << std::endl;

	m_clients.event(pkt->getPeerId(), CSE_GotInit2);
	u16 protocol_version = m_clients.getProtocolVersion(pkt->getPeerId());

	std::string lang;
	if (pkt->getSize() > 0)
		*pkt >> lang;

	/*
		Send some initialization data
	*/

	infostream << "Server: Sending content to "
			<< getPlayerName(pkt->getPeerId()) << std::endl;

	// Send player movement settings
	SendMovement(pkt->getPeerId());

	// Send item definitions
	SendItemDef(pkt->getPeerId(), m_itemdef, protocol_version);

	// Send node definitions
	SendNodeDef(pkt->getPeerId(), m_nodedef, protocol_version);

	m_clients.event(pkt->getPeerId(), CSE_SetDefinitionsSent);

	// Send media announcement
	sendMediaAnnouncement(pkt->getPeerId(), lang);

	// Send detached inventories
	sendDetachedInventories(pkt->getPeerId());

	// Send time of day
	u16 time = m_env->getTimeOfDay();
	float time_speed = g_settings->getFloat("time_speed");
	SendTimeOfDay(pkt->getPeerId(), time, time_speed);

	SendCSMRestrictionFlags(pkt->getPeerId());

	// Warnings about protocol version can be issued here
	if (getClient(pkt->getPeerId())->net_proto_version < LATEST_PROTOCOL_VERSION) {
		SendChatMessage(pkt->getPeerId(), ChatMessage(CHATMESSAGE_TYPE_SYSTEM,
				L"# Server: WARNING: YOUR CLIENT'S VERSION MAY NOT BE FULLY COMPATIBLE "
				L"WITH THIS SERVER!"));

	}
}

void Server::handleCommand_RequestMedia(NetworkPacket* pkt)
{
	std::vector<std::string> tosend;
	u16 numfiles;

	*pkt >> numfiles;

	infostream << "Sending " << numfiles << " files to "
			<< getPlayerName(pkt->getPeerId()) << std::endl;
	verbosestream << "TOSERVER_REQUEST_MEDIA: " << std::endl;

	for (u16 i = 0; i < numfiles; i++) {
		std::string name;

		*pkt >> name;

		tosend.push_back(name);
		verbosestream << "TOSERVER_REQUEST_MEDIA: requested file "
				<< name << std::endl;
	}

	sendRequestedMedia(pkt->getPeerId(), tosend);
}

void Server::handleCommand_ClientReady(NetworkPacket* pkt)
{
	session_t peer_id = pkt->getPeerId();

	PlayerSAO* playersao = StageTwoClientInit(peer_id);

	if (playersao == NULL) {
		actionstream
			<< "TOSERVER_CLIENT_READY stage 2 client init failed for peer_id: "
			<< peer_id << std::endl;
		DisconnectPeer(peer_id);
		return;
	}


	if (pkt->getSize() < 8) {
		errorstream
			<< "TOSERVER_CLIENT_READY client sent inconsistent data, disconnecting peer_id: "
			<< peer_id << std::endl;
		DisconnectPeer(peer_id);
		return;
	}

	u8 major_ver, minor_ver, patch_ver, reserved;
	std::string full_ver;
	*pkt >> major_ver >> minor_ver >> patch_ver >> reserved >> full_ver;

	m_clients.setClientVersion(
			peer_id, major_ver, minor_ver, patch_ver,
			full_ver);

	const std::vector<std::string> &players = m_clients.getPlayerNames();
	NetworkPacket list_pkt(TOCLIENT_UPDATE_PLAYER_LIST, 0, peer_id);
	list_pkt << (u8) PLAYER_LIST_INIT << (u16) players.size();
	for (const std::string &player: players) {
		list_pkt <<  player;
	}
	m_clients.send(peer_id, 0, &list_pkt, true);

	NetworkPacket notice_pkt(TOCLIENT_UPDATE_PLAYER_LIST, 0, PEER_ID_INEXISTENT);
	// (u16) 1 + std::string represents a pseudo vector serialization representation
	notice_pkt << (u8) PLAYER_LIST_ADD << (u16) 1 << std::string(playersao->getPlayer()->getName());
	m_clients.sendToAll(&notice_pkt);

	m_clients.event(peer_id, CSE_SetClientReady);
	m_script->on_joinplayer(playersao);
	// Send shutdown timer if shutdown has been scheduled
	if (m_shutdown_state.isTimerRunning()) {
		SendChatMessage(pkt->getPeerId(), m_shutdown_state.getShutdownTimerMessage());
	}
}

void Server::handleCommand_GotBlocks(NetworkPacket* pkt)
{
	if (pkt->getSize() < 1)
		return;

	/*
		[0] u16 command
		[2] u8 count
		[3] v3s16 pos_0
		[3+6] v3s16 pos_1
		...
	*/

	u8 count;
	*pkt >> count;

	RemoteClient *client = getClient(pkt->getPeerId());

	if ((s16)pkt->getSize() < 1 + (int)count * 6) {
		throw con::InvalidIncomingDataException
				("GOTBLOCKS length is too short");
	}

	for (u16 i = 0; i < count; i++) {
		v3s16 p;
		*pkt >> p;
		client->GotBlock(p);
	}
}

void Server::process_PlayerPos(RemotePlayer *player, PlayerSAO *playersao,
	NetworkPacket *pkt)
{
	if (pkt->getRemainingBytes() < 12 + 12 + 4 + 4 + 4 + 1 + 1)
		return;

	v3s32 ps, ss;
	s32 f32pitch, f32yaw;
	u8 f32fov;

	*pkt >> ps;
	*pkt >> ss;
	*pkt >> f32pitch;
	*pkt >> f32yaw;

	f32 pitch = (f32)f32pitch / 100.0f;
	f32 yaw = (f32)f32yaw / 100.0f;
	u32 keyPressed = 0;

	// default behavior (in case an old client doesn't send these)
	f32 fov = 0;
	u8 wanted_range = 0;

	*pkt >> keyPressed;
	*pkt >> f32fov;
	fov = (f32)f32fov / 80.0f;
	*pkt >> wanted_range;

	v3f position((f32)ps.X / 100.0f, (f32)ps.Y / 100.0f, (f32)ps.Z / 100.0f);
	v3f speed((f32)ss.X / 100.0f, (f32)ss.Y / 100.0f, (f32)ss.Z / 100.0f);

	pitch = modulo360f(pitch);
	yaw = wrapDegrees_0_360(yaw);

	playersao->setBasePosition(position);
	player->setSpeed(speed);
	playersao->setLookPitch(pitch);
	playersao->setPlayerYaw(yaw);
	playersao->setFov(fov);
	playersao->setWantedRange(wanted_range);
	player->keyPressed = keyPressed;
	player->control.up = (keyPressed & 1);
	player->control.down = (keyPressed & 2);
	player->control.left = (keyPressed & 4);
	player->control.right = (keyPressed & 8);
	player->control.jump = (keyPressed & 16);
	player->control.aux1 = (keyPressed & 32);
	player->control.sneak = (keyPressed & 64);
	player->control.LMB = (keyPressed & 128);
	player->control.RMB = (keyPressed & 256);

	if (playersao->checkMovementCheat()) {
		// Call callbacks
		m_script->on_cheat(playersao, "moved_too_fast");
		SendMovePlayer(pkt->getPeerId());
	}
}

void Server::handleCommand_PlayerPos(NetworkPacket* pkt)
{
	RemotePlayer *player = m_env->getPlayer(pkt->getPeerId());
	if (player == NULL) {
		errorstream << "Server::ProcessData(): Canceling: "
				"No player for peer_id=" << pkt->getPeerId()
				<< " disconnecting peer!" << std::endl;
		DisconnectPeer(pkt->getPeerId());
		return;
	}

	PlayerSAO *playersao = player->getPlayerSAO();
	if (playersao == NULL) {
		errorstream << "Server::ProcessData(): Canceling: "
				"No player object for peer_id=" << pkt->getPeerId()
				<< " disconnecting peer!" << std::endl;
		DisconnectPeer(pkt->getPeerId());
		return;
	}

	// If player is dead we don't care of this packet
	if (playersao->isDead()) {
		verbosestream << "TOSERVER_PLAYERPOS: " << player->getName()
				<< " is dead. Ignoring packet";
		return;
	}

	process_PlayerPos(player, playersao, pkt);
}

void Server::handleCommand_DeletedBlocks(NetworkPacket* pkt)
{
	if (pkt->getSize() < 1)
		return;

	/*
		[0] u16 command
		[2] u8 count
		[3] v3s16 pos_0
		[3+6] v3s16 pos_1
		...
	*/

	u8 count;
	*pkt >> count;

	RemoteClient *client = getClient(pkt->getPeerId());

	if ((s16)pkt->getSize() < 1 + (int)count * 6) {
		throw con::InvalidIncomingDataException
				("DELETEDBLOCKS length is too short");
	}

	for (u16 i = 0; i < count; i++) {
		v3s16 p;
		*pkt >> p;
		client->SetBlockNotSent(p);
	}
}

void Server::handleCommand_InventoryAction(NetworkPacket* pkt)
{
	RemotePlayer *player = m_env->getPlayer(pkt->getPeerId());

	if (player == NULL) {
		errorstream << "Server::ProcessData(): Canceling: "
				"No player for peer_id=" << pkt->getPeerId()
				<< " disconnecting peer!" << std::endl;
		DisconnectPeer(pkt->getPeerId());
		return;
	}

	PlayerSAO *playersao = player->getPlayerSAO();
	if (playersao == NULL) {
		errorstream << "Server::ProcessData(): Canceling: "
				"No player object for peer_id=" << pkt->getPeerId()
				<< " disconnecting peer!" << std::endl;
		DisconnectPeer(pkt->getPeerId());
		return;
	}

	// Strip command and create a stream
	std::string datastring(pkt->getString(0), pkt->getSize());
	verbosestream << "TOSERVER_INVENTORY_ACTION: data=" << datastring
		<< std::endl;
	std::istringstream is(datastring, std::ios_base::binary);
	// Create an action
	InventoryAction *a = InventoryAction::deSerialize(is);
	if (!a) {
		infostream << "TOSERVER_INVENTORY_ACTION: "
				<< "InventoryAction::deSerialize() returned NULL"
				<< std::endl;
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
	if (a->getType() == IAction::Move) {
		IMoveAction *ma = (IMoveAction*)a;

		ma->from_inv.applyCurrentPlayer(player->getName());
		ma->to_inv.applyCurrentPlayer(player->getName());

		setInventoryModified(ma->from_inv, false);
		if (ma->from_inv != ma->to_inv) {
			setInventoryModified(ma->to_inv, false);
		}

		bool from_inv_is_current_player =
			(ma->from_inv.type == InventoryLocation::PLAYER) &&
			(ma->from_inv.name == player->getName());

		bool to_inv_is_current_player =
			(ma->to_inv.type == InventoryLocation::PLAYER) &&
			(ma->to_inv.name == player->getName());

		InventoryLocation *remote = from_inv_is_current_player ?
			&ma->to_inv : &ma->from_inv;

		// Check for out-of-range interaction
		if (remote->type == InventoryLocation::NODEMETA) {
			v3f node_pos   = intToFloat(remote->p, BS);
			v3f player_pos = player->getPlayerSAO()->getEyePosition();
			f32 d = player_pos.getDistanceFrom(node_pos);
			if (!checkInteractDistance(player, d, "inventory"))
				return;
		}

		/*
			Disable moving items out of craftpreview
		*/
		if (ma->from_list == "craftpreview") {
			infostream << "Ignoring IMoveAction from "
					<< (ma->from_inv.dump()) << ":" << ma->from_list
					<< " to " << (ma->to_inv.dump()) << ":" << ma->to_list
					<< " because src is " << ma->from_list << std::endl;
			delete a;
			return;
		}

		/*
			Disable moving items into craftresult and craftpreview
		*/
		if (ma->to_list == "craftpreview" || ma->to_list == "craftresult") {
			infostream << "Ignoring IMoveAction from "
					<< (ma->from_inv.dump()) << ":" << ma->from_list
					<< " to " << (ma->to_inv.dump()) << ":" << ma->to_list
					<< " because dst is " << ma->to_list << std::endl;
			delete a;
			return;
		}

		// Disallow moving items in elsewhere than player's inventory
		// if not allowed to interact
		if (!checkPriv(player->getName(), "interact") &&
				(!from_inv_is_current_player ||
				!to_inv_is_current_player)) {
			infostream << "Cannot move outside of player's inventory: "
					<< "No interact privilege" << std::endl;
			delete a;
			return;
		}
	}
	/*
		Handle restrictions and special cases of the drop action
	*/
	else if (a->getType() == IAction::Drop) {
		IDropAction *da = (IDropAction*)a;

		da->from_inv.applyCurrentPlayer(player->getName());

		setInventoryModified(da->from_inv, false);

		/*
			Disable dropping items out of craftpreview
		*/
		if (da->from_list == "craftpreview") {
			infostream << "Ignoring IDropAction from "
					<< (da->from_inv.dump()) << ":" << da->from_list
					<< " because src is " << da->from_list << std::endl;
			delete a;
			return;
		}

		// Disallow dropping items if not allowed to interact
		if (!checkPriv(player->getName(), "interact")) {
			delete a;
			return;
		}

		// Disallow dropping items if dead
		if (playersao->isDead()) {
			infostream << "Ignoring IDropAction from "
					<< (da->from_inv.dump()) << ":" << da->from_list
					<< " because player is dead." << std::endl;
			delete a;
			return;
		}
	}
	/*
		Handle restrictions and special cases of the craft action
	*/
	else if (a->getType() == IAction::Craft) {
		ICraftAction *ca = (ICraftAction*)a;

		ca->craft_inv.applyCurrentPlayer(player->getName());

		setInventoryModified(ca->craft_inv, false);

		//bool craft_inv_is_current_player =
		//	(ca->craft_inv.type == InventoryLocation::PLAYER) &&
		//	(ca->craft_inv.name == player->getName());

		// Disallow crafting if not allowed to interact
		if (!checkPriv(player->getName(), "interact")) {
			infostream << "Cannot craft: "
					<< "No interact privilege" << std::endl;
			delete a;
			return;
		}
	}

	// Do the action
	a->apply(this, playersao, this);
	// Eat the action
	delete a;

	SendInventory(playersao);
}

void Server::handleCommand_ChatMessage(NetworkPacket* pkt)
{
	/*
		u16 command
		u16 length
		wstring message
	*/
	u16 len;
	*pkt >> len;

	std::wstring message;
	for (u16 i = 0; i < len; i++) {
		u16 tmp_wchar;
		*pkt >> tmp_wchar;

		message += (wchar_t)tmp_wchar;
	}

	RemotePlayer *player = m_env->getPlayer(pkt->getPeerId());
	if (player == NULL) {
		errorstream << "Server::ProcessData(): Canceling: "
				"No player for peer_id=" << pkt->getPeerId()
				<< " disconnecting peer!" << std::endl;
		DisconnectPeer(pkt->getPeerId());
		return;
	}

	// Get player name of this client
	std::string name = player->getName();
	std::wstring wname = narrow_to_wide(name);

	std::wstring answer_to_sender = handleChat(name, wname, message, true, player);
	if (!answer_to_sender.empty()) {
		// Send the answer to sender
		SendChatMessage(pkt->getPeerId(), ChatMessage(CHATMESSAGE_TYPE_NORMAL,
				answer_to_sender, wname));
	}
}

void Server::handleCommand_Damage(NetworkPacket* pkt)
{
	u16 damage;

	*pkt >> damage;

	RemotePlayer *player = m_env->getPlayer(pkt->getPeerId());

	if (player == NULL) {
		errorstream << "Server::ProcessData(): Canceling: "
				"No player for peer_id=" << pkt->getPeerId()
				<< " disconnecting peer!" << std::endl;
		DisconnectPeer(pkt->getPeerId());
		return;
	}

	PlayerSAO *playersao = player->getPlayerSAO();
	if (playersao == NULL) {
		errorstream << "Server::ProcessData(): Canceling: "
				"No player object for peer_id=" << pkt->getPeerId()
				<< " disconnecting peer!" << std::endl;
		DisconnectPeer(pkt->getPeerId());
		return;
	}

	if (g_settings->getBool("enable_damage")) {
		if (playersao->isDead()) {
			verbosestream << "Server::ProcessData(): Info: "
				"Ignoring damage as player " << player->getName()
				<< " is already dead." << std::endl;
			return;
		}

		actionstream << player->getName() << " damaged by "
				<< (int)damage << " hp at " << PP(playersao->getBasePosition() / BS)
				<< std::endl;

		PlayerHPChangeReason reason(PlayerHPChangeReason::FALL);
		playersao->setHP((s32)playersao->getHP() - (s32)damage, reason);
		SendPlayerHPOrDie(playersao, reason);
	}
}

void Server::handleCommand_Password(NetworkPacket* pkt)
{
	if (pkt->getSize() != PASSWORD_SIZE * 2)
		return;

	std::string oldpwd;
	std::string newpwd;

	// Deny for clients using the new protocol
	RemoteClient* client = getClient(pkt->getPeerId(), CS_Created);
	if (client->net_proto_version >= 25) {
		infostream << "Server::handleCommand_Password(): Denying change: "
			<< " Client protocol version for peer_id=" << pkt->getPeerId()
			<< " too new!" << std::endl;
		return;
	}

	for (u16 i = 0; i < PASSWORD_SIZE - 1; i++) {
		char c = pkt->getChar(i);
		if (c == 0)
			break;
		oldpwd += c;
	}

	for (u16 i = 0; i < PASSWORD_SIZE - 1; i++) {
		char c = pkt->getChar(PASSWORD_SIZE + i);
		if (c == 0)
			break;
		newpwd += c;
	}

	RemotePlayer *player = m_env->getPlayer(pkt->getPeerId());
	if (player == NULL) {
		errorstream << "Server::ProcessData(): Canceling: "
				"No player for peer_id=" << pkt->getPeerId()
				<< " disconnecting peer!" << std::endl;
		DisconnectPeer(pkt->getPeerId());
		return;
	}

	if (!base64_is_valid(newpwd)) {
		infostream<<"Server: " << player->getName() <<
				" supplied invalid password hash" << std::endl;
		// Wrong old password supplied!!
		SendChatMessage(pkt->getPeerId(), ChatMessage(CHATMESSAGE_TYPE_SYSTEM,
				L"Invalid new password hash supplied. Password NOT changed."));
		return;
	}

	infostream << "Server: Client requests a password change from "
			<< "'" << oldpwd << "' to '" << newpwd << "'" << std::endl;

	std::string playername = player->getName();

	std::string checkpwd;
	m_script->getAuth(playername, &checkpwd, NULL);

	if (oldpwd != checkpwd) {
		infostream << "Server: invalid old password" << std::endl;
		// Wrong old password supplied!!
		SendChatMessage(pkt->getPeerId(), ChatMessage(CHATMESSAGE_TYPE_SYSTEM,
				L"Invalid old password supplied. Password NOT changed."));
		return;
	}

	bool success = m_script->setPassword(playername, newpwd);
	if (success) {
		actionstream << player->getName() << " changes password" << std::endl;
		SendChatMessage(pkt->getPeerId(), ChatMessage(CHATMESSAGE_TYPE_SYSTEM,
				L"Password change successful."));
	} else {
		actionstream << player->getName() << " tries to change password but "
				<< "it fails" << std::endl;
		SendChatMessage(pkt->getPeerId(), ChatMessage(CHATMESSAGE_TYPE_SYSTEM,
				L"Password change failed or unavailable."));
	}
}

void Server::handleCommand_PlayerItem(NetworkPacket* pkt)
{
	if (pkt->getSize() < 2)
		return;

	RemotePlayer *player = m_env->getPlayer(pkt->getPeerId());

	if (player == NULL) {
		errorstream << "Server::ProcessData(): Canceling: "
				"No player for peer_id=" << pkt->getPeerId()
				<< " disconnecting peer!" << std::endl;
		DisconnectPeer(pkt->getPeerId());
		return;
	}

	PlayerSAO *playersao = player->getPlayerSAO();
	if (playersao == NULL) {
		errorstream << "Server::ProcessData(): Canceling: "
				"No player object for peer_id=" << pkt->getPeerId()
				<< " disconnecting peer!" << std::endl;
		DisconnectPeer(pkt->getPeerId());
		return;
	}

	u16 item;

	*pkt >> item;

	playersao->setWieldIndex(item);
}

void Server::handleCommand_Respawn(NetworkPacket* pkt)
{
	RemotePlayer *player = m_env->getPlayer(pkt->getPeerId());
	if (player == NULL) {
		errorstream << "Server::ProcessData(): Canceling: "
				"No player for peer_id=" << pkt->getPeerId()
				<< " disconnecting peer!" << std::endl;
		DisconnectPeer(pkt->getPeerId());
		return;
	}

	PlayerSAO *playersao = player->getPlayerSAO();
	assert(playersao);

	if (!playersao->isDead())
		return;

	RespawnPlayer(pkt->getPeerId());

	actionstream << player->getName() << " respawns at "
			<< PP(playersao->getBasePosition() / BS) << std::endl;

	// ActiveObject is added to environment in AsyncRunStep after
	// the previous addition has been successfully removed
}

bool Server::checkInteractDistance(RemotePlayer *player, const f32 d, const std::string &what)
{
	PlayerSAO *playersao = player->getPlayerSAO();
	const InventoryList *hlist = playersao->getInventory()->getList("hand");
	const ItemDefinition &playeritem_def =
		playersao->getWieldedItem().getDefinition(m_itemdef);
	const ItemDefinition &hand_def =
		hlist ? hlist->getItem(0).getDefinition(m_itemdef) : m_itemdef->get("");

	float max_d = BS * playeritem_def.range;
	float max_d_hand = BS * hand_def.range;

	if (max_d < 0 && max_d_hand >= 0)
		max_d = max_d_hand;
	else if (max_d < 0)
		max_d = BS * 4.0f;

	// Cube diagonal * 1.5 for maximal supported node extents:
	// sqrt(3) * 1.5 ≅ 2.6
	if (d > max_d + 2.6f * BS) {
		actionstream << "Player " << player->getName()
				<< " tried to access " << what
				<< " from too far: "
				<< "d=" << d <<", max_d=" << max_d
				<< ". ignoring." << std::endl;
		// Call callbacks
		m_script->on_cheat(playersao, "interacted_too_far");
		return false;
	}
	return true;
}

void Server::handleCommand_Interact(NetworkPacket* pkt)
{
	/*
		[0] u16 command
		[2] u8 action
		[3] u16 item
		[5] u32 length of the next item (plen)
		[9] serialized PointedThing
		[9 + plen] player position information
		actions:
		0: start digging (from undersurface) or use
		1: stop digging (all parameters ignored)
		2: digging completed
		3: place block or item (to abovesurface)
		4: use item
		5: rightclick air ("activate")
	*/
	u8 action;
	u16 item_i;
	*pkt >> action;
	*pkt >> item_i;
	std::istringstream tmp_is(pkt->readLongString(), std::ios::binary);
	PointedThing pointed;
	pointed.deSerialize(tmp_is);

	verbosestream << "TOSERVER_INTERACT: action=" << (int)action << ", item="
			<< item_i << ", pointed=" << pointed.dump() << std::endl;

	RemotePlayer *player = m_env->getPlayer(pkt->getPeerId());

	if (player == NULL) {
		errorstream << "Server::ProcessData(): Canceling: "
				"No player for peer_id=" << pkt->getPeerId()
				<< " disconnecting peer!" << std::endl;
		DisconnectPeer(pkt->getPeerId());
		return;
	}

	PlayerSAO *playersao = player->getPlayerSAO();
	if (playersao == NULL) {
		errorstream << "Server::ProcessData(): Canceling: "
				"No player object for peer_id=" << pkt->getPeerId()
				<< " disconnecting peer!" << std::endl;
		DisconnectPeer(pkt->getPeerId());
		return;
	}

	if (playersao->isDead()) {
		actionstream << "Server: NoCheat: " << player->getName()
				<< " tried to interact while dead; ignoring." << std::endl;
		if (pointed.type == POINTEDTHING_NODE) {
			// Re-send block to revert change on client-side
			RemoteClient *client = getClient(pkt->getPeerId());
			v3s16 blockpos = getNodeBlockPos(pointed.node_undersurface);
			client->SetBlockNotSent(blockpos);
		}
		// Call callbacks
		m_script->on_cheat(playersao, "interacted_while_dead");
		return;
	}

	process_PlayerPos(player, playersao, pkt);

	v3f player_pos = playersao->getLastGoodPosition();

	// Update wielded item
	playersao->setWieldIndex(item_i);

	// Get pointed to node (undefined if not POINTEDTYPE_NODE)
	v3s16 p_under = pointed.node_undersurface;
	v3s16 p_above = pointed.node_abovesurface;

	// Get pointed to object (NULL if not POINTEDTYPE_OBJECT)
	ServerActiveObject *pointed_object = NULL;
	if (pointed.type == POINTEDTHING_OBJECT) {
		pointed_object = m_env->getActiveObject(pointed.object_id);
		if (pointed_object == NULL) {
			verbosestream << "TOSERVER_INTERACT: "
				"pointed object is NULL" << std::endl;
			return;
		}

	}

	v3f pointed_pos_under = player_pos;
	v3f pointed_pos_above = player_pos;
	if (pointed.type == POINTEDTHING_NODE) {
		pointed_pos_under = intToFloat(p_under, BS);
		pointed_pos_above = intToFloat(p_above, BS);
	}
	else if (pointed.type == POINTEDTHING_OBJECT) {
		pointed_pos_under = pointed_object->getBasePosition();
		pointed_pos_above = pointed_pos_under;
	}

	/*
		Make sure the player is allowed to do it
	*/
	if (!checkPriv(player->getName(), "interact")) {
		actionstream<<player->getName()<<" attempted to interact with "
				<<pointed.dump()<<" without 'interact' privilege"
				<<std::endl;
		// Re-send block to revert change on client-side
		RemoteClient *client = getClient(pkt->getPeerId());
		// Digging completed -> under
		if (action == 2) {
			v3s16 blockpos = getNodeBlockPos(floatToInt(pointed_pos_under, BS));
			client->SetBlockNotSent(blockpos);
		}
		// Placement -> above
		else if (action == 3) {
			v3s16 blockpos = getNodeBlockPos(floatToInt(pointed_pos_above, BS));
			client->SetBlockNotSent(blockpos);
		}
		return;
	}

	/*
		Check that target is reasonably close
		(only when digging or placing things)
	*/
	static thread_local const bool enable_anticheat =
			!g_settings->getBool("disable_anticheat");

	if ((action == 0 || action == 2 || action == 3 || action == 4) &&
			enable_anticheat && !isSingleplayer()) {
		float d = playersao->getEyePosition()
			.getDistanceFrom(pointed_pos_under);

		if (!checkInteractDistance(player, d, pointed.dump())) {
			// Re-send block to revert change on client-side
			RemoteClient *client = getClient(pkt->getPeerId());
			v3s16 blockpos = getNodeBlockPos(floatToInt(pointed_pos_under, BS));
			client->SetBlockNotSent(blockpos);
			return;
		}
	}

	/*
		If something goes wrong, this player is to blame
	*/
	RollbackScopeActor rollback_scope(m_rollback,
			std::string("player:")+player->getName());

	/*
		0: start digging or punch object
	*/
	if (action == 0) {
		if (pointed.type == POINTEDTHING_NODE) {
			MapNode n(CONTENT_IGNORE);
			bool pos_ok;

			n = m_env->getMap().getNodeNoEx(p_under, &pos_ok);
			if (!pos_ok) {
				infostream << "Server: Not punching: Node not found."
						<< " Adding block to emerge queue."
						<< std::endl;
				m_emerge->enqueueBlockEmerge(pkt->getPeerId(),
					getNodeBlockPos(p_above), false);
			}

			if (n.getContent() != CONTENT_IGNORE)
				m_script->node_on_punch(p_under, n, playersao, pointed);

			// Cheat prevention
			playersao->noCheatDigStart(p_under);
		}
		else if (pointed.type == POINTEDTHING_OBJECT) {
			// Skip if object can't be interacted with anymore
			if (pointed_object->isGone())
				return;

			actionstream<<player->getName()<<" punches object "
					<<pointed.object_id<<": "
					<<pointed_object->getDescription()<<std::endl;

			ItemStack punchitem = playersao->getWieldedItemOrHand();
			ToolCapabilities toolcap =
					punchitem.getToolCapabilities(m_itemdef);
			v3f dir = (pointed_object->getBasePosition() -
					(playersao->getBasePosition() + playersao->getEyeOffset())
						).normalize();
			float time_from_last_punch =
				playersao->resetTimeFromLastPunch();

			u16 src_original_hp = pointed_object->getHP();
			u16 dst_origin_hp = playersao->getHP();

			pointed_object->punch(dir, &toolcap, playersao,
					time_from_last_punch);

			// If the object is a player and its HP changed
			if (src_original_hp != pointed_object->getHP() &&
					pointed_object->getType() == ACTIVEOBJECT_TYPE_PLAYER) {
				SendPlayerHPOrDie((PlayerSAO *)pointed_object,
						PlayerHPChangeReason(PlayerHPChangeReason::PLAYER_PUNCH, playersao));
			}

			// If the puncher is a player and its HP changed
			if (dst_origin_hp != playersao->getHP())
				SendPlayerHPOrDie(playersao,
						PlayerHPChangeReason(PlayerHPChangeReason::PLAYER_PUNCH, pointed_object));
		}

	} // action == 0

	/*
		1: stop digging
	*/
	else if (action == 1) {
	} // action == 1

	/*
		2: Digging completed
	*/
	else if (action == 2) {
		// Only digging of nodes
		if (pointed.type == POINTEDTHING_NODE) {
			bool pos_ok;
			MapNode n = m_env->getMap().getNodeNoEx(p_under, &pos_ok);
			if (!pos_ok) {
				infostream << "Server: Not finishing digging: Node not found."
						<< " Adding block to emerge queue."
						<< std::endl;
				m_emerge->enqueueBlockEmerge(pkt->getPeerId(),
					getNodeBlockPos(p_above), false);
			}

			/* Cheat prevention */
			bool is_valid_dig = true;
			if (enable_anticheat && !isSingleplayer()) {
				v3s16 nocheat_p = playersao->getNoCheatDigPos();
				float nocheat_t = playersao->getNoCheatDigTime();
				playersao->noCheatDigEnd();
				// If player didn't start digging this, ignore dig
				if (nocheat_p != p_under) {
					infostream << "Server: NoCheat: " << player->getName()
							<< " started digging "
							<< PP(nocheat_p) << " and completed digging "
							<< PP(p_under) << "; not digging." << std::endl;
					is_valid_dig = false;
					// Call callbacks
					m_script->on_cheat(playersao, "finished_unknown_dig");
				}
				// Get player's wielded item
				ItemStack playeritem = playersao->getWieldedItemOrHand();
				ToolCapabilities playeritem_toolcap =
						playeritem.getToolCapabilities(m_itemdef);
				// Get diggability and expected digging time
				DigParams params = getDigParams(m_nodedef->get(n).groups,
						&playeritem_toolcap);
				// If can't dig, try hand
				if (!params.diggable) {
					InventoryList *hlist = playersao->getInventory()->getList("hand");
					const ToolCapabilities *tp = hlist
						? &hlist->getItem(0).getToolCapabilities(m_itemdef)
						: m_itemdef->get("").tool_capabilities;

					if (tp)
						params = getDigParams(m_nodedef->get(n).groups, tp);
				}
				// If can't dig, ignore dig
				if (!params.diggable) {
					infostream << "Server: NoCheat: " << player->getName()
							<< " completed digging " << PP(p_under)
							<< ", which is not diggable with tool. not digging."
							<< std::endl;
					is_valid_dig = false;
					// Call callbacks
					m_script->on_cheat(playersao, "dug_unbreakable");
				}
				// Check digging time
				// If already invalidated, we don't have to
				if (!is_valid_dig) {
					// Well not our problem then
				}
				// Clean and long dig
				else if (params.time > 2.0 && nocheat_t * 1.2 > params.time) {
					// All is good, but grab time from pool; don't care if
					// it's actually available
					playersao->getDigPool().grab(params.time);
				}
				// Short or laggy dig
				// Try getting the time from pool
				else if (playersao->getDigPool().grab(params.time)) {
					// All is good
				}
				// Dig not possible
				else {
					infostream << "Server: NoCheat: " << player->getName()
							<< " completed digging " << PP(p_under)
							<< "too fast; not digging." << std::endl;
					is_valid_dig = false;
					// Call callbacks
					m_script->on_cheat(playersao, "dug_too_fast");
				}
			}

			/* Actually dig node */

			if (is_valid_dig && n.getContent() != CONTENT_IGNORE)
				m_script->node_on_dig(p_under, n, playersao);

			v3s16 blockpos = getNodeBlockPos(floatToInt(pointed_pos_under, BS));
			RemoteClient *client = getClient(pkt->getPeerId());
			// Send unusual result (that is, node not being removed)
			if (m_env->getMap().getNodeNoEx(p_under).getContent() != CONTENT_AIR) {
				// Re-send block to revert change on client-side
				client->SetBlockNotSent(blockpos);
			}
			else {
				client->ResendBlockIfOnWire(blockpos);
			}
		}
	} // action == 2

	/*
		3: place block or right-click object
	*/
	else if (action == 3) {
		ItemStack item = playersao->getWieldedItem();

		// Reset build time counter
		if (pointed.type == POINTEDTHING_NODE &&
				item.getDefinition(m_itemdef).type == ITEM_NODE)
			getClient(pkt->getPeerId())->m_time_from_building = 0.0;

		if (pointed.type == POINTEDTHING_OBJECT) {
			// Right click object

			// Skip if object can't be interacted with anymore
			if (pointed_object->isGone())
				return;

			actionstream << player->getName() << " right-clicks object "
					<< pointed.object_id << ": "
					<< pointed_object->getDescription() << std::endl;

			// Do stuff
			pointed_object->rightClick(playersao);
		}
		else if (m_script->item_OnPlace(
				item, playersao, pointed)) {
			// Placement was handled in lua

			// Apply returned ItemStack
			if (playersao->setWieldedItem(item)) {
				SendInventory(playersao);
			}
		}

		// If item has node placement prediction, always send the
		// blocks to make sure the client knows what exactly happened
		RemoteClient *client = getClient(pkt->getPeerId());
		v3s16 blockpos = getNodeBlockPos(floatToInt(pointed_pos_above, BS));
		v3s16 blockpos2 = getNodeBlockPos(floatToInt(pointed_pos_under, BS));
		if (!item.getDefinition(m_itemdef).node_placement_prediction.empty()) {
			client->SetBlockNotSent(blockpos);
			if (blockpos2 != blockpos) {
				client->SetBlockNotSent(blockpos2);
			}
		}
		else {
			client->ResendBlockIfOnWire(blockpos);
			if (blockpos2 != blockpos) {
				client->ResendBlockIfOnWire(blockpos2);
			}
		}
	} // action == 3

	/*
		4: use
	*/
	else if (action == 4) {
		ItemStack item = playersao->getWieldedItem();

		actionstream << player->getName() << " uses " << item.name
				<< ", pointing at " << pointed.dump() << std::endl;

		if (m_script->item_OnUse(
				item, playersao, pointed)) {
			// Apply returned ItemStack
			if (playersao->setWieldedItem(item)) {
				SendInventory(playersao);
			}
		}

	} // action == 4

	/*
		5: rightclick air
	*/
	else if (action == 5) {
		ItemStack item = playersao->getWieldedItem();

		actionstream << player->getName() << " activates "
				<< item.name << std::endl;

		if (m_script->item_OnSecondaryUse(
				item, playersao)) {
			if( playersao->setWieldedItem(item)) {
				SendInventory(playersao);
			}
		}
	}


	/*
		Catch invalid actions
	*/
	else {
		warningstream << "Server: Invalid action "
				<< action << std::endl;
	}
}

void Server::handleCommand_RemovedSounds(NetworkPacket* pkt)
{
	u16 num;
	*pkt >> num;
	for (u16 k = 0; k < num; k++) {
		s32 id;

		*pkt >> id;

		std::unordered_map<s32, ServerPlayingSound>::iterator i =
			m_playing_sounds.find(id);
		if (i == m_playing_sounds.end())
			continue;

		ServerPlayingSound &psound = i->second;
		psound.clients.erase(pkt->getPeerId());
		if (psound.clients.empty())
			m_playing_sounds.erase(i++);
	}
}

void Server::handleCommand_NodeMetaFields(NetworkPacket* pkt)
{
	v3s16 p;
	std::string formname;
	u16 num;

	*pkt >> p >> formname >> num;

	StringMap fields;
	for (u16 k = 0; k < num; k++) {
		std::string fieldname;
		*pkt >> fieldname;
		fields[fieldname] = pkt->readLongString();
	}

	RemotePlayer *player = m_env->getPlayer(pkt->getPeerId());

	if (player == NULL) {
		errorstream << "Server::ProcessData(): Canceling: "
				"No player for peer_id=" << pkt->getPeerId()
				<< " disconnecting peer!" << std::endl;
		DisconnectPeer(pkt->getPeerId());
		return;
	}

	PlayerSAO *playersao = player->getPlayerSAO();
	if (playersao == NULL) {
		errorstream << "Server::ProcessData(): Canceling: "
				"No player object for peer_id=" << pkt->getPeerId()
				<< " disconnecting peer!"  << std::endl;
		DisconnectPeer(pkt->getPeerId());
		return;
	}

	// If something goes wrong, this player is to blame
	RollbackScopeActor rollback_scope(m_rollback,
			std::string("player:")+player->getName());

	// Check the target node for rollback data; leave others unnoticed
	RollbackNode rn_old(&m_env->getMap(), p, this);

	m_script->node_on_receive_fields(p, formname, fields, playersao);

	// Report rollback data
	RollbackNode rn_new(&m_env->getMap(), p, this);
	if (rollback() && rn_new != rn_old) {
		RollbackAction action;
		action.setSetNode(p, rn_old, rn_new);
		rollback()->reportAction(action);
	}
}

void Server::handleCommand_InventoryFields(NetworkPacket* pkt)
{
	std::string client_formspec_name;
	u16 num;

	*pkt >> client_formspec_name >> num;

	StringMap fields;
	for (u16 k = 0; k < num; k++) {
		std::string fieldname;
		*pkt >> fieldname;
		fields[fieldname] = pkt->readLongString();
	}

	RemotePlayer *player = m_env->getPlayer(pkt->getPeerId());

	if (player == NULL) {
		errorstream << "Server::ProcessData(): Canceling: "
				"No player for peer_id=" << pkt->getPeerId()
				<< " disconnecting peer!" << std::endl;
		DisconnectPeer(pkt->getPeerId());
		return;
	}

	PlayerSAO *playersao = player->getPlayerSAO();
	if (playersao == NULL) {
		errorstream << "Server::ProcessData(): Canceling: "
				"No player object for peer_id=" << pkt->getPeerId()
				<< " disconnecting peer!" << std::endl;
		DisconnectPeer(pkt->getPeerId());
		return;
	}

	if (client_formspec_name.empty()) { // pass through inventory submits
		m_script->on_playerReceiveFields(playersao, client_formspec_name, fields);
		return;
	}

	// verify that we displayed the formspec to the user
	const auto peer_state_iterator = m_formspec_state_data.find(pkt->getPeerId());
	if (peer_state_iterator != m_formspec_state_data.end()) {
		const std::string &server_formspec_name = peer_state_iterator->second;
		if (client_formspec_name == server_formspec_name) {
			auto it = fields.find("quit");
			if (it != fields.end() && it->second == "true")
				m_formspec_state_data.erase(peer_state_iterator);

			m_script->on_playerReceiveFields(playersao, client_formspec_name, fields);
			return;
		}
		actionstream << "'" << player->getName()
			<< "' submitted formspec ('" << client_formspec_name
			<< "') but the name of the formspec doesn't match the"
			" expected name ('" << server_formspec_name << "')";

	} else {
		actionstream << "'" << player->getName()
			<< "' submitted formspec ('" << client_formspec_name
			<< "') but server hasn't sent formspec to client";
	}
	actionstream << ", possible exploitation attempt" << std::endl;
}

void Server::handleCommand_FirstSrp(NetworkPacket* pkt)
{
	RemoteClient* client = getClient(pkt->getPeerId(), CS_Invalid);
	ClientState cstate = client->getState();

	std::string playername = client->getName();

	std::string salt;
	std::string verification_key;

	std::string addr_s = getPeerAddress(pkt->getPeerId()).serializeString();
	u8 is_empty;

	*pkt >> salt >> verification_key >> is_empty;

	verbosestream << "Server: Got TOSERVER_FIRST_SRP from " << addr_s
		<< ", with is_empty=" << (is_empty == 1) << std::endl;

	// Either this packet is sent because the user is new or to change the password
	if (cstate == CS_HelloSent) {
		if (!client->isMechAllowed(AUTH_MECHANISM_FIRST_SRP)) {
			actionstream << "Server: Client from " << addr_s
					<< " tried to set password without being "
					<< "authenticated, or the username being new." << std::endl;
			DenyAccess(pkt->getPeerId(), SERVER_ACCESSDENIED_UNEXPECTED_DATA);
			return;
		}

		if (!isSingleplayer() &&
				g_settings->getBool("disallow_empty_password") &&
				is_empty == 1) {
			actionstream << "Server: " << playername
					<< " supplied empty password from " << addr_s << std::endl;
			DenyAccess(pkt->getPeerId(), SERVER_ACCESSDENIED_EMPTY_PASSWORD);
			return;
		}

		std::string initial_ver_key;

		initial_ver_key = encode_srp_verifier(verification_key, salt);
		m_script->createAuth(playername, initial_ver_key);

		acceptAuth(pkt->getPeerId(), false);
	} else {
		if (cstate < CS_SudoMode) {
			infostream << "Server::ProcessData(): Ignoring TOSERVER_FIRST_SRP from "
					<< addr_s << ": " << "Client has wrong state " << cstate << "."
					<< std::endl;
			return;
		}
		m_clients.event(pkt->getPeerId(), CSE_SudoLeave);
		std::string pw_db_field = encode_srp_verifier(verification_key, salt);
		bool success = m_script->setPassword(playername, pw_db_field);
		if (success) {
			actionstream << playername << " changes password" << std::endl;
			SendChatMessage(pkt->getPeerId(), ChatMessage(CHATMESSAGE_TYPE_SYSTEM,
					L"Password change successful."));
		} else {
			actionstream << playername << " tries to change password but "
				<< "it fails" << std::endl;
			SendChatMessage(pkt->getPeerId(), ChatMessage(CHATMESSAGE_TYPE_SYSTEM,
					L"Password change failed or unavailable."));
		}
	}
}

void Server::handleCommand_SrpBytesA(NetworkPacket* pkt)
{
	RemoteClient* client = getClient(pkt->getPeerId(), CS_Invalid);
	ClientState cstate = client->getState();

	bool wantSudo = (cstate == CS_Active);

	if (!((cstate == CS_HelloSent) || (cstate == CS_Active))) {
		actionstream << "Server: got SRP _A packet in wrong state "
			<< cstate << " from "
			<< getPeerAddress(pkt->getPeerId()).serializeString()
			<< ". Ignoring." << std::endl;
		return;
	}

	if (client->chosen_mech != AUTH_MECHANISM_NONE) {
		actionstream << "Server: got SRP _A packet, while auth"
			<< "is already going on with mech " << client->chosen_mech
			<< " from " << getPeerAddress(pkt->getPeerId()).serializeString()
			<< " (wantSudo=" << wantSudo << "). Ignoring." << std::endl;
		if (wantSudo) {
			DenySudoAccess(pkt->getPeerId());
			return;
		}

		DenyAccess(pkt->getPeerId(), SERVER_ACCESSDENIED_UNEXPECTED_DATA);
		return;
	}

	std::string bytes_A;
	u8 based_on;
	*pkt >> bytes_A >> based_on;

	infostream << "Server: TOSERVER_SRP_BYTES_A received with "
		<< "based_on=" << int(based_on) << " and len_A="
		<< bytes_A.length() << "." << std::endl;

	AuthMechanism chosen = (based_on == 0) ?
		AUTH_MECHANISM_LEGACY_PASSWORD : AUTH_MECHANISM_SRP;

	if (wantSudo) {
		if (!client->isSudoMechAllowed(chosen)) {
			actionstream << "Server: Player \"" << client->getName()
				<< "\" at " << getPeerAddress(pkt->getPeerId()).serializeString()
				<< " tried to change password using unallowed mech "
				<< chosen << "." << std::endl;
			DenySudoAccess(pkt->getPeerId());
			return;
		}
	} else {
		if (!client->isMechAllowed(chosen)) {
			actionstream << "Server: Client tried to authenticate from "
				<< getPeerAddress(pkt->getPeerId()).serializeString()
				<< " using unallowed mech " << chosen << "." << std::endl;
			DenyAccess(pkt->getPeerId(), SERVER_ACCESSDENIED_UNEXPECTED_DATA);
			return;
		}
	}

	client->chosen_mech = chosen;

	std::string salt;
	std::string verifier;

	if (based_on == 0) {

		generate_srp_verifier_and_salt(client->getName(), client->enc_pwd,
			&verifier, &salt);
	} else if (!decode_srp_verifier_and_salt(client->enc_pwd, &verifier, &salt)) {
		// Non-base64 errors should have been catched in the init handler
		actionstream << "Server: User " << client->getName()
			<< " tried to log in, but srp verifier field"
			<< " was invalid (most likely invalid base64)." << std::endl;
		DenyAccess(pkt->getPeerId(), SERVER_ACCESSDENIED_SERVER_FAIL);
		return;
	}

	char *bytes_B = 0;
	size_t len_B = 0;

	client->auth_data = srp_verifier_new(SRP_SHA256, SRP_NG_2048,
		client->getName().c_str(),
		(const unsigned char *) salt.c_str(), salt.size(),
		(const unsigned char *) verifier.c_str(), verifier.size(),
		(const unsigned char *) bytes_A.c_str(), bytes_A.size(),
		NULL, 0,
		(unsigned char **) &bytes_B, &len_B, NULL, NULL);

	if (!bytes_B) {
		actionstream << "Server: User " << client->getName()
			<< " tried to log in, SRP-6a safety check violated in _A handler."
			<< std::endl;
		if (wantSudo) {
			DenySudoAccess(pkt->getPeerId());
			return;
		}

		DenyAccess(pkt->getPeerId(), SERVER_ACCESSDENIED_UNEXPECTED_DATA);
		return;
	}

	NetworkPacket resp_pkt(TOCLIENT_SRP_BYTES_S_B, 0, pkt->getPeerId());
	resp_pkt << salt << std::string(bytes_B, len_B);
	Send(&resp_pkt);
}

void Server::handleCommand_SrpBytesM(NetworkPacket* pkt)
{
	RemoteClient* client = getClient(pkt->getPeerId(), CS_Invalid);
	ClientState cstate = client->getState();

	bool wantSudo = (cstate == CS_Active);

	verbosestream << "Server: Received TOCLIENT_SRP_BYTES_M." << std::endl;

	if (!((cstate == CS_HelloSent) || (cstate == CS_Active))) {
		actionstream << "Server: got SRP _M packet in wrong state "
			<< cstate << " from "
			<< getPeerAddress(pkt->getPeerId()).serializeString()
			<< ". Ignoring." << std::endl;
		return;
	}

	if (client->chosen_mech != AUTH_MECHANISM_SRP &&
			client->chosen_mech != AUTH_MECHANISM_LEGACY_PASSWORD) {
		actionstream << "Server: got SRP _M packet, while auth"
			<< "is going on with mech " << client->chosen_mech
			<< " from " << getPeerAddress(pkt->getPeerId()).serializeString()
			<< " (wantSudo=" << wantSudo << "). Denying." << std::endl;
		if (wantSudo) {
			DenySudoAccess(pkt->getPeerId());
			return;
		}

		DenyAccess(pkt->getPeerId(), SERVER_ACCESSDENIED_UNEXPECTED_DATA);
		return;
	}

	std::string bytes_M;
	*pkt >> bytes_M;

	if (srp_verifier_get_session_key_length((SRPVerifier *) client->auth_data)
			!= bytes_M.size()) {
		actionstream << "Server: User " << client->getName()
			<< " at " << getPeerAddress(pkt->getPeerId()).serializeString()
			<< " sent bytes_M with invalid length " << bytes_M.size() << std::endl;
		DenyAccess(pkt->getPeerId(), SERVER_ACCESSDENIED_UNEXPECTED_DATA);
		return;
	}

	unsigned char *bytes_HAMK = 0;

	srp_verifier_verify_session((SRPVerifier *) client->auth_data,
		(unsigned char *)bytes_M.c_str(), &bytes_HAMK);

	if (!bytes_HAMK) {
		if (wantSudo) {
			actionstream << "Server: User " << client->getName()
				<< " at " << getPeerAddress(pkt->getPeerId()).serializeString()
				<< " tried to change their password, but supplied wrong"
				<< " (SRP) password for authentication." << std::endl;
			DenySudoAccess(pkt->getPeerId());
			return;
		}

		std::string ip = getPeerAddress(pkt->getPeerId()).serializeString();
		actionstream << "Server: User " << client->getName()
			<< " at " << ip
			<< " supplied wrong password (auth mechanism: SRP)."
			<< std::endl;
		m_script->on_auth_failure(client->getName(), ip);
		DenyAccess(pkt->getPeerId(), SERVER_ACCESSDENIED_WRONG_PASSWORD);
		return;
	}

	if (client->create_player_on_auth_success) {
		std::string playername = client->getName();
		m_script->createAuth(playername, client->enc_pwd);

		std::string checkpwd; // not used, but needed for passing something
		if (!m_script->getAuth(playername, &checkpwd, NULL)) {
			actionstream << "Server: " << playername << " cannot be authenticated"
				<< " (auth handler does not work?)" << std::endl;
			DenyAccess(pkt->getPeerId(), SERVER_ACCESSDENIED_SERVER_FAIL);
			return;
		}
		client->create_player_on_auth_success = false;
	}

	acceptAuth(pkt->getPeerId(), wantSudo);
}

/*
 * Mod channels
 */

void Server::handleCommand_ModChannelJoin(NetworkPacket *pkt)
{
	std::string channel_name;
	*pkt >> channel_name;

	NetworkPacket resp_pkt(TOCLIENT_MODCHANNEL_SIGNAL, 1 + 2 + channel_name.size(),
		pkt->getPeerId());

	// Send signal to client to notify join succeed or not
	if (g_settings->getBool("enable_mod_channels") &&
			m_modchannel_mgr->joinChannel(channel_name, pkt->getPeerId())) {
		resp_pkt << (u8) MODCHANNEL_SIGNAL_JOIN_OK;
		infostream << "Peer " << pkt->getPeerId() << " joined channel " << channel_name
				<< std::endl;
	}
	else {
		resp_pkt << (u8)MODCHANNEL_SIGNAL_JOIN_FAILURE;
		infostream << "Peer " << pkt->getPeerId() << " tried to join channel "
			<< channel_name << ", but was already registered." << std::endl;
	}
	resp_pkt << channel_name;
	Send(&resp_pkt);
}

void Server::handleCommand_ModChannelLeave(NetworkPacket *pkt)
{
	std::string channel_name;
	*pkt >> channel_name;

	NetworkPacket resp_pkt(TOCLIENT_MODCHANNEL_SIGNAL, 1 + 2 + channel_name.size(),
		pkt->getPeerId());

	// Send signal to client to notify join succeed or not
	if (g_settings->getBool("enable_mod_channels") &&
			m_modchannel_mgr->leaveChannel(channel_name, pkt->getPeerId())) {
		resp_pkt << (u8)MODCHANNEL_SIGNAL_LEAVE_OK;
		infostream << "Peer " << pkt->getPeerId() << " left channel " << channel_name
				<< std::endl;
	} else {
		resp_pkt << (u8) MODCHANNEL_SIGNAL_LEAVE_FAILURE;
		infostream << "Peer " << pkt->getPeerId() << " left channel " << channel_name
				<< ", but was not registered." << std::endl;
	}
	resp_pkt << channel_name;
	Send(&resp_pkt);
}

void Server::handleCommand_ModChannelMsg(NetworkPacket *pkt)
{
	std::string channel_name, channel_msg;
	*pkt >> channel_name >> channel_msg;

	verbosestream << "Mod channel message received from peer " << pkt->getPeerId()
			<< " on channel " << channel_name << " message: " << channel_msg << std::endl;

	// If mod channels are not enabled, discard message
	if (!g_settings->getBool("enable_mod_channels")) {
		return;
	}

	// If channel not registered, signal it and ignore message
	if (!m_modchannel_mgr->channelRegistered(channel_name)) {
		NetworkPacket resp_pkt(TOCLIENT_MODCHANNEL_SIGNAL, 1 + 2 + channel_name.size(),
			pkt->getPeerId());
		resp_pkt << (u8)MODCHANNEL_SIGNAL_CHANNEL_NOT_REGISTERED << channel_name;
		Send(&resp_pkt);
		return;
	}

	// @TODO: filter, rate limit

	broadcastModChannelMessage(channel_name, channel_msg, pkt->getPeerId());
}
