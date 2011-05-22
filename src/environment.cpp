/*
Minetest-c55
Copyright (C) 2010-2011 celeron55, Perttu Ahola <celeron55@gmail.com>

This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License along
with this program; if not, write to the Free Software Foundation, Inc.,
51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
*/

#include "environment.h"
#include "filesys.h"
#include "porting.h"
#include "collision.h"


Environment::Environment():
	m_time_of_day(9000)
{
}

Environment::~Environment()
{
	// Deallocate players
	for(core::list<Player*>::Iterator i = m_players.begin();
			i != m_players.end(); i++)
	{
		delete (*i);
	}
}

void Environment::addPlayer(Player *player)
{
	DSTACK(__FUNCTION_NAME);
	/*
		Check that peer_ids are unique.
		Also check that names are unique.
		Exception: there can be multiple players with peer_id=0
	*/
	// If peer id is non-zero, it has to be unique.
	if(player->peer_id != 0)
		assert(getPlayer(player->peer_id) == NULL);
	// Name has to be unique.
	assert(getPlayer(player->getName()) == NULL);
	// Add.
	m_players.push_back(player);
}

void Environment::removePlayer(u16 peer_id)
{
	DSTACK(__FUNCTION_NAME);
re_search:
	for(core::list<Player*>::Iterator i = m_players.begin();
			i != m_players.end(); i++)
	{
		Player *player = *i;
		if(player->peer_id != peer_id)
			continue;
		
		delete player;
		m_players.erase(i);
		// See if there is an another one
		// (shouldn't be, but just to be sure)
		goto re_search;
	}
}

Player * Environment::getPlayer(u16 peer_id)
{
	for(core::list<Player*>::Iterator i = m_players.begin();
			i != m_players.end(); i++)
	{
		Player *player = *i;
		if(player->peer_id == peer_id)
			return player;
	}
	return NULL;
}

Player * Environment::getPlayer(const char *name)
{
	for(core::list<Player*>::Iterator i = m_players.begin();
			i != m_players.end(); i++)
	{
		Player *player = *i;
		if(strcmp(player->getName(), name) == 0)
			return player;
	}
	return NULL;
}

Player * Environment::getRandomConnectedPlayer()
{
	core::list<Player*> connected_players = getPlayers(true);
	u32 chosen_one = myrand() % connected_players.size();
	u32 j = 0;
	for(core::list<Player*>::Iterator
			i = connected_players.begin();
			i != connected_players.end(); i++)
	{
		if(j == chosen_one)
		{
			Player *player = *i;
			return player;
		}
		j++;
	}
	return NULL;
}

Player * Environment::getNearestConnectedPlayer(v3f pos)
{
	core::list<Player*> connected_players = getPlayers(true);
	f32 nearest_d = 0;
	Player *nearest_player = NULL;
	for(core::list<Player*>::Iterator
			i = connected_players.begin();
			i != connected_players.end(); i++)
	{
		Player *player = *i;
		f32 d = player->getPosition().getDistanceFrom(pos);
		if(d < nearest_d || nearest_player == NULL)
		{
			nearest_d = d;
			nearest_player = player;
		}
	}
	return nearest_player;
}

core::list<Player*> Environment::getPlayers()
{
	return m_players;
}

core::list<Player*> Environment::getPlayers(bool ignore_disconnected)
{
	core::list<Player*> newlist;
	for(core::list<Player*>::Iterator
			i = m_players.begin();
			i != m_players.end(); i++)
	{
		Player *player = *i;
		
		if(ignore_disconnected)
		{
			// Ignore disconnected players
			if(player->peer_id == 0)
				continue;
		}

		newlist.push_back(player);
	}
	return newlist;
}

void Environment::printPlayers(std::ostream &o)
{
	o<<"Players in environment:"<<std::endl;
	for(core::list<Player*>::Iterator i = m_players.begin();
			i != m_players.end(); i++)
	{
		Player *player = *i;
		o<<"Player peer_id="<<player->peer_id<<std::endl;
	}
}

/*void Environment::setDayNightRatio(u32 r)
{
	getDayNightRatio() = r;
}*/

u32 Environment::getDayNightRatio()
{
	//return getDayNightRatio();
	return time_to_daynight_ratio(m_time_of_day);
}

/*
	ActiveBlockList
*/

void fillRadiusBlock(v3s16 p0, s16 r, core::map<v3s16, bool> &list)
{
	v3s16 p;
	for(p.X=p0.X-r; p.X<=p0.X+r; p.X++)
	for(p.Y=p0.Y-r; p.Y<=p0.Y+r; p.Y++)
	for(p.Z=p0.Z-r; p.Z<=p0.Z+r; p.Z++)
	{
		// Set in list
		list[p] = true;
	}
}

void ActiveBlockList::update(core::list<v3s16> &active_positions,
		s16 radius,
		core::map<v3s16, bool> &blocks_removed,
		core::map<v3s16, bool> &blocks_added)
{
	/*
		Create the new list
	*/
	core::map<v3s16, bool> newlist;
	for(core::list<v3s16>::Iterator i = active_positions.begin();
			i != active_positions.end(); i++)
	{
		fillRadiusBlock(*i, radius, newlist);
	}

	/*
		Find out which blocks on the old list are not on the new list
	*/
	// Go through old list
	for(core::map<v3s16, bool>::Iterator i = m_list.getIterator();
			i.atEnd()==false; i++)
	{
		v3s16 p = i.getNode()->getKey();
		// If not on new list, it's been removed
		if(newlist.find(p) == NULL)
			blocks_removed.insert(p, true);
	}

	/*
		Find out which blocks on the new list are not on the old list
	*/
	// Go through new list
	for(core::map<v3s16, bool>::Iterator i = newlist.getIterator();
			i.atEnd()==false; i++)
	{
		v3s16 p = i.getNode()->getKey();
		// If not on old list, it's been added
		if(m_list.find(p) == NULL)
			blocks_added.insert(p, true);
	}

	/*
		Update m_list
	*/
	m_list.clear();
	for(core::map<v3s16, bool>::Iterator i = newlist.getIterator();
			i.atEnd()==false; i++)
	{
		v3s16 p = i.getNode()->getKey();
		m_list.insert(p, true);
	}
}

