/*
Minetest-c55
Copyright (C) 2010-2011 celeron55, Perttu Ahola <celeron55@gmail.com>

This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License along
with this program; if not, write to the Free Software Foundation, Inc.,
51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
*/

#include "player.h"
#include "map.h"
#include "connection.h"
#include "constants.h"
#include "utility.h"

// Convert a privileges value into a human-readable string,
// with each component separated by a comma.
std::wstring privsToString(u64 privs)
{
	std::wostringstream os(std::ios_base::binary);
	if(privs & PRIV_BUILD)
		os<<L"build,";
	if(privs & PRIV_TELEPORT)
		os<<L"teleport,";
	if(privs & PRIV_SETTIME)
		os<<L"settime,";
	if(privs & PRIV_PRIVS)
		os<<L"privs,";
	if(os.tellp())
	{
		// Drop the trailing comma. (Why on earth can't
		// you truncate a C++ stream anyway???)
		std::wstring tmp = os.str();
		return tmp.substr(0, tmp.length() -1);
	}
	return os.str();
}

// Converts a comma-seperated list of privilege values into a
// privileges value. The reverse of privsToString(). Returns
// PRIV_INVALID if there is anything wrong with the input.
u64 stringToPrivs(std::wstring str)
{
	u64 privs=0;
	std::vector<std::wstring> pr;
	pr=str_split(str, ',');
	for(std::vector<std::wstring>::iterator i = pr.begin();
		i != pr.end(); ++i)
	{
		if(*i == L"build")
			privs |= PRIV_BUILD;
		else if(*i == L"teleport")
			privs |= PRIV_TELEPORT;
		else if(*i == L"settime")
			privs |= PRIV_SETTIME;
		else if(*i == L"privs")
			privs |= PRIV_PRIVS;
		else
			return PRIV_INVALID;
	}
	return privs;
}


Player::Player():
	touching_ground(false),
	in_water(false),
	in_water_stable(false),
	swimming_up(false),
	craftresult_is_preview(true),
	hp(20),
	privs(PRIV_DEFAULT),
	peer_id(PEER_ID_INEXISTENT),
	m_pitch(0),
	m_yaw(0),
	m_speed(0,0,0),
	m_position(0,0,0)
{
	updateName("<not set>");
	updatePassword("");
	resetInventory();
}

Player::~Player()
{
}

void Player::resetInventory()
{
	inventory.clear();
	inventory.addList("main", PLAYER_INVENTORY_SIZE);
	inventory.addList("craft", 9);
	inventory.addList("craftresult", 1);
}

// Y direction is ignored
void Player::accelerate(v3f target_speed, f32 max_increase)
{
	v3f d_wanted = target_speed - m_speed;
	d_wanted.Y = 0;
	f32 dl_wanted = d_wanted.getLength();
	f32 dl = dl_wanted;
	if(dl > max_increase)
		dl = max_increase;
	
	v3f d = d_wanted.normalize() * dl;

	m_speed.X += d.X;
	m_speed.Z += d.Z;
	//m_speed += d;

#if 0 // old code
	if(m_speed.X < target_speed.X - max_increase)
		m_speed.X += max_increase;
	else if(m_speed.X > target_speed.X + max_increase)
		m_speed.X -= max_increase;
	else if(m_speed.X < target_speed.X)
		m_speed.X = target_speed.X;
	else if(m_speed.X > target_speed.X)
		m_speed.X = target_speed.X;

	if(m_speed.Z < target_speed.Z - max_increase)
		m_speed.Z += max_increase;
	else if(m_speed.Z > target_speed.Z + max_increase)
		m_speed.Z -= max_increase;
	else if(m_speed.Z < target_speed.Z)
		m_speed.Z = target_speed.Z;
	else if(m_speed.Z > target_speed.Z)
		m_speed.Z = target_speed.Z;
#endif
}

