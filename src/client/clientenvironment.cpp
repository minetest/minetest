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

#include "util/serialize.h"
#include "util/pointedthing.h"
#include "client.h"
#include "clientenvironment.h"
#include "clientsimpleobject.h"
#include "clientmap.h"
#include "scripting_client.h"
#include "mapblock_mesh.h"
#include "mtevent.h"
#include "collision.h"
#include "nodedef.h"
#include "profiler.h"
#include "raycast.h"
#include "voxelalgorithms.h"
#include "settings.h"
#include "shader.h"
#include "content_cao.h"
#include <algorithm>
#include "client/renderingengine.h"

/*
	CAOShaderConstantSetter
*/

//! Shader constant setter for passing material emissive color to the CAO object_shader
class CAOShaderConstantSetter : public IShaderConstantSetter
{
public:
	CAOShaderConstantSetter():
			m_emissive_color_setting("emissiveColor")
	{}

	~CAOShaderConstantSetter() override = default;

	void onSetConstants(video::IMaterialRendererServices *services,
			bool is_highlevel) override
	{
		if (!is_highlevel)
			return;

		// Ambient color
		video::SColorf emissive_color(m_emissive_color);

		float as_array[4] = {
			emissive_color.r,
			emissive_color.g,
			emissive_color.b,
			emissive_color.a,
		};
		m_emissive_color_setting.set(as_array, services);
	}

	void onSetMaterial(const video::SMaterial& material) override
	{
		m_emissive_color = material.EmissiveColor;
	}

private:
	video::SColor m_emissive_color;
	CachedPixelShaderSetting<float, 4> m_emissive_color_setting;
};

class CAOShaderConstantSetterFactory : public IShaderConstantSetterFactory
{
public:
	CAOShaderConstantSetterFactory()
	{}

	virtual IShaderConstantSetter* create()
	{
		return new CAOShaderConstantSetter();
	}
};

/*
	ClientEnvironment
*/

ClientEnvironment::ClientEnvironment(ClientMap *map,
	ITextureSource *texturesource, Client *client):
	Environment(client),
	m_map(map),
	m_texturesource(texturesource),
	m_client(client)
{
	auto *shdrsrc = m_client->getShaderSource();
	shdrsrc->addShaderConstantSetterFactory(new CAOShaderConstantSetterFactory());
}

ClientEnvironment::~ClientEnvironment()
{
	m_ao_manager.clear();

	for (auto &simple_object : m_simple_objects) {
		delete simple_object;
	}

	// Drop/delete map
	m_map->drop();

	delete m_local_player;
}

Map & ClientEnvironment::getMap()
{
	return *m_map;
}

ClientMap & ClientEnvironment::getClientMap()
{
	return *m_map;
}

void ClientEnvironment::setLocalPlayer(LocalPlayer *player)
{
	/*
		It is a failure if already is a local player
	*/
	FATAL_ERROR_IF(m_local_player != NULL,
		"Local player already allocated");

	m_local_player = player;
}

