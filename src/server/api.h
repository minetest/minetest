#pragma once

#include <memory>
#include <set>
#include <string>
#include <vector>
#include "irrlichttypes_bloated.h"
#include "mapnode.h"
#include "modchannels.h"
#include "util/string.h"

class InventoryList;
class ServerActiveObject;
class ServerScripting;
struct collisionMoveResult;
struct InventoryLocation;
struct ItemStack;
struct MoveAction;
struct ObjectProperties;
struct PlayerHPChangeReason;
struct PointedThing;
struct ToolCapabilities;

namespace api
{
namespace server
{

class Entity;
class Environment;
class Global;
class Inventory;
class Item;
class ModChannels;
class Node;
class NodeMeta;
class Player;

class Router
{
public:
	Router() = default;
	~Router() = default;

	template <typename T> void registerAPI(std::shared_ptr<T> api);

	void setLuaAPI(ServerScripting *lua_pi)
	{
		assert(!m_lua_api);
		m_lua_api = lua_pi;
	}
	ServerScripting *getLuaAPI();

	/*
	 * Server callback routes
	 */

	bool on_chat_message(const std::string &name, const std::string &message);
	void on_shutdown();

	std::string formatChatMessage(
			const std::string &name, const std::string &message);

	bool getAuth(const std::string &playername, std::string *dst_password,
			std::set<std::string> *dst_privs);
	void createAuth(const std::string &playername, const std::string &password);
	bool setPassword(const std::string &playername, const std::string &password);

	/*
	 * Environment routes
	 */

	// Called on environment step
	void environment_Step(float dtime);

	// Called after generating a piece of map
	void environment_OnGenerated(v3s16 minp, v3s16 maxp, u32 blockseed);

	// Called on player event
	void player_event(ServerActiveObject *player, const std::string &type);

	// Called after emerge of a block queued from core.emerge_area()
	void on_emerge_area_completion(v3s16 blockpos, int action, void *state);

	/*
	 * Player callback routes
	 */
	void on_newplayer(ServerActiveObject *player);
	void on_dieplayer(ServerActiveObject *player, const PlayerHPChangeReason &reason);
	bool on_respawnplayer(ServerActiveObject *player);
	bool on_prejoinplayer(const std::string &name, const std::string &ip,
			std::string *reason);
	bool can_bypass_userlimit(const std::string &name, const std::string &ip);
	void on_joinplayer(ServerActiveObject *player);
	void on_leaveplayer(ServerActiveObject *player, bool timeout);
	void on_cheat(ServerActiveObject *player, const std::string &cheat_type);
	bool on_punchplayer(ServerActiveObject *player, ServerActiveObject *hitter,
			float time_from_last_punch, const ToolCapabilities *toolcap,
			v3f dir, s16 damage);
	s32 on_player_hpchange(ServerActiveObject *player, s32 hp_change,
			const PlayerHPChangeReason &reason);
	void on_playerReceiveFields(ServerActiveObject *player,
			const std::string &formname, const StringMap &fields);
	void on_auth_failure(const std::string &name, const std::string &ip);

	// Player inventory callbacks
	// Return number of accepted items to be moved
	int player_inventory_AllowMove(
			const MoveAction &ma, int count, ServerActiveObject *player);
	// Return number of accepted items to be put
	int player_inventory_AllowPut(const MoveAction &ma, const ItemStack &stack,
			ServerActiveObject *player);
	// Return number of accepted items to be taken
	int player_inventory_AllowTake(const MoveAction &ma, const ItemStack &stack,
			ServerActiveObject *player);
	// Report moved items
	void player_inventory_OnMove(
			const MoveAction &ma, int count, ServerActiveObject *player);
	// Report put items
	void player_inventory_OnPut(const MoveAction &ma, const ItemStack &stack,
			ServerActiveObject *player);
	// Report taken items
	void player_inventory_OnTake(const MoveAction &ma, const ItemStack &stack,
			ServerActiveObject *player);

	/*
	 * Items
	 */

	bool item_OnDrop(ItemStack &item, ServerActiveObject *dropper, v3f pos);
	bool item_OnPlace(ItemStack &item, ServerActiveObject *placer,
			const PointedThing &pointed);
	bool item_OnUse(ItemStack &item, ServerActiveObject *user,
			const PointedThing &pointed);
	bool item_OnSecondaryUse(ItemStack &item, ServerActiveObject *user,
			const PointedThing &pointed);
	bool item_OnCraft(ItemStack &item, ServerActiveObject *user,
			const InventoryList *old_craft_grid,
			const InventoryLocation &craft_inv);
	bool item_CraftPredict(ItemStack &item, ServerActiveObject *user,
			const InventoryList *old_craft_grid,
			const InventoryLocation &craft_inv);

