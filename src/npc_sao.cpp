#include "npc_sao.h"
#include "util/serialize.h"
#include "environment.h"
#include "server.h"
#include "scripting_server.h"

// Prototype (registers item for deserialization)
NpcSAO proto_NpcEntitySAO(NULL, v3f(0,0,0), "_prototype", "");

NpcSAO::NpcSAO(ServerEnvironment *env, v3f pos,
	const std::string &name, const std::string &state):
		LuaEntitySAO(env, pos, name, state)
{
	if(env == NULL){
		ServerActiveObject::registerType(getType(), create);
		return;
	}
}

NpcSAO::~NpcSAO()
{

}

ServerActiveObject* NpcSAO::create(ServerEnvironment *env, v3f pos,
	const std::string &data)
{
	std::string name;
	std::string state;
	s16 hp = 1;
	v3f velocity;
	float yaw = 0;
	if (!data.empty()) {
		std::istringstream is(data, std::ios::binary);
		// read version
		u8 version = readU8(is);
		// check if version is supported
		if (version == 1) {
			name = deSerializeString(is);
			state = deSerializeLongString(is);
			hp = readS16(is);
			velocity = readV3F1000(is);
			yaw = readF1000(is);
		}
	}
	// create object
	infostream << "NpcSAO::create(name=\"" << name << "\" state=\""
			   << state << "\")" << std::endl;
	NpcSAO *sao = new NpcSAO(env, pos, name, state);
	sao->m_hp = hp;
	sao->m_velocity = velocity;
	sao->m_yaw = yaw;
	return sao;
}

void NpcSAO::addedToEnvironment(u32 dtime_s)
{
	ServerActiveObject::addedToEnvironment(dtime_s);

	// Create entity from name
	m_registered = m_env->getScriptIface()->luanpc_Add(m_id, m_init_name.c_str());

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