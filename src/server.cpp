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

#include "server.h"
#include <iostream>
#include <queue>
#include <algorithm>
#include "network/networkprotocol.h"
#include "network/serveropcodes.h"
#include "ban.h"
#include "environment.h"
#include "map.h"
#include "threading/mutex_auto_lock.h"
#include "constants.h"
#include "voxel.h"
#include "config.h"
#include "version.h"
#include "filesys.h"
#include "mapblock.h"
#include "serverobject.h"
#include "genericobject.h"
#include "settings.h"
#include "profiler.h"
#include "log.h"
#include "scripting_server.h"
#include "nodedef.h"
#include "itemdef.h"
#include "craftdef.h"
#include "emerge.h"
#include "mapgen.h"
#include "mg_biome.h"
#include "content_mapnode.h"
#include "content_nodemeta.h"
#include "content_abm.h"
#include "content_sao.h"
#include "mods.h"
#include "event_manager.h"
#include "serverlist.h"
#include "util/string.h"
#include "rollback.h"
#include "util/serialize.h"
#include "util/thread.h"
#include "defaultsettings.h"
#include "util/base64.h"
#include "util/sha1.h"
#include "util/hex.h"
#include "database.h"

class ClientNotFoundException : public BaseException
{
public:
	ClientNotFoundException(const char *s):
		BaseException(s)
	{}
};

class ServerThread : public Thread
{
public:

	ServerThread(Server *server):
		Thread("Server"),
		m_server(server)
	{}

	void *run();

private:
	Server *m_server;
};

void *ServerThread::run()
{
	DSTACK(FUNCTION_NAME);
	BEGIN_DEBUG_EXCEPTION_HANDLER

	m_server->AsyncRunStep(true);

	while (!stopRequested()) {
		try {
			//TimeTaker timer("AsyncRunStep() + Receive()");

			m_server->AsyncRunStep();

			m_server->Receive();

		} catch (con::NoIncomingDataException &e) {
		} catch (con::PeerNotFoundException &e) {
			infostream<<"Server: PeerNotFoundException"<<std::endl;
		} catch (ClientNotFoundException &e) {
		} catch (con::ConnectionBindFailed &e) {
			m_server->setAsyncFatalError(e.what());
		} catch (LuaError &e) {
			m_server->setAsyncFatalError(
					"ServerThread::run Lua: " + std::string(e.what()));
		}
	}

	END_DEBUG_EXCEPTION_HANDLER

	return NULL;
}

v3f ServerSoundParams::getPos(ServerEnvironment *env, bool *pos_exists) const
{
	if(pos_exists) *pos_exists = false;
	switch(type){
	case SSP_LOCAL:
		return v3f(0,0,0);
	case SSP_POSITIONAL:
		if(pos_exists) *pos_exists = true;
		return pos;
	case SSP_OBJECT: {
		if(object == 0)
			return v3f(0,0,0);
		ServerActiveObject *sao = env->getActiveObject(object);
		if(!sao)
			return v3f(0,0,0);
		if(pos_exists) *pos_exists = true;
		return sao->getBasePosition(); }
	}
	return v3f(0,0,0);
}



/*
	Server
*/

Server::Server(
		const std::string &path_world,
		const SubgameSpec &gamespec,
		bool simple_singleplayer_mode,
		bool ipv6,
		bool dedicated,
		ChatInterface *iface
	):
	m_path_world(path_world),
	m_gamespec(gamespec),
	m_simple_singleplayer_mode(simple_singleplayer_mode),
	m_dedicated(dedicated),
	m_async_fatal_error(""),
	m_env(NULL),
	m_con(PROTOCOL_ID,
			512,
			CONNECTION_TIMEOUT,
			ipv6,
			this),
	m_banmanager(NULL),
	m_rollback(NULL),
	m_enable_rollback_recording(false),
	m_emerge(NULL),
	m_script(NULL),
	m_itemdef(createItemDefManager()),
	m_nodedef(createNodeDefManager()),
	m_craftdef(createCraftDefManager()),
	m_event(new EventManager()),
	m_thread(NULL),
	m_time_of_day_send_timer(0),
	m_uptime(0),
	m_clients(&m_con),
	m_shutdown_requested(false),
	m_shutdown_ask_reconnect(false),
	m_shutdown_timer(0.0f),
	m_admin_chat(iface),
	m_ignore_map_edit_events(false),
	m_ignore_map_edit_events_peer_id(0),
	m_next_sound_id(0),
	m_mod_storage_save_timer(10.0f)
{
	m_liquid_transform_timer = 0.0;
	m_liquid_transform_every = 1.0;
	m_masterserver_timer = 0.0;
	m_emergethread_trigger_timer = 0.0;
	m_savemap_timer = 0.0;

	m_step_dtime = 0.0;
	m_lag = g_settings->getFloat("dedicated_server_step");

	if(path_world == "")
		throw ServerError("Supplied empty world path");

	if(!gamespec.isValid())
		throw ServerError("Supplied invalid gamespec");

	infostream<<"Server created for gameid \""<<m_gamespec.id<<"\"";
	if(m_simple_singleplayer_mode)
		infostream<<" in simple singleplayer mode"<<std::endl;
	else
		infostream<<std::endl;
	infostream<<"- world:  "<<m_path_world<<std::endl;
	infostream<<"- game:   "<<m_gamespec.path<<std::endl;

	// Create world if it doesn't exist
	if(!loadGameConfAndInitWorld(m_path_world, m_gamespec))
		throw ServerError("Failed to initialize world");

	// Create server thread
	m_thread = new ServerThread(this);

	// Create emerge manager
	m_emerge = new EmergeManager(this);

	// Create ban manager
	std::string ban_path = m_path_world + DIR_DELIM "ipban.txt";
	m_banmanager = new BanManager(ban_path);

	ServerModConfiguration modconf(m_path_world);
	m_mods = modconf.getMods();
	std::vector<ModSpec> unsatisfied_mods = modconf.getUnsatisfiedMods();
	// complain about mods with unsatisfied dependencies
	if (!modconf.isConsistent()) {
		modconf.printUnsatisfiedModsError();
	}

	//lock environment
	MutexAutoLock envlock(m_env_mutex);

	// Create the Map (loads map_meta.txt, overriding configured mapgen params)
	ServerMap *servermap = new ServerMap(path_world, this, m_emerge);

	// Initialize scripting
	infostream<<"Server: Initializing Lua"<<std::endl;

	m_script = new ServerScripting(this);

	m_script->loadMod(getBuiltinLuaPath() + DIR_DELIM "init.lua", BUILTIN_MOD_NAME);

	// Print mods
	infostream << "Server: Loading mods: ";
	for (std::vector<ModSpec>::const_iterator i = m_mods.begin();
			i != m_mods.end(); ++i) {
		infostream << (*i).name << " ";
	}
	infostream << std::endl;
	// Load and run "mod" scripts
	for (std::vector<ModSpec>::const_iterator it = m_mods.begin();
			it != m_mods.end(); ++it) {
		const ModSpec &mod = *it;
		if (!string_allowed(mod.name, MODNAME_ALLOWED_CHARS)) {
			throw ModError("Error loading mod \"" + mod.name +
				"\": Mod name does not follow naming conventions: "
				"Only characters [a-z0-9_] are allowed.");
		}
		std::string script_path = mod.path + DIR_DELIM + "init.lua";
		infostream << "  [" << padStringRight(mod.name, 12) << "] [\""
				<< script_path << "\"]" << std::endl;
		m_script->loadMod(script_path, mod.name);
	}

	// Read Textures and calculate sha1 sums
	fillMediaCache();

	// Apply item aliases in the node definition manager
	m_nodedef->updateAliases(m_itemdef);

	// Apply texture overrides from texturepack/override.txt
	std::string texture_path = g_settings->get("texture_path");
	if (texture_path != "" && fs::IsDir(texture_path))
		m_nodedef->applyTextureOverrides(texture_path + DIR_DELIM + "override.txt");

	m_nodedef->setNodeRegistrationStatus(true);

	// Perform pending node name resolutions
	m_nodedef->runNodeResolveCallbacks();

	// unmap node names for connected nodeboxes
	m_nodedef->mapNodeboxConnections();

	// init the recipe hashes to speed up crafting
	m_craftdef->initHashes(this);

	// Initialize Environment
	m_env = new ServerEnvironment(servermap, m_script, this, m_path_world);

	m_clients.setEnv(m_env);

	if (!servermap->settings_mgr.makeMapgenParams())
		FATAL_ERROR("Couldn't create any mapgen type");

	// Initialize mapgens
	m_emerge->initMapgens(servermap->getMapgenParams());

	m_enable_rollback_recording = g_settings->getBool("enable_rollback_recording");
	if (m_enable_rollback_recording) {
		// Create rollback manager
		m_rollback = new RollbackManager(m_path_world, this);
	}

	// Give environment reference to scripting api
	m_script->initializeEnvironment(m_env);

	// Register us to receive map edit events
	servermap->addEventReceiver(this);

	// If file exists, load environment metadata
	if (fs::PathExists(m_path_world + DIR_DELIM "env_meta.txt")) {
		infostream << "Server: Loading environment metadata" << std::endl;
		m_env->loadMeta();
	} else {
		m_env->loadDefaultMeta();
	}

	// Add some test ActiveBlockModifiers to environment
	add_legacy_abms(m_env, m_nodedef);

	m_liquid_transform_every = g_settings->getFloat("liquid_update");
	m_max_chatmessage_length = g_settings->getU16("chat_message_max_size");
}

Server::~Server()
{
	infostream << "Server destructing" << std::endl;

	// Send shutdown message
	SendChatMessage(PEER_ID_INEXISTENT, L"*** Server shutting down");

	{
		MutexAutoLock envlock(m_env_mutex);
		
		infostream << "Server: Saving players" << std::endl;
		m_env->saveLoadedPlayers();

		infostream << "Server: Kicking players" << std::endl;
		std::string kick_msg;
		bool reconnect = false;
		if (getShutdownRequested()) {
			reconnect = m_shutdown_ask_reconnect;
			kick_msg = m_shutdown_msg;
		}
		if (kick_msg == "") {
			kick_msg = g_settings->get("kick_msg_shutdown");
		}
		m_env->kickAllPlayers(SERVER_ACCESSDENIED_SHUTDOWN,
			kick_msg, reconnect);
	}

	// Do this before stopping the server in case mapgen callbacks need to access
	// server-controlled resources (like ModStorages). Also do them before
	// shutdown callbacks since they may modify state that is finalized in a
	// callback.
	m_emerge->stopThreads();

	{
		MutexAutoLock envlock(m_env_mutex);

		// Execute script shutdown hooks
		infostream << "Executing shutdown hooks" << std::endl;
		m_script->on_shutdown();

		infostream << "Server: Saving environment metadata" << std::endl;
		m_env->saveMeta();
	}

	// Stop threads
	stop();
	delete m_thread;

	// Delete things in the reverse order of creation
	delete m_emerge;
	delete m_env;
	delete m_rollback;
	delete m_banmanager;
	delete m_event;
	delete m_itemdef;
	delete m_nodedef;
	delete m_craftdef;

	// Deinitialize scripting
	infostream << "Server: Deinitializing scripting" << std::endl;
	delete m_script;

	// Delete detached inventories
	for (std::map<std::string, Inventory*>::iterator
			i = m_detached_inventories.begin();
			i != m_detached_inventories.end(); ++i) {
		delete i->second;
	}
}

void Server::start(Address bind_addr)
{
	DSTACK(FUNCTION_NAME);

	m_bind_addr = bind_addr;

	infostream<<"Starting server on "
			<< bind_addr.serializeString() <<"..."<<std::endl;

	// Stop thread if already running
	m_thread->stop();

	// Initialize connection
	m_con.SetTimeoutMs(30);
	m_con.Serve(bind_addr);

	// Start thread
	m_thread->start();

	// ASCII art for the win!
	actionstream
	<<"        .__               __                   __   "<<std::endl
	<<"  _____ |__| ____   _____/  |_  ____   _______/  |_ "<<std::endl
	<<" /     \\|  |/    \\_/ __ \\   __\\/ __ \\ /  ___/\\   __\\"<<std::endl
	<<"|  Y Y  \\  |   |  \\  ___/|  | \\  ___/ \\___ \\  |  |  "<<std::endl
	<<"|__|_|  /__|___|  /\\___  >__|  \\___  >____  > |__|  "<<std::endl
	<<"      \\/        \\/     \\/          \\/     \\/        "<<std::endl;
	actionstream<<"World at ["<<m_path_world<<"]"<<std::endl;
	actionstream<<"Server for gameid=\""<<m_gamespec.id
			<<"\" listening on "<<bind_addr.serializeString()<<":"
			<<bind_addr.getPort() << "."<<std::endl;
}

void Server::stop()
{
	DSTACK(FUNCTION_NAME);

	infostream<<"Server: Stopping and waiting threads"<<std::endl;

	// Stop threads (set run=false first so both start stopping)
	m_thread->stop();
	//m_emergethread.setRun(false);
	m_thread->wait();
	//m_emergethread.stop();

	infostream<<"Server: Threads stopped"<<std::endl;
}

void Server::step(float dtime)
{
	DSTACK(FUNCTION_NAME);
	// Limit a bit
	if (dtime > 2.0)
		dtime = 2.0;
	{
		MutexAutoLock lock(m_step_dtime_mutex);
		m_step_dtime += dtime;
	}
	// Throw if fatal error occurred in thread
	std::string async_err = m_async_fatal_error.get();
	if (!async_err.empty()) {
		if (!m_simple_singleplayer_mode) {
			m_env->kickAllPlayers(SERVER_ACCESSDENIED_CRASH,
				g_settings->get("kick_msg_crash"),
				g_settings->getBool("ask_reconnect_on_crash"));
		}
		throw ServerError("AsyncErr: " + async_err);
	}
}

