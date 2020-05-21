/*
Minetest
Copyright (C) 2010-2013 celeron55, Perttu Ahola <celeron55@gmail.com>
Copyright (C) 2013-2020 Minetest core developers & community

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

#include "luaentity_sao.h"
#include "collision.h"
#include "constants.h"
#include "player_sao.h"
#include "scripting_server.h"
#include "server.h"
#include "serverenvironment.h"

LuaEntitySAO::LuaEntitySAO(ServerEnvironment *env, v3f pos, const std::string &data)
	: UnitSAO(env, pos)
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

		name = deSerializeString16(is);
		state = deSerializeString32(is);

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

	m_init_name = name;
	m_init_state = state;
	m_hp = hp;
	m_velocity = velocity;
	m_rotation = rotation;
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

void LuaEntitySAO::removingFromEnvironment()
{
	// Notify registered entity of deactivation
	if (m_registered)
		m_env->getScriptIface( )->luaentity_Deactivate(m_id);

	ServerActiveObject::removingFromEnvironment();
}

void LuaEntitySAO::step(float dtime, bool send_recommended)
{
	collisionMoveResult moveresult;
	v3f old_velocity;
	v3f new_velocity;

	if(!m_properties_sent)
	{
		m_properties_sent = true;
		std::string str = getPropertyPacket();
		// create message and add to list
		m_messages_out.emplace(getId(), true, str);
	}

	// If attached, check that our parent is still there. If it isn't, detach.
	if (m_attachment_parent_id && !isAttached()) {
		// This is handled when objects are removed from the map
		warningstream << "LuaEntitySAO::step() id=" << m_id <<
			" is attached to nonexistent parent. This is a bug." << std::endl;
		clearParentAttachment();
		sendPosition(false, true);
	}

	m_last_sent_position_timer += dtime;

	// Each frame, parent position is copied if the object is attached, otherwise it's calculated normally
	// If the object gets detached this comes into effect automatically from the last known origin
	if (isAttached()) {
		v3f pos = m_env->getActiveObject(m_attachment_parent_id)->getBasePosition();
		m_base_position = pos;
		m_velocity = v3f(0,0,0);
		m_acceleration = v3f(0,0,0);
		old_velocity = v3f(0,0,0);
		new_velocity = v3f(0,0,0);
	}
	else {
		old_velocity = m_velocity;

		if (m_turn_rate != 0) {
			// Precision adjustment avoids some rounding errors
			int dtime_fixed = MYMAX(1, dtime * 100);

			// Handle continuous or periodic rotation
			// This is set via obj:turn_by(yaw_delta, period, cycles)
			if (m_turn_period == 0) {
				m_rotation.Y = modulo360f(m_rotation.Y +
					(float)dtime_fixed * m_turn_rate);
			}
			else {
				// Calculate next interval, but don't overshoot
				float interval = MYMIN(m_turn_period, dtime_fixed);

				m_rotation.Y = modulo360f(m_rotation.Y +
					interval * m_turn_rate);
				m_turn_period = m_turn_period - interval;

				if (m_turn_period <= 0) { 
					// We're done, so stop turning and tidy up result
					m_turn_rate = 0.0f;
					m_turn_period = 0.0f;
					m_rotation.Y = myround(m_rotation.Y);
				}
			}

			if (m_speed != 0) {
				// Adjust horizontal velocity to account for new heading
				float yaw = m_rotation.Y + m_yaw_offset;
				m_velocity.X = -sin(yaw * core::DEGTORAD) * m_speed;
				m_velocity.Z = cos(yaw * core::DEGTORAD) * m_speed;
			}
		}
		else if (m_prop.automatic_face_movement_dir &&
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

		if (m_prop.physical) {
			aabb3f box = m_prop.collisionbox;
			box.MinEdge *= BS;
			box.MaxEdge *= BS;
			f32 pos_max_d = BS*0.25; // Distance per iteration
			v3f p_pos = m_base_position;
			v3f p_velocity = m_velocity;
			v3f p_acceleration = m_acceleration;
			IGameDef *gamedef = m_env->getGameDef();

			moveresult = collisionMoveSimple(m_env, gamedef,
					pos_max_d, box, m_prop.stepheight, dtime,
					&p_pos, &p_velocity, p_acceleration,
					this, m_prop.collideWithObjects);

			// Apply changes to position, acceleration, and velocity
			if(!m_velocity_lock) {
				m_velocity = p_velocity;
			}
			else {
				// Constant +/- speed overrides horizontal velocity
				// This is set via obj:set_speed(speed, yaw_offset)
				m_velocity.Y = p_velocity.Y;
			}
			m_base_position = p_pos;
			m_acceleration = p_acceleration;
			new_velocity = p_velocity;
		}
		else {
			m_base_position += dtime * m_velocity + 0.5 * dtime
					* dtime * m_acceleration;
			m_velocity += dtime * m_acceleration;
			new_velocity = m_velocity;
		}
	}

	if (m_registered) {
		m_env->getScriptIface()->luaentity_Step(m_id, dtime,
			m_base_position, m_rotation, new_velocity, old_velocity,
			m_prop.physical ? &moveresult : nullptr);
	}

	if (!send_recommended)
		return;

	if(!isAttached())
	{
		// TODO: force send when acceleration changes enough?
		float minchange = 0.2 * BS;
		if (m_last_sent_position_timer > 1.0)
			minchange = 0.01 * BS;
		else if (m_last_sent_position_timer > 0.2)
			minchange = 0.05 * BS;

		float pos_d = m_base_position.getDistanceFrom(m_last_sent_position);
		pos_d += m_last_sent_move_precision;
		float vel_d = m_velocity.getDistanceFrom(m_last_sent_velocity);

		// TODO: Find a better way to overcome buggy client-side
		// synchronization with velocity changes since the client
		// does not correctly predict entity-player collisions.
		bool force_send = m_last_sent_position_timer > 0.5 &&
			m_velocity.getLength() > 0.05 && m_prop.collideWithObjects;

		if (force_send || pos_d > minchange || vel_d > minchange ||
				std::fabs(m_rotation.X - m_last_sent_rotation.X) > 1.0f ||
				std::fabs(m_rotation.Y - m_last_sent_rotation.Y) > 1.0f ||
				std::fabs(m_rotation.Z - m_last_sent_rotation.Z) > 1.0f) {

			sendPosition(true, false);
		}
	}

	sendOutdatedData();
}

std::string LuaEntitySAO::getClientInitializationData(u16 protocol_version)
{
	std::ostringstream os(std::ios::binary);

	// PROTOCOL_VERSION >= 37
	writeU8(os, 1); // version
	os << serializeString16(""); // name
	writeU8(os, 0); // is_player
	writeU16(os, getId()); //id
	writeV3F32(os, m_base_position);
	writeV3F32(os, m_rotation);
	writeU16(os, m_hp);

	std::ostringstream msg_os(std::ios::binary);
	msg_os << serializeString32(getPropertyPacket()); // message 1
	msg_os << serializeString32(generateUpdateArmorGroupsCommand()); // 2
	msg_os << serializeString32(generateUpdateAnimationCommand()); // 3
	for (const auto &bone_pos : m_bone_position) {
		msg_os << serializeString32(generateUpdateBonePositionCommand(
			bone_pos.first, bone_pos.second.X, bone_pos.second.Y)); // 3 + N
	}
	msg_os << serializeString32(generateUpdateAttachmentCommand()); // 4 + m_bone_position.size

	int message_count = 4 + m_bone_position.size();

	for (const auto &id : getAttachmentChildIds()) {
		if (ServerActiveObject *obj = m_env->getActiveObject(id)) {
			message_count++;
			msg_os << serializeString32(obj->generateUpdateInfantCommand(
				id, protocol_version));
		}
	}

	msg_os << serializeString32(generateSetTextureModCommand());
	message_count++;

	writeU8(os, message_count);
	std::string serialized = msg_os.str();
	os.write(serialized.c_str(), serialized.size());

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
	os<<serializeString16(m_init_name);
	// state
	if(m_registered){
		std::string state = m_env->getScriptIface()->
			luaentity_GetStaticdata(m_id);
		os<<serializeString32(state);
	} else {
		os<<serializeString32(m_init_state);
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
				PlayerHPChangeReason(PlayerHPChangeReason::PLAYER_PUNCH, puncher));

			// create message and add to list
			sendPunchCommand();
		}
	}

	if (getHP() == 0 && !isGone()) {
		clearParentAttachment();
		clearChildAttachments();
		m_env->getScriptIface()->luaentity_on_death(m_id, puncher);
		m_pending_removal = true;
	}

	actionstream << puncher->getDescription() << " (id=" << puncher->getId() <<
			", hp=" << puncher->getHP() << ") punched " <<
			getDescription() << " (id=" << m_id << ", hp=" << m_hp <<
			"), damage=" << (old_hp - (s32)getHP()) <<
			(damage_handled ? " (handled by Lua)" : "") << std::endl;

	// TODO: give Lua control over wear
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
	std::ostringstream oss;
	oss << "LuaEntitySAO \"" << m_init_name << "\" ";
	auto pos = floatToInt(m_base_position, BS);
	oss << "at " << PP(pos);
	return oss.str();
}

void LuaEntitySAO::setHP(s32 hp, const PlayerHPChangeReason &reason)
{
	m_hp = rangelim(hp, 0, U16_MAX);
}

u16 LuaEntitySAO::getHP() const
{
	return m_hp;
}

void LuaEntitySAO::setSpeed(float speed, float yaw_offset)
{
	lockVelocity( );
	m_yaw_offset = m_prop.yaw_origin + yaw_offset;

	// Avoid cumulative rounding errors near zero
	if (fabs( speed ) > 0.001) {
		float yaw = m_rotation.Y + m_yaw_offset;
		m_velocity.X = -sin(yaw * core::DEGTORAD) * speed;
		m_velocity.Z = cos(yaw * core::DEGTORAD) * speed;
		m_speed = speed;
	}
	else {
		// Zero speed will restrict all horizontal movement
		m_velocity.X = 0.0f;
		m_velocity.Z = 0.0f;
		m_speed = 0.0f;
	}
}

void LuaEntitySAO::setSpeedLateral(float speed_x, float speed_y)
{
	lockVelocity( );

	// Calculate speed with correct sign
	float speed = v3f(speed_x, 0, speed_y).getLength() * (speed_y < 0 ? -1 : 1);

	if (fabs(speed) > 0.001) {
		// Account for negative speed_y by inverting vector
		m_yaw_offset = speed_y >= 0 ?
			m_prop.yaw_origin + atan2(-speed_x, speed_y) * core::RADTODEG :
			m_prop.yaw_origin + atan2(speed_x, -speed_y) * core::RADTODEG;

		float yaw = m_rotation.Y + m_yaw_offset;
		m_velocity.X = -sin(yaw * core::DEGTORAD) * speed;
		m_velocity.Z = cos(yaw * core::DEGTORAD) * speed;
		m_speed = speed;
	}
	else {
		m_yaw_offset = 0.0f;
		m_velocity.X = 0.0f;
		m_velocity.Z = 0.0f;
		m_speed = 0.0f;
	}
}

void LuaEntitySAO::addSpeed(float speed)
{
	lockVelocity( );

	m_speed += speed;
	m_velocity.X = -sin((m_rotation.Y + m_yaw_offset) * core::DEGTORAD) * m_speed;
	m_velocity.Z = cos((m_rotation.Y + m_yaw_offset) * core::DEGTORAD) * m_speed;
}

float LuaEntitySAO::getSpeed()
{
	return m_speed;
}

void LuaEntitySAO::lockVelocity()
{
	m_velocity_lock = true;
	m_acceleration.X = 0.0f;
	m_acceleration.Z = 0.0f;
}

void LuaEntitySAO::unlockVelocity()
{
	m_speed = 0.0f;
	m_yaw_offset = 0.0f;
	m_velocity_lock = false;
}

bool LuaEntitySAO::isVelocityLocked()
{
	return m_velocity_lock;
}

void LuaEntitySAO::setVelocity(v3f velocity)
{
	if (!m_velocity_lock) {
		m_velocity = velocity;
	}
	else {
		m_velocity.Y = velocity.Y;
	}
}

void LuaEntitySAO::setVelocityHorz(float vel_x, float vel_z)
{
	if (!m_velocity_lock) {
		m_velocity.X = vel_x;
		m_velocity.Z = vel_z;
	}
}

void LuaEntitySAO::setVelocityVert(float vel_y)
{
	m_velocity.Y = vel_y;
}

void LuaEntitySAO::addVelocity(v3f velocity)
{
	if (!m_velocity_lock)
		m_velocity += velocity;
	else
		m_velocity.Y += velocity.Y;
}

v3f LuaEntitySAO::getVelocity()
{
	return m_velocity;
}

void LuaEntitySAO::setAcceleration(v3f acceleration)
{
	if (!m_velocity_lock)
		m_acceleration = acceleration;
	else
		m_acceleration.Y = acceleration.Y;
}

void LuaEntitySAO::setAccelerationVert(float acc_y)
{
	m_acceleration.Y = acc_y;
}

v3f LuaEntitySAO::getAcceleration()
{
	return m_acceleration;
}

void LuaEntitySAO::setRotation(v3f rotation)
{
	UnitSAO::setRotation(rotation);
	if (m_speed != 0) {
		m_velocity.X = -sin((m_rotation.Y + m_yaw_offset) * core::DEGTORAD) * m_speed;
		m_velocity.Z = cos((m_rotation.Y + m_yaw_offset) * core::DEGTORAD) * m_speed;
	}
}

void LuaEntitySAO::addRotation(v3f rotation)
{
	UnitSAO::addRotation(rotation);
	if (m_speed != 0) {
		m_velocity.X = -sin((m_rotation.Y + m_yaw_offset) * core::DEGTORAD) * m_speed;
		m_velocity.Z = cos((m_rotation.Y + m_yaw_offset) * core::DEGTORAD) * m_speed;
	}
}

void LuaEntitySAO::turnBy(float yaw_delta, float period, u16 cycles)
{
	if (period == 0 || yaw_delta == 0) {
		m_turn_rate = 0.0f;  // Abort turning in progress
		m_turn_period = 0.0f;
		return;
	}

	// Use 1/100s precision for period to improve accuracy
	m_turn_rate = yaw_delta / period / 100;
	if (cycles > 0)
		m_turn_period = 100 * period * cycles;
	else
		m_turn_period = 0.0f;
}

void LuaEntitySAO::setTextureMod(const std::string &mod)
{
	m_current_texture_modifier = mod;
	// create message and add to list
	m_messages_out.emplace(getId(), true, generateSetTextureModCommand());
}

std::string LuaEntitySAO::getTextureMod() const
{
	return m_current_texture_modifier;
}


std::string LuaEntitySAO::generateSetTextureModCommand() const
{
	std::ostringstream os(std::ios::binary);
	// command
	writeU8(os, AO_CMD_SET_TEXTURE_MOD);
	// parameters
	os << serializeString16(m_current_texture_modifier);
	return os.str();
}

std::string LuaEntitySAO::generateSetSpriteCommand(v2s16 p, u16 num_frames,
	f32 framelength, bool select_horiz_by_yawpitch)
{
	std::ostringstream os(std::ios::binary);
	// command
	writeU8(os, AO_CMD_SET_SPRITE);
	// parameters
	writeV2S16(os, p);
	writeU16(os, num_frames);
	writeF32(os, framelength);
	writeU8(os, select_horiz_by_yawpitch);
	return os.str();
}

void LuaEntitySAO::setSprite(v2s16 p, int num_frames, float framelength,
		bool select_horiz_by_yawpitch)
{
	std::string str = generateSetSpriteCommand(
		p,
		num_frames,
		framelength,
		select_horiz_by_yawpitch
	);
	// create message and add to list
	m_messages_out.emplace(getId(), true, str);
}

std::string LuaEntitySAO::getName()
{
	return m_init_name;
}

std::string LuaEntitySAO::getPropertyPacket()
{
	return generateSetPropertiesCommand(m_prop);
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

	std::string str = generateUpdatePositionCommand(
		m_base_position,
		m_velocity,
		m_acceleration,
		m_rotation,
		do_interpolate,
		is_movement_end,
		update_interval
	);
	// create message and add to list
	m_messages_out.emplace(getId(), false, str);
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