/*
	ServerEnvironment
*/

ServerEnvironment::ServerEnvironment(ServerMap *map, Server *server):
	m_map(map),
	m_server(server),
	m_random_spawn_timer(3),
	m_send_recommended_timer(0),
	m_game_time(0),
	m_game_time_fraction_counter(0)
{
}

ServerEnvironment::~ServerEnvironment()
{
	// Clear active block list.
	// This makes the next one delete all active objects.
	m_active_blocks.clear();

	// Convert all objects to static and delete the active objects
	deactivateFarObjects(true);

	// Drop/delete map
	m_map->drop();
}

void ServerEnvironment::serializePlayers(const std::string &savedir)
{
	std::string players_path = savedir + "/players";
	fs::CreateDir(players_path);

	core::map<Player*, bool> saved_players;

	std::vector<fs::DirListNode> player_files = fs::GetDirListing(players_path);
	for(u32 i=0; i<player_files.size(); i++)
	{
		if(player_files[i].dir)
			continue;
		
		// Full path to this file
		std::string path = players_path + "/" + player_files[i].name;

		//dstream<<"Checking player file "<<path<<std::endl;

		// Load player to see what is its name
		ServerRemotePlayer testplayer;
		{
			// Open file and deserialize
			std::ifstream is(path.c_str(), std::ios_base::binary);
			if(is.good() == false)
			{
				dstream<<"Failed to read "<<path<<std::endl;
				continue;
			}
			testplayer.deSerialize(is);
		}

		//dstream<<"Loaded test player with name "<<testplayer.getName()<<std::endl;
		
		// Search for the player
		std::string playername = testplayer.getName();
		Player *player = getPlayer(playername.c_str());
		if(player == NULL)
		{
			dstream<<"Didn't find matching player, ignoring file "<<path<<std::endl;
			continue;
		}

		//dstream<<"Found matching player, overwriting."<<std::endl;

		// OK, found. Save player there.
		{
			// Open file and serialize
			std::ofstream os(path.c_str(), std::ios_base::binary);
			if(os.good() == false)
			{
				dstream<<"Failed to overwrite "<<path<<std::endl;
				continue;
			}
			player->serialize(os);
			saved_players.insert(player, true);
		}
	}

	for(core::list<Player*>::Iterator i = m_players.begin();
			i != m_players.end(); i++)
	{
		Player *player = *i;
		if(saved_players.find(player) != NULL)
		{
			/*dstream<<"Player "<<player->getName()
					<<" was already saved."<<std::endl;*/
			continue;
		}
		std::string playername = player->getName();
		// Don't save unnamed player
		if(playername == "")
		{
			//dstream<<"Not saving unnamed player."<<std::endl;
			continue;
		}
		/*
			Find a sane filename
		*/
		if(string_allowed(playername, PLAYERNAME_ALLOWED_CHARS) == false)
			playername = "player";
		std::string path = players_path + "/" + playername;
		bool found = false;
		for(u32 i=0; i<1000; i++)
		{
			if(fs::PathExists(path) == false)
			{
				found = true;
				break;
			}
			path = players_path + "/" + playername + itos(i);
		}
		if(found == false)
		{
			dstream<<"WARNING: Didn't find free file for player"<<std::endl;
			continue;
		}

		{
			/*dstream<<"Saving player "<<player->getName()<<" to "
					<<path<<std::endl;*/
			// Open file and serialize
			std::ofstream os(path.c_str(), std::ios_base::binary);
			if(os.good() == false)
			{
				dstream<<"WARNING: Failed to overwrite "<<path<<std::endl;
				continue;
			}
			player->serialize(os);
			saved_players.insert(player, true);
		}
	}

	//dstream<<"Saved "<<saved_players.size()<<" players."<<std::endl;
}

void ServerEnvironment::deSerializePlayers(const std::string &savedir)
{
	std::string players_path = savedir + "/players";

	core::map<Player*, bool> saved_players;

	std::vector<fs::DirListNode> player_files = fs::GetDirListing(players_path);
	for(u32 i=0; i<player_files.size(); i++)
	{
		if(player_files[i].dir)
			continue;
		
		// Full path to this file
		std::string path = players_path + "/" + player_files[i].name;

		dstream<<"Checking player file "<<path<<std::endl;

		// Load player to see what is its name
		ServerRemotePlayer testplayer;
		{
			// Open file and deserialize
			std::ifstream is(path.c_str(), std::ios_base::binary);
			if(is.good() == false)
			{
				dstream<<"Failed to read "<<path<<std::endl;
				continue;
			}
			testplayer.deSerialize(is);
		}

		dstream<<"Loaded test player with name "<<testplayer.getName()<<std::endl;
		
		// Search for the player
		std::string playername = testplayer.getName();
		Player *player = getPlayer(playername.c_str());
		bool newplayer = false;
		if(player == NULL)
		{
			dstream<<"Is a new player"<<std::endl;
			player = new ServerRemotePlayer();
			newplayer = true;
		}

		// Load player
		{
			dstream<<"Reading player "<<testplayer.getName()<<" from "
					<<path<<std::endl;
			// Open file and deserialize
			std::ifstream is(path.c_str(), std::ios_base::binary);
			if(is.good() == false)
			{
				dstream<<"Failed to read "<<path<<std::endl;
				continue;
			}
			player->deSerialize(is);
		}

		if(newplayer)
			addPlayer(player);
	}
}

void ServerEnvironment::saveMeta(const std::string &savedir)
{
	std::string path = savedir + "/env_meta.txt";

	// Open file and serialize
	std::ofstream os(path.c_str(), std::ios_base::binary);
	if(os.good() == false)
	{
		dstream<<"WARNING: ServerEnvironment::saveMeta(): Failed to open "
				<<path<<std::endl;
		throw SerializationError("Couldn't save env meta");
	}

	Settings args;
	args.setU64("game_time", m_game_time);
	args.setU64("time_of_day", getTimeOfDay());
	args.writeLines(os);
	os<<"EnvArgsEnd\n";
}

