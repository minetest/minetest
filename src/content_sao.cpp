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

#include "content_sao.h"
#include "util/serialize.h"
#include "collision.h"
#include "environment.h"
#include "tool.h" // For ToolCapabilities
#include "gamedef.h"
#include "nodedef.h"
#include "remoteplayer.h"
#include "server.h"
#include "scripting_server.h"
#include "genericobject.h"
#include "settings.h"
#include <algorithm>
#include <cmath>

std::map<u16, ServerActiveObject::Factory> ServerActiveObject::m_types;

/*
	TestSAO
*/

class TestSAO : public ServerActiveObject
{
public:
	TestSAO(ServerEnvironment *env, v3f pos):
		ServerActiveObject(env, pos),
		m_timer1(0),
		m_age(0)
	{
		ServerActiveObject::registerType(getType(), create);
	}
	ActiveObjectType getType() const
	{ return ACTIVEOBJECT_TYPE_TEST; }

	static ServerActiveObject* create(ServerEnvironment *env, v3f pos,
			const std::string &data)
	{
		return new TestSAO(env, pos);
	}

	void step(float dtime, bool send_recommended)
	{
		m_age += dtime;
		if(m_age > 10)
		{
			m_pending_removal = true;
			return;
		}

		m_base_position.Y += dtime * BS * 2;
		if(m_base_position.Y > 8*BS)
			m_base_position.Y = 2*BS;

		if (!send_recommended)
			return;

		m_timer1 -= dtime;
		if(m_timer1 < 0.0)
		{
			m_timer1 += 0.125;

			std::string data;

			data += itos(0); // 0 = position
			data += " ";
			data += itos(m_base_position.X);
			data += " ";
			data += itos(m_base_position.Y);
			data += " ";
			data += itos(m_base_position.Z);

			ActiveObjectMessage aom(getId(), false, data);
			m_messages_out.push(aom);
		}
	}

	bool getCollisionBox(aabb3f *toset) const { return false; }

	virtual bool getSelectionBox(aabb3f *toset) const { return false; }

	bool collideWithObjects() const { return false; }

private:
	float m_timer1;
	float m_age;
};

// Prototype (registers item for deserialization)
TestSAO proto_TestSAO(NULL, v3f(0,0,0));

/*
	UnitSAO
 */

UnitSAO::UnitSAO(ServerEnvironment *env, v3f pos):
	ServerActiveObject(env, pos)
{
	// Initialize something to armor groups
	m_armor_groups["fleshy"] = 100;
}

ServerActiveObject *UnitSAO::getParent() const
{
	if (!m_attachment_parent_id)
		return nullptr;
	// Check if the parent still exists
	ServerActiveObject *obj = m_env->getActiveObject(m_attachment_parent_id);

	return obj;
}

void UnitSAO::setArmorGroups(const ItemGroupList &armor_groups)
{
	m_armor_groups = armor_groups;
	m_armor_groups_sent = false;
}

const ItemGroupList &UnitSAO::getArmorGroups() const
{
	return m_armor_groups;
}

void UnitSAO::setAnimation(v2f frame_range, float frame_speed, float frame_blend, bool frame_loop)
{
	// store these so they can be updated to clients
	m_animation_range = frame_range;
	m_animation_speed = frame_speed;
	m_animation_blend = frame_blend;
	m_animation_loop = frame_loop;
	m_animation_sent = false;
}

void UnitSAO::getAnimation(v2f *frame_range, float *frame_speed, float *frame_blend, bool *frame_loop)
{
	*frame_range = m_animation_range;
	*frame_speed = m_animation_speed;
	*frame_blend = m_animation_blend;
	*frame_loop = m_animation_loop;
}

void UnitSAO::setAnimationSpeed(float frame_speed)
{
	m_animation_speed = frame_speed;
	m_animation_speed_sent = false;
}

void UnitSAO::setBonePosition(const std::string &bone, v3f position, v3f rotation)
{
	// store these so they can be updated to clients
	m_bone_position[bone] = core::vector2d<v3f>(position, rotation);
	m_bone_position_sent = false;
}

void UnitSAO::getBonePosition(const std::string &bone, v3f *position, v3f *rotation)
{
	*position = m_bone_position[bone].X;
	*rotation = m_bone_position[bone].Y;
}

