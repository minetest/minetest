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

#include "localplayer.h"

#include "event.h"
#include "collision.h"
#include "nodedef.h"
#include "settings.h"
#include "environment.h"
#include "map.h"
#include "client.h"

/*
	LocalPlayer
*/

LocalPlayer::LocalPlayer(Client *client, const char *name):
	Player(name, client->idef()),
	parent(0),
	hp(PLAYER_MAX_HP),
	got_teleported(false),
	isAttached(false),
	touching_ground(false),
	in_liquid(false),
	in_liquid_stable(false),
	liquid_viscosity(0),
	is_climbing(false),
	swimming_vertical(false),
	// Movement overrides are multipliers and must be 1 by default
	physics_override_speed(1.0f),
	physics_override_jump(1.0f),
	physics_override_gravity(1.0f),
	physics_override_sneak(true),
	physics_override_sneak_glitch(false),
	physics_override_new_move(true),  // Temporary option for old move code
	overridePosition(v3f(0,0,0)),
	last_position(v3f(0,0,0)),
	last_speed(v3f(0,0,0)),
	last_pitch(0),
	last_yaw(0),
	last_keyPressed(0),
	last_camera_fov(0),
	last_wanted_range(0),
	camera_impact(0.f),
	last_animation(NO_ANIM),
	hotbar_image(""),
	hotbar_selected_image(""),
	light_color(255,255,255,255),
	hurt_tilt_timer(0.0f),
	hurt_tilt_strength(0.0f),
	m_position(0,0,0),
	m_sneak_node(32767,32767,32767),
	m_sneak_node_bb_ymax(0),  // To support temporary option for old move code
	m_sneak_node_bb_top(0,0,0,0,0,0),
	m_sneak_node_exists(false),
	m_need_to_get_new_sneak_node(true),
	m_sneak_ladder_detected(false),
	m_ledge_detected(false),
	m_old_node_below(32767,32767,32767),
	m_old_node_below_type("air"),
	m_can_jump(false),
	m_breath(PLAYER_MAX_BREATH),
	m_yaw(0),
	m_pitch(0),
	camera_barely_in_ceiling(false),
	m_collisionbox(-BS * 0.30, 0.0, -BS * 0.30, BS * 0.30, BS * 1.75, BS * 0.30),
	m_cao(NULL),
	m_client(client)
{
	// Initialize hp to 0, so that no hearts will be shown if server
	// doesn't support health points
	hp = 0;
	eye_offset_first = v3f(0,0,0);
	eye_offset_third = v3f(0,0,0);
}

LocalPlayer::~LocalPlayer()
{
}

static aabb3f getTopBoundingBox(const std::vector<aabb3f> &nodeboxes)
{
	aabb3f b_max;
	b_max.reset(-BS, -BS, -BS);
	for (std::vector<aabb3f>::const_iterator it = nodeboxes.begin();
			it != nodeboxes.end(); ++it) {
		aabb3f box = *it;
		if (box.MaxEdge.Y > b_max.MaxEdge.Y)
			b_max = box;
		else if (box.MaxEdge.Y == b_max.MaxEdge.Y)
			b_max.addInternalBox(box);
	}
	return aabb3f(v3f(b_max.MinEdge.X, b_max.MaxEdge.Y, b_max.MinEdge.Z), b_max.MaxEdge);
}

#define GETNODE(map, p3, v2, y, valid) \
	(map)->getNodeNoEx((p3) + v3s16((v2).X, y, (v2).Y), valid)

