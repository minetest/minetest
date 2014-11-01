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
#include "util/mathconstants.h"
#include "collision.h"
#include "environment.h"
#include "settings.h"
#include "main.h" // For g_profiler
#include "profiler.h"
#include "serialization.h" // For compressZlib
#include "tool.h" // For ToolCapabilities
#include "gamedef.h"
#include "player.h"
#include "scripting_game.h"
#include "genericobject.h"
#include "log.h"

std::map<u16, ServerActiveObject::Factory> ServerActiveObject::m_types;

/*
	DummyLoadSAO
*/

class DummyLoadSAO : public ServerActiveObject
{
public:
	DummyLoadSAO(ServerEnvironment *env, v3f pos, u8 type):
		ServerActiveObject(env, pos)
	{
		ServerActiveObject::registerType(type, create);
	}
	// Pretend to be the test object (to fool the client)
	u8 getType() const
	{ return ACTIVEOBJECT_TYPE_TEST; }
	// And never save to disk
	bool isStaticAllowed() const
	{ return false; }
	
	static ServerActiveObject* create(ServerEnvironment *env, v3f pos,
			const std::string &data)
	{
		return new DummyLoadSAO(env, pos, 0);
	}

	void step(float dtime, bool send_recommended)
	{
		m_removed = true;
		infostream<<"DummyLoadSAO step"<<std::endl;
	}

	bool getCollisionBox(aabb3f *toset) {
		return false;
	}

	bool collideWithObjects() {
		return false;
	}

private:
};

// Prototype (registers item for deserialization)
DummyLoadSAO proto1_DummyLoadSAO(NULL, v3f(0,0,0), ACTIVEOBJECT_TYPE_RAT);
DummyLoadSAO proto2_DummyLoadSAO(NULL, v3f(0,0,0), ACTIVEOBJECT_TYPE_OERKKI1);
DummyLoadSAO proto3_DummyLoadSAO(NULL, v3f(0,0,0), ACTIVEOBJECT_TYPE_FIREFLY);
DummyLoadSAO proto4_DummyLoadSAO(NULL, v3f(0,0,0), ACTIVEOBJECT_TYPE_MOBV2);

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
	u8 getType() const
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
			m_removed = true;
			return;
		}

		m_base_position.Y += dtime * BS * 2;
		if(m_base_position.Y > 8*BS)
			m_base_position.Y = 2*BS;

		if(send_recommended == false)
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
			m_messages_out.push_back(aom);
		}
	}

	bool getCollisionBox(aabb3f *toset) {
		return false;
	}

	bool collideWithObjects() {
		return false;
	}

private:
	float m_timer1;
	float m_age;
};

// Prototype (registers item for deserialization)
TestSAO proto_TestSAO(NULL, v3f(0,0,0));

/*
	ItemSAO

	DEPRECATED: New dropped items are implemented in Lua; see
	            builtin/item_entity.lua.
*/

class ItemSAO : public ServerActiveObject
{
public:
	u8 getType() const
	{ return ACTIVEOBJECT_TYPE_ITEM; }
	
	float getMinimumSavedMovement()
	{ return 0.1*BS; }

	static ServerActiveObject* create(ServerEnvironment *env, v3f pos,
			const std::string &data)
	{
		std::istringstream is(data, std::ios::binary);
		char buf[1];
		// read version
		is.read(buf, 1);
		u8 version = buf[0];
		// check if version is supported
		if(version != 0)
			return NULL;
		std::string itemstring = deSerializeString(is);
		infostream<<"create(): Creating item \""
				<<itemstring<<"\""<<std::endl;
		return new ItemSAO(env, pos, itemstring);
	}

	ItemSAO(ServerEnvironment *env, v3f pos,
			const std::string &itemstring):
		ServerActiveObject(env, pos),
		m_itemstring(itemstring),
		m_itemstring_changed(false),
		m_speed_f(0,0,0),
		m_last_sent_position(0,0,0)
	{
		ServerActiveObject::registerType(getType(), create);
	}

