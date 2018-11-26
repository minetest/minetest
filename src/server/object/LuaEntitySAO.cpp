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

#include <cmath>
#include "util/serialize.h"
#include "collision.h"
#include "server.h"
#include "scripting_server.h"
#include "genericobject.h"
#include "server/object/LuaEntitySAO.h"
#include "server/object/PlayerHPChangeReason.h"

LuaEntitySAO::LuaEntitySAO(ServerEnvironment *env, v3f pos,
		const std::string &name, const std::string &state):
	UnitSAO(env, pos),
	m_init_name(name),
	m_init_state(state)
{
	// Only register type if no environment supplied
	if(env == NULL){
		ServerActiveObject::registerType(getType(), create);
		return;
	}
}

LuaEntitySAO::~LuaEntitySAO()
{
	if(m_registered){
		m_env->getScriptIface()->luaentity_Remove(m_id);
	}

	for (u32 attached_particle_spawner : m_attached_particle_spawners) {
		m_env->deleteParticleSpawner(attached_particle_spawner, false);
	}
}

void LuaEntitySAO::addedToEnvironment(u32 dtime_s)
{
	ServerActiveObject::addedToEnvironment(dtime_s);

	// Create entity from name
	m_registered = m_env->getScriptIface()->
		luaentity_Add(m_id, m_init_name.c_str());

	if(m_registered){
		// Get properties
		m_env->getScriptIface()->
			luaentity_GetProperties(m_id, &m_prop);
		// Initialize HP from properties
		m_hp = m_prop.hp_max;
		// Activate entity, supplying serialized state
		m_env->getScriptIface()->
			luaentity_Activate(m_id, m_init_state, dtime_s);
	} else {
		m_prop.infotext = m_init_name;
	}
}

ServerActiveObject* LuaEntitySAO::create(ServerEnvironment *env, v3f pos,
		const std::string &data)
{
	std::string name;
	std::string state;
	s16 hp = 1;
	v3f velocity;
	v3f rotation;
	if (!data.empty()) {
		std::istringstream is(data, std::ios::binary);
		// read version
		u8 version = readU8(is);
		// check if version is supported
		if(version == 0){
			name = deSerializeString(is);
			state = deSerializeLongString(is);
		}
		else if(version == 1){
			name = deSerializeString(is);
			state = deSerializeLongString(is);
			hp = readS16(is);
			velocity = readV3F1000(is);
			rotation = readV3F1000(is);
		}
	}
	// create object
	infostream<<"LuaEntitySAO::create(name=\""<<name<<"\" state=\""
			<<state<<"\")"<<std::endl;
	LuaEntitySAO *sao = new LuaEntitySAO(env, pos, name, state);
	sao->m_hp = hp;
	sao->m_velocity = velocity;
	sao->m_rotation = rotation;
	return sao;
}

