/*
Minetest-c55
Copyright (C) 2010-2012 celeron55, Perttu Ahola <celeron55@gmail.com>

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
#include "map.h"
#include "util/numeric.h"

/*
	LocalPlayer
*/

LocalPlayer::LocalPlayer(IGameDef *gamedef):
	Player(gamedef),
	m_sneak_node(32767,32767,32767),
	m_sneak_node_exists(false),
	m_old_node_below(32767,32767,32767),
	m_old_node_below_type("air"),
	m_need_to_get_new_sneak_node(true),
	m_can_jump(false)
{
	// Initialize hp to 0, so that no hearts will be shown if server
	// doesn't support health points
	hp = 0;
}

LocalPlayer::~LocalPlayer()
{
}

void LocalPlayer::move(f32 dtime, Map &map, f32 pos_max_d,
		core::list<CollisionInfo> *collision_info)
{
	INodeDefManager *nodemgr = m_gamedef->ndef();

	v3f position = getPosition();

	v3f old_speed = m_speed;

	// Skip collision detection if a special movement mode is used
	bool fly_allowed = m_gamedef->checkLocalPrivilege("fly");
	bool free_move = fly_allowed && g_settings->getBool("free_move");
	if(free_move)
	{
        position += m_speed * dtime;
		setPosition(position);
		return;
	}

	/*
		Collision detection
	*/
	
	/*
		Check if player is in water (the oscillating value)
	*/
	try{
		// If in water, the threshold of coming out is at higher y
		if(in_water)
		{
			v3s16 pp = floatToInt(position + v3f(0,BS*0.1,0), BS);
			in_water = nodemgr->get(map.getNode(pp).getContent()).isLiquid();
		}
		// If not in water, the threshold of going in is at lower y
		else
		{
			v3s16 pp = floatToInt(position + v3f(0,BS*0.5,0), BS);
			in_water = nodemgr->get(map.getNode(pp).getContent()).isLiquid();
		}
	}
	catch(InvalidPositionException &e)
	{
		in_water = false;
	}

	/*
		Check if player is in water (the stable value)
	*/
	try{
		v3s16 pp = floatToInt(position + v3f(0,0,0), BS);
		in_water_stable = nodemgr->get(map.getNode(pp).getContent()).isLiquid();
	}
	catch(InvalidPositionException &e)
	{
		in_water_stable = false;
	}

	/*
	        Check if player is climbing
	*/

	try {
		v3s16 pp = floatToInt(position + v3f(0,0.5*BS,0), BS);
		v3s16 pp2 = floatToInt(position + v3f(0,-0.2*BS,0), BS);
		is_climbing = ((nodemgr->get(map.getNode(pp).getContent()).climbable ||
		nodemgr->get(map.getNode(pp2).getContent()).climbable) && !free_move);
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

	float player_radius = BS*0.30;
	float player_height = BS*1.55;
	
	// Maximum distance over border for sneaking
	f32 sneak_max = BS*0.4;

	/*
		If sneaking, keep in range from the last walked node and don't
		fall off from it
	*/
	if(control.sneak && m_sneak_node_exists)
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

	/*
		Calculate player collision box (new and old)
	*/
	core::aabbox3d<f32> playerbox(
		-player_radius,
		0.0,
		-player_radius,
		player_radius,
		player_height,
		player_radius
	);

	float player_stepheight = touching_ground ? (BS*0.6) : (BS*0.2);

	v3f accel_f = v3f(0,0,0);

	collisionMoveResult result = collisionMoveSimple(&map, m_gamedef,
			pos_max_d, playerbox, player_stepheight, dtime,
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
	   nodemgr->get(map.getNodeNoEx(m_old_node_below)).name == "air" &&
	   m_old_node_below_type != "air")
	{
		// Old node appears to have been removed; that is,
		// it wasn't air before but now it is
		m_need_to_get_new_sneak_node = false;
		m_sneak_node_exists = false;
	}
	else if(nodemgr->get(map.getNodeNoEx(current_node)).name != "air")
	{
		// We are on something, so make sure to recalculate the sneak
		// node.
		m_need_to_get_new_sneak_node = true;
	}
	if(m_need_to_get_new_sneak_node)
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
				if(nodemgr->get(map.getNode(p)).walkable == false)
					continue;
				// And the node above it has to be nonwalkable
				if(nodemgr->get(map.getNode(p+v3s16(0,1,0))).walkable == true)
					continue;
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
	if(collision_info)
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
		m_speed.Y += 6.5*BS;
		touching_ground = false;
		MtEvent *e = new SimpleTriggerEvent("PlayerJump");
		m_gamedef->event()->put(e);
	}

	if(!touching_ground_was && touching_ground){
		MtEvent *e = new SimpleTriggerEvent("PlayerRegainGround");
		m_gamedef->event()->put(e);
	}

	{
		camera_barely_in_ceiling = false;
		v3s16 camera_np = floatToInt(getEyePosition(), BS);
		MapNode n = map.getNodeNoEx(camera_np);
		if(n.getContent() != CONTENT_IGNORE){
			if(nodemgr->get(n).walkable){
				camera_barely_in_ceiling = true;
			}
		}
	}

	/*
		Update the node last under the player
	*/
	m_old_node_below = floatToInt(position - v3f(0,BS/2,0), BS);
	m_old_node_below_type = nodemgr->get(map.getNodeNoEx(m_old_node_below)).name;
	
	/*
		Check properties of the node on which the player is standing
	*/
	const ContentFeatures &f = nodemgr->get(map.getNodeNoEx(getStandingNodePos()));
	// Determine if jumping is possible
	m_can_jump = touching_ground;
	if(itemgroup_get(f.groups, "disable_jump"))
		m_can_jump = false;
}

