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
#include "clientserver.h"
#include "ban.h"
#include "environment.h"
#include "map.h"
#include "jthread/jmutexautolock.h"
#include "main.h"
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
#include "scripting_game.h"
#include "nodedef.h"
#include "itemdef.h"
#include "craftdef.h"
#include "emerge.h"
#include "mapgen.h"
#include "biome.h"
#include "content_mapnode.h"
#include "content_nodemeta.h"
#include "content_abm.h"
#include "content_sao.h"
#include "mods.h"
#include "sha1.h"
#include "base64.h"
#include "tool.h"
#include "sound.h" // dummySoundManager
#include "event_manager.h"
#include "hex.h"
#include "serverlist.h"
#include "util/string.h"
#include "util/pointedthing.h"
#include "util/mathconstants.h"
#include "rollback.h"
#include "util/serialize.h"
#include "util/thread.h"
#include "defaultsettings.h"

class ClientNotFoundException : public BaseException
{
public:
	ClientNotFoundException(const char *s):
		BaseException(s)
	{}
};

class ServerThread : public JThread
{
	Server *m_server;

public:

	ServerThread(Server *server):
		JThread(),
		m_server(server)
	{
	}

	void * Thread();
};

struct SendableMediaAnnouncement
{
	std::string name;
	std::string sha1_digest;

	SendableMediaAnnouncement(const std::string name_="",
			const std::string sha1_digest_=""):
		name(name_),
		sha1_digest(sha1_digest_)
	{}
};

struct SendableMedia
{
	std::string name;
	std::string path;
	std::string data;

	SendableMedia(const std::string &name_="", const std::string path_="",
			const std::string &data_=""):
		name(name_),
		path(path_),
		data(data_)
	{}
};

class BoolScopeSet
{
public:
	BoolScopeSet(bool *dst, bool val):
		m_dst(dst)
	{
		m_orig_state = *m_dst;
		*m_dst = val;
	}
	~BoolScopeSet()
	{
		*m_dst = m_orig_state;
	}
private:
	bool *m_dst;
	bool m_orig_state;
};

void * ServerThread::Thread()
{
	ThreadStarted();

	log_register_thread("ServerThread");

	DSTACK(__FUNCTION_NAME);

	BEGIN_DEBUG_EXCEPTION_HANDLER

	while(!StopRequested())
	{
		try{
			//TimeTaker timer("AsyncRunStep() + Receive()");

			{
				//TimeTaker timer("AsyncRunStep()");
				m_server->AsyncRunStep();
			}

			//infostream<<"Running m_server->Receive()"<<std::endl;
			m_server->Receive();
		}
		catch(con::NoIncomingDataException &e)
		{
		}
		catch(con::PeerNotFoundException &e)
		{
			infostream<<"Server: PeerNotFoundException"<<std::endl;
		}
		catch(ClientNotFoundException &e)
		{
		}
		catch(con::ConnectionBindFailed &e)
		{
			m_server->setAsyncFatalError(e.what());
		}
		catch(LuaError &e)
		{
			m_server->setAsyncFatalError(e.what());
		}
	}

	END_DEBUG_EXCEPTION_HANDLER(errorstream)

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
		bool simple_singleplayer_mode
	):
	m_path_world(path_world),
	m_gamespec(gamespec),
	m_simple_singleplayer_mode(simple_singleplayer_mode),
	m_async_fatal_error(""),
	m_env(NULL),
	m_con(PROTOCOL_ID, 512, CONNECTION_TIMEOUT,
	      g_settings->getBool("enable_ipv6") && g_settings->getBool("ipv6_server"), this),
	m_banmanager(NULL),
	m_rollback(NULL),
	m_rollback_sink_enabled(true),
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
	m_shutdown_requested(false),
	m_ignore_map_edit_events(false),
	m_ignore_map_edit_events_peer_id(0)
{
	m_liquid_transform_timer = 0.0;
	m_liquid_transform_every = 1.0;
	m_print_info_timer = 0.0;
	m_masterserver_timer = 0.0;
	m_objectdata_timer = 0.0;
	m_emergethread_trigger_timer = 0.0;
	m_savemap_timer = 0.0;

	m_step_dtime = 0.0;

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

	// Initialize default settings and override defaults with those provided
	// by the game
	set_default_settings(g_settings);
	Settings gamedefaults;
	getGameMinetestConfig(gamespec.path, gamedefaults);
	override_default_settings(g_settings, &gamedefaults);

	// Create server thread
	m_thread = new ServerThread(this);

	// Create emerge manager
	m_emerge = new EmergeManager(this);

	// Create ban manager
	std::string ban_path = m_path_world+DIR_DELIM+"ipban.txt";
	m_banmanager = new BanManager(ban_path);

	// Create rollback manager
	std::string rollback_path = m_path_world+DIR_DELIM+"rollback.txt";
	m_rollback = createRollbackManager(rollback_path, this);

	// Create world if it doesn't exist
	if(!initializeWorld(m_path_world, m_gamespec.id))
		throw ServerError("Failed to initialize world");

	ModConfiguration modconf(m_path_world);
	m_mods = modconf.getMods();
	std::vector<ModSpec> unsatisfied_mods = modconf.getUnsatisfiedMods();
	// complain about mods with unsatisfied dependencies
	if(!modconf.isConsistent())
	{
		for(std::vector<ModSpec>::iterator it = unsatisfied_mods.begin();
			it != unsatisfied_mods.end(); ++it)
		{
			ModSpec mod = *it;
			errorstream << "mod \"" << mod.name << "\" has unsatisfied dependencies: ";
			for(std::set<std::string>::iterator dep_it = mod.unsatisfied_depends.begin();
				dep_it != mod.unsatisfied_depends.end(); ++dep_it)
				errorstream << " \"" << *dep_it << "\"";
			errorstream << std::endl;
		}
	}

	Settings worldmt_settings;
	std::string worldmt = m_path_world + DIR_DELIM + "world.mt";
	worldmt_settings.readConfigFile(worldmt.c_str());
	std::vector<std::string> names = worldmt_settings.getNames();
	std::set<std::string> load_mod_names;
	for(std::vector<std::string>::iterator it = names.begin();
		it != names.end(); ++it)
	{
		std::string name = *it;
		if(name.compare(0,9,"load_mod_")==0 && worldmt_settings.getBool(name))
			load_mod_names.insert(name.substr(9));
	}
	// complain about mods declared to be loaded, but not found
	for(std::vector<ModSpec>::iterator it = m_mods.begin();
			it != m_mods.end(); ++it)
		load_mod_names.erase((*it).name);
	for(std::vector<ModSpec>::iterator it = unsatisfied_mods.begin();
			it != unsatisfied_mods.end(); ++it)
		load_mod_names.erase((*it).name);
	if(!load_mod_names.empty())
	{
		errorstream << "The following mods could not be found:";
		for(std::set<std::string>::iterator it = load_mod_names.begin();
			it != load_mod_names.end(); ++it)
			errorstream << " \"" << (*it) << "\"";
		errorstream << std::endl;
	}

	// Path to builtin.lua
	std::string builtinpath = getBuiltinLuaPath() + DIR_DELIM + "builtin.lua";

	// Lock environment
	JMutexAutoLock envlock(m_env_mutex);
	JMutexAutoLock conlock(m_con_mutex);

	// Initialize scripting

	infostream<<"Server: Initializing Lua"<<std::endl;

	m_script = new GameScripting(this);


	// Load and run builtin.lua
	infostream<<"Server: Loading builtin.lua [\""
			<<builtinpath<<"\"]"<<std::endl;
	bool success = m_script->loadMod(builtinpath, "__builtin");
	if(!success){
		errorstream<<"Server: Failed to load and run "
				<<builtinpath<<std::endl;
		throw ModError("Failed to load and run "+builtinpath);
	}
	// Print 'em
	infostream<<"Server: Loading mods: ";
	for(std::vector<ModSpec>::iterator i = m_mods.begin();
			i != m_mods.end(); i++){
		const ModSpec &mod = *i;
		infostream<<mod.name<<" ";
	}
	infostream<<std::endl;
	// Load and run "mod" scripts
	for(std::vector<ModSpec>::iterator i = m_mods.begin();
			i != m_mods.end(); i++){
		const ModSpec &mod = *i;
		std::string scriptpath = mod.path + DIR_DELIM + "init.lua";
		infostream<<"  ["<<padStringRight(mod.name, 12)<<"] [\""
				<<scriptpath<<"\"]"<<std::endl;
		bool success = m_script->loadMod(scriptpath, mod.name);
		if(!success){
			errorstream<<"Server: Failed to load and run "
					<<scriptpath<<std::endl;
			throw ModError("Failed to load and run "+scriptpath);
		}
	}

	// Read Textures and calculate sha1 sums
	fillMediaCache();

	// Apply item aliases in the node definition manager
	m_nodedef->updateAliases(m_itemdef);

	// Initialize Environment
	ServerMap *servermap = new ServerMap(path_world, this, m_emerge);
	m_env = new ServerEnvironment(servermap, m_script, this, m_emerge);
	
	// Run some callbacks after the MG params have been set up but before activation
	MapgenParams *mgparams = servermap->getMapgenParams();
	m_script->environment_OnMapgenInit(mgparams);
	
	// Initialize mapgens
	m_emerge->initMapgens(mgparams);
	servermap->setMapgenParams(m_emerge->params);

	// Give environment reference to scripting api
	m_script->initializeEnvironment(m_env);

	// Register us to receive map edit events
	servermap->addEventReceiver(this);

	// If file exists, load environment metadata
	if(fs::PathExists(m_path_world+DIR_DELIM+"env_meta.txt"))
	{
		infostream<<"Server: Loading environment metadata"<<std::endl;
		m_env->loadMeta(m_path_world);
	}

	// Load players
	infostream<<"Server: Loading players"<<std::endl;
	m_env->deSerializePlayers(m_path_world);

	/*
		Add some test ActiveBlockModifiers to environment
	*/
	add_legacy_abms(m_env, m_nodedef);

	m_liquid_transform_every = g_settings->getFloat("liquid_update");
}