void ServerEnvironment::loadMeta(const std::string &savedir)
{
	std::string path = savedir + "/env_meta.txt";

	// Open file and deserialize
	std::ifstream is(path.c_str(), std::ios_base::binary);
	if(is.good() == false)
	{
		dstream<<"WARNING: ServerEnvironment::loadMeta(): Failed to open "
				<<path<<std::endl;
		throw SerializationError("Couldn't load env meta");
	}

	Settings args;
	
	for(;;)
	{
		if(is.eof())
			throw SerializationError
					("ServerEnvironment::loadMeta(): EnvArgsEnd not found");
		std::string line;
		std::getline(is, line);
		std::string trimmedline = trim(line);
		if(trimmedline == "EnvArgsEnd")
			break;
		args.parseConfigLine(line);
	}
	
	try{
		m_game_time = args.getU64("game_time");
	}catch(SettingNotFoundException &e){
		// Getting this is crucial, otherwise timestamps are useless
		throw SerializationError("Couldn't load env meta game_time");
	}

	try{
		m_time_of_day = args.getU64("time_of_day");
	}catch(SettingNotFoundException &e){
		// This is not as important
		m_time_of_day = 9000;
	}
}

#if 0
// This is probably very useless
void spawnRandomObjects(MapBlock *block)
{
	for(s16 z0=0; z0<MAP_BLOCKSIZE; z0++)
	for(s16 x0=0; x0<MAP_BLOCKSIZE; x0++)
	{
		bool last_node_walkable = false;
		for(s16 y0=0; y0<MAP_BLOCKSIZE; y0++)
		{
			v3s16 p(x0,y0,z0);
			MapNode n = block->getNodeNoEx(p);
			if(n.d == CONTENT_IGNORE)
				continue;
			if(content_features(n.d).liquid_type != LIQUID_NONE)
				continue;
			if(content_features(n.d).walkable)
			{
				last_node_walkable = true;
				continue;
			}
			if(last_node_walkable)
			{
				// If block contains light information
				if(content_features(n.d).param_type == CPT_LIGHT)
				{
					if(n.getLight(LIGHTBANK_DAY) <= 5)
					{
						if(myrand() % 1000 == 0)
						{
							v3f pos_f = intToFloat(p+block->getPosRelative(), BS);
							pos_f.Y -= BS*0.4;
							ServerActiveObject *obj = new Oerkki1SAO(NULL,0,pos_f);
							std::string data = obj->getStaticData();
							StaticObject s_obj(obj->getType(),
									obj->getBasePosition(), data);
							// Add one
							block->m_static_objects.insert(0, s_obj);
							delete obj;
							block->setChangedFlag();
						}
					}
				}
			}
			last_node_walkable = false;
		}
	}
}
#endif

