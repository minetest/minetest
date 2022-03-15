#pragma once

class ServerActiveObject;
struct ItemStack;
struct MoveAction;
struct ToolCapabilities;

namespace api
{
namespace server
{
/*
 * Player callback events
 */

class Player
{
public:
	virtual void on_newplayer(ServerActiveObject *player) {}
	virtual void on_dieplayer(
			ServerActiveObject *player, const PlayerHPChangeReason &reason)
	{
	}
	virtual bool on_respawnplayer(ServerActiveObject *player) { return false; }
	virtual bool on_prejoinplayer(const std::string &name, const std::string &ip,
			std::string *reason)
	{
		return false;
	}
	virtual bool can_bypass_userlimit(const std::string &name, const std::string &ip)
	{
		return false;
	}
	virtual void on_joinplayer(ServerActiveObject *player) {}
	virtual void on_leaveplayer(ServerActiveObject *player, bool timeout) {}
	virtual void on_cheat(ServerActiveObject *player, const std::string &cheat_type)
	{
	}
	virtual bool on_punchplayer(ServerActiveObject *player,
			ServerActiveObject *hitter, float time_from_last_punch,
			const ToolCapabilities *toolcap, v3f dir, s16 damage)
	{
		return false;
	}
	virtual s32 on_player_hpchange(ServerActiveObject *player, s32 hp_change,
			const PlayerHPChangeReason &reason)
	{
		return 0;
	}
	virtual void on_playerReceiveFields(ServerActiveObject *player,
			const std::string &formname, const StringMap &fields)
	{
	}
	virtual void on_auth_failure(const std::string &name, const std::string &ip) {}

	virtual int player_inventory_AllowMove(
			const MoveAction &ma, int count, ServerActiveObject *player)
	{
		return 0;
	}
	// Return number of accepted items to be put
	virtual int player_inventory_AllowPut(const MoveAction &ma,
			const ItemStack &stack, ServerActiveObject *player)
	{
		return 0;
	}
	// Return number of accepted items to be taken
	virtual int player_inventory_AllowTake(const MoveAction &ma,
			const ItemStack &stack, ServerActiveObject *player)
	{
		return 0;
	}
	// Report moved items
	virtual void player_inventory_OnMove(
			const MoveAction &ma, int count, ServerActiveObject *player)
	{
	}
	// Report put items
	virtual void player_inventory_OnPut(const MoveAction &ma, const ItemStack &stack,
			ServerActiveObject *player)
	{
	}
	// Report taken items
	virtual void player_inventory_OnTake(const MoveAction &ma, const ItemStack &stack,
			ServerActiveObject *player)
	{
	}
};
}
}