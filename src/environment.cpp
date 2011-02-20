/*
Minetest-c55
Copyright (C) 2010 celeron55, Perttu Ahola <celeron55@gmail.com>

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

Environment::Environment()
{
	m_daynight_ratio = 0.5;
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

void Environment::setDayNightRatio(u32 r)
{
	m_daynight_ratio = r;
}

u32 Environment::getDayNightRatio()
{
	return m_daynight_ratio;
}

/*
	ServerEnvironment
*/

ServerEnvironment::ServerEnvironment(ServerMap *map):
	m_map(map),
	m_random_spawn_timer(0)
{
	/*
		TEST CODE
	*/
	TestSAO *obj = new TestSAO(0, v3f(0, BS*5, 0));
	addActiveObject(obj);
}

ServerEnvironment::~ServerEnvironment()
{
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

void ServerEnvironment::step(float dtime)
{
	DSTACK(__FUNCTION_NAME);

	// Get some settings
	//bool free_move = g_settings.getBool("free_move");
	bool footprints = g_settings.getBool("footprints");

	{
		//TimeTaker timer("Server m_map->timerUpdate()", g_device);
		m_map->timerUpdate(dtime);
	}

	/*
		Handle players
	*/
	for(core::list<Player*>::Iterator i = m_players.begin();
			i != m_players.end(); i++)
	{
		Player *player = *i;
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
		Step active objects
	*/
	for(core::map<u16, ServerActiveObject*>::Iterator
			i = m_active_objects.getIterator();
			i.atEnd()==false; i++)
	{
		ServerActiveObject* obj = i.getNode()->getValue();
		// Step object, putting messages directly to the queue
		obj->step(dtime, m_active_object_messages);
	}

	/*
		Remove (m_removed && m_known_by_count==0) objects
	*/
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
			else
			{
				// If not m_removed, don't remove.
				if(obj->m_removed == false)
					continue;
				// Delete
				delete obj;
				// Id to be removed from m_active_objects
				objects_to_remove.push_back(id);
			}
		}
		// Remove references from m_active_objects
		for(core::list<u16>::Iterator i = objects_to_remove.begin();
				i != objects_to_remove.end(); i++)
		{
			m_active_objects.remove(*i);
		}
	}

	/*
		TEST CODE
	*/
	m_random_spawn_timer -= dtime;
	if(m_random_spawn_timer < 0)
	{
		m_random_spawn_timer += 0.1;
		TestSAO *obj = new TestSAO(0,
				v3f(myrand_range(-2*BS,2*BS), BS*5, myrand_range(-2*BS,2*BS)));
		addActiveObject(obj);
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
	dstream<<"INGO: ServerEnvironment::addActiveObject(): "
			<<"added (id="<<object->getId()<<")"<<std::endl;
	m_active_objects.insert(object->getId(), object);
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
		//TimeTaker timer("Client m_map->timerUpdate()", g_device);
		m_map->timerUpdate(dtime);
	}

	/*
		Get the speed the player is going
	*/
	f32 player_speed = 0.001; // just some small value
	LocalPlayer *lplayer = getLocalPlayer();
	if(lplayer)
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
			dtime_part = dtime_max_increment;
		else
			dtime_part = dtime;
		dtime_downcount -= dtime_part;
		
		/*
			Handle local player
		*/
		
		{
			Player *player = getLocalPlayer();

			v3f playerpos = player->getPosition();
			
			// Apply physics
			if(free_move == false)
			{
				// Gravity
				v3f speed = player->getSpeed();
				if(player->swimming_up == false)
					speed.Y -= 9.81 * BS * dtime_part * 2;

				// Water resistance
				if(player->in_water_stable || player->in_water)
				{
					f32 max_down = 2.0*BS;
					if(speed.Y < -max_down) speed.Y = -max_down;

					f32 max = 2.5*BS;
					if(speed.getLength() > max)
					{
						speed = speed / speed.getLength() * max;
					}
				}

				player->setSpeed(speed);
			}

			/*
				Move the player.
				This also does collision detection.
			*/
			player->move(dtime_part, *m_map, position_max_increment);
		}
	}
	while(dtime_downcount > 0.001);
		
	//std::cout<<"Looped "<<loopcount<<" times."<<std::endl;
	
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
				light = n.getLightBlend(m_daynight_ratio);
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
						b->updateMesh(m_daynight_ratio);
					}
				}
			}
			catch(InvalidPositionException &e)
			{
			}
		}
	}
	
	/*
		Step active objects
	*/
	for(core::map<u16, ClientActiveObject*>::Iterator
			i = m_active_objects.getIterator();
			i.atEnd()==false; i++)
	{
		ClientActiveObject* obj = i.getNode()->getValue();
		// Step object
		obj->step(dtime);
	}
}

void ClientEnvironment::updateMeshes(v3s16 blockpos)
{
	m_map->updateMeshes(blockpos, m_daynight_ratio);
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

void ClientEnvironment::addActiveObject(u16 id, u8 type)
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

#endif // #ifndef SERVER