void ServerEnvironment::step(float dtime)
{
	DSTACK(__FUNCTION_NAME);
	
	//TimeTaker timer("ServerEnv step");

	// Get some settings
	bool footprints = g_settings.getBool("footprints");

	/*
		Increment game time
	*/
	{
		m_game_time_fraction_counter += dtime;
		u32 inc_i = (u32)m_game_time_fraction_counter;
		m_game_time += inc_i;
		m_game_time_fraction_counter -= (float)inc_i;
	}
	
	/*
		Let map update it's timers
	*/
	{
		//TimeTaker timer("Server m_map->timerUpdate()");
		m_map->timerUpdate(dtime);
	}

	/*
		Handle players
	*/
	for(core::list<Player*>::Iterator i = m_players.begin();
			i != m_players.end(); i++)
	{
		Player *player = *i;
		
		// Ignore disconnected players
		if(player->peer_id == 0)
			continue;

		v3f playerpos = player->getPosition();
		
		// Move
		player->move(dtime, *m_map, 100*BS);
		
		/*
			Add footsteps to grass
		*/
		if(footprints)
		{
			// Get node that is at BS/4 under player
			v3s16 bottompos = floatToInt(playerpos + v3f(0,-BS/4,0), BS);
			try{
				MapNode n = m_map->getNode(bottompos);
				if(n.d == CONTENT_GRASS)
				{
					n.d = CONTENT_GRASS_FOOTSTEPS;
					m_map->setNode(bottompos, n);
				}
			}
			catch(InvalidPositionException &e)
			{
			}
		}
	}

	/*
		Manage active block list
	*/
	if(m_active_blocks_management_interval.step(dtime, 2.0))
	{
		/*
			Get player block positions
		*/
		core::list<v3s16> players_blockpos;
		for(core::list<Player*>::Iterator
				i = m_players.begin();
				i != m_players.end(); i++)
		{
			Player *player = *i;
			// Ignore disconnected players
			if(player->peer_id == 0)
				continue;
			v3s16 blockpos = getNodeBlockPos(
					floatToInt(player->getPosition(), BS));
			players_blockpos.push_back(blockpos);
		}
		
		/*
			Update list of active blocks, collecting changes
		*/
		const s16 active_block_range = 5;
		core::map<v3s16, bool> blocks_removed;
		core::map<v3s16, bool> blocks_added;
		m_active_blocks.update(players_blockpos, active_block_range,
				blocks_removed, blocks_added);

		/*
			Handle removed blocks
		*/

		// Convert active objects that are no more in active blocks to static
		deactivateFarObjects(false);
		
		for(core::map<v3s16, bool>::Iterator
				i = blocks_removed.getIterator();
				i.atEnd()==false; i++)
		{
			v3s16 p = i.getNode()->getKey();

			/*dstream<<"Server: Block ("<<p.X<<","<<p.Y<<","<<p.Z
					<<") became inactive"<<std::endl;*/
			
			MapBlock *block = m_map->getBlockNoCreateNoEx(p);
			if(block==NULL)
				continue;
			
			// Set current time as timestamp
			block->setTimestamp(m_game_time);
		}

		/*
			Handle added blocks
		*/

		for(core::map<v3s16, bool>::Iterator
				i = blocks_added.getIterator();
				i.atEnd()==false; i++)
		{
			v3s16 p = i.getNode()->getKey();
			
			/*dstream<<"Server: Block ("<<p.X<<","<<p.Y<<","<<p.Z
					<<") became active"<<std::endl;*/

			MapBlock *block = m_map->getBlockNoCreateNoEx(p);
			if(block==NULL)
				continue;
			
			// Get time difference
			u32 dtime_s = 0;
			u32 stamp = block->getTimestamp();
			if(m_game_time > stamp && stamp != BLOCK_TIMESTAMP_UNDEFINED)
				dtime_s = m_game_time - block->getTimestamp();

			// Set current time as timestamp
			block->setTimestamp(m_game_time);

			//dstream<<"Block is "<<dtime_s<<" seconds old."<<std::endl;
			
			// Activate stored objects
			activateObjects(block);

			// TODO: Do something
			
			// Here's a quick demonstration
			v3s16 p0;
			for(p0.X=0; p0.X<MAP_BLOCKSIZE; p0.X++)
			for(p0.Y=0; p0.Y<MAP_BLOCKSIZE; p0.Y++)
			for(p0.Z=0; p0.Z<MAP_BLOCKSIZE; p0.Z++)
			{
				v3s16 p = p0 + block->getPosRelative();
				MapNode n = block->getNodeNoEx(p0);
				// Test something:
				// Convert all mud under proper day lighting to grass
				if(n.d == CONTENT_MUD)
				{
					if(1)
					{
						MapNode n_top = block->getNodeNoEx(p0+v3s16(0,1,0));
						if(content_features(n_top.d).walkable == false &&
								n_top.getLight(LIGHTBANK_DAY) >= 13)
						{
							n.d = CONTENT_GRASS;
							m_map->addNodeWithEvent(p, n);
						}
					}
				}
			}
		}
	}

	/*
		Mess around in active blocks
	*/
	if(m_active_blocks_test_interval.step(dtime, 5.0))
	{
		for(core::map<v3s16, bool>::Iterator
				i = m_active_blocks.m_list.getIterator();
				i.atEnd()==false; i++)
		{
			v3s16 p = i.getNode()->getKey();
			
			/*dstream<<"Server: Block ("<<p.X<<","<<p.Y<<","<<p.Z
					<<") being handled"<<std::endl;*/

			MapBlock *block = m_map->getBlockNoCreateNoEx(p);
			if(block==NULL)
				continue;
			
			// Set current time as timestamp
			block->setTimestamp(m_game_time);
			
			/*
				Do stuff!

				Note that map modifications should be done using the event-
				making map methods so that the server gets information
				about them.

				Reading can be done quickly directly from the block.

				Everything should bind to inside this single content
				searching loop to keep things fast.
			*/

			v3s16 p0;
			for(p0.X=0; p0.X<MAP_BLOCKSIZE; p0.X++)
			for(p0.Y=0; p0.Y<MAP_BLOCKSIZE; p0.Y++)
			for(p0.Z=0; p0.Z<MAP_BLOCKSIZE; p0.Z++)
			{
				v3s16 p = p0 + block->getPosRelative();
				MapNode n = block->getNodeNoEx(p0);
				// Test something:
				// Convert mud under proper lighting to grass
				if(n.d == CONTENT_MUD)
				{
					if(myrand()%4 == 0)
					{
						MapNode n_top = block->getNodeNoEx(p0+v3s16(0,1,0));
						if(content_features(n_top.d).walkable == false &&
								n_top.getLightBlend(getDayNightRatio()) >= 13)
						{
							n.d = CONTENT_GRASS;
							m_map->addNodeWithEvent(p, n);
						}
					}
				}
			}
		}
	}
	
	/*
		Step active objects
	*/
	{
		//TimeTaker timer("Step active objects");
		
		// This helps the objects to send data at the same time
		bool send_recommended = false;
		m_send_recommended_timer += dtime;
		if(m_send_recommended_timer > 0.15)
		{
			m_send_recommended_timer = 0;
			send_recommended = true;
		}

		for(core::map<u16, ServerActiveObject*>::Iterator
				i = m_active_objects.getIterator();
				i.atEnd()==false; i++)
		{
			ServerActiveObject* obj = i.getNode()->getValue();
			// Don't step if is to be removed or stored statically
			if(obj->m_removed || obj->m_pending_deactivation)
				continue;
			// Step object, putting messages directly to the queue
			obj->step(dtime, m_active_object_messages, send_recommended);
		}
	}
	
	/*
		Manage active objects
	*/
	if(m_object_management_interval.step(dtime, 0.5))
	{
		/*
			Remove objects that satisfy (m_removed && m_known_by_count==0)
		*/
		removeRemovedObjects();
	}

	if(g_settings.getBool("enable_experimental"))
	{

	/*
		TEST CODE
	*/
#if 1
	m_random_spawn_timer -= dtime;
	if(m_random_spawn_timer < 0)
	{
		//m_random_spawn_timer += myrand_range(2.0, 20.0);
		//m_random_spawn_timer += 2.0;
		m_random_spawn_timer += 200.0;

		/*
			Find some position
		*/

		/*v2s16 p2d(myrand_range(-5,5), myrand_range(-5,5));
		s16 y = 1 + getServerMap().findGroundLevel(p2d);
		v3f pos(p2d.X*BS,y*BS,p2d.Y*BS);*/
		
		Player *player = getRandomConnectedPlayer();
		v3f pos(0,0,0);
		if(player)
			pos = player->getPosition();
		pos += v3f(
			myrand_range(-3,3)*BS,
			0,
			myrand_range(-3,3)*BS
		);

		/*
			Create a ServerActiveObject
		*/

		//TestSAO *obj = new TestSAO(this, 0, pos);
		//ServerActiveObject *obj = new ItemSAO(this, 0, pos, "CraftItem Stick 1");
		//ServerActiveObject *obj = new RatSAO(this, 0, pos);
		ServerActiveObject *obj = new Oerkki1SAO(this, 0, pos);
		addActiveObject(obj);
	}
#endif

	} // enable_experimental
}

ServerActiveObject* ServerEnvironment::getActiveObject(u16 id)
{
	core::map<u16, ServerActiveObject*>::Node *n;
	n = m_active_objects.find(id);
	if(n == NULL)
		return NULL;
	return n->getValue();
}

bool isFreeServerActiveObjectId(u16 id,
		core::map<u16, ServerActiveObject*> &objects)
{
	if(id == 0)
		return false;
	
	for(core::map<u16, ServerActiveObject*>::Iterator
			i = objects.getIterator();
			i.atEnd()==false; i++)
	{
		if(i.getNode()->getKey() == id)
			return false;
	}
	return true;
}

u16 getFreeServerActiveObjectId(
		core::map<u16, ServerActiveObject*> &objects)
{
	u16 new_id = 1;
	for(;;)
	{
		if(isFreeServerActiveObjectId(new_id, objects))
			return new_id;
		
		if(new_id == 65535)
			return 0;

		new_id++;
	}
}

u16 ServerEnvironment::addActiveObject(ServerActiveObject *object)
{
	assert(object);
	if(object->getId() == 0)
	{
		u16 new_id = getFreeServerActiveObjectId(m_active_objects);
		if(new_id == 0)
		{
			dstream<<"WARNING: ServerEnvironment::addActiveObject(): "
					<<"no free ids available"<<std::endl;
			delete object;
			return 0;
		}
		object->setId(new_id);
	}
	if(isFreeServerActiveObjectId(object->getId(), m_active_objects) == false)
	{
		dstream<<"WARNING: ServerEnvironment::addActiveObject(): "
				<<"id is not free ("<<object->getId()<<")"<<std::endl;
		delete object;
		return 0;
	}
	/*dstream<<"INGO: ServerEnvironment::addActiveObject(): "
			<<"added (id="<<object->getId()<<")"<<std::endl;*/
			
	m_active_objects.insert(object->getId(), object);

	// Add static object to active static list of the block
	v3f objectpos = object->getBasePosition();
	std::string staticdata = object->getStaticData();
	StaticObject s_obj(object->getType(), objectpos, staticdata);
	// Add to the block where the object is located in
	v3s16 blockpos = getNodeBlockPos(floatToInt(objectpos, BS));
	MapBlock *block = m_map->getBlockNoCreateNoEx(blockpos);
	if(block)
	{
		block->m_static_objects.m_active.insert(object->getId(), s_obj);
		object->m_static_exists = true;
		object->m_static_block = blockpos;
	}
	else{
		dstream<<"WARNING: Server: Could not find a block for "
				<<"storing newly added static active object"<<std::endl;
	}

	return object->getId();
}

/*
	Finds out what new objects have been added to
	inside a radius around a position
*/
void ServerEnvironment::getAddedActiveObjects(v3s16 pos, s16 radius,
		core::map<u16, bool> &current_objects,
		core::map<u16, bool> &added_objects)
{
	v3f pos_f = intToFloat(pos, BS);
	f32 radius_f = radius * BS;
	/*
		Go through the object list,
		- discard m_removed objects,
		- discard objects that are too far away,
		- discard objects that are found in current_objects.
		- add remaining objects to added_objects
	*/
	for(core::map<u16, ServerActiveObject*>::Iterator
			i = m_active_objects.getIterator();
			i.atEnd()==false; i++)
	{
		u16 id = i.getNode()->getKey();
		// Get object
		ServerActiveObject *object = i.getNode()->getValue();
		if(object == NULL)
			continue;
		// Discard if removed
		if(object->m_removed)
			continue;
		// Discard if too far
		f32 distance_f = object->getBasePosition().getDistanceFrom(pos_f);
		if(distance_f > radius_f)
			continue;
		// Discard if already on current_objects
		core::map<u16, bool>::Node *n;
		n = current_objects.find(id);
		if(n != NULL)
			continue;
		// Add to added_objects
		added_objects.insert(id, false);
	}
}

/*
	Finds out what objects have been removed from
	inside a radius around a position
*/
void ServerEnvironment::getRemovedActiveObjects(v3s16 pos, s16 radius,
		core::map<u16, bool> &current_objects,
		core::map<u16, bool> &removed_objects)
{
	v3f pos_f = intToFloat(pos, BS);
	f32 radius_f = radius * BS;
	/*
		Go through current_objects; object is removed if:
		- object is not found in m_active_objects (this is actually an
		  error condition; objects should be set m_removed=true and removed
		  only after all clients have been informed about removal), or
		- object has m_removed=true, or
		- object is too far away
	*/
	for(core::map<u16, bool>::Iterator
			i = current_objects.getIterator();
			i.atEnd()==false; i++)
	{
		u16 id = i.getNode()->getKey();
		ServerActiveObject *object = getActiveObject(id);
		if(object == NULL)
		{
			dstream<<"WARNING: ServerEnvironment::getRemovedActiveObjects():"
					<<" object in current_objects is NULL"<<std::endl;
		}
		else if(object->m_removed == false)
		{
			f32 distance_f = object->getBasePosition().getDistanceFrom(pos_f);
			/*dstream<<"removed == false"
					<<"distance_f = "<<distance_f
					<<", radius_f = "<<radius_f<<std::endl;*/
			if(distance_f < radius_f)
			{
				// Not removed
				continue;
			}
		}
		removed_objects.insert(id, false);
	}
}

ActiveObjectMessage ServerEnvironment::getActiveObjectMessage()
{
	if(m_active_object_messages.size() == 0)
		return ActiveObjectMessage(0);
	
	return m_active_object_messages.pop_front();
}

/*
	************ Private methods *************
*/

/*
	Remove objects that satisfy (m_removed && m_known_by_count==0)
*/
void ServerEnvironment::removeRemovedObjects()
{
	core::list<u16> objects_to_remove;
	for(core::map<u16, ServerActiveObject*>::Iterator
			i = m_active_objects.getIterator();
			i.atEnd()==false; i++)
	{
		u16 id = i.getNode()->getKey();
		ServerActiveObject* obj = i.getNode()->getValue();
		// This shouldn't happen but check it
		if(obj == NULL)
		{
			dstream<<"WARNING: NULL object found in ServerEnvironment"
					<<" while finding removed objects. id="<<id<<std::endl;
			// Id to be removed from m_active_objects
			objects_to_remove.push_back(id);
			continue;
		}

		/*
			We will delete objects that are marked as removed or thatare
			waiting for deletion after deactivation
		*/
		if(obj->m_removed == false && obj->m_pending_deactivation == false)
			continue;

		/*
			Delete static data from block if is marked as removed
		*/
		if(obj->m_static_exists && obj->m_removed)
		{
			MapBlock *block = m_map->getBlockNoCreateNoEx(obj->m_static_block);
			if(block)
			{
				block->m_static_objects.remove(id);
				block->setChangedFlag();
			}
		}

		// If m_known_by_count > 0, don't actually remove.
		if(obj->m_known_by_count > 0)
			continue;
		
		// Delete
		delete obj;
		// Id to be removed from m_active_objects
		objects_to_remove.push_back(id);
	}
	// Remove references from m_active_objects
	for(core::list<u16>::Iterator i = objects_to_remove.begin();
			i != objects_to_remove.end(); i++)
	{
		m_active_objects.remove(*i);
	}
}

/*
	Convert stored objects from blocks near the players to active.
*/
void ServerEnvironment::activateObjects(MapBlock *block)
{
	if(block==NULL)
		return;
	// Ignore if no stored objects (to not set changed flag)
	if(block->m_static_objects.m_stored.size() == 0)
		return;
	// A list for objects that couldn't be converted to static for some
	// reason. They will be stored back.
	core::list<StaticObject> new_stored;
	// Loop through stored static objects
	for(core::list<StaticObject>::Iterator
			i = block->m_static_objects.m_stored.begin();
			i != block->m_static_objects.m_stored.end(); i++)
	{
		/*dstream<<"INFO: Server: Creating an active object from "
				<<"static data"<<std::endl;*/
		StaticObject &s_obj = *i;
		// Create an active object from the data
		ServerActiveObject *obj = ServerActiveObject::create
				(s_obj.type, this, 0, s_obj.pos, s_obj.data);
		// If couldn't create object, store static data back.
		if(obj==NULL)
		{
			new_stored.push_back(s_obj);
			continue;
		}
		// This will also add the object to the active static list
		addActiveObject(obj);
		//u16 id = addActiveObject(obj);
	}
	// Clear stored list
	block->m_static_objects.m_stored.clear();
	// Add leftover failed stuff to stored list
	for(core::list<StaticObject>::Iterator
			i = new_stored.begin();
			i != new_stored.end(); i++)
	{
		StaticObject &s_obj = *i;
		block->m_static_objects.m_stored.push_back(s_obj);
	}
	// Block has been modified
	block->setChangedFlag();
}

/*
	Convert objects that are not in active blocks to static.

	If m_known_by_count != 0, active object is not deleted, but static
	data is still updated.

	If force_delete is set, active object is deleted nevertheless. It
	shall only be set so in the destructor of the environment.
*/
void ServerEnvironment::deactivateFarObjects(bool force_delete)
{
	core::list<u16> objects_to_remove;
	for(core::map<u16, ServerActiveObject*>::Iterator
			i = m_active_objects.getIterator();
			i.atEnd()==false; i++)
	{
		ServerActiveObject* obj = i.getNode()->getValue();
		u16 id = i.getNode()->getKey();
		v3f objectpos = obj->getBasePosition();

		// This shouldn't happen but check it
		if(obj == NULL)
		{
			dstream<<"WARNING: NULL object found in ServerEnvironment"
					<<std::endl;
			assert(0);
			continue;
		}

		// The block in which the object resides in
		v3s16 blockpos_o = getNodeBlockPos(floatToInt(objectpos, BS));
		
		// If block is active, don't remove
		if(m_active_blocks.contains(blockpos_o))
			continue;

		/*
			Update the static data
		*/

		// Delete old static object
		MapBlock *oldblock = NULL;
		if(obj->m_static_exists)
		{
			MapBlock *block = m_map->getBlockNoCreateNoEx
					(obj->m_static_block);
			if(block)
			{
				block->m_static_objects.remove(id);
				oldblock = block;
			}
		}
		// Create new static object
		std::string staticdata = obj->getStaticData();
		StaticObject s_obj(obj->getType(), objectpos, staticdata);
		// Add to the block where the object is located in
		v3s16 blockpos = getNodeBlockPos(floatToInt(objectpos, BS));
		MapBlock *block = m_map->getBlockNoCreateNoEx(blockpos);
		if(block)
		{
			block->m_static_objects.insert(0, s_obj);
			block->setChangedFlag();
			obj->m_static_exists = true;
			obj->m_static_block = block->getPos();
		}
		// If not possible, add back to previous block
		else if(oldblock)
		{
			oldblock->m_static_objects.insert(0, s_obj);
			oldblock->setChangedFlag();
			obj->m_static_exists = true;
			obj->m_static_block = oldblock->getPos();
		}
		else{
			dstream<<"WARNING: Server: Could not find a block for "
					<<"storing static object"<<std::endl;
			obj->m_static_exists = false;
			continue;
		}

		/*
			Delete active object if not known by some client,
			else set pending deactivation
		*/

		// If known by some client, don't delete.
		if(obj->m_known_by_count > 0 && force_delete == false)
		{
			obj->m_pending_deactivation = true;
			continue;
		}
		
		/*dstream<<"INFO: Server: Stored static data. Deleting object."
				<<std::endl;*/
		// Delete active object
		delete obj;
		// Id to be removed from m_active_objects
		objects_to_remove.push_back(id);
	}

	// Remove references from m_active_objects
	for(core::list<u16>::Iterator i = objects_to_remove.begin();
			i != objects_to_remove.end(); i++)
	{
		m_active_objects.remove(*i);
	}
}


#ifndef SERVER

/*
	ClientEnvironment
*/

ClientEnvironment::ClientEnvironment(ClientMap *map, scene::ISceneManager *smgr):
	m_map(map),
	m_smgr(smgr)
{
	assert(m_map);
	assert(m_smgr);
}

ClientEnvironment::~ClientEnvironment()
{
	// delete active objects
	for(core::map<u16, ClientActiveObject*>::Iterator
			i = m_active_objects.getIterator();
			i.atEnd()==false; i++)
	{
		delete i.getNode()->getValue();
	}

	// Drop/delete map
	m_map->drop();
}

void ClientEnvironment::addPlayer(Player *player)
{
	DSTACK(__FUNCTION_NAME);
	/*
		It is a failure if player is local and there already is a local
		player
	*/
	assert(!(player->isLocal() == true && getLocalPlayer() != NULL));

	Environment::addPlayer(player);
}

LocalPlayer * ClientEnvironment::getLocalPlayer()
{
	for(core::list<Player*>::Iterator i = m_players.begin();
			i != m_players.end(); i++)
	{
		Player *player = *i;
		if(player->isLocal())
			return (LocalPlayer*)player;
	}
	return NULL;
}

void ClientEnvironment::step(float dtime)
{
	DSTACK(__FUNCTION_NAME);

	// Get some settings
	bool free_move = g_settings.getBool("free_move");
	bool footprints = g_settings.getBool("footprints");

	{
		//TimeTaker timer("Client m_map->timerUpdate()");
		m_map->timerUpdate(dtime);
	}
	
	// Get local player
	LocalPlayer *lplayer = getLocalPlayer();
	assert(lplayer);
	// collision info queue
	core::list<CollisionInfo> player_collisions;
	
	/*
		Get the speed the player is going
	*/
	f32 player_speed = 0.001; // just some small value
	player_speed = lplayer->getSpeed().getLength();
	
	/*
		Maximum position increment
	*/
	//f32 position_max_increment = 0.05*BS;
	f32 position_max_increment = 0.1*BS;

	// Maximum time increment (for collision detection etc)
	// time = distance / speed
	f32 dtime_max_increment = position_max_increment / player_speed;
	
	// Maximum time increment is 10ms or lower
	if(dtime_max_increment > 0.01)
		dtime_max_increment = 0.01;
	
	// Don't allow overly huge dtime
	if(dtime > 0.5)
		dtime = 0.5;
	
	f32 dtime_downcount = dtime;

	/*
		Stuff that has a maximum time increment
	*/

	u32 loopcount = 0;
	do
	{
		loopcount++;

		f32 dtime_part;
		if(dtime_downcount > dtime_max_increment)
		{
			dtime_part = dtime_max_increment;
			dtime_downcount -= dtime_part;
		}
		else
		{
			dtime_part = dtime_downcount;
			/*
				Setting this to 0 (no -=dtime_part) disables an infinite loop
				when dtime_part is so small that dtime_downcount -= dtime_part
				does nothing
			*/
			dtime_downcount = 0;
		}
		
		/*
			Handle local player
		*/
		
		{
			v3f lplayerpos = lplayer->getPosition();
			
			// Apply physics
			if(free_move == false)
			{
				// Gravity
				v3f speed = lplayer->getSpeed();
				if(lplayer->swimming_up == false)
					speed.Y -= 9.81 * BS * dtime_part * 2;

				// Water resistance
				if(lplayer->in_water_stable || lplayer->in_water)
				{
					f32 max_down = 2.0*BS;
					if(speed.Y < -max_down) speed.Y = -max_down;

					f32 max = 2.5*BS;
					if(speed.getLength() > max)
					{
						speed = speed / speed.getLength() * max;
					}
				}

				lplayer->setSpeed(speed);
			}

			/*
				Move the lplayer.
				This also does collision detection.
			*/
			lplayer->move(dtime_part, *m_map, position_max_increment,
					&player_collisions);
		}
	}
	while(dtime_downcount > 0.001);
		
	//std::cout<<"Looped "<<loopcount<<" times."<<std::endl;

	for(core::list<CollisionInfo>::Iterator
			i = player_collisions.begin();
			i != player_collisions.end(); i++)
	{
		CollisionInfo &info = *i;
		if(info.t == COLLISION_FALL)
		{
			//f32 tolerance = BS*10; // 2 without damage
			f32 tolerance = BS*12; // 3 without damage
			f32 factor = 1;
			if(info.speed > tolerance)
			{
				f32 damage_f = (info.speed - tolerance)/BS*factor;
				u16 damage = (u16)(damage_f+0.5);
				if(lplayer->hp > damage)
					lplayer->hp -= damage;
				else
					lplayer->hp = 0;

				ClientEnvEvent event;
				event.type = CEE_PLAYER_DAMAGE;
				event.player_damage.amount = damage;
				m_client_event_queue.push_back(event);
			}
		}
	}
	
	/*
		Stuff that can be done in an arbitarily large dtime
	*/
	for(core::list<Player*>::Iterator i = m_players.begin();
			i != m_players.end(); i++)
	{
		Player *player = *i;
		v3f playerpos = player->getPosition();
		
		/*
			Handle non-local players
		*/
		if(player->isLocal() == false)
		{
			// Move
			player->move(dtime, *m_map, 100*BS);

			// Update lighting on remote players on client
			u8 light = LIGHT_MAX;
			try{
				// Get node at head
				v3s16 p = floatToInt(playerpos + v3f(0,BS+BS/2,0), BS);
				MapNode n = m_map->getNode(p);
				light = n.getLightBlend(getDayNightRatio());
			}
			catch(InvalidPositionException &e) {}
			player->updateLight(light);
		}
		
		/*
			Add footsteps to grass
		*/
		if(footprints)
		{
			// Get node that is at BS/4 under player
			v3s16 bottompos = floatToInt(playerpos + v3f(0,-BS/4,0), BS);
			try{
				MapNode n = m_map->getNode(bottompos);
				if(n.d == CONTENT_GRASS)
				{
					n.d = CONTENT_GRASS_FOOTSTEPS;
					m_map->setNode(bottompos, n);
					// Update mesh on client
					if(m_map->mapType() == MAPTYPE_CLIENT)
					{
						v3s16 p_blocks = getNodeBlockPos(bottompos);
						MapBlock *b = m_map->getBlockNoCreate(p_blocks);
						//b->updateMesh(getDayNightRatio());
						b->setMeshExpired(true);
					}
				}
			}
			catch(InvalidPositionException &e)
			{
			}
		}
	}
	
	/*
		Step active objects and update lighting of them
	*/
	
	for(core::map<u16, ClientActiveObject*>::Iterator
			i = m_active_objects.getIterator();
			i.atEnd()==false; i++)
	{
		ClientActiveObject* obj = i.getNode()->getValue();
		// Step object
		obj->step(dtime, this);
		// Update lighting
		//u8 light = LIGHT_MAX;
		u8 light = 0;
		try{
			// Get node at head
			v3s16 p = obj->getLightPosition();
			MapNode n = m_map->getNode(p);
			light = n.getLightBlend(getDayNightRatio());
		}
		catch(InvalidPositionException &e) {}
		obj->updateLight(light);
	}
}