void ClientEnvironment::step(float dtime)
{
	/* Step time of day */
	stepTimeOfDay(dtime);

	// Get some settings
	bool fly_allowed = m_client->checkLocalPrivilege("fly");
	bool free_move = fly_allowed && g_settings->getBool("free_move");

	// Get local player
	LocalPlayer *lplayer = getLocalPlayer();
	assert(lplayer);
	// collision info queue
	std::vector<CollisionInfo> player_collisions;

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
			// Control local player
			lplayer->applyControl(dtime_part, this);

			// Apply physics
			if (!free_move && !is_climbing) {
				// Gravity
				v3f speed = lplayer->getSpeed();
				if (!lplayer->in_liquid)
					speed.Y -= lplayer->movement_gravity *
						lplayer->physics_override_gravity * dtime_part * 2.0f;

				// Liquid floating / sinking
				if (lplayer->in_liquid && !lplayer->swimming_vertical &&
						!lplayer->swimming_pitch)
					speed.Y -= lplayer->movement_liquid_sink * dtime_part * 2.0f;

				// Liquid resistance
				if (lplayer->in_liquid_stable || lplayer->in_liquid) {
					// How much the node's viscosity blocks movement, ranges
					// between 0 and 1. Should match the scale at which viscosity
					// increase affects other liquid attributes.
					static const f32 viscosity_factor = 0.3f;

					v3f d_wanted = -speed / lplayer->movement_liquid_fluidity;
					f32 dl = d_wanted.getLength();
					if (dl > lplayer->movement_liquid_fluidity_smooth)
						dl = lplayer->movement_liquid_fluidity_smooth;

					dl *= (lplayer->liquid_viscosity * viscosity_factor) +
						(1 - viscosity_factor);
					v3f d = d_wanted.normalize() * (dl * dtime_part * 100.0f);
					speed += d;
				}

				lplayer->setSpeed(speed);
			}

			/*
				Move the lplayer.
				This also does collision detection.
			*/
			lplayer->move(dtime_part, this, position_max_increment,
				&player_collisions);
		}
	} while (dtime_downcount > 0.001);

	bool player_immortal = lplayer->getCAO() && lplayer->getCAO()->isImmortal();

	for (const CollisionInfo &info : player_collisions) {
		v3f speed_diff = info.new_speed - info.old_speed;;
		// Handle only fall damage
		// (because otherwise walking against something in fast_move kills you)
		if (speed_diff.Y < 0 || info.old_speed.Y >= 0)
			continue;
		// Get rid of other components
		speed_diff.X = 0;
		speed_diff.Z = 0;
		f32 pre_factor = 1; // 1 hp per node/s
		f32 tolerance = BS*14; // 5 without damage
		f32 post_factor = 1; // 1 hp per node/s
		if (info.type == COLLISION_NODE) {
			const ContentFeatures &f = m_client->ndef()->
				get(m_map->getNode(info.node_p));
			// Determine fall damage multiplier
			int addp = itemgroup_get(f.groups, "fall_damage_add_percent");
			pre_factor = 1.0f + (float)addp / 100.0f;
		}
		float speed = pre_factor * speed_diff.getLength();
		if (speed > tolerance && !player_immortal) {
			f32 damage_f = (speed - tolerance) / BS * post_factor;
			u16 damage = (u16)MYMIN(damage_f + 0.5, U16_MAX);
			if (damage != 0) {
				damageLocalPlayer(damage, true);
				m_client->getEventManager()->put(
					new SimpleTriggerEvent(MtEvent::PLAYER_FALLING_DAMAGE));
			}
		}
	}

	if (m_client->modsLoaded())
		m_script->environment_step(dtime);

	// Update lighting on local player (used for wield item)
	u32 day_night_ratio = getDayNightRatio();
	{
		// Get node at head

		// On InvalidPositionException, use this as default
		// (day: LIGHT_SUN, night: 0)
		MapNode node_at_lplayer(CONTENT_AIR, 0x0f, 0);

		v3s16 p = lplayer->getLightPosition();
		node_at_lplayer = m_map->getNode(p);

		u16 light = getInteriorLight(node_at_lplayer, 0, m_client->ndef());
		final_color_blend(&lplayer->light_color, light, day_night_ratio);
	}

	/*
		Step active objects and update lighting of them
	*/

	bool update_lighting = m_active_object_light_update_interval.step(dtime, 0.21);
	auto cb_state = [this, dtime, update_lighting, day_night_ratio] (ClientActiveObject *cao) {
		// Step object
		cao->step(dtime, this);

		if (update_lighting)
			cao->updateLight(day_night_ratio);
	};

	m_ao_manager.step(dtime, cb_state);

	/*
		Step and handle simple objects
	*/
	g_profiler->avg("ClientEnv: CSO count [#]", m_simple_objects.size());
	for (auto i = m_simple_objects.begin(); i != m_simple_objects.end();) {
		ClientSimpleObject *simple = *i;

		simple->step(dtime);
		if(simple->m_to_be_removed) {
			delete simple;
			i = m_simple_objects.erase(i);
		}
		else {
			++i;
		}
	}
}

void ClientEnvironment::addSimpleObject(ClientSimpleObject *simple)
{
	m_simple_objects.push_back(simple);
}

GenericCAO* ClientEnvironment::getGenericCAO(u16 id)
{
	ClientActiveObject *obj = getActiveObject(id);
	if (obj && obj->getType() == ACTIVEOBJECT_TYPE_GENERIC)
		return (GenericCAO*) obj;

	return NULL;
}

bool isFreeClientActiveObjectId(const u16 id,
	ClientActiveObjectMap &objects)
{
	return id != 0 && objects.find(id) == objects.end();

}

u16 getFreeClientActiveObjectId(ClientActiveObjectMap &objects)
{
	// try to reuse id's as late as possible
	static u16 last_used_id = 0;
	u16 startid = last_used_id;
	for(;;) {
		last_used_id ++;
		if (isFreeClientActiveObjectId(last_used_id, objects))
			return last_used_id;

		if (last_used_id == startid)
			return 0;
	}
}

