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
#include "collision.h"

static aabb3f getNodeBoundingBox(const std::vector<aabb3f> &nodeboxes);

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
	//debugVec("Speed after CLE", m_speed);

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

void Player::move(f32 dtime, Environment *env, f32 pos_max_d)
{
	move(dtime, env, pos_max_d, NULL);
}
void Player::move(f32 dtime, Environment *env, f32 pos_max_d,
		std::vector<CollisionInfo> *collision_info)
{
	if (!collision_info || collision_info->empty()) {
		// Node below the feet, update each Environment::step()
		m_standing_node = floatToInt(m_position, BS) - v3s16(0, 1, 0);
	}

	// drop the old move code?

	Map *map = &env->getMap();
	// TODO: check for NULL?
	const NodeDefManager *nodemgr = getNodeDefManager();

	v3f position = getPosition();
	//debugVec("Starting move with", position);

	// Copy parent position if player is attached
	if (isAttached()) {
		_handleAttachedMove();
		return;
	}

	// Skip collision detection if noclip mode is used
	bool fly_allowed = checkPrivilege("fly");
	bool noclip = checkPrivilege("noclip") &&
		g_settings->getBool("noclip");
	bool free_move = g_settings->getBool("free_move") && fly_allowed;

	if (noclip && free_move) {
		position += m_speed * dtime;
		setPosition(position);
		return;
	}

	// Need an INodeDefManager for all following things
	if (!nodemgr) {
		return;
	}

	/*
		Collision detection
	*/

	bool is_valid_position;
	MapNode node;
	v3s16 pp;

	/*
		Check if player is in liquid (the oscillating value)
	*/

	// If in liquid, the threshold of coming out is at higher y
	if (in_liquid)
	{
		pp = floatToInt(position + v3f(0,BS*0.1,0), BS);
		node = map->getNodeNoEx(pp, &is_valid_position);
		if (is_valid_position) {
			in_liquid = nodemgr->get(node.getContent()).isLiquid();
			liquid_viscosity = nodemgr->get(node.getContent()).liquid_viscosity;
		} else {
			in_liquid = false;
		}
	}
	// If not in liquid, the threshold of going in is at lower y
	else
	{
		pp = floatToInt(position + v3f(0,BS*0.5,0), BS);
		node = map->getNodeNoEx(pp, &is_valid_position);
		if (is_valid_position) {
			in_liquid = nodemgr->get(node.getContent()).isLiquid();
			liquid_viscosity = nodemgr->get(node.getContent()).liquid_viscosity;
		} else {
			in_liquid = false;
		}
	}

	/*
		Check if player is in liquid (the stable value)
	*/
	pp = floatToInt(position + v3f(0,0,0), BS);
	node = map->getNodeNoEx(pp, &is_valid_position);
	if (is_valid_position) {
		in_liquid_stable = nodemgr->get(node.getContent()).isLiquid();
	} else {
		in_liquid_stable = false;
	}

	/*
	        Check if player is climbing
	*/


	pp = floatToInt(position + v3f(0,0.5*BS,0), BS);
	v3s16 pp2 = floatToInt(position + v3f(0,-0.2*BS,0), BS);
	node = map->getNodeNoEx(pp, &is_valid_position);
	bool is_valid_position2;
	MapNode node2 = map->getNodeNoEx(pp2, &is_valid_position2);

	if (!(is_valid_position && is_valid_position2)) {
		is_climbing = false;
	} else {
		is_climbing = (nodemgr->get(node.getContent()).climbable
				|| nodemgr->get(node2.getContent()).climbable) && !free_move;
	}

	/*
		Collision uncertainty radius
		Make it a bit larger than the maximum distance of movement
	*/
	// TODO: Where is this actually used?!?!?
	//f32 d = pos_max_d * 1.1;
	// A fairly large value in here makes moving smoother
	//f32 d = 0.15*BS;

	// This should always apply, otherwise there are glitches
	//dstream << "sanity check: " << d << " > " << pos_max_d << std::endl;
	//sanity_check(d > pos_max_d);

	float player_stepheight = _getStepHeight();

	// TODO: here was where android players get a larger step height
	//       to compensate for a missing autojump.

	v3f accel_f = v3f(0,0,0);

	// TODO: check if it is ok to use env->getGameDef, or if we
	//       really need to pass m_client/m_server somehow
	//debugVec("Before collision", position);
	//debugVec("Before collision speed", m_speed);
	collisionMoveResult result = collisionMoveSimple(env, env->getGameDef(),
		pos_max_d, m_collisionbox, player_stepheight, dtime,
		&position, &m_speed, accel_f);
	//debugVec("After collision", position);
	//debugVec("After collision speed", m_speed);

	bool could_sneak = control.sneak && !free_move && !in_liquid &&
		!is_climbing && physics_override_sneak;

	// Add new collisions to the vector
	if (collision_info && !free_move) {
		v3f diff = intToFloat(m_standing_node, BS) - position;
		f32 distance = diff.getLength();
		// Force update each ClientEnvironment::step()
		bool is_first = collision_info->empty();

		for (const auto &colinfo : result.collisions) {
			collision_info->push_back(colinfo);

			if (colinfo.type != COLLISION_NODE ||
					colinfo.new_speed.Y != 0 ||
					(could_sneak && m_sneak_node_exists))
				continue;

			diff = intToFloat(colinfo.node_p, BS) - position;

			// Find nearest colliding node
			f32 len = diff.getLength();
			if (is_first || len < distance) {
				m_standing_node = colinfo.node_p;
				distance = len;
			}
		}
	}

	/*
		If the player's feet touch the topside of any node, this is
		set to true.

		Player is allowed to jump when this is true.
	*/
	bool touching_ground_was = touching_ground;
	touching_ground = result.touching_ground;
	bool sneak_can_jump = false;

	// Max. distance (X, Z) over border for sneaking determined by collision box
	// * 0.49 to keep the center just barely on the node
	v3f sneak_max = m_collisionbox.getExtent() * 0.49;


	if (m_sneak_ladder_detected) {
		// restore legacy behaviour (this makes the m_speed.Y hack necessary)
		sneak_max = v3f(0.4 * BS, 0, 0.4 * BS);
	}

	/*
		If sneaking, keep on top of last walked node and don't fall off
	*/
	if (could_sneak && m_sneak_node_exists) {
		//debugStr("Sneaking", "enabled");
		const v3f sn_f = intToFloat(m_sneak_node, BS);
		const v3f bmin = sn_f + m_sneak_node_bb_top.MinEdge;
		const v3f bmax = sn_f + m_sneak_node_bb_top.MaxEdge;
		const v3f old_pos = position;
		const v3f old_speed = m_speed;
		f32 y_diff = bmax.Y - position.Y;
		m_standing_node = m_sneak_node;

		// (BS * 0.6f) is the basic stepheight while standing on ground
		if (y_diff < BS * 0.6f) {
			// Only center player when they're on the node
			position.X = rangelim(position.X,
				bmin.X - sneak_max.X, bmax.X + sneak_max.X);
			position.Z = rangelim(position.Z,
				bmin.Z - sneak_max.Z, bmax.Z + sneak_max.Z);

			if (position.X != old_pos.X)
				m_speed.X = 0;
			if (position.Z != old_pos.Z)
				m_speed.Z = 0;
		}

		if (y_diff > 0 && m_speed.Y < 0 &&
				(physics_override_sneak_glitch || y_diff < BS * 0.6f)) {
			// Move player to the maximal height when falling or when
			// the ledge is climbed on the next step.
			position.Y = bmax.Y;
			m_speed.Y = 0;
		}

		// Allow jumping on node edges while sneaking
		if (m_speed.Y == 0 || m_sneak_ladder_detected)
			sneak_can_jump = true;

		if (collision_info &&
				m_speed.Y - old_speed.Y > BS) {
			// Collide with sneak node, report fall damage
			CollisionInfo sn_info;
			sn_info.node_p = m_sneak_node;
			sn_info.old_speed = old_speed;
			sn_info.new_speed = m_speed;
			collision_info->push_back(sn_info);
		}
	}


	/*
		Find the next sneak node if necessary
	*/
	bool new_sneak_node_exists = false;

	if (could_sneak)
		new_sneak_node_exists = updateSneakNode(map, position, sneak_max);

	/*
		Set new position but keep sneak node set
	*/
	//debugVec("Setting position", position);
	setPosition(position);
	m_sneak_node_exists = new_sneak_node_exists;

	/*
		Report collisions
	*/

	if(!result.standing_on_object && !touching_ground_was && touching_ground) {
		reportRegainGround();
	}

	calculateCameraInCeiling(map, nodemgr);

	/*
		Check properties of the node on which the player is standing
	*/
	const ContentFeatures &f = nodemgr->get(map->getNodeNoEx(m_standing_node));
	// Determine if jumping is possible
	m_can_jump = (touching_ground && !in_liquid && !is_climbing)
			|| sneak_can_jump;
	if (itemgroup_get(f.groups, "disable_jump"))
		m_can_jump = false;

	// SERVER SIDE MOVEMENT: this depends on the player's control!
	// -> need to pass a ControlLogEntry?
	// or rather: why is this here and not in applyControl?

	// Jump key pressed while jumping off from a bouncy block
	if (m_can_jump && control.jump && itemgroup_get(f.groups, "bouncy") &&
		m_speed.Y >= -0.5 * BS) {
		float jumpspeed = movement_speed_jump * physics_override_jump;
		if (m_speed.Y > 1) {
			// Reduce boost when speed already is high
			m_speed.Y += jumpspeed / (1 + (m_speed.Y / 16 ));
		} else {
			m_speed.Y += jumpspeed;
		}
		setSpeed(m_speed);
		m_can_jump = false;
	}

}

bool Player::updateSneakNode(Map *map, const v3f &position,
		const v3f &sneak_max)
{
	static const v3s16 dir9_center[9] = {
		v3s16( 0, 0,  0),
		v3s16( 1, 0,  0),
		v3s16(-1, 0,  0),
		v3s16( 0, 0,  1),
		v3s16( 0, 0, -1),
		v3s16( 1, 0,  1),
		v3s16(-1, 0,  1),
		v3s16( 1, 0, -1),
		v3s16(-1, 0, -1)
	};

	const NodeDefManager *nodemgr = getNodeDefManager();
	MapNode node;
	bool is_valid_position;
	bool new_sneak_node_exists = m_sneak_node_exists;

	// We want the top of the sneak node to be below the players feet
	f32 position_y_mod = 0.05 * BS;
	if (m_sneak_node_exists)
		position_y_mod = m_sneak_node_bb_top.MaxEdge.Y - position_y_mod;

	// Get position of current standing node
	const v3s16 current_node = floatToInt(position - v3f(0, position_y_mod, 0), BS);

	if (current_node != m_sneak_node) {
		new_sneak_node_exists = false;
	} else {
		node = map->getNodeNoEx(current_node, &is_valid_position);
		if (!is_valid_position || !nodemgr->get(node).walkable)
			new_sneak_node_exists = false;
	}

	// Keep old sneak node
	if (new_sneak_node_exists)
		return true;

	// Get new sneak node
	m_sneak_ladder_detected = false;
	f32 min_distance_f = 100000.0 * BS;

	for (const auto &d : dir9_center) {
		const v3s16 p = current_node + d;
		const v3f pf = intToFloat(p, BS);
		const v2f diff(position.X - pf.X, position.Z - pf.Z);
		f32 distance_f = diff.getLength();

		if (distance_f > min_distance_f ||
				fabs(diff.X) > (.5 + .1) * BS + sneak_max.X ||
				fabs(diff.Y) > (.5 + .1) * BS + sneak_max.Z)
			continue;


		// The node to be sneaked on has to be walkable
		node = map->getNodeNoEx(p, &is_valid_position);
		if (!is_valid_position || !nodemgr->get(node).walkable)
			continue;
		// And the node(s) above have to be nonwalkable
		bool ok = true;
		if (!physics_override_sneak_glitch) {
			u16 height = ceilf(
					(m_collisionbox.MaxEdge.Y - m_collisionbox.MinEdge.Y) / BS
			);
			for (u16 y = 1; y <= height; y++) {
				node = map->getNodeNoEx(p + v3s16(0, y, 0), &is_valid_position);
				if (!is_valid_position || nodemgr->get(node).walkable) {
					ok = false;
					break;
				}
			}
		} else {
			// legacy behaviour: check just one node
			node = map->getNodeNoEx(p + v3s16(0, 1, 0), &is_valid_position);
			ok = is_valid_position && !nodemgr->get(node).walkable;
		}
		if (!ok)
			continue;

		min_distance_f = distance_f;
		m_sneak_node = p;
		new_sneak_node_exists = true;
	}

	if (!new_sneak_node_exists)
		return false;

	// Update saved top bounding box of sneak node
	node = map->getNodeNoEx(m_sneak_node);
	std::vector<aabb3f> nodeboxes;
	node.getCollisionBoxes(nodemgr, &nodeboxes);
	m_sneak_node_bb_top = getNodeBoundingBox(nodeboxes);

	if (physics_override_sneak_glitch) {
		// Detect sneak ladder:
		// Node two meters above sneak node must be solid
		node = map->getNodeNoEx(m_sneak_node + v3s16(0, 2, 0),
			&is_valid_position);
		if (is_valid_position && nodemgr->get(node).walkable) {
			// Node three meters above: must be non-solid
			node = map->getNodeNoEx(m_sneak_node + v3s16(0, 3, 0),
				&is_valid_position);
			m_sneak_ladder_detected = is_valid_position &&
				!nodemgr->get(node).walkable;
		}
	}
	return true;
}

static aabb3f getNodeBoundingBox(const std::vector<aabb3f> &nodeboxes)
{
	if (nodeboxes.empty())
		return aabb3f(0, 0, 0, 0, 0, 0);

	aabb3f b_max;

	std::vector<aabb3f>::const_iterator it = nodeboxes.begin();
	b_max = aabb3f(it->MinEdge, it->MaxEdge);

	++it;
	for (; it != nodeboxes.end(); ++it)
		b_max.addInternalBox(*it);

	return b_max;
}

void Player::debugVec(const std::string &title, const v3f &v, const std::string &prefix) const {
	dstream << prefix << title << ": <" << v.X << " " << v.Y << " " << v.Z << ">" << std::endl;
}
void Player::debugStr(const std::string &str, bool newline, const std::string &prefix) const {
	dstream << prefix << str;
	if (newline) dstream << std::endl;
}