	void step(float dtime, bool send_recommended)
	{
		ScopeProfiler sp2(g_profiler, "step avg", SPT_AVG);

		assert(m_env);

		const float interval = 0.2;
		if(m_move_interval.step(dtime, interval)==false)
			return;
		dtime = interval;
		
		core::aabbox3d<f32> box(-BS/3.,0.0,-BS/3., BS/3.,BS*2./3.,BS/3.);
		collisionMoveResult moveresult;
		// Apply gravity
		m_speed_f += v3f(0, -dtime*9.81*BS, 0);
		// Maximum movement without glitches
		f32 pos_max_d = BS*0.25;
		// Limit speed
		if(m_speed_f.getLength()*dtime > pos_max_d)
			m_speed_f *= pos_max_d / (m_speed_f.getLength()*dtime);
		v3f pos_f = getBasePosition();
		v3f pos_f_old = pos_f;
		v3f accel_f = v3f(0,0,0);
		f32 stepheight = 0;
		moveresult = collisionMoveSimple(m_env,m_env->getGameDef(),
				pos_max_d, box, stepheight, dtime,
				pos_f, m_speed_f, accel_f);
		
		if(send_recommended == false)
			return;

		if(pos_f.getDistanceFrom(m_last_sent_position) > 0.05*BS)
		{
			setBasePosition(pos_f);
			m_last_sent_position = pos_f;

			std::ostringstream os(std::ios::binary);
			// command (0 = update position)
			writeU8(os, 0);
			// pos
			writeV3F1000(os, m_base_position);
			// create message and add to list
			ActiveObjectMessage aom(getId(), false, os.str());
			m_messages_out.push_back(aom);
		}
		if(m_itemstring_changed)
		{
			m_itemstring_changed = false;

			std::ostringstream os(std::ios::binary);
			// command (1 = update itemstring)
			writeU8(os, 1);
			// itemstring
			os<<serializeString(m_itemstring);
			// create message and add to list
			ActiveObjectMessage aom(getId(), false, os.str());
			m_messages_out.push_back(aom);
		}
	}

	std::string getClientInitializationData(u16 protocol_version)
	{
		std::ostringstream os(std::ios::binary);
		// version
		writeU8(os, 0);
		// pos
		writeV3F1000(os, m_base_position);
		// itemstring
		os<<serializeString(m_itemstring);
		return os.str();
	}

	std::string getStaticData()
	{
		infostream<<__FUNCTION_NAME<<std::endl;
		std::ostringstream os(std::ios::binary);
		// version
		writeU8(os, 0);
		// itemstring
		os<<serializeString(m_itemstring);
		return os.str();
	}

	ItemStack createItemStack()
	{
		try{
			IItemDefManager *idef = m_env->getGameDef()->idef();
			ItemStack item;
			item.deSerialize(m_itemstring, idef);
			infostream<<__FUNCTION_NAME<<": m_itemstring=\""<<m_itemstring
					<<"\" -> item=\""<<item.getItemString()<<"\""
					<<std::endl;
			return item;
		}
		catch(SerializationError &e)
		{
			infostream<<__FUNCTION_NAME<<": serialization error: "
					<<"m_itemstring=\""<<m_itemstring<<"\""<<std::endl;
			return ItemStack();
		}
	}

	int punch(v3f dir,
			const ToolCapabilities *toolcap,
			ServerActiveObject *puncher,
			float time_from_last_punch)
	{
		// Take item into inventory
		ItemStack item = createItemStack();
		Inventory *inv = puncher->getInventory();
		if(inv != NULL)
		{
			std::string wieldlist = puncher->getWieldList();
			ItemStack leftover = inv->addItem(wieldlist, item);
			puncher->setInventoryModified();
			if(leftover.empty())
			{
				m_removed = true;
			}
			else
			{
				m_itemstring = leftover.getItemString();
				m_itemstring_changed = true;
			}
		}
		
		return 0;
	}

	bool getCollisionBox(aabb3f *toset) {
		return false;
	}

	bool collideWithObjects() {
		return false;
	}

private:
	std::string m_itemstring;
	bool m_itemstring_changed;
	v3f m_speed_f;
	v3f m_last_sent_position;
	IntervalLimiter m_move_interval;
};

// Prototype (registers item for deserialization)
ItemSAO proto_ItemSAO(NULL, v3f(0,0,0), "");

ServerActiveObject* createItemSAO(ServerEnvironment *env, v3f pos,
                                  const std::string &itemstring)
{
	return new ItemSAO(env, pos, itemstring);
}

/*
	LuaEntitySAO
*/

// Prototype (registers item for deserialization)
LuaEntitySAO proto_LuaEntitySAO(NULL, v3f(0,0,0), "_prototype", "");

LuaEntitySAO::LuaEntitySAO(ServerEnvironment *env, v3f pos,
		const std::string &name, const std::string &state):
	ServerActiveObject(env, pos),
	m_init_name(name),
	m_init_state(state),
	m_registered(false),
	m_hp(-1),
	m_velocity(0,0,0),
	m_acceleration(0,0,0),
	m_yaw(0),
	m_properties_sent(true),
	m_last_sent_yaw(0),
	m_last_sent_position(0,0,0),
	m_last_sent_velocity(0,0,0),
	m_last_sent_position_timer(0),
	m_last_sent_move_precision(0),
	m_armor_groups_sent(false),
	m_animation_speed(0),
	m_animation_blend(0),
	m_animation_sent(false),
	m_bone_position_sent(false),
	m_attachment_parent_id(0),
	m_attachment_sent(false)
{
	// Only register type if no environment supplied
	if(env == NULL){
		ServerActiveObject::registerType(getType(), create);
		return;
	}
	
	// Initialize something to armor groups
	m_armor_groups["fleshy"] = 100;
}