void Player::serialize(std::ostream &os)
{
	// Utilize a Settings object for storing values
	Settings args;
	args.setS32("version", 1);
	args.set("name", m_name);
	args.set("password", m_password);
	args.setFloat("pitch", m_pitch);
	args.setFloat("yaw", m_yaw);
	args.setV3F("position", m_position);
	args.setBool("craftresult_is_preview", craftresult_is_preview);
	args.setS32("hp", hp);
	args.setU64("privs", privs);

	args.writeLines(os);

	os<<"PlayerArgsEnd\n";

	inventory.serialize(os);
}

void Player::deSerialize(std::istream &is)
{
	Settings args;
	
	for(;;)
	{
		if(is.eof())
			throw SerializationError
					("Player::deSerialize(): PlayerArgsEnd not found");
		std::string line;
		std::getline(is, line);
		std::string trimmedline = trim(line);
		if(trimmedline == "PlayerArgsEnd")
			break;
		args.parseConfigLine(line);
	}

	//args.getS32("version");
	std::string name = args.get("name");
	updateName(name.c_str());
	std::string password = "";
	if(args.exists("password"))
		password = args.get("password");
	updatePassword(password.c_str());
	m_pitch = args.getFloat("pitch");
	m_yaw = args.getFloat("yaw");
	m_position = args.getV3F("position");
	try{
		craftresult_is_preview = args.getBool("craftresult_is_preview");
	}catch(SettingNotFoundException &e){
		craftresult_is_preview = true;
	}
	try{
		hp = args.getS32("hp");
	}catch(SettingNotFoundException &e){
		hp = 20;
	}
	try{
		std::string sprivs = args.get("privs");
		if(sprivs == "all")
		{
			privs = PRIV_ALL;
		}
		else
		{
			std::istringstream ss(sprivs);
			ss>>privs;
		}
	}catch(SettingNotFoundException &e){
		privs = PRIV_DEFAULT;
	}

	inventory.deSerialize(is);
}

/*
	RemotePlayer
*/

#ifndef SERVER

RemotePlayer::RemotePlayer(
		scene::ISceneNode* parent,
		IrrlichtDevice *device,
		s32 id):
	scene::ISceneNode(parent, (device==NULL)?NULL:device->getSceneManager(), id),
	m_text(NULL)
{
	m_box = core::aabbox3d<f32>(-BS/2,0,-BS/2,BS/2,BS*2,BS/2);

	if(parent != NULL && device != NULL)
	{
		// ISceneNode stores a member called SceneManager
		scene::ISceneManager* mgr = SceneManager;
		video::IVideoDriver* driver = mgr->getVideoDriver();
		gui::IGUIEnvironment* gui = device->getGUIEnvironment();

		// Add a text node for showing the name
		wchar_t wname[1] = {0};
		m_text = mgr->addTextSceneNode(gui->getBuiltInFont(),
				wname, video::SColor(255,255,255,255), this);
		m_text->setPosition(v3f(0, (f32)BS*2.1, 0));

		// Attach a simple mesh to the player for showing an image
		scene::SMesh *mesh = new scene::SMesh();
		{ // Front
		scene::IMeshBuffer *buf = new scene::SMeshBuffer();
		video::SColor c(255,255,255,255);
		video::S3DVertex vertices[4] =
		{
			video::S3DVertex(-BS/2,0,0, 0,0,0, c, 0,1),
			video::S3DVertex(BS/2,0,0, 0,0,0, c, 1,1),
			video::S3DVertex(BS/2,BS*2,0, 0,0,0, c, 1,0),
			video::S3DVertex(-BS/2,BS*2,0, 0,0,0, c, 0,0),
		};
		u16 indices[] = {0,1,2,2,3,0};
		buf->append(vertices, 4, indices, 6);
		// Set material
		buf->getMaterial().setFlag(video::EMF_LIGHTING, false);
		//buf->getMaterial().setFlag(video::EMF_BACK_FACE_CULLING, false);
		buf->getMaterial().setTexture(0, driver->getTexture(getTexturePath("player.png").c_str()));
		buf->getMaterial().setFlag(video::EMF_BILINEAR_FILTER, false);
		buf->getMaterial().setFlag(video::EMF_FOG_ENABLE, true);
		//buf->getMaterial().MaterialType = video::EMT_TRANSPARENT_ALPHA_CHANNEL;
		buf->getMaterial().MaterialType = video::EMT_TRANSPARENT_ALPHA_CHANNEL_REF;
		// Add to mesh
		mesh->addMeshBuffer(buf);
		buf->drop();
		}
		{ // Back
		scene::IMeshBuffer *buf = new scene::SMeshBuffer();
		video::SColor c(255,255,255,255);
		video::S3DVertex vertices[4] =
		{
			video::S3DVertex(BS/2,0,0, 0,0,0, c, 1,1),
			video::S3DVertex(-BS/2,0,0, 0,0,0, c, 0,1),
			video::S3DVertex(-BS/2,BS*2,0, 0,0,0, c, 0,0),
			video::S3DVertex(BS/2,BS*2,0, 0,0,0, c, 1,0),
		};
		u16 indices[] = {0,1,2,2,3,0};
		buf->append(vertices, 4, indices, 6);
		// Set material
		buf->getMaterial().setFlag(video::EMF_LIGHTING, false);
		//buf->getMaterial().setFlag(video::EMF_BACK_FACE_CULLING, false);
		buf->getMaterial().setTexture(0, driver->getTexture(getTexturePath("player_back.png").c_str()));
		buf->getMaterial().setFlag(video::EMF_BILINEAR_FILTER, false);
		buf->getMaterial().setFlag(video::EMF_FOG_ENABLE, true);
		buf->getMaterial().MaterialType = video::EMT_TRANSPARENT_ALPHA_CHANNEL_REF;
		// Add to mesh
		mesh->addMeshBuffer(buf);
		buf->drop();
		}
		m_node = mgr->addMeshSceneNode(mesh, this);
		mesh->drop();
		m_node->setPosition(v3f(0,0,0));
	}
}

