#include "lua_api/l_node_modifier.h"
#include "lua_api/l_internal.h"
#include "common/c_content.h"
#include "common/c_converter.h"
#include "node_modifier.h"
#include "server.h"
#include "lua_api/l_item.h"
#include "serverenvironment.h"
#include "map.h"
#include "util/string.h"

static void read_node_modifier(lua_State *L, NodeModifier &nmod, int index)
{
	if (index < 0)
		index = lua_gettop(L) + 1 + index;

	bool is_present = getintfield(L, index, "light_source", nmod.m_light_source);
	if (is_present) {
		nmod.m_modified_props_mask |= MP_LightSource;
		if (nmod.m_light_source > LIGHT_MAX) {
			warningstream << "Node " << nmod.m_name.c_str()
				<< " had greater light_source than " << LIGHT_MAX
				<< ", it was reduced." << std::endl;
			nmod.m_light_source = LIGHT_MAX;
		}
	}
}

int ModApiNodeModifier::l_register_node_modifier_raw(lua_State *L)
{
	NO_MAP_LOCK_REQUIRED;
	luaL_checktype(L, 1, LUA_TTABLE);
	int table = 1;

	auto *nmod = getServer(L)->getWritableNodeModifierManager();

	std::string name;
	lua_getfield(L, table, "name");
	luaL_checktype(L, -1, LUA_TSTRING);
	name = readParam<std::string>(L, -1);

	if (!isValidModScopedName(name))
		throw LuaError("Node modifier name must have the format modname:name "
				"with letters, digits and underscores (lowercase modname)");

	NodeModifier mod(name);
	read_node_modifier(L, mod, table);
	u16 inserted_id = nmod->add(std::move(mod));
	if (inserted_id == MODIFIER_IGNORE || inserted_id >= MAX_REGISTERED_NODE_MODIFIERS) {
		throw LuaError("Number of registerable node modifiers (about "
				+ itos(MAX_REGISTERED_NODE_MODIFIERS) + ") exceeded: " + name);
	}

	return 0;
}

void ModApiNodeModifier::Initialize(lua_State *L, int top)
{
	API_FCT(register_node_modifier_raw);
}

std::string NodeModifiersRef::readName(lua_State *L, int index)
{
	if (lua_type(L, index) != LUA_TSTRING)
		throw LuaError("Modifier name must be a string");
	std::string name = readParam<std::string>(L, index);
	if (!isValidModScopedName(name))
		throw LuaError("Invalid modifier name: " + name);
	return name;
}

void NodeModifiersRef::readList(lua_State *L, int table, std::vector<std::string> &modifiers)
{
	assert(modifiers.empty());
	lua_pushvalue(L, table);
	if (!lua_istable(L, -1))
		throw LuaError("modifiers must be an array of valid modifier names");

	const size_t count = lua_objlen(L, -1);
	modifiers.reserve(count);
	for (size_t i = 1; i <= count; ++i) {
		lua_rawgeti(L, -1, i);
		modifiers.push_back(readName(L, -1));
		lua_pop(L, 1);
	}
	lua_pop(L, 1);
}

bool NodeModifiersRef::set(std::vector<std::string> names)
{
	if (m_stack)
		return m_stack->getItem().metadata.setModifiers(std::move(names));
	Map &map = m_env->getMap();
	MapBlock *block = map.getBlockNoCreateNoEx(getNodeBlockPos(m_pos));
	if (!block)
		return false;
	IGameDef *gamedef = m_env->getGameDef();
	const NodeModifierManager *manager = gamedef->getNodeModifierManager();
	std::vector<u16> ids;
	ids.reserve(names.size());
	for (const std::string &name : names) {
		u16 id = manager->getId(name);
		if (id == MODIFIER_IGNORE)
			id = gamedef->allocateUnknownNodeModifierId(name);
		if (id == MODIFIER_IGNORE)
			return false;
		ids.push_back(id);
	}
	return map.setNodeModifiers(m_pos, ids);
}


