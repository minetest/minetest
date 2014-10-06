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

#include "main.h" // For g_settings
#include "event.h"
#include "collision.h"
#include "gamedef.h"
#include "nodedef.h"
#include "settings.h"
#include "environment.h"
#include "map.h"
#include "util/numeric.h"

/*
	LocalPlayer
*/

LocalPlayer::LocalPlayer(IGameDef *gamedef, const char *name):
	Player(gamedef, name),
	parent(0),
	isAttached(false),
	overridePosition(v3f(0,0,0)),
	last_position(v3f(0,0,0)),
	last_speed(v3f(0,0,0)),
	last_pitch(0),
	last_yaw(0),
	last_keyPressed(0),
	eye_offset_first(v3f(0,0,0)),
	eye_offset_third(v3f(0,0,0)),
	last_animation(NO_ANIM),
	hotbar_image(""),
	hotbar_selected_image(""),
	m_sneak_node(32767,32767,32767),
	m_sneak_node_exists(false),
	m_old_node_below(32767,32767,32767),
	m_old_node_below_type("air"),
	m_need_to_get_new_sneak_node(true),
	m_can_jump(false),
	m_cao(NULL)
{
	// Initialize hp to 0, so that no hearts will be shown if server
	// doesn't support health points
	hp = 0;
}

LocalPlayer::~LocalPlayer()
{
}