Server::~Server()
{
	infostream<<"Server destructing"<<std::endl;

	/*
		Send shutdown message
	*/
	{
		JMutexAutoLock conlock(m_con_mutex);

		std::wstring line = L"*** Server shutting down";

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

			try{
				SendChatMessage(client->peer_id, line);
			}
			catch(con::PeerNotFoundException &e)
			{}
		}
	}

	{
		JMutexAutoLock envlock(m_env_mutex);
		JMutexAutoLock conlock(m_con_mutex);

		/*
			Execute script shutdown hooks
		*/
		m_script->on_shutdown();
	}

	{
		JMutexAutoLock envlock(m_env_mutex);

		/*
			Save players
		*/
		infostream<<"Server: Saving players"<<std::endl;
		m_env->serializePlayers(m_path_world);

		/*
			Save environment metadata
		*/
		infostream<<"Server: Saving environment metadata"<<std::endl;
		m_env->saveMeta(m_path_world);
	}

	/*
		Stop threads
	*/
	stop();
	delete m_thread;

	//shutdown all emerge threads first!
	delete m_emerge;

	/*
		Delete clients
	*/
	{
		JMutexAutoLock clientslock(m_con_mutex);

		for(std::map<u16, RemoteClient*>::iterator
			i = m_clients.begin();
			i != m_clients.end(); ++i)
		{

			// Delete client
			delete i->second;
		}
	}

	// Delete things in the reverse order of creation
	delete m_env;
	delete m_rollback;
	delete m_banmanager;
	delete m_event;
	delete m_itemdef;
	delete m_nodedef;
	delete m_craftdef;

	// Deinitialize scripting
	infostream<<"Server: Deinitializing scripting"<<std::endl;
	delete m_script;

	// Delete detached inventories
	{
		for(std::map<std::string, Inventory*>::iterator
				i = m_detached_inventories.begin();
				i != m_detached_inventories.end(); i++){
			delete i->second;
		}
	}
}

void Server::start(unsigned short port)
{
	DSTACK(__FUNCTION_NAME);
	infostream<<"Starting server on port "<<port<<"..."<<std::endl;

	// Stop thread if already running
	m_thread->Stop();

	// Initialize connection
	m_con.SetTimeoutMs(30);
	m_con.Serve(port);

	// Start thread
	m_thread->Start();

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
			<<"\" listening on port "<<port<<"."<<std::endl;
}

void Server::stop()
{
	DSTACK(__FUNCTION_NAME);

	infostream<<"Server: Stopping and waiting threads"<<std::endl;

	// Stop threads (set run=false first so both start stopping)
	m_thread->Stop();
	//m_emergethread.setRun(false);
	m_thread->Wait();
	//m_emergethread.stop();

	infostream<<"Server: Threads stopped"<<std::endl;
}

void Server::step(float dtime)
{
	DSTACK(__FUNCTION_NAME);
	// Limit a bit
	if(dtime > 2.0)
		dtime = 2.0;
	{
		JMutexAutoLock lock(m_step_dtime_mutex);
		m_step_dtime += dtime;
	}
	// Throw if fatal error occurred in thread
	std::string async_err = m_async_fatal_error.get();
	if(async_err != ""){
		throw ServerError(async_err);
	}
}