NodeModifiersRef::NodeModifiersRef(LuaItemStack *stack) : m_stack(stack)
{
	m_stack->grab();
}

NodeModifiersRef::~NodeModifiersRef()
{
	if (m_stack)
		m_stack->drop();
}

int NodeModifiersRef::l_add(lua_State *L)
{
	MAP_LOCK_REQUIRED;
	auto *ref = checkObject<NodeModifiersRef>(L, 1);
	const auto name = readName(L, 2);
	const auto *manager = getGameDef(L)->getNodeModifierManager();
	u16 id = manager->getId(name);
	bool result = false;
	if (id != MODIFIER_IGNORE && !manager->get(id).m_is_dummy) {
		bool prune = lua_toboolean(L, 3);
		result = ref->m_stack ?
				ref->m_stack->getItem().metadata.addModifier(name, prune ? manager : nullptr) :
				ref->m_env->getMap().addNodeModifier(ref->m_pos, id, prune);
	}
	lua_pushboolean(L, result);
	return 1;
}

int NodeModifiersRef::l_remove(lua_State *L)
{
	MAP_LOCK_REQUIRED;
	auto *ref = checkObject<NodeModifiersRef>(L, 1);
	const auto name = readName(L, 2);
	if (ref->m_stack) {
		ref->m_stack->getItem().metadata.removeModifier(name);
	} else {
		u16 id = ref->m_env->getGameDef()->getNodeModifierManager()->getId(name);
		if (id != MODIFIER_IGNORE)
			ref->m_env->getMap().removeNodeModifier(ref->m_pos, id);
	}
	return 0;
}

int NodeModifiersRef::l_set_list(lua_State *L)
{
	MAP_LOCK_REQUIRED;
	auto *ref = checkObject<NodeModifiersRef>(L, 1);
	std::vector<std::string> names;
	readList(L, 2, names);
	lua_pushboolean(L, ref->set(std::move(names)));
	return 1;
}

void NodeModifiersRef::pushList(lua_State *L) const
{
	lua_newtable(L);
	int i = 0;
	auto append = [&](const std::string &name) {
		lua_pushlstring(L, name.data(), name.size());
		lua_rawseti(L, -2, ++i);
	};
	if (m_stack) {
		for (const auto &name : m_stack->getItem().metadata.getModifiersList())
			append(name);
	} else {
		const auto *manager = m_env->getGameDef()->getNodeModifierManager();
		auto [first, last] = m_env->getMap().getNodeModifiers(m_pos);
		for (auto it = first; it != last; ++it)
			if (it->id < manager->size())
				append(manager->get(it->id).m_name);
	}
}

int NodeModifiersRef::l_get_list(lua_State *L)
{
	MAP_LOCK_REQUIRED;
	checkObject<NodeModifiersRef>(L, 1)->pushList(L);
	return 1;
}

int NodeModifiersRef::gc_object(lua_State *L)
{
	delete takeObjectForGC<NodeModifiersRef>(L);
	return 0;
}

const char NodeModifiersRef::className[] = "NodeModifiersRef";

void NodeModifiersRef::create(lua_State *L, NodeModifiersRef *ref)
{
	luaL_getmetatable(L, className);
	bool registered = !lua_isnil(L, -1);
	lua_pop(L, 1);
	if (!registered) {
		static const luaL_Reg methods[] = {
			luamethod(NodeModifiersRef, add),
			luamethod(NodeModifiersRef, remove),
			luamethod(NodeModifiersRef, set_list),
			luamethod(NodeModifiersRef, get_list),
			{nullptr, nullptr}
		};
		static const luaL_Reg metamethods[] = {
			{"__gc", gc_object}, {nullptr, nullptr}
		};
		registerClass<NodeModifiersRef>(L, methods, metamethods);
	}
	*(void **)lua_newuserdata(L, sizeof(void *)) = ref;
	luaL_getmetatable(L, className);
	lua_setmetatable(L, -2);
}
