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

#include "player.h"

#include <cmath>
#include "threading/mutex_auto_lock.h"
#include "util/numeric.h"
#include "hud.h"
#include "constants.h"
#include "gamedef.h"
#include "settings.h"
#include "log.h"
#include "porting.h"  // strlcpy


Player::Player(const char *name, IItemDefManager *idef):
	inventory(idef)
{
	strlcpy(m_name, name, PLAYERNAME_SIZE);

	inventory.clear();
	inventory.addList("main", PLAYER_INVENTORY_SIZE);
	InventoryList *craft = inventory.addList("craft", 9);
	craft->setWidth(3);
	inventory.addList("craftpreview", 1);
	inventory.addList("craftresult", 1);
	inventory.addList("offhand", 1);
	inventory.setModified(false);

	// Can be redefined via Lua
	inventory_formspec = "size[8,7.5]"
		//"image[1,0.6;1,2;player.png]"
		"list[current_player;main;0,3.5;8,4;]"
		"list[current_player;craft;3,0;3,3;]"
		"listring[]"
		"list[current_player;craftpreview;7,1;1,1;]";

	// Initialize movement settings at default values, so movement can work
	// if the server fails to send them
	movement_acceleration_default   = 3    * BS;
	movement_acceleration_air       = 2    * BS;
	movement_acceleration_fast      = 10   * BS;
	movement_speed_walk             = 4    * BS;
	movement_speed_crouch           = 1.35 * BS;
	movement_speed_fast             = 20   * BS;
	movement_speed_climb            = 2    * BS;
	movement_speed_jump             = 6.5  * BS;
	movement_liquid_fluidity        = 1    * BS;
	movement_liquid_fluidity_smooth = 0.5  * BS;
	movement_liquid_sink            = 10   * BS;
	movement_gravity                = 9.81 * BS;
	local_animation_speed           = 0.0;

	hud_flags =
		HUD_FLAG_HOTBAR_VISIBLE    | HUD_FLAG_HEALTHBAR_VISIBLE |
		HUD_FLAG_CROSSHAIR_VISIBLE | HUD_FLAG_WIELDITEM_VISIBLE |
		HUD_FLAG_BREATHBAR_VISIBLE | HUD_FLAG_MINIMAP_VISIBLE   |
		HUD_FLAG_MINIMAP_RADAR_VISIBLE | HUD_FLAG_BASIC_DEBUG   |
		HUD_FLAG_CHAT_VISIBLE;

	hud_hotbar_itemcount = HUD_HOTBAR_ITEMCOUNT_DEFAULT;
}

Player::~Player()
{
	clearHud();
}

void Player::setWieldIndex(u16 index)
{
	const InventoryList *mlist = inventory.getList("main");
	m_wield_index = MYMIN(index, mlist ? mlist->getSize() : 0);
}

ItemStack &Player::getWieldedItem(ItemStack *selected, ItemStack *hand) const
{
	assert(selected);

	const InventoryList *mlist = inventory.getList("main"); // TODO: Make this generic
	const InventoryList *hlist = inventory.getList("hand");

	if (current_used_hand == MAINHAND) {
		if (mlist && m_wield_index < mlist->getSize())
			*selected = mlist->getItem(m_wield_index);
	}
	else
		getOffhandWieldedItem(selected);

	if (hand && hlist)
		*hand = hlist->getItem(0);

	// Return effective tool item
	return (hand && selected->name.empty()) ? *hand : *selected;
}

void Player::getOffhandWieldedItem(ItemStack *offhand) const
{
	assert(offhand);

	const InventoryList *olist = inventory.getList("offhand");

	if (olist)
		*offhand = olist->getItem(0);
}

HandIndex Player::getCurrentUsedHand(IItemDefManager *idef, const PointedThing &pointed) const
{
	ItemStack main, offhand;

	getWieldedItem(&main, nullptr);
	getOffhandWieldedItem(&offhand);

	const ItemDefinition &main_def = main.getDefinition(idef);
	const ItemDefinition &offhand_def = offhand.getDefinition(idef);

	bool main_usable, offhand_usable;

	// figure out which item to use for placements

	if (pointed.type == POINTEDTHING_NODE) {
		// an item can be used on nodes if it has a place handler or prediction
		main_usable = main_def.has_on_place || main_def.node_placement_prediction != "";
		offhand_usable = offhand_def.has_on_place || offhand_def.node_placement_prediction != "";
	} else {
		// an item can be used on anything else if it has a secondary use handler
		main_usable = main_def.has_on_secondary_use;
		offhand_usable = offhand_def.has_on_secondary_use;
	}

	// main hand has priority
	return (HandIndex)(offhand_usable && !main_usable);
}