// pos is the node the player is standing inside(!)
static bool detectSneakLadder(Map *map, INodeDefManager *nodemgr, v3s16 pos)
{
	// Detects a structure known as "sneak ladder" or "sneak elevator"
	// that relies on bugs to provide a fast means of vertical transportation,
	// the bugs have since been fixed but this function remains to keep it working.
	// NOTE: This is just entirely a huge hack and causes way too many problems.
	bool is_valid_position;
	MapNode node;
	// X/Z vectors for 4 neighboring nodes
	static const v2s16 vecs[] = { v2s16(-1, 0), v2s16(1, 0), v2s16(0, -1), v2s16(0, 1) };

	for (u16 i = 0; i < ARRLEN(vecs); i++) {
		const v2s16 vec = vecs[i];

		// walkability of bottom & top node should differ
		node = GETNODE(map, pos, vec, 0, &is_valid_position);
		if (!is_valid_position)
			continue;
		bool w = nodemgr->get(node).walkable;
		node = GETNODE(map, pos, vec, 1, &is_valid_position);
		if (!is_valid_position || w == nodemgr->get(node).walkable)
			continue;

		// check one more node above OR below with corresponding walkability
		node = GETNODE(map, pos, vec, -1, &is_valid_position);
		bool ok = is_valid_position && w != nodemgr->get(node).walkable;
		if (!ok) {
			node = GETNODE(map, pos, vec, 2, &is_valid_position);
			ok = is_valid_position && w == nodemgr->get(node).walkable;
		}

		if (ok)
			return true;
	}

	return false;
}

static bool detectLedge(Map *map, INodeDefManager *nodemgr, v3s16 pos)
{
	bool is_valid_position;
	MapNode node;
	// X/Z vectors for 4 neighboring nodes
	static const v2s16 vecs[] = {v2s16(-1, 0), v2s16(1, 0), v2s16(0, -1), v2s16(0, 1)};

	for (u16 i = 0; i < ARRLEN(vecs); i++) {
		const v2s16 vec = vecs[i];

		node = GETNODE(map, pos, vec, 1, &is_valid_position);
		if (is_valid_position && nodemgr->get(node).walkable) {
			// Ledge exists
			node = GETNODE(map, pos, vec, 2, &is_valid_position);
			if (is_valid_position && !nodemgr->get(node).walkable)
				// Space above ledge exists
				return true;
		}
	}

	return false;
}

#undef GETNODE

