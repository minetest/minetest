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
		HUD_FLAG_MINIMAP_RADAR_VISIBLE;

	hud_hotbar_itemcount = HUD_HOTBAR_ITEMCOUNT_DEFAULT;

	m_player_settings.readGlobalSettings();
	// Register player setting callbacks
	for (const std::string &name : m_player_settings.setting_names)
		g_settings->registerChangedCallback(name,
			&Player::settingsChangedCallback, &m_player_settings);
}

Player::~Player()
{
	// m_player_settings becomes invalid, remove callbacks
	for (const std::string &name : m_player_settings.setting_names)
		g_settings->deregisterChangedCallback(name,
			&Player::settingsChangedCallback, &m_player_settings);
	clearHud();
}

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

void PlayerSettings::readGlobalSettings()
{
	free_move = g_settings->getBool("free_move");
	pitch_move = g_settings->getBool("pitch_move");
	fast_move = g_settings->getBool("fast_move");
	continuous_forward = g_settings->getBool("continuous_forward");
	always_fly_fast = g_settings->getBool("always_fly_fast");
	aux1_descends = g_settings->getBool("aux1_descends");
	noclip = g_settings->getBool("noclip");
	autojump = g_settings->getBool("autojump");
}

void Player::settingsChangedCallback(const std::string &name, void *data)
{
	((PlayerSettings *)data)->readGlobalSettings();
}
