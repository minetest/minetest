/*
Minetest-c55
Copyright (C) 2010-2011 celeron55, Perttu Ahola <celeron55@gmail.com>

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

#include <set>
#include <list>
#include <map>
#include "environment.h"
#include "filesys.h"
#include "porting.h"
#include "collision.h"
#include "content_mapnode.h"
#include "mapblock.h"
#include "serverobject.h"
#include "content_sao.h"
#include "mapgen.h"
#include "settings.h"
#include "log.h"
#include "profiler.h"
#include "scriptapi.h"
#include "nodedef.h"
#include "nodemetadata.h"
#include "main.h" // For g_settings, g_profiler
#include "gamedef.h"
#ifndef SERVER
#include "clientmap.h"
#include "localplayer.h"
#endif
#include "daynightratio.h"
#include "map.h"

#define PP(x) "("<<(x).X<<","<<(x).Y<<","<<(x).Z<<")"

Environment::Environment():
	m_time_of_day(9000),
	m_time_of_day_f(9000./24000),
	m_time_of_day_speed(0),
	m_time_counter(0)
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

u32 Environment::getDayNightRatio()
{
	return time_to_daynight_ratio(m_time_of_day);
}

void Environment::stepTimeOfDay(float dtime)
{
	m_time_counter += dtime;
	f32 speed = m_time_of_day_speed * 24000./(24.*3600);
	u32 units = (u32)(m_time_counter*speed);
	m_time_counter -= (f32)units / speed;
	bool sync_f = false;
	if(units > 0){
		// Sync at overflow
		if(m_time_of_day + units >= 24000)
			sync_f = true;
		m_time_of_day = (m_time_of_day + units) % 24000;
		if(sync_f)
			m_time_of_day_f = (float)m_time_of_day / 24000.0;
	}
	if(!sync_f){
		m_time_of_day_f += m_time_of_day_speed/24/3600*dtime;
		if(m_time_of_day_f > 1.0)
			m_time_of_day_f -= 1.0;
		if(m_time_of_day_f < 0.0)
			m_time_of_day_f += 1.0;
	}
}

/*
	ABMWithState
*/

ABMWithState::ABMWithState(ActiveBlockModifier *abm_):
	abm(abm_),
	timer(0)
{
	// Initialize timer to random value to spread processing
	float itv = abm->getTriggerInterval();
	itv = MYMAX(0.001, itv); // No less than 1ms
	int minval = MYMAX(-0.51*itv, -60); // Clamp to
	int maxval = MYMIN(0.51*itv, 60);   // +-60 seconds
	timer = myrand_range(minval, maxval);
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

ServerEnvironment::ServerEnvironment(ServerMap *map, lua_State *L,
		IGameDef *gamedef, IBackgroundBlockEmerger *emerger):
	m_map(map),
	m_lua(L),
	m_gamedef(gamedef),
	m_emerger(emerger),
	m_random_spawn_timer(3),
	m_send_recommended_timer(0),
	m_active_block_interval_overload_skip(0),
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

	// Delete ActiveBlockModifiers
	for(core::list<ABMWithState>::Iterator
			i = m_abms.begin(); i != m_abms.end(); i++){
		delete i->abm;
	}
}

Map & ServerEnvironment::getMap()
{
	return *m_map;
}

ServerMap & ServerEnvironment::getServerMap()
{
	return *m_map;
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

		//infostream<<"Checking player file "<<path<<std::endl;

		// Load player to see what is its name
		RemotePlayer testplayer(m_gamedef);
		{
			// Open file and deserialize
			std::ifstream is(path.c_str(), std::ios_base::binary);
			if(is.good() == false)
			{
				infostream<<"Failed to read "<<path<<std::endl;
				continue;
			}
			testplayer.deSerialize(is);
		}

		//infostream<<"Loaded test player with name "<<testplayer.getName()<<std::endl;
		
		// Search for the player
		std::string playername = testplayer.getName();
		Player *player = getPlayer(playername.c_str());
		if(player == NULL)
		{
			infostream<<"Didn't find matching player, ignoring file "<<path<<std::endl;
			continue;
		}

		//infostream<<"Found matching player, overwriting."<<std::endl;

		// OK, found. Save player there.
		{
			// Open file and serialize
			std::ofstream os(path.c_str(), std::ios_base::binary);
			if(os.good() == false)
			{
				infostream<<"Failed to overwrite "<<path<<std::endl;
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
			/*infostream<<"Player "<<player->getName()
					<<" was already saved."<<std::endl;*/
			continue;
		}
		std::string playername = player->getName();
		// Don't save unnamed player
		if(playername == "")
		{
			//infostream<<"Not saving unnamed player."<<std::endl;
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
			infostream<<"Didn't find free file for player"<<std::endl;
			continue;
		}

		{
			/*infostream<<"Saving player "<<player->getName()<<" to "
					<<path<<std::endl;*/
			// Open file and serialize
			std::ofstream os(path.c_str(), std::ios_base::binary);
			if(os.good() == false)
			{
				infostream<<"Failed to overwrite "<<path<<std::endl;
				continue;
			}
			player->serialize(os);
			saved_players.insert(player, true);
		}
	}

	//infostream<<"Saved "<<saved_players.size()<<" players."<<std::endl;
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

		//infostream<<"Checking player file "<<path<<std::endl;

		// Load player to see what is its name
		RemotePlayer testplayer(m_gamedef);
		{
			// Open file and deserialize
			std::ifstream is(path.c_str(), std::ios_base::binary);
			if(is.good() == false)
			{
				infostream<<"Failed to read "<<path<<std::endl;
				continue;
			}
			testplayer.deSerialize(is);
		}

		if(!string_allowed(testplayer.getName(), PLAYERNAME_ALLOWED_CHARS))
		{
			infostream<<"Not loading player with invalid name: "
					<<testplayer.getName()<<std::endl;
		}

		/*infostream<<"Loaded test player with name "<<testplayer.getName()
				<<std::endl;*/
		
		// Search for the player
		std::string playername = testplayer.getName();
		Player *player = getPlayer(playername.c_str());
		bool newplayer = false;
		if(player == NULL)
		{
			//infostream<<"Is a new player"<<std::endl;
			player = new RemotePlayer(m_gamedef);
			newplayer = true;
		}

		// Load player
		{
			verbosestream<<"Reading player "<<testplayer.getName()<<" from "
					<<path<<std::endl;
			// Open file and deserialize
			std::ifstream is(path.c_str(), std::ios_base::binary);
			if(is.good() == false)
			{
				infostream<<"Failed to read "<<path<<std::endl;
				continue;
			}
			player->deSerialize(is);
		}

		if(newplayer)
		{
			addPlayer(player);
		}
	}
}