	/*
	 * Detached inventory
	 */

	// Return number of accepted items to be moved
	int detached_inventory_AllowMove(
			const MoveAction &ma, int count, ServerActiveObject *player);
	// Return number of accepted items to be put
	int detached_inventory_AllowPut(const MoveAction &ma, const ItemStack &stack,
			ServerActiveObject *player);
	// Return number of accepted items to be taken
	int detached_inventory_AllowTake(const MoveAction &ma, const ItemStack &stack,
			ServerActiveObject *player);
	// Report moved items
	void detached_inventory_OnMove(
			const MoveAction &ma, int count, ServerActiveObject *player);
	// Report put items
	void detached_inventory_OnPut(const MoveAction &ma, const ItemStack &stack,
			ServerActiveObject *player);
	// Report taken items
	void detached_inventory_OnTake(const MoveAction &ma, const ItemStack &stack,
			ServerActiveObject *player);

	/*
	 * Lua entities
	 */

	void addObjectReference(ServerActiveObject *obj);
	void removeObjectReference(ServerActiveObject *obj);

	bool luaentity_Add(u16 id, const char *name);
	void luaentity_Activate(u16 id, const std::string &staticdata, u32 dtime_s);
	void luaentity_Remove(u16 id);
	std::string luaentity_GetStaticdata(u16 id);
	void luaentity_GetProperties(
			u16 id, ServerActiveObject *self, ObjectProperties *prop);
	void on_entity_step(u16 id, float dtime, const collisionMoveResult *moveresult);
	bool on_entity_punched(u16 id, ServerActiveObject *puncher,
			float time_from_last_punch, const ToolCapabilities *toolcap,
			v3f dir, s16 damage);
	bool on_entity_death(u16 id, ServerActiveObject *killer);
	void on_entity_rightclick(u16 id, ServerActiveObject *clicker);
	void luaentity_on_attach_child(u16 id, ServerActiveObject *child);
	void luaentity_on_detach_child(u16 id, ServerActiveObject *child);
	void luaentity_on_detach(u16 id, ServerActiveObject *parent);

	/*
	 * Mod channels
	 */
	void on_modchannel_message(const std::string &channel, const std::string &sender,
			const std::string &message);
	void on_modchannel_signal(const std::string &channel, ModChannelSignal signal);

	/*
	 * Nodes
	 */

	bool node_on_punch(v3s16 p, MapNode node, ServerActiveObject *puncher,
			const PointedThing &pointed);
	bool node_on_dig(v3s16 p, MapNode node, ServerActiveObject *digger);
	void node_on_construct(v3s16 p, MapNode node);
	void node_on_destruct(v3s16 p, MapNode node);
	bool node_on_flood(v3s16 p, MapNode node, MapNode newnode);
	void node_after_destruct(v3s16 p, MapNode node);
	bool node_on_timer(v3s16 p, MapNode node, f32 dtime);
	void node_on_receive_fields(v3s16 p, const std::string &formname,
			const StringMap &fields, ServerActiveObject *sender);

	// Return number of accepted items to be moved
	int nodemeta_inventory_AllowMove(
			const MoveAction &ma, int count, ServerActiveObject *player);
	// Return number of accepted items to be put
	int nodemeta_inventory_AllowPut(const MoveAction &ma, const ItemStack &stack,
			ServerActiveObject *player);
	// Return number of accepted items to be taken
	int nodemeta_inventory_AllowTake(const MoveAction &ma, const ItemStack &stack,
			ServerActiveObject *player);
	// Report moved items
	void nodemeta_inventory_OnMove(
			const MoveAction &ma, int count, ServerActiveObject *player);
	// Report put items
	void nodemeta_inventory_OnPut(const MoveAction &ma, const ItemStack &stack,
			ServerActiveObject *player);
	// Report taken items
	void nodemeta_inventory_OnTake(const MoveAction &ma, const ItemStack &stack,
			ServerActiveObject *player);

private:
	std::vector<std::shared_ptr<Entity>> m_entity_implementations;
	std::vector<std::shared_ptr<Environment>> m_env_implementations;
	std::vector<std::shared_ptr<Global>> m_global_implementations;
	std::vector<std::shared_ptr<Inventory>> m_inv_implementations;
	std::vector<std::shared_ptr<Item>> m_item_implementations;
	std::vector<std::shared_ptr<ModChannels>> m_modchannel_implementations;
	std::vector<std::shared_ptr<Node>> m_node_implementations;
	std::vector<std::shared_ptr<NodeMeta>> m_node_meta_implementations;
	std::vector<std::shared_ptr<Player>> m_player_implementations;

	// Special route for lua embedded API for LuaLBM & LuaABM
	ServerScripting *m_lua_api = nullptr;
};

}
}