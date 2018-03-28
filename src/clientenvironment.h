/*
Minetest
Copyright (C) 2010-2017 celeron55, Perttu Ahola <celeron55@gmail.com>

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

#pragma once

#include "environment.h"
#include <ISceneManager.h>
#include "clientobject.h"
#include "util/numeric.h"

class ClientSimpleObject;
class ClientMap;
class ClientScripting;
class ClientActiveObject;
class GenericCAO;
class LocalPlayer;

/*
	The client-side environment.

	This is not thread-safe.
	Must be called from main (irrlicht) thread (uses the SceneManager)
	Client uses an environment mutex.
*/

enum ClientEnvEventType
{
	CEE_NONE,
	CEE_PLAYER_DAMAGE,
	CEE_PLAYER_BREATH
};

struct ClientEnvEvent
{
	ClientEnvEventType type;
	union {
		//struct{
		//} none;
		struct{
			u8 amount;
			bool send_to_server;
		} player_damage;
		struct{
			u16 amount;
		} player_breath;
	};
};

typedef std::unordered_map<u16, ClientActiveObject*> ClientActiveObjectMap;
class ClientEnvironment : public Environment
{
public:
	ClientEnvironment(ClientMap *map, ITextureSource *texturesource, Client *client);
	~ClientEnvironment();

	Map & getMap();
	ClientMap & getClientMap();

	Client *getGameDef() { return m_client; }
	void setScript(ClientScripting *script) { m_script = script; }

	void step(f32 dtime);

	virtual void setLocalPlayer(LocalPlayer *player);
	LocalPlayer *getLocalPlayer() const { return m_local_player; }

	/*
		ClientSimpleObjects
	*/

	void addSimpleObject(ClientSimpleObject *simple);

	/*
		ActiveObjects
	*/

	GenericCAO* getGenericCAO(u16 id);
	ClientActiveObject* getActiveObject(u16 id);

	/*
		Adds an active object to the environment.
		Environment handles deletion of object.
		Object may be deleted by environment immediately.
		If id of object is 0, assigns a free id to it.
		Returns the id of the object.
		Returns 0 if not added and thus deleted.
	*/
	u16 addActiveObject(ClientActiveObject *object);

	void addActiveObject(u16 id, u8 type, const std::string &init_data);
	void removeActiveObject(u16 id);

	void processActiveObjectMessage(u16 id, const std::string &data);

	/*
		Callbacks for activeobjects
	*/

	void damageLocalPlayer(u8 damage, bool handle_hp=true);
	void updateLocalPlayerBreath(u16 breath);

	/*
		Client likes to call these
	*/

	// Get all nearby objects
	void getActiveObjects(v3f origin, f32 max_d,
		std::vector<DistanceSortedActiveObject> &dest);

	bool hasClientEnvEvents() const { return !m_client_event_queue.empty(); }

	// Get event from queue. If queue is empty, it triggers an assertion failure.
	ClientEnvEvent getClientEnvEvent();

	virtual void getSelectedActiveObjects(
		const core::line3d<f32> &shootline_on_map,
		std::vector<PointedThing> &objects
	);

	u16 attachement_parent_ids[USHRT_MAX + 1];

	const std::list<std::string> &getPlayerNames() { return m_player_names; }
	void addPlayerName(const std::string &name) { m_player_names.push_back(name); }
	void removePlayerName(const std::string &name) { m_player_names.remove(name); }
	void updateCameraOffset(const v3s16 &camera_offset)
	{ m_camera_offset = camera_offset; }
	v3s16 getCameraOffset() const { return m_camera_offset; }
private:
	ClientMap *m_map;
	LocalPlayer *m_local_player = nullptr;
	ITextureSource *m_texturesource;
	Client *m_client;
	ClientScripting *m_script = nullptr;
	ClientActiveObjectMap m_active_objects;
	std::vector<ClientSimpleObject*> m_simple_objects;
	std::queue<ClientEnvEvent> m_client_event_queue;
	IntervalLimiter m_active_object_light_update_interval;
	IntervalLimiter m_lava_hurt_interval;
	IntervalLimiter m_drowning_interval;
	IntervalLimiter m_breathing_interval;
	std::list<std::string> m_player_names;
	v3s16 m_camera_offset;
};