void UnitSAO::setAttachment(int parent_id, const std::string &bone, v3f position, v3f rotation)
{
	// Attachments need to be handled on both the server and client.
	// If we just attach on the server, we can only copy the position of the parent. Attachments
	// are still sent to clients at an interval so players might see them lagging, plus we can't
	// read and attach to skeletal bones.
	// If we just attach on the client, the server still sees the child at its original location.
	// This breaks some things so we also give the server the most accurate representation
	// even if players only see the client changes.

	int old_parent = m_attachment_parent_id;
	m_attachment_parent_id = parent_id;
	m_attachment_bone = bone;
	m_attachment_position = position;
	m_attachment_rotation = rotation;
	m_attachment_sent = false;

	if (parent_id != old_parent) {
		onDetach(old_parent);
		onAttach(parent_id);
	}
}

void UnitSAO::getAttachment(int *parent_id, std::string *bone, v3f *position,
	v3f *rotation) const
{
	*parent_id = m_attachment_parent_id;
	*bone = m_attachment_bone;
	*position = m_attachment_position;
	*rotation = m_attachment_rotation;
}

void UnitSAO::clearChildAttachments()
{
	for (int child_id : m_attachment_child_ids) {
		// Child can be NULL if it was deleted earlier
		if (ServerActiveObject *child = m_env->getActiveObject(child_id))
			child->setAttachment(0, "", v3f(0, 0, 0), v3f(0, 0, 0));
	}
	m_attachment_child_ids.clear();
}

void UnitSAO::clearParentAttachment()
{
	ServerActiveObject *parent = nullptr;
	if (m_attachment_parent_id) {
		parent = m_env->getActiveObject(m_attachment_parent_id);
		setAttachment(0, "", m_attachment_position, m_attachment_rotation);
	} else {
		setAttachment(0, "", v3f(0, 0, 0), v3f(0, 0, 0));
	}
	// Do it
	if (parent)
		parent->removeAttachmentChild(m_id);
}

void UnitSAO::addAttachmentChild(int child_id)
{
	m_attachment_child_ids.insert(child_id);
}

void UnitSAO::removeAttachmentChild(int child_id)
{
	m_attachment_child_ids.erase(child_id);
}

const std::unordered_set<int> &UnitSAO::getAttachmentChildIds() const
{
	return m_attachment_child_ids;
}

void UnitSAO::onAttach(int parent_id)
{
	if (!parent_id)
		return;

	ServerActiveObject *parent = m_env->getActiveObject(parent_id);

	if (!parent || parent->isGone())
		return; // Do not try to notify soon gone parent

	if (parent->getType() == ACTIVEOBJECT_TYPE_LUAENTITY) {
		// Call parent's on_attach field
		m_env->getScriptIface()->luaentity_on_attach_child(parent_id, this);
	}
}

void UnitSAO::onDetach(int parent_id)
{
	if (!parent_id)
		return;

	ServerActiveObject *parent = m_env->getActiveObject(parent_id);
	if (getType() == ACTIVEOBJECT_TYPE_LUAENTITY)
		m_env->getScriptIface()->luaentity_on_detach(m_id, parent);

	if (!parent || parent->isGone())
		return; // Do not try to notify soon gone parent

	if (parent->getType() == ACTIVEOBJECT_TYPE_LUAENTITY)
		m_env->getScriptIface()->luaentity_on_detach_child(parent_id, this);
}

ObjectProperties* UnitSAO::accessObjectProperties()
{
	return &m_prop;
}

void UnitSAO::notifyObjectPropertiesModified()
{
	m_properties_sent = false;
}

/*
	LuaEntitySAO
*/