void Server::AsyncRunStep(bool initial_step)
{
	DSTACK(FUNCTION_NAME);

	g_profiler->add("Server::AsyncRunStep (num)", 1);

	float dtime;
	{
		MutexAutoLock lock1(m_step_dtime_mutex);
		dtime = m_step_dtime;
	}

	{
		// Send blocks to clients
		SendBlocks(dtime);
	}

	if((dtime < 0.001) && (initial_step == false))
		return;

	g_profiler->add("Server::AsyncRunStep with dtime (num)", 1);

	//infostream<<"Server steps "<<dtime<<std::endl;
	//infostream<<"Server::AsyncRunStep(): dtime="<<dtime<<std::endl;

	{
		MutexAutoLock lock1(m_step_dtime_mutex);
		m_step_dtime -= dtime;
	}

	/*
		Update uptime
	*/
	{
		m_uptime.set(m_uptime.get() + dtime);
	}

	handlePeerChanges();

	/*
		Update time of day and overall game time
	*/
	m_env->setTimeOfDaySpeed(g_settings->getFloat("time_speed"));

	/*
		Send to clients at constant intervals
	*/

	m_time_of_day_send_timer -= dtime;
	if(m_time_of_day_send_timer < 0.0) {
		m_time_of_day_send_timer = g_settings->getFloat("time_send_interval");
		u16 time = m_env->getTimeOfDay();
		float time_speed = g_settings->getFloat("time_speed");
		SendTimeOfDay(PEER_ID_INEXISTENT, time, time_speed);
	}

	{
		MutexAutoLock lock(m_env_mutex);
		// Figure out and report maximum lag to environment
		float max_lag = m_env->getMaxLagEstimate();
		max_lag *= 0.9998; // Decrease slowly (about half per 5 minutes)
		if(dtime > max_lag){
			if(dtime > 0.1 && dtime > max_lag * 2.0)
				infostream<<"Server: Maximum lag peaked to "<<dtime
						<<" s"<<std::endl;
			max_lag = dtime;
		}
		m_env->reportMaxLagEstimate(max_lag);
		// Step environment
		ScopeProfiler sp(g_profiler, "SEnv step");
		ScopeProfiler sp2(g_profiler, "SEnv step avg", SPT_AVG);
		m_env->step(dtime);
	}

	static const float map_timer_and_unload_dtime = 2.92;
	if(m_map_timer_and_unload_interval.step(dtime, map_timer_and_unload_dtime))
	{
		MutexAutoLock lock(m_env_mutex);
		// Run Map's timers and unload unused data
		ScopeProfiler sp(g_profiler, "Server: map timer and unload");
		m_env->getMap().timerUpdate(map_timer_and_unload_dtime,
			g_settings->getFloat("server_unload_unused_data_timeout"),
			U32_MAX);
	}

	/*
		Listen to the admin chat, if available
	*/
	if (m_admin_chat) {
		if (!m_admin_chat->command_queue.empty()) {
			MutexAutoLock lock(m_env_mutex);
			while (!m_admin_chat->command_queue.empty()) {
				ChatEvent *evt = m_admin_chat->command_queue.pop_frontNoEx();
				handleChatInterfaceEvent(evt);
				delete evt;
			}
		}
		m_admin_chat->outgoing_queue.push_back(
			new ChatEventTimeInfo(m_env->getGameTime(), m_env->getTimeOfDay()));
	}

	/*
		Do background stuff
	*/

	/* Transform liquids */
	m_liquid_transform_timer += dtime;
	if(m_liquid_transform_timer >= m_liquid_transform_every)
	{
		m_liquid_transform_timer -= m_liquid_transform_every;

		MutexAutoLock lock(m_env_mutex);

		ScopeProfiler sp(g_profiler, "Server: liquid transform");

		std::map<v3s16, MapBlock*> modified_blocks;
		m_env->getMap().transformLiquids(modified_blocks, m_env);
#if 0
		/*
			Update lighting
		*/
		core::map<v3s16, MapBlock*> lighting_modified_blocks;
		ServerMap &map = ((ServerMap&)m_env->getMap());
		map.updateLighting(modified_blocks, lighting_modified_blocks);

		// Add blocks modified by lighting to modified_blocks
		for(core::map<v3s16, MapBlock*>::Iterator
				i = lighting_modified_blocks.getIterator();
				i.atEnd() == false; i++)
		{
			MapBlock *block = i.getNode()->getValue();
			modified_blocks.insert(block->getPos(), block);
		}
#endif
		/*
			Set the modified blocks unsent for all the clients
		*/
		if(!modified_blocks.empty())
		{
			SetBlocksNotSent(modified_blocks);
		}
	}
	m_clients.step(dtime);

	m_lag += (m_lag > dtime ? -1 : 1) * dtime/100;
#if USE_CURL
	// send masterserver announce
	{
		float &counter = m_masterserver_timer;
		if (!isSingleplayer() && (!counter || counter >= 300.0) &&
				g_settings->getBool("server_announce")) {
			ServerList::sendAnnounce(counter ? ServerList::AA_UPDATE :
						ServerList::AA_START,
					m_bind_addr.getPort(),
					m_clients.getPlayerNames(),
					m_uptime.get(),
					m_env->getGameTime(),
					m_lag,
					m_gamespec.id,
					Mapgen::getMapgenName(m_emerge->mgparams->mgtype),
					m_mods,
					m_dedicated);
			counter = 0.01;
		}
		counter += dtime;
	}
#endif

	/*
		Check added and deleted active objects
	*/
	{
		//infostream<<"Server: Checking added and deleted active objects"<<std::endl;
		MutexAutoLock envlock(m_env_mutex);

		m_clients.lock();
		UNORDERED_MAP<u16, RemoteClient*> clients = m_clients.getClientList();
		ScopeProfiler sp(g_profiler, "Server: checking added and deleted objs");

		// Radius inside which objects are active
		static const s16 radius =
			g_settings->getS16("active_object_send_range_blocks") * MAP_BLOCKSIZE;

		// Radius inside which players are active
		static const bool is_transfer_limited =
			g_settings->exists("unlimited_player_transfer_distance") &&
			!g_settings->getBool("unlimited_player_transfer_distance");
		static const s16 player_transfer_dist = g_settings->getS16("player_transfer_distance") * MAP_BLOCKSIZE;
		s16 player_radius = player_transfer_dist;
		if (player_radius == 0 && is_transfer_limited)
			player_radius = radius;

		for (UNORDERED_MAP<u16, RemoteClient*>::iterator i = clients.begin();
			i != clients.end(); ++i) {
			RemoteClient *client = i->second;

			// If definitions and textures have not been sent, don't
			// send objects either
			if (client->getState() < CS_DefinitionsSent)
				continue;

			RemotePlayer *player = m_env->getPlayer(client->peer_id);
			if (player == NULL) {
				// This can happen if the client timeouts somehow
				/*warningstream<<FUNCTION_NAME<<": Client "
						<<client->peer_id
						<<" has no associated player"<<std::endl;*/
				continue;
			}

			PlayerSAO *playersao = player->getPlayerSAO();
			if (playersao == NULL)
				continue;

			s16 my_radius = MYMIN(radius, playersao->getWantedRange() * MAP_BLOCKSIZE);
			if (my_radius <= 0) my_radius = radius;
			//infostream << "Server: Active Radius " << my_radius << std::endl;

			std::queue<u16> removed_objects;
			std::queue<u16> added_objects;
			m_env->getRemovedActiveObjects(playersao, my_radius, player_radius,
					client->m_known_objects, removed_objects);
			m_env->getAddedActiveObjects(playersao, my_radius, player_radius,
					client->m_known_objects, added_objects);

			// Ignore if nothing happened
			if (removed_objects.empty() && added_objects.empty()) {
				continue;
			}

			std::string data_buffer;

			char buf[4];

			// Handle removed objects
			writeU16((u8*)buf, removed_objects.size());
			data_buffer.append(buf, 2);
			while (!removed_objects.empty()) {
				// Get object
				u16 id = removed_objects.front();
				ServerActiveObject* obj = m_env->getActiveObject(id);

				// Add to data buffer for sending
				writeU16((u8*)buf, id);
				data_buffer.append(buf, 2);

				// Remove from known objects
				client->m_known_objects.erase(id);

				if(obj && obj->m_known_by_count > 0)
					obj->m_known_by_count--;
				removed_objects.pop();
			}

			// Handle added objects
			writeU16((u8*)buf, added_objects.size());
			data_buffer.append(buf, 2);
			while (!added_objects.empty()) {
				// Get object
				u16 id = added_objects.front();
				ServerActiveObject* obj = m_env->getActiveObject(id);

				// Get object type
				u8 type = ACTIVEOBJECT_TYPE_INVALID;
				if(obj == NULL)
					warningstream<<FUNCTION_NAME
							<<": NULL object"<<std::endl;
				else
					type = obj->getSendType();

				// Add to data buffer for sending
				writeU16((u8*)buf, id);
				data_buffer.append(buf, 2);
				writeU8((u8*)buf, type);
				data_buffer.append(buf, 1);

				if(obj)
					data_buffer.append(serializeLongString(
							obj->getClientInitializationData(client->net_proto_version)));
				else
					data_buffer.append(serializeLongString(""));

				// Add to known objects
				client->m_known_objects.insert(id);

				if(obj)
					obj->m_known_by_count++;

				added_objects.pop();
			}

			u32 pktSize = SendActiveObjectRemoveAdd(client->peer_id, data_buffer);
			verbosestream << "Server: Sent object remove/add: "
					<< removed_objects.size() << " removed, "
					<< added_objects.size() << " added, "
					<< "packet size is " << pktSize << std::endl;
		}
		m_clients.unlock();

		m_mod_storage_save_timer -= dtime;
		if (m_mod_storage_save_timer <= 0.0f) {
			infostream << "Saving registered mod storages." << std::endl;
			m_mod_storage_save_timer = g_settings->getFloat("server_map_save_interval");
			for (UNORDERED_MAP<std::string, ModMetadata *>::const_iterator
				it = m_mod_storages.begin(); it != m_mod_storages.end(); ++it) {
				if (it->second->isModified()) {
					it->second->save(getModStoragePath());
				}
			}
		}
	}

	/*
		Send object messages
	*/
	{
		MutexAutoLock envlock(m_env_mutex);
		ScopeProfiler sp(g_profiler, "Server: sending object messages");

		// Key = object id
		// Value = data sent by object
		UNORDERED_MAP<u16, std::vector<ActiveObjectMessage>* > buffered_messages;

		// Get active object messages from environment
		for(;;) {
			ActiveObjectMessage aom = m_env->getActiveObjectMessage();
			if (aom.id == 0)
				break;

			std::vector<ActiveObjectMessage>* message_list = NULL;
			UNORDERED_MAP<u16, std::vector<ActiveObjectMessage>* >::iterator n;
			n = buffered_messages.find(aom.id);
			if (n == buffered_messages.end()) {
				message_list = new std::vector<ActiveObjectMessage>;
				buffered_messages[aom.id] = message_list;
			}
			else {
				message_list = n->second;
			}
			message_list->push_back(aom);
		}

		m_clients.lock();
		UNORDERED_MAP<u16, RemoteClient*> clients = m_clients.getClientList();
		// Route data to every client
		for (UNORDERED_MAP<u16, RemoteClient*>::iterator i = clients.begin();
			i != clients.end(); ++i) {
			RemoteClient *client = i->second;
			std::string reliable_data;
			std::string unreliable_data;
			// Go through all objects in message buffer
			for (UNORDERED_MAP<u16, std::vector<ActiveObjectMessage>* >::iterator
					j = buffered_messages.begin();
					j != buffered_messages.end(); ++j) {
				// If object is not known by client, skip it
				u16 id = j->first;
				if (client->m_known_objects.find(id) == client->m_known_objects.end())
					continue;

				// Get message list of object
				std::vector<ActiveObjectMessage>* list = j->second;
				// Go through every message
				for (std::vector<ActiveObjectMessage>::iterator
						k = list->begin(); k != list->end(); ++k) {
					// Compose the full new data with header
					ActiveObjectMessage aom = *k;
					std::string new_data;
					// Add object id
					char buf[2];
					writeU16((u8*)&buf[0], aom.id);
					new_data.append(buf, 2);
					// Add data
					new_data += serializeString(aom.datastring);
					// Add data to buffer
					if(aom.reliable)
						reliable_data += new_data;
					else
						unreliable_data += new_data;
				}
			}
			/*
				reliable_data and unreliable_data are now ready.
				Send them.
			*/
			if(reliable_data.size() > 0) {
				SendActiveObjectMessages(client->peer_id, reliable_data);
			}

			if(unreliable_data.size() > 0) {
				SendActiveObjectMessages(client->peer_id, unreliable_data, false);
			}
		}
		m_clients.unlock();

		// Clear buffered_messages
		for (UNORDERED_MAP<u16, std::vector<ActiveObjectMessage>* >::iterator
				i = buffered_messages.begin();
				i != buffered_messages.end(); ++i) {
			delete i->second;
		}
	}

	/*
		Send queued-for-sending map edit events.
	*/
	{
		// We will be accessing the environment
		MutexAutoLock lock(m_env_mutex);

		// Don't send too many at a time
		//u32 count = 0;

		// Single change sending is disabled if queue size is not small
		bool disable_single_change_sending = false;
		if(m_unsent_map_edit_queue.size() >= 4)
			disable_single_change_sending = true;

		int event_count = m_unsent_map_edit_queue.size();

		// We'll log the amount of each
		Profiler prof;

		while(m_unsent_map_edit_queue.size() != 0)
		{
			MapEditEvent* event = m_unsent_map_edit_queue.front();
			m_unsent_map_edit_queue.pop();

			// Players far away from the change are stored here.
			// Instead of sending the changes, MapBlocks are set not sent
			// for them.
			std::vector<u16> far_players;

			switch (event->type) {
			case MEET_ADDNODE:
			case MEET_SWAPNODE:
				prof.add("MEET_ADDNODE", 1);
				sendAddNode(event->p, event->n, event->already_known_by_peer,
						&far_players, disable_single_change_sending ? 5 : 30,
						event->type == MEET_ADDNODE);
				break;
			case MEET_REMOVENODE:
				prof.add("MEET_REMOVENODE", 1);
				sendRemoveNode(event->p, event->already_known_by_peer,
						&far_players, disable_single_change_sending ? 5 : 30);
				break;
			case MEET_BLOCK_NODE_METADATA_CHANGED:
				infostream << "Server: MEET_BLOCK_NODE_METADATA_CHANGED" << std::endl;
						prof.add("MEET_BLOCK_NODE_METADATA_CHANGED", 1);
						setBlockNotSent(event->p);
				break;
			case MEET_OTHER:
				infostream << "Server: MEET_OTHER" << std::endl;
				prof.add("MEET_OTHER", 1);
				for(std::set<v3s16>::iterator
						i = event->modified_blocks.begin();
						i != event->modified_blocks.end(); ++i) {
					setBlockNotSent(*i);
				}
				break;
			default:
				prof.add("unknown", 1);
				warningstream << "Server: Unknown MapEditEvent "
						<< ((u32)event->type) << std::endl;
				break;
			}

			/*
				Set blocks not sent to far players
			*/
			if(!far_players.empty()) {
				// Convert list format to that wanted by SetBlocksNotSent
				std::map<v3s16, MapBlock*> modified_blocks2;
				for(std::set<v3s16>::iterator
						i = event->modified_blocks.begin();
						i != event->modified_blocks.end(); ++i) {
					modified_blocks2[*i] =
							m_env->getMap().getBlockNoCreateNoEx(*i);
				}

				// Set blocks not sent
				for(std::vector<u16>::iterator
						i = far_players.begin();
						i != far_players.end(); ++i) {
					if(RemoteClient *client = getClient(*i))
						client->SetBlocksNotSent(modified_blocks2);
				}
			}

			delete event;

			/*// Don't send too many at a time
			count++;
			if(count >= 1 && m_unsent_map_edit_queue.size() < 100)
				break;*/
		}

		if(event_count >= 5){
			infostream<<"Server: MapEditEvents:"<<std::endl;
			prof.print(infostream);
		} else if(event_count != 0){
			verbosestream<<"Server: MapEditEvents:"<<std::endl;
			prof.print(verbosestream);
		}

	}

	/*
		Trigger emergethread (it somehow gets to a non-triggered but
		bysy state sometimes)
	*/
	{
		float &counter = m_emergethread_trigger_timer;
		counter += dtime;
		if (counter >= 2.0) {
			counter = 0.0;

			m_emerge->startThreads();
		}
	}

	// Save map, players and auth stuff
	{
		float &counter = m_savemap_timer;
		counter += dtime;
		static const float save_interval =
			g_settings->getFloat("server_map_save_interval");
		if (counter >= save_interval) {
			counter = 0.0;
			MutexAutoLock lock(m_env_mutex);

			ScopeProfiler sp(g_profiler, "Server: saving stuff");

			// Save ban file
			if (m_banmanager->isModified()) {
				m_banmanager->save();
			}

			// Save changed parts of map
			m_env->getMap().save(MOD_STATE_WRITE_NEEDED);

			// Save players
			m_env->saveLoadedPlayers();

			// Save environment metadata
			m_env->saveMeta();
		}
	}

	// Timed shutdown
	static const float shutdown_msg_times[] =
	{
		1, 2, 3, 4, 5, 10, 15, 20, 25, 30, 45, 60, 120, 180, 300, 600, 1200, 1800, 3600
	};

	if (m_shutdown_timer > 0.0f) {
		// Automated messages
		if (m_shutdown_timer < shutdown_msg_times[ARRLEN(shutdown_msg_times) - 1]) {
			for (u16 i = 0; i < ARRLEN(shutdown_msg_times) - 1; i++) {
				// If shutdown timer matches an automessage, shot it
				if (m_shutdown_timer > shutdown_msg_times[i] &&
					m_shutdown_timer - dtime < shutdown_msg_times[i]) {
					std::wstringstream ws;

					ws << L"*** Server shutting down in "
						<< duration_to_string(myround(m_shutdown_timer - dtime)).c_str()
						<< ".";

					infostream << wide_to_utf8(ws.str()).c_str() << std::endl;
					SendChatMessage(PEER_ID_INEXISTENT, ws.str());
					break;
				}
			}
		}

		m_shutdown_timer -= dtime;
		if (m_shutdown_timer < 0.0f) {
			m_shutdown_timer = 0.0f;
			m_shutdown_requested = true;
		}
	}
}

