/*
Minetest-c55
Copyright (C) 2010-2012 celeron55, Perttu Ahola <celeron55@gmail.com>

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
#include <iostream>
#include "content_sao_lua.h"

/*
	LuaEntitySAO
*/

LuaEntitySAO::LuaEntitySAO(ServerEnvironment *env, v3f pos,
		const std::string &name, const std::string &state):
	ServerActiveObject(env, pos),
	m_init_name(name),
	m_init_state(state),
	m_registered(false),
	m_prop(new LuaEntityProperties),
	m_velocity(0,0,0),
	m_acceleration(0,0,0),
	m_yaw(0),
	m_last_sent_yaw(0),
	m_last_sent_position(0,0,0),
	m_last_sent_velocity(0,0,0),
	m_last_sent_position_timer(0),
	m_last_sent_move_precision(0)
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
		lua_State *L = m_env->getLua();
		scriptapi_luaentity_rm(L, m_id);
	}
	delete m_prop;
}

void LuaEntitySAO::addedToEnvironment()
{
	ServerActiveObject::addedToEnvironment();

	// Create entity from name and state
	lua_State *L = m_env->getLua();
	m_registered = scriptapi_luaentity_add(L, m_id, m_init_name.c_str(),
			m_init_state.c_str());

	if(m_registered){
		// Get properties
		scriptapi_luaentity_get_properties(L, m_id, m_prop);
	}
}

ServerActiveObject* LuaEntitySAO::create(ServerEnvironment *env, v3f pos,
		const std::string &data)
{
	std::istringstream is(data, std::ios::binary);
	// read version
	u8 version = readU8(is);
	// check if version is supported
	if(version != 0)
		return NULL;
	// read name
	std::string name = deSerializeString(is);
	// read state
	std::string state = deSerializeLongString(is);
	// create object
	infostream<<"LuaEntitySAO::create(name=\""<<name<<"\" state=\""
			<<state<<"\")"<<std::endl;
	return new LuaEntitySAO(env, pos, name, state);
}

void LuaEntitySAO::step(float dtime, bool send_recommended)
{
	//errorstream << this << " step: old_vel=(" << m_velocity.X << "," << m_velocity.Y << "," << m_velocity.Z << ")" << std::endl;
	//errorstream << this << " step: old_pos=(" << m_base_position.X << "," << m_base_position.Y << "," << m_base_position.Z << ")" << std::endl;
	m_last_sent_position_timer += dtime;

	if(m_prop->physical){
		core::aabbox3d<f32> box = m_prop->collisionbox;
		box.MinEdge *= BS;
		box.MaxEdge *= BS;
		collisionMoveResult moveresult;
		f32 pos_max_d = BS*0.25; // Distance per iteration
		v3f p_pos = getBasePosition();
		v3f p_velocity = m_velocity;
		IGameDef *gamedef = m_env->getGameDef();
		moveresult = collisionMovePrecise(&m_env->getMap(), gamedef,
				pos_max_d, box, dtime, p_pos, p_velocity);
		// Apply results
		setBasePosition(p_pos);
		m_velocity = p_velocity;
		//if (moveresult.collides) errorstream << "collision" << std::endl;
		//if (moveresult.touching_ground) errorstream << "ground" << std::endl;
		m_velocity += dtime * m_acceleration;
	} else {
		m_base_position += dtime * m_velocity + 0.5 * dtime
				* dtime * m_acceleration;
		m_velocity += dtime * m_acceleration;
	}

	//errorstream << this << " step: new_vel=(" << m_velocity.X << "," << m_velocity.Y << "," << m_velocity.Z << ")" << std::endl;
	//errorstream << this << " step: new_pos=(" << m_base_position.X << "," << m_base_position.Y << "," << m_base_position.Z << ")" << std::endl;

	if(m_registered){
		lua_State *L = m_env->getLua();
		scriptapi_luaentity_step(L, m_id, dtime);
	}

	if(send_recommended == false)
		return;

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
	if(move_d > minchange || vel_d > minchange ||
			fabs(m_yaw - m_last_sent_yaw) > 1.0 ||
			m_last_sent_position_timer > 10.0){
		sendPosition(true, false);
	}
}

std::string LuaEntitySAO::getClientInitializationData()
{
	std::ostringstream os(std::ios::binary);
	// version
	writeU8(os, 0);
	// pos
	writeV3F1000(os, m_base_position);
	// yaw
	writeF1000(os, m_yaw);
	// properties
	std::ostringstream prop_os(std::ios::binary);
	m_prop->serialize(prop_os);
	os<<serializeLongString(prop_os.str());
	// return result
	return os.str();
}