void ServerEnvironment::saveMeta(const std::string &savedir)
{
	std::string path = savedir + "/env_meta.txt";

	// Open file and serialize
	std::ofstream os(path.c_str(), std::ios_base::binary);
	if(os.good() == false)
	{
		infostream<<"ServerEnvironment::saveMeta(): Failed to open "
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
		infostream<<"ServerEnvironment::loadMeta(): Failed to open "
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

struct ActiveABM
{
	ActiveBlockModifier *abm;
	int chance;
	std::set<content_t> required_neighbors;
};

class ABMHandler
{
private:
	ServerEnvironment *m_env;
	std::map<content_t, std::list<ActiveABM> > m_aabms;
public:
	ABMHandler(core::list<ABMWithState> &abms,
			float dtime_s, ServerEnvironment *env,
			bool use_timers):
		m_env(env)
	{
		if(dtime_s < 0.001)
			return;
		INodeDefManager *ndef = env->getGameDef()->ndef();
		for(core::list<ABMWithState>::Iterator
				i = abms.begin(); i != abms.end(); i++){
			ActiveBlockModifier *abm = i->abm;
			float trigger_interval = abm->getTriggerInterval();
			if(trigger_interval < 0.001)
				trigger_interval = 0.001;
			float actual_interval = dtime_s;
			if(use_timers){
				i->timer += dtime_s;
				if(i->timer < trigger_interval)
					continue;
				i->timer -= trigger_interval;
				actual_interval = trigger_interval;
			}
			float intervals = actual_interval / trigger_interval;
			if(intervals == 0)
				continue;
			float chance = abm->getTriggerChance();
			if(chance == 0)
				chance = 1;
			ActiveABM aabm;
			aabm.abm = abm;
			aabm.chance = chance / intervals;
			if(aabm.chance == 0)
				aabm.chance = 1;
			// Trigger neighbors
			std::set<std::string> required_neighbors_s
					= abm->getRequiredNeighbors();
			for(std::set<std::string>::iterator
					i = required_neighbors_s.begin();
					i != required_neighbors_s.end(); i++)
			{
				ndef->getIds(*i, aabm.required_neighbors);
			}
			// Trigger contents
			std::set<std::string> contents_s = abm->getTriggerContents();
			for(std::set<std::string>::iterator
					i = contents_s.begin(); i != contents_s.end(); i++)
			{
				std::set<content_t> ids;
				ndef->getIds(*i, ids);
				for(std::set<content_t>::const_iterator k = ids.begin();
						k != ids.end(); k++)
				{
					content_t c = *k;
					std::map<content_t, std::list<ActiveABM> >::iterator j;
					j = m_aabms.find(c);
					if(j == m_aabms.end()){
						std::list<ActiveABM> aabmlist;
						m_aabms[c] = aabmlist;
						j = m_aabms.find(c);
					}
					j->second.push_back(aabm);
				}
			}
		}
	}
	void apply(MapBlock *block)
	{
		if(m_aabms.empty())
			return;

		ServerMap *map = &m_env->getServerMap();

		v3s16 p0;
		for(p0.X=0; p0.X<MAP_BLOCKSIZE; p0.X++)
		for(p0.Y=0; p0.Y<MAP_BLOCKSIZE; p0.Y++)
		for(p0.Z=0; p0.Z<MAP_BLOCKSIZE; p0.Z++)
		{
			MapNode n = block->getNodeNoEx(p0);
			content_t c = n.getContent();
			v3s16 p = p0 + block->getPosRelative();

			std::map<content_t, std::list<ActiveABM> >::iterator j;
			j = m_aabms.find(c);
			if(j == m_aabms.end())
				continue;

			for(std::list<ActiveABM>::iterator
					i = j->second.begin(); i != j->second.end(); i++)
			{
				if(myrand() % i->chance != 0)
					continue;

				// Check neighbors
				if(!i->required_neighbors.empty())
				{
					v3s16 p1;
					for(p1.X = p.X-1; p1.X <= p.X+1; p1.X++)
					for(p1.Y = p.Y-1; p1.Y <= p.Y+1; p1.Y++)
					for(p1.Z = p.Z-1; p1.Z <= p.Z+1; p1.Z++)
					{
						if(p1 == p)
							continue;
						MapNode n = map->getNodeNoEx(p1);
						content_t c = n.getContent();
						std::set<content_t>::const_iterator k;
						k = i->required_neighbors.find(c);
						if(k != i->required_neighbors.end()){
							goto neighbor_found;
						}
					}
					// No required neighbor found
					continue;
				}
neighbor_found:

				// Find out how many objects the block contains
				u32 active_object_count = block->m_static_objects.m_active.size();
				// Find out how many objects this and all the neighbors contain
				u32 active_object_count_wider = 0;
				for(s16 x=-1; x<=1; x++)
				for(s16 y=-1; y<=1; y++)
				for(s16 z=-1; z<=1; z++)
				{
					MapBlock *block2 = map->getBlockNoCreateNoEx(
							block->getPos() + v3s16(x,y,z));
					if(block2==NULL)
						continue;
					active_object_count_wider +=
							block2->m_static_objects.m_active.size()
							+ block2->m_static_objects.m_stored.size();
				}

				// Call all the trigger variations
				i->abm->trigger(m_env, p, n);
				i->abm->trigger(m_env, p, n,
						active_object_count, active_object_count_wider);
			}
		}
	}
};

void ServerEnvironment::activateBlock(MapBlock *block, u32 additional_dtime)
{
	// Get time difference
	u32 dtime_s = 0;
	u32 stamp = block->getTimestamp();
	if(m_game_time > stamp && stamp != BLOCK_TIMESTAMP_UNDEFINED)
		dtime_s = m_game_time - block->getTimestamp();
	dtime_s += additional_dtime;

	/*infostream<<"ServerEnvironment::activateBlock(): block timestamp: "
			<<stamp<<", game time: "<<m_game_time<<std::endl;*/

	// Set current time as timestamp
	block->setTimestampNoChangedFlag(m_game_time);

	/*infostream<<"ServerEnvironment::activateBlock(): block is "
			<<dtime_s<<" seconds old."<<std::endl;*/
	
	// Activate stored objects
	activateObjects(block);

	// Run node timers
	std::map<v3s16, NodeTimer> elapsed_timers =
		block->m_node_timers.step((float)dtime_s);
	if(!elapsed_timers.empty()){
		MapNode n;
		for(std::map<v3s16, NodeTimer>::iterator
				i = elapsed_timers.begin();
				i != elapsed_timers.end(); i++){
			n = block->getNodeNoEx(i->first);
			if(scriptapi_node_on_timer(m_lua,i->first,n,i->second.elapsed))
				block->setNodeTimer(i->first,NodeTimer(i->second.timeout,0));
		}
	}

	/* Handle ActiveBlockModifiers */
	ABMHandler abmhandler(m_abms, dtime_s, this, false);
	abmhandler.apply(block);
}

void ServerEnvironment::addActiveBlockModifier(ActiveBlockModifier *abm)
{
	m_abms.push_back(ABMWithState(abm));
}

std::set<u16> ServerEnvironment::getObjectsInsideRadius(v3f pos, float radius)
{
	std::set<u16> objects;
	for(core::map<u16, ServerActiveObject*>::Iterator
			i = m_active_objects.getIterator();
			i.atEnd()==false; i++)
	{
		ServerActiveObject* obj = i.getNode()->getValue();
		u16 id = i.getNode()->getKey();
		v3f objectpos = obj->getBasePosition();
		if(objectpos.getDistanceFrom(pos) > radius)
			continue;
		objects.insert(id);
	}
	return objects;
}

void ServerEnvironment::clearAllObjects()
{
	infostream<<"ServerEnvironment::clearAllObjects(): "
			<<"Removing all active objects"<<std::endl;
	core::list<u16> objects_to_remove;
	for(core::map<u16, ServerActiveObject*>::Iterator
			i = m_active_objects.getIterator();
			i.atEnd()==false; i++)
	{
		ServerActiveObject* obj = i.getNode()->getValue();
		if(obj->getType() == ACTIVEOBJECT_TYPE_PLAYER)
			continue;
		u16 id = i.getNode()->getKey();		
		v3f objectpos = obj->getBasePosition();	
		// Delete static object if block is loaded
		if(obj->m_static_exists){
			MapBlock *block = m_map->getBlockNoCreateNoEx(obj->m_static_block);
			if(block){
				block->m_static_objects.remove(id);
				block->raiseModified(MOD_STATE_WRITE_NEEDED,
						"clearAllObjects");
				obj->m_static_exists = false;
			}
		}
		// If known by some client, don't delete immediately
		if(obj->m_known_by_count > 0){
			obj->m_pending_deactivation = true;
			obj->m_removed = true;
			continue;
		}

		// Tell the object about removal
		obj->removingFromEnvironment();
		// Deregister in scripting api
		scriptapi_rm_object_reference(m_lua, obj);

		// Delete active object
		if(obj->environmentDeletes())
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

	core::list<v3s16> loadable_blocks;
	infostream<<"ServerEnvironment::clearAllObjects(): "
			<<"Listing all loadable blocks"<<std::endl;
	m_map->listAllLoadableBlocks(loadable_blocks);
	infostream<<"ServerEnvironment::clearAllObjects(): "
			<<"Done listing all loadable blocks: "
			<<loadable_blocks.size()
			<<", now clearing"<<std::endl;
	u32 report_interval = loadable_blocks.size() / 10;
	u32 num_blocks_checked = 0;
	u32 num_blocks_cleared = 0;
	u32 num_objs_cleared = 0;
	for(core::list<v3s16>::Iterator i = loadable_blocks.begin();
			i != loadable_blocks.end(); i++)
	{
		v3s16 p = *i;
		MapBlock *block = m_map->emergeBlock(p, false);
		if(!block){
			errorstream<<"ServerEnvironment::clearAllObjects(): "
					<<"Failed to emerge block "<<PP(p)<<std::endl;
			continue;
		}
		u32 num_stored = block->m_static_objects.m_stored.size();
		u32 num_active = block->m_static_objects.m_active.size();
		if(num_stored != 0 || num_active != 0){
			block->m_static_objects.m_stored.clear();
			block->m_static_objects.m_active.clear();
			block->raiseModified(MOD_STATE_WRITE_NEEDED,
					"clearAllObjects");
			num_objs_cleared += num_stored + num_active;
			num_blocks_cleared++;
		}
		num_blocks_checked++;

		if(num_blocks_checked % report_interval == 0){
			float percent = 100.0 * (float)num_blocks_checked /
					loadable_blocks.size();
			infostream<<"ServerEnvironment::clearAllObjects(): "
					<<"Cleared "<<num_objs_cleared<<" objects"
					<<" in "<<num_blocks_cleared<<" blocks ("
					<<percent<<"%)"<<std::endl;
		}
	}
	infostream<<"ServerEnvironment::clearAllObjects(): "
			<<"Finished: Cleared "<<num_objs_cleared<<" objects"
			<<" in "<<num_blocks_cleared<<" blocks"<<std::endl;
}

void ServerEnvironment::step(float dtime)
{
	DSTACK(__FUNCTION_NAME);
	
	//TimeTaker timer("ServerEnv step");

	/* Step time of day */
	stepTimeOfDay(dtime);

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
		Handle players
	*/
	{
		ScopeProfiler sp(g_profiler, "SEnv: handle players avg", SPT_AVG);
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
		}
	}

	/*
		Manage active block list
	*/
	if(m_active_blocks_management_interval.step(dtime, 2.0))
	{
		ScopeProfiler sp(g_profiler, "SEnv: manage act. block list avg /2s", SPT_AVG);
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
		const s16 active_block_range = g_settings->getS16("active_block_range");
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

			/*infostream<<"Server: Block ("<<p.X<<","<<p.Y<<","<<p.Z
					<<") became inactive"<<std::endl;*/
			
			MapBlock *block = m_map->getBlockNoCreateNoEx(p);
			if(block==NULL)
				continue;
			
			// Set current time as timestamp (and let it set ChangedFlag)
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
			
			/*infostream<<"Server: Block ("<<p.X<<","<<p.Y<<","<<p.Z
					<<") became active"<<std::endl;*/

			MapBlock *block = m_map->getBlockNoCreateNoEx(p);
			if(block==NULL){
				// Block needs to be fetched first
				m_emerger->queueBlockEmerge(p, false);
				m_active_blocks.m_list.remove(p);
				continue;
			}

			activateBlock(block);
		}
	}

	/*
		Mess around in active blocks
	*/
	if(m_active_blocks_nodemetadata_interval.step(dtime, 1.0))
	{
		ScopeProfiler sp(g_profiler, "SEnv: mess in act. blocks avg /1s", SPT_AVG);
		
		float dtime = 1.0;

		for(core::map<v3s16, bool>::Iterator
				i = m_active_blocks.m_list.getIterator();
				i.atEnd()==false; i++)
		{
			v3s16 p = i.getNode()->getKey();
			
			/*infostream<<"Server: Block ("<<p.X<<","<<p.Y<<","<<p.Z
					<<") being handled"<<std::endl;*/

			MapBlock *block = m_map->getBlockNoCreateNoEx(p);
			if(block==NULL)
				continue;

			// Reset block usage timer
			block->resetUsageTimer();
			
			// Set current time as timestamp
			block->setTimestampNoChangedFlag(m_game_time);
			// If time has changed much from the one on disk,
			// set block to be saved when it is unloaded
			if(block->getTimestamp() > block->getDiskTimestamp() + 60)
				block->raiseModified(MOD_STATE_WRITE_AT_UNLOAD,
						"Timestamp older than 60s (step)");

			// Run node timers
			std::map<v3s16, NodeTimer> elapsed_timers =
				block->m_node_timers.step((float)dtime);
			if(!elapsed_timers.empty()){
				MapNode n;
				for(std::map<v3s16, NodeTimer>::iterator
						i = elapsed_timers.begin();
						i != elapsed_timers.end(); i++){
					n = block->getNodeNoEx(i->first);
					p = i->first + block->getPosRelative();
					if(scriptapi_node_on_timer(m_lua,p,n,i->second.elapsed))
						block->setNodeTimer(i->first,NodeTimer(i->second.timeout,0));
				}
			}
		}
	}
	
	const float abm_interval = 1.0;
	if(m_active_block_modifier_interval.step(dtime, abm_interval))
	do{ // breakable
		if(m_active_block_interval_overload_skip > 0){
			ScopeProfiler sp(g_profiler, "SEnv: ABM overload skips");
			m_active_block_interval_overload_skip--;
			break;
		}
		ScopeProfiler sp(g_profiler, "SEnv: modify in blocks avg /1s", SPT_AVG);
		TimeTaker timer("modify in active blocks");
		
		// Initialize handling of ActiveBlockModifiers
		ABMHandler abmhandler(m_abms, abm_interval, this, true);

		for(core::map<v3s16, bool>::Iterator
				i = m_active_blocks.m_list.getIterator();
				i.atEnd()==false; i++)
		{
			v3s16 p = i.getNode()->getKey();
			
			/*infostream<<"Server: Block ("<<p.X<<","<<p.Y<<","<<p.Z
					<<") being handled"<<std::endl;*/

			MapBlock *block = m_map->getBlockNoCreateNoEx(p);
			if(block==NULL)
				continue;
			
			// Set current time as timestamp
			block->setTimestampNoChangedFlag(m_game_time);

			/* Handle ActiveBlockModifiers */
			abmhandler.apply(block);
		}

		u32 time_ms = timer.stop(true);
		u32 max_time_ms = 200;
		if(time_ms > max_time_ms){
			infostream<<"WARNING: active block modifiers took "
					<<time_ms<<"ms (longer than "
					<<max_time_ms<<"ms)"<<std::endl;
			m_active_block_interval_overload_skip = (time_ms / max_time_ms) + 1;
		}
	}while(0);
	
	/*
		Step script environment (run global on_step())
	*/
	scriptapi_environment_step(m_lua, dtime);

	/*
		Step active objects
	*/
	{
		ScopeProfiler sp(g_profiler, "SEnv: step act. objs avg", SPT_AVG);
		//TimeTaker timer("Step active objects");

		g_profiler->avg("SEnv: num of objects", m_active_objects.size());
		
		// This helps the objects to send data at the same time
		bool send_recommended = false;
		m_send_recommended_timer += dtime;
		if(m_send_recommended_timer > getSendRecommendedInterval())
		{
			m_send_recommended_timer -= getSendRecommendedInterval();
			send_recommended = true;
		}

		for(core::map<u16, ServerActiveObject*>::Iterator
				i = m_active_objects.getIterator();
				i.atEnd()==false; i++)
		{
			ServerActiveObject* obj = i.getNode()->getValue();
			// Remove non-peaceful mobs on peaceful mode
			if(g_settings->getBool("only_peaceful_mobs")){
				if(!obj->isPeaceful())
					obj->m_removed = true;
			}
			// Don't step if is to be removed or stored statically
			if(obj->m_removed || obj->m_pending_deactivation)
				continue;
			// Step object
			obj->step(dtime, send_recommended);
			// Read messages from object
			while(obj->m_messages_out.size() > 0)
			{
				m_active_object_messages.push_back(
						obj->m_messages_out.pop_front());
			}
		}
	}
	
	/*
		Manage active objects
	*/
	if(m_object_management_interval.step(dtime, 0.5))
	{
		ScopeProfiler sp(g_profiler, "SEnv: remove removed objs avg /.5s", SPT_AVG);
		/*
			Remove objects that satisfy (m_removed && m_known_by_count==0)
		*/
		removeRemovedObjects();
	}
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
	u16 id = addActiveObjectRaw(object, true);
	return id;
}

bool ServerEnvironment::addActiveObjectAsStatic(ServerActiveObject *obj)
{
	assert(obj);

	v3f objectpos = obj->getBasePosition();	

	// The block in which the object resides in
	v3s16 blockpos_o = getNodeBlockPos(floatToInt(objectpos, BS));

	/*
		Update the static data
	*/

	// Create new static object
	std::string staticdata = obj->getStaticData();
	StaticObject s_obj(obj->getType(), objectpos, staticdata);
	// Add to the block where the object is located in
	v3s16 blockpos = getNodeBlockPos(floatToInt(objectpos, BS));
	// Get or generate the block
	MapBlock *block = m_map->emergeBlock(blockpos);

	bool succeeded = false;

	if(block)
	{
		block->m_static_objects.insert(0, s_obj);
		block->raiseModified(MOD_STATE_WRITE_AT_UNLOAD,
				"addActiveObjectAsStatic");
		succeeded = true;
	}
	else{
		infostream<<"ServerEnvironment::addActiveObjectAsStatic: "
				<<"Could not find or generate "
				<<"a block for storing static object"<<std::endl;
		succeeded = false;
	}

	if(obj->environmentDeletes())
		delete obj;

	return succeeded;
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
		if(object->unlimitedTransferDistance() == false){
			// Discard if too far
			f32 distance_f = object->getBasePosition().getDistanceFrom(pos_f);
			if(distance_f > radius_f)
				continue;
		}
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

		if(object == NULL){
			infostream<<"ServerEnvironment::getRemovedActiveObjects():"
					<<" object in current_objects is NULL"<<std::endl;
			removed_objects.insert(id, false);
			continue;
		}

		if(object->m_removed)
		{
			removed_objects.insert(id, false);
			continue;
		}
		
		// If transfer distance is unlimited, don't remove
		if(object->unlimitedTransferDistance())
			continue;

		f32 distance_f = object->getBasePosition().getDistanceFrom(pos_f);

		if(distance_f >= radius_f)
		{
			removed_objects.insert(id, false);
			continue;
		}
		
		// Not removed
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

u16 ServerEnvironment::addActiveObjectRaw(ServerActiveObject *object,
		bool set_changed)
{
	assert(object);
	if(object->getId() == 0){
		u16 new_id = getFreeServerActiveObjectId(m_active_objects);
		if(new_id == 0)
		{
			errorstream<<"ServerEnvironment::addActiveObjectRaw(): "
					<<"no free ids available"<<std::endl;
			if(object->environmentDeletes())
				delete object;
			return 0;
		}
		object->setId(new_id);
	}
	else{
		verbosestream<<"ServerEnvironment::addActiveObjectRaw(): "
				<<"supplied with id "<<object->getId()<<std::endl;
	}
	if(isFreeServerActiveObjectId(object->getId(), m_active_objects) == false)
	{
		errorstream<<"ServerEnvironment::addActiveObjectRaw(): "
				<<"id is not free ("<<object->getId()<<")"<<std::endl;
		if(object->environmentDeletes())
			delete object;
		return 0;
	}
	/*infostream<<"ServerEnvironment::addActiveObjectRaw(): "
			<<"added (id="<<object->getId()<<")"<<std::endl;*/
			
	m_active_objects.insert(object->getId(), object);
  
	verbosestream<<"ServerEnvironment::addActiveObjectRaw(): "
			<<"Added id="<<object->getId()<<"; there are now "
			<<m_active_objects.size()<<" active objects."
			<<std::endl;
	
	// Register reference in scripting api (must be done before post-init)
	scriptapi_add_object_reference(m_lua, object);
	// Post-initialize object
	object->addedToEnvironment();
	
	// Add static data to block
	if(object->isStaticAllowed())
	{
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

			if(set_changed)
				block->raiseModified(MOD_STATE_WRITE_NEEDED, 
						"addActiveObjectRaw");
		}
		else{
			errorstream<<"ServerEnvironment::addActiveObjectRaw(): "
					<<"could not find block for storing id="<<object->getId()
					<<" statically"<<std::endl;
		}
	}
	
	return object->getId();
}

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
			infostream<<"NULL object found in ServerEnvironment"
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
			MapBlock *block = m_map->emergeBlock(obj->m_static_block);
			if(block)
			{
				block->m_static_objects.remove(id);
				block->raiseModified(MOD_STATE_WRITE_NEEDED,
						"removeRemovedObjects");
				obj->m_static_exists = false;
			}
		}

		// If m_known_by_count > 0, don't actually remove.
		if(obj->m_known_by_count > 0)
			continue;
		
		// Tell the object about removal
		obj->removingFromEnvironment();
		// Deregister in scripting api
		scriptapi_rm_object_reference(m_lua, obj);

		// Delete
		if(obj->environmentDeletes())
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