void Server::Receive()
{
	DSTACK(FUNCTION_NAME);
	SharedBuffer<u8> data;
	u16 peer_id;
	try {
		NetworkPacket pkt;
		m_con.Receive(&pkt);
		peer_id = pkt.getPeerId();
		ProcessData(&pkt);
	}
	catch(con::InvalidIncomingDataException &e) {
		infostream<<"Server::Receive(): "
				"InvalidIncomingDataException: what()="
				<<e.what()<<std::endl;
	}
	catch(SerializationError &e) {
		infostream<<"Server::Receive(): "
				"SerializationError: what()="
				<<e.what()<<std::endl;
	}
	catch(ClientStateError &e) {
		errorstream << "ProcessData: peer=" << peer_id  << e.what() << std::endl;
		DenyAccess_Legacy(peer_id, L"Your client sent something server didn't expect."
				L"Try reconnecting or updating your client");
	}
	catch(con::PeerNotFoundException &e) {
		// Do nothing
	}
}

PlayerSAO* Server::StageTwoClientInit(u16 peer_id)
{
	std::string playername = "";
	PlayerSAO *playersao = NULL;
	m_clients.lock();
	try {
		RemoteClient* client = m_clients.lockedGetClientNoEx(peer_id, CS_InitDone);
		if (client != NULL) {
			playername = client->getName();
			playersao = emergePlayer(playername.c_str(), peer_id, client->net_proto_version);
		}
	} catch (std::exception &e) {
		m_clients.unlock();
		throw;
	}
	m_clients.unlock();

	RemotePlayer *player = m_env->getPlayer(playername.c_str());

	// If failed, cancel
	if ((playersao == NULL) || (player == NULL)) {
		if (player && player->peer_id != 0) {
			actionstream << "Server: Failed to emerge player \"" << playername
					<< "\" (player allocated to an another client)" << std::endl;
			DenyAccess_Legacy(peer_id, L"Another client is connected with this "
					L"name. If your client closed unexpectedly, try again in "
					L"a minute.");
		} else {
			errorstream << "Server: " << playername << ": Failed to emerge player"
					<< std::endl;
			DenyAccess_Legacy(peer_id, L"Could not allocate player.");
		}
		return NULL;
	}

	/*
		Send complete position information
	*/
	SendMovePlayer(peer_id);

	// Send privileges
	SendPlayerPrivileges(peer_id);

	// Send inventory formspec
	SendPlayerInventoryFormspec(peer_id);

	// Send inventory
	SendInventory(playersao);

	// Send HP or death screen
	if (playersao->isDead())
		SendDeathscreen(peer_id, false, v3f(0,0,0));
	else
		SendPlayerHPOrDie(playersao);

	// Send Breath
	SendPlayerBreath(playersao);

	// Note things in chat if not in simple singleplayer mode
	if (!m_simple_singleplayer_mode && g_settings->getBool("show_statusline_on_connect")) {
		// Send information about server to player in chat
		SendChatMessage(peer_id, getStatusString());
	}
	Address addr = getPeerAddress(player->peer_id);
	std::string ip_str = addr.serializeString();
	actionstream<<player->getName() <<" [" << ip_str << "] joins game. " << std::endl;
	/*
		Print out action
	*/
	{
		const std::vector<std::string> &names = m_clients.getPlayerNames();

		actionstream << player->getName() << " joins game. List of players: ";

		for (std::vector<std::string>::const_iterator i = names.begin();
				i != names.end(); ++i) {
			actionstream << *i << " ";
		}

		actionstream << player->getName() <<std::endl;
	}
	return playersao;
}

inline void Server::handleCommand(NetworkPacket* pkt)
{
	const ToServerCommandHandler& opHandle = toServerCommandTable[pkt->getCommand()];
	(this->*opHandle.handler)(pkt);
}

void Server::ProcessData(NetworkPacket *pkt)
{
	DSTACK(FUNCTION_NAME);
	// Environment is locked first.
	MutexAutoLock envlock(m_env_mutex);

	ScopeProfiler sp(g_profiler, "Server::ProcessData");
	u32 peer_id = pkt->getPeerId();

	try {
		Address address = getPeerAddress(peer_id);
		std::string addr_s = address.serializeString();

		if(m_banmanager->isIpBanned(addr_s)) {
			std::string ban_name = m_banmanager->getBanName(addr_s);
			infostream << "Server: A banned client tried to connect from "
					<< addr_s << "; banned name was "
					<< ban_name << std::endl;
			// This actually doesn't seem to transfer to the client
			DenyAccess_Legacy(peer_id, L"Your ip is banned. Banned name was "
					+ utf8_to_wide(ban_name));
			return;
		}
	}
	catch(con::PeerNotFoundException &e) {
		/*
		 * no peer for this packet found
		 * most common reason is peer timeout, e.g. peer didn't
		 * respond for some time, your server was overloaded or
		 * things like that.
		 */
		infostream << "Server::ProcessData(): Canceling: peer "
				<< peer_id << " not found" << std::endl;
		return;
	}

	try {
		ToServerCommand command = (ToServerCommand) pkt->getCommand();

		// Command must be handled into ToServerCommandHandler
		if (command >= TOSERVER_NUM_MSG_TYPES) {
			infostream << "Server: Ignoring unknown command "
					 << command << std::endl;
			return;
		}

		if (toServerCommandTable[command].state == TOSERVER_STATE_NOT_CONNECTED) {
			handleCommand(pkt);
			return;
		}

		u8 peer_ser_ver = getClient(peer_id, CS_InitDone)->serialization_version;

		if(peer_ser_ver == SER_FMT_VER_INVALID) {
			errorstream << "Server::ProcessData(): Cancelling: Peer"
					" serialization format invalid or not initialized."
					" Skipping incoming command=" << command << std::endl;
			return;
		}

		/* Handle commands related to client startup */
		if (toServerCommandTable[command].state == TOSERVER_STATE_STARTUP) {
			handleCommand(pkt);
			return;
		}

		if (m_clients.getClientState(peer_id) < CS_Active) {
			if (command == TOSERVER_PLAYERPOS) return;

			errorstream << "Got packet command: " << command << " for peer id "
					<< peer_id << " but client isn't active yet. Dropping packet "
					<< std::endl;
			return;
		}

		handleCommand(pkt);
	} catch (SendFailedException &e) {
		errorstream << "Server::ProcessData(): SendFailedException: "
				<< "what=" << e.what()
				<< std::endl;
	} catch (PacketError &e) {
		actionstream << "Server::ProcessData(): PacketError: "
				<< "what=" << e.what()
				<< std::endl;
	}
}

void Server::setTimeOfDay(u32 time)
{
	m_env->setTimeOfDay(time);
	m_time_of_day_send_timer = 0;
}

void Server::onMapEditEvent(MapEditEvent *event)
{
	if(m_ignore_map_edit_events)
		return;
	if(m_ignore_map_edit_events_area.contains(event->getArea()))
		return;
	MapEditEvent *e = event->clone();
	m_unsent_map_edit_queue.push(e);
}

Inventory* Server::getInventory(const InventoryLocation &loc)
{
	switch (loc.type) {
	case InventoryLocation::UNDEFINED:
	case InventoryLocation::CURRENT_PLAYER:
		break;
	case InventoryLocation::PLAYER:
	{
		RemotePlayer *player = dynamic_cast<RemotePlayer *>(m_env->getPlayer(loc.name.c_str()));
		if(!player)
			return NULL;
		PlayerSAO *playersao = player->getPlayerSAO();
		if(!playersao)
			return NULL;
		return playersao->getInventory();
	}
		break;
	case InventoryLocation::NODEMETA:
	{
		NodeMetadata *meta = m_env->getMap().getNodeMetadata(loc.p);
		if(!meta)
			return NULL;
		return meta->getInventory();
	}
		break;
	case InventoryLocation::DETACHED:
	{
		if(m_detached_inventories.count(loc.name) == 0)
			return NULL;
		return m_detached_inventories[loc.name];
	}
		break;
	default:
		sanity_check(false); // abort
		break;
	}
	return NULL;
}
void Server::setInventoryModified(const InventoryLocation &loc, bool playerSend)
{
	switch(loc.type){
	case InventoryLocation::UNDEFINED:
		break;
	case InventoryLocation::PLAYER:
	{
		if (!playerSend)
			return;

		RemotePlayer *player =
			dynamic_cast<RemotePlayer *>(m_env->getPlayer(loc.name.c_str()));

		if (!player)
			return;

		PlayerSAO *playersao = player->getPlayerSAO();
		if(!playersao)
			return;

		SendInventory(playersao);
	}
		break;
	case InventoryLocation::NODEMETA:
	{
		v3s16 blockpos = getNodeBlockPos(loc.p);

		MapBlock *block = m_env->getMap().getBlockNoCreateNoEx(blockpos);
		if(block)
			block->raiseModified(MOD_STATE_WRITE_NEEDED);

		setBlockNotSent(blockpos);
	}
		break;
	case InventoryLocation::DETACHED:
	{
		sendDetachedInventory(loc.name,PEER_ID_INEXISTENT);
	}
		break;
	default:
		sanity_check(false); // abort
		break;
	}
}

void Server::SetBlocksNotSent(std::map<v3s16, MapBlock *>& block)
{
	std::vector<u16> clients = m_clients.getClientIDs();
	m_clients.lock();
	// Set the modified blocks unsent for all the clients
	for (std::vector<u16>::iterator i = clients.begin();
		 i != clients.end(); ++i) {
			if (RemoteClient *client = m_clients.lockedGetClientNoEx(*i))
				client->SetBlocksNotSent(block);
	}
	m_clients.unlock();
}

void Server::peerAdded(con::Peer *peer)
{
	DSTACK(FUNCTION_NAME);
	verbosestream<<"Server::peerAdded(): peer->id="
			<<peer->id<<std::endl;

	con::PeerChange c;
	c.type = con::PEER_ADDED;
	c.peer_id = peer->id;
	c.timeout = false;
	m_peer_change_queue.push(c);
}

void Server::deletingPeer(con::Peer *peer, bool timeout)
{
	DSTACK(FUNCTION_NAME);
	verbosestream<<"Server::deletingPeer(): peer->id="
			<<peer->id<<", timeout="<<timeout<<std::endl;

	m_clients.event(peer->id, CSE_Disconnect);
	con::PeerChange c;
	c.type = con::PEER_REMOVED;
	c.peer_id = peer->id;
	c.timeout = timeout;
	m_peer_change_queue.push(c);
}

bool Server::getClientConInfo(u16 peer_id, con::rtt_stat_type type, float* retval)
{
	*retval = m_con.getPeerStat(peer_id,type);
	if (*retval == -1) return false;
	return true;
}

