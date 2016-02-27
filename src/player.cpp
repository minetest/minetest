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

#include <fstream>
#include "threading/mutex_auto_lock.h"
#include "util/numeric.h"
#include "hud.h"
#include "constants.h"
#include "gamedef.h"
#include "settings.h"
#include "content_sao.h"
#include "filesys.h"
#include "log.h"
#include "porting.h"  // strlcpy


Player::Player(IGameDef *gamedef, const char *name):
	touching_ground(false),
	in_liquid(false),
	in_liquid_stable(false),
	liquid_viscosity(0),
	is_climbing(false),
	swimming_vertical(false),
	camera_barely_in_ceiling(false),
	inventory(gamedef->idef()),
	hp(PLAYER_MAX_HP),
	hurt_tilt_timer(0),
	hurt_tilt_strength(0),
	protocol_version(0),
	peer_id(PEER_ID_INEXISTENT),
	keyPressed(0),
// protected
	m_gamedef(gamedef),
	m_breath(PLAYER_MAX_BREATH),
	m_pitch(0),
	m_yaw(0),
	m_speed(0,0,0),
	m_position(0,0,0),
	m_collisionbox(-BS*0.30,0.0,-BS*0.30,BS*0.30,BS*1.75,BS*0.30),
	m_dirty(false)
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

	// Movement overrides are multipliers and must be 1 by default
	physics_override_speed        = 1;
	physics_override_jump         = 1;
	physics_override_gravity      = 1;
	physics_override_sneak        = true;
	physics_override_sneak_glitch = true;

	hud_flags =
		HUD_FLAG_HOTBAR_VISIBLE    | HUD_FLAG_HEALTHBAR_VISIBLE |
		HUD_FLAG_CROSSHAIR_VISIBLE | HUD_FLAG_WIELDITEM_VISIBLE |
		HUD_FLAG_BREATHBAR_VISIBLE | HUD_FLAG_MINIMAP_VISIBLE;

	hud_hotbar_itemcount = HUD_HOTBAR_ITEMCOUNT_DEFAULT;
}

Player::~Player()
{
	clearHud();
}

v3s16 Player::getLightPosition() const
{
	return floatToInt(m_position + v3f(0,BS+BS/2,0), BS);
}

void Player::serialize(std::ostream &os)
{
	// Utilize a Settings object for storing values
	Settings args;
	args.setS32("version", 1);
	args.set("name", m_name);
	//args.set("password", m_password);
	args.setFloat("pitch", m_pitch);
	args.setFloat("yaw", m_yaw);
	args.setV3F("position", m_position);
	args.setS32("hp", hp);
	args.setS32("breath", m_breath);

	args.writeLines(os);

	os<<"PlayerArgsEnd\n";

	inventory.serialize(os);
}

void Player::deSerialize(std::istream &is, std::string playername)
{
	Settings args;

	if (!args.parseConfigLines(is, "PlayerArgsEnd")) {
		throw SerializationError("PlayerArgsEnd of player " +
				playername + " not found!");
	}

	m_dirty = true;
	//args.getS32("version"); // Version field value not used
	std::string name = args.get("name");
	strlcpy(m_name, name.c_str(), PLAYERNAME_SIZE);
	setPitch(args.getFloat("pitch"));
	setYaw(args.getFloat("yaw"));
	setPosition(args.getV3F("position"));
	try{
		hp = args.getS32("hp");
	}catch(SettingNotFoundException &e) {
		hp = PLAYER_MAX_HP;
	}
	try{
		m_breath = args.getS32("breath");
	}catch(SettingNotFoundException &e) {
		m_breath = PLAYER_MAX_BREATH;
	}

	inventory.deSerialize(is);

	if(inventory.getList("craftpreview") == NULL) {
		// Convert players without craftpreview
		inventory.addList("craftpreview", 1);

		bool craftresult_is_preview = true;
		if(args.exists("craftresult_is_preview"))
			craftresult_is_preview = args.getBool("craftresult_is_preview");
		if(craftresult_is_preview)
		{
			// Clear craftresult
			inventory.getList("craftresult")->changeItem(0, ItemStack());
		}
	}
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

RemotePlayer::RemotePlayer(IGameDef *gamedef, const char *name):
	Player(gamedef, name),
	m_sao(NULL)
{
	movement_acceleration_default   = g_settings->getFloat("movement_acceleration_default")   * BS;
	movement_acceleration_air       = g_settings->getFloat("movement_acceleration_air")       * BS;
	movement_acceleration_fast      = g_settings->getFloat("movement_acceleration_fast")      * BS;
	movement_speed_walk             = g_settings->getFloat("movement_speed_walk")             * BS;
	movement_speed_crouch           = g_settings->getFloat("movement_speed_crouch")           * BS;
	movement_speed_fast             = g_settings->getFloat("movement_speed_fast")             * BS;
	movement_speed_climb            = g_settings->getFloat("movement_speed_climb")            * BS;
	movement_speed_jump             = g_settings->getFloat("movement_speed_jump")             * BS;
	movement_liquid_fluidity        = g_settings->getFloat("movement_liquid_fluidity")        * BS;
	movement_liquid_fluidity_smooth = g_settings->getFloat("movement_liquid_fluidity_smooth") * BS;
	movement_liquid_sink            = g_settings->getFloat("movement_liquid_sink")            * BS;
	movement_gravity                = g_settings->getFloat("movement_gravity")                * BS;
}

void RemotePlayer::save(std::string savedir)
{
	/*
	 * We have to open all possible player files in the players directory
	 * and check their player names because some file systems are not
	 * case-sensitive and player names are case-sensitive.
	 */

	// A player to deserialize files into to check their names
	RemotePlayer testplayer(m_gamedef, "");

	savedir += DIR_DELIM;
	std::string path = savedir + m_name;
	for (u32 i = 0; i < PLAYER_FILE_ALTERNATE_TRIES; i++) {
		if (!fs::PathExists(path)) {
			// Open file and serialize
			std::ostringstream ss(std::ios_base::binary);
			serialize(ss);
			if (!fs::safeWriteToFile(path, ss.str())) {
				infostream << "Failed to write " << path << std::endl;
			}
			setModified(false);
			return;
		}
		// Open file and deserialize
		std::ifstream is(path.c_str(), std::ios_base::binary);
		if (!is.good()) {
			infostream << "Failed to open " << path << std::endl;
			return;
		}
		testplayer.deSerialize(is, path);
		is.close();
		if (strcmp(testplayer.getName(), m_name) == 0) {
			// Open file and serialize
			std::ostringstream ss(std::ios_base::binary);
			serialize(ss);
			if (!fs::safeWriteToFile(path, ss.str())) {
				infostream << "Failed to write " << path << std::endl;
			}
			setModified(false);
			return;
		}
		path = savedir + m_name + itos(i);
	}

	infostream << "Didn't find free file for player " << m_name << std::endl;
	return;
}

/*
	RemotePlayer
*/
void RemotePlayer::setPosition(const v3f &position)
{
	Player::setPosition(position);
	if(m_sao)
		m_sao->setBasePosition(position);
}