void LocalPlayer::move(f32 dtime, Environment *env, f32 pos_max_d,
		std::list<CollisionInfo> *collision_info)
{
	Map *map = &env->getMap();
	INodeDefManager *nodemgr = m_gamedef->ndef();

	v3f position = getPosition();

	v3f old_speed = m_speed;

	// Copy parent position if local player is attached
	if(isAttached)
	{
		setPosition(overridePosition);
		m_sneak_node_exists = false;
		return;
	}

	// Skip collision detection if noclip mode is used
	bool fly_allowed = m_gamedef->checkLocalPrivilege("fly");
	bool noclip = m_gamedef->checkLocalPrivilege("noclip") &&
		g_settings->getBool("noclip");
	bool free_move = noclip && fly_allowed && g_settings->getBool("free_move");
	if(free_move)
	{
        position += m_speed * dtime;
		setPosition(position);
		m_sneak_node_exists = false;
		return;
	}

	/*
		Collision detection
	*/
	
	/*
		Check if player is in liquid (the oscillating value)
	*/
	try{
		// If in liquid, the threshold of coming out is at higher y
		if(in_liquid)
		{
			v3s16 pp = floatToInt(position + v3f(0,BS*0.1,0), BS);
			in_liquid = nodemgr->get(map->getNode(pp).getContent()).isLiquid();
			liquid_viscosity = nodemgr->get(map->getNode(pp).getContent()).liquid_viscosity;
		}
		// If not in liquid, the threshold of going in is at lower y
		else
		{
			v3s16 pp = floatToInt(position + v3f(0,BS*0.5,0), BS);
			in_liquid = nodemgr->get(map->getNode(pp).getContent()).isLiquid();
			liquid_viscosity = nodemgr->get(map->getNode(pp).getContent()).liquid_viscosity;
		}
	}
	catch(InvalidPositionException &e)
	{
		in_liquid = false;
	}

	/*
		Check if player is in liquid (the stable value)
	*/
	try{
		v3s16 pp = floatToInt(position + v3f(0,0,0), BS);
		in_liquid_stable = nodemgr->get(map->getNode(pp).getContent()).isLiquid();
	}
	catch(InvalidPositionException &e)
	{
		in_liquid_stable = false;
	}

	/*
	        Check if player is climbing
	*/

	try {
		v3s16 pp = floatToInt(position + v3f(0,0.5*BS,0), BS);
		v3s16 pp2 = floatToInt(position + v3f(0,-0.2*BS,0), BS);
		is_climbing = ((nodemgr->get(map->getNode(pp).getContent()).climbable ||
		nodemgr->get(map->getNode(pp2).getContent()).climbable) && !free_move);
	}
	catch(InvalidPositionException &e)
	{
		is_climbing = false;
	}

	/*
		Collision uncertainty radius
		Make it a bit larger than the maximum distance of movement
	*/
	//f32 d = pos_max_d * 1.1;
	// A fairly large value in here makes moving smoother
	f32 d = 0.15*BS;

	// This should always apply, otherwise there are glitches
	assert(d > pos_max_d);

	// Maximum distance over border for sneaking
	f32 sneak_max = BS*0.4;

	/*
		If sneaking, keep in range from the last walked node and don't
		fall off from it
	*/
	if(control.sneak && m_sneak_node_exists &&
			!(fly_allowed && g_settings->getBool("free_move")) && !in_liquid &&
			physics_override_sneak)
	{
		f32 maxd = 0.5*BS + sneak_max;
		v3f lwn_f = intToFloat(m_sneak_node, BS);
		position.X = rangelim(position.X, lwn_f.X-maxd, lwn_f.X+maxd);
		position.Z = rangelim(position.Z, lwn_f.Z-maxd, lwn_f.Z+maxd);
		
		if(!is_climbing)
		{
			f32 min_y = lwn_f.Y + 0.5*BS;
			if(position.Y < min_y)
			{
				position.Y = min_y;

				if(m_speed.Y < 0)
					m_speed.Y = 0;
			}
		}
	}

	float player_stepheight = touching_ground ? (BS*0.6) : (BS*0.2);

	v3f accel_f = v3f(0,0,0);

	collisionMoveResult result = collisionMoveSimple(env, m_gamedef,
			pos_max_d, m_collisionbox, player_stepheight, dtime,
			position, m_speed, accel_f);

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
	v3s16 current_node = floatToInt(position - v3f(0,BS/2,0), BS);
	if(m_sneak_node_exists &&
	   nodemgr->get(map->getNodeNoEx(m_old_node_below)).name == "air" &&
	   m_old_node_below_type != "air")
	{
		// Old node appears to have been removed; that is,
		// it wasn't air before but now it is
		m_need_to_get_new_sneak_node = false;
		m_sneak_node_exists = false;
	}
	else if(nodemgr->get(map->getNodeNoEx(current_node)).name != "air")
	{
		// We are on something, so make sure to recalculate the sneak
		// node.
		m_need_to_get_new_sneak_node = true;
	}
	if(m_need_to_get_new_sneak_node && physics_override_sneak)
	{
		v3s16 pos_i_bottom = floatToInt(position - v3f(0,BS/2,0), BS);
		v2f player_p2df(position.X, position.Z);
		f32 min_distance_f = 100000.0*BS;
		// If already seeking from some node, compare to it.
		/*if(m_sneak_node_exists)
		{
			v3f sneaknode_pf = intToFloat(m_sneak_node, BS);
			v2f sneaknode_p2df(sneaknode_pf.X, sneaknode_pf.Z);
			f32 d_horiz_f = player_p2df.getDistanceFrom(sneaknode_p2df);
			f32 d_vert_f = fabs(sneaknode_pf.Y + BS*0.5 - position.Y);
			// Ignore if player is not on the same level (likely dropped)
			if(d_vert_f < 0.15*BS)
				min_distance_f = d_horiz_f;
		}*/
		v3s16 new_sneak_node = m_sneak_node;
		for(s16 x=-1; x<=1; x++)
		for(s16 z=-1; z<=1; z++)
		{
			v3s16 p = pos_i_bottom + v3s16(x,0,z);
			v3f pf = intToFloat(p, BS);
			v2f node_p2df(pf.X, pf.Z);
			f32 distance_f = player_p2df.getDistanceFrom(node_p2df);
			f32 max_axis_distance_f = MYMAX(
					fabs(player_p2df.X-node_p2df.X),
					fabs(player_p2df.Y-node_p2df.Y));
					
			if(distance_f > min_distance_f ||
					max_axis_distance_f > 0.5*BS + sneak_max + 0.1*BS)
				continue;

			try{
				// The node to be sneaked on has to be walkable
				if(nodemgr->get(map->getNode(p)).walkable == false)
					continue;
				// And the node above it has to be nonwalkable
				if(nodemgr->get(map->getNode(p+v3s16(0,1,0))).walkable == true) {
					continue;
				}
				if (!physics_override_sneak_glitch) {
					if (nodemgr->get(map->getNode(p+v3s16(0,2,0))).walkable)
						continue;
				}
			}
			catch(InvalidPositionException &e)
			{
				continue;
			}

			min_distance_f = distance_f;
			new_sneak_node = p;
		}
		
		bool sneak_node_found = (min_distance_f < 100000.0*BS*0.9);

		m_sneak_node = new_sneak_node;
		m_sneak_node_exists = sneak_node_found;

		/*
			If sneaking, the player's collision box can be in air, so
			this has to be set explicitly
		*/
		if(sneak_node_found && control.sneak)
			touching_ground = true;
	}
	
	/*
		Set new position
	*/
	setPosition(position);
	
	/*
		Report collisions
	*/
	bool bouncy_jump = false;
	// Dont report if flying
	if(collision_info && !(g_settings->getBool("free_move") && fly_allowed))
	{
		for(size_t i=0; i<result.collisions.size(); i++){
			const CollisionInfo &info = result.collisions[i];
			collision_info->push_back(info);
			if(info.new_speed.Y - info.old_speed.Y > 0.1*BS &&
					info.bouncy)
				bouncy_jump = true;
		}
	}

	if(bouncy_jump && control.jump){
		m_speed.Y += movement_speed_jump*BS;
		touching_ground = false;
		MtEvent *e = new SimpleTriggerEvent("PlayerJump");
		m_gamedef->event()->put(e);
	}

	if(!touching_ground_was && touching_ground){
		MtEvent *e = new SimpleTriggerEvent("PlayerRegainGround");
		m_gamedef->event()->put(e);

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
	if(itemgroup_get(f.groups, "disable_jump"))
		m_can_jump = false;
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
	
	bool fly_allowed = m_gamedef->checkLocalPrivilege("fly");
	bool fast_allowed = m_gamedef->checkLocalPrivilege("fast");

	bool free_move = fly_allowed && g_settings->getBool("free_move");
	bool fast_move = fast_allowed && g_settings->getBool("fast_move");
	// When aux1_descends is enabled the fast key is used to go down, so fast isn't possible
	bool fast_climb = fast_move && control.aux1 && !g_settings->getBool("aux1_descends");
	bool continuous_forward = g_settings->getBool("continuous_forward");

	// Whether superspeed mode is used or not
	bool superspeed = false;
	
	if(g_settings->getBool("always_fly_fast") && free_move && fast_move)
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
				if(fast_move && (control.aux1 || g_settings->getBool("always_fly_fast")))
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

	if(continuous_forward)
		speedH += move_direction;

	if(control.up)
	{
		if(continuous_forward)
			superspeed = true;
		else
			speedH += move_direction;
	}
	if(control.down)
	{
		speedH -= move_direction;
	}
	if(control.left)
	{
		speedH += move_direction.crossProduct(v3f(0,1,0));
	}
	if(control.right)
	{
		speedH += move_direction.crossProduct(v3f(0,-1,0));
	}
	if(control.jump)
	{
		if(free_move)
		{
			if(g_settings->getBool("aux1_descends") || g_settings->getBool("always_fly_fast"))
			{
				if(fast_move)
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
			if(speedJ.Y >= -0.5 * BS)
			{
				speedJ.Y = movement_speed_jump * physics_override_jump;
				setSpeed(speedJ);
				
				MtEvent *e = new SimpleTriggerEvent("PlayerJump");
				m_gamedef->event()->put(e);
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