u16 ClientEnvironment::addActiveObject(ClientActiveObject *object)
{
	// Register object. If failed return zero id
	if (!m_ao_manager.registerObject(object))
		return 0;

	object->addToScene(m_texturesource);

	// Update lighting immediately
	object->updateLight(getDayNightRatio());
	return object->getId();
}

void ClientEnvironment::addActiveObject(u16 id, u8 type,
	const std::string &init_data)
{
	ClientActiveObject* obj =
		ClientActiveObject::create((ActiveObjectType) type, m_client, this);
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
			<<": SerializationError in initialize(): "
			<<e.what()
			<<": init_data="<<serializeJsonString(init_data)
			<<std::endl;
	}

	u16 new_id = addActiveObject(obj);
	// Object initialized:
	if ((obj = getActiveObject(new_id))) {
		// Final step is to update all children which are already known
		// Data provided by AO_CMD_SPAWN_INFANT
		const auto &children = obj->getAttachmentChildIds();
		for (auto c_id : children) {
			if (auto *o = getActiveObject(c_id))
				o->updateAttachments();
		}
	}
}


void ClientEnvironment::removeActiveObject(u16 id)
{
	// Get current attachment childs to detach them visually
	std::unordered_set<int> attachment_childs;
	if (auto *obj = getActiveObject(id))
		attachment_childs = obj->getAttachmentChildIds();

	m_ao_manager.removeObject(id);

	// Perform a proper detach in Irrlicht
	for (auto c_id : attachment_childs) {
		if (ClientActiveObject *child = getActiveObject(c_id))
			child->updateAttachments();
	}
}

void ClientEnvironment::processActiveObjectMessage(u16 id, const std::string &data)
{
	ClientActiveObject *obj = getActiveObject(id);
	if (obj == NULL) {
		infostream << "ClientEnvironment::processActiveObjectMessage():"
			<< " got message for id=" << id << ", which doesn't exist."
			<< std::endl;
		return;
	}

	try {
		obj->processMessage(data);
	} catch (SerializationError &e) {
		errorstream<<"ClientEnvironment::processActiveObjectMessage():"
			<< " id=" << id << " type=" << obj->getType()
			<< " SerializationError in processMessage(): " << e.what()
			<< std::endl;
	}
}

/*
	Callbacks for activeobjects
*/

void ClientEnvironment::damageLocalPlayer(u16 damage, bool handle_hp)
{
	LocalPlayer *lplayer = getLocalPlayer();
	assert(lplayer);

	if (handle_hp) {
		if (lplayer->hp > damage)
			lplayer->hp -= damage;
		else
			lplayer->hp = 0;
	}

	ClientEnvEvent event;
	event.type = CEE_PLAYER_DAMAGE;
	event.player_damage.amount = damage;
	event.player_damage.send_to_server = handle_hp;
	m_client_event_queue.push(event);
}

/*
	Client likes to call these
*/

ClientEnvEvent ClientEnvironment::getClientEnvEvent()
{
	FATAL_ERROR_IF(m_client_event_queue.empty(),
			"ClientEnvironment::getClientEnvEvent(): queue is empty");

	ClientEnvEvent event = m_client_event_queue.front();
	m_client_event_queue.pop();
	return event;
}

void ClientEnvironment::getSelectedActiveObjects(
	const core::line3d<f32> &shootline_on_map,
	std::vector<PointedThing> &objects)
{
	std::vector<DistanceSortedActiveObject> allObjects;
	getActiveObjects(shootline_on_map.start,
		shootline_on_map.getLength() + 10.0f, allObjects);
	const v3f line_vector = shootline_on_map.getVector();

	for (const auto &allObject : allObjects) {
		ClientActiveObject *obj = allObject.obj;
		aabb3f selection_box;
		if (!obj->getSelectionBox(&selection_box))
			continue;

		const v3f &pos = obj->getPosition();
		aabb3f offsetted_box(selection_box.MinEdge + pos,
			selection_box.MaxEdge + pos);

		v3f current_intersection;
		v3s16 current_normal;
		if (boxLineCollision(offsetted_box, shootline_on_map.start, line_vector,
				&current_intersection, &current_normal)) {
			objects.emplace_back((s16) obj->getId(), current_intersection, current_normal,
				(current_intersection - shootline_on_map.start).getLengthSQ());
		}
	}
}