void LocalPlayer::move(f32 dtime, Map &map, f32 pos_max_d)
{
	move(dtime, map, pos_max_d, NULL);
}

void LocalPlayer::applyControl(float dtime)
{
	// Clear stuff
	swimming_up = false;

	// Random constants
	f32 walk_acceleration = 4.0 * BS;
	f32 walkspeed_max = 4.0 * BS;
	
	setPitch(control.pitch);
	setYaw(control.yaw);
	
	v3f move_direction = v3f(0,0,1);
	move_direction.rotateXZBy(getYaw());
	
	v3f speed = v3f(0,0,0);
	
	bool fly_allowed = m_gamedef->checkLocalPrivilege("fly");
	bool fast_allowed = m_gamedef->checkLocalPrivilege("fast");

	bool free_move = fly_allowed && g_settings->getBool("free_move");
	bool fast_move = fast_allowed && g_settings->getBool("fast_move");
	bool continuous_forward = g_settings->getBool("continuous_forward");

	if(free_move || is_climbing)
	{
		v3f speed = getSpeed();
		speed.Y = 0;
		setSpeed(speed);
	}

	// Whether superspeed mode is used or not
	bool superspeed = false;
	
	// If free movement and fast movement, always move fast
	if(free_move && fast_move)
		superspeed = true;
	
	// Old descend control
	if(g_settings->getBool("aux1_descends"))
	{
		// Auxiliary button 1 (E)
		if(control.aux1)
		{
			if(free_move)
			{
				// In free movement mode, aux1 descends
				v3f speed = getSpeed();
				if(fast_move)
					speed.Y = -20*BS;
				else
					speed.Y = -walkspeed_max;
				setSpeed(speed);
			}
			else if(is_climbing)
			{
					v3f speed = getSpeed();
				speed.Y = -3*BS;
				setSpeed(speed);
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
			if(!free_move && !is_climbing)
			{
				// If not free movement but fast is allowed, aux1 is
				// "Turbo button"
				if(fast_move)
					superspeed = true;
			}
		}

		if(control.sneak)
		{
			if(free_move)
			{
				// In free movement mode, sneak descends
				v3f speed = getSpeed();
				if(fast_move)
					speed.Y = -20*BS;
				else
					speed.Y = -walkspeed_max;
					setSpeed(speed);
			}
			else if(is_climbing)
			{
				v3f speed = getSpeed();
				speed.Y = -3*BS;
				setSpeed(speed);
			}
		}
	}

	if(continuous_forward)
		speed += move_direction;

	if(control.up)
	{
		if(continuous_forward)
			superspeed = true;
		else
			speed += move_direction;
	}
	if(control.down)
	{
		speed -= move_direction;
	}
	if(control.left)
	{
		speed += move_direction.crossProduct(v3f(0,1,0));
	}
	if(control.right)
	{
		speed += move_direction.crossProduct(v3f(0,-1,0));
	}
	if(control.jump)
	{
		if(free_move)
		{
			v3f speed = getSpeed();
			if(fast_move)
				speed.Y = 20*BS;
			else
				speed.Y = walkspeed_max;
			setSpeed(speed);
		}
		else if(m_can_jump)
		{
			/*
				NOTE: The d value in move() affects jump height by
				raising the height at which the jump speed is kept
				at its starting value
			*/
			v3f speed = getSpeed();
			if(speed.Y >= -0.5*BS)
			{
				speed.Y = 6.5*BS;
				setSpeed(speed);
				
				MtEvent *e = new SimpleTriggerEvent("PlayerJump");
				m_gamedef->event()->put(e);
			}
		}
		// Use the oscillating value for getting out of water
		// (so that the player doesn't fly on the surface)
		else if(in_water)
		{
			v3f speed = getSpeed();
			speed.Y = 1.5*BS;
			setSpeed(speed);
			swimming_up = true;
		}
		else if(is_climbing)
		{
	                v3f speed = getSpeed();
			speed.Y = 3*BS;
			setSpeed(speed);
		}
	}

	// The speed of the player (Y is ignored)
	if(superspeed)
		speed = speed.normalize() * walkspeed_max * 5.0;
	else if(control.sneak)
		speed = speed.normalize() * walkspeed_max / 3.0;
	else
		speed = speed.normalize() * walkspeed_max;
	
	f32 inc = walk_acceleration * BS * dtime;
	
	// Faster acceleration if fast and free movement
	if(free_move && fast_move)
		inc = walk_acceleration * BS * dtime * 10;
	
	// Accelerate to target speed with maximum increment
	accelerate(speed, inc);
}

v3s16 LocalPlayer::getStandingNodePos()
{
	if(m_sneak_node_exists)
		return m_sneak_node;
	return floatToInt(getPosition(), BS);
}