void LocalPlayer::move(f32 dtime, Environment *env, f32 pos_max_d,
		std::vector<CollisionInfo> *collision_info)
{
	// Temporary option for old move code
	if (!physics_override_new_move) {
		old_move(dtime, env, pos_max_d, collision_info);
		return;
	}

	Map *map = &env->getMap();
	INodeDefManager *nodemgr = m_client->ndef();

	v3f position = getPosition();

	// Copy parent position if local player is attached
	if(isAttached)
	{
		setPosition(overridePosition);
		m_sneak_node_exists = false;
		return;
	}

	// Skip collision detection if noclip mode is used
	bool fly_allowed = m_client->checkLocalPrivilege("fly");
	bool noclip = m_client->checkLocalPrivilege("noclip") &&
		g_settings->getBool("noclip");
	bool free_move = noclip && fly_allowed && g_settings->getBool("free_move");
	if (free_move) {
		position += m_speed * dtime;
		setPosition(position);
		m_sneak_node_exists = false;
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
	//f32 d = pos_max_d * 1.1;
	// A fairly large value in here makes moving smoother
	f32 d = 0.15*BS;

	// This should always apply, otherwise there are glitches
	sanity_check(d > pos_max_d);

	// Max. distance (X, Z) over border for sneaking determined by collision box
	// * 0.49 to keep the center just barely on the node
	v3f sneak_max = m_collisionbox.getExtent() * 0.49;
	if (m_sneak_ladder_detected) {
		// restore legacy behaviour (this makes the m_speed.Y hack necessary)
		sneak_max = v3f(0.4 * BS, 0, 0.4 * BS);
	}

	/*
		If sneaking, keep in range from the last walked node and don't
		fall off from it
	*/
	if (control.sneak && m_sneak_node_exists &&
			!(fly_allowed && g_settings->getBool("free_move")) &&
			!in_liquid && !is_climbing &&
			physics_override_sneak && !got_teleported) {
		v3f sn_f = intToFloat(m_sneak_node, BS);
		const v3f bmin = m_sneak_node_bb_top.MinEdge;
		const v3f bmax = m_sneak_node_bb_top.MaxEdge;

		position.X = rangelim(position.X,
				sn_f.X+bmin.X - sneak_max.X, sn_f.X+bmax.X + sneak_max.X);
		position.Z = rangelim(position.Z,
				sn_f.Z+bmin.Z - sneak_max.Z, sn_f.Z+bmax.Z + sneak_max.Z);

		// Because we keep the player collision box on the node, limiting
		// position.Y is not necessary but useful to prevent players from
		// being inside a node if sneaking on e.g. the lower part of a stair
		if (!m_sneak_ladder_detected) {
			position.Y = MYMAX(position.Y, sn_f.Y+bmax.Y);
		} else {
			// legacy behaviour that sometimes causes some weird slow sinking
			m_speed.Y = MYMAX(m_speed.Y, 0);
		}
	}

	if (got_teleported)
		got_teleported = false;

	// TODO: this shouldn't be hardcoded but transmitted from server
	float player_stepheight = touching_ground ? (BS*0.6) : (BS*0.2);

#ifdef __ANDROID__
	player_stepheight += (0.6 * BS);
#endif

	v3f accel_f = v3f(0,0,0);

	collisionMoveResult result = collisionMoveSimple(env, m_client,
		pos_max_d, m_collisionbox, player_stepheight, dtime,
		&position, &m_speed, accel_f);

	/*
		If the player's feet touch the topside of any node, this is
		set to true.

		Player is allowed to jump when this is true.
	*/
	bool touching_ground_was = touching_ground;
	touching_ground = result.touching_ground;

	// We want the top of the sneak node to be below the players feet
	f32 position_y_mod;
	if (m_sneak_node_exists)
		position_y_mod = m_sneak_node_bb_top.MaxEdge.Y - 0.05 * BS;
	else
		position_y_mod = (0.5 - 0.05) * BS;
	v3s16 current_node = floatToInt(position - v3f(0, position_y_mod, 0), BS);
	/*
		Check the nodes under the player to see from which node the
		player is sneaking from, if any.  If the node from under
		the player has been removed, the player falls.
	*/
	if (m_sneak_node_exists &&
			nodemgr->get(map->getNodeNoEx(m_old_node_below)).name == "air" &&
			m_old_node_below_type != "air") {
		// Old node appears to have been removed; that is,
		// it wasn't air before but now it is
		m_need_to_get_new_sneak_node = false;
		m_sneak_node_exists = false;
	} else if (nodemgr->get(map->getNodeNoEx(current_node)).name != "air") {
		// We are on something, so make sure to recalculate the sneak
		// node.
		m_need_to_get_new_sneak_node = true;
	}

	if (m_need_to_get_new_sneak_node && physics_override_sneak) {
		v2f player_p2df(position.X, position.Z);
		f32 min_distance_f = 100000.0 * BS;
		v3s16 new_sneak_node = m_sneak_node;
		for(s16 x=-1; x<=1; x++)
		for(s16 z=-1; z<=1; z++)
		{
			v3s16 p = current_node + v3s16(x,0,z);
			v3f pf = intToFloat(p, BS);
			v2f node_p2df(pf.X, pf.Z);
			f32 distance_f = player_p2df.getDistanceFrom(node_p2df);

			if (distance_f > min_distance_f ||
					fabs(player_p2df.X-node_p2df.X) > (.5+.1)*BS + sneak_max.X ||
					fabs(player_p2df.Y-node_p2df.Y) > (.5+.1)*BS + sneak_max.Z)
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
					node = map->getNodeNoEx(p + v3s16(0,y,0), &is_valid_position);
					if (!is_valid_position || nodemgr->get(node).walkable) {
						ok = false;
						break;
					}
				}
			} else {
				// legacy behaviour: check just one node
				node = map->getNodeNoEx(p + v3s16(0,1,0), &is_valid_position);
				ok = is_valid_position && !nodemgr->get(node).walkable;
			}
			if (!ok)
				continue;

			min_distance_f = distance_f;
			new_sneak_node = p;
		}

		bool sneak_node_found = (min_distance_f < 100000.0 * BS);
		m_sneak_node = new_sneak_node;
		m_sneak_node_exists = sneak_node_found;

		if (sneak_node_found) {
			// Update saved top bounding box of sneak node
			MapNode n = map->getNodeNoEx(m_sneak_node);
			std::vector<aabb3f> nodeboxes;
			n.getCollisionBoxes(nodemgr, &nodeboxes);
			m_sneak_node_bb_top = getTopBoundingBox(nodeboxes);

			m_sneak_ladder_detected = physics_override_sneak_glitch &&
					detectSneakLadder(map, nodemgr, floatToInt(position, BS));
		} else {
			m_sneak_ladder_detected = false;
		}
	}

	/*
		If 'sneak glitch' enabled detect ledge for old sneak-jump
		behaviour of climbing onto a ledge 2 nodes up.
	*/
	if (physics_override_sneak_glitch && control.sneak && control.jump)
		m_ledge_detected = detectLedge(map, nodemgr, floatToInt(position, BS));

	/*
		Set new position
	*/
	setPosition(position);

	/*
		Report collisions
	*/

	// Dont report if flying
	if(collision_info && !(g_settings->getBool("free_move") && fly_allowed)) {
		for(size_t i=0; i<result.collisions.size(); i++) {
			const CollisionInfo &info = result.collisions[i];
			collision_info->push_back(info);
		}
	}

	if(!result.standing_on_object && !touching_ground_was && touching_ground) {
		MtEvent *e = new SimpleTriggerEvent("PlayerRegainGround");
		m_client->event()->put(e);

		// Set camera impact value to be used for view bobbing
		camera_impact = getSpeed().Y * -1;
	}

	{
		camera_barely_in_ceiling = false;
		v3s16 camera_np = floatToInt(getEyePosition(), BS);
		MapNode n = map->getNodeNoEx(camera_np);
		if(n.getContent() != CONTENT_IGNORE){
			if(nodemgr->get(n).walkable && nodemgr->get(n).solidness == 2){
				camera_barely_in_ceiling = true;
			}
		}
	}

	/*
		Update the node last under the player
	*/
	m_old_node_below = floatToInt(position - v3f(0,BS/2,0), BS);
	m_old_node_below_type = nodemgr->get(map->getNodeNoEx(m_old_node_below)).name;

	/*
		Check properties of the node on which the player is standing
	*/
	const ContentFeatures &f = nodemgr->get(map->getNodeNoEx(getStandingNodePos()));
	// Determine if jumping is possible
	m_can_jump = touching_ground && !in_liquid;
	if (itemgroup_get(f.groups, "disable_jump"))
		m_can_jump = false;
	else if (control.sneak && m_sneak_ladder_detected && !in_liquid && !is_climbing)
		m_can_jump = true;

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