bool Server::getClientInfo(
		u16          peer_id,
		ClientState* state,
		u32*         uptime,
		u8*          ser_vers,
		u16*         prot_vers,
		u8*          major,
		u8*          minor,
		u8*          patch,
		std::string* vers_string
	)
{
	*state = m_clients.getClientState(peer_id);
	m_clients.lock();
	RemoteClient* client = m_clients.lockedGetClientNoEx(peer_id, CS_Invalid);

	if (client == NULL) {
		m_clients.unlock();
		return false;
	}

	*uptime = client->uptime();
	*ser_vers = client->serialization_version;
	*prot_vers = client->net_proto_version;

	*major = client->getMajor();
	*minor = client->getMinor();
	*patch = client->getPatch();
	*vers_string = client->getPatch();

	m_clients.unlock();

	return true;
}

void Server::handlePeerChanges()
{
	while(m_peer_change_queue.size() > 0)
	{
		con::PeerChange c = m_peer_change_queue.front();
		m_peer_change_queue.pop();

		verbosestream<<"Server: Handling peer change: "
				<<"id="<<c.peer_id<<", timeout="<<c.timeout
				<<std::endl;

		switch(c.type)
		{
		case con::PEER_ADDED:
			m_clients.CreateClient(c.peer_id);
			break;

		case con::PEER_REMOVED:
			DeleteClient(c.peer_id, c.timeout?CDR_TIMEOUT:CDR_LEAVE);
			break;

		default:
			FATAL_ERROR("Invalid peer change event received!");
			break;
		}
	}
}

void Server::printToConsoleOnly(const std::string &text)
{
	if (m_admin_chat) {
		m_admin_chat->outgoing_queue.push_back(
			new ChatEventChat("", utf8_to_wide(text)));
	} else {
		std::cout << text << std::endl;
	}
}

void Server::Send(NetworkPacket* pkt)
{
	m_clients.send(pkt->getPeerId(),
		clientCommandFactoryTable[pkt->getCommand()].channel,
		pkt,
		clientCommandFactoryTable[pkt->getCommand()].reliable);
}

void Server::SendMovement(u16 peer_id)
{
	DSTACK(FUNCTION_NAME);
	std::ostringstream os(std::ios_base::binary);

	NetworkPacket pkt(TOCLIENT_MOVEMENT, 12 * sizeof(float), peer_id);

	pkt << g_settings->getFloat("movement_acceleration_default");
	pkt << g_settings->getFloat("movement_acceleration_air");
	pkt << g_settings->getFloat("movement_acceleration_fast");
	pkt << g_settings->getFloat("movement_speed_walk");
	pkt << g_settings->getFloat("movement_speed_crouch");
	pkt << g_settings->getFloat("movement_speed_fast");
	pkt << g_settings->getFloat("movement_speed_climb");
	pkt << g_settings->getFloat("movement_speed_jump");
	pkt << g_settings->getFloat("movement_liquid_fluidity");
	pkt << g_settings->getFloat("movement_liquid_fluidity_smooth");
	pkt << g_settings->getFloat("movement_liquid_sink");
	pkt << g_settings->getFloat("movement_gravity");

	Send(&pkt);
}

void Server::SendPlayerHPOrDie(PlayerSAO *playersao)
{
	if (!g_settings->getBool("enable_damage"))
		return;

	u16 peer_id   = playersao->getPeerID();
	bool is_alive = playersao->getHP() > 0;

	if (is_alive)
		SendPlayerHP(peer_id);
	else
		DiePlayer(peer_id);
}

void Server::SendHP(u16 peer_id, u8 hp)
{
	DSTACK(FUNCTION_NAME);

	NetworkPacket pkt(TOCLIENT_HP, 1, peer_id);
	pkt << hp;
	Send(&pkt);
}

void Server::SendBreath(u16 peer_id, u16 breath)
{
	DSTACK(FUNCTION_NAME);

	NetworkPacket pkt(TOCLIENT_BREATH, 2, peer_id);
	pkt << (u16) breath;
	Send(&pkt);
}

void Server::SendAccessDenied(u16 peer_id, AccessDeniedCode reason,
		const std::string &custom_reason, bool reconnect)
{
	assert(reason < SERVER_ACCESSDENIED_MAX);

	NetworkPacket pkt(TOCLIENT_ACCESS_DENIED, 1, peer_id);
	pkt << (u8)reason;
	if (reason == SERVER_ACCESSDENIED_CUSTOM_STRING)
		pkt << custom_reason;
	else if (reason == SERVER_ACCESSDENIED_SHUTDOWN ||
			reason == SERVER_ACCESSDENIED_CRASH)
		pkt << custom_reason << (u8)reconnect;
	Send(&pkt);
}

void Server::SendAccessDenied_Legacy(u16 peer_id,const std::wstring &reason)
{
	DSTACK(FUNCTION_NAME);

	NetworkPacket pkt(TOCLIENT_ACCESS_DENIED_LEGACY, 0, peer_id);
	pkt << reason;
	Send(&pkt);
}

void Server::SendDeathscreen(u16 peer_id,bool set_camera_point_target,
		v3f camera_point_target)
{
	DSTACK(FUNCTION_NAME);

	NetworkPacket pkt(TOCLIENT_DEATHSCREEN, 1 + sizeof(v3f), peer_id);
	pkt << set_camera_point_target << camera_point_target;
	Send(&pkt);
}

void Server::SendItemDef(u16 peer_id,
		IItemDefManager *itemdef, u16 protocol_version)
{
	DSTACK(FUNCTION_NAME);

	NetworkPacket pkt(TOCLIENT_ITEMDEF, 0, peer_id);

	/*
		u16 command
		u32 length of the next item
		zlib-compressed serialized ItemDefManager
	*/
	std::ostringstream tmp_os(std::ios::binary);
	itemdef->serialize(tmp_os, protocol_version);
	std::ostringstream tmp_os2(std::ios::binary);
	compressZlib(tmp_os.str(), tmp_os2);
	pkt.putLongString(tmp_os2.str());

	// Make data buffer
	verbosestream << "Server: Sending item definitions to id(" << peer_id
			<< "): size=" << pkt.getSize() << std::endl;

	Send(&pkt);
}

void Server::SendNodeDef(u16 peer_id,
		INodeDefManager *nodedef, u16 protocol_version)
{
	DSTACK(FUNCTION_NAME);

	NetworkPacket pkt(TOCLIENT_NODEDEF, 0, peer_id);

	/*
		u16 command
		u32 length of the next item
		zlib-compressed serialized NodeDefManager
	*/
	std::ostringstream tmp_os(std::ios::binary);
	nodedef->serialize(tmp_os, protocol_version);
	std::ostringstream tmp_os2(std::ios::binary);
	compressZlib(tmp_os.str(), tmp_os2);

	pkt.putLongString(tmp_os2.str());

	// Make data buffer
	verbosestream << "Server: Sending node definitions to id(" << peer_id
			<< "): size=" << pkt.getSize() << std::endl;

	Send(&pkt);
}

/*
	Non-static send methods
*/

void Server::SendInventory(PlayerSAO* playerSAO)
{
	DSTACK(FUNCTION_NAME);

	UpdateCrafting(playerSAO->getPlayer());

	/*
		Serialize it
	*/

	NetworkPacket pkt(TOCLIENT_INVENTORY, 0, playerSAO->getPeerID());

	std::ostringstream os;
	playerSAO->getInventory()->serialize(os);

	std::string s = os.str();

	pkt.putRawString(s.c_str(), s.size());
	Send(&pkt);
}

void Server::SendChatMessage(u16 peer_id, const std::wstring &message)
{
	DSTACK(FUNCTION_NAME);
	if (peer_id != PEER_ID_INEXISTENT) {
		NetworkPacket pkt(TOCLIENT_CHAT_MESSAGE, 0, peer_id);

		if (m_clients.getProtocolVersion(peer_id) < 27)
			pkt << unescape_enriched(message);
		else
			pkt << message;

		Send(&pkt);
	} else {
		for (u16 id : m_clients.getClientIDs())
			SendChatMessage(id, message);
	}
}

void Server::SendShowFormspecMessage(u16 peer_id, const std::string &formspec,
                                     const std::string &formname)
{
	DSTACK(FUNCTION_NAME);

	NetworkPacket pkt(TOCLIENT_SHOW_FORMSPEC, 0 , peer_id);
	if (formspec == "" ){
		//the client should close the formspec
		pkt.putLongString("");
	} else {
		pkt.putLongString(FORMSPEC_VERSION_STRING + formspec);
	}
	pkt << formname;

	Send(&pkt);
}

// Spawns a particle on peer with peer_id
void Server::SendSpawnParticle(u16 peer_id, u16 protocol_version,
				v3f pos, v3f velocity, v3f acceleration,
				float expirationtime, float size, bool collisiondetection,
				bool collision_removal,
				bool vertical, const std::string &texture,
				const struct TileAnimationParams &animation, u8 glow)
{
	DSTACK(FUNCTION_NAME);
	static const float radius =
			g_settings->getS16("max_block_send_distance") * MAP_BLOCKSIZE * BS;

	if (peer_id == PEER_ID_INEXISTENT) {
		std::vector<u16> clients = m_clients.getClientIDs();

		for (std::vector<u16>::iterator i = clients.begin(); i != clients.end(); ++i) {
			RemotePlayer *player = m_env->getPlayer(*i);
			if (!player)
				continue;

			PlayerSAO *sao = player->getPlayerSAO();
			if (!sao)
				continue;

			// Do not send to distant clients
			if (sao->getBasePosition().getDistanceFrom(pos * BS) > radius)
				continue;

			SendSpawnParticle(*i, player->protocol_version,
					pos, velocity, acceleration,
					expirationtime, size, collisiondetection,
					collision_removal, vertical, texture, animation, glow);
		}
		return;
	}

	NetworkPacket pkt(TOCLIENT_SPAWN_PARTICLE, 0, peer_id);

	pkt << pos << velocity << acceleration << expirationtime
			<< size << collisiondetection;
	pkt.putLongString(texture);
	pkt << vertical;
	pkt << collision_removal;
	// This is horrible but required (why are there two ways to serialize pkts?)
	std::ostringstream os(std::ios_base::binary);
	animation.serialize(os, protocol_version);
	pkt.putRawString(os.str());
	pkt << glow;

	Send(&pkt);
}

// Adds a ParticleSpawner on peer with peer_id
void Server::SendAddParticleSpawner(u16 peer_id, u16 protocol_version,
	u16 amount, float spawntime, v3f minpos, v3f maxpos,
	v3f minvel, v3f maxvel, v3f minacc, v3f maxacc, float minexptime, float maxexptime,
	float minsize, float maxsize, bool collisiondetection, bool collision_removal,
	u16 attached_id, bool vertical, const std::string &texture, u32 id,
	const struct TileAnimationParams &animation, u8 glow)
{
	DSTACK(FUNCTION_NAME);
	if (peer_id == PEER_ID_INEXISTENT) {
		// This sucks and should be replaced:
		std::vector<u16> clients = m_clients.getClientIDs();
		for (std::vector<u16>::iterator i = clients.begin(); i != clients.end(); ++i) {
			RemotePlayer *player = m_env->getPlayer(*i);
			if (!player)
				continue;
			SendAddParticleSpawner(*i, player->protocol_version,
					amount, spawntime, minpos, maxpos,
					minvel, maxvel, minacc, maxacc, minexptime, maxexptime,
					minsize, maxsize, collisiondetection, collision_removal,
					attached_id, vertical, texture, id, animation, glow);
		}
		return;
	}

	NetworkPacket pkt(TOCLIENT_ADD_PARTICLESPAWNER, 0, peer_id);

	pkt << amount << spawntime << minpos << maxpos << minvel << maxvel
			<< minacc << maxacc << minexptime << maxexptime << minsize
			<< maxsize << collisiondetection;

	pkt.putLongString(texture);

	pkt << id << vertical;
	pkt << collision_removal;
	pkt << attached_id;
	// This is horrible but required
	std::ostringstream os(std::ios_base::binary);
	animation.serialize(os, protocol_version);
	pkt.putRawString(os.str());
	pkt << glow;

	Send(&pkt);
}

void Server::SendDeleteParticleSpawner(u16 peer_id, u32 id)
{
	DSTACK(FUNCTION_NAME);

	NetworkPacket pkt(TOCLIENT_DELETE_PARTICLESPAWNER_LEGACY, 2, peer_id);

	// Ugly error in this packet
	pkt << (u16) id;

	if (peer_id != PEER_ID_INEXISTENT) {
		Send(&pkt);
	}
	else {
		m_clients.sendToAll(&pkt);
	}

}

void Server::SendHUDAdd(u16 peer_id, u32 id, HudElement *form)
{
	NetworkPacket pkt(TOCLIENT_HUDADD, 0 , peer_id);

	pkt << id << (u8) form->type << form->pos << form->name << form->scale
			<< form->text << form->number << form->item << form->dir
			<< form->align << form->offset << form->world_pos << form->size;

	Send(&pkt);
}

void Server::SendHUDRemove(u16 peer_id, u32 id)
{
	NetworkPacket pkt(TOCLIENT_HUDRM, 4, peer_id);
	pkt << id;
	Send(&pkt);
}

void Server::SendHUDChange(u16 peer_id, u32 id, HudElementStat stat, void *value)
{
	NetworkPacket pkt(TOCLIENT_HUDCHANGE, 0, peer_id);
	pkt << id << (u8) stat;

	switch (stat) {
		case HUD_STAT_POS:
		case HUD_STAT_SCALE:
		case HUD_STAT_ALIGN:
		case HUD_STAT_OFFSET:
			pkt << *(v2f *) value;
			break;
		case HUD_STAT_NAME:
		case HUD_STAT_TEXT:
			pkt << *(std::string *) value;
			break;
		case HUD_STAT_WORLD_POS:
			pkt << *(v3f *) value;
			break;
		case HUD_STAT_SIZE:
			pkt << *(v2s32 *) value;
			break;
		case HUD_STAT_NUMBER:
		case HUD_STAT_ITEM:
		case HUD_STAT_DIR:
		default:
			pkt << *(u32 *) value;
			break;
	}

	Send(&pkt);
}