void LuaEntitySAO::step(float dtime, bool send_recommended)
{
	if(!m_properties_sent)
	{
		m_properties_sent = true;
		std::string str = getPropertyPacket();
		// create message and add to list
		ActiveObjectMessage aom(getId(), true, str);
		m_messages_out.push(aom);
	}

	// If attached, check that our parent is still there. If it isn't, detach.
	if(m_attachment_parent_id && !isAttached())
	{
		m_attachment_parent_id = 0;
		m_attachment_bone = "";
		m_attachment_position = v3f(0,0,0);
		m_attachment_rotation = v3f(0,0,0);
		sendPosition(false, true);
	}

	m_last_sent_position_timer += dtime;

	// Each frame, parent position is copied if the object is attached, otherwise it's calculated normally
	// If the object gets detached this comes into effect automatically from the last known origin
	if(isAttached())
	{
		v3f pos = m_env->getActiveObject(m_attachment_parent_id)->getBasePosition();
		m_base_position = pos;
		m_velocity = v3f(0,0,0);
		m_acceleration = v3f(0,0,0);
	}
	else
	{
		if(m_prop.physical){
			aabb3f box = m_prop.collisionbox;
			box.MinEdge *= BS;
			box.MaxEdge *= BS;
			collisionMoveResult moveresult;
			f32 pos_max_d = BS*0.25; // Distance per iteration
			v3f p_pos = m_base_position;
			v3f p_velocity = m_velocity;
			v3f p_acceleration = m_acceleration;
			moveresult = collisionMoveSimple(m_env, m_env->getGameDef(),
					pos_max_d, box, m_prop.stepheight, dtime,
					&p_pos, &p_velocity, p_acceleration,
					this, m_prop.collideWithObjects);

			// Apply results
			m_base_position = p_pos;
			m_velocity = p_velocity;
			m_acceleration = p_acceleration;
		} else {
			m_base_position += dtime * m_velocity + 0.5 * dtime
					* dtime * m_acceleration;
			m_velocity += dtime * m_acceleration;
		}

		if (m_prop.automatic_face_movement_dir &&
				(fabs(m_velocity.Z) > 0.001 || fabs(m_velocity.X) > 0.001)) {

			float target_yaw = atan2(m_velocity.Z, m_velocity.X) * 180 / M_PI
				+ m_prop.automatic_face_movement_dir_offset;
			float max_rotation_delta =
					dtime * m_prop.automatic_face_movement_max_rotation_per_sec;

			m_rotation.Y = wrapDegrees_0_360(m_rotation.Y);
			wrappedApproachShortest(m_rotation.Y, target_yaw, max_rotation_delta, 360.f);
		}
	}

	if(m_registered){
		m_env->getScriptIface()->luaentity_Step(m_id, dtime);
	}

	if (!send_recommended)
		return;

	if(!isAttached())
	{
		// TODO: force send when acceleration changes enough?
		float minchange = 0.2*BS;
		if(m_last_sent_position_timer > 1.0){
			minchange = 0.01*BS;
		} else if(m_last_sent_position_timer > 0.2){
			minchange = 0.05*BS;
		}
		float move_d = m_base_position.getDistanceFrom(m_last_sent_position);
		move_d += m_last_sent_move_precision;
		float vel_d = m_velocity.getDistanceFrom(m_last_sent_velocity);
		if (move_d > minchange || vel_d > minchange ||
				std::fabs(m_rotation.X - m_last_sent_rotation.X) > 1.0f ||
				std::fabs(m_rotation.Y - m_last_sent_rotation.Y) > 1.0f ||
				std::fabs(m_rotation.Z - m_last_sent_rotation.Z) > 1.0f) {

			sendPosition(true, false);
		}
	}

	if (!m_armor_groups_sent) {
		m_armor_groups_sent = true;
		std::string str = gob_cmd_update_armor_groups(
				m_armor_groups);
		// create message and add to list
		ActiveObjectMessage aom(getId(), true, str);
		m_messages_out.push(aom);
	}

	if (!m_animation_sent) {
		m_animation_sent = true;
		std::string str = gob_cmd_update_animation(
			m_animation_range, m_animation_speed, m_animation_blend, m_animation_loop);
		// create message and add to list
		ActiveObjectMessage aom(getId(), true, str);
		m_messages_out.push(aom);
	}

	if (!m_animation_speed_sent) {
		m_animation_speed_sent = true;
		std::string str = gob_cmd_update_animation_speed(m_animation_speed);
		// create message and add to list
		ActiveObjectMessage aom(getId(), true, str);
		m_messages_out.push(aom);
	}

	if (!m_bone_position_sent) {
		m_bone_position_sent = true;
		for (std::unordered_map<std::string, core::vector2d<v3f>>::const_iterator
				ii = m_bone_position.begin(); ii != m_bone_position.end(); ++ii){
			std::string str = gob_cmd_update_bone_position((*ii).first,
					(*ii).second.X, (*ii).second.Y);
			// create message and add to list
			ActiveObjectMessage aom(getId(), true, str);
			m_messages_out.push(aom);
		}
	}

	if (!m_attachment_sent) {
		m_attachment_sent = true;
		std::string str = gob_cmd_update_attachment(m_attachment_parent_id, m_attachment_bone, m_attachment_position, m_attachment_rotation);
		// create message and add to list
		ActiveObjectMessage aom(getId(), true, str);
		m_messages_out.push(aom);
	}
}

