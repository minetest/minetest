#include "api.h"
#include "scripting_server.h"
#include "util/Optional.h"

namespace api
{
namespace server
{

ServerScripting *Router::getLuaAPI()
{
	return (ServerScripting *)m_lua_api;
}

template <> void Router::registerAPI(std::shared_ptr<Entity> api)
{
	m_entity_implementations.push_back(api);
}

template <> void Router::registerAPI(std::shared_ptr<Environment> api)
{
	m_env_implementations.push_back(api);
}

template <> void Router::registerAPI(std::shared_ptr<Global> api)
{
	m_global_implementations.push_back(api);
}

template <> void Router::registerAPI(std::shared_ptr<Inventory> api)
{
	m_inv_implementations.push_back(api);
}

template <> void Router::registerAPI(std::shared_ptr<Item> api)
{
	m_item_implementations.push_back(api);
}

template <> void Router::registerAPI(std::shared_ptr<ModChannels> api)
{
	m_modchannel_implementations.push_back(api);
}

template <> void Router::registerAPI(std::shared_ptr<Node> api)
{
	m_node_implementations.push_back(api);
}

template <> void Router::registerAPI(std::shared_ptr<NodeMeta> api)
{
	m_node_meta_implementations.push_back(api);
}

template <> void Router::registerAPI(std::shared_ptr<Player> api)
{
	m_player_implementations.push_back(api);
}
/*
 * Player routes
 */

void Router::on_newplayer(ServerActiveObject *player)
{
	for (const auto &impl : m_player_implementations)
		impl->on_newplayer(player);
}

void Router::on_dieplayer(ServerActiveObject *player, const PlayerHPChangeReason &reason)
{
	for (const auto &impl : m_player_implementations)
		impl->on_dieplayer(player, reason);
}

bool Router::on_respawnplayer(ServerActiveObject *player)
{
	for (const auto &impl : m_player_implementations) {
		if (impl->on_respawnplayer(player))
			return true;
	}

	return false;
}
bool Router::on_prejoinplayer(
		const std::string &name, const std::string &ip, std::string *reason)
{
	for (const auto &impl : m_player_implementations) {
		if (impl->on_prejoinplayer(name, ip, reason))
			return true;
	}

	return false;
}

bool Router::can_bypass_userlimit(const std::string &name, const std::string &ip)
{
	for (const auto &impl : m_player_implementations) {
		if (impl->can_bypass_userlimit(name, ip))
			return true;
	}

	return false;
}

void Router::on_joinplayer (
      ServerActiveObject *player,
      s64 last_login)
{
	for (const auto &impl : m_player_implementations)
		impl->on_joinplayer(player);
}

void Router::on_leaveplayer(ServerActiveObject *player, bool timeout)
{
	for (const auto &impl : m_player_implementations)
		impl->on_leaveplayer(player, timeout);
}

void Router::on_cheat(ServerActiveObject *player, const std::string &cheat_type)
{
	for (const auto &impl : m_player_implementations)
		impl->on_cheat(player, cheat_type);
}

bool Router::on_punchplayer(ServerActiveObject *player, ServerActiveObject *hitter,
		float time_from_last_punch, const ToolCapabilities *toolcap, v3f dir,
		s16 damage)
{
	for (const auto &impl : m_player_implementations) {
		if (impl->on_punchplayer(player, hitter, time_from_last_punch, toolcap,
				    dir, damage))
			return true;
	}

	return false;
}

s32 Router::on_player_hpchange(ServerActiveObject *player, s32 hp_change,
		const PlayerHPChangeReason &reason)
{
	for (const auto &impl : m_player_implementations)
		return impl->on_player_hpchange(player, hp_change, reason);

	return 0;
}

void Router::on_playerReceiveFields(ServerActiveObject *player,
		const std::string &formname, const StringMap &fields)
{
	for (const auto &impl : m_player_implementations)
		impl->on_playerReceiveFields(player, formname, fields);
}

void Router::on_authplayer(const std::string &name, const std::string &ip, bool is_success)
{
	for (const auto &impl : m_player_implementations)
      impl->on_authplayer(name, ip, is_success);
}

/*
 * Player inventory routes
 */

// Return number of accepted items to be moved
int Router::player_inventory_AllowMove(
		const MoveAction &ma, int count, ServerActiveObject *player)
{
	for (const auto &impl : m_player_implementations) {
		int new_count = impl->player_inventory_AllowMove(ma, count, player);
		if (new_count != 0)
			return new_count;
	}

	return 0;
}

// Return number of accepted items to be put
int Router::player_inventory_AllowPut(
		const MoveAction &ma, const ItemStack &stack, ServerActiveObject *player)
{
	for (const auto &impl : m_player_implementations) {
		int new_count = impl->player_inventory_AllowPut(ma, stack, player);
		if (new_count != 0)
			return new_count;
	}

	return 0;
}

// Return number of accepted items to be taken
int Router::player_inventory_AllowTake(
		const MoveAction &ma, const ItemStack &stack, ServerActiveObject *player)
{
	for (const auto &impl : m_player_implementations) {
		int new_count = impl->player_inventory_AllowTake(ma, stack, player);
		if (new_count != 0)
			return new_count;
	}

	return 0;
}

// Report moved items
void Router::player_inventory_OnMove(
		const MoveAction &ma, int count, ServerActiveObject *player)
{
	for (const auto &impl : m_player_implementations)
		impl->player_inventory_OnMove(ma, count, player);
}

// Report put items
void Router::player_inventory_OnPut(
		const MoveAction &ma, const ItemStack &stack, ServerActiveObject *player)
{
	for (const auto &impl : m_player_implementations)
		impl->player_inventory_OnPut(ma, stack, player);
}

// Report taken items
void Router::player_inventory_OnTake(
		const MoveAction &ma, const ItemStack &stack, ServerActiveObject *player)
{
	for (const auto &impl : m_player_implementations)
		impl->player_inventory_OnTake(ma, stack, player);
}

/*
 * Server routes
 */

bool Router::on_chat_message(const std::string &name, const std::string &message)
{
	for (const auto &impl : m_global_implementations) {
		if (impl->on_chat_message(name, message))
			return true;
	}

	return false;
}

void Router::on_shutdown()
{
	for (const auto &impl : m_global_implementations)
		impl->on_shutdown();
}

std::string Router::formatChatMessage(const std::string &name, const std::string &message)
{
	for (const auto &impl : m_global_implementations)
		return impl->formatChatMessage(name, message);

	return "";
}

bool Router::getAuth (
      const std::string &playername,
      std::string *dst_password,
      std::set<std::string> *dst_privs,
      s64 *dst_last_login)
{
   for (const std::shared_ptr<Global> &impl : m_global_implementations)
   {
      if (impl->getAuth(playername, dst_password, dst_privs, dst_last_login))
			return true;
	}

	return false;
}

void Router::createAuth(const std::string &playername, const std::string &password)
{
   for (const std::shared_ptr<Global> &impl : m_global_implementations) {
		impl->createAuth(playername, password);
	}
}

bool Router::setPassword(const std::string &playername, const std::string &password)
{
   for (const std::shared_ptr<Global> &impl : m_global_implementations) {
		if (impl->setPassword(playername, password))
			return true;
	}

	return false;
}

// u32 Router::allocateDynamicMediaCallback(lua_State *L, int f_idx) // TODO: test if needed
// {}

void Router::freeDynamicMediaCallback(u32 token)
{
   for (const std::shared_ptr<Global> &impl : m_global_implementations)
   {
      impl->freeDynamicMediaCallback(token);
   }
}

void Router::on_dynamic_media_added(u32 token, const char *playername) // TODO: test if needed
{
   for (const std::shared_ptr<Global> &impl : m_global_implementations)
   {
      impl->on_dynamic_media_added(token, playername);
   }
}

void Router::environment_Step(float dtime) // TODO: test if needed
{
	for (const auto &impl : m_env_implementations)
		impl->environment_Step(dtime);
}

void Router::environment_OnGenerated(v3s16 minp, v3s16 maxp, u32 blockseed)
{
	for (const auto &impl : m_env_implementations)
		impl->environment_OnGenerated(minp, maxp, blockseed);
}

void Router::player_event(ServerActiveObject *player, const std::string &type)
{
	for (const auto &impl : m_env_implementations)
		impl->player_event(player, type);
}

void Router::on_emerge_area_completion(v3s16 blockpos, int action, void *state)
{
	for (const auto &impl : m_env_implementations)
		impl->on_emerge_area_completion(blockpos, action, state);
}

bool Router::node_on_punch(v3s16 p, MapNode node, ServerActiveObject *puncher,
		const PointedThing &pointed)
{
	for (const auto &impl : m_node_implementations) {
		if (impl->node_on_punch(p, node, puncher, pointed))
			return true;
	}

	return false;
}

bool Router::node_on_dig(v3s16 p, MapNode node, ServerActiveObject *digger)
{
	for (const auto &impl : m_node_implementations) {
		if (impl->node_on_dig(p, node, digger))
			return true;
	}

	return false;
}
void Router::node_on_construct(v3s16 p, MapNode node)
{
	for (const auto &impl : m_node_implementations)
		impl->node_on_construct(p, node);
}

void Router::node_on_destruct(v3s16 p, MapNode node)
{
	for (const auto &impl : m_node_implementations)
		impl->node_on_destruct(p, node);
}

bool Router::node_on_flood(v3s16 p, MapNode node, MapNode newnode)
{
	for (const auto &impl : m_node_implementations) {
		if (impl->node_on_flood(p, node, newnode))
			return true;
	}

	return false;
}

void Router::node_after_destruct(v3s16 p, MapNode node)
{
	for (const auto &impl : m_node_implementations)
		impl->node_after_destruct(p, node);
}

bool Router::node_on_timer(v3s16 p, MapNode node, f32 dtime)
{
	for (const auto &impl : m_node_implementations) {
		if (impl->node_on_timer(p, node, dtime))
			return true;
	}

	return false;
}

void Router::node_on_receive_fields(v3s16 p, const std::string &formname,
		const StringMap &fields, ServerActiveObject *sender)
{
	for (const auto &impl : m_node_implementations)
		impl->node_on_receive_fields(p, formname, fields, sender);
}

void Router::addObjectReference(ServerActiveObject *obj)
{
	for (const auto &impl : m_entity_implementations)
		impl->addObjectReference(obj);
}

void Router::removeObjectReference(ServerActiveObject *obj)
{
	for (const auto &impl : m_entity_implementations)
		impl->removeObjectReference(obj);
}

bool Router::luaentity_Add(u16 id, const char *name)
{
	for (const auto &impl : m_entity_implementations) {
		if (impl->luaentity_Add(id, name))
			return true;
	}

	return false;
}

void Router::luaentity_Activate(u16 id, const std::string &staticdata, u32 dtime_s)
{
	for (const auto &impl : m_entity_implementations)
		impl->luaentity_Activate(id, staticdata, dtime_s);
}

void Router::luaentity_Deactivate(u16 id)
{
   for (const auto &impl : m_entity_implementations)
      impl->luaentity_Deactivate(id);
}

void Router::luaentity_Remove(u16 id)
{
	for (const auto &impl : m_entity_implementations)
		impl->luaentity_Remove(id);
}

std::string Router::luaentity_GetStaticdata(u16 id)
{
	for (const auto &impl : m_entity_implementations)
		return impl->luaentity_GetStaticdata(id);

	return "";
}

void Router::luaentity_GetProperties(
		u16 id, ServerActiveObject *self, ObjectProperties *prop)
{
	for (const auto &impl : m_entity_implementations)
		impl->luaentity_GetProperties(id, self, prop);
}

void Router::luaentity_Step(u16 id, float dtime, const collisionMoveResult *moveresult)
{
	for (const auto &impl : m_entity_implementations)
      impl->luaentity_Step(id, dtime, moveresult);
}

bool Router::luaentity_Punch(u16 id, ServerActiveObject *puncher,
		float time_from_last_punch, const ToolCapabilities *toolcap, v3f dir,
		s16 damage)
{
	for (const auto &impl : m_entity_implementations) {
      if (impl->luaentity_Punch(id, puncher, time_from_last_punch, toolcap,
				    dir, damage))
			return true;
	}

	return false;
}

bool Router::luaentity_on_death(u16 id, ServerActiveObject *killer)
{
	for (const auto &impl : m_entity_implementations) {
      if (impl->luaentity_on_death(id, killer))
			return true;
	}

	return false;
}

void Router::luaentity_Rightclick(u16 id, ServerActiveObject *clicker)
{
	for (const auto &impl : m_entity_implementations)
      impl->luaentity_Rightclick(id, clicker);
}

void Router::luaentity_on_attach_child(u16 id, ServerActiveObject *child)
{
	for (const auto &impl : m_entity_implementations)
		impl->luaentity_on_attach_child(id, child);
}

void Router::luaentity_on_detach_child(u16 id, ServerActiveObject *child)
{
	for (const auto &impl : m_entity_implementations)
		impl->luaentity_on_detach_child(id, child);
}

void Router::luaentity_on_detach(u16 id, ServerActiveObject *parent)
{
	for (const auto &impl : m_entity_implementations)
		impl->luaentity_on_detach(id, parent);
}

void Router::on_modchannel_message(const std::string &channel, const std::string &sender,
		const std::string &message)
{
	for (const auto &impl : m_modchannel_implementations)
		impl->on_modchannel_message(channel, sender, message);
}

void Router::on_modchannel_signal(const std::string &channel, ModChannelSignal signal)
{
	for (const auto &impl : m_modchannel_implementations)
		impl->on_modchannel_signal(channel, signal);
}

bool Router::item_OnDrop(ItemStack &item, ServerActiveObject *dropper, v3f pos)
{
	for (const auto &impl : m_item_implementations) {
		if (impl->item_OnDrop(item, dropper, pos))
			return true;
	}

	return false;
}

bool Router::item_OnPlace(
      Optional<ItemStack> &item,
      ServerActiveObject *placer,
      const PointedThing &pointed)
{
	for (const auto &impl : m_item_implementations) {
		if (impl->item_OnPlace(item, placer, pointed))
			return true;
	}

	return false;
}

bool Router::item_OnUse(
      Optional<ItemStack> &item,
      ServerActiveObject *user,
      const PointedThing &pointed)
{
	for (const auto &impl : m_item_implementations) {
		if (impl->item_OnUse(item, user, pointed))
			return true;
	}

	return false;
}

bool Router::item_OnSecondaryUse (
      Optional<ItemStack> &item,
      ServerActiveObject *user,
      const PointedThing &pointed)
{
	for (const auto &impl : m_item_implementations) {
      if (impl->item_OnSecondaryUse (item, user, pointed))
			return true;
	}

	return false;
}

bool Router::item_OnCraft(ItemStack &item, ServerActiveObject *user,
		const InventoryList *old_craft_grid, const InventoryLocation &craft_inv)
{
	for (const auto &impl : m_item_implementations) {
		if (impl->item_OnCraft(item, user, old_craft_grid, craft_inv))
			return true;
	}

	return false;
}

bool Router::item_CraftPredict(ItemStack &item, ServerActiveObject *user,
		const InventoryList *old_craft_grid, const InventoryLocation &craft_inv)
{
	for (const auto &impl : m_item_implementations) {
		if (impl->item_CraftPredict(item, user, old_craft_grid, craft_inv))
			return true;
	}

	return false;
}

// Return number of accepted items to be moved
int Router::nodemeta_inventory_AllowMove(
		const MoveAction &ma, int count, ServerActiveObject *player)
{
	for (const auto &impl : m_node_meta_implementations) {
		int changes = impl->nodemeta_inventory_AllowMove(ma, count, player);
		if (changes != 0)
			return changes;
	}

	return 0;
}

// Return number of accepted items to be put
int Router::nodemeta_inventory_AllowPut(
		const MoveAction &ma, const ItemStack &stack, ServerActiveObject *player)
{
	for (const auto &impl : m_node_meta_implementations) {
		int changes = impl->nodemeta_inventory_AllowPut(ma, stack, player);
		if (changes != 0)
			return changes;
	}

	return 0;
}

// Return number of accepted items to be taken
int Router::nodemeta_inventory_AllowTake(
		const MoveAction &ma, const ItemStack &stack, ServerActiveObject *player)
{
	for (const auto &impl : m_node_meta_implementations) {
		int changes = impl->nodemeta_inventory_AllowTake(ma, stack, player);
		if (changes != 0)
			return changes;
	}

	return 0;
}

// Report moved items
void Router::nodemeta_inventory_OnMove(
		const MoveAction &ma, int count, ServerActiveObject *player)
{
	for (const auto &impl : m_node_meta_implementations)
		impl->nodemeta_inventory_OnMove(ma, count, player);
}

// Report put items
void Router::nodemeta_inventory_OnPut(
		const MoveAction &ma, const ItemStack &stack, ServerActiveObject *player)
{
	for (const auto &impl : m_node_meta_implementations)
		impl->nodemeta_inventory_OnPut(ma, stack, player);
}

// Report taken items
void Router::nodemeta_inventory_OnTake(
		const MoveAction &ma, const ItemStack &stack, ServerActiveObject *player)
{
	for (const auto &impl : m_node_meta_implementations)
		impl->nodemeta_inventory_OnTake(ma, stack, player);
}

// Return number of accepted items to be moved
int Router::detached_inventory_AllowMove(
		const MoveAction &ma, int count, ServerActiveObject *player)
{
	for (const auto &impl : m_inv_implementations) {
		int changed = impl->detached_inventory_AllowMove(ma, count, player);
		if (changed != 0)
			return changed;
	}

	return 0;
}

// Return number of accepted items to be put
int Router::detached_inventory_AllowPut(
		const MoveAction &ma, const ItemStack &stack, ServerActiveObject *player)
{
	for (const auto &impl : m_inv_implementations) {
		int changed = impl->detached_inventory_AllowPut(ma, stack, player);
		if (changed != 0)
			return changed;
	}
	return 0;
}

// Return number of accepted items to be taken
int Router::detached_inventory_AllowTake(
		const MoveAction &ma, const ItemStack &stack, ServerActiveObject *player)
{
	for (const auto &impl : m_inv_implementations) {
		int changed = impl->detached_inventory_AllowTake(ma, stack, player);
		if (changed != 0)
			return changed;
	}

	return 0;
}

// Report moved items
void Router::detached_inventory_OnMove(
		const MoveAction &ma, int count, ServerActiveObject *player)
{
	for (const auto &impl : m_inv_implementations)
		impl->detached_inventory_OnMove(ma, count, player);
}

// Report put items
void Router::detached_inventory_OnPut(
		const MoveAction &ma, const ItemStack &stack, ServerActiveObject *player)
{
	for (const auto &impl : m_inv_implementations)
		impl->detached_inventory_OnPut(ma, stack, player);
}

// Report taken items
void Router::detached_inventory_OnTake(
		const MoveAction &ma, const ItemStack &stack, ServerActiveObject *player)
{
	for (const auto &impl : m_inv_implementations)
		impl->detached_inventory_OnTake(ma, stack, player);
}

}
}