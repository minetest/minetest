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
#include "nodedef.h"
#include "map.h"


Player::Player(const char *name, IItemDefManager *idef):
	inventory(idef)
{
	strlcpy(m_name, name, PLAYERNAME_SIZE);

	inventory.clear();
	inventory.addList("main", PLAYER_INVENTORY_SIZE);
	inventory.addList("hand", 1);
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
}

Player::~Player()
{
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

void Player::applyControlLogEntry(const ControlLogEntry &cle, Environment *env)
{

	// SERVER SIDE MOVEMENT: this method will need to be abstracted into:
	// - a wrapper that pushes the current controls into the control log
	// - a function that can be called on the client OR on the server (when
	//   replaying the control log).


	float dtime = cle.getDtime();

	// Clear stuff
	swimming_vertical = false;

	setPitch(cle.getPitch());
	setYaw(cle.getYaw());

	// Nullify speed and don't run positioning code if the player is attached
	if(isAttached())
	{
		setSpeed(v3f(0,0,0));
		return;
	}

	v3f move_direction = v3f(0,0,1);
	move_direction.rotateXZBy(getYaw());

	v3f speedH = v3f(0,0,0); // Horizontal (X, Z)
	v3f speedV = v3f(0,0,0); // Vertical (Y)

	bool fly_allowed = checkPrivilege("fly");
	bool fast_allowed = checkPrivilege("fast");

	bool free_move = fly_allowed && cle.getFreeMove();
	bool fast_move = fast_allowed && cle.getFastMove();
	// When aux1_descends is enabled the fast key is used to go down, so fast isn't possible
	bool fast_climb = fast_move && cle.getAux1() && !cle.getAux1Descends();
	bool continuous_forward = cle.getContinuousForward();
	bool always_fly_fast = cle.getAlwaysFlyFast();

	// Whether superspeed mode is used or not
	bool superspeed = false;

	if (always_fly_fast && free_move && fast_move)
		superspeed = true;

	// Old descend control
	if(cle.getAux1Descends())
	{
		// If free movement and fast movement, always move fast
		if(free_move && fast_move)
			superspeed = true;

		// Auxiliary button 1 (E)
		if(cle.getAux1())
		{
			if(free_move)
			{
				// In free movement mode, aux1 descends
				if(fast_move)
					speedV.Y = -movement_speed_fast;
				else
					speedV.Y = -movement_speed_walk;
			}
			else if(in_liquid || in_liquid_stable)
			{
				speedV.Y = -movement_speed_walk;
				swimming_vertical = true;
			}
			else if(is_climbing)
			{
				speedV.Y = -movement_speed_climb;
			}
			else
			{
				// If not free movement but fast is allowed, aux1 is
				// "Turbo button"
				if(fast_move)
					superspeed = true;
			}
		}
	}
	// New minecraft-like descend control
	else
	{
		// Auxiliary button 1 (E)
		if(cle.getAux1())
		{
			if(!is_climbing)
			{
				// aux1 is "Turbo button"
				if(fast_move)
					superspeed = true;
			}
		}

		if(cle.getSneak())
		{
			if(free_move)
			{
				// In free movement mode, sneak descends
				if (fast_move && (cle.getAux1() || always_fly_fast))
					speedV.Y = -movement_speed_fast;
				else
					speedV.Y = -movement_speed_walk;
			}
			else if(in_liquid || in_liquid_stable)
			{
				if(fast_climb)
					speedV.Y = -movement_speed_fast;
				else
					speedV.Y = -movement_speed_walk;
				swimming_vertical = true;
			}
			else if(is_climbing)
			{
				if(fast_climb)
					speedV.Y = -movement_speed_fast;
				else
					speedV.Y = -movement_speed_climb;
			}
		}
	}

	if (continuous_forward)
		speedH += move_direction;

	if (cle.getUp()) {
		if (continuous_forward) {
			if (fast_move)
				superspeed = true;
		} else {
			speedH += move_direction;
		}
	}
	if (cle.getDown()) {
		speedH -= move_direction;
	}
	if (!cle.getUp() && !cle.getDown()) {
		speedH -= move_direction * cle.getJoyForw();
	}
	if (cle.getLeft()) {
		speedH += move_direction.crossProduct(v3f(0,1,0));
	}
	if (cle.getRight()) {
		speedH += move_direction.crossProduct(v3f(0,-1,0));
	}
	if (!cle.getLeft() && !cle.getRight()) {
		speedH -= move_direction.crossProduct(v3f(0,1,0)) *
			cle.getJoySidew();
	}
	if(cle.getJump())
	{
		if (free_move) {
			if (cle.getAux1Descends() || always_fly_fast) {
				if (fast_move)
					speedV.Y = movement_speed_fast;
				else
					speedV.Y = movement_speed_walk;
			} else {
				if(fast_move && cle.getAux1())
					speedV.Y = movement_speed_fast;
				else
					speedV.Y = movement_speed_walk;
			}
		}
		else if(m_can_jump)
		{
			/*
				NOTE: The d value in move() affects jump height by
				raising the height at which the jump speed is kept
				at its starting value
			*/
			v3f speedJ = getSpeed();
			if(speedJ.Y >= -0.5 * BS) {
				speedJ.Y = movement_speed_jump * physics_override_jump;
				setSpeed(speedJ);

				triggerJumpEvent();
			}
		}
		else if(in_liquid)
		{
			if(fast_climb)
				speedV.Y = movement_speed_fast;
			else
				speedV.Y = movement_speed_walk;
			swimming_vertical = true;
		}
		else if(is_climbing)
		{
			if(fast_climb)
				speedV.Y = movement_speed_fast;
			else
				speedV.Y = movement_speed_climb;
		}
	}

	// The speed of the player (Y is ignored)
	if(superspeed || (is_climbing && fast_climb) || ((in_liquid || in_liquid_stable) && fast_climb))
		speedH = speedH.normalize() * movement_speed_fast;
	else if(cle.getSneak() && !free_move && !in_liquid && !in_liquid_stable)
		speedH = speedH.normalize() * movement_speed_crouch;
	else
		speedH = speedH.normalize() * movement_speed_walk;

	// Acceleration increase
	f32 incH = 0; // Horizontal (X, Z)
	f32 incV = 0; // Vertical (Y)
	if((!touching_ground && !free_move && !is_climbing && !in_liquid) || (!free_move && m_can_jump && cle.getJump()))
	{
		// Jumping and falling
		if(superspeed || (fast_move && cle.getAux1()))
			incH = movement_acceleration_fast * BS * dtime;
		else
			incH = movement_acceleration_air * BS * dtime;
		incV = 0; // No vertical acceleration in air
	}
	else if (superspeed || (is_climbing && fast_climb) || ((in_liquid || in_liquid_stable) && fast_climb))
		incH = incV = movement_acceleration_fast * BS * dtime;
	else
		incH = incV = movement_acceleration_default * BS * dtime;

	float slip_factor = 1.0f;
	if (!free_move)
		slip_factor = getSlipFactor(env, speedH);

	// Accelerate to target speed with maximum increment
	accelerateHorizontal(speedH * physics_override_speed,
			incH * physics_override_speed * slip_factor);
	accelerateVertical(speedV * physics_override_speed,
			incV * physics_override_speed);
}

// Horizontal acceleration (X and Z), Y direction is ignored
void Player::accelerateHorizontal(const v3f &target_speed,
	const f32 max_increase)
{
        if (max_increase == 0)
                return;

	v3f d_wanted = target_speed - m_speed;
	d_wanted.Y = 0.0f;
	f32 dl = d_wanted.getLength();
	if (dl > max_increase)
		dl = max_increase;

	v3f d = d_wanted.normalize() * dl;

	m_speed.X += d.X;
	m_speed.Z += d.Z;
}

// Vertical acceleration (Y), X and Z directions are ignored
void Player::accelerateVertical(const v3f &target_speed, const f32 max_increase)
{
	if (max_increase == 0)
		return;

	f32 d_wanted = target_speed.Y - m_speed.Y;
	if (d_wanted > max_increase)
		d_wanted = max_increase;
	else if (d_wanted < -max_increase)
		d_wanted = -max_increase;

	m_speed.Y += d_wanted;
}

float Player::getSlipFactor(Environment *env, const v3f &speedH)
{
	// Slip on slippery nodes
	const NodeDefManager *nodemgr = env->getGameDef()->ndef();
	Map *map = &env->getMap();
	const ContentFeatures &f = nodemgr->get(map->getNodeNoEx(
			getStandingNodePos()));
	int slippery = 0;
	if (f.walkable)
		slippery = itemgroup_get(f.groups, "slippery");

	if (slippery >= 1) {
		if (speedH == v3f(0.0f)) {
			slippery = slippery * 2;
		}
		return core::clamp(1.0f / (slippery + 1), 0.001f, 1.0f);
	}
	return 1.0f;
}

v3s16 Player::getStandingNodePos()
{
	if(m_sneak_node_exists)
		return m_sneak_node;
	return m_standing_node;
}