void Server::SendHUDSetFlags(u16 peer_id, u32 flags, u32 mask)
{
	NetworkPacket pkt(TOCLIENT_HUD_SET_FLAGS, 4 + 4, peer_id);

	flags &= ~(HUD_FLAG_HEALTHBAR_VISIBLE | HUD_FLAG_BREATHBAR_VISIBLE);

	pkt << flags << mask;

	Send(&pkt);
}

void Server::SendHUDSetParam(u16 peer_id, u16 param, const std::string &value)
{
	NetworkPacket pkt(TOCLIENT_HUD_SET_PARAM, 0, peer_id);
	pkt << param << value;
	Send(&pkt);
}

void Server::SendSetSky(u16 peer_id, const video::SColor &bgcolor,
		const std::string &type, const std::vector<std::string> &params,
		bool &clouds)
{
	NetworkPacket pkt(TOCLIENT_SET_SKY, 0, peer_id);
	pkt << bgcolor << type << (u16) params.size();

	for(size_t i=0; i<params.size(); i++)
		pkt << params[i];

	pkt << clouds;

	Send(&pkt);
}

void Server::SendCloudParams(u16 peer_id, float density,
		const video::SColor &color_bright,
		const video::SColor &color_ambient,
		float height,
		float thickness,
		const v2f &speed)
{
	NetworkPacket pkt(TOCLIENT_CLOUD_PARAMS, 0, peer_id);
	pkt << density << color_bright << color_ambient
			<< height << thickness << speed;

	Send(&pkt);
}

void Server::SendOverrideDayNightRatio(u16 peer_id, bool do_override,
		float ratio)
{
	NetworkPacket pkt(TOCLIENT_OVERRIDE_DAY_NIGHT_RATIO,
			1 + 2, peer_id);

	pkt << do_override << (u16) (ratio * 65535);

	Send(&pkt);
}

void Server::SendTimeOfDay(u16 peer_id, u16 time, f32 time_speed)
{
	DSTACK(FUNCTION_NAME);

	NetworkPacket pkt(TOCLIENT_TIME_OF_DAY, 0, peer_id);
	pkt << time << time_speed;

	if (peer_id == PEER_ID_INEXISTENT) {
		m_clients.sendToAll(&pkt);
	}
	else {
		Send(&pkt);
	}
}

void Server::SendPlayerHP(u16 peer_id)
{
	DSTACK(FUNCTION_NAME);
	PlayerSAO *playersao = getPlayerSAO(peer_id);
	// In some rare case if the player is disconnected
	// while Lua call l_punch, for example, this can be NULL
	if (!playersao)
		return;

	SendHP(peer_id, playersao->getHP());
	m_script->player_event(playersao,"health_changed");

	// Send to other clients
	std::string str = gob_cmd_punched(playersao->readDamage(), playersao->getHP());
	ActiveObjectMessage aom(playersao->getId(), true, str);
	playersao->m_messages_out.push(aom);
}

void Server::SendPlayerBreath(PlayerSAO *sao)
{
	DSTACK(FUNCTION_NAME);
	assert(sao);

	m_script->player_event(sao, "breath_changed");
	SendBreath(sao->getPeerID(), sao->getBreath());
}

void Server::SendMovePlayer(u16 peer_id)
{
	DSTACK(FUNCTION_NAME);
	RemotePlayer *player = m_env->getPlayer(peer_id);
	assert(player);
	PlayerSAO *sao = player->getPlayerSAO();
	assert(sao);

	NetworkPacket pkt(TOCLIENT_MOVE_PLAYER, sizeof(v3f) + sizeof(f32) * 2, peer_id);
	pkt << sao->getBasePosition() << sao->getPitch() << sao->getYaw();

	{
		v3f pos = sao->getBasePosition();
		verbosestream << "Server: Sending TOCLIENT_MOVE_PLAYER"
				<< " pos=(" << pos.X << "," << pos.Y << "," << pos.Z << ")"
				<< " pitch=" << sao->getPitch()
				<< " yaw=" << sao->getYaw()
				<< std::endl;
	}

	Send(&pkt);
}

void Server::SendLocalPlayerAnimations(u16 peer_id, v2s32 animation_frames[4], f32 animation_speed)
{
	NetworkPacket pkt(TOCLIENT_LOCAL_PLAYER_ANIMATIONS, 0,
		peer_id);

	pkt << animation_frames[0] << animation_frames[1] << animation_frames[2]
			<< animation_frames[3] << animation_speed;

	Send(&pkt);
}

void Server::SendEyeOffset(u16 peer_id, v3f first, v3f third)
{
	NetworkPacket pkt(TOCLIENT_EYE_OFFSET, 0, peer_id);
	pkt << first << third;
	Send(&pkt);
}
void Server::SendPlayerPrivileges(u16 peer_id)
{
	RemotePlayer *player = m_env->getPlayer(peer_id);
	assert(player);
	if(player->peer_id == PEER_ID_INEXISTENT)
		return;

	std::set<std::string> privs;
	m_script->getAuth(player->getName(), NULL, &privs);

	NetworkPacket pkt(TOCLIENT_PRIVILEGES, 0, peer_id);
	pkt << (u16) privs.size();

	for(std::set<std::string>::const_iterator i = privs.begin();
			i != privs.end(); ++i) {
		pkt << (*i);
	}

	Send(&pkt);
}

void Server::SendPlayerInventoryFormspec(u16 peer_id)
{
	RemotePlayer *player = m_env->getPlayer(peer_id);
	assert(player);
	if(player->peer_id == PEER_ID_INEXISTENT)
		return;

	NetworkPacket pkt(TOCLIENT_INVENTORY_FORMSPEC, 0, peer_id);
	pkt.putLongString(FORMSPEC_VERSION_STRING + player->inventory_formspec);
	Send(&pkt);
}

u32 Server::SendActiveObjectRemoveAdd(u16 peer_id, const std::string &datas)
{
	NetworkPacket pkt(TOCLIENT_ACTIVE_OBJECT_REMOVE_ADD, datas.size(), peer_id);
	pkt.putRawString(datas.c_str(), datas.size());
	Send(&pkt);
	return pkt.getSize();
}

void Server::SendActiveObjectMessages(u16 peer_id, const std::string &datas, bool reliable)
{
	NetworkPacket pkt(TOCLIENT_ACTIVE_OBJECT_MESSAGES,
			datas.size(), peer_id);

	pkt.putRawString(datas.c_str(), datas.size());

	m_clients.send(pkt.getPeerId(),
			reliable ? clientCommandFactoryTable[pkt.getCommand()].channel : 1,
			&pkt, reliable);

}

s32 Server::playSound(const SimpleSoundSpec &spec,
		const ServerSoundParams &params)
{
	// Find out initial position of sound
	bool pos_exists = false;
	v3f pos = params.getPos(m_env, &pos_exists);
	// If position is not found while it should be, cancel sound
	if(pos_exists != (params.type != ServerSoundParams::SSP_LOCAL))
		return -1;

	// Filter destination clients
	std::vector<u16> dst_clients;
	if(params.to_player != "")
	{
		RemotePlayer *player = m_env->getPlayer(params.to_player.c_str());
		if(!player){
			infostream<<"Server::playSound: Player \""<<params.to_player
					<<"\" not found"<<std::endl;
			return -1;
		}
		if(player->peer_id == PEER_ID_INEXISTENT){
			infostream<<"Server::playSound: Player \""<<params.to_player
					<<"\" not connected"<<std::endl;
			return -1;
		}
		dst_clients.push_back(player->peer_id);
	}
	else {
		std::vector<u16> clients = m_clients.getClientIDs();

		for (std::vector<u16>::iterator i = clients.begin(); i != clients.end(); ++i) {
			RemotePlayer *player = m_env->getPlayer(*i);
			if (!player)
				continue;

			PlayerSAO *sao = player->getPlayerSAO();
			if (!sao)
				continue;

			if (pos_exists) {
				if(sao->getBasePosition().getDistanceFrom(pos) >
						params.max_hear_distance)
					continue;
			}
			dst_clients.push_back(*i);
		}
	}

	if(dst_clients.empty())
		return -1;

	// Create the sound
	s32 id = m_next_sound_id++;
	// The sound will exist as a reference in m_playing_sounds
	m_playing_sounds[id] = ServerPlayingSound();
	ServerPlayingSound &psound = m_playing_sounds[id];
	psound.params = params;
	psound.spec = spec;

	float gain = params.gain * spec.gain;
	NetworkPacket pkt(TOCLIENT_PLAY_SOUND, 0);
	pkt << id << spec.name << gain
			<< (u8) params.type << pos << params.object
			<< params.loop << params.fade;

	// Backwards compability
	bool play_sound = gain > 0;

	for (std::vector<u16>::iterator i = dst_clients.begin();
			i != dst_clients.end(); ++i) {
		if (play_sound || m_clients.getProtocolVersion(*i) >= 32) {
			psound.clients.insert(*i);
			m_clients.send(*i, 0, &pkt, true);
		}
	}
	return id;
}
void Server::stopSound(s32 handle)
{
	// Get sound reference
	UNORDERED_MAP<s32, ServerPlayingSound>::iterator i = m_playing_sounds.find(handle);
	if (i == m_playing_sounds.end())
		return;
	ServerPlayingSound &psound = i->second;

	NetworkPacket pkt(TOCLIENT_STOP_SOUND, 4);
	pkt << handle;

	for (UNORDERED_SET<u16>::iterator i = psound.clients.begin();
			i != psound.clients.end(); ++i) {
		// Send as reliable
		m_clients.send(*i, 0, &pkt, true);
	}
	// Remove sound reference
	m_playing_sounds.erase(i);
}

void Server::fadeSound(s32 handle, float step, float gain)
{
	// Get sound reference
	UNORDERED_MAP<s32, ServerPlayingSound>::iterator i =
			m_playing_sounds.find(handle);
	if (i == m_playing_sounds.end())
		return;

	ServerPlayingSound &psound = i->second;
	psound.params.gain = gain;

	NetworkPacket pkt(TOCLIENT_FADE_SOUND, 4);
	pkt << handle << step << gain;

	// Backwards compability
	bool play_sound = gain > 0;
	ServerPlayingSound compat_psound = psound;
	compat_psound.clients.clear();

	NetworkPacket compat_pkt(TOCLIENT_STOP_SOUND, 4);
	compat_pkt << handle;

	for (UNORDERED_SET<u16>::iterator it = psound.clients.begin();
			it != psound.clients.end();) {
		if (m_clients.getProtocolVersion(*it) >= 32) {
			// Send as reliable
			m_clients.send(*it, 0, &pkt, true);
			++it;
		} else {
			compat_psound.clients.insert(*it);
			// Stop old sound
			m_clients.send(*it, 0, &compat_pkt, true);
			psound.clients.erase(it++);
		}
	}

	// Remove sound reference
	if (!play_sound || psound.clients.size() == 0)
		m_playing_sounds.erase(i);

	if (play_sound && compat_psound.clients.size() > 0) {
		// Play new sound volume on older clients
		playSound(compat_psound.spec, compat_psound.params);
	}
}

void Server::sendRemoveNode(v3s16 p, u16 ignore_id,
	std::vector<u16> *far_players, float far_d_nodes)
{
	float maxd = far_d_nodes*BS;
	v3f p_f = intToFloat(p, BS);

	NetworkPacket pkt(TOCLIENT_REMOVENODE, 6);
	pkt << p;

	std::vector<u16> clients = m_clients.getClientIDs();
	for (std::vector<u16>::iterator i = clients.begin(); i != clients.end(); ++i) {
		if (far_players) {
			// Get player
			if (RemotePlayer *player = m_env->getPlayer(*i)) {
				PlayerSAO *sao = player->getPlayerSAO();
				if (!sao)
					continue;

				// If player is far away, only set modified blocks not sent
				v3f player_pos = sao->getBasePosition();
				if (player_pos.getDistanceFrom(p_f) > maxd) {
					far_players->push_back(*i);
					continue;
				}
			}
		}

		// Send as reliable
		m_clients.send(*i, 0, &pkt, true);
	}
}

void Server::sendAddNode(v3s16 p, MapNode n, u16 ignore_id,
		std::vector<u16> *far_players, float far_d_nodes,
		bool remove_metadata)
{
	float maxd = far_d_nodes*BS;
	v3f p_f = intToFloat(p, BS);

	std::vector<u16> clients = m_clients.getClientIDs();
	for(std::vector<u16>::iterator i = clients.begin();	i != clients.end(); ++i) {
		if (far_players) {
			// Get player
			if (RemotePlayer *player = m_env->getPlayer(*i)) {
				PlayerSAO *sao = player->getPlayerSAO();
				if (!sao)
					continue;

				// If player is far away, only set modified blocks not sent
				v3f player_pos = sao->getBasePosition();
				if(player_pos.getDistanceFrom(p_f) > maxd) {
					far_players->push_back(*i);
					continue;
				}
			}
		}

		NetworkPacket pkt(TOCLIENT_ADDNODE, 6 + 2 + 1 + 1 + 1);
		m_clients.lock();
		RemoteClient* client = m_clients.lockedGetClientNoEx(*i);
		if (client != 0) {
			pkt << p << n.param0 << n.param1 << n.param2
					<< (u8) (remove_metadata ? 0 : 1);
		}
		m_clients.unlock();

		// Send as reliable
		if (pkt.getSize() > 0)
			m_clients.send(*i, 0, &pkt, true);
	}
}

void Server::setBlockNotSent(v3s16 p)
{
	std::vector<u16> clients = m_clients.getClientIDs();
	m_clients.lock();
	for(std::vector<u16>::iterator i = clients.begin();
		i != clients.end(); ++i) {
		RemoteClient *client = m_clients.lockedGetClientNoEx(*i);
		client->SetBlockNotSent(p);
	}
	m_clients.unlock();
}

void Server::SendBlockNoLock(u16 peer_id, MapBlock *block, u8 ver, u16 net_proto_version)
{
	DSTACK(FUNCTION_NAME);

	v3s16 p = block->getPos();

	/*
		Create a packet with the block in the right format
	*/

	std::ostringstream os(std::ios_base::binary);
	block->serialize(os, ver, false);
	block->serializeNetworkSpecific(os);
	std::string s = os.str();

	NetworkPacket pkt(TOCLIENT_BLOCKDATA, 2 + 2 + 2 + 2 + s.size(), peer_id);

	pkt << p;
	pkt.putRawString(s.c_str(), s.size());
	Send(&pkt);
}

void Server::SendBlocks(float dtime)
{
	DSTACK(FUNCTION_NAME);

	MutexAutoLock envlock(m_env_mutex);
	//TODO check if one big lock could be faster then multiple small ones

	ScopeProfiler sp(g_profiler, "Server: sel and send blocks to clients");

	std::vector<PrioritySortedBlockTransfer> queue;

	s32 total_sending = 0;

	{
		ScopeProfiler sp(g_profiler, "Server: selecting blocks for sending");

		std::vector<u16> clients = m_clients.getClientIDs();

		m_clients.lock();
		for(std::vector<u16>::iterator i = clients.begin();
			i != clients.end(); ++i) {
			RemoteClient *client = m_clients.lockedGetClientNoEx(*i, CS_Active);

			if (client == NULL)
				continue;

			total_sending += client->SendingCount();
			client->GetNextBlocks(m_env,m_emerge, dtime, queue);
		}
		m_clients.unlock();
	}

	// Sort.
	// Lowest priority number comes first.
	// Lowest is most important.
	std::sort(queue.begin(), queue.end());

	m_clients.lock();
	for(u32 i=0; i<queue.size(); i++)
	{
		//TODO: Calculate limit dynamically
		if(total_sending >= g_settings->getS32
				("max_simultaneous_block_sends_server_total"))
			break;

		PrioritySortedBlockTransfer q = queue[i];

		MapBlock *block = NULL;
		try
		{
			block = m_env->getMap().getBlockNoCreate(q.pos);
		}
		catch(InvalidPositionException &e)
		{
			continue;
		}

		RemoteClient *client = m_clients.lockedGetClientNoEx(q.peer_id, CS_Active);

		if(!client)
			continue;

		SendBlockNoLock(q.peer_id, block, client->serialization_version, client->net_proto_version);

		client->SentBlock(q.pos);
		total_sending++;
	}
	m_clients.unlock();
}

void Server::fillMediaCache()
{
	DSTACK(FUNCTION_NAME);

	infostream<<"Server: Calculating media file checksums"<<std::endl;

	// Collect all media file paths
	std::vector<std::string> paths;
	for(std::vector<ModSpec>::iterator i = m_mods.begin();
			i != m_mods.end(); ++i) {
		const ModSpec &mod = *i;
		paths.push_back(mod.path + DIR_DELIM + "textures");
		paths.push_back(mod.path + DIR_DELIM + "sounds");
		paths.push_back(mod.path + DIR_DELIM + "media");
		paths.push_back(mod.path + DIR_DELIM + "models");
	}
	paths.push_back(porting::path_user + DIR_DELIM + "textures" + DIR_DELIM + "server");

	// Collect media file information from paths into cache
	for(std::vector<std::string>::iterator i = paths.begin();
			i != paths.end(); ++i) {
		std::string mediapath = *i;
		std::vector<fs::DirListNode> dirlist = fs::GetDirListing(mediapath);
		for (u32 j = 0; j < dirlist.size(); j++) {
			if (dirlist[j].dir) // Ignode dirs
				continue;
			std::string filename = dirlist[j].name;
			// If name contains illegal characters, ignore the file
			if (!string_allowed(filename, TEXTURENAME_ALLOWED_CHARS)) {
				infostream<<"Server: ignoring illegal file name: \""
						<< filename << "\"" << std::endl;
				continue;
			}
			// If name is not in a supported format, ignore it
			const char *supported_ext[] = {
				".png", ".jpg", ".bmp", ".tga",
				".pcx", ".ppm", ".psd", ".wal", ".rgb",
				".ogg",
				".x", ".b3d", ".md2", ".obj",
				NULL
			};
			if (removeStringEnd(filename, supported_ext) == ""){
				infostream << "Server: ignoring unsupported file extension: \""
						<< filename << "\"" << std::endl;
				continue;
			}
			// Ok, attempt to load the file and add to cache
			std::string filepath = mediapath + DIR_DELIM + filename;
			// Read data
			std::ifstream fis(filepath.c_str(), std::ios_base::binary);
			if (!fis.good()) {
				errorstream << "Server::fillMediaCache(): Could not open \""
						<< filename << "\" for reading" << std::endl;
				continue;
			}
			std::ostringstream tmp_os(std::ios_base::binary);
			bool bad = false;
			for(;;) {
				char buf[1024];
				fis.read(buf, 1024);
				std::streamsize len = fis.gcount();
				tmp_os.write(buf, len);
				if (fis.eof())
					break;
				if (!fis.good()) {
					bad = true;
					break;
				}
			}
			if(bad) {
				errorstream<<"Server::fillMediaCache(): Failed to read \""
						<< filename << "\"" << std::endl;
				continue;
			}
			if(tmp_os.str().length() == 0) {
				errorstream << "Server::fillMediaCache(): Empty file \""
						<< filepath << "\"" << std::endl;
				continue;
			}

			SHA1 sha1;
			sha1.addBytes(tmp_os.str().c_str(), tmp_os.str().length());

			unsigned char *digest = sha1.getDigest();
			std::string sha1_base64 = base64_encode(digest, 20);
			std::string sha1_hex = hex_encode((char*)digest, 20);
			free(digest);

			// Put in list
			m_media[filename] = MediaInfo(filepath, sha1_base64);
			verbosestream << "Server: " << sha1_hex << " is " << filename
					<< std::endl;
		}
	}
}

void Server::sendMediaAnnouncement(u16 peer_id)
{
	DSTACK(FUNCTION_NAME);

	verbosestream << "Server: Announcing files to id(" << peer_id << ")"
		<< std::endl;

	// Make packet
	std::ostringstream os(std::ios_base::binary);

	NetworkPacket pkt(TOCLIENT_ANNOUNCE_MEDIA, 0, peer_id);
	pkt << (u16) m_media.size();

	for (UNORDERED_MAP<std::string, MediaInfo>::iterator i = m_media.begin();
			i != m_media.end(); ++i) {
		pkt << i->first << i->second.sha1_digest;
	}

	pkt << g_settings->get("remote_media");
	Send(&pkt);
}

struct SendableMedia
{
	std::string name;
	std::string path;
	std::string data;

	SendableMedia(const std::string &name_="", const std::string &path_="",
	              const std::string &data_=""):
		name(name_),
		path(path_),
		data(data_)
	{}
};

void Server::sendRequestedMedia(u16 peer_id,
		const std::vector<std::string> &tosend)
{
	DSTACK(FUNCTION_NAME);

	verbosestream<<"Server::sendRequestedMedia(): "
			<<"Sending files to client"<<std::endl;

	/* Read files */

	// Put 5kB in one bunch (this is not accurate)
	u32 bytes_per_bunch = 5000;

	std::vector< std::vector<SendableMedia> > file_bunches;
	file_bunches.push_back(std::vector<SendableMedia>());

	u32 file_size_bunch_total = 0;

	for(std::vector<std::string>::const_iterator i = tosend.begin();
			i != tosend.end(); ++i) {
		const std::string &name = *i;

		if (m_media.find(name) == m_media.end()) {
			errorstream<<"Server::sendRequestedMedia(): Client asked for "
					<<"unknown file \""<<(name)<<"\""<<std::endl;
			continue;
		}

		//TODO get path + name
		std::string tpath = m_media[name].path;

		// Read data
		std::ifstream fis(tpath.c_str(), std::ios_base::binary);
		if(fis.good() == false){
			errorstream<<"Server::sendRequestedMedia(): Could not open \""
					<<tpath<<"\" for reading"<<std::endl;
			continue;
		}
		std::ostringstream tmp_os(std::ios_base::binary);
		bool bad = false;
		for(;;) {
			char buf[1024];
			fis.read(buf, 1024);
			std::streamsize len = fis.gcount();
			tmp_os.write(buf, len);
			file_size_bunch_total += len;
			if(fis.eof())
				break;
			if(!fis.good()) {
				bad = true;
				break;
			}
		}
		if(bad) {
			errorstream<<"Server::sendRequestedMedia(): Failed to read \""
					<<name<<"\""<<std::endl;
			continue;
		}
		/*infostream<<"Server::sendRequestedMedia(): Loaded \""
				<<tname<<"\""<<std::endl;*/
		// Put in list
		file_bunches[file_bunches.size()-1].push_back(
				SendableMedia(name, tpath, tmp_os.str()));

		// Start next bunch if got enough data
		if(file_size_bunch_total >= bytes_per_bunch) {
			file_bunches.push_back(std::vector<SendableMedia>());
			file_size_bunch_total = 0;
		}

	}

	/* Create and send packets */

	u16 num_bunches = file_bunches.size();
	for(u16 i = 0; i < num_bunches; i++) {
		/*
			u16 command
			u16 total number of texture bunches
			u16 index of this bunch
			u32 number of files in this bunch
			for each file {
				u16 length of name
				string name
				u32 length of data
				data
			}
		*/

		NetworkPacket pkt(TOCLIENT_MEDIA, 4 + 0, peer_id);
		pkt << num_bunches << i << (u32) file_bunches[i].size();

		for(std::vector<SendableMedia>::iterator
				j = file_bunches[i].begin();
				j != file_bunches[i].end(); ++j) {
			pkt << j->name;
			pkt.putLongString(j->data);
		}

		verbosestream << "Server::sendRequestedMedia(): bunch "
				<< i << "/" << num_bunches
				<< " files=" << file_bunches[i].size()
				<< " size="  << pkt.getSize() << std::endl;
		Send(&pkt);
	}
}

void Server::sendDetachedInventory(const std::string &name, u16 peer_id)
{
	if(m_detached_inventories.count(name) == 0) {
		errorstream<<FUNCTION_NAME<<": \""<<name<<"\" not found"<<std::endl;
		return;
	}
	Inventory *inv = m_detached_inventories[name];
	std::ostringstream os(std::ios_base::binary);

	os << serializeString(name);
	inv->serialize(os);

	// Make data buffer
	std::string s = os.str();

	NetworkPacket pkt(TOCLIENT_DETACHED_INVENTORY, 0, peer_id);
	pkt.putRawString(s.c_str(), s.size());

	const std::string &check = m_detached_inventories_player[name];
	if (peer_id == PEER_ID_INEXISTENT) {
		if (check == "")
			return m_clients.sendToAll(&pkt);
		RemotePlayer *p = m_env->getPlayer(check.c_str());
		if (p)
			m_clients.send(p->peer_id, 0, &pkt, true);
	} else {
		if (check == "" || getPlayerName(peer_id) == check)
			Send(&pkt);
	}
}

void Server::sendDetachedInventories(u16 peer_id)
{
	DSTACK(FUNCTION_NAME);

	for(std::map<std::string, Inventory*>::iterator
			i = m_detached_inventories.begin();
			i != m_detached_inventories.end(); ++i) {
		const std::string &name = i->first;
		//Inventory *inv = i->second;
		sendDetachedInventory(name, peer_id);
	}
}

/*
	Something random
*/

void Server::DiePlayer(u16 peer_id)
{
	DSTACK(FUNCTION_NAME);
	PlayerSAO *playersao = getPlayerSAO(peer_id);
	// In some rare cases this can be NULL -- if the player is disconnected
	// when a Lua function modifies l_punch, for example
	if (!playersao)
		return;

	infostream << "Server::DiePlayer(): Player "
			<< playersao->getPlayer()->getName()
			<< " dies" << std::endl;

	playersao->setHP(0);

	// Trigger scripted stuff
	m_script->on_dieplayer(playersao);

	SendPlayerHP(peer_id);
	SendDeathscreen(peer_id, false, v3f(0,0,0));
}

void Server::RespawnPlayer(u16 peer_id)
{
	DSTACK(FUNCTION_NAME);

	PlayerSAO *playersao = getPlayerSAO(peer_id);
	assert(playersao);

	infostream << "Server::RespawnPlayer(): Player "
			<< playersao->getPlayer()->getName()
			<< " respawns" << std::endl;

	playersao->setHP(PLAYER_MAX_HP);
	playersao->setBreath(PLAYER_MAX_BREATH);

	bool repositioned = m_script->on_respawnplayer(playersao);
	if (!repositioned) {
		// setPos will send the new position to client
		playersao->setPos(findSpawnPos());
	}

	SendPlayerHP(peer_id);
}


void Server::DenySudoAccess(u16 peer_id)
{
	DSTACK(FUNCTION_NAME);

	NetworkPacket pkt(TOCLIENT_DENY_SUDO_MODE, 0, peer_id);
	Send(&pkt);
}


void Server::DenyAccessVerCompliant(u16 peer_id, u16 proto_ver, AccessDeniedCode reason,
		const std::string &str_reason, bool reconnect)
{
	if (proto_ver >= 25) {
		SendAccessDenied(peer_id, reason, str_reason, reconnect);
	} else {
		std::wstring wreason = utf8_to_wide(
			reason == SERVER_ACCESSDENIED_CUSTOM_STRING ? str_reason :
			accessDeniedStrings[(u8)reason]);
		SendAccessDenied_Legacy(peer_id, wreason);
	}

	m_clients.event(peer_id, CSE_SetDenied);
	m_con.DisconnectPeer(peer_id);
}


void Server::DenyAccess(u16 peer_id, AccessDeniedCode reason, const std::string &custom_reason)
{
	DSTACK(FUNCTION_NAME);

	SendAccessDenied(peer_id, reason, custom_reason);
	m_clients.event(peer_id, CSE_SetDenied);
	m_con.DisconnectPeer(peer_id);
}

// 13/03/15: remove this function when protocol version 25 will become
// the minimum version for MT users, maybe in 1 year
void Server::DenyAccess_Legacy(u16 peer_id, const std::wstring &reason)
{
	DSTACK(FUNCTION_NAME);

	SendAccessDenied_Legacy(peer_id, reason);
	m_clients.event(peer_id, CSE_SetDenied);
	m_con.DisconnectPeer(peer_id);
}