// Prototype (registers item for deserialization)
LuaEntitySAO proto_LuaEntitySAO(NULL, v3f(0,0,0), "_prototype", "");

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
			luaentity_GetProperties(m_id, this, &m_prop);
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
	u16 hp = 1;
	v3f velocity;
	v3f rotation;

	while (!data.empty()) { // breakable, run for one iteration
		std::istringstream is(data, std::ios::binary);
		// 'version' does not allow to incrementally extend the parameter list thus
		// we need another variable to build on top of 'version=1'. Ugly hack but works™
		u8 version2 = 0;
		u8 version = readU8(is);

		name = deSerializeString(is);
		state = deSerializeLongString(is);

		if (version < 1)
			break;

		hp = readU16(is);
		velocity = readV3F1000(is);
		// yaw must be yaw to be backwards-compatible
		rotation.Y = readF1000(is);

		if (is.good()) // EOF for old formats
			version2 = readU8(is);

		if (version2 < 1) // PROTOCOL_VERSION < 37
			break;

		// version2 >= 1
		rotation.X = readF1000(is);
		rotation.Z = readF1000(is);

		// if (version2 < 2)
		//     break;
		// <read new values>
		break;
	}
	// create object
	infostream << "LuaEntitySAO::create(name=\"" << name << "\" state=\""
			 << state << "\")" << std::endl;
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
			float max_rotation_per_sec =
					m_prop.automatic_face_movement_max_rotation_per_sec;

			if (max_rotation_per_sec > 0) {
				m_rotation.Y = wrapDegrees_0_360(m_rotation.Y);
				wrappedApproachShortest(m_rotation.Y, target_yaw,
					dtime * max_rotation_per_sec, 360.f);
			} else {
				// Negative values of max_rotation_per_sec mean disabled.
				m_rotation.Y = target_yaw;
			}
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

	// PROTOCOL_VERSION >= 37
	writeU8(os, 1); // version
	os << serializeString(""); // name
	writeU8(os, 0); // is_player
	writeU16(os, getId()); //id
	writeV3F32(os, m_base_position);
	writeV3F32(os, m_rotation);
	writeU16(os, m_hp);

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
			// TODO after a protocol bump: only send the object initialization data
			// to older clients (superfluous since this message exists)
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
	// version must be 1 to keep backwards-compatibility. See version2
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
	writeU16(os, m_hp);
	writeV3F1000(os, m_velocity);
	// yaw
	writeF1000(os, m_rotation.Y);

	// version2. Increase this variable for new values
	writeU8(os, 1); // PROTOCOL_VERSION >= 37

	writeF1000(os, m_rotation.X);
	writeF1000(os, m_rotation.Z);

	// <write new values>

	*result = os.str();
}