void Server::AsyncRunStep()
{
	DSTACK(__FUNCTION_NAME);

	g_profiler->add("Server::AsyncRunStep (num)", 1);

	float dtime;
	{
		JMutexAutoLock lock1(m_step_dtime_mutex);
		dtime = m_step_dtime;
	}

	{
		// Send blocks to clients
		SendBlocks(dtime);
	}

	if(dtime < 0.001)
		return;

	g_profiler->add("Server::AsyncRunStep with dtime (num)", 1);

	//infostream<<"Server steps "<<dtime<<std::endl;
	//infostream<<"Server::AsyncRunStep(): dtime="<<dtime<<std::endl;

	{
		JMutexAutoLock lock1(m_step_dtime_mutex);
		m_step_dtime -= dtime;
	}

	/*
		Update uptime
	*/
	{
		m_uptime.set(m_uptime.get() + dtime);
	}

	{
		// Process connection's timeouts
		JMutexAutoLock lock2(m_con_mutex);
		ScopeProfiler sp(g_profiler, "Server: connection timeout processing");
		m_con.RunTimeouts(dtime);
	}

	{
		// This has to be called so that the client list gets synced
		// with the peer list of the connection
		handlePeerChanges();
	}

	/*
		Update time of day and overall game time
	*/
	{
		JMutexAutoLock envlock(m_env_mutex);

		m_env->setTimeOfDaySpeed(g_settings->getFloat("time_speed"));

		/*
			Send to clients at constant intervals
		*/

		m_time_of_day_send_timer -= dtime;
		if(m_time_of_day_send_timer < 0.0)
		{
			m_time_of_day_send_timer = g_settings->getFloat("time_send_interval");

			//JMutexAutoLock envlock(m_env_mutex);
			JMutexAutoLock conlock(m_con_mutex);

			u16 time = m_env->getTimeOfDay();
			float time_speed = g_settings->getFloat("time_speed");

			for(std::map<u16, RemoteClient*>::iterator
				i = m_clients.begin();
				i != m_clients.end(); ++i)
			{
				RemoteClient *client = i->second;
				SendTimeOfDay(client->peer_id, time, time_speed);
			}
		}
	}

	{
		JMutexAutoLock lock(m_env_mutex);
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

	const float map_timer_and_unload_dtime = 2.92;
	if(m_map_timer_and_unload_interval.step(dtime, map_timer_and_unload_dtime))
	{
		JMutexAutoLock lock(m_env_mutex);
		// Run Map's timers and unload unused data
		ScopeProfiler sp(g_profiler, "Server: map timer and unload");
		m_env->getMap().timerUpdate(map_timer_and_unload_dtime,
				g_settings->getFloat("server_unload_unused_data_timeout"));
	}

	/*
		Do background stuff
	*/

	/*
		Handle players
	*/
	{
		JMutexAutoLock lock(m_env_mutex);
		JMutexAutoLock lock2(m_con_mutex);

		ScopeProfiler sp(g_profiler, "Server: handle players");

		for(std::map<u16, RemoteClient*>::iterator
			i = m_clients.begin();
			i != m_clients.end(); ++i)
		{
			RemoteClient *client = i->second;
			PlayerSAO *playersao = getPlayerSAO(client->peer_id);
			if(playersao == NULL)
				continue;

			/*
				Handle player HPs (die if hp=0)
			*/
			if(playersao->m_hp_not_sent && g_settings->getBool("enable_damage"))
			{
				if(playersao->getHP() == 0)
					DiePlayer(client->peer_id);
				else
					SendPlayerHP(client->peer_id);
			}

			/*
				Send player breath if changed
			*/
			if(playersao->m_breath_not_sent){
				SendPlayerBreath(client->peer_id);
			}

			/*
				Send player inventories if necessary
			*/
			if(playersao->m_moved){
				SendMovePlayer(client->peer_id);
				playersao->m_moved = false;
			}
			if(playersao->m_inventory_not_sent){
				UpdateCrafting(client->peer_id);
				SendInventory(client->peer_id);
			}
		}
	}

	/* Transform liquids */
	m_liquid_transform_timer += dtime;
	if(m_liquid_transform_timer >= m_liquid_transform_every)
	{
		m_liquid_transform_timer -= m_liquid_transform_every;

		JMutexAutoLock lock(m_env_mutex);

		ScopeProfiler sp(g_profiler, "Server: liquid transform");

		std::map<v3s16, MapBlock*> modified_blocks;
		m_env->getMap().transformLiquids(modified_blocks);

		/*
			Set the modified blocks unsent for all the clients
		*/

		JMutexAutoLock lock2(m_con_mutex);

		for(std::map<u16, RemoteClient*>::iterator
				i = m_clients.begin();
				i != m_clients.end(); ++i)
		{
			RemoteClient *client = i->second;

			if(modified_blocks.size() > 0)
			{
				// Remove block from sent history
				client->SetBlocksNotSent(modified_blocks);
			}
		}
	}

	// Periodically print some info
	{
		float &counter = m_print_info_timer;
		counter += dtime;
		if(counter >= 30.0)
		{
			counter = 0.0;

			JMutexAutoLock lock2(m_con_mutex);
			m_clients_names.clear();
			if(m_clients.size() != 0)
				infostream<<"Players:"<<std::endl;
			for(std::map<u16, RemoteClient*>::iterator
				i = m_clients.begin();
				i != m_clients.end(); ++i)
			{
				//u16 peer_id = i.getNode()->getKey();
				RemoteClient *client = i->second;
				Player *player = m_env->getPlayer(client->peer_id);
				if(player==NULL)
					continue;
				infostream<<"* "<<player->getName()<<"\t";
				client->PrintInfo(infostream);
				m_clients_names.push_back(player->getName());
			}
		}
	}


#if USE_CURL
	// send masterserver announce
	{
		float &counter = m_masterserver_timer;
		if(!isSingleplayer() && (!counter || counter >= 300.0) && g_settings->getBool("server_announce") == true)
		{
			ServerList::sendAnnounce(!counter ? "start" : "update", m_clients_names, m_uptime.get(), m_env->getGameTime(), m_gamespec.id, m_mods);
			counter = 0.01;
		}
		counter += dtime;
	}
#endif

	//if(g_settings->getBool("enable_experimental"))
	{

	/*
		Check added and deleted active objects
	*/
	{
		//infostream<<"Server: Checking added and deleted active objects"<<std::endl;
		JMutexAutoLock envlock(m_env_mutex);
		JMutexAutoLock conlock(m_con_mutex);

		ScopeProfiler sp(g_profiler, "Server: checking added and deleted objs");

		// Radius inside which objects are active
		s16 radius = g_settings->getS16("active_object_send_range_blocks");
		radius *= MAP_BLOCKSIZE;

		for(std::map<u16, RemoteClient*>::iterator
			i = m_clients.begin();
			i != m_clients.end(); ++i)
		{
			RemoteClient *client = i->second;

			// If definitions and textures have not been sent, don't
			// send objects either
			if(!client->definitions_sent)
				continue;

			Player *player = m_env->getPlayer(client->peer_id);
			if(player==NULL)
			{
				// This can happen if the client timeouts somehow
				/*infostream<<"WARNING: "<<__FUNCTION_NAME<<": Client "
						<<client->peer_id
						<<" has no associated player"<<std::endl;*/
				continue;
			}
			v3s16 pos = floatToInt(player->getPosition(), BS);

			std::set<u16> removed_objects;
			std::set<u16> added_objects;
			m_env->getRemovedActiveObjects(pos, radius,
					client->m_known_objects, removed_objects);
			m_env->getAddedActiveObjects(pos, radius,
					client->m_known_objects, added_objects);

			// Ignore if nothing happened
			if(removed_objects.size() == 0 && added_objects.size() == 0)
			{
				//infostream<<"active objects: none changed"<<std::endl;
				continue;
			}

			std::string data_buffer;

			char buf[4];

			// Handle removed objects
			writeU16((u8*)buf, removed_objects.size());
			data_buffer.append(buf, 2);
			for(std::set<u16>::iterator
					i = removed_objects.begin();
					i != removed_objects.end(); ++i)
			{
				// Get object
				u16 id = *i;
				ServerActiveObject* obj = m_env->getActiveObject(id);

				// Add to data buffer for sending
				writeU16((u8*)buf, id);
				data_buffer.append(buf, 2);

				// Remove from known objects
				client->m_known_objects.erase(id);

				if(obj && obj->m_known_by_count > 0)
					obj->m_known_by_count--;
			}

			// Handle added objects
			writeU16((u8*)buf, added_objects.size());
			data_buffer.append(buf, 2);
			for(std::set<u16>::iterator
					i = added_objects.begin();
					i != added_objects.end(); ++i)
			{
				// Get object
				u16 id = *i;
				ServerActiveObject* obj = m_env->getActiveObject(id);

				// Get object type
				u8 type = ACTIVEOBJECT_TYPE_INVALID;
				if(obj == NULL)
					infostream<<"WARNING: "<<__FUNCTION_NAME
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
			}

			// Send packet
			SharedBuffer<u8> reply(2 + data_buffer.size());
			writeU16(&reply[0], TOCLIENT_ACTIVE_OBJECT_REMOVE_ADD);
			memcpy((char*)&reply[2], data_buffer.c_str(),
					data_buffer.size());
			// Send as reliable
			m_con.Send(client->peer_id, 0, reply, true);

			verbosestream<<"Server: Sent object remove/add: "
					<<removed_objects.size()<<" removed, "
					<<added_objects.size()<<" added, "
					<<"packet size is "<<reply.getSize()<<std::endl;
		}
	}

	/*
		Send object messages
	*/
	{
		JMutexAutoLock envlock(m_env_mutex);
		JMutexAutoLock conlock(m_con_mutex);

		ScopeProfiler sp(g_profiler, "Server: sending object messages");

		// Key = object id
		// Value = data sent by object
		std::map<u16, std::list<ActiveObjectMessage>* > buffered_messages;

		// Get active object messages from environment
		for(;;)
		{
			ActiveObjectMessage aom = m_env->getActiveObjectMessage();
			if(aom.id == 0)
				break;

			std::list<ActiveObjectMessage>* message_list = NULL;
			std::map<u16, std::list<ActiveObjectMessage>* >::iterator n;
			n = buffered_messages.find(aom.id);
			if(n == buffered_messages.end())
			{
				message_list = new std::list<ActiveObjectMessage>;
				buffered_messages[aom.id] = message_list;
			}
			else
			{
				message_list = n->second;
			}
			message_list->push_back(aom);
		}

		// Route data to every client
		for(std::map<u16, RemoteClient*>::iterator
			i = m_clients.begin();
			i != m_clients.end(); ++i)
		{
			RemoteClient *client = i->second;
			std::string reliable_data;
			std::string unreliable_data;
			// Go through all objects in message buffer
			for(std::map<u16, std::list<ActiveObjectMessage>* >::iterator
					j = buffered_messages.begin();
					j != buffered_messages.end(); ++j)
			{
				// If object is not known by client, skip it
				u16 id = j->first;
				if(client->m_known_objects.find(id) == client->m_known_objects.end())
					continue;
				// Get message list of object
				std::list<ActiveObjectMessage>* list = j->second;
				// Go through every message
				for(std::list<ActiveObjectMessage>::iterator
						k = list->begin(); k != list->end(); ++k)
				{
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
			if(reliable_data.size() > 0)
			{
				SharedBuffer<u8> reply(2 + reliable_data.size());
				writeU16(&reply[0], TOCLIENT_ACTIVE_OBJECT_MESSAGES);
				memcpy((char*)&reply[2], reliable_data.c_str(),
						reliable_data.size());
				// Send as reliable
				m_con.Send(client->peer_id, 0, reply, true);
			}
			if(unreliable_data.size() > 0)
			{
				SharedBuffer<u8> reply(2 + unreliable_data.size());
				writeU16(&reply[0], TOCLIENT_ACTIVE_OBJECT_MESSAGES);
				memcpy((char*)&reply[2], unreliable_data.c_str(),
						unreliable_data.size());
				// Send as unreliable
				m_con.Send(client->peer_id, 0, reply, false);
			}

			/*if(reliable_data.size() > 0 || unreliable_data.size() > 0)
			{
				infostream<<"Server: Size of object message data: "
						<<"reliable: "<<reliable_data.size()
						<<", unreliable: "<<unreliable_data.size()
						<<std::endl;
			}*/
		}

		// Clear buffered_messages
		for(std::map<u16, std::list<ActiveObjectMessage>* >::iterator
				i = buffered_messages.begin();
				i != buffered_messages.end(); ++i)
		{
			delete i->second;
		}
	}

	} // enable_experimental

	/*
		Send queued-for-sending map edit events.
	*/
	{
		// We will be accessing the environment and the connection
		JMutexAutoLock lock(m_env_mutex);
		JMutexAutoLock conlock(m_con_mutex);

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
			MapEditEvent* event = m_unsent_map_edit_queue.pop_front();

			// Players far away from the change are stored here.
			// Instead of sending the changes, MapBlocks are set not sent
			// for them.
			std::list<u16> far_players;

			if(event->type == MEET_ADDNODE || event->type == MEET_SWAPNODE)
			{
				//infostream<<"Server: MEET_ADDNODE"<<std::endl;
				prof.add("MEET_ADDNODE", 1);
				if(disable_single_change_sending)
					sendAddNode(event->p, event->n, event->already_known_by_peer,
							&far_players, 5, event->type == MEET_ADDNODE);
				else
					sendAddNode(event->p, event->n, event->already_known_by_peer,
							&far_players, 30, event->type == MEET_ADDNODE);
			}
			else if(event->type == MEET_REMOVENODE)
			{
				//infostream<<"Server: MEET_REMOVENODE"<<std::endl;
				prof.add("MEET_REMOVENODE", 1);
				if(disable_single_change_sending)
					sendRemoveNode(event->p, event->already_known_by_peer,
							&far_players, 5);
				else
					sendRemoveNode(event->p, event->already_known_by_peer,
							&far_players, 30);
			}
			else if(event->type == MEET_BLOCK_NODE_METADATA_CHANGED)
			{
				infostream<<"Server: MEET_BLOCK_NODE_METADATA_CHANGED"<<std::endl;
				prof.add("MEET_BLOCK_NODE_METADATA_CHANGED", 1);
				setBlockNotSent(event->p);
			}
			else if(event->type == MEET_OTHER)
			{
				infostream<<"Server: MEET_OTHER"<<std::endl;
				prof.add("MEET_OTHER", 1);
				for(std::set<v3s16>::iterator
						i = event->modified_blocks.begin();
						i != event->modified_blocks.end(); ++i)
				{
					setBlockNotSent(*i);
				}
			}
			else
			{
				prof.add("unknown", 1);
				infostream<<"WARNING: Server: Unknown MapEditEvent "
						<<((u32)event->type)<<std::endl;
			}

			/*
				Set blocks not sent to far players
			*/
			if(far_players.size() > 0)
			{
				// Convert list format to that wanted by SetBlocksNotSent
				std::map<v3s16, MapBlock*> modified_blocks2;
				for(std::set<v3s16>::iterator
						i = event->modified_blocks.begin();
						i != event->modified_blocks.end(); ++i)
				{
					modified_blocks2[*i] =
							m_env->getMap().getBlockNoCreateNoEx(*i);
				}
				// Set blocks not sent
				for(std::list<u16>::iterator
						i = far_players.begin();
						i != far_players.end(); ++i)
				{
					u16 peer_id = *i;
					RemoteClient *client = getClient(peer_id);
					if(client==NULL)
						continue;
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
		if(counter >= 2.0)
		{
			counter = 0.0;

			m_emerge->startAllThreads();

			// Update m_enable_rollback_recording here too
			m_enable_rollback_recording =
					g_settings->getBool("enable_rollback_recording");
		}
	}

	// Save map, players and auth stuff
	{
		float &counter = m_savemap_timer;
		counter += dtime;
		if(counter >= g_settings->getFloat("server_map_save_interval"))
		{
			counter = 0.0;
			JMutexAutoLock lock(m_env_mutex);

			ScopeProfiler sp(g_profiler, "Server: saving stuff");

			//Ban stuff
			if(m_banmanager->isModified())
				m_banmanager->save();

			// Save changed parts of map
			m_env->getMap().save(MOD_STATE_WRITE_NEEDED);

			// Save players
			m_env->serializePlayers(m_path_world);

			// Save environment metadata
			m_env->saveMeta(m_path_world);
		}
	}
}

void Server::Receive()
{
	DSTACK(__FUNCTION_NAME);
	SharedBuffer<u8> data;
	u16 peer_id;
	u32 datasize;
	try{
		{
			JMutexAutoLock conlock(m_con_mutex);
			datasize = m_con.Receive(peer_id, data);
		}

		// This has to be called so that the client list gets synced
		// with the peer list of the connection
		handlePeerChanges();

		ProcessData(*data, datasize, peer_id);
	}
	catch(con::InvalidIncomingDataException &e)
	{
		infostream<<"Server::Receive(): "
				"InvalidIncomingDataException: what()="
				<<e.what()<<std::endl;
	}
	catch(con::PeerNotFoundException &e)
	{
		//NOTE: This is not needed anymore

		// The peer has been disconnected.
		// Find the associated player and remove it.

		/*JMutexAutoLock envlock(m_env_mutex);

		infostream<<"ServerThread: peer_id="<<peer_id
				<<" has apparently closed connection. "
				<<"Removing player."<<std::endl;

		m_env->removePlayer(peer_id);*/
	}
}



void Server::setTimeOfDay(u32 time)
{
	DSTACK(__FUNCTION_NAME);
	m_env->setTimeOfDay(time);
	m_time_of_day_send_timer = 0;
}

void Server::onMapEditEvent(MapEditEvent *event)
{
	DSTACK(__FUNCTION_NAME);
	//infostream<<"Server::onMapEditEvent()"<<std::endl;
	if(m_ignore_map_edit_events)
		return;
	if(m_ignore_map_edit_events_area.contains(event->getArea()))
		return;
	MapEditEvent *e = event->clone();
	m_unsent_map_edit_queue.push_back(e);
}

Inventory* Server::getInventory(const InventoryLocation &loc)
{
	DSTACK(__FUNCTION_NAME);
	switch(loc.type){
	case InventoryLocation::UNDEFINED:
	{}
	break;
	case InventoryLocation::CURRENT_PLAYER:
	{}
	break;
	case InventoryLocation::PLAYER:
	{
		Player *player = m_env->getPlayer(loc.name.c_str());
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
		assert(0);
	}
	return NULL;
}

void Server::setInventoryModified(const InventoryLocation &loc)
{
	DSTACK(__FUNCTION_NAME);
	switch(loc.type){
	case InventoryLocation::UNDEFINED:
	{}
	break;
	case InventoryLocation::PLAYER:
	{
		Player *player = m_env->getPlayer(loc.name.c_str());
		if(!player)
			return;
		PlayerSAO *playersao = player->getPlayerSAO();
		if(!playersao)
			return;
		playersao->m_inventory_not_sent = true;
		playersao->m_wielded_item_not_sent = true;
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
		sendDetachedInventoryToAll(loc.name);
	}
	break;
	default:
		assert(0);
	}
}

void Server::peerAdded(con::Peer *peer)
{
	DSTACK(__FUNCTION_NAME);
	verbosestream<<"Server::peerAdded(): peer->id="
			<<peer->id<<std::endl;

	PeerChange c;
	c.type = PEER_ADDED;
	c.peer_id = peer->id;
	c.timeout = false;
	m_peer_change_queue.push_back(c);
}

void Server::deletingPeer(con::Peer *peer, bool timeout)
{
	DSTACK(__FUNCTION_NAME);
	verbosestream<<"Server::deletingPeer(): peer->id="
			<<peer->id<<", timeout="<<timeout<<std::endl;

	PeerChange c;
	c.type = PEER_REMOVED;
	c.peer_id = peer->id;
	c.timeout = timeout;
	m_peer_change_queue.push_back(c);
}

/*
	Static send methods
*/

void Server::SendMovement(con::Connection &con, u16 peer_id)
{
	DSTACK(__FUNCTION_NAME);
	std::ostringstream os(std::ios_base::binary);

	writeU16(os, TOCLIENT_MOVEMENT);
	writeF1000(os, g_settings->getFloat("movement_acceleration_default"));
	writeF1000(os, g_settings->getFloat("movement_acceleration_air"));
	writeF1000(os, g_settings->getFloat("movement_acceleration_fast"));
	writeF1000(os, g_settings->getFloat("movement_speed_walk"));
	writeF1000(os, g_settings->getFloat("movement_speed_crouch"));
	writeF1000(os, g_settings->getFloat("movement_speed_fast"));
	writeF1000(os, g_settings->getFloat("movement_speed_climb"));
	writeF1000(os, g_settings->getFloat("movement_speed_jump"));
	writeF1000(os, g_settings->getFloat("movement_liquid_fluidity"));
	writeF1000(os, g_settings->getFloat("movement_liquid_fluidity_smooth"));
	writeF1000(os, g_settings->getFloat("movement_liquid_sink"));
	writeF1000(os, g_settings->getFloat("movement_gravity"));

	// Make data buffer
	std::string s = os.str();
	SharedBuffer<u8> data((u8*)s.c_str(), s.size());
	// Send as reliable
	con.Send(peer_id, 0, data, true);
}

void Server::SendHP(con::Connection &con, u16 peer_id, u8 hp)
{
	DSTACK(__FUNCTION_NAME);
	std::ostringstream os(std::ios_base::binary);

	writeU16(os, TOCLIENT_HP);
	writeU8(os, hp);

	// Make data buffer
	std::string s = os.str();
	SharedBuffer<u8> data((u8*)s.c_str(), s.size());
	// Send as reliable
	con.Send(peer_id, 0, data, true);
}

void Server::SendBreath(con::Connection &con, u16 peer_id, u16 breath)
{
	DSTACK(__FUNCTION_NAME);
	std::ostringstream os(std::ios_base::binary);

	writeU16(os, TOCLIENT_BREATH);
	writeU16(os, breath);

	// Make data buffer
	std::string s = os.str();
	SharedBuffer<u8> data((u8*)s.c_str(), s.size());
	// Send as reliable
	con.Send(peer_id, 0, data, true);
}

void Server::SendAccessDenied(con::Connection &con, u16 peer_id,
		const std::wstring &reason)
{
	DSTACK(__FUNCTION_NAME);
	std::ostringstream os(std::ios_base::binary);

	writeU16(os, TOCLIENT_ACCESS_DENIED);
	os<<serializeWideString(reason);

	// Make data buffer
	std::string s = os.str();
	SharedBuffer<u8> data((u8*)s.c_str(), s.size());
	// Send as reliable
	con.Send(peer_id, 0, data, true);
}

void Server::SendDeathscreen(con::Connection &con, u16 peer_id,
		bool set_camera_point_target, v3f camera_point_target)
{
	DSTACK(__FUNCTION_NAME);
	std::ostringstream os(std::ios_base::binary);

	writeU16(os, TOCLIENT_DEATHSCREEN);
	writeU8(os, set_camera_point_target);
	writeV3F1000(os, camera_point_target);

	// Make data buffer
	std::string s = os.str();
	SharedBuffer<u8> data((u8*)s.c_str(), s.size());
	// Send as reliable
	con.Send(peer_id, 0, data, true);
}

void Server::SendItemDef(con::Connection &con, u16 peer_id,
		IItemDefManager *itemdef, u16 protocol_version)
{
	DSTACK(__FUNCTION_NAME);
	std::ostringstream os(std::ios_base::binary);

	/*
		u16 command
		u32 length of the next item
		zlib-compressed serialized ItemDefManager
	*/
	writeU16(os, TOCLIENT_ITEMDEF);
	std::ostringstream tmp_os(std::ios::binary);
	itemdef->serialize(tmp_os, protocol_version);
	std::ostringstream tmp_os2(std::ios::binary);
	compressZlib(tmp_os.str(), tmp_os2);
	os<<serializeLongString(tmp_os2.str());

	// Make data buffer
	std::string s = os.str();
	verbosestream<<"Server: Sending item definitions to id("<<peer_id
			<<"): size="<<s.size()<<std::endl;
	SharedBuffer<u8> data((u8*)s.c_str(), s.size());
	// Send as reliable
	con.Send(peer_id, 0, data, true);
}

void Server::SendNodeDef(con::Connection &con, u16 peer_id,
		INodeDefManager *nodedef, u16 protocol_version)
{
	DSTACK(__FUNCTION_NAME);
	std::ostringstream os(std::ios_base::binary);

	/*
		u16 command
		u32 length of the next item
		zlib-compressed serialized NodeDefManager
	*/
	writeU16(os, TOCLIENT_NODEDEF);
	std::ostringstream tmp_os(std::ios::binary);
	nodedef->serialize(tmp_os, protocol_version);
	std::ostringstream tmp_os2(std::ios::binary);
	compressZlib(tmp_os.str(), tmp_os2);
	os<<serializeLongString(tmp_os2.str());

	// Make data buffer
	std::string s = os.str();
	verbosestream<<"Server: Sending node definitions to id("<<peer_id
			<<"): size="<<s.size()<<std::endl;
	SharedBuffer<u8> data((u8*)s.c_str(), s.size());
	// Send as reliable
	con.Send(peer_id, 0, data, true);
}

/*
	Non-static send methods
*/

void Server::SendInventory(u16 peer_id)
{
	DSTACK(__FUNCTION_NAME);

	PlayerSAO *playersao = getPlayerSAO(peer_id);
	assert(playersao);

	playersao->m_inventory_not_sent = false;

	/*
		Serialize it
	*/

	std::ostringstream os;
	playersao->getInventory()->serialize(os);

	std::string s = os.str();

	SharedBuffer<u8> data(s.size()+2);
	writeU16(&data[0], TOCLIENT_INVENTORY);
	memcpy(&data[2], s.c_str(), s.size());

	// Send as reliable
	m_con.Send(peer_id, 0, data, true);
}

void Server::SendChatMessage(u16 peer_id, const std::wstring &message)
{
	DSTACK(__FUNCTION_NAME);

	std::ostringstream os(std::ios_base::binary);
	u8 buf[12];

	// Write command
	writeU16(buf, TOCLIENT_CHAT_MESSAGE);
	os.write((char*)buf, 2);

	// Write length
	writeU16(buf, message.size());
	os.write((char*)buf, 2);

	// Write string
	for(u32 i=0; i<message.size(); i++)
	{
		u16 w = message[i];
		writeU16(buf, w);
		os.write((char*)buf, 2);
	}

	// Make data buffer
	std::string s = os.str();
	SharedBuffer<u8> data((u8*)s.c_str(), s.size());
	// Send as reliable
	m_con.Send(peer_id, 0, data, true);
}

void Server::SendShowFormspecMessage(u16 peer_id, const std::string formspec,
					const std::string formname)
{
	DSTACK(__FUNCTION_NAME);

	std::ostringstream os(std::ios_base::binary);
	u8 buf[12];

	// Write command
	writeU16(buf, TOCLIENT_SHOW_FORMSPEC);
	os.write((char*)buf, 2);
	os<<serializeLongString(formspec);
	os<<serializeString(formname);

	// Make data buffer
	std::string s = os.str();
	SharedBuffer<u8> data((u8*)s.c_str(), s.size());
	// Send as reliable
	m_con.Send(peer_id, 0, data, true);
}

void Server::BroadcastChatMessage(const std::wstring &message)
{
	DSTACK(__FUNCTION_NAME);
	for(std::map<u16, RemoteClient*>::iterator
		i = m_clients.begin();
		i != m_clients.end(); ++i)
	{
		// Get client and check that it is valid
		RemoteClient *client = i->second;
		assert(client->peer_id == i->first);
		if(client->serialization_version == SER_FMT_VER_INVALID)
			continue;

		SendChatMessage(client->peer_id, message);
	}
}

void Server::SendTimeOfDay(u16 peer_id, u16 time, f32 time_speed)
{
	DSTACK(__FUNCTION_NAME);

	// Make packet
	SharedBuffer<u8> data(2+2+4);
	writeU16(&data[0], TOCLIENT_TIME_OF_DAY);
	writeU16(&data[2], time);
	writeF1000(&data[4], time_speed);

	// Send as reliable
	m_con.Send(peer_id, 0, data, true);
}

void Server::SendPlayerHP(u16 peer_id)
{
	DSTACK(__FUNCTION_NAME);
	PlayerSAO *playersao = getPlayerSAO(peer_id);
	assert(playersao);
	playersao->m_hp_not_sent = false;
	SendHP(m_con, peer_id, playersao->getHP());

	// Send to other clients
	std::string str = gob_cmd_punched(playersao->readDamage(), playersao->getHP());
	ActiveObjectMessage aom(playersao->getId(), true, str);
	playersao->m_messages_out.push_back(aom);
}

void Server::SendPlayerBreath(u16 peer_id)
{
	DSTACK(__FUNCTION_NAME);
	PlayerSAO *playersao = getPlayerSAO(peer_id);
	assert(playersao);
	playersao->m_breath_not_sent = false;
	SendBreath(m_con, peer_id, playersao->getBreath());
}

void Server::SendMovePlayer(u16 peer_id)
{
	DSTACK(__FUNCTION_NAME);
	Player *player = m_env->getPlayer(peer_id);
	assert(player);

	std::ostringstream os(std::ios_base::binary);
	writeU16(os, TOCLIENT_MOVE_PLAYER);
	writeV3F1000(os, player->getPosition());
	writeF1000(os, player->getPitch());
	writeF1000(os, player->getYaw());

	{
		v3f pos = player->getPosition();
		f32 pitch = player->getPitch();
		f32 yaw = player->getYaw();
		verbosestream<<"Server: Sending TOCLIENT_MOVE_PLAYER"
				<<" pos=("<<pos.X<<","<<pos.Y<<","<<pos.Z<<")"
				<<" pitch="<<pitch
				<<" yaw="<<yaw
				<<std::endl;
	}

	// Make data buffer
	std::string s = os.str();
	SharedBuffer<u8> data((u8*)s.c_str(), s.size());
	// Send as reliable
	m_con.Send(peer_id, 0, data, true);
}

void Server::SendPlayerPrivileges(u16 peer_id)
{
	DSTACK(__FUNCTION_NAME);
	Player *player = m_env->getPlayer(peer_id);
	assert(player);
	if(player->peer_id == PEER_ID_INEXISTENT)
		return;

	std::set<std::string> privs;
	m_script->getAuth(player->getName(), NULL, &privs);

	std::ostringstream os(std::ios_base::binary);
	writeU16(os, TOCLIENT_PRIVILEGES);
	writeU16(os, privs.size());
	for(std::set<std::string>::const_iterator i = privs.begin();
			i != privs.end(); i++){
		os<<serializeString(*i);
	}

	// Make data buffer
	std::string s = os.str();
	SharedBuffer<u8> data((u8*)s.c_str(), s.size());
	// Send as reliable
	m_con.Send(peer_id, 0, data, true);
}

void Server::SendPlayerInventoryFormspec(u16 peer_id)
{
	DSTACK(__FUNCTION_NAME);
	Player *player = m_env->getPlayer(peer_id);
	assert(player);
	if(player->peer_id == PEER_ID_INEXISTENT)
		return;

	std::ostringstream os(std::ios_base::binary);
	writeU16(os, TOCLIENT_INVENTORY_FORMSPEC);
	os<<serializeLongString(player->inventory_formspec);

	// Make data buffer
	std::string s = os.str();
	SharedBuffer<u8> data((u8*)s.c_str(), s.size());
	// Send as reliable
	m_con.Send(peer_id, 0, data, true);
}

s32 Server::playSound(const SimpleSoundSpec &spec,
		const ServerSoundParams &params)
{
	DSTACK(__FUNCTION_NAME);
	// Find out initial position of sound
	bool pos_exists = false;
	v3f pos = params.getPos(m_env, &pos_exists);
	// If position is not found while it should be, cancel sound
	if(pos_exists != (params.type != ServerSoundParams::SSP_LOCAL))
		return -1;
	// Filter destination clients
	std::set<RemoteClient*> dst_clients;
	if(params.to_player != "")
	{
		Player *player = m_env->getPlayer(params.to_player.c_str());
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
		RemoteClient *client = getClient(player->peer_id);
		dst_clients.insert(client);
	}
	else
	{
		for(std::map<u16, RemoteClient*>::iterator
				i = m_clients.begin(); i != m_clients.end(); ++i)
		{
			RemoteClient *client = i->second;
			Player *player = m_env->getPlayer(client->peer_id);
			if(!player)
				continue;
			if(pos_exists){
				if(player->getPosition().getDistanceFrom(pos) >
						params.max_hear_distance)
					continue;
			}
			dst_clients.insert(client);
		}
	}
	if(dst_clients.size() == 0)
		return -1;
	// Create the sound
	s32 id = m_next_sound_id++;
	// The sound will exist as a reference in m_playing_sounds
	m_playing_sounds[id] = ServerPlayingSound();
	ServerPlayingSound &psound = m_playing_sounds[id];
	psound.params = params;
	for(std::set<RemoteClient*>::iterator i = dst_clients.begin();
			i != dst_clients.end(); i++)
		psound.clients.insert((*i)->peer_id);
	// Create packet
	std::ostringstream os(std::ios_base::binary);
	writeU16(os, TOCLIENT_PLAY_SOUND);
	writeS32(os, id);
	os<<serializeString(spec.name);
	writeF1000(os, spec.gain * params.gain);
	writeU8(os, params.type);
	writeV3F1000(os, pos);
	writeU16(os, params.object);
	writeU8(os, params.loop);
	// Make data buffer
	std::string s = os.str();
	SharedBuffer<u8> data((u8*)s.c_str(), s.size());
	// Send
	for(std::set<RemoteClient*>::iterator i = dst_clients.begin();
			i != dst_clients.end(); i++){
		// Send as reliable
		m_con.Send((*i)->peer_id, 0, data, true);
	}
	return id;
}
void Server::stopSound(s32 handle)
{
	DSTACK(__FUNCTION_NAME);
	// Get sound reference
	std::map<s32, ServerPlayingSound>::iterator i =
			m_playing_sounds.find(handle);
	if(i == m_playing_sounds.end())
		return;
	ServerPlayingSound &psound = i->second;
	// Create packet
	std::ostringstream os(std::ios_base::binary);
	writeU16(os, TOCLIENT_STOP_SOUND);
	writeS32(os, handle);
	// Make data buffer
	std::string s = os.str();
	SharedBuffer<u8> data((u8*)s.c_str(), s.size());
	// Send
	for(std::set<u16>::iterator i = psound.clients.begin();
			i != psound.clients.end(); i++){
		// Send as reliable
		m_con.Send(*i, 0, data, true);
	}
	// Remove sound reference
	m_playing_sounds.erase(i);
}

void Server::sendRemoveNode(v3s16 p, u16 ignore_id,
	std::list<u16> *far_players, float far_d_nodes)
{
	DSTACK(__FUNCTION_NAME);
	float maxd = far_d_nodes*BS;
	v3f p_f = intToFloat(p, BS);

	// Create packet
	u32 replysize = 8;
	SharedBuffer<u8> reply(replysize);
	writeU16(&reply[0], TOCLIENT_REMOVENODE);
	writeS16(&reply[2], p.X);
	writeS16(&reply[4], p.Y);
	writeS16(&reply[6], p.Z);

	for(std::map<u16, RemoteClient*>::iterator
		i = m_clients.begin();
		i != m_clients.end(); ++i)
	{
		// Get client and check that it is valid
		RemoteClient *client = i->second;
		assert(client->peer_id == i->first);
		if(client->serialization_version == SER_FMT_VER_INVALID)
			continue;

		// Don't send if it's the same one
		if(client->peer_id == ignore_id)
			continue;

		if(far_players)
		{
			// Get player
			Player *player = m_env->getPlayer(client->peer_id);
			if(player)
			{
				// If player is far away, only set modified blocks not sent
				v3f player_pos = player->getPosition();
				if(player_pos.getDistanceFrom(p_f) > maxd)
				{
					far_players->push_back(client->peer_id);
					continue;
				}
			}
		}

		// Send as reliable
		m_con.Send(client->peer_id, 0, reply, true);
	}
}

void Server::sendAddNode(v3s16 p, MapNode n, u16 ignore_id,
		std::list<u16> *far_players, float far_d_nodes,
		bool remove_metadata)
{
	DSTACK(__FUNCTION_NAME);
	float maxd = far_d_nodes*BS;
	v3f p_f = intToFloat(p, BS);

	for(std::map<u16, RemoteClient*>::iterator
		i = m_clients.begin();
		i != m_clients.end(); ++i)
	{
		// Get client and check that it is valid
		RemoteClient *client = i->second;
		assert(client->peer_id == i->first);
		if(client->serialization_version == SER_FMT_VER_INVALID)
			continue;

		// Don't send if it's the same one
		if(client->peer_id == ignore_id)
			continue;

		if(far_players)
		{
			// Get player
			Player *player = m_env->getPlayer(client->peer_id);
			if(player)
			{
				// If player is far away, only set modified blocks not sent
				v3f player_pos = player->getPosition();
				if(player_pos.getDistanceFrom(p_f) > maxd)
				{
					far_players->push_back(client->peer_id);
					continue;
				}
			}
		}

		// Create packet
		u32 replysize = 9 + MapNode::serializedLength(client->serialization_version);
		SharedBuffer<u8> reply(replysize);
		writeU16(&reply[0], TOCLIENT_ADDNODE);
		writeS16(&reply[2], p.X);
		writeS16(&reply[4], p.Y);
		writeS16(&reply[6], p.Z);
		n.serialize(&reply[8], client->serialization_version);
		u32 index = 8 + MapNode::serializedLength(client->serialization_version);
		writeU8(&reply[index], remove_metadata ? 0 : 1);
		
		if (!remove_metadata) {
			if (client->net_proto_version <= 21) {
				// Old clients always clear metadata; fix it
				// by sending the full block again.
				client->SetBlockNotSent(p);
			}
		}

		// Send as reliable
		m_con.Send(client->peer_id, 0, reply, true);
	}
}

void Server::setBlockNotSent(v3s16 p)
{
	DSTACK(__FUNCTION_NAME);
	for(std::map<u16, RemoteClient*>::iterator
		i = m_clients.begin();
		i != m_clients.end(); ++i)
	{
		RemoteClient *client = i->second;
		client->SetBlockNotSent(p);
	}
}

void Server::SendBlockNoLock(u16 peer_id, MapBlock *block, u8 ver, u16 net_proto_version)
{
	DSTACK(__FUNCTION_NAME);

	v3s16 p = block->getPos();

	/*
		Create a packet with the block in the right format
	*/

	std::ostringstream os(std::ios_base::binary);
	block->serialize(os, ver, false);
	block->serializeNetworkSpecific(os, net_proto_version);
	std::string s = os.str();
	SharedBuffer<u8> blockdata((u8*)s.c_str(), s.size());

	u32 replysize = 8 + blockdata.getSize();
	SharedBuffer<u8> reply(replysize);
	writeU16(&reply[0], TOCLIENT_BLOCKDATA);
	writeS16(&reply[2], p.X);
	writeS16(&reply[4], p.Y);
	writeS16(&reply[6], p.Z);
	memcpy(&reply[8], *blockdata, blockdata.getSize());

	/*infostream<<"Server: Sending block ("<<p.X<<","<<p.Y<<","<<p.Z<<")"
			<<":  \tpacket size: "<<replysize<<std::endl;*/

	/*
		Send packet
	*/
	m_con.Send(peer_id, 1, reply, true);
}

void Server::SendBlocks(float dtime)
{
	DSTACK(__FUNCTION_NAME);

	JMutexAutoLock envlock(m_env_mutex);
	JMutexAutoLock conlock(m_con_mutex);

	ScopeProfiler sp(g_profiler, "Server: sel and send blocks to clients");

	std::vector<PrioritySortedBlockTransfer> queue;

	s32 total_sending = 0;

	{
		ScopeProfiler sp(g_profiler, "Server: selecting blocks for sending");

		for(std::map<u16, RemoteClient*>::iterator
			i = m_clients.begin();
			i != m_clients.end(); ++i)
		{
			RemoteClient *client = i->second;
			assert(client->peer_id == i->first);

			// If definitions and textures have not been sent, don't
			// send MapBlocks either
			if(!client->definitions_sent)
				continue;

			total_sending += client->SendingCount();

			if(client->serialization_version == SER_FMT_VER_INVALID)
				continue;

			client->GetNextBlocks(this, dtime, queue);
		}
	}

	// Sort.
	// Lowest priority number comes first.
	// Lowest is most important.
	std::sort(queue.begin(), queue.end());

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

		RemoteClient *client = getClientNoEx(q.peer_id);
		if(!client)
			continue;
		if(client->denied)
			continue;

		SendBlockNoLock(q.peer_id, block, client->serialization_version, client->net_proto_version);

		client->SentBlock(q.pos);

		total_sending++;
	}
}

void Server::fillMediaCache()
{
	DSTACK(__FUNCTION_NAME);

	infostream<<"Server: Calculating media file checksums"<<std::endl;

	// Collect all media file paths
	std::list<std::string> paths;
	for(std::vector<ModSpec>::iterator i = m_mods.begin();
			i != m_mods.end(); i++){
		const ModSpec &mod = *i;
		paths.push_back(mod.path + DIR_DELIM + "textures");
		paths.push_back(mod.path + DIR_DELIM + "sounds");
		paths.push_back(mod.path + DIR_DELIM + "media");
		paths.push_back(mod.path + DIR_DELIM + "models");
	}
	paths.push_back(porting::path_user + DIR_DELIM + "textures" + DIR_DELIM + "server");

	// Collect media file information from paths into cache
	for(std::list<std::string>::iterator i = paths.begin();
			i != paths.end(); i++)
	{
		std::string mediapath = *i;
		std::vector<fs::DirListNode> dirlist = fs::GetDirListing(mediapath);
		for(u32 j=0; j<dirlist.size(); j++){
			if(dirlist[j].dir) // Ignode dirs
				continue;
			std::string filename = dirlist[j].name;
			// If name contains illegal characters, ignore the file
			if(!string_allowed(filename, TEXTURENAME_ALLOWED_CHARS)){
				infostream<<"Server: ignoring illegal file name: \""
						<<filename<<"\""<<std::endl;
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
			if(removeStringEnd(filename, supported_ext) == ""){
				infostream<<"Server: ignoring unsupported file extension: \""
						<<filename<<"\""<<std::endl;
				continue;
			}
			// Ok, attempt to load the file and add to cache
			std::string filepath = mediapath + DIR_DELIM + filename;
			// Read data
			std::ifstream fis(filepath.c_str(), std::ios_base::binary);
			if(fis.good() == false){
				errorstream<<"Server::fillMediaCache(): Could not open \""
						<<filename<<"\" for reading"<<std::endl;
				continue;
			}
			std::ostringstream tmp_os(std::ios_base::binary);
			bool bad = false;
			for(;;){
				char buf[1024];
				fis.read(buf, 1024);
				std::streamsize len = fis.gcount();
				tmp_os.write(buf, len);
				if(fis.eof())
					break;
				if(!fis.good()){
					bad = true;
					break;
				}
			}
			if(bad){
				errorstream<<"Server::fillMediaCache(): Failed to read \""
						<<filename<<"\""<<std::endl;
				continue;
			}
			if(tmp_os.str().length() == 0){
				errorstream<<"Server::fillMediaCache(): Empty file \""
						<<filepath<<"\""<<std::endl;
				continue;
			}

			SHA1 sha1;
			sha1.addBytes(tmp_os.str().c_str(), tmp_os.str().length());

			unsigned char *digest = sha1.getDigest();
			std::string sha1_base64 = base64_encode(digest, 20);
			std::string sha1_hex = hex_encode((char*)digest, 20);
			free(digest);

			// Put in list
			this->m_media[filename] = MediaInfo(filepath, sha1_base64);
			verbosestream<<"Server: "<<sha1_hex<<" is "<<filename<<std::endl;
		}
	}
}

void Server::sendMediaAnnouncement(u16 peer_id)
{
	DSTACK(__FUNCTION_NAME);

	verbosestream<<"Server: Announcing files to id("<<peer_id<<")"
			<<std::endl;

	std::list<SendableMediaAnnouncement> file_announcements;

	for(std::map<std::string, MediaInfo>::iterator i = m_media.begin();
			i != m_media.end(); i++){
		// Put in list
		file_announcements.push_back(
				SendableMediaAnnouncement(i->first, i->second.sha1_digest));
	}

	// Make packet
	std::ostringstream os(std::ios_base::binary);

	/*
		u16 command
		u32 number of files
		for each texture {
			u16 length of name
			string name
			u16 length of sha1_digest
			string sha1_digest
		}
	*/

	writeU16(os, TOCLIENT_ANNOUNCE_MEDIA);
	writeU16(os, file_announcements.size());

	for(std::list<SendableMediaAnnouncement>::iterator
			j = file_announcements.begin();
			j != file_announcements.end(); ++j){
		os<<serializeString(j->name);
		os<<serializeString(j->sha1_digest);
	}
	os<<serializeString(g_settings->get("remote_media"));

	// Make data buffer
	std::string s = os.str();
	SharedBuffer<u8> data((u8*)s.c_str(), s.size());

	// Send as reliable
	m_con.Send(peer_id, 0, data, true);
}

void Server::sendRequestedMedia(u16 peer_id,
		const std::list<std::string> &tosend)
{
	DSTACK(__FUNCTION_NAME);

	verbosestream<<"Server::sendRequestedMedia(): "
			<<"Sending files to client"<<std::endl;

	/* Read files */

	// Put 5kB in one bunch (this is not accurate)
	u32 bytes_per_bunch = 5000;

	std::vector< std::list<SendableMedia> > file_bunches;
	file_bunches.push_back(std::list<SendableMedia>());

	u32 file_size_bunch_total = 0;

	for(std::list<std::string>::const_iterator i = tosend.begin();
			i != tosend.end(); ++i)
	{
		const std::string &name = *i;

		if(m_media.find(name) == m_media.end()){
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
		for(;;){
			char buf[1024];
			fis.read(buf, 1024);
			std::streamsize len = fis.gcount();
			tmp_os.write(buf, len);
			file_size_bunch_total += len;
			if(fis.eof())
				break;
			if(!fis.good()){
				bad = true;
				break;
			}
		}
		if(bad){
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
		if(file_size_bunch_total >= bytes_per_bunch){
			file_bunches.push_back(std::list<SendableMedia>());
			file_size_bunch_total = 0;
		}

	}

	/* Create and send packets */

	u32 num_bunches = file_bunches.size();
	for(u32 i=0; i<num_bunches; i++)
	{
		std::ostringstream os(std::ios_base::binary);

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

		writeU16(os, TOCLIENT_MEDIA);
		writeU16(os, num_bunches);
		writeU16(os, i);
		writeU32(os, file_bunches[i].size());

		for(std::list<SendableMedia>::iterator
				j = file_bunches[i].begin();
				j != file_bunches[i].end(); ++j){
			os<<serializeString(j->name);
			os<<serializeLongString(j->data);
		}

		// Make data buffer
		std::string s = os.str();
		verbosestream<<"Server::sendRequestedMedia(): bunch "
				<<i<<"/"<<num_bunches
				<<" files="<<file_bunches[i].size()
				<<" size=" <<s.size()<<std::endl;
		SharedBuffer<u8> data((u8*)s.c_str(), s.size());
		// Send as reliable
		m_con.Send(peer_id, 0, data, true);
	}
}

void Server::sendDetachedInventory(const std::string &name, u16 peer_id)
{
	DSTACK(__FUNCTION_NAME);
	if(m_detached_inventories.count(name) == 0){
		errorstream<<__FUNCTION_NAME<<": \""<<name<<"\" not found"<<std::endl;
		return;
	}
	Inventory *inv = m_detached_inventories[name];

	std::ostringstream os(std::ios_base::binary);
	writeU16(os, TOCLIENT_DETACHED_INVENTORY);
	os<<serializeString(name);
	inv->serialize(os);

	// Make data buffer
	std::string s = os.str();
	SharedBuffer<u8> data((u8*)s.c_str(), s.size());
	// Send as reliable
	m_con.Send(peer_id, 0, data, true);
}

void Server::sendDetachedInventoryToAll(const std::string &name)
{
	DSTACK(__FUNCTION_NAME);

	for(std::map<u16, RemoteClient*>::iterator
			i = m_clients.begin();
			i != m_clients.end(); ++i){
		RemoteClient *client = i->second;
		sendDetachedInventory(name, client->peer_id);
	}
}

void Server::sendDetachedInventories(u16 peer_id)
{
	DSTACK(__FUNCTION_NAME);

	for(std::map<std::string, Inventory*>::iterator
			i = m_detached_inventories.begin();
			i != m_detached_inventories.end(); i++){
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
	DSTACK(__FUNCTION_NAME);

	PlayerSAO *playersao = getPlayerSAO(peer_id);
	assert(playersao);

	infostream<<"Server::DiePlayer(): Player "
			<<playersao->getPlayer()->getName()
			<<" dies"<<std::endl;

	playersao->setHP(0);

	// Trigger scripted stuff
	m_script->on_dieplayer(playersao);

	SendPlayerHP(peer_id);
	SendDeathscreen(m_con, peer_id, false, v3f(0,0,0));
}

void Server::RespawnPlayer(u16 peer_id)
{
	DSTACK(__FUNCTION_NAME);

	PlayerSAO *playersao = getPlayerSAO(peer_id);
	assert(playersao);

	infostream<<"Server::RespawnPlayer(): Player "
			<<playersao->getPlayer()->getName()
			<<" respawns"<<std::endl;

	playersao->setHP(PLAYER_MAX_HP);

	bool repositioned = m_script->on_respawnplayer(playersao);
	if(!repositioned){
		v3f pos = findSpawnPos(m_env->getServerMap());
		playersao->setPos(pos);
	}
}

void Server::DenyAccess(u16 peer_id, const std::wstring &reason)
{
	DSTACK(__FUNCTION_NAME);

	SendAccessDenied(m_con, peer_id, reason);

	RemoteClient *client = getClientNoEx(peer_id);
	if(client)
		client->denied = true;

	// If there are way too many clients, get rid of denied new ones immediately
	if((int)m_clients.size() > 2 * g_settings->getU16("max_users")){
		verbosestream<<"Server: DenyAccess: Too many clients; getting rid of "
				<<"peer_id="<<peer_id<<" immediately"<<std::endl;
		// Delete peer to stop sending it data
		m_con.DeletePeer(peer_id);
		// Delete client also to stop block sends and other stuff
		DeleteClient(peer_id, CDR_DENY);
	}
}

void Server::DeleteClient(u16 peer_id, ClientDeletionReason reason)
{
	DSTACK(__FUNCTION_NAME);

	// Error check
	std::map<u16, RemoteClient*>::iterator n;
	n = m_clients.find(peer_id);
	// The client may not exist; clients are immediately removed if their
	// access is denied, and this event occurs later then.
	if(n == m_clients.end())
		return;

	/*
		Mark objects to be not known by the client
	*/
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

	/*
		Clear references to playing sounds
	*/
	for(std::map<s32, ServerPlayingSound>::iterator
			i = m_playing_sounds.begin();
			i != m_playing_sounds.end();)
	{
		ServerPlayingSound &psound = i->second;
		psound.clients.erase(peer_id);
		if(psound.clients.size() == 0)
			m_playing_sounds.erase(i++);
		else
			i++;
	}

	Player *player = m_env->getPlayer(peer_id);

	// Collect information about leaving in chat
	std::wstring message;
	{
		if(player != NULL && reason != CDR_DENY)
		{
			std::wstring name = narrow_to_wide(player->getName());
			message += L"*** ";
			message += name;
			message += L" left the game.";
			if(reason == CDR_TIMEOUT)
				message += L" (timed out)";
		}
	}

	/* Run scripts and remove from environment */
	{
		if(player != NULL)
		{
			PlayerSAO *playersao = player->getPlayerSAO();
			assert(playersao);

			m_script->on_leaveplayer(playersao);

			playersao->disconnected();
		}
	}

	/*
		Print out action
	*/
	{
		if(player != NULL && reason != CDR_DENY)
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

			actionstream<<player->getName()<<" "
					<<(reason==CDR_TIMEOUT?"times out.":"leaves game.")
					<<" List of players: "<<os.str()<<std::endl;
		}
	}

	// Delete client
	delete m_clients[peer_id];
	m_clients.erase(peer_id);

	// Send leave chat message to all remaining clients
	if(message.length() != 0)
		BroadcastChatMessage(message);
}

void Server::UpdateCrafting(u16 peer_id)
{
	DSTACK(__FUNCTION_NAME);

	Player* player = m_env->getPlayer(peer_id);
	assert(player);

	// Get a preview for crafting
	ItemStack preview;
	InventoryLocation loc;
	loc.setPlayer(player->getName());
	getCraftingResult(&player->inventory, preview, false, this);
	m_env->getScriptIface()->item_CraftPredict(preview, player->getPlayerSAO(), (&player->inventory)->getList("craft"), loc);

	// Put the new preview in
	InventoryList *plist = player->inventory.getList("craftpreview");
	assert(plist);
	assert(plist->getSize() >= 1);
	plist->changeItem(0, preview);
}

RemoteClient* Server::getClient(u16 peer_id)
{
	DSTACK(__FUNCTION_NAME);
	RemoteClient *client = getClientNoEx(peer_id);
	if(!client)
		throw ClientNotFoundException("Client not found");
	return client;
}

RemoteClient* Server::getClientNoEx(u16 peer_id)
{
	DSTACK(__FUNCTION_NAME);
	std::map<u16, RemoteClient*>::iterator n;
	n = m_clients.find(peer_id);
	// The client may not exist; clients are immediately removed if their
	// access is denied, and this event occurs later then.
	if(n == m_clients.end())
		return NULL;
	return n->second;
}

std::string Server::getPlayerName(u16 peer_id)
{
	DSTACK(__FUNCTION_NAME);
	Player *player = m_env->getPlayer(peer_id);
	if(player == NULL)
		return "[id="+itos(peer_id)+"]";
	return player->getName();
}

PlayerSAO* Server::getPlayerSAO(u16 peer_id)
{
	DSTACK(__FUNCTION_NAME);
	Player *player = m_env->getPlayer(peer_id);
	if(player == NULL)
		return NULL;
	return player->getPlayerSAO();
}

std::wstring Server::getStatusString()
{
	DSTACK(__FUNCTION_NAME);
	std::wostringstream os(std::ios_base::binary);
	os<<L"# Server: ";
	// Version
	os<<L"version="<<narrow_to_wide(minetest_version_simple);
	// Uptime
	os<<L", uptime="<<m_uptime.get();
	// Max lag estimate
	os<<L", max_lag="<<m_env->getMaxLagEstimate();
	// Information about clients
	std::map<u16, RemoteClient*>::iterator i;
	bool first;
	os<<L", clients={";
	for(i = m_clients.begin(), first = true;
		i != m_clients.end(); ++i)
	{
		// Get client and check that it is valid
		RemoteClient *client = i->second;
		assert(client->peer_id == i->first);
		if(client->serialization_version == SER_FMT_VER_INVALID)
			continue;
		// Get player
		Player *player = m_env->getPlayer(client->peer_id);
		// Get name of player
		std::wstring name = L"unknown";
		if(player != NULL)
			name = narrow_to_wide(player->getName());
		// Add name to information string
		if(!first)
			os<<L",";
		else
			first = false;
		os<<name;
	}
	os<<L"}";
	if(((ServerMap*)(&m_env->getMap()))->isSavingEnabled() == false)
		os<<std::endl<<L"# Server: "<<" WARNING: Map saving is disabled.";
	if(g_settings->get("motd") != "")
		os<<std::endl<<L"# Server: "<<narrow_to_wide(g_settings->get("motd"));
	return os.str();
}

std::set<std::string> Server::getPlayerEffectivePrivs(const std::string &name)
{
	DSTACK(__FUNCTION_NAME);
	std::set<std::string> privs;
	m_script->getAuth(name, NULL, &privs);
	return privs;
}

bool Server::checkPriv(const std::string &name, const std::string &priv)
{
	DSTACK(__FUNCTION_NAME);
	std::set<std::string> privs = getPlayerEffectivePrivs(name);
	return (privs.count(priv) != 0);
}

void Server::reportPrivsModified(const std::string &name)
{
	DSTACK(__FUNCTION_NAME);
	if(name == ""){
		for(std::map<u16, RemoteClient*>::iterator
				i = m_clients.begin();
				i != m_clients.end(); ++i){
			RemoteClient *client = i->second;
			Player *player = m_env->getPlayer(client->peer_id);
			reportPrivsModified(player->getName());
		}
	} else {
		Player *player = m_env->getPlayer(name.c_str());
		if(!player)
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
	DSTACK(__FUNCTION_NAME);
	Player *player = m_env->getPlayer(name.c_str());
	if(!player)
		return;
	SendPlayerInventoryFormspec(player->peer_id);
}

void Server::setIpBanned(const std::string &ip, const std::string &name)
{
	DSTACK(__FUNCTION_NAME);
	m_banmanager->add(ip, name);
}

void Server::unsetIpBanned(const std::string &ip_or_name)
{
	DSTACK(__FUNCTION_NAME);
	m_banmanager->remove(ip_or_name);
}

std::string Server::getBanDescription(const std::string &ip_or_name)
{
	DSTACK(__FUNCTION_NAME);
	return m_banmanager->getBanDescription(ip_or_name);
}

void Server::notifyPlayer(const char *name, const std::wstring msg, const bool prepend = true)
{
	DSTACK(__FUNCTION_NAME);
	Player *player = m_env->getPlayer(name);
	if(!player)
		return;
	if (prepend)
		SendChatMessage(player->peer_id, std::wstring(L"Server -!- ")+msg);
	else
		SendChatMessage(player->peer_id, msg);
}

bool Server::showFormspec(const char *playername, const std::string &formspec, const std::string &formname)
{
	DSTACK(__FUNCTION_NAME);
	Player *player = m_env->getPlayer(playername);

	if(!player)
	{
		infostream<<"showFormspec: couldn't find player:"<<playername<<std::endl;
		return false;
	}

	SendShowFormspecMessage(player->peer_id, formspec, formname);
	return true;
}



void Server::notifyPlayers(const std::wstring msg)
{
	DSTACK(__FUNCTION_NAME);
	BroadcastChatMessage(msg);
}

Inventory* Server::createDetachedInventory(const std::string &name)
{
	DSTACK(__FUNCTION_NAME);
	if(m_detached_inventories.count(name) > 0){
		infostream<<"Server clearing detached inventory \""<<name<<"\""<<std::endl;
		delete m_detached_inventories[name];
	} else {
		infostream<<"Server creating detached inventory \""<<name<<"\""<<std::endl;
	}
	Inventory *inv = new Inventory(m_itemdef);
	assert(inv);
	m_detached_inventories[name] = inv;
	sendDetachedInventoryToAll(name);
	return inv;
}

// actions: time-reversed list
// Return value: success/failure
bool Server::rollbackRevertActions(const std::list<RollbackAction> &actions,
		std::list<std::string> *log)
{
	DSTACK(__FUNCTION_NAME);
	infostream<<"Server::rollbackRevertActions(len="<<actions.size()<<")"<<std::endl;
	ServerMap *map = (ServerMap*)(&m_env->getMap());
	// Disable rollback report sink while reverting
	BoolScopeSet rollback_scope_disable(&m_rollback_sink_enabled, false);

	// Fail if no actions to handle
	if(actions.empty()){
		log->push_back("Nothing to do.");
		return false;
	}

	int num_tried = 0;
	int num_failed = 0;

	for(std::list<RollbackAction>::const_iterator
			i = actions.begin();
			i != actions.end(); i++)
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
IItemDefManager* Server::getItemDefManager()
{
	return m_itemdef;
}
INodeDefManager* Server::getNodeDefManager()
{
	return m_nodedef;
}
ICraftDefManager* Server::getCraftDefManager()
{
	return m_craftdef;
}
ITextureSource* Server::getTextureSource()
{
	return NULL;
}
IShaderSource* Server::getShaderSource()
{
	return NULL;
}
u16 Server::allocateUnknownNodeId(const std::string &name)
{
	return m_nodedef->allocateDummy(name);
}
ISoundManager* Server::getSoundManager()
{
	return &dummySoundManager;
}
MtEventManager* Server::getEventManager()
{
	return m_event;
}
IRollbackReportSink* Server::getRollbackReportSink()
{
	if(!m_enable_rollback_recording)
		return NULL;
	if(!m_rollback_sink_enabled)
		return NULL;
	return m_rollback;
}
IWritableItemDefManager* Server::getWritableItemDefManager()
{
	return m_itemdef;
}
IWritableNodeDefManager* Server::getWritableNodeDefManager()
{
	return m_nodedef;
}
IWritableCraftDefManager* Server::getWritableCraftDefManager()
{
	return m_craftdef;
}

const ModSpec* Server::getModSpec(const std::string &modname)
{
	DSTACK(__FUNCTION_NAME);
	for(std::vector<ModSpec>::iterator i = m_mods.begin();
			i != m_mods.end(); i++){
		const ModSpec &mod = *i;
		if(mod.name == modname)
			return &mod;
	}
	return NULL;
}

void Server::getModNames(std::list<std::string> &modlist)
{
	DSTACK(__FUNCTION_NAME);
	for(std::vector<ModSpec>::iterator i = m_mods.begin(); i != m_mods.end(); i++)
	{
		modlist.push_back(i->name);
	}
}

std::string Server::getBuiltinLuaPath()
{
	return porting::path_share + DIR_DELIM + "builtin";
}

v3f findSpawnPos(ServerMap &map)
{
	//return v3f(50,50,50)*BS;

	v3s16 nodepos;

	s16 water_level = map.m_mgparams->water_level;

	// Try to find a good place a few times
	for(s32 i=0; i<1000; i++)
	{
		s32 range = 1 + i;
		// We're going to try to throw the player to this position
		v2s16 nodepos2d = v2s16(
				-range + (myrand() % (range * 2)),
				-range + (myrand() % (range * 2)));

		// Get ground height at point
		s16 groundheight = map.findGroundLevel(nodepos2d, g_settings->getBool("cache_block_before_spawn"));
		if (groundheight <= water_level) // Don't go underwater
			continue;
		if (groundheight > water_level + g_settings->getS16("max_spawn_height")) // Don't go to high places
			continue;

		nodepos = v3s16(nodepos2d.X, groundheight, nodepos2d.Y);
		bool is_good = false;
		s32 air_count = 0;
		for (s32 i = 0; i < 10; i++) {
			v3s16 blockpos = getNodeBlockPos(nodepos);
			map.emergeBlock(blockpos, true);
			content_t c = map.getNodeNoEx(nodepos).getContent();
			if (c == CONTENT_AIR || c == CONTENT_IGNORE) {
				air_count++;
				if (air_count >= 2){
					is_good = true;
					break;
				}
			}
			nodepos.Y++;
		}
		if(is_good){
			// Found a good place
			//infostream<<"Searched through "<<i<<" places."<<std::endl;
			break;
		}
	}

	return intToFloat(nodepos, BS);
}

PlayerSAO* Server::emergePlayer(const char *name, u16 peer_id)
{
	DSTACK(__FUNCTION_NAME);
	RemotePlayer *player = NULL;
	bool newplayer = false;

	/*
		Try to get an existing player
	*/
	player = static_cast<RemotePlayer*>(m_env->getPlayer(name));

	// If player is already connected, cancel
	if(player != NULL && player->peer_id != 0)
	{
		infostream<<"emergePlayer(): Player already connected"<<std::endl;
		return NULL;
	}

	/*
		If player with the wanted peer_id already exists, cancel.
	*/
	if(m_env->getPlayer(peer_id) != NULL)
	{
		infostream<<"emergePlayer(): Player with wrong name but same"
				" peer_id already exists"<<std::endl;
		return NULL;
	}

	/*
		Create a new player if it doesn't exist yet
	*/
	if(player == NULL)
	{
		newplayer = true;
		player = new RemotePlayer(this);
		player->updateName(name);

		/* Set player position */
		infostream<<"Server: Finding spawn place for player \""
				<<name<<"\""<<std::endl;
		v3f pos = findSpawnPos(m_env->getServerMap());
		player->setPosition(pos);

		/* Add player to environment */
		m_env->addPlayer(player);
	}

	/*
		Create a new player active object
	*/
	PlayerSAO *playersao = new PlayerSAO(m_env, player, peer_id,
			getPlayerEffectivePrivs(player->getName()),
			isSingleplayer());

	/* Clean up old HUD elements from previous sessions */
	player->hud.clear();

	/* Add object to environment */
	m_env->addActiveObject(playersao);

	/* Run scripts */
	if(newplayer)
		m_script->on_newplayer(playersao);

	m_script->on_joinplayer(playersao);

	return playersao;
}

void Server::handlePeerChange(PeerChange &c)
{
	DSTACK(__FUNCTION_NAME);
	JMutexAutoLock envlock(m_env_mutex);
	JMutexAutoLock conlock(m_con_mutex);

	if(c.type == PEER_ADDED)
	{
		/*
			Add
		*/

		// Error check
		std::map<u16, RemoteClient*>::iterator n;
		n = m_clients.find(c.peer_id);
		// The client shouldn't already exist
		assert(n == m_clients.end());

		// Create client
		RemoteClient *client = new RemoteClient();
		client->peer_id = c.peer_id;
		m_clients[client->peer_id] = client;

	} // PEER_ADDED
	else if(c.type == PEER_REMOVED)
	{
		/*
			Delete
		*/

		DeleteClient(c.peer_id, c.timeout?CDR_TIMEOUT:CDR_LEAVE);

	} // PEER_REMOVED
	else
	{
		assert(0);
	}
}

void Server::handlePeerChanges()
{
	DSTACK(__FUNCTION_NAME);
	while(m_peer_change_queue.size() > 0)
	{
		PeerChange c = m_peer_change_queue.pop_front();

		verbosestream<<"Server: Handling peer change: "
				<<"id="<<c.peer_id<<", timeout="<<c.timeout
				<<std::endl;

		handlePeerChange(c);
	}
}

void dedicated_server_loop(Server &server, bool &kill)
{
	DSTACK(__FUNCTION_NAME);

	verbosestream<<"dedicated_server_loop()"<<std::endl;

	IntervalLimiter m_profiler_interval;

	for(;;)
	{
		float steplen = g_settings->getFloat("dedicated_server_step");
		// This is kind of a hack but can be done like this
		// because server.step() is very light
		{
			ScopeProfiler sp(g_profiler, "dedicated server sleep");
			sleep_ms((int)(steplen*1000.0));
		}
		server.step(steplen);

		if(server.getShutdownRequested() || kill)
		{
			infostream<<"Dedicated server quitting"<<std::endl;
#if USE_CURL
			if(g_settings->getBool("server_announce") == true)
				ServerList::sendAnnounce("delete");
#endif
			break;
		}

		/*
			Profiler
		*/
		float profiler_print_interval =
				g_settings->getFloat("profiler_print_interval");
		if(profiler_print_interval != 0)
		{
			if(m_profiler_interval.step(steplen, profiler_print_interval))
			{
				infostream<<"Profiler:"<<std::endl;
				g_profiler->print(infostream);
				g_profiler->clear();
			}
		}
	}
}