/*bool Player::getOffhandWieldedItem(ItemStack *offhand, ItemStack *place, IItemDefManager *idef, const PointedThing &pointed) const
{
	assert(offhand);

	ItemStack main;

	const InventoryList *mlist = inventory.getList("main");
	const InventoryList *olist = inventory.getList("offhand");

	if (olist)
		*offhand = olist->getItem(0);

	if (mlist && m_wield_index < mlist->getSize())
		main = mlist->getItem(m_wield_index);

	const ItemDefinition &main_def = main.getDefinition(idef);
	const ItemDefinition &offhand_def = offhand->getDefinition(idef);
	bool main_usable, offhand_usable;

	// figure out which item to use for placements

	if (pointed.type == POINTEDTHING_NODE) {
		// an item can be used on nodes if it has a place handler or prediction
		main_usable = main_def.has_on_place || main_def.node_placement_prediction != "";
		offhand_usable = offhand_def.has_on_place || offhand_def.node_placement_prediction != "";
	} else {
		// an item can be used on anything else if it has a secondary use handler
		main_usable = main_def.has_on_secondary_use;
		offhand_usable = offhand_def.has_on_secondary_use;
	}

	// main hand has priority
	bool use_offhand = offhand_usable && !main_usable;

	if (place)
		*place = use_offhand ? *offhand : main;

	return use_offhand;
}*/

u32 Player::addHud(HudElement *toadd)
{
	MutexAutoLock lock(m_mutex);

	u32 id = getFreeHudID();

	if (id < hud.size())
		hud[id] = toadd;
	else
		hud.push_back(toadd);

	return id;
}

HudElement* Player::getHud(u32 id)
{
	MutexAutoLock lock(m_mutex);

	if (id < hud.size())
		return hud[id];

	return NULL;
}

void Player::hudApply(std::function<void(const std::vector<HudElement*>&)> f)
{
	MutexAutoLock lock(m_mutex);
	f(hud);
}

HudElement* Player::removeHud(u32 id)
{
	MutexAutoLock lock(m_mutex);

	HudElement* retval = NULL;
	if (id < hud.size()) {
		retval = hud[id];
		hud[id] = NULL;
	}
	return retval;
}

void Player::clearHud()
{
	MutexAutoLock lock(m_mutex);

	while(!hud.empty()) {
		delete hud.back();
		hud.pop_back();
	}
}

#ifndef SERVER

u32 PlayerControl::getKeysPressed() const
{
	u32 keypress_bits =
		( (u32)(jump  & 1) << 4) |
		( (u32)(aux1  & 1) << 5) |
		( (u32)(sneak & 1) << 6) |
		( (u32)(dig   & 1) << 7) |
		( (u32)(place & 1) << 8) |
		( (u32)(zoom  & 1) << 9)
	;

	// If any direction keys are pressed pass those through
	if (direction_keys != 0)
	{
		keypress_bits |= direction_keys;
	}
	// Otherwise set direction keys based on joystick movement (for mod compatibility)
	else if (isMoving())
	{
		float abs_d;

		// (absolute value indicates forward / backward)
		abs_d = std::abs(movement_direction);
		if (abs_d < 3.0f / 8.0f * M_PI)
			keypress_bits |= (u32)1; // Forward
		if (abs_d > 5.0f / 8.0f * M_PI)
			keypress_bits |= (u32)1 << 1; // Backward

		// rotate entire coordinate system by 90 degree
		abs_d = movement_direction + M_PI_2;
		if (abs_d >= M_PI)
			abs_d -= 2 * M_PI;
		abs_d = std::abs(abs_d);
		// (value now indicates left / right)
		if (abs_d < 3.0f / 8.0f * M_PI)
			keypress_bits |= (u32)1 << 2; // Left
		if (abs_d > 5.0f / 8.0f * M_PI)
			keypress_bits |= (u32)1 << 3; // Right
	}

	return keypress_bits;
}

#endif

void PlayerControl::unpackKeysPressed(u32 keypress_bits)
{
	direction_keys = keypress_bits & 0xf;
	jump  = keypress_bits & (1 << 4);
	aux1  = keypress_bits & (1 << 5);
	sneak = keypress_bits & (1 << 6);
	dig   = keypress_bits & (1 << 7);
	place = keypress_bits & (1 << 8);
	zoom  = keypress_bits & (1 << 9);
}