void ClientEnvironment::updateMeshes(v3s16 blockpos)
{
	m_map->updateMeshes(blockpos, getDayNightRatio());
}

void ClientEnvironment::expireMeshes(bool only_daynight_diffed)
{
	m_map->expireMeshes(only_daynight_diffed);
}

ClientActiveObject* ClientEnvironment::getActiveObject(u16 id)
{
	core::map<u16, ClientActiveObject*>::Node *n;
	n = m_active_objects.find(id);
	if(n == NULL)
		return NULL;
	return n->getValue();
}

bool isFreeClientActiveObjectId(u16 id,
		core::map<u16, ClientActiveObject*> &objects)
{
	if(id == 0)
		return false;
	
	for(core::map<u16, ClientActiveObject*>::Iterator
			i = objects.getIterator();
			i.atEnd()==false; i++)
	{
		if(i.getNode()->getKey() == id)
			return false;
	}
	return true;
}

u16 getFreeClientActiveObjectId(
		core::map<u16, ClientActiveObject*> &objects)
{
	u16 new_id = 1;
	for(;;)
	{
		if(isFreeClientActiveObjectId(new_id, objects))
			return new_id;
		
		if(new_id == 65535)
			return 0;

		new_id++;
	}
}

u16 ClientEnvironment::addActiveObject(ClientActiveObject *object)
{
	assert(object);
	if(object->getId() == 0)
	{
		u16 new_id = getFreeClientActiveObjectId(m_active_objects);
		if(new_id == 0)
		{
			dstream<<"WARNING: ClientEnvironment::addActiveObject(): "
					<<"no free ids available"<<std::endl;
			delete object;
			return 0;
		}
		object->setId(new_id);
	}
	if(isFreeClientActiveObjectId(object->getId(), m_active_objects) == false)
	{
		dstream<<"WARNING: ClientEnvironment::addActiveObject(): "
				<<"id is not free ("<<object->getId()<<")"<<std::endl;
		delete object;
		return 0;
	}
	dstream<<"INGO: ClientEnvironment::addActiveObject(): "
			<<"added (id="<<object->getId()<<")"<<std::endl;
	m_active_objects.insert(object->getId(), object);
	object->addToScene(m_smgr);
	return object->getId();
}

void ClientEnvironment::addActiveObject(u16 id, u8 type,
		const std::string &init_data)
{
	ClientActiveObject* obj = ClientActiveObject::create(type);
	if(obj == NULL)
	{
		dstream<<"WARNING: ClientEnvironment::addActiveObject(): "
				<<"id="<<id<<" type="<<type<<": Couldn't create object"
				<<std::endl;
		return;
	}
	
	obj->setId(id);

	addActiveObject(obj);

	obj->initialize(init_data);
}

void ClientEnvironment::removeActiveObject(u16 id)
{
	dstream<<"ClientEnvironment::removeActiveObject(): "
			<<"id="<<id<<std::endl;
	ClientActiveObject* obj = getActiveObject(id);
	if(obj == NULL)
	{
		dstream<<"WARNING: ClientEnvironment::removeActiveObject(): "
				<<"id="<<id<<" not found"<<std::endl;
		return;
	}
	obj->removeFromScene();
	delete obj;
	m_active_objects.remove(id);
}

void ClientEnvironment::processActiveObjectMessage(u16 id,
		const std::string &data)
{
	ClientActiveObject* obj = getActiveObject(id);
	if(obj == NULL)
	{
		dstream<<"WARNING: ClientEnvironment::processActiveObjectMessage():"
				<<" got message for id="<<id<<", which doesn't exist."
				<<std::endl;
		return;
	}
	obj->processMessage(data);
}

/*
	Callbacks for activeobjects
*/

void ClientEnvironment::damageLocalPlayer(u8 damage)
{
	LocalPlayer *lplayer = getLocalPlayer();
	assert(lplayer);

	if(lplayer->hp > damage)
		lplayer->hp -= damage;
	else
		lplayer->hp = 0;

	ClientEnvEvent event;
	event.type = CEE_PLAYER_DAMAGE;
	event.player_damage.amount = damage;
	m_client_event_queue.push_back(event);
}

/*
	Client likes to call these
*/
	
void ClientEnvironment::getActiveObjects(v3f origin, f32 max_d,
		core::array<DistanceSortedActiveObject> &dest)
{
	for(core::map<u16, ClientActiveObject*>::Iterator
			i = m_active_objects.getIterator();
			i.atEnd()==false; i++)
	{
		ClientActiveObject* obj = i.getNode()->getValue();

		f32 d = (obj->getPosition() - origin).getLength();

		if(d > max_d)
			continue;

		DistanceSortedActiveObject dso(obj, d);

		dest.push_back(dso);
	}
}

ClientEnvEvent ClientEnvironment::getClientEvent()
{
	if(m_client_event_queue.size() == 0)
	{
		ClientEnvEvent event;
		event.type = CEE_NONE;
		return event;
	}
	return m_client_event_queue.pop_front();
}

#endif // #ifndef SERVER