std::string LuaEntitySAO::getClientInitializationData(u16 protocol_version)
{
	std::ostringstream os(std::ios::binary);

	// protocol >= 14
	writeU8(os, 1); // version
	os << serializeString(""); // name
	writeU8(os, 0); // is_player
	writeS16(os, getId()); //id
	writeV3F1000(os, m_base_position);
	writeV3F1000(os, m_rotation);
	writeS16(os, m_hp);

	std::ostringstream msg_os(std::ios::binary);
	msg_os << serializeLongString(getPropertyPacket()); // message 1
	msg_os << serializeLongString(gob_cmd_update_armor_groups(m_armor_groups)); // 2
	msg_os << serializeLongString(gob_cmd_update_animation(
		m_animation_range, m_animation_speed, m_animation_blend, m_animation_loop)); // 3
	for (std::unordered_map<std::string, core::vector2d<v3f>>::const_iterator
			ii = m_bone_position.begin(); ii != m_bone_position.end(); ++ii) {
		msg_os << serializeLongString(gob_cmd_update_bone_position((*ii).first,
				(*ii).second.X, (*ii).second.Y)); // m_bone_position.size
	}
	msg_os << serializeLongString(gob_cmd_update_attachment(m_attachment_parent_id,
		m_attachment_bone, m_attachment_position, m_attachment_rotation)); // 4
	int message_count = 4 + m_bone_position.size();
	for (std::unordered_set<int>::const_iterator ii = m_attachment_child_ids.begin();
			(ii != m_attachment_child_ids.end()); ++ii) {
		if (ServerActiveObject *obj = m_env->getActiveObject(*ii)) {
			message_count++;
			msg_os << serializeLongString(gob_cmd_update_infant(*ii, obj->getSendType(),
				obj->getClientInitializationData(protocol_version)));
		}
	}

	msg_os << serializeLongString(gob_cmd_set_texture_mod(m_current_texture_modifier));
	message_count++;

	writeU8(os, message_count);
	os.write(msg_os.str().c_str(), msg_os.str().size());

	// return result
	return os.str();
}

void LuaEntitySAO::getStaticData(std::string *result) const
{
	verbosestream<<FUNCTION_NAME<<std::endl;
	std::ostringstream os(std::ios::binary);
	// version
	writeU8(os, 1);
	// name
	os<<serializeString(m_init_name);
	// state
	if(m_registered){
		std::string state = m_env->getScriptIface()->
			luaentity_GetStaticdata(m_id);
		os<<serializeLongString(state);
	} else {
		os<<serializeLongString(m_init_state);
	}
	// hp
	writeS16(os, m_hp);
	// velocity
	writeV3F1000(os, m_velocity);
	// rotation
	writeV3F1000(os, m_rotation);

	*result = os.str();
}

int LuaEntitySAO::punch(v3f dir,
		const ToolCapabilities *toolcap,
		ServerActiveObject *puncher,
		float time_from_last_punch)
{
	if (!m_registered) {
		// Delete unknown LuaEntities when punched
		m_pending_removal = true;
		return 0;
	}

	ItemStack *punchitem = NULL;
	ItemStack punchitem_static;
	if (puncher) {
		punchitem_static = puncher->getWieldedItem();
		punchitem = &punchitem_static;
	}

	PunchDamageResult result = getPunchDamage(
			m_armor_groups,
			toolcap,
			punchitem,
			time_from_last_punch);

	bool damage_handled = m_env->getScriptIface()->luaentity_Punch(m_id, puncher,
			time_from_last_punch, toolcap, dir, result.did_punch ? result.damage : 0);

	if (!damage_handled) {
		if (result.did_punch) {
			setHP(getHP() - result.damage,
				PlayerHPChangeReason(PlayerHPChangeReason::SET_HP));

			if (result.damage > 0) {
				std::string punchername = puncher ? puncher->getDescription() : "nil";

				actionstream << getDescription() << " punched by "
						<< punchername << ", damage " << result.damage
						<< " hp, health now " << getHP() << " hp" << std::endl;
			}

			std::string str = gob_cmd_punched(result.damage, getHP());
			// create message and add to list
			ActiveObjectMessage aom(getId(), true, str);
			m_messages_out.push(aom);
		}
	}

	if (getHP() == 0 && !isGone()) {
		m_pending_removal = true;
		clearParentAttachment();
		clearChildAttachments();
		m_env->getScriptIface()->luaentity_on_death(m_id, puncher);
	}

	return result.wear;
}

void LuaEntitySAO::rightClick(ServerActiveObject *clicker)
{
	if (!m_registered)
		return;

	m_env->getScriptIface()->luaentity_Rightclick(m_id, clicker);
}