RemotePlayer::~RemotePlayer()
{
	if(SceneManager != NULL)
		ISceneNode::remove();
}

void RemotePlayer::updateName(const char *name)
{
	Player::updateName(name);
	if(m_text != NULL)
	{
		wchar_t wname[PLAYERNAME_SIZE];
		mbstowcs(wname, m_name, strlen(m_name)+1);
		m_text->setText(wname);
	}
}

void RemotePlayer::move(f32 dtime, Map &map, f32 pos_max_d)
{
	m_pos_animation_time_counter += dtime;
	m_pos_animation_counter += dtime;
	v3f movevector = m_position - m_oldpos;
	f32 moveratio;
	if(m_pos_animation_time < 0.001)
		moveratio = 1.0;
	else
		moveratio = m_pos_animation_counter / m_pos_animation_time;
	if(moveratio > 1.5)
		moveratio = 1.5;
	m_showpos = m_oldpos + movevector * moveratio;
	
	ISceneNode::setPosition(m_showpos);
}

#endif

#ifndef SERVER
/*
	LocalPlayer
*/

LocalPlayer::LocalPlayer():
	m_sneak_node(32767,32767,32767),
	m_sneak_node_exists(false)
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
	v3f position = getPosition();
	v3f oldpos = position;
	v3s16 oldpos_i = floatToInt(oldpos, BS);

	/*std::cout<<"oldpos_i=("<<oldpos_i.X<<","<<oldpos_i.Y<<","
			<<oldpos_i.Z<<")"<<std::endl;*/

	/*
		Calculate new position
	*/
	position += m_speed * dtime;

	// Skip collision detection if a special movement mode is used
	bool free_move = g_settings.getBool("free_move");
	if(free_move)
	{
		setPosition(position);
		return;
	}

	/*
		Collision detection
	*/
	
	// Player position in nodes
	v3s16 pos_i = floatToInt(position, BS);
	
	/*
		Check if player is in water (the oscillating value)
	*/
	try{
		// If in water, the threshold of coming out is at higher y
		if(in_water)
		{
			v3s16 pp = floatToInt(position + v3f(0,BS*0.1,0), BS);
			in_water = content_liquid(map.getNode(pp).d);
		}
		// If not in water, the threshold of going in is at lower y
		else
		{
			v3s16 pp = floatToInt(position + v3f(0,BS*0.5,0), BS);
			in_water = content_liquid(map.getNode(pp).d);
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
		in_water_stable = content_liquid(map.getNode(pp).d);
	}
	catch(InvalidPositionException &e)
	{
		in_water_stable = false;
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

	float player_radius = BS*0.35;
	float player_height = BS*1.7;
	
	// Maximum distance over border for sneaking
	f32 sneak_max = BS*0.4;

	/*
		If sneaking, player has larger collision radius to keep from
		falling
	*/
	/*if(control.sneak)
		player_radius = sneak_max + d*1.1;*/
	
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
		
		f32 min_y = lwn_f.Y + 0.5*BS;
		if(position.Y < min_y)
		{
			position.Y = min_y;
			if(m_speed.Y < 0)
				m_speed.Y = 0;
		}
	}

	/*
		Calculate player collision box (new and old)
	*/
	core::aabbox3d<f32> playerbox(
		position.X - player_radius,
		position.Y - 0.0,
		position.Z - player_radius,
		position.X + player_radius,
		position.Y + player_height,
		position.Z + player_radius
	);
	core::aabbox3d<f32> playerbox_old(
		oldpos.X - player_radius,
		oldpos.Y - 0.0,
		oldpos.Z - player_radius,
		oldpos.X + player_radius,
		oldpos.Y + player_height,
		oldpos.Z + player_radius
	);

	/*
		If the player's feet touch the topside of any node, this is
		set to true.

		Player is allowed to jump when this is true.
	*/
	touching_ground = false;
	
	/*std::cout<<"Checking collisions for ("
			<<oldpos_i.X<<","<<oldpos_i.Y<<","<<oldpos_i.Z
			<<") -> ("
			<<pos_i.X<<","<<pos_i.Y<<","<<pos_i.Z
			<<"):"<<std::endl;*/
	
	/*
		Go through every node around the player
	*/
	for(s16 y = oldpos_i.Y - 1; y <= oldpos_i.Y + 2; y++)
	for(s16 z = oldpos_i.Z - 1; z <= oldpos_i.Z + 1; z++)
	for(s16 x = oldpos_i.X - 1; x <= oldpos_i.X + 1; x++)
	{
		try{
			// Player collides into walkable nodes
			if(content_walkable(map.getNode(v3s16(x,y,z)).d) == false)
				continue;
		}
		catch(InvalidPositionException &e)
		{
			// Doing nothing here will block the player from
			// walking over map borders
		}

		core::aabbox3d<f32> nodebox = getNodeBox(v3s16(x,y,z), BS);
		
		/*
			See if the player is touching ground.

			Player touches ground if player's minimum Y is near node's
			maximum Y and player's X-Z-area overlaps with the node's
			X-Z-area.

			Use 0.15*BS so that it is easier to get on a node.
		*/
		if(
				//fabs(nodebox.MaxEdge.Y-playerbox.MinEdge.Y) < d
				fabs(nodebox.MaxEdge.Y-playerbox.MinEdge.Y) < 0.15*BS
				&& nodebox.MaxEdge.X-d > playerbox.MinEdge.X
				&& nodebox.MinEdge.X+d < playerbox.MaxEdge.X
				&& nodebox.MaxEdge.Z-d > playerbox.MinEdge.Z
				&& nodebox.MinEdge.Z+d < playerbox.MaxEdge.Z
		){
			touching_ground = true;
		}
		
		// If player doesn't intersect with node, ignore node.
		if(playerbox.intersectsWithBox(nodebox) == false)
			continue;
		
		/*
			Go through every axis
		*/
		v3f dirs[3] = {
			v3f(0,0,1), // back-front
			v3f(0,1,0), // top-bottom
			v3f(1,0,0), // right-left
		};
		for(u16 i=0; i<3; i++)
		{
			/*
				Calculate values along the axis
			*/
			f32 nodemax = nodebox.MaxEdge.dotProduct(dirs[i]);
			f32 nodemin = nodebox.MinEdge.dotProduct(dirs[i]);
			f32 playermax = playerbox.MaxEdge.dotProduct(dirs[i]);
			f32 playermin = playerbox.MinEdge.dotProduct(dirs[i]);
			f32 playermax_old = playerbox_old.MaxEdge.dotProduct(dirs[i]);
			f32 playermin_old = playerbox_old.MinEdge.dotProduct(dirs[i]);
			
			/*
				Check collision for the axis.
				Collision happens when player is going through a surface.
			*/
			/*f32 neg_d = d;
			f32 pos_d = d;
			// Make it easier to get on top of a node
			if(i == 1)
				neg_d = 0.15*BS;
			bool negative_axis_collides =
				(nodemax > playermin && nodemax <= playermin_old + neg_d
					&& m_speed.dotProduct(dirs[i]) < 0);
			bool positive_axis_collides =
				(nodemin < playermax && nodemin >= playermax_old - pos_d
					&& m_speed.dotProduct(dirs[i]) > 0);*/
			bool negative_axis_collides =
				(nodemax > playermin && nodemax <= playermin_old + d
					&& m_speed.dotProduct(dirs[i]) < 0);
			bool positive_axis_collides =
				(nodemin < playermax && nodemin >= playermax_old - d
					&& m_speed.dotProduct(dirs[i]) > 0);
			bool main_axis_collides =
					negative_axis_collides || positive_axis_collides;
			
			/*
				Check overlap of player and node in other axes
			*/
			bool other_axes_overlap = true;
			for(u16 j=0; j<3; j++)
			{
				if(j == i)
					continue;
				f32 nodemax = nodebox.MaxEdge.dotProduct(dirs[j]);
				f32 nodemin = nodebox.MinEdge.dotProduct(dirs[j]);
				f32 playermax = playerbox.MaxEdge.dotProduct(dirs[j]);
				f32 playermin = playerbox.MinEdge.dotProduct(dirs[j]);
				if(!(nodemax - d > playermin && nodemin + d < playermax))
				{
					other_axes_overlap = false;
					break;
				}
			}
			
			/*
				If this is a collision, revert the position in the main
				direction.
			*/
			if(other_axes_overlap && main_axis_collides)
			{
				v3f old_speed = m_speed;

				m_speed -= m_speed.dotProduct(dirs[i]) * dirs[i];
				position -= position.dotProduct(dirs[i]) * dirs[i];
				position += oldpos.dotProduct(dirs[i]) * dirs[i];
				
				if(collision_info)
				{
					// Report fall collision
					if(old_speed.Y < m_speed.Y - 0.1)
					{
						CollisionInfo info;
						info.t = COLLISION_FALL;
						info.speed = m_speed.Y - old_speed.Y;
						collision_info->push_back(info);
					}
				}
			}
		
		}
	} // xyz

	/*
		Check the nodes under the player to see from which node the
		player is sneaking from, if any.
	*/
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
				if(content_walkable(map.getNode(p).d) == false)
					continue;
				// And the node above it has to be nonwalkable
				if(content_walkable(map.getNode(p+v3s16(0,1,0)).d) == true)
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
		
		if(control.sneak && m_sneak_node_exists)
		{
			if(sneak_node_found)
				m_sneak_node = new_sneak_node;
		}
		else
		{
			m_sneak_node = new_sneak_node;
			m_sneak_node_exists = sneak_node_found;
		}

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

	bool free_move = g_settings.getBool("free_move");
	bool fast_move = g_settings.getBool("fast_move");
	bool continuous_forward = g_settings.getBool("continuous_forward");

	if(free_move)
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
		else
		{
			// If not free movement but fast is allowed, aux1 is
			// "Turbo button"
			if(fast_move)
				superspeed = true;
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
		else if(touching_ground)
		{
			v3f speed = getSpeed();
			/*
				NOTE: The d value in move() affects jump height by
				raising the height at which the jump speed is kept
				at its starting value
			*/
			speed.Y = 6.5*BS;
			setSpeed(speed);
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
#endif