void Server::acceptAuth(u16 peer_id, bool forSudoMode)
{
	DSTACK(FUNCTION_NAME);

	if (!forSudoMode) {
		RemoteClient* client = getClient(peer_id, CS_Invalid);

		NetworkPacket resp_pkt(TOCLIENT_AUTH_ACCEPT, 1 + 6 + 8 + 4, peer_id);

		// Right now, the auth mechs don't change between login and sudo mode.
		u32 sudo_auth_mechs = client->allowed_auth_mechs;
		client->allowed_sudo_mechs = sudo_auth_mechs;

		resp_pkt << v3f(0,0,0) << (u64) m_env->getServerMap().getSeed()
				<< g_settings->getFloat("dedicated_server_step")
				<< sudo_auth_mechs;

		Send(&resp_pkt);
		m_clients.event(peer_id, CSE_AuthAccept);
	} else {
		NetworkPacket resp_pkt(TOCLIENT_ACCEPT_SUDO_MODE, 1 + 6 + 8 + 4, peer_id);

		// We only support SRP right now
		u32 sudo_auth_mechs = AUTH_MECHANISM_FIRST_SRP;

		resp_pkt << sudo_auth_mechs;
		Send(&resp_pkt);
		m_clients.event(peer_id, CSE_SudoSuccess);
	}
}

void Server::DeleteClient(u16 peer_id, ClientDeletionReason reason)
{
	DSTACK(FUNCTION_NAME);
	std::wstring message;
	{
		/*
			Clear references to playing sounds
		*/
		for (UNORDERED_MAP<s32, ServerPlayingSound>::iterator
				 i = m_playing_sounds.begin(); i != m_playing_sounds.end();) {
			ServerPlayingSound &psound = i->second;
			psound.clients.erase(peer_id);
			if (psound.clients.empty())
				m_playing_sounds.erase(i++);
			else
				++i;
		}

		RemotePlayer *player = m_env->getPlayer(peer_id);

		/* Run scripts and remove from environment */
		if (player != NULL) {
			PlayerSAO *playersao = player->getPlayerSAO();
			assert(playersao);

			m_script->on_leaveplayer(playersao, reason == CDR_TIMEOUT);

			playersao->disconnected();
		}

		/*
			Print out action
		*/
		{
			if(player != NULL && reason != CDR_DENY) {
				std::ostringstream os(std::ios_base::binary);
				std::vector<u16> clients = m_clients.getClientIDs();

				for(std::vector<u16>::iterator i = clients.begin();
					i != clients.end(); ++i) {
					// Get player
					RemotePlayer *player = m_env->getPlayer(*i);
					if (!player)
						continue;

					// Get name of player
					os << player->getName() << " ";
				}

				std::string name = player->getName();
				actionstream << name << " "
						<< (reason == CDR_TIMEOUT ? "times out." : "leaves game.")
						<< " List of players: " << os.str() << std::endl;
				if (m_admin_chat)
					m_admin_chat->outgoing_queue.push_back(
						new ChatEventNick(CET_NICK_REMOVE, name));
			}
		}
		{
			MutexAutoLock env_lock(m_env_mutex);
			m_clients.DeleteClient(peer_id);
		}
	}

	// Send leave chat message to all remaining clients
	if(message.length() != 0)
		SendChatMessage(PEER_ID_INEXISTENT,message);
}

void Server::UpdateCrafting(RemotePlayer *player)
{
	DSTACK(FUNCTION_NAME);

	// Get a preview for crafting
	ItemStack preview;
	InventoryLocation loc;
	loc.setPlayer(player->getName());
	std::vector<ItemStack> output_replacements;
	getCraftingResult(&player->inventory, preview, output_replacements, false, this);
	m_env->getScriptIface()->item_CraftPredict(preview, player->getPlayerSAO(),
			(&player->inventory)->getList("craft"), loc);

	// Put the new preview in
	InventoryList *plist = player->inventory.getList("craftpreview");
	sanity_check(plist);
	sanity_check(plist->getSize() >= 1);
	plist->changeItem(0, preview);
}

void Server::handleChatInterfaceEvent(ChatEvent *evt)
{
	if (evt->type == CET_NICK_ADD) {
		// The terminal informed us of its nick choice
		m_admin_nick = ((ChatEventNick *)evt)->nick;
		if (!m_script->getAuth(m_admin_nick, NULL, NULL)) {
			errorstream << "You haven't set up an account." << std::endl
				<< "Please log in using the client as '"
				<< m_admin_nick << "' with a secure password." << std::endl
				<< "Until then, you can't execute admin tasks via the console," << std::endl
				<< "and everybody can claim the user account instead of you," << std::endl
				<< "giving them full control over this server." << std::endl;
		}
	} else {
		assert(evt->type == CET_CHAT);
		handleAdminChat((ChatEventChat *)evt);
	}
}

std::wstring Server::handleChat(const std::string &name, const std::wstring &wname,
	std::wstring wmessage, bool check_shout_priv, RemotePlayer *player)
{
	// If something goes wrong, this player is to blame
	RollbackScopeActor rollback_scope(m_rollback,
		std::string("player:") + name);

	if (g_settings->getBool("strip_color_codes"))
		wmessage = unescape_enriched(wmessage);

	if (player) {
		switch (player->canSendChatMessage()) {
			case RPLAYER_CHATRESULT_FLOODING: {
				std::wstringstream ws;
				ws << L"You cannot send more messages. You are limited to "
				   << g_settings->getFloat("chat_message_limit_per_10sec")
				   << L" messages per 10 seconds.";
				return ws.str();
			}
			case RPLAYER_CHATRESULT_KICK:
				DenyAccess_Legacy(player->peer_id,
						L"You have been kicked due to message flooding.");
				return L"";
			case RPLAYER_CHATRESULT_OK:
				break;
			default:
				FATAL_ERROR("Unhandled chat filtering result found.");
		}
	}

	if (m_max_chatmessage_length > 0
			&& wmessage.length() > m_max_chatmessage_length) {
		return L"Your message exceed the maximum chat message limit set on the server. "
				L"It was refused. Send a shorter message";
	}

	// Run script hook, exit if script ate the chat message
	if (m_script->on_chat_message(name, wide_to_utf8(wmessage)))
		return L"";

	// Line to send
	std::wstring line;
	// Whether to send line to the player that sent the message, or to all players
	bool broadcast_line = true;

	if (check_shout_priv && !checkPriv(name, "shout")) {
		line += L"-!- You don't have permission to shout.";
		broadcast_line = false;
	} else {
		line += L"<";
		line += wname;
		line += L"> ";
		line += wmessage;
	}

	/*
		Tell calling method to send the message to sender
	*/
	if (!broadcast_line) {
		return line;
	} else {
		/*
			Send the message to others
		*/
		actionstream << "CHAT: " << wide_to_narrow(unescape_enriched(line)) << std::endl;

		std::vector<u16> clients = m_clients.getClientIDs();

		/*
			Send the message back to the inital sender
			if they are using protocol version >= 29
		*/

		u16 peer_id_to_avoid_sending = (player ? player->peer_id : PEER_ID_INEXISTENT);
		if (player && player->protocol_version >= 29)
			peer_id_to_avoid_sending = PEER_ID_INEXISTENT;

		for (u16 i = 0; i < clients.size(); i++) {
			u16 cid = clients[i];
			if (cid != peer_id_to_avoid_sending)
				SendChatMessage(cid, line);
		}
	}
	return L"";
}

void Server::handleAdminChat(const ChatEventChat *evt)
{
	std::string name = evt->nick;
	std::wstring wname = utf8_to_wide(name);
	std::wstring wmessage = evt->evt_msg;

	std::wstring answer = handleChat(name, wname, wmessage);

	// If asked to send answer to sender
	if (!answer.empty()) {
		m_admin_chat->outgoing_queue.push_back(new ChatEventChat("", answer));
	}
}

RemoteClient* Server::getClient(u16 peer_id, ClientState state_min)
{
	RemoteClient *client = getClientNoEx(peer_id,state_min);
	if(!client)
		throw ClientNotFoundException("Client not found");

	return client;
}
RemoteClient* Server::getClientNoEx(u16 peer_id, ClientState state_min)
{
	return m_clients.getClientNoEx(peer_id, state_min);
}

std::string Server::getPlayerName(u16 peer_id)
{
	RemotePlayer *player = m_env->getPlayer(peer_id);
	if (player == NULL)
		return "[id="+itos(peer_id)+"]";
	return player->getName();
}

PlayerSAO* Server::getPlayerSAO(u16 peer_id)
{
	RemotePlayer *player = m_env->getPlayer(peer_id);
	if (player == NULL)
		return NULL;
	return player->getPlayerSAO();
}

std::wstring Server::getStatusString()
{
	std::wostringstream os(std::ios_base::binary);
	os<<L"# Server: ";
	// Version
	os<<L"version="<<narrow_to_wide(g_version_string);
	// Uptime
	os<<L", uptime="<<m_uptime.get();
	// Max lag estimate
	os<<L", max_lag="<<m_env->getMaxLagEstimate();
	// Information about clients
	bool first = true;
	os<<L", clients={";
	std::vector<u16> clients = m_clients.getClientIDs();
	for (std::vector<u16>::iterator i = clients.begin(); i != clients.end(); ++i) {
		// Get player
		RemotePlayer *player = m_env->getPlayer(*i);
		// Get name of player
		std::wstring name = L"unknown";
		if (player != NULL)
			name = narrow_to_wide(player->getName());
		// Add name to information string
		if(!first)
			os << L", ";
		else
			first = false;
		os << name;
	}
	os << L"}";
	if(((ServerMap*)(&m_env->getMap()))->isSavingEnabled() == false)
		os<<std::endl<<L"# Server: "<<" WARNING: Map saving is disabled.";
	if(g_settings->get("motd") != "")
		os<<std::endl<<L"# Server: "<<narrow_to_wide(g_settings->get("motd"));
	return os.str();
}

std::set<std::string> Server::getPlayerEffectivePrivs(const std::string &name)
{
	std::set<std::string> privs;
	m_script->getAuth(name, NULL, &privs);
	return privs;
}

bool Server::checkPriv(const std::string &name, const std::string &priv)
{
	std::set<std::string> privs = getPlayerEffectivePrivs(name);
	return (privs.count(priv) != 0);
}

void Server::reportPrivsModified(const std::string &name)
{
	if(name == "") {
		std::vector<u16> clients = m_clients.getClientIDs();
		for(std::vector<u16>::iterator i = clients.begin();
				i != clients.end(); ++i) {
			RemotePlayer *player = m_env->getPlayer(*i);
			reportPrivsModified(player->getName());
		}
	} else {
		RemotePlayer *player = m_env->getPlayer(name.c_str());
		if (!player)
			return;
		SendPlayerPrivileges(player->peer_id);
		PlayerSAO *sao = player->getPlayerSAO();
		if(!sao)
			return;
		sao->updatePrivileges(
				getPlayerEffectivePrivs(name),
				isSingleplayer());
	}
}

void Server::reportInventoryFormspecModified(const std::string &name)
{
	RemotePlayer *player = m_env->getPlayer(name.c_str());
	if (!player)
		return;
	SendPlayerInventoryFormspec(player->peer_id);
}

void Server::setIpBanned(const std::string &ip, const std::string &name)
{
	m_banmanager->add(ip, name);
}

void Server::unsetIpBanned(const std::string &ip_or_name)
{
	m_banmanager->remove(ip_or_name);
}

std::string Server::getBanDescription(const std::string &ip_or_name)
{
	return m_banmanager->getBanDescription(ip_or_name);
}

void Server::notifyPlayer(const char *name, const std::wstring &msg)
{
	// m_env will be NULL if the server is initializing
	if (!m_env)
		return;

	if (m_admin_nick == name && !m_admin_nick.empty()) {
		m_admin_chat->outgoing_queue.push_back(new ChatEventChat("", msg));
	}

	RemotePlayer *player = m_env->getPlayer(name);
	if (!player) {
		return;
	}

	if (player->peer_id == PEER_ID_INEXISTENT)
		return;

	SendChatMessage(player->peer_id, msg);
}

bool Server::showFormspec(const char *playername, const std::string &formspec,
	const std::string &formname)
{
	// m_env will be NULL if the server is initializing
	if (!m_env)
		return false;

	RemotePlayer *player = m_env->getPlayer(playername);
	if (!player)
		return false;

	SendShowFormspecMessage(player->peer_id, formspec, formname);
	return true;
}

u32 Server::hudAdd(RemotePlayer *player, HudElement *form)
{
	if (!player)
		return -1;

	u32 id = player->addHud(form);

	SendHUDAdd(player->peer_id, id, form);

	return id;
}

bool Server::hudRemove(RemotePlayer *player, u32 id) {
	if (!player)
		return false;

	HudElement* todel = player->removeHud(id);

	if (!todel)
		return false;

	delete todel;

	SendHUDRemove(player->peer_id, id);
	return true;
}

bool Server::hudChange(RemotePlayer *player, u32 id, HudElementStat stat, void *data)
{
	if (!player)
		return false;

	SendHUDChange(player->peer_id, id, stat, data);
	return true;
}

bool Server::hudSetFlags(RemotePlayer *player, u32 flags, u32 mask)
{
	if (!player)
		return false;

	SendHUDSetFlags(player->peer_id, flags, mask);
	player->hud_flags &= ~mask;
	player->hud_flags |= flags;

	PlayerSAO* playersao = player->getPlayerSAO();

	if (playersao == NULL)
		return false;

	m_script->player_event(playersao, "hud_changed");
	return true;
}

bool Server::hudSetHotbarItemcount(RemotePlayer *player, s32 hotbar_itemcount)
{
	if (!player)
		return false;

	if (hotbar_itemcount <= 0 || hotbar_itemcount > HUD_HOTBAR_ITEMCOUNT_MAX)
		return false;

	player->setHotbarItemcount(hotbar_itemcount);
	std::ostringstream os(std::ios::binary);
	writeS32(os, hotbar_itemcount);
	SendHUDSetParam(player->peer_id, HUD_PARAM_HOTBAR_ITEMCOUNT, os.str());
	return true;
}

void Server::hudSetHotbarImage(RemotePlayer *player, std::string name)
{
	if (!player)
		return;

	player->setHotbarImage(name);
	SendHUDSetParam(player->peer_id, HUD_PARAM_HOTBAR_IMAGE, name);
}

std::string Server::hudGetHotbarImage(RemotePlayer *player)
{
	if (!player)
		return "";
	return player->getHotbarImage();
}

void Server::hudSetHotbarSelectedImage(RemotePlayer *player, std::string name)
{
	if (!player)
		return;

	player->setHotbarSelectedImage(name);
	SendHUDSetParam(player->peer_id, HUD_PARAM_HOTBAR_SELECTED_IMAGE, name);
}