u16 LuaEntitySAO::punch(v3f dir,
		const ToolCapabilities *toolcap,
		ServerActiveObject *puncher,
		float time_from_last_punch)
{
	if (!m_registered) {
		// Delete unknown LuaEntities when punched
		m_pending_removal = true;
		return 0;
	}

	FATAL_ERROR_IF(!puncher, "Punch action called without SAO");

	s32 old_hp = getHP();
	ItemStack selected_item, hand_item;
	ItemStack tool_item = puncher->getWieldedItem(&selected_item, &hand_item);

	PunchDamageResult result = getPunchDamage(
			m_armor_groups,
			toolcap,
			&tool_item,
			time_from_last_punch);

	bool damage_handled = m_env->getScriptIface()->luaentity_Punch(m_id, puncher,
			time_from_last_punch, toolcap, dir, result.did_punch ? result.damage : 0);

	if (!damage_handled) {
		if (result.did_punch) {
			setHP((s32)getHP() - result.damage,
				PlayerHPChangeReason(PlayerHPChangeReason::SET_HP));

			std::string str = gob_cmd_punched(getHP());
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

	actionstream << puncher->getDescription() << " (id=" << puncher->getId() <<
			", hp=" << puncher->getHP() << ") punched " <<
			getDescription() << " (id=" << m_id << ", hp=" << m_hp <<
			"), damage=" << (old_hp - (s32)getHP()) <<
			(damage_handled ? " (handled by Lua)" : "") << std::endl;

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

void LuaEntitySAO::setHP(s32 hp, const PlayerHPChangeReason &reason)
{
	m_hp = rangelim(hp, 0, U16_MAX);
}

u16 LuaEntitySAO::getHP() const
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

/*
	PlayerSAO
*/

// No prototype, PlayerSAO does not need to be deserialized

PlayerSAO::PlayerSAO(ServerEnvironment *env_, RemotePlayer *player_, session_t peer_id_,
		bool is_singleplayer):
	UnitSAO(env_, v3f(0,0,0)),
	m_player(player_),
	m_peer_id(peer_id_),
	m_is_singleplayer(is_singleplayer)
{
	assert(m_peer_id != 0);	// pre-condition

	m_prop.hp_max = PLAYER_MAX_HP_DEFAULT;
	m_prop.breath_max = PLAYER_MAX_BREATH_DEFAULT;
	m_prop.physical = false;
	m_prop.collisionbox = aabb3f(-0.3f, 0.0f, -0.3f, 0.3f, 1.77f, 0.3f);
	m_prop.selectionbox = aabb3f(-0.3f, 0.0f, -0.3f, 0.3f, 1.77f, 0.3f);
	m_prop.pointable = true;
	// Start of default appearance, this should be overwritten by Lua
	m_prop.visual = "upright_sprite";
	m_prop.visual_size = v3f(1, 2, 1);
	m_prop.textures.clear();
	m_prop.textures.emplace_back("player.png");
	m_prop.textures.emplace_back("player_back.png");
	m_prop.colors.clear();
	m_prop.colors.emplace_back(255, 255, 255, 255);
	m_prop.spritediv = v2s16(1,1);
	m_prop.eye_height = 1.625f;
	// End of default appearance
	m_prop.is_visible = true;
	m_prop.backface_culling = false;
	m_prop.makes_footstep_sound = true;
	m_prop.stepheight = PLAYER_DEFAULT_STEPHEIGHT * BS;
	m_hp = m_prop.hp_max;
	m_breath = m_prop.breath_max;
	// Disable zoom in survival mode using a value of 0
	m_prop.zoom_fov = g_settings->getBool("creative_mode") ? 15.0f : 0.0f;

	if (!g_settings->getBool("enable_damage"))
		m_armor_groups["immortal"] = 1;
}

void PlayerSAO::finalize(RemotePlayer *player, const std::set<std::string> &privs)
{
	assert(player);
	m_player = player;
	m_privs = privs;
}

v3f PlayerSAO::getEyeOffset() const
{
	return v3f(0, BS * m_prop.eye_height, 0);
}

std::string PlayerSAO::getDescription()
{
	return std::string("player ") + m_player->getName();
}

// Called after id has been set and has been inserted in environment
void PlayerSAO::addedToEnvironment(u32 dtime_s)
{
	ServerActiveObject::addedToEnvironment(dtime_s);
	ServerActiveObject::setBasePosition(m_base_position);
	m_player->setPlayerSAO(this);
	m_player->setPeerId(m_peer_id);
	m_last_good_position = m_base_position;
}

// Called before removing from environment
void PlayerSAO::removingFromEnvironment()
{
	ServerActiveObject::removingFromEnvironment();
	if (m_player->getPlayerSAO() == this) {
		unlinkPlayerSessionAndSave();
		for (u32 attached_particle_spawner : m_attached_particle_spawners) {
			m_env->deleteParticleSpawner(attached_particle_spawner, false);
		}
	}
}

std::string PlayerSAO::getClientInitializationData(u16 protocol_version)
{
	std::ostringstream os(std::ios::binary);

	// Protocol >= 15
	writeU8(os, 1); // version
	os << serializeString(m_player->getName()); // name
	writeU8(os, 1); // is_player
	writeS16(os, getId()); // id
	writeV3F32(os, m_base_position);
	writeV3F32(os, m_rotation);
	writeU16(os, getHP());

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
	msg_os << serializeLongString(gob_cmd_update_physics_override(m_physics_override_speed,
			m_physics_override_jump, m_physics_override_gravity, m_physics_override_sneak,
			m_physics_override_sneak_glitch, m_physics_override_new_move)); // 5
	// (GENERIC_CMD_UPDATE_NAMETAG_ATTRIBUTES) : Deprecated, for backwards compatibility only.
	msg_os << serializeLongString(gob_cmd_update_nametag_attributes(m_prop.nametag_color)); // 6
	int message_count = 6 + m_bone_position.size();
	for (std::unordered_set<int>::const_iterator ii = m_attachment_child_ids.begin();
			ii != m_attachment_child_ids.end(); ++ii) {
		if (ServerActiveObject *obj = m_env->getActiveObject(*ii)) {
			message_count++;
			msg_os << serializeLongString(gob_cmd_update_infant(*ii, obj->getSendType(),
				obj->getClientInitializationData(protocol_version)));
		}
	}

	writeU8(os, message_count);
	os.write(msg_os.str().c_str(), msg_os.str().size());

	// return result
	return os.str();
}

void PlayerSAO::getStaticData(std::string * result) const
{
	FATAL_ERROR("Obsolete function");
}

void PlayerSAO::step(float dtime, bool send_recommended)
{
	if (!isImmortal() && m_drowning_interval.step(dtime, 2.0f)) {
		// Get nose/mouth position, approximate with eye position
		v3s16 p = floatToInt(getEyePosition(), BS);
		MapNode n = m_env->getMap().getNode(p);
		const ContentFeatures &c = m_env->getGameDef()->ndef()->get(n);
		// If node generates drown
		if (c.drowning > 0 && m_hp > 0) {
			if (m_breath > 0)
				setBreath(m_breath - 1);

			// No more breath, damage player
			if (m_breath == 0) {
				PlayerHPChangeReason reason(PlayerHPChangeReason::DROWNING);
				setHP(m_hp - c.drowning, reason);
				m_env->getGameDef()->SendPlayerHPOrDie(this, reason);
			}
		}
	}

	if (m_breathing_interval.step(dtime, 0.5f) && !isImmortal()) {
		// Get nose/mouth position, approximate with eye position
		v3s16 p = floatToInt(getEyePosition(), BS);
		MapNode n = m_env->getMap().getNode(p);
		const ContentFeatures &c = m_env->getGameDef()->ndef()->get(n);
		// If player is alive & not drowning & not in ignore & not immortal, breathe
		if (m_breath < m_prop.breath_max && c.drowning == 0 &&
				n.getContent() != CONTENT_IGNORE && m_hp > 0)
			setBreath(m_breath + 1);
	}

	if (!isImmortal() && m_node_hurt_interval.step(dtime, 1.0f)) {
		u32 damage_per_second = 0;
		std::string nodename;
		// Lowest and highest damage points are 0.1 within collisionbox
		float dam_top = m_prop.collisionbox.MaxEdge.Y - 0.1f;

		// Sequence of damage points, starting 0.1 above feet and progressing
		// upwards in 1 node intervals, stopping below top damage point.
		for (float dam_height = 0.1f; dam_height < dam_top; dam_height++) {
			v3s16 p = floatToInt(m_base_position +
				v3f(0.0f, dam_height * BS, 0.0f), BS);
			MapNode n = m_env->getMap().getNode(p);
			const ContentFeatures &c = m_env->getGameDef()->ndef()->get(n);
			if (c.damage_per_second > damage_per_second) {
				damage_per_second = c.damage_per_second;
				nodename = c.name;
			}
		}

		// Top damage point
		v3s16 ptop = floatToInt(m_base_position +
			v3f(0.0f, dam_top * BS, 0.0f), BS);
		MapNode ntop = m_env->getMap().getNode(ptop);
		const ContentFeatures &c = m_env->getGameDef()->ndef()->get(ntop);
		if (c.damage_per_second > damage_per_second) {
			damage_per_second = c.damage_per_second;
			nodename = c.name;
		}

		if (damage_per_second != 0 && m_hp > 0) {
			s32 newhp = (s32)m_hp - (s32)damage_per_second;
			PlayerHPChangeReason reason(PlayerHPChangeReason::NODE_DAMAGE, nodename);
			setHP(newhp, reason);
			m_env->getGameDef()->SendPlayerHPOrDie(this, reason);
		}
	}

	if (!m_properties_sent) {
		m_properties_sent = true;
		std::string str = getPropertyPacket();
		// create message and add to list
		ActiveObjectMessage aom(getId(), true, str);
		m_messages_out.push(aom);
		m_env->getScriptIface()->player_event(this, "properties_changed");
	}

	// If attached, check that our parent is still there. If it isn't, detach.
	if (m_attachment_parent_id && !isAttached()) {
		m_attachment_parent_id = 0;
		m_attachment_bone = "";
		m_attachment_position = v3f(0.0f, 0.0f, 0.0f);
		m_attachment_rotation = v3f(0.0f, 0.0f, 0.0f);
		setBasePosition(m_last_good_position);
		m_env->getGameDef()->SendMovePlayer(m_peer_id);
	}

	//dstream<<"PlayerSAO::step: dtime: "<<dtime<<std::endl;

	// Set lag pool maximums based on estimated lag
	const float LAG_POOL_MIN = 5.0f;
	float lag_pool_max = m_env->getMaxLagEstimate() * 2.0f;
	if(lag_pool_max < LAG_POOL_MIN)
		lag_pool_max = LAG_POOL_MIN;
	m_dig_pool.setMax(lag_pool_max);
	m_move_pool.setMax(lag_pool_max);

	// Increment cheat prevention timers
	m_dig_pool.add(dtime);
	m_move_pool.add(dtime);
	m_time_from_last_teleport += dtime;
	m_time_from_last_punch += dtime;
	m_nocheat_dig_time += dtime;
	m_max_speed_override_time = MYMAX(m_max_speed_override_time - dtime, 0.0f);

	// Each frame, parent position is copied if the object is attached,
	// otherwise it's calculated normally.
	// If the object gets detached this comes into effect automatically from
	// the last known origin.
	if (isAttached()) {
		v3f pos = m_env->getActiveObject(m_attachment_parent_id)->getBasePosition();
		m_last_good_position = pos;
		setBasePosition(pos);
	}

	if (!send_recommended)
		return;

	if (m_position_not_sent) {
		m_position_not_sent = false;
		float update_interval = m_env->getSendRecommendedInterval();
		v3f pos;
		// When attached, the position is only sent to clients where the
		// parent isn't known
		if (isAttached())
			pos = m_last_good_position;
		else
			pos = m_base_position;

		std::string str = gob_cmd_update_position(
			pos,
			v3f(0.0f, 0.0f, 0.0f),
			v3f(0.0f, 0.0f, 0.0f),
			m_rotation,
			true,
			false,
			update_interval
		);
		// create message and add to list
		ActiveObjectMessage aom(getId(), false, str);
		m_messages_out.push(aom);
	}

	if (!m_armor_groups_sent) {
		m_armor_groups_sent = true;
		std::string str = gob_cmd_update_armor_groups(
				m_armor_groups);
		// create message and add to list
		ActiveObjectMessage aom(getId(), true, str);
		m_messages_out.push(aom);
	}

	if (!m_physics_override_sent) {
		m_physics_override_sent = true;
		std::string str = gob_cmd_update_physics_override(m_physics_override_speed,
				m_physics_override_jump, m_physics_override_gravity,
				m_physics_override_sneak, m_physics_override_sneak_glitch,
				m_physics_override_new_move);
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

	if (!m_bone_position_sent) {
		m_bone_position_sent = true;
		for (std::unordered_map<std::string, core::vector2d<v3f>>::const_iterator
				ii = m_bone_position.begin(); ii != m_bone_position.end(); ++ii) {
			std::string str = gob_cmd_update_bone_position((*ii).first,
					(*ii).second.X, (*ii).second.Y);
			// create message and add to list
			ActiveObjectMessage aom(getId(), true, str);
			m_messages_out.push(aom);
		}
	}

	if (!m_attachment_sent) {
		m_attachment_sent = true;
		std::string str = gob_cmd_update_attachment(m_attachment_parent_id,
				m_attachment_bone, m_attachment_position, m_attachment_rotation);
		// create message and add to list
		ActiveObjectMessage aom(getId(), true, str);
		m_messages_out.push(aom);
	}
}

void PlayerSAO::setBasePosition(const v3f &position)
{
	if (m_player && position != m_base_position)
		m_player->setDirty(true);

	// This needs to be ran for attachments too
	ServerActiveObject::setBasePosition(position);

	// Updating is not wanted/required for player migration
	if (m_env) {
		m_position_not_sent = true;
	}
}

void PlayerSAO::setPos(const v3f &pos)
{
	if(isAttached())
		return;

	// Send mapblock of target location
	v3s16 blockpos = v3s16(pos.X / MAP_BLOCKSIZE, pos.Y / MAP_BLOCKSIZE, pos.Z / MAP_BLOCKSIZE);
	m_env->getGameDef()->SendBlock(m_peer_id, blockpos);

	setBasePosition(pos);
	// Movement caused by this command is always valid
	m_last_good_position = pos;
	m_move_pool.empty();
	m_time_from_last_teleport = 0.0;
	m_env->getGameDef()->SendMovePlayer(m_peer_id);
}

void PlayerSAO::moveTo(v3f pos, bool continuous)
{
	if(isAttached())
		return;

	setBasePosition(pos);
	// Movement caused by this command is always valid
	m_last_good_position = pos;
	m_move_pool.empty();
	m_time_from_last_teleport = 0.0;
	m_env->getGameDef()->SendMovePlayer(m_peer_id);
}

void PlayerSAO::setPlayerYaw(const float yaw)
{
	v3f rotation(0, yaw, 0);
	if (m_player && yaw != m_rotation.Y)
		m_player->setDirty(true);

	// Set player model yaw, not look view
	UnitSAO::setRotation(rotation);
}

void PlayerSAO::setFov(const float fov)
{
	if (m_player && fov != m_fov)
		m_player->setDirty(true);

	m_fov = fov;
}

void PlayerSAO::setWantedRange(const s16 range)
{
	if (m_player && range != m_wanted_range)
		m_player->setDirty(true);

	m_wanted_range = range;
}

void PlayerSAO::setPlayerYawAndSend(const float yaw)
{
	setPlayerYaw(yaw);
	m_env->getGameDef()->SendMovePlayer(m_peer_id);
}

void PlayerSAO::setLookPitch(const float pitch)
{
	if (m_player && pitch != m_pitch)
		m_player->setDirty(true);

	m_pitch = pitch;
}

void PlayerSAO::setLookPitchAndSend(const float pitch)
{
	setLookPitch(pitch);
	m_env->getGameDef()->SendMovePlayer(m_peer_id);
}

u16 PlayerSAO::punch(v3f dir,
	const ToolCapabilities *toolcap,
	ServerActiveObject *puncher,
	float time_from_last_punch)
{
	if (!toolcap)
		return 0;

	FATAL_ERROR_IF(!puncher, "Punch action called without SAO");

	// No effect if PvP disabled or if immortal
	if (isImmortal() || !g_settings->getBool("enable_pvp")) {
		if (puncher->getType() == ACTIVEOBJECT_TYPE_PLAYER) {
			std::string str = gob_cmd_punched(getHP());
			// create message and add to list
			ActiveObjectMessage aom(getId(), true, str);
			m_messages_out.push(aom);
			return 0;
		}
	}

	s32 old_hp = getHP();
	HitParams hitparams = getHitParams(m_armor_groups, toolcap,
			time_from_last_punch);

	PlayerSAO *playersao = m_player->getPlayerSAO();

	bool damage_handled = m_env->getScriptIface()->on_punchplayer(playersao,
				puncher, time_from_last_punch, toolcap, dir,
				hitparams.hp);

	if (!damage_handled) {
		setHP((s32)getHP() - (s32)hitparams.hp,
				PlayerHPChangeReason(PlayerHPChangeReason::PLAYER_PUNCH, puncher));
	} else { // override client prediction
		if (puncher->getType() == ACTIVEOBJECT_TYPE_PLAYER) {
			std::string str = gob_cmd_punched(getHP());
			// create message and add to list
			ActiveObjectMessage aom(getId(), true, str);
			m_messages_out.push(aom);
		}
	}

	actionstream << puncher->getDescription() << " (id=" << puncher->getId() <<
		", hp=" << puncher->getHP() << ") punched " <<
		getDescription() << " (id=" << m_id << ", hp=" << m_hp <<
		"), damage=" << (old_hp - (s32)getHP()) <<
		(damage_handled ? " (handled by Lua)" : "") << std::endl;

	return hitparams.wear;
}

void PlayerSAO::setHP(s32 hp, const PlayerHPChangeReason &reason)
{
	s32 oldhp = m_hp;

	hp = rangelim(hp, 0, m_prop.hp_max);

	if (oldhp != hp) {
		s32 hp_change = m_env->getScriptIface()->on_player_hpchange(this, hp - oldhp, reason);
		if (hp_change == 0)
			return;

		hp = rangelim(oldhp + hp_change, 0, m_prop.hp_max);
	}

	if (hp < oldhp && isImmortal())
		return;

	m_hp = hp;

	// Update properties on death
	if ((hp == 0) != (oldhp == 0))
		m_properties_sent = false;
}

void PlayerSAO::setBreath(const u16 breath, bool send)
{
	if (m_player && breath != m_breath)
		m_player->setDirty(true);

	m_breath = rangelim(breath, 0, m_prop.breath_max);

	if (send)
		m_env->getGameDef()->SendPlayerBreath(this);
}

Inventory *PlayerSAO::getInventory() const
{
	return m_player ? &m_player->inventory : nullptr;
}

InventoryLocation PlayerSAO::getInventoryLocation() const
{
	InventoryLocation loc;
	loc.setPlayer(m_player->getName());
	return loc;
}

u16 PlayerSAO::getWieldIndex() const
{
	return m_player->getWieldIndex();
}

ItemStack PlayerSAO::getWieldedItem(ItemStack *selected, ItemStack *hand) const
{
	return m_player->getWieldedItem(selected, hand);
}

bool PlayerSAO::setWieldedItem(const ItemStack &item)
{
	InventoryList *mlist = m_player->inventory.getList(getWieldList());
	if (mlist) {
		mlist->changeItem(m_player->getWieldIndex(), item);
		return true;
	}
	return false;
}

void PlayerSAO::disconnected()
{
	m_peer_id = 0;
	m_pending_removal = true;
}

void PlayerSAO::unlinkPlayerSessionAndSave()
{
	assert(m_player->getPlayerSAO() == this);
	m_player->setPeerId(PEER_ID_INEXISTENT);
	m_env->savePlayer(m_player);
	m_player->setPlayerSAO(NULL);
	m_env->removePlayer(m_player);
}

std::string PlayerSAO::getPropertyPacket()
{
	m_prop.is_visible = (true);
	return gob_cmd_set_properties(m_prop);
}

void PlayerSAO::setMaxSpeedOverride(const v3f &vel)
{
	if (m_max_speed_override_time == 0.0f)
		m_max_speed_override = vel;
	else
		m_max_speed_override += vel;
	if (m_player) {
		float accel = MYMIN(m_player->movement_acceleration_default,
				m_player->movement_acceleration_air);
		m_max_speed_override_time = m_max_speed_override.getLength() / accel / BS;
	}
}

bool PlayerSAO::checkMovementCheat()
{
	if (isAttached() || m_is_singleplayer ||
			g_settings->getBool("disable_anticheat")) {
		m_last_good_position = m_base_position;
		return false;
	}

	bool cheated = false;
	/*
		Check player movements

		NOTE: Actually the server should handle player physics like the
		client does and compare player's position to what is calculated
		on our side. This is required when eg. players fly due to an
		explosion. Altough a node-based alternative might be possible
		too, and much more lightweight.
	*/

	float override_max_H, override_max_V;
	if (m_max_speed_override_time > 0.0f) {
		override_max_H = MYMAX(fabs(m_max_speed_override.X), fabs(m_max_speed_override.Z));
		override_max_V = fabs(m_max_speed_override.Y);
	} else {
		override_max_H = override_max_V = 0.0f;
	}

	float player_max_walk = 0; // horizontal movement
	float player_max_jump = 0; // vertical upwards movement

	if (m_privs.count("fast") != 0)
		player_max_walk = m_player->movement_speed_fast; // Fast speed
	else
		player_max_walk = m_player->movement_speed_walk; // Normal speed
	player_max_walk *= m_physics_override_speed;
	player_max_walk = MYMAX(player_max_walk, override_max_H);

	player_max_jump = m_player->movement_speed_jump * m_physics_override_jump;
	// FIXME: Bouncy nodes cause practically unbound increase in Y speed,
	//        until this can be verified correctly, tolerate higher jumping speeds
	player_max_jump *= 2.0;
	player_max_jump = MYMAX(player_max_jump, override_max_V);

	// Don't divide by zero!
	if (player_max_walk < 0.0001f)
		player_max_walk = 0.0001f;
	if (player_max_jump < 0.0001f)
		player_max_jump = 0.0001f;

	v3f diff = (m_base_position - m_last_good_position);
	float d_vert = diff.Y;
	diff.Y = 0;
	float d_horiz = diff.getLength();
	float required_time = d_horiz / player_max_walk;

	// FIXME: Checking downwards movement is not easily possible currently,
	//        the server could calculate speed differences to examine the gravity
	if (d_vert > 0) {
		// In certain cases (water, ladders) walking speed is applied vertically
		float s = MYMAX(player_max_jump, player_max_walk);
		required_time = MYMAX(required_time, d_vert / s);
	}

	if (m_move_pool.grab(required_time)) {
		m_last_good_position = m_base_position;
	} else {
		const float LAG_POOL_MIN = 5.0;
		float lag_pool_max = m_env->getMaxLagEstimate() * 2.0;
		lag_pool_max = MYMAX(lag_pool_max, LAG_POOL_MIN);
		if (m_time_from_last_teleport > lag_pool_max) {
			actionstream << "Player " << m_player->getName()
					<< " moved too fast; resetting position"
					<< std::endl;
			cheated = true;
		}
		setBasePosition(m_last_good_position);
	}
	return cheated;
}

bool PlayerSAO::getCollisionBox(aabb3f *toset) const
{
	//update collision box
	toset->MinEdge = m_prop.collisionbox.MinEdge * BS;
	toset->MaxEdge = m_prop.collisionbox.MaxEdge * BS;

	toset->MinEdge += m_base_position;
	toset->MaxEdge += m_base_position;
	return true;
}

bool PlayerSAO::getSelectionBox(aabb3f *toset) const
{
	if (!m_prop.is_visible || !m_prop.pointable) {
		return false;
	}

	toset->MinEdge = m_prop.selectionbox.MinEdge * BS;
	toset->MaxEdge = m_prop.selectionbox.MaxEdge * BS;

	return true;
}

float PlayerSAO::getZoomFOV() const
{
	return m_prop.zoom_fov;
}