LuaEntitySAO::~LuaEntitySAO()
{
	if(m_registered){
		m_env->getScriptIface()->luaentity_Remove(m_id);
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
			luaentity_Activate(m_id, m_init_state.c_str(), dtime_s);
	}
}

ServerActiveObject* LuaEntitySAO::create(ServerEnvironment *env, v3f pos,
		const std::string &data)
{
	std::string name;
	std::string state;
	s16 hp = 1;
	v3f velocity;
	float yaw = 0;
	if(data != ""){
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
			yaw = readF1000(is);
		}
	}
	// create object
	infostream<<"LuaEntitySAO::create(name=\""<<name<<"\" state=\""
			<<state<<"\")"<<std::endl;
	LuaEntitySAO *sao = new LuaEntitySAO(env, pos, name, state);
	sao->m_hp = hp;
	sao->m_velocity = velocity;
	sao->m_yaw = yaw;
	return sao;
}

bool LuaEntitySAO::isAttached()
{
	if(!m_attachment_parent_id)
		return false;
	// Check if the parent still exists
	ServerActiveObject *obj = m_env->getActiveObject(m_attachment_parent_id);
	if(obj)
		return true;
	return false;
}

void LuaEntitySAO::step(float dtime, bool send_recommended)
{
	if(!m_properties_sent)
	{
		m_properties_sent = true;
		std::string str = getPropertyPacket();
		// create message and add to list
		ActiveObjectMessage aom(getId(), true, str);
		m_messages_out.push_back(aom);
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
			core::aabbox3d<f32> box = m_prop.collisionbox;
			box.MinEdge *= BS;
			box.MaxEdge *= BS;
			collisionMoveResult moveresult;
			f32 pos_max_d = BS*0.25; // Distance per iteration
			v3f p_pos = m_base_position;
			v3f p_velocity = m_velocity;
			v3f p_acceleration = m_acceleration;
			moveresult = collisionMoveSimple(m_env,m_env->getGameDef(),
					pos_max_d, box, m_prop.stepheight, dtime,
					p_pos, p_velocity, p_acceleration,
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

		if((m_prop.automatic_face_movement_dir) &&
				(fabs(m_velocity.Z) > 0.001 || fabs(m_velocity.X) > 0.001)){
			m_yaw = atan2(m_velocity.Z,m_velocity.X) * 180 / M_PI + m_prop.automatic_face_movement_dir_offset;
		}
	}

	if(m_registered){
		m_env->getScriptIface()->luaentity_Step(m_id, dtime);
	}

	if(send_recommended == false)
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
		if(move_d > minchange || vel_d > minchange ||
				fabs(m_yaw - m_last_sent_yaw) > 1.0){
			sendPosition(true, false);
		}
	}

	if(m_armor_groups_sent == false){
		m_armor_groups_sent = true;
		std::string str = gob_cmd_update_armor_groups(
				m_armor_groups);
		// create message and add to list
		ActiveObjectMessage aom(getId(), true, str);
		m_messages_out.push_back(aom);
	}

	if(m_animation_sent == false){
		m_animation_sent = true;
		std::string str = gob_cmd_update_animation(m_animation_range, m_animation_speed, m_animation_blend);
		// create message and add to list
		ActiveObjectMessage aom(getId(), true, str);
		m_messages_out.push_back(aom);
	}

	if(m_bone_position_sent == false){
		m_bone_position_sent = true;
		for(std::map<std::string, core::vector2d<v3f> >::const_iterator ii = m_bone_position.begin(); ii != m_bone_position.end(); ++ii){
			std::string str = gob_cmd_update_bone_position((*ii).first, (*ii).second.X, (*ii).second.Y);
			// create message and add to list
			ActiveObjectMessage aom(getId(), true, str);
			m_messages_out.push_back(aom);
		}
	}

	if(m_attachment_sent == false){
		m_attachment_sent = true;
		std::string str = gob_cmd_update_attachment(m_attachment_parent_id, m_attachment_bone, m_attachment_position, m_attachment_rotation);
		// create message and add to list
		ActiveObjectMessage aom(getId(), true, str);
		m_messages_out.push_back(aom);
	}
}