void LuaEntitySAO::setPos(const v3f &pos)
{
	if(isAttached())
		return;
	m_base_position = pos;
	sendPosition(false, true);
}

void LuaEntitySAO::moveTo(v3f pos, bool continuous)
{
	if(isAttached())
		return;
	m_base_position = pos;
	if(!continuous)
		sendPosition(true, true);
}

float LuaEntitySAO::getMinimumSavedMovement()
{
	return 0.1 * BS;
}

std::string LuaEntitySAO::getDescription()
{
	std::ostringstream os(std::ios::binary);
	os<<"LuaEntitySAO at (";
	os<<(m_base_position.X/BS)<<",";
	os<<(m_base_position.Y/BS)<<",";
	os<<(m_base_position.Z/BS);
	os<<")";
	return os.str();
}

void LuaEntitySAO::setHP(s16 hp, const PlayerHPChangeReason &reason)
{
	if (hp < 0)
		hp = 0;
	m_hp = hp;
}

s16 LuaEntitySAO::getHP() const
{
	return m_hp;
}

void LuaEntitySAO::setVelocity(v3f velocity)
{
	m_velocity = velocity;
}

v3f LuaEntitySAO::getVelocity()
{
	return m_velocity;
}

void LuaEntitySAO::setAcceleration(v3f acceleration)
{
	m_acceleration = acceleration;
}

v3f LuaEntitySAO::getAcceleration()
{
	return m_acceleration;
}

void LuaEntitySAO::setTextureMod(const std::string &mod)
{
	std::string str = gob_cmd_set_texture_mod(mod);
	m_current_texture_modifier = mod;
	// create message and add to list
	ActiveObjectMessage aom(getId(), true, str);
	m_messages_out.push(aom);
}

std::string LuaEntitySAO::getTextureMod() const
{
	return m_current_texture_modifier;
}

void LuaEntitySAO::setSprite(v2s16 p, int num_frames, float framelength,
		bool select_horiz_by_yawpitch)
{
	std::string str = gob_cmd_set_sprite(
		p,
		num_frames,
		framelength,
		select_horiz_by_yawpitch
	);
	// create message and add to list
	ActiveObjectMessage aom(getId(), true, str);
	m_messages_out.push(aom);
}

std::string LuaEntitySAO::getName()
{
	return m_init_name;
}

std::string LuaEntitySAO::getPropertyPacket()
{
	return gob_cmd_set_properties(m_prop);
}

void LuaEntitySAO::sendPosition(bool do_interpolate, bool is_movement_end)
{
	// If the object is attached client-side, don't waste bandwidth sending its position to clients
	if(isAttached())
		return;

	m_last_sent_move_precision = m_base_position.getDistanceFrom(
			m_last_sent_position);
	m_last_sent_position_timer = 0;
	m_last_sent_position = m_base_position;
	m_last_sent_velocity = m_velocity;
	//m_last_sent_acceleration = m_acceleration;
	m_last_sent_rotation = m_rotation;

	float update_interval = m_env->getSendRecommendedInterval();

	std::string str = gob_cmd_update_position(
		m_base_position,
		m_velocity,
		m_acceleration,
		m_rotation,
		do_interpolate,
		is_movement_end,
		update_interval
	);
	// create message and add to list
	ActiveObjectMessage aom(getId(), false, str);
	m_messages_out.push(aom);
}

bool LuaEntitySAO::getCollisionBox(aabb3f *toset) const
{
	if (m_prop.physical)
	{
		//update collision box
		toset->MinEdge = m_prop.collisionbox.MinEdge * BS;
		toset->MaxEdge = m_prop.collisionbox.MaxEdge * BS;

		toset->MinEdge += m_base_position;
		toset->MaxEdge += m_base_position;

		return true;
	}

	return false;
}

bool LuaEntitySAO::getSelectionBox(aabb3f *toset) const
{
	if (!m_prop.is_visible || !m_prop.pointable) {
		return false;
	}

	toset->MinEdge = m_prop.selectionbox.MinEdge * BS;
	toset->MaxEdge = m_prop.selectionbox.MaxEdge * BS;

	return true;
}

bool LuaEntitySAO::collideWithObjects() const
{
	return m_prop.collideWithObjects;
}

