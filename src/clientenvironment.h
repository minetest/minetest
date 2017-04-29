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

#ifndef CLIENT_ENVIRONMENT_HEADER
#define CLIENT_ENVIRONMENT_HEADER

#include <IrrlichtDevice.h>
#include <ISceneManager.h>
#include "environment.h"
#include "clientobject.h"

class ClientSimpleObject;
class ClientMap;
class ClientScripting;
class ClientActiveObject;
class GenericCAO;
class LocalPlayer;
struct PointedThing;

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

class ClientEnvironment : public Environment
{
public:
	ClientEnvironment(ClientMap *map, scene::ISceneManager *smgr,
		ITextureSource *texturesource, Client *client,
		IrrlichtDevice *device);
	~ClientEnvironment();

	Map & getMap();
	ClientMap & getClientMap();

	Client *getGameDef() { return m_client; }
	void setScript(ClientScripting *script) { m_script = script; }

	void step(f32 dtime);

	virtual void setLocalPlayer(LocalPlayer *player);
	LocalPlayer *getLocalPlayer() { return m_local_player; }

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

	/*!
	 * Gets closest object pointed by the shootline.
	 * Returns NULL if not found.
	 *
	 * \param[in]  shootline_on_map    the shootline for
	 * the test in world coordinates
	 * \param[out] intersection_point  the first point where
	 * the shootline meets the object. Valid only if
	 * not NULL is returned.
	 * \param[out] intersection_normal the normal vector of
	 * the intersection, pointing outwards. Zero vector if
	 * the shootline starts in an active object.
	 * Valid only if not NULL is returned.
	 */
	ClientActiveObject * getSelectedActiveObject(
		const core::line3d<f32> &shootline_on_map,
		v3f *intersection_point,
		v3s16 *intersection_normal
	);

	/*!
	 * Performs a raycast on the world.
	 * Returns the first thing the shootline meets.
	 *
	 * @param[in]  shootline         the shootline, starting from
	 * the camera position. This also gives the maximal distance
	 * of the search.
	 * @param[in]  liquids_pointable if false, liquids are ignored
	 * @param[in]  look_for_object   if false, objects are ignored
	 */
	PointedThing getPointedThing(
		core::line3d<f32> shootline,
		bool liquids_pointable,
		bool look_for_object);

	u16 attachement_parent_ids[USHRT_MAX + 1];

	const std::list<std::string> &getPlayerNames() { return m_player_names; }
	void addPlayerName(const std::string &name) { m_player_names.push_back(name); }
	void removePlayerName(const std::string &name) { m_player_names.remove(name); }
	void updateCameraOffset(v3s16 camera_offset)
	{ m_camera_offset = camera_offset; }
	v3s16 getCameraOffset() const { return m_camera_offset; }
private:
	ClientMap *m_map;
	LocalPlayer *m_local_player;
	scene::ISceneManager *m_smgr;
	ITextureSource *m_texturesource;
	Client *m_client;
	ClientScripting *m_script;
	IrrlichtDevice *m_irr;
	UNORDERED_MAP<u16, ClientActiveObject*> m_active_objects;
	std::vector<ClientSimpleObject*> m_simple_objects;
	std::queue<ClientEnvEvent> m_client_event_queue;
	IntervalLimiter m_active_object_light_update_interval;
	IntervalLimiter m_lava_hurt_interval;
	IntervalLimiter m_drowning_interval;
	IntervalLimiter m_breathing_interval;
	std::list<std::string> m_player_names;
	v3s16 m_camera_offset;
};

#endif