bool Server::setLocalPlayerAnimations(RemotePlayer *player,
		v2s32 animation_frames[4], f32 frame_speed)
{
	if (!player)
		return false;

	player->setLocalAnimations(animation_frames, frame_speed);
	SendLocalPlayerAnimations(player->peer_id, animation_frames, frame_speed);
	return true;
}

bool Server::setPlayerEyeOffset(RemotePlayer *player, v3f first, v3f third)
{
	if (!player)
		return false;

	player->eye_offset_first = first;
	player->eye_offset_third = third;
	SendEyeOffset(player->peer_id, first, third);
	return true;
}

bool Server::setSky(RemotePlayer *player, const video::SColor &bgcolor,
	const std::string &type, const std::vector<std::string> &params,
	bool &clouds)
{
	if (!player)
		return false;

	player->setSky(bgcolor, type, params, clouds);
	SendSetSky(player->peer_id, bgcolor, type, params, clouds);
	return true;
}

bool Server::setClouds(RemotePlayer *player, float density,
	const video::SColor &color_bright,
	const video::SColor &color_ambient,
	float height,
	float thickness,
	const v2f &speed)
{
	if (!player)
		return false;

	SendCloudParams(player->peer_id, density,
			color_bright, color_ambient, height,
			thickness, speed);
	return true;
}

bool Server::overrideDayNightRatio(RemotePlayer *player, bool do_override,
	float ratio)
{
	if (!player)
		return false;

	player->overrideDayNightRatio(do_override, ratio);
	SendOverrideDayNightRatio(player->peer_id, do_override, ratio);
	return true;
}

void Server::notifyPlayers(const std::wstring &msg)
{
	SendChatMessage(PEER_ID_INEXISTENT,msg);
}

void Server::spawnParticle(const std::string &playername, v3f pos,
	v3f velocity, v3f acceleration,
	float expirationtime, float size, bool
	collisiondetection, bool collision_removal,
	bool vertical, const std::string &texture,
	const struct TileAnimationParams &animation, u8 glow)
{
	// m_env will be NULL if the server is initializing
	if (!m_env)
		return;

	u16 peer_id = PEER_ID_INEXISTENT, proto_ver = 0;
	if (playername != "") {
		RemotePlayer *player = m_env->getPlayer(playername.c_str());
		if (!player)
			return;
		peer_id = player->peer_id;
		proto_ver = player->protocol_version;
	}

	SendSpawnParticle(peer_id, proto_ver, pos, velocity, acceleration,
			expirationtime, size, collisiondetection,
			collision_removal, vertical, texture, animation, glow);
}

u32 Server::addParticleSpawner(u16 amount, float spawntime,
	v3f minpos, v3f maxpos, v3f minvel, v3f maxvel, v3f minacc, v3f maxacc,
	float minexptime, float maxexptime, float minsize, float maxsize,
	bool collisiondetection, bool collision_removal,
	ServerActiveObject *attached, bool vertical, const std::string &texture,
	const std::string &playername, const struct TileAnimationParams &animation,
	u8 glow)
{
	// m_env will be NULL if the server is initializing
	if (!m_env)
		return -1;

	u16 peer_id = PEER_ID_INEXISTENT, proto_ver = 0;
	if (playername != "") {
		RemotePlayer *player = m_env->getPlayer(playername.c_str());
		if (!player)
			return -1;
		peer_id = player->peer_id;
		proto_ver = player->protocol_version;
	}

	u16 attached_id = attached ? attached->getId() : 0;

	u32 id;
	if (attached_id == 0)
		id = m_env->addParticleSpawner(spawntime);
	else
		id = m_env->addParticleSpawner(spawntime, attached_id);

	SendAddParticleSpawner(peer_id, proto_ver, amount, spawntime,
		minpos, maxpos, minvel, maxvel, minacc, maxacc,
		minexptime, maxexptime, minsize, maxsize,
		collisiondetection, collision_removal, attached_id, vertical,
		texture, id, animation, glow);

	return id;
}

void Server::deleteParticleSpawner(const std::string &playername, u32 id)
{
	// m_env will be NULL if the server is initializing
	if (!m_env)
		throw ServerError("Can't delete particle spawners during initialisation!");

	u16 peer_id = PEER_ID_INEXISTENT;
	if (playername != "") {
		RemotePlayer *player = m_env->getPlayer(playername.c_str());
		if (!player)
			return;
		peer_id = player->peer_id;
	}

	m_env->deleteParticleSpawner(id);
	SendDeleteParticleSpawner(peer_id, id);
}

Inventory* Server::createDetachedInventory(const std::string &name, const std::string &player)
{
	if(m_detached_inventories.count(name) > 0){
		infostream<<"Server clearing detached inventory \""<<name<<"\""<<std::endl;
		delete m_detached_inventories[name];
	} else {
		infostream<<"Server creating detached inventory \""<<name<<"\""<<std::endl;
	}
	Inventory *inv = new Inventory(m_itemdef);
	sanity_check(inv);
	m_detached_inventories[name] = inv;
	m_detached_inventories_player[name] = player;
	//TODO find a better way to do this
	sendDetachedInventory(name,PEER_ID_INEXISTENT);
	return inv;
}

// actions: time-reversed list
// Return value: success/failure
bool Server::rollbackRevertActions(const std::list<RollbackAction> &actions,
		std::list<std::string> *log)
{
	infostream<<"Server::rollbackRevertActions(len="<<actions.size()<<")"<<std::endl;
	ServerMap *map = (ServerMap*)(&m_env->getMap());

	// Fail if no actions to handle
	if(actions.empty()){
		log->push_back("Nothing to do.");
		return false;
	}

	int num_tried = 0;
	int num_failed = 0;

	for(std::list<RollbackAction>::const_iterator
			i = actions.begin();
			i != actions.end(); ++i)
	{
		const RollbackAction &action = *i;
		num_tried++;
		bool success = action.applyRevert(map, this, this);
		if(!success){
			num_failed++;
			std::ostringstream os;
			os<<"Revert of step ("<<num_tried<<") "<<action.toString()<<" failed";
			infostream<<"Map::rollbackRevertActions(): "<<os.str()<<std::endl;
			if(log)
				log->push_back(os.str());
		}else{
			std::ostringstream os;
			os<<"Successfully reverted step ("<<num_tried<<") "<<action.toString();
			infostream<<"Map::rollbackRevertActions(): "<<os.str()<<std::endl;
			if(log)
				log->push_back(os.str());
		}
	}

	infostream<<"Map::rollbackRevertActions(): "<<num_failed<<"/"<<num_tried
			<<" failed"<<std::endl;

	// Call it done if less than half failed
	return num_failed <= num_tried/2;
}

// IGameDef interface
// Under envlock
IItemDefManager *Server::getItemDefManager()
{
	return m_itemdef;
}

INodeDefManager *Server::getNodeDefManager()
{
	return m_nodedef;
}

ICraftDefManager *Server::getCraftDefManager()
{
	return m_craftdef;
}

u16 Server::allocateUnknownNodeId(const std::string &name)
{
	return m_nodedef->allocateDummy(name);
}

MtEventManager *Server::getEventManager()
{
	return m_event;
}

IWritableItemDefManager *Server::getWritableItemDefManager()
{
	return m_itemdef;
}

IWritableNodeDefManager *Server::getWritableNodeDefManager()
{
	return m_nodedef;
}

IWritableCraftDefManager *Server::getWritableCraftDefManager()
{
	return m_craftdef;
}

const ModSpec *Server::getModSpec(const std::string &modname) const
{
	std::vector<ModSpec>::const_iterator it;
	for (it = m_mods.begin(); it != m_mods.end(); ++it) {
		const ModSpec &mod = *it;
		if (mod.name == modname)
			return &mod;
	}
	return NULL;
}

void Server::getModNames(std::vector<std::string> &modlist)
{
	std::vector<ModSpec>::iterator it;
	for (it = m_mods.begin(); it != m_mods.end(); ++it)
		modlist.push_back(it->name);
}

std::string Server::getBuiltinLuaPath()
{
	return porting::path_share + DIR_DELIM + "builtin";
}

std::string Server::getModStoragePath() const
{
	return m_path_world + DIR_DELIM + "mod_storage";
}

v3f Server::findSpawnPos()
{
	ServerMap &map = m_env->getServerMap();
	v3f nodeposf;
	if (g_settings->getV3FNoEx("static_spawnpoint", nodeposf)) {
		return nodeposf * BS;
	}

	bool is_good = false;
	// Limit spawn range to mapgen edges (determined by 'mapgen_limit')
	s32 range_max = map.getMapgenParams()->getSpawnRangeMax();

	// Try to find a good place a few times
	for(s32 i = 0; i < 4000 && !is_good; i++) {
		s32 range = MYMIN(1 + i, range_max);
		// We're going to try to throw the player to this position
		v2s16 nodepos2d = v2s16(
			-range + (myrand() % (range * 2)),
			-range + (myrand() % (range * 2)));

		// Get spawn level at point
		s16 spawn_level = m_emerge->getSpawnLevelAtPoint(nodepos2d);
		// Continue if MAX_MAP_GENERATION_LIMIT was returned by
		// the mapgen to signify an unsuitable spawn position
		if (spawn_level == MAX_MAP_GENERATION_LIMIT)
			continue;

		v3s16 nodepos(nodepos2d.X, spawn_level, nodepos2d.Y);

		s32 air_count = 0;
		for (s32 i = 0; i < 10; i++) {
			v3s16 blockpos = getNodeBlockPos(nodepos);
			map.emergeBlock(blockpos, true);
			content_t c = map.getNodeNoEx(nodepos).getContent();
			if (c == CONTENT_AIR || c == CONTENT_IGNORE) {
				air_count++;
				if (air_count >= 2) {
					nodeposf = intToFloat(nodepos, BS);
					// Don't spawn the player outside map boundaries
					if (objectpos_over_limit(nodeposf))
						continue;
					is_good = true;
					break;
				}
			}
			nodepos.Y++;
		}
	}

	return nodeposf;
}

void Server::requestShutdown(const std::string &msg, bool reconnect, float delay)
{
	m_shutdown_timer = delay;
	m_shutdown_msg = msg;
	m_shutdown_ask_reconnect = reconnect;

	if (delay == 0.0f) {
	// No delay, shutdown immediately
		m_shutdown_requested = true;
		// only print to the infostream, a chat message saying 
		// "Server Shutting Down" is sent when the server destructs.
		infostream << "*** Immediate Server shutdown requested." << std::endl;
	} else if (delay < 0.0f && m_shutdown_timer > 0.0f) {
	// Negative delay, cancel shutdown if requested
		m_shutdown_timer = 0.0f;
		m_shutdown_msg = "";
		m_shutdown_ask_reconnect = false;
		m_shutdown_requested = false;
		std::wstringstream ws;

		ws << L"*** Server shutdown canceled.";

		infostream << wide_to_utf8(ws.str()).c_str() << std::endl;
		SendChatMessage(PEER_ID_INEXISTENT, ws.str());
	} else if (delay > 0.0f) {
	// Positive delay, tell the clients when the server will shut down
		std::wstringstream ws;

		ws << L"*** Server shutting down in "
				<< duration_to_string(myround(m_shutdown_timer)).c_str()
				<< ".";

		infostream << wide_to_utf8(ws.str()).c_str() << std::endl;
		SendChatMessage(PEER_ID_INEXISTENT, ws.str());
	}
}

PlayerSAO* Server::emergePlayer(const char *name, u16 peer_id, u16 proto_version)
{
	/*
		Try to get an existing player
	*/
	RemotePlayer *player = m_env->getPlayer(name);

	// If player is already connected, cancel
	if (player != NULL && player->peer_id != 0) {
		infostream<<"emergePlayer(): Player already connected"<<std::endl;
		return NULL;
	}

	/*
		If player with the wanted peer_id already exists, cancel.
	*/
	if (m_env->getPlayer(peer_id) != NULL) {
		infostream<<"emergePlayer(): Player with wrong name but same"
				" peer_id already exists"<<std::endl;
		return NULL;
	}

	if (!player) {
		player = new RemotePlayer(name, idef());
	}

	bool newplayer = false;

	// Load player
	PlayerSAO *playersao = m_env->loadPlayer(player, &newplayer, peer_id, isSingleplayer());

	// Complete init with server parts
	playersao->finalize(player, getPlayerEffectivePrivs(player->getName()));
	player->protocol_version = proto_version;

	/* Run scripts */
	if (newplayer) {
		m_script->on_newplayer(playersao);
	}

	return playersao;
}

bool Server::registerModStorage(ModMetadata *storage)
{
	if (m_mod_storages.find(storage->getModName()) != m_mod_storages.end()) {
		errorstream << "Unable to register same mod storage twice. Storage name: "
				<< storage->getModName() << std::endl;
		return false;
	}

	m_mod_storages[storage->getModName()] = storage;
	return true;
}

void Server::unregisterModStorage(const std::string &name)
{
	UNORDERED_MAP<std::string, ModMetadata *>::const_iterator it = m_mod_storages.find(name);
	if (it != m_mod_storages.end()) {
		// Save unconditionaly on unregistration
		it->second->save(getModStoragePath());
		m_mod_storages.erase(name);
	}
}

void dedicated_server_loop(Server &server, bool &kill)
{
	DSTACK(FUNCTION_NAME);

	verbosestream<<"dedicated_server_loop()"<<std::endl;

	IntervalLimiter m_profiler_interval;

	static const float steplen = g_settings->getFloat("dedicated_server_step");
	static const float profiler_print_interval =
			g_settings->getFloat("profiler_print_interval");

	for(;;) {
		// This is kind of a hack but can be done like this
		// because server.step() is very light
		{
			ScopeProfiler sp(g_profiler, "dedicated server sleep");
			sleep_ms((int)(steplen*1000.0));
		}
		server.step(steplen);

		if (server.getShutdownRequested() || kill)
			break;

		/*
			Profiler
		*/
		if (profiler_print_interval != 0) {
			if(m_profiler_interval.step(steplen, profiler_print_interval))
			{
				infostream<<"Profiler:"<<std::endl;
				g_profiler->print(infostream);
				g_profiler->clear();
			}
		}
	}

	infostream << "Dedicated server quitting" << std::endl;
#if USE_CURL
	if (g_settings->getBool("server_announce"))
		ServerList::sendAnnounce(ServerList::AA_DELETE,
			server.m_bind_addr.getPort());
#endif
}