std::string LuaEntitySAO::getClientInitializationData(u16 protocol_version)
{
	std::ostringstream os(std::ios::binary);

	if(protocol_version >= 14)
	{
		writeU8(os, 1); // version
		os<<serializeString(""); // name
		writeU8(os, 0); // is_player
		writeS16(os, getId()); //id
		writeV3F1000(os, m_base_position);
		writeF1000(os, m_yaw);
		writeS16(os, m_hp);

		writeU8(os, 4 + m_bone_position.size()); // number of messages stuffed in here
		os<<serializeLongString(getPropertyPacket()); // message 1
		os<<serializeLongString(gob_cmd_update_armor_groups(m_armor_groups)); // 2
		os<<serializeLongString(gob_cmd_update_animation(m_animation_range, m_animation_speed, m_animation_blend)); // 3
		for(std::map<std::string, core::vector2d<v3f> >::const_iterator ii = m_bone_position.begin(); ii != m_bone_position.end(); ++ii){
			os<<serializeLongString(gob_cmd_update_bone_position((*ii).first, (*ii).second.X, (*ii).second.Y)); // m_bone_position.size
		}
		os<<serializeLongString(gob_cmd_update_attachment(m_attachment_parent_id, m_attachment_bone, m_attachment_position, m_attachment_rotation)); // 4
	}
	else
	{
		writeU8(os, 0); // version
		os<<serializeString(""); // name
		writeU8(os, 0); // is_player
		writeV3F1000(os, m_base_position);
		writeF1000(os, m_yaw);
		writeS16(os, m_hp);
		writeU8(os, 2); // number of messages stuffed in here
		os<<serializeLongString(getPropertyPacket()); // message 1
		os<<serializeLongString(gob_cmd_update_armor_groups(m_armor_groups)); // 2
	}

	// return result
	return os.str();
}

std::string LuaEntitySAO::getStaticData()
{
	verbosestream<<__FUNCTION_NAME<<std::endl;
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
	// yaw
	writeF1000(os, m_yaw);
	return os.str();
}

int LuaEntitySAO::punch(v3f dir,
		const ToolCapabilities *toolcap,
		ServerActiveObject *puncher,
		float time_from_last_punch)
{
	if(!m_registered){
		// Delete unknown LuaEntities when punched
		m_removed = true;
		return 0;
	}

	// It's best that attachments cannot be punched 
	if(isAttached())
		return 0;
	
	ItemStack *punchitem = NULL;
	ItemStack punchitem_static;
	if(puncher){
		punchitem_static = puncher->getWieldedItem();
		punchitem = &punchitem_static;
	}

	PunchDamageResult result = getPunchDamage(
			m_armor_groups,
			toolcap,
			punchitem,
			time_from_last_punch);
	
	if(result.did_punch)
	{
		setHP(getHP() - result.damage);
		

		std::string punchername = "nil";

		if ( puncher != 0 )
			punchername = puncher->getDescription();

		actionstream<<getDescription()<<" punched by "
				<<punchername<<", damage "<<result.damage
				<<" hp, health now "<<getHP()<<" hp"<<std::endl;
		
		{
			std::string str = gob_cmd_punched(result.damage, getHP());
			// create message and add to list
			ActiveObjectMessage aom(getId(), true, str);
			m_messages_out.push_back(aom);
		}

		if(getHP() == 0)
			m_removed = true;
	}

	m_env->getScriptIface()->luaentity_Punch(m_id, puncher,
			time_from_last_punch, toolcap, dir);

	return result.wear;
}

void LuaEntitySAO::rightClick(ServerActiveObject *clicker)
{
	if(!m_registered)
		return;
	// It's best that attachments cannot be clicked
	if(isAttached())
		return;
	m_env->getScriptIface()->luaentity_Rightclick(m_id, clicker);
}

void LuaEntitySAO::setPos(v3f pos)
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

void LuaEntitySAO::setHP(s16 hp)
{
	if(hp < 0) hp = 0;
	m_hp = hp;
}

s16 LuaEntitySAO::getHP() const
{
	return m_hp;
}

void LuaEntitySAO::setArmorGroups(const ItemGroupList &armor_groups)
{
	m_armor_groups = armor_groups;
	m_armor_groups_sent = false;
}

void LuaEntitySAO::setAnimation(v2f frame_range, float frame_speed, float frame_blend)
{
	m_animation_range = frame_range;
	m_animation_speed = frame_speed;
	m_animation_blend = frame_blend;
	m_animation_sent = false;
}

void LuaEntitySAO::setBonePosition(std::string bone, v3f position, v3f rotation)
{
	m_bone_position[bone] = core::vector2d<v3f>(position, rotation);
	m_bone_position_sent = false;
}

void LuaEntitySAO::setAttachment(int parent_id, std::string bone, v3f position, v3f rotation)
{
	// Attachments need to be handled on both the server and client.
	// If we just attach on the server, we can only copy the position of the parent. Attachments
	// are still sent to clients at an interval so players might see them lagging, plus we can't
	// read and attach to skeletal bones.
	// If we just attach on the client, the server still sees the child at its original location.
	// This breaks some things so we also give the server the most accurate representation
	// even if players only see the client changes.

	m_attachment_parent_id = parent_id;
	m_attachment_bone = bone;
	m_attachment_position = position;
	m_attachment_rotation = rotation;
	m_attachment_sent = false;
}

ObjectProperties* LuaEntitySAO::accessObjectProperties()
{
	return &m_prop;
}