static void print_hexdump(std::ostream &o, const std::string &data)
{
	const int linelength = 16;
	for(int l=0; ; l++){
		int i0 = linelength * l;
		bool at_end = false;
		int thislinelength = linelength;
		if(i0 + thislinelength > (int)data.size()){
			thislinelength = data.size() - i0;
			at_end = true;
		}
		for(int di=0; di<linelength; di++){
			int i = i0 + di;
			char buf[4];
			if(di<thislinelength)
				snprintf(buf, 4, "%.2x ", data[i]);
			else
				snprintf(buf, 4, "   ");
			o<<buf;
		}
		o<<" ";
		for(int di=0; di<thislinelength; di++){
			int i = i0 + di;
			if(data[i] >= 32)
				o<<data[i];
			else
				o<<".";
		}
		o<<std::endl;
		if(at_end)
			break;
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
	verbosestream<<"ServerEnvironment::activateObjects(): "
			<<"activating objects of block "<<PP(block->getPos())
			<<" ("<<block->m_static_objects.m_stored.size()
			<<" objects)"<<std::endl;
	bool large_amount = (block->m_static_objects.m_stored.size() > 49);
	if(large_amount){
		errorstream<<"suspiciously large amount of objects detected: "
				<<block->m_static_objects.m_stored.size()<<" in "
				<<PP(block->getPos())
				<<"; removing all of them."<<std::endl;
		// Clear stored list
		block->m_static_objects.m_stored.clear();
		block->raiseModified(MOD_STATE_WRITE_NEEDED,
				"stored list cleared in activateObjects due to "
				"large amount of objects");
		return;
	}
	// A list for objects that couldn't be converted to static for some
	// reason. They will be stored back.
	core::list<StaticObject> new_stored;
	// Loop through stored static objects
	for(core::list<StaticObject>::Iterator
			i = block->m_static_objects.m_stored.begin();
			i != block->m_static_objects.m_stored.end(); i++)
	{
		/*infostream<<"Server: Creating an active object from "
				<<"static data"<<std::endl;*/
		StaticObject &s_obj = *i;
		// Create an active object from the data
		ServerActiveObject *obj = ServerActiveObject::create
				(s_obj.type, this, 0, s_obj.pos, s_obj.data);
		// If couldn't create object, store static data back.
		if(obj==NULL)
		{
			errorstream<<"ServerEnvironment::activateObjects(): "
					<<"failed to create active object from static object "
					<<"in block "<<PP(s_obj.pos/BS)
					<<" type="<<(int)s_obj.type<<" data:"<<std::endl;
			print_hexdump(verbosestream, s_obj.data);
			
			new_stored.push_back(s_obj);
			continue;
		}
		verbosestream<<"ServerEnvironment::activateObjects(): "
				<<"activated static object pos="<<PP(s_obj.pos/BS)
				<<" type="<<(int)s_obj.type<<std::endl;
		// This will also add the object to the active static list
		addActiveObjectRaw(obj, false);
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
	/*
		Note: Block hasn't really been modified here.
		The objects have just been activated and moved from the stored
		static list to the active static list.
		As such, the block is essentially the same.
		Thus, do not call block->raiseModified(MOD_STATE_WRITE_NEEDED).
		Otherwise there would be a huge amount of unnecessary I/O.
	*/
}

/*
	Convert objects that are not standing inside active blocks to static.

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
		assert(obj);
		
		// Do not deactivate if static data creation not allowed
		if(!force_delete && !obj->isStaticAllowed())
			continue;

		// If pending deactivation, let removeRemovedObjects() do it
		if(!force_delete && obj->m_pending_deactivation)
			continue;

		u16 id = i.getNode()->getKey();		
		v3f objectpos = obj->getBasePosition();	

		// The block in which the object resides in
		v3s16 blockpos_o = getNodeBlockPos(floatToInt(objectpos, BS));

		// If block is active, don't remove
		if(!force_delete && m_active_blocks.contains(blockpos_o))
			continue;

		verbosestream<<"ServerEnvironment::deactivateFarObjects(): "
				<<"deactivating object id="<<id<<" on inactive block "
				<<PP(blockpos_o)<<std::endl;

		// If known by some client, don't immediately delete.
		bool pending_delete = (obj->m_known_by_count > 0 && !force_delete);

		/*
			Update the static data
		*/

		if(obj->isStaticAllowed())
		{
			// Create new static object
			std::string staticdata_new = obj->getStaticData();
			StaticObject s_obj(obj->getType(), objectpos, staticdata_new);
			
			bool stays_in_same_block = false;
			bool data_changed = true;

			if(obj->m_static_exists){
				if(obj->m_static_block == blockpos_o)
					stays_in_same_block = true;

				MapBlock *block = m_map->emergeBlock(obj->m_static_block, false);
				
				core::map<u16, StaticObject>::Node *n =
						block->m_static_objects.m_active.find(id);
				if(n){
					StaticObject static_old = n->getValue();

					float save_movem = obj->getMinimumSavedMovement();

					if(static_old.data == staticdata_new &&
							(static_old.pos - objectpos).getLength() < save_movem)
						data_changed = false;
				} else {
					errorstream<<"ServerEnvironment::deactivateFarObjects(): "
							<<"id="<<id<<" m_static_exists=true but "
							<<"static data doesn't actually exist in "
							<<PP(obj->m_static_block)<<std::endl;
				}
			}

			bool shall_be_written = (!stays_in_same_block || data_changed);
			
			// Delete old static object
			if(obj->m_static_exists)
			{
				MapBlock *block = m_map->emergeBlock(obj->m_static_block, false);
				if(block)
				{
					block->m_static_objects.remove(id);
					obj->m_static_exists = false;
					// Only mark block as modified if data changed considerably
					if(shall_be_written)
						block->raiseModified(MOD_STATE_WRITE_NEEDED,
								"deactivateFarObjects: Static data "
								"changed considerably");
				}
			}

			// Add to the block where the object is located in
			v3s16 blockpos = getNodeBlockPos(floatToInt(objectpos, BS));
			// Get or generate the block
			MapBlock *block = m_map->emergeBlock(blockpos);

			if(block)
			{
				if(block->m_static_objects.m_stored.size() >= 49){
					errorstream<<"ServerEnv: Trying to store id="<<obj->getId()
							<<" statically but block "<<PP(blockpos)
							<<" already contains "
							<<block->m_static_objects.m_stored.size()
							<<" (over 49) objects."
							<<" Forcing delete."<<std::endl;
					force_delete = true;
				} else {
					u16 new_id = pending_delete ? id : 0;
					// If static counterpart already exists, remove it first.
					// This shouldn't happen, but happens rarely for some
					// unknown reason. Unsuccessful attempts have been made to
					// find said reason.
					if(new_id && block->m_static_objects.m_active.find(new_id)){
						infostream<<"ServerEnv: WARNING: Performing hack #83274"
								<<std::endl;
						block->m_static_objects.remove(new_id);
					}
					block->m_static_objects.insert(new_id, s_obj);
					
					// Only mark block as modified if data changed considerably
					if(shall_be_written)
						block->raiseModified(MOD_STATE_WRITE_NEEDED,
								"deactivateFarObjects: Static data "
								"changed considerably");
					
					obj->m_static_exists = true;
					obj->m_static_block = block->getPos();
				}
			}
			else{
				if(!force_delete){
					errorstream<<"ServerEnv: Could not find or generate "
							<<"a block for storing id="<<obj->getId()
							<<" statically"<<std::endl;
					continue;
				}
			}
		}

		/*
			If known by some client, set pending deactivation.
			Otherwise delete it immediately.
		*/

		if(pending_delete && !force_delete)
		{
			verbosestream<<"ServerEnvironment::deactivateFarObjects(): "
					<<"object id="<<id<<" is known by clients"
					<<"; not deleting yet"<<std::endl;

			obj->m_pending_deactivation = true;
			continue;
		}
		
		verbosestream<<"ServerEnvironment::deactivateFarObjects(): "
				<<"object id="<<id<<" is not known by clients"
				<<"; deleting"<<std::endl;

		// Tell the object about removal
		obj->removingFromEnvironment();
		// Deregister in scripting api
		scriptapi_rm_object_reference(m_lua, obj);

		// Delete active object
		if(obj->environmentDeletes())
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

#include "clientsimpleobject.h"

/*
	ClientEnvironment
*/

ClientEnvironment::ClientEnvironment(ClientMap *map, scene::ISceneManager *smgr,
		ITextureSource *texturesource, IGameDef *gamedef,
		IrrlichtDevice *irr):
	m_map(map),
	m_smgr(smgr),
	m_texturesource(texturesource),
	m_gamedef(gamedef),
	m_irr(irr)
{
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

	for(core::list<ClientSimpleObject*>::Iterator
			i = m_simple_objects.begin(); i != m_simple_objects.end(); i++)
	{
		delete *i;
	}

	// Drop/delete map
	m_map->drop();
}

Map & ClientEnvironment::getMap()
{
	return *m_map;
}

ClientMap & ClientEnvironment::getClientMap()
{
	return *m_map;
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

	/* Step time of day */
	stepTimeOfDay(dtime);

	// Get some settings
	bool fly_allowed = m_gamedef->checkLocalPrivilege("fly");
	bool free_move = fly_allowed && g_settings->getBool("free_move");

	// Get local player
	LocalPlayer *lplayer = getLocalPlayer();
	assert(lplayer);
	// collision info queue
	core::list<CollisionInfo> player_collisions;
	
	/*
		Get the speed the player is going
	*/
	bool is_climbing = lplayer->is_climbing;
	
	f32 player_speed = lplayer->getSpeed().getLength();
	
	/*
		Maximum position increment
	*/
	//f32 position_max_increment = 0.05*BS;
	f32 position_max_increment = 0.1*BS;

	// Maximum time increment (for collision detection etc)
	// time = distance / speed
	f32 dtime_max_increment = 1;
	if(player_speed > 0.001)
		dtime_max_increment = position_max_increment / player_speed;
	
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
			if(free_move == false && is_climbing == false)
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
		v3f speed_diff = info.new_speed - info.old_speed;;
		// Handle only fall damage
		// (because otherwise walking against something in fast_move kills you)
		if(speed_diff.Y < 0 || info.old_speed.Y >= 0)
			continue;
		// Get rid of other components
		speed_diff.X = 0;
		speed_diff.Z = 0;
		f32 pre_factor = 1; // 1 hp per node/s
		f32 tolerance = BS*14; // 5 without damage
		f32 post_factor = 1; // 1 hp per node/s
		if(info.type == COLLISION_NODE)
		{
			const ContentFeatures &f = m_gamedef->ndef()->
					get(m_map->getNodeNoEx(info.node_p));
			// Determine fall damage multiplier
			int addp = itemgroup_get(f.groups, "fall_damage_add_percent");
			pre_factor = 1.0 + (float)addp/100.0;
		}
		float speed = pre_factor * speed_diff.getLength();
		if(speed > tolerance)
		{
			f32 damage_f = (speed - tolerance)/BS * post_factor;
			u16 damage = (u16)(damage_f+0.5);
			if(damage != 0)
				damageLocalPlayer(damage, true);
		}
	}
	
	/*
		A quick draft of lava damage
	*/
	if(m_lava_hurt_interval.step(dtime, 1.0))
	{
		v3f pf = lplayer->getPosition();
		
		// Feet, middle and head
		v3s16 p1 = floatToInt(pf + v3f(0, BS*0.1, 0), BS);
		MapNode n1 = m_map->getNodeNoEx(p1);
		v3s16 p2 = floatToInt(pf + v3f(0, BS*0.8, 0), BS);
		MapNode n2 = m_map->getNodeNoEx(p2);
		v3s16 p3 = floatToInt(pf + v3f(0, BS*1.6, 0), BS);
		MapNode n3 = m_map->getNodeNoEx(p2);

		u32 damage_per_second = 0;
		damage_per_second = MYMAX(damage_per_second,
				m_gamedef->ndef()->get(n1).damage_per_second);
		damage_per_second = MYMAX(damage_per_second,
				m_gamedef->ndef()->get(n2).damage_per_second);
		damage_per_second = MYMAX(damage_per_second,
				m_gamedef->ndef()->get(n3).damage_per_second);
		
		if(damage_per_second != 0)
		{
			damageLocalPlayer(damage_per_second, true);
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

		}
		
		// Update lighting on all players on client
		u8 light = LIGHT_MAX;
		try{
			// Get node at head
			v3s16 p = player->getLightPosition();
			MapNode n = m_map->getNode(p);
			light = n.getLightBlend(getDayNightRatio(), m_gamedef->ndef());
		}
		catch(InvalidPositionException &e){
			light = blend_light(getDayNightRatio(), LIGHT_SUN, 0);
		}
		player->light = light;
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

		if(m_active_object_light_update_interval.step(dtime, 0.21))
		{
			// Update lighting
			u8 light = 0;
			try{
				// Get node at head
				v3s16 p = obj->getLightPosition();
				MapNode n = m_map->getNode(p);
				light = n.getLightBlend(getDayNightRatio(), m_gamedef->ndef());
			}
			catch(InvalidPositionException &e){
				light = blend_light(getDayNightRatio(), LIGHT_SUN, 0);
			}
			obj->updateLight(light);
		}
	}

	/*
		Step and handle simple objects
	*/
	for(core::list<ClientSimpleObject*>::Iterator
			i = m_simple_objects.begin(); i != m_simple_objects.end();)
	{
		ClientSimpleObject *simple = *i;
		core::list<ClientSimpleObject*>::Iterator cur = i;
		i++;
		simple->step(dtime);
		if(simple->m_to_be_removed){
			delete simple;
			m_simple_objects.erase(cur);
		}
	}
}
	
void ClientEnvironment::addSimpleObject(ClientSimpleObject *simple)
{
	m_simple_objects.push_back(simple);
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
			infostream<<"ClientEnvironment::addActiveObject(): "
					<<"no free ids available"<<std::endl;
			delete object;
			return 0;
		}
		object->setId(new_id);
	}
	if(isFreeClientActiveObjectId(object->getId(), m_active_objects) == false)
	{
		infostream<<"ClientEnvironment::addActiveObject(): "
				<<"id is not free ("<<object->getId()<<")"<<std::endl;
		delete object;
		return 0;
	}
	infostream<<"ClientEnvironment::addActiveObject(): "
			<<"added (id="<<object->getId()<<")"<<std::endl;
	m_active_objects.insert(object->getId(), object);
	object->addToScene(m_smgr, m_texturesource, m_irr);
	{ // Update lighting immediately
		u8 light = 0;
		try{
			// Get node at head
			v3s16 p = object->getLightPosition();
			MapNode n = m_map->getNode(p);
			light = n.getLightBlend(getDayNightRatio(), m_gamedef->ndef());
		}
		catch(InvalidPositionException &e){
			light = blend_light(getDayNightRatio(), LIGHT_SUN, 0);
		}
		object->updateLight(light);
	}
	return object->getId();
}