std::string LuaEntitySAO::getStaticData()
{
	infostream<<__FUNCTION_NAME<<std::endl;
	std::ostringstream os(std::ios::binary);
	// version
	writeU8(os, 0);
	// name
	os<<serializeString(m_init_name);
	// state
	if(m_registered){
		lua_State *L = m_env->getLua();
		std::string state = scriptapi_luaentity_get_staticdata(L, m_id);
		os<<serializeLongString(state);
	} else {
		os<<serializeLongString(m_init_state);
	}
	return os.str();
}

void LuaEntitySAO::punch(ServerActiveObject *puncher, float time_from_last_punch)
{
	if(!m_registered){
		// Delete unknown LuaEntities when punched
		m_removed = true;
		return;
	}
	lua_State *L = m_env->getLua();
	scriptapi_luaentity_punch(L, m_id, puncher, time_from_last_punch);
}

void LuaEntitySAO::rightClick(ServerActiveObject *clicker)
{
	if(!m_registered)
		return;
	lua_State *L = m_env->getLua();
	scriptapi_luaentity_rightclick(L, m_id, clicker);
}

void LuaEntitySAO::setPos(v3f pos)
{
	m_base_position = pos;
	sendPosition(false, true);
}

void LuaEntitySAO::moveTo(v3f pos, bool continuous)
{
	m_base_position = pos;
	if(!continuous)
		sendPosition(true, true);
}

float LuaEntitySAO::getMinimumSavedMovement()
{
	return 0.1 * BS;
}

void LuaEntitySAO::setVelocity(v3f velocity)
{
	//errorstream << this << " Set velocity: (" << velocity.X << "," << velocity.Y << "," <<velocity.Z << ")" << std::endl;
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

void LuaEntitySAO::setYaw(float yaw)
{
	m_yaw = yaw;
}

float LuaEntitySAO::getYaw()
{
	return m_yaw;
}

void LuaEntitySAO::setTextureMod(const std::string &mod)
{
	std::ostringstream os(std::ios::binary);
	// command (1 = set texture modification)
	writeU8(os, AO_Message_type::SetTextureMod);
	// parameters
	os<<serializeString(mod);
	// create message and add to list
	ActiveObjectMessage aom(getId(), false, os.str());
	m_messages_out.push_back(aom);
}

void LuaEntitySAO::setSprite(v2s16 p, int num_frames, float framelength,
		bool select_horiz_by_yawpitch)
{
	std::ostringstream os(std::ios::binary);
	// command (2 = set sprite)
	writeU8(os, AO_Message_type::SetSprite);
	// parameters
	writeV2S16(os, p);
	writeU16(os, num_frames);
	writeF1000(os, framelength);
	writeU8(os, select_horiz_by_yawpitch);
	// create message and add to list
	ActiveObjectMessage aom(getId(), false, os.str());
	m_messages_out.push_back(aom);
}

std::string LuaEntitySAO::getName()
{
	return m_init_name;
}

void LuaEntitySAO::sendPosition(bool do_interpolate, bool is_movement_end)
{
	m_last_sent_move_precision = m_base_position.getDistanceFrom(
			m_last_sent_position);
	m_last_sent_position_timer = 0;
	m_last_sent_yaw = m_yaw;
	m_last_sent_position = m_base_position;
	m_last_sent_velocity = m_velocity;
	//m_last_sent_acceleration = m_acceleration;

	float update_interval = m_env->getSendRecommendedInterval();

	std::ostringstream os(std::ios::binary);
	// command (0 = update position)
	writeU8(os, AO_Message_type::SetPosition);

	// do_interpolate
	writeU8(os, do_interpolate);
	// pos
	writeV3F1000(os, m_base_position);
	// velocity
	writeV3F1000(os, m_velocity);
	// acceleration
	writeV3F1000(os, m_acceleration);
	// yaw
	writeF1000(os, m_yaw);
	// is_end_position (for interpolation)
	writeU8(os, is_movement_end);
	// update_interval (for interpolation)
	writeF1000(os, update_interval);

	// create message and add to list
	ActiveObjectMessage aom(getId(), false, os.str());
	m_messages_out.push_back(aom);
}

// Prototype
LuaEntitySAO proto_LuaEntitySAO(NULL, v3f(0,0,0), "_prototype", "");