void LocalPlayer::move(f32 dtime, Environment *env, f32 pos_max_d)
{
	move(dtime, env, pos_max_d, NULL);
}

void LocalPlayer::applyControl(float dtime)
{
	// Clear stuff
	swimming_vertical = false;

	setPitch(control.pitch);
	setYaw(control.yaw);

	// Nullify speed and don't run positioning code if the player is attached
	if(isAttached)
	{
		setSpeed(v3f(0,0,0));
		return;
	}

	v3f move_direction = v3f(0,0,1);
	move_direction.rotateXZBy(getYaw());

	v3f speedH = v3f(0,0,0); // Horizontal (X, Z)
	v3f speedV = v3f(0,0,0); // Vertical (Y)

	bool fly_allowed = m_client->checkLocalPrivilege("fly");
	bool fast_allowed = m_client->checkLocalPrivilege("fast");

	bool free_move = fly_allowed && g_settings->getBool("free_move");
	bool fast_move = fast_allowed && g_settings->getBool("fast_move");
	// When aux1_descends is enabled the fast key is used to go down, so fast isn't possible
	bool fast_climb = fast_move && control.aux1 && !g_settings->getBool("aux1_descends");
	bool continuous_forward = g_settings->getBool("continuous_forward");
	bool always_fly_fast = g_settings->getBool("always_fly_fast");

	// Whether superspeed mode is used or not
	bool superspeed = false;

	if (always_fly_fast && free_move && fast_move)
		superspeed = true;

	// Old descend control
	if(g_settings->getBool("aux1_descends"))
	{
		// If free movement and fast movement, always move fast
		if(free_move && fast_move)
			superspeed = true;

		// Auxiliary button 1 (E)
		if(control.aux1)
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
		if(control.aux1)
		{
			if(!is_climbing)
			{
				// aux1 is "Turbo button"
				if(fast_move)
					superspeed = true;
			}
		}

		if(control.sneak)
		{
			if(free_move)
			{
				// In free movement mode, sneak descends
				if (fast_move && (control.aux1 || always_fly_fast))
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

	if (control.up) {
		if (continuous_forward) {
			if (fast_move)
				superspeed = true;
		} else {
			speedH += move_direction;
		}
	}
	if (control.down) {
		speedH -= move_direction;
	}
	if (!control.up && !control.down) {
		speedH -= move_direction *
			(control.forw_move_joystick_axis / 32767.f);
	}
	if (control.left) {
		speedH += move_direction.crossProduct(v3f(0,1,0));
	}
	if (control.right) {
		speedH += move_direction.crossProduct(v3f(0,-1,0));
	}
	if (!control.left && !control.right) {
		speedH -= move_direction.crossProduct(v3f(0,1,0)) *
			(control.sidew_move_joystick_axis / 32767.f);
	}
	if(control.jump)
	{
		if (free_move) {
			if (g_settings->getBool("aux1_descends") || always_fly_fast) {
				if (fast_move)
					speedV.Y = movement_speed_fast;
				else
					speedV.Y = movement_speed_walk;
			} else {
				if(fast_move && control.aux1)
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
				if (m_ledge_detected) {
					// Limit jump speed to a minimum that allows
					// jumping up onto a ledge 2 nodes up.
					speedJ.Y = movement_speed_jump *
							MYMAX(physics_override_jump, 1.3f);
					setSpeed(speedJ);
					m_ledge_detected = false;
				} else {
					speedJ.Y = movement_speed_jump * physics_override_jump;
					setSpeed(speedJ);
				}

				MtEvent *e = new SimpleTriggerEvent("PlayerJump");
				m_client->event()->put(e);
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
	else if(control.sneak && !free_move && !in_liquid && !in_liquid_stable)
		speedH = speedH.normalize() * movement_speed_crouch;
	else
		speedH = speedH.normalize() * movement_speed_walk;

	// Acceleration increase
	f32 incH = 0; // Horizontal (X, Z)
	f32 incV = 0; // Vertical (Y)
	if((!touching_ground && !free_move && !is_climbing && !in_liquid) || (!free_move && m_can_jump && control.jump))
	{
		// Jumping and falling
		if(superspeed || (fast_move && control.aux1))
			incH = movement_acceleration_fast * BS * dtime;
		else
			incH = movement_acceleration_air * BS * dtime;
		incV = 0; // No vertical acceleration in air
	}
	else if (superspeed || (is_climbing && fast_climb) || ((in_liquid || in_liquid_stable) && fast_climb))
		incH = incV = movement_acceleration_fast * BS * dtime;
	else
		incH = incV = movement_acceleration_default * BS * dtime;

	// Accelerate to target speed with maximum increment
	accelerateHorizontal(speedH * physics_override_speed, incH * physics_override_speed);
	accelerateVertical(speedV * physics_override_speed, incV * physics_override_speed);
}

v3s16 LocalPlayer::getStandingNodePos()
{
	if(m_sneak_node_exists)
		return m_sneak_node;
	return floatToInt(getPosition() - v3f(0, BS, 0), BS);
}

v3s16 LocalPlayer::getFootstepNodePos()
{
	if (touching_ground)
		// BS * 0.05 below the player's feet ensures a 1/16th height
		// nodebox is detected instead of the node below it.
		return floatToInt(getPosition() - v3f(0, BS * 0.05f, 0), BS);
	// A larger distance below is necessary for a footstep sound
	// when landing after a jump or fall. BS * 0.5 ensures water
	// sounds when swimming in 1 node deep water.
	return floatToInt(getPosition() - v3f(0, BS * 0.5f, 0), BS);
}

v3s16 LocalPlayer::getLightPosition() const
{
	return floatToInt(m_position + v3f(0,BS+BS/2,0), BS);
}

v3f LocalPlayer::getEyeOffset() const
{
	float eye_height = camera_barely_in_ceiling ? 1.5f : 1.625f;
	return v3f(0, BS * eye_height, 0);
}

// Horizontal acceleration (X and Z), Y direction is ignored
void LocalPlayer::accelerateHorizontal(const v3f &target_speed, const f32 max_increase)
{
        if (max_increase == 0)
                return;

        v3f d_wanted = target_speed - m_speed;
        d_wanted.Y = 0;
        f32 dl = d_wanted.getLength();
        if (dl > max_increase)
                dl = max_increase;

        v3f d = d_wanted.normalize() * dl;

        m_speed.X += d.X;
        m_speed.Z += d.Z;
}

// Vertical acceleration (Y), X and Z directions are ignored
void LocalPlayer::accelerateVertical(const v3f &target_speed, const f32 max_increase)
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

// Temporary option for old move code
void LocalPlayer::old_move(f32 dtime, Environment *env, f32 pos_max_d,
		std::vector<CollisionInfo> *collision_info)
{
	Map *map = &env->getMap();
	INodeDefManager *nodemgr = m_client->ndef();

	v3f position = getPosition();

	// Copy parent position if local player is attached
	if (isAttached) {
		setPosition(overridePosition);
		m_sneak_node_exists = false;
		return;
	}

	// Skip collision detection if noclip mode is used
	bool fly_allowed = m_client->checkLocalPrivilege("fly");
	bool noclip = m_client->checkLocalPrivilege("noclip") &&
		g_settings->getBool("noclip");
	bool free_move = noclip && fly_allowed && g_settings->getBool("free_move");
	if (free_move) {
		position += m_speed * dtime;
		setPosition(position);
		m_sneak_node_exists = false;
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
	if (in_liquid) {
		// If in liquid, the threshold of coming out is at higher y
		pp = floatToInt(position + v3f(0, BS * 0.1, 0), BS);
		node = map->getNodeNoEx(pp, &is_valid_position);
		if (is_valid_position) {
			in_liquid = nodemgr->get(node.getContent()).isLiquid();
			liquid_viscosity = nodemgr->get(node.getContent()).liquid_viscosity;
		} else {
			in_liquid = false;
		}
	} else {
		// If not in liquid, the threshold of going in is at lower y
		pp = floatToInt(position + v3f(0, BS * 0.5, 0), BS);
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
	pp = floatToInt(position + v3f(0, 0, 0), BS);
	node = map->getNodeNoEx(pp, &is_valid_position);
	if (is_valid_position)
		in_liquid_stable = nodemgr->get(node.getContent()).isLiquid();
	else
		in_liquid_stable = false;

	/*
		Check if player is climbing
	*/
	pp = floatToInt(position + v3f(0, 0.5 * BS, 0), BS);
	v3s16 pp2 = floatToInt(position + v3f(0, -0.2 * BS, 0), BS);
	node = map->getNodeNoEx(pp, &is_valid_position);
	bool is_valid_position2;
	MapNode node2 = map->getNodeNoEx(pp2, &is_valid_position2);

	if (!(is_valid_position && is_valid_position2))
		is_climbing = false;
	else
		is_climbing = (nodemgr->get(node.getContent()).climbable ||
				nodemgr->get(node2.getContent()).climbable) && !free_move;

	/*
		Collision uncertainty radius
		Make it a bit larger than the maximum distance of movement
	*/
	//f32 d = pos_max_d * 1.1;
	// A fairly large value in here makes moving smoother
	f32 d = 0.15 * BS;
	// This should always apply, otherwise there are glitches
	sanity_check(d > pos_max_d);
	// Maximum distance over border for sneaking
	f32 sneak_max = BS * 0.4;

	/*
		If sneaking, keep in range from the last walked node and don't
		fall off from it
	*/
	if (control.sneak && m_sneak_node_exists &&
			!(fly_allowed && g_settings->getBool("free_move")) && !in_liquid &&
			physics_override_sneak && !got_teleported) {
		f32 maxd = 0.5 * BS + sneak_max;
		v3f lwn_f = intToFloat(m_sneak_node, BS);
		position.X = rangelim(position.X, lwn_f.X - maxd, lwn_f.X + maxd);
		position.Z = rangelim(position.Z, lwn_f.Z - maxd, lwn_f.Z + maxd);

		if (!is_climbing) {
			// Move up if necessary
			f32 new_y = (lwn_f.Y - 0.5 * BS) + m_sneak_node_bb_ymax;
			if (position.Y < new_y)
				position.Y = new_y;
			/*
				Collision seems broken, since player is sinking when
				sneaking over the edges of current sneaking_node.
				TODO (when fixed): Set Y-speed only to 0 when position.Y < new_y.
			*/
			if (m_speed.Y < 0)
				m_speed.Y = 0;
		}
	}

	if (got_teleported)
		got_teleported = false;

	// this shouldn't be hardcoded but transmitted from server
	float player_stepheight = touching_ground ? (BS * 0.6) : (BS * 0.2);

#ifdef __ANDROID__
	player_stepheight += (0.6 * BS);
#endif

	v3f accel_f = v3f(0, 0, 0);

	collisionMoveResult result = collisionMoveSimple(env, m_client,
		pos_max_d, m_collisionbox, player_stepheight, dtime,
		&position, &m_speed, accel_f);

	/*
		If the player's feet touch the topside of any node, this is
		set to true.

		Player is allowed to jump when this is true.
	*/
	bool touching_ground_was = touching_ground;
	touching_ground = result.touching_ground;

    //bool standing_on_unloaded = result.standing_on_unloaded;

	/*
		Check the nodes under the player to see from which node the
		player is sneaking from, if any.  If the node from under
		the player has been removed, the player falls.
	*/
	f32 position_y_mod = 0.05 * BS;
	if (m_sneak_node_bb_ymax > 0)
		position_y_mod = m_sneak_node_bb_ymax - position_y_mod;
	v3s16 current_node = floatToInt(position - v3f(0, position_y_mod, 0), BS);
	if (m_sneak_node_exists &&
			nodemgr->get(map->getNodeNoEx(m_old_node_below)).name == "air" &&
			m_old_node_below_type != "air") {
		// Old node appears to have been removed; that is,
		// it wasn't air before but now it is
		m_need_to_get_new_sneak_node = false;
		m_sneak_node_exists = false;
	} else if (nodemgr->get(map->getNodeNoEx(current_node)).name != "air") {
		// We are on something, so make sure to recalculate the sneak
		// node.
		m_need_to_get_new_sneak_node = true;
	}

	if (m_need_to_get_new_sneak_node && physics_override_sneak) {
		m_sneak_node_bb_ymax = 0;
		v3s16 pos_i_bottom = floatToInt(position - v3f(0, position_y_mod, 0), BS);
		v2f player_p2df(position.X, position.Z);
		f32 min_distance_f = 100000.0 * BS;
		// If already seeking from some node, compare to it.
		v3s16 new_sneak_node = m_sneak_node;
		for (s16 x= -1; x <= 1; x++)
		for (s16 z= -1; z <= 1; z++) {
			v3s16 p = pos_i_bottom + v3s16(x, 0, z);
			v3f pf = intToFloat(p, BS);
			v2f node_p2df(pf.X, pf.Z);
			f32 distance_f = player_p2df.getDistanceFrom(node_p2df);
			f32 max_axis_distance_f = MYMAX(
					fabs(player_p2df.X - node_p2df.X),
					fabs(player_p2df.Y - node_p2df.Y));

			if (distance_f > min_distance_f ||
					max_axis_distance_f > 0.5 * BS + sneak_max + 0.1 * BS)
				continue;

			// The node to be sneaked on has to be walkable
			node = map->getNodeNoEx(p, &is_valid_position);
			if (!is_valid_position || nodemgr->get(node).walkable == false)
				continue;
			// And the node above it has to be nonwalkable
			node = map->getNodeNoEx(p + v3s16(0, 1, 0), &is_valid_position);
			if (!is_valid_position || nodemgr->get(node).walkable)
				continue;
			// If not 'sneak_glitch' the node 2 nodes above it has to be nonwalkable
			if (!physics_override_sneak_glitch) {
				node =map->getNodeNoEx(p + v3s16(0, 2, 0), &is_valid_position);
				if (!is_valid_position || nodemgr->get(node).walkable)
					continue;
			}

			min_distance_f = distance_f;
			new_sneak_node = p;
		}

		bool sneak_node_found = (min_distance_f < 100000.0 * BS * 0.9);

		m_sneak_node = new_sneak_node;
		m_sneak_node_exists = sneak_node_found;

		if (sneak_node_found) {
			f32 cb_max = 0;
			MapNode n = map->getNodeNoEx(m_sneak_node);
			std::vector<aabb3f> nodeboxes;
			n.getCollisionBoxes(nodemgr, &nodeboxes);
			for (std::vector<aabb3f>::iterator it = nodeboxes.begin();
					it != nodeboxes.end(); ++it) {
				aabb3f box = *it;
				if (box.MaxEdge.Y > cb_max)
					cb_max = box.MaxEdge.Y;
			}
			m_sneak_node_bb_ymax = cb_max;
		}

		/*
			If sneaking, the player's collision box can be in air, so
			this has to be set explicitly
		*/
		if (sneak_node_found && control.sneak)
			touching_ground = true;
	}

	/*
		Set new position
	*/
	setPosition(position);

	/*
		Report collisions
	*/
	// Dont report if flying
	if (collision_info && !(g_settings->getBool("free_move") && fly_allowed)) {
		for (size_t i = 0; i < result.collisions.size(); i++) {
			const CollisionInfo &info = result.collisions[i];
			collision_info->push_back(info);
		}
	}

	if (!result.standing_on_object && !touching_ground_was && touching_ground) {
		MtEvent *e = new SimpleTriggerEvent("PlayerRegainGround");
		m_client->event()->put(e);
		// Set camera impact value to be used for view bobbing
		camera_impact = getSpeed().Y * -1;
	}

	{
		camera_barely_in_ceiling = false;
		v3s16 camera_np = floatToInt(getEyePosition(), BS);
		MapNode n = map->getNodeNoEx(camera_np);
		if (n.getContent() != CONTENT_IGNORE) {
			if (nodemgr->get(n).walkable && nodemgr->get(n).solidness == 2)
				camera_barely_in_ceiling = true;
		}
	}

	/*
		Update the node last under the player
	*/
	m_old_node_below = floatToInt(position - v3f(0, BS / 2, 0), BS);
	m_old_node_below_type = nodemgr->get(map->getNodeNoEx(m_old_node_below)).name;

	/*
		Check properties of the node on which the player is standing
	*/
	const ContentFeatures &f = nodemgr->get(map->getNodeNoEx(getStandingNodePos()));
	// Determine if jumping is possible
	m_can_jump = touching_ground && !in_liquid;
	if (itemgroup_get(f.groups, "disable_jump"))
		m_can_jump = false;
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