void ClientEnvironment::addActiveObject(u16 id, u8 type,
		const std::string &init_data)
{
	ClientActiveObject* obj =
			ClientActiveObject::create(type, m_gamedef, this);
	if(obj == NULL)
	{
		infostream<<"ClientEnvironment::addActiveObject(): "
				<<"id="<<id<<" type="<<type<<": Couldn't create object"
				<<std::endl;
		return;
	}
	
	obj->setId(id);

	try
	{
		obj->initialize(init_data);
	}
	catch(SerializationError &e)
	{
		errorstream<<"ClientEnvironment::addActiveObject():"
				<<" id="<<id<<" type="<<type
				<<": SerializationError in initialize(),"
				<<" init_data="<<serializeJsonString(init_data)
				<<std::endl;
	}

	addActiveObject(obj);
}

void ClientEnvironment::removeActiveObject(u16 id)
{
	verbosestream<<"ClientEnvironment::removeActiveObject(): "
			<<"id="<<id<<std::endl;
	ClientActiveObject* obj = getActiveObject(id);
	if(obj == NULL)
	{
		infostream<<"ClientEnvironment::removeActiveObject(): "
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
		infostream<<"ClientEnvironment::processActiveObjectMessage():"
				<<" got message for id="<<id<<", which doesn't exist."
				<<std::endl;
		return;
	}
	try
	{
		obj->processMessage(data);
	}
	catch(SerializationError &e)
	{
		errorstream<<"ClientEnvironment::processActiveObjectMessage():"
				<<" id="<<id<<" type="<<obj->getType()
				<<" SerializationError in processMessage(),"
				<<" message="<<serializeJsonString(data)
				<<std::endl;
	}
}

/*
	Callbacks for activeobjects
*/

void ClientEnvironment::damageLocalPlayer(u8 damage, bool handle_hp)
{
	LocalPlayer *lplayer = getLocalPlayer();
	assert(lplayer);
	
	if(handle_hp){
		if(lplayer->hp > damage)
			lplayer->hp -= damage;
		else
			lplayer->hp = 0;
	}

	ClientEnvEvent event;
	event.type = CEE_PLAYER_DAMAGE;
	event.player_damage.amount = damage;
	event.player_damage.send_to_server = handle_hp;
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