void LuaEntitySAO::notifyObjectPropertiesModified()
{
	m_properties_sent = false;
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
	std::string str = gob_cmd_set_texture_mod(mod);
	// create message and add to list
	ActiveObjectMessage aom(getId(), true, str);
	m_messages_out.push_back(aom);
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
	m_messages_out.push_back(aom);
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
	m_last_sent_yaw = m_yaw;
	m_last_sent_position = m_base_position;
	m_last_sent_velocity = m_velocity;
	//m_last_sent_acceleration = m_acceleration;

	float update_interval = m_env->getSendRecommendedInterval();

	std::string str = gob_cmd_update_position(
		m_base_position,
		m_velocity,
		m_acceleration,
		m_yaw,
		do_interpolate,
		is_movement_end,
		update_interval
	);
	// create message and add to list
	ActiveObjectMessage aom(getId(), false, str);
	m_messages_out.push_back(aom);
}

bool LuaEntitySAO::getCollisionBox(aabb3f *toset) {
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

bool LuaEntitySAO::collideWithObjects(){
	return m_prop.collideWithObjects;
}

/*
	PlayerSAO
*/

// No prototype, PlayerSAO does not need to be deserialized

PlayerSAO::PlayerSAO(ServerEnvironment *env_, Player *player_, u16 peer_id_,
		const std::set<std::string> &privs, bool is_singleplayer):
	ServerActiveObject(env_, v3f(0,0,0)),
	m_player(player_),
	m_peer_id(peer_id_),
	m_inventory(NULL),
	m_damage(0),
	m_last_good_position(0,0,0),
	m_time_from_last_punch(0),
	m_nocheat_dig_pos(32767, 32767, 32767),
	m_nocheat_dig_time(0),
	m_wield_index(0),
	m_position_not_sent(false),
	m_armor_groups_sent(false),
	m_properties_sent(true),
	m_privs(privs),
	m_is_singleplayer(is_singleplayer),
	m_animation_speed(0),
	m_animation_blend(0),
	m_animation_sent(false),
	m_bone_position_sent(false),
	m_attachment_parent_id(0),
	m_attachment_sent(false),
	// public
	m_moved(false),
	m_inventory_not_sent(false),
	m_hp_not_sent(false),
	m_breath_not_sent(false),
	m_wielded_item_not_sent(false),
	m_physics_override_speed(1),
	m_physics_override_jump(1),
	m_physics_override_gravity(1),
	m_physics_override_sneak(true),
	m_physics_override_sneak_glitch(true),
	m_physics_override_sent(false)
{
	assert(m_player);
	assert(m_peer_id != 0);
	setBasePosition(m_player->getPosition());
	m_inventory = &m_player->inventory;
	m_armor_groups["fleshy"] = 100;

	m_prop.hp_max = PLAYER_MAX_HP;
	m_prop.physical = false;
	m_prop.weight = 75;
	m_prop.collisionbox = core::aabbox3d<f32>(-1/3.,-1.0,-1/3., 1/3.,1.0,1/3.);
	// start of default appearance, this should be overwritten by LUA
	m_prop.visual = "upright_sprite";
	m_prop.visual_size = v2f(1, 2);
	m_prop.textures.clear();
	m_prop.textures.push_back("player.png");
	m_prop.textures.push_back("player_back.png");
	m_prop.colors.clear();
	m_prop.colors.push_back(video::SColor(255, 255, 255, 255));
	m_prop.spritediv = v2s16(1,1);
	// end of default appearance
	m_prop.is_visible = true;
	m_prop.makes_footstep_sound = true;
}

PlayerSAO::~PlayerSAO()
{
	if(m_inventory != &m_player->inventory)
		delete m_inventory;

}

std::string PlayerSAO::getDescription()
{
	return std::string("player ") + m_player->getName();
}

// Called after id has been set and has been inserted in environment
void PlayerSAO::addedToEnvironment(u32 dtime_s)
{
	ServerActiveObject::addedToEnvironment(dtime_s);
	ServerActiveObject::setBasePosition(m_player->getPosition());
	m_player->setPlayerSAO(this);
	m_player->peer_id = m_peer_id;
	m_last_good_position = m_player->getPosition();
}

// Called before removing from environment
void PlayerSAO::removingFromEnvironment()
{
	ServerActiveObject::removingFromEnvironment();
	if(m_player->getPlayerSAO() == this)
	{
		m_player->setPlayerSAO(NULL);
		m_player->peer_id = 0;
		m_env->savePlayer(m_player->getName());
		m_env->removePlayer(m_player->getName());
	}
}

bool PlayerSAO::isStaticAllowed() const
{
	return false;
}

bool PlayerSAO::unlimitedTransferDistance() const
{
	return g_settings->getBool("unlimited_player_transfer_distance");
}

std::string PlayerSAO::getClientInitializationData(u16 protocol_version)
{
	std::ostringstream os(std::ios::binary);

	if(protocol_version >= 15)
	{
		writeU8(os, 1); // version
		os<<serializeString(m_player->getName()); // name
		writeU8(os, 1); // is_player
		writeS16(os, getId()); //id
		writeV3F1000(os, m_player->getPosition() + v3f(0,BS*1,0));
		writeF1000(os, m_player->getYaw());
		writeS16(os, getHP());

		writeU8(os, 5 + m_bone_position.size()); // number of messages stuffed in here
		os<<serializeLongString(getPropertyPacket()); // message 1
		os<<serializeLongString(gob_cmd_update_armor_groups(m_armor_groups)); // 2
		os<<serializeLongString(gob_cmd_update_animation(m_animation_range, m_animation_speed, m_animation_blend)); // 3
		for(std::map<std::string, core::vector2d<v3f> >::const_iterator ii = m_bone_position.begin(); ii != m_bone_position.end(); ++ii){
			os<<serializeLongString(gob_cmd_update_bone_position((*ii).first, (*ii).second.X, (*ii).second.Y)); // m_bone_position.size
		}
		os<<serializeLongString(gob_cmd_update_attachment(m_attachment_parent_id, m_attachment_bone, m_attachment_position, m_attachment_rotation)); // 4
		os<<serializeLongString(gob_cmd_update_physics_override(m_physics_override_speed,
				m_physics_override_jump, m_physics_override_gravity, m_physics_override_sneak,
				m_physics_override_sneak_glitch)); // 5
	}
	else
	{
		writeU8(os, 0); // version
		os<<serializeString(m_player->getName()); // name
		writeU8(os, 1); // is_player
		writeV3F1000(os, m_player->getPosition() + v3f(0,BS*1,0));
		writeF1000(os, m_player->getYaw());
		writeS16(os, getHP());
		writeU8(os, 2); // number of messages stuffed in here
		os<<serializeLongString(getPropertyPacket()); // message 1
		os<<serializeLongString(gob_cmd_update_armor_groups(m_armor_groups)); // 2
	}

	// return result
	return os.str();
}

std::string PlayerSAO::getStaticData()
{
	assert(0);
	return "";
}

bool PlayerSAO::isAttached()
{
	if(!m_attachment_parent_id)
		return false;
	// Check if the parent still exists
	ServerActiveObject *obj = m_env->getActiveObject(m_attachment_parent_id);
	if(obj)
		return true;
	return false;
}

void PlayerSAO::step(float dtime, bool send_recommended)
{
	if(!m_properties_sent)
	{
		m_properties_sent = true;
		std::string str = getPropertyPacket();
		// create message and add to list
		ActiveObjectMessage aom(getId(), true, str);
		m_messages_out.push_back(aom);
	}

	// If attached, check that our parent is still there. If it isn't, detach.
	if(m_attachment_parent_id && !isAttached())
	{
		m_attachment_parent_id = 0;
		m_attachment_bone = "";
		m_attachment_position = v3f(0,0,0);
		m_attachment_rotation = v3f(0,0,0);
		m_player->setPosition(m_last_good_position);
		m_moved = true;
	}

	//dstream<<"PlayerSAO::step: dtime: "<<dtime<<std::endl;

	// Set lag pool maximums based on estimated lag
	const float LAG_POOL_MIN = 5.0;
	float lag_pool_max = m_env->getMaxLagEstimate() * 2.0;
	if(lag_pool_max < LAG_POOL_MIN)
		lag_pool_max = LAG_POOL_MIN;
	m_dig_pool.setMax(lag_pool_max);
	m_move_pool.setMax(lag_pool_max);

	// Increment cheat prevention timers
	m_dig_pool.add(dtime);
	m_move_pool.add(dtime);
	m_time_from_last_punch += dtime;
	m_nocheat_dig_time += dtime;

	// Each frame, parent position is copied if the object is attached, otherwise it's calculated normally
	// If the object gets detached this comes into effect automatically from the last known origin
	if(isAttached())
	{
		v3f pos = m_env->getActiveObject(m_attachment_parent_id)->getBasePosition();
		m_last_good_position = pos;
		m_player->setPosition(pos);
	}

	if(send_recommended == false)
		return;

	// If the object is attached client-side, don't waste bandwidth sending its position to clients
	if(m_position_not_sent && !isAttached())
	{
		m_position_not_sent = false;
		float update_interval = m_env->getSendRecommendedInterval();
		v3f pos;
		if(isAttached()) // Just in case we ever do send attachment position too
			pos = m_env->getActiveObject(m_attachment_parent_id)->getBasePosition();
		else
			pos = m_player->getPosition() + v3f(0,BS*1,0);
		std::string str = gob_cmd_update_position(
			pos,
			v3f(0,0,0),
			v3f(0,0,0),
			m_player->getYaw(),
			true,
			false,
			update_interval
		);
		// create message and add to list
		ActiveObjectMessage aom(getId(), false, str);
		m_messages_out.push_back(aom);
	}

	if(m_wielded_item_not_sent)
	{
		m_wielded_item_not_sent = false;
		// GenericCAO has no special way to show this
	}

	if(m_armor_groups_sent == false){
		m_armor_groups_sent = true;
		std::string str = gob_cmd_update_armor_groups(
				m_armor_groups);
		// create message and add to list
		ActiveObjectMessage aom(getId(), true, str);
		m_messages_out.push_back(aom);
	}

	if(m_physics_override_sent == false){
		m_physics_override_sent = true;
		std::string str = gob_cmd_update_physics_override(m_physics_override_speed,
				m_physics_override_jump, m_physics_override_gravity,
				m_physics_override_sneak, m_physics_override_sneak_glitch);
		// create message and add to list
		ActiveObjectMessage aom(getId(), true, str);
		m_messages_out.push_back(aom);
	}

	if(m_animation_sent == false){
		m_animation_sent = true;
		std::string str = gob_cmd_update_animation(m_animation_range, m_animation_speed, m_animation_blend);
		// create message and add to list
		ActiveObjectMessage aom(getId(), true, str);
		m_messages_out.push_back(aom);
	}

	if(m_bone_position_sent == false){
		m_bone_position_sent = true;
		for(std::map<std::string, core::vector2d<v3f> >::const_iterator ii = m_bone_position.begin(); ii != m_bone_position.end(); ++ii){
			std::string str = gob_cmd_update_bone_position((*ii).first, (*ii).second.X, (*ii).second.Y);
			// create message and add to list
			ActiveObjectMessage aom(getId(), true, str);
			m_messages_out.push_back(aom);
		}
	}

	if(m_attachment_sent == false){
		m_attachment_sent = true;
		std::string str = gob_cmd_update_attachment(m_attachment_parent_id, m_attachment_bone, m_attachment_position, m_attachment_rotation);
		// create message and add to list
		ActiveObjectMessage aom(getId(), true, str);
		m_messages_out.push_back(aom);
	}
}

void PlayerSAO::setBasePosition(const v3f &position)
{
	// This needs to be ran for attachments too
	ServerActiveObject::setBasePosition(position);
	m_position_not_sent = true;
}

void PlayerSAO::setPos(v3f pos)
{
	if(isAttached())
		return;
	m_player->setPosition(pos);
	// Movement caused by this command is always valid
	m_last_good_position = pos;
	// Force position change on client
	m_moved = true;
}

void PlayerSAO::moveTo(v3f pos, bool continuous)
{
	if(isAttached())
		return;
	m_player->setPosition(pos);
	// Movement caused by this command is always valid
	m_last_good_position = pos;
	// Force position change on client
	m_moved = true;
}

void PlayerSAO::setYaw(float yaw)
{
	m_player->setYaw(yaw);
	// Force change on client
	m_moved = true;
}

void PlayerSAO::setPitch(float pitch)
{
	m_player->setPitch(pitch);
	// Force change on client
	m_moved = true;
}

int PlayerSAO::punch(v3f dir,
	const ToolCapabilities *toolcap,
	ServerActiveObject *puncher,
	float time_from_last_punch)
{
	// It's best that attachments cannot be punched 
	if(isAttached())
		return 0;

	if(!toolcap)
		return 0;

	// No effect if PvP disabled
	if(g_settings->getBool("enable_pvp") == false){
		if(puncher->getType() == ACTIVEOBJECT_TYPE_PLAYER){
			std::string str = gob_cmd_punched(0, getHP());
			// create message and add to list
			ActiveObjectMessage aom(getId(), true, str);
			m_messages_out.push_back(aom);
			return 0;
		}
	}

	HitParams hitparams = getHitParams(m_armor_groups, toolcap,
			time_from_last_punch);

	std::string punchername = "nil";

	if ( puncher != 0 )
		punchername = puncher->getDescription();

	actionstream<<"Player "<<m_player->getName()<<" punched by "
			<<punchername<<", damage "<<hitparams.hp
			<<" HP"<<std::endl;

	setHP(getHP() - hitparams.hp);

	return hitparams.wear;
}

void PlayerSAO::rightClick(ServerActiveObject *clicker)
{
}

s16 PlayerSAO::getHP() const
{
	return m_player->hp;
}

s16 PlayerSAO::readDamage()
{
	s16 damage = m_damage;
	m_damage = 0;
	return damage;
}

void PlayerSAO::setHP(s16 hp)
{
	s16 oldhp = m_player->hp;

	if(hp < 0)
		hp = 0;
	else if(hp > PLAYER_MAX_HP)
		hp = PLAYER_MAX_HP;

	if(hp < oldhp && g_settings->getBool("enable_damage") == false)
	{
		m_hp_not_sent = true; // fix wrong prediction on client
		return;
	}

	m_player->hp = hp;

	if(hp != oldhp) {
		m_hp_not_sent = true;
		if(oldhp > hp)
			m_damage += oldhp - hp;
	}

	// Update properties on death
	if((hp == 0) != (oldhp == 0))
		m_properties_sent = false;
}

u16 PlayerSAO::getBreath() const
{
	return m_player->getBreath();
}

void PlayerSAO::setBreath(u16 breath)
{
	m_player->setBreath(breath);
}

void PlayerSAO::setArmorGroups(const ItemGroupList &armor_groups)
{
	m_armor_groups = armor_groups;
	m_armor_groups_sent = false;
}

void PlayerSAO::setAnimation(v2f frame_range, float frame_speed, float frame_blend)
{
	// store these so they can be updated to clients
	m_animation_range = frame_range;
	m_animation_speed = frame_speed;
	m_animation_blend = frame_blend;
	m_animation_sent = false;
}

void PlayerSAO::setBonePosition(std::string bone, v3f position, v3f rotation)
{
	// store these so they can be updated to clients
	m_bone_position[bone] = core::vector2d<v3f>(position, rotation);
	m_bone_position_sent = false;
}

void PlayerSAO::setAttachment(int parent_id, std::string bone, v3f position, v3f rotation)
{
	// Attachments need to be handled on both the server and client.
	// If we just attach on the server, we can only copy the position of the parent. Attachments
	// are still sent to clients at an interval so players might see them lagging, plus we can't
	// read and attach to skeletal bones.
	// If we just attach on the client, the server still sees the child at its original location.
	// This breaks some things so we also give the server the most accurate representation
	// even if players only see the client changes.

	m_attachment_parent_id = parent_id;
	m_attachment_bone = bone;
	m_attachment_position = position;
	m_attachment_rotation = rotation;
	m_attachment_sent = false;
}

ObjectProperties* PlayerSAO::accessObjectProperties()
{
	return &m_prop;
}

void PlayerSAO::notifyObjectPropertiesModified()
{
	m_properties_sent = false;
}

Inventory* PlayerSAO::getInventory()
{
	return m_inventory;
}
const Inventory* PlayerSAO::getInventory() const
{
	return m_inventory;
}

InventoryLocation PlayerSAO::getInventoryLocation() const
{
	InventoryLocation loc;
	loc.setPlayer(m_player->getName());
	return loc;
}

void PlayerSAO::setInventoryModified()
{
	m_inventory_not_sent = true;
}

std::string PlayerSAO::getWieldList() const
{
	return "main";
}

int PlayerSAO::getWieldIndex() const
{
	return m_wield_index;
}

void PlayerSAO::setWieldIndex(int i)
{
	if(i != m_wield_index)
	{
		m_wield_index = i;
		m_wielded_item_not_sent = true;
	}
}

void PlayerSAO::disconnected()
{
	m_peer_id = 0;
	m_removed = true;
	if(m_player->getPlayerSAO() == this)
	{
		m_player->setPlayerSAO(NULL);
		m_player->peer_id = 0;
	}
}

std::string PlayerSAO::getPropertyPacket()
{
	m_prop.is_visible = (true);
	return gob_cmd_set_properties(m_prop);
}

bool PlayerSAO::checkMovementCheat()
{
	bool cheated = false;
	if(isAttached() || m_is_singleplayer ||
			g_settings->getBool("disable_anticheat"))
	{
		m_last_good_position = m_player->getPosition();
	}
	else
	{
		/*
			Check player movements

			NOTE: Actually the server should handle player physics like the
			client does and compare player's position to what is calculated
			on our side. This is required when eg. players fly due to an
			explosion. Altough a node-based alternative might be possible
			too, and much more lightweight.
		*/

		float player_max_speed = 0;
		float player_max_speed_up = 0;
		if(m_privs.count("fast") != 0){
			// Fast speed
			player_max_speed = m_player->movement_speed_fast;
			player_max_speed_up = m_player->movement_speed_fast;
		} else {
			// Normal speed
			player_max_speed = m_player->movement_speed_walk;
			player_max_speed_up = m_player->movement_speed_walk;
		}
		// Tolerance. With the lag pool we shouldn't need it.
		//player_max_speed *= 2.5;
		//player_max_speed_up *= 2.5;

		v3f diff = (m_player->getPosition() - m_last_good_position);
		float d_vert = diff.Y;
		diff.Y = 0;
		float d_horiz = diff.getLength();
		float required_time = d_horiz/player_max_speed;
		if(d_vert > 0 && d_vert/player_max_speed > required_time)
			required_time = d_vert/player_max_speed;
		if(m_move_pool.grab(required_time)){
			m_last_good_position = m_player->getPosition();
		} else {
			actionstream<<"Player "<<m_player->getName()
					<<" moved too fast; resetting position"
					<<std::endl;
			m_player->setPosition(m_last_good_position);
			m_moved = true;
			cheated = true;
		}
	}
	return cheated;
}

bool PlayerSAO::getCollisionBox(aabb3f *toset) {
	//update collision box
	*toset = m_player->getCollisionbox();

	toset->MinEdge += m_base_position;
	toset->MaxEdge += m_base_position;

	return true;
}

bool PlayerSAO::collideWithObjects(){
	return true;
}

