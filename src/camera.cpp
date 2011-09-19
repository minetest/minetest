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

#include "camera.h"
#include "debug.h"
#include "client.h"
#include "main.h" // for g_settings
#include "map.h"
#include "player.h"
#include <cmath>

const s32 BOBFRAMES = 0x1000000; // must be a power of two

Camera::Camera(scene::ISceneManager* smgr, MapDrawControl& draw_control):
	m_smgr(smgr),
	m_playernode(NULL),
	m_headnode(NULL),
	m_cameranode(NULL),
	m_draw_control(draw_control),
	m_viewing_range_min(5.0),
	m_viewing_range_max(5.0),

	m_camera_position(0,0,0),
	m_camera_direction(0,0,0),

	m_aspect(1.0),
	m_fov_x(1.0),
	m_fov_y(1.0),

	m_wanted_frametime(0.0),
	m_added_frametime(0),
	m_added_frames(0),
	m_range_old(0),
	m_frametime_old(0),
	m_frametime_counter(0),
	m_time_per_range(30. / 50), // a sane default of 30ms per 50 nodes of range

	m_view_bobbing_anim(0),
	m_view_bobbing_state(0),
	m_view_bobbing_speed(0)
{
	//dstream<<__FUNCTION_NAME<<std::endl;

	// note: making the camera node a child of the player node
	// would lead to unexpected behaviour, so we don't do that.
	m_playernode = smgr->addEmptySceneNode(smgr->getRootSceneNode());
	m_headnode = smgr->addEmptySceneNode(m_playernode);
	m_cameranode = smgr->addCameraSceneNode(smgr->getRootSceneNode());
	m_cameranode->bindTargetAndRotation(true);
	m_wieldnode = new ExtrudedSpriteSceneNode(smgr->getRootSceneNode(), smgr, -1, v3f(0, 120, 10), v3f(0, 0, 0), v3f(100, 100, 100));
	//m_wieldnode = new ExtrudedSpriteSceneNode(smgr->getRootSceneNode(), smgr, -1);

	updateSettings();
}

Camera::~Camera()
{
}

bool Camera::successfullyCreated(std::wstring& error_message)
{
	if (m_playernode == NULL)
	{
		error_message = L"Failed to create the player scene node";
		return false;
	}
	if (m_headnode == NULL)
	{
		error_message = L"Failed to create the head scene node";
		return false;
	}
	if (m_cameranode == NULL)
	{
		error_message = L"Failed to create the camera scene node";
		return false;
	}
	return true;
}

void Camera::step(f32 dtime)
{
	if (m_view_bobbing_state != 0)
	{
		s32 offset = MYMAX(dtime * m_view_bobbing_speed * 0.035 * BOBFRAMES, 1);
		if (m_view_bobbing_state == 2)
		{
			// Animation is getting turned off
			s32 subanim = (m_view_bobbing_anim & (BOBFRAMES/2-1));
			if (subanim < BOBFRAMES/4)
				offset = -1 *  MYMIN(offset, subanim);
			else
				offset = MYMIN(offset, BOBFRAMES/2 - subanim);
		}
		m_view_bobbing_anim = (m_view_bobbing_anim + offset) & (BOBFRAMES-1);
	}
}

void Camera::update(LocalPlayer* player, f32 frametime, v2u32 screensize)
{
	// Set player node transformation
	m_playernode->setPosition(player->getPosition());
	m_playernode->setRotation(v3f(0, -1 * player->getYaw(), 0));
	m_playernode->updateAbsolutePosition();

	// Set head node transformation
	v3f eye_offset = player->getEyePosition() - player->getPosition();
	m_headnode->setPosition(eye_offset);
	m_headnode->setRotation(v3f(player->getPitch(), 0, 0));
	m_headnode->updateAbsolutePosition();

	// Compute relative camera position and target
	v3f rel_cam_pos = v3f(0,0,0);
	v3f rel_cam_target = v3f(0,0,1);
	v3f rel_cam_up = v3f(0,1,0);

	s32 bobframe = m_view_bobbing_anim & (BOBFRAMES/2-1);
	if (bobframe != 0)
	{
		f32 bobfrac = (f32) bobframe / (BOBFRAMES/2);
		f32 bobdir = (m_view_bobbing_anim < (BOBFRAMES/2)) ? 1.0 : -1.0;

		#if 1
		f32 bobknob = 1.2;
		f32 bobtmp = sin(pow(bobfrac, bobknob) * PI);

		v3f bobvec = v3f(
			bobdir * sin(bobfrac * PI),
			0.8 * bobtmp * bobtmp,
			0.);

		rel_cam_pos += 0.03 * bobvec;
		rel_cam_target += 0.045 * bobvec;
		rel_cam_up.rotateXYBy(0.03 * bobdir * bobtmp * PI);
		#else
		f32 angle_deg = 1 * bobdir * sin(bobfrac * PI);
		f32 angle_rad = angle_deg * PI / 180;
		f32 r = 0.05;
		v3f off = v3f(
			r * sin(angle_rad),
			r * (cos(angle_rad) - 1),
			0);
		rel_cam_pos += off;
		//rel_cam_target += off;
		rel_cam_up.rotateXYBy(angle_deg);
		#endif

	}

	// Compute absolute camera position and target
	m_headnode->getAbsoluteTransformation().transformVect(m_camera_position, rel_cam_pos);
	m_headnode->getAbsoluteTransformation().transformVect(m_camera_direction, rel_cam_target);
	m_camera_direction -= m_camera_position;

	v3f abs_cam_up;
	m_headnode->getAbsoluteTransformation().transformVect(abs_cam_up, rel_cam_pos + rel_cam_up);
	abs_cam_up -= m_camera_position;

	// Set camera node transformation
	m_cameranode->setPosition(m_camera_position);
	m_cameranode->setUpVector(abs_cam_up);
	m_cameranode->setTarget(m_camera_position + m_camera_direction);

	// FOV and and aspect ratio
	m_aspect = (f32)screensize.X / (f32) screensize.Y;
	m_fov_x = 2 * atan(0.5 * m_aspect * tan(m_fov_y));
	m_cameranode->setAspectRatio(m_aspect);
	m_cameranode->setFOV(m_fov_y);
	// Just so big a value that everything rendered is visible
	// Some more allowance that m_viewing_range_max * BS because of active objects etc.
	m_cameranode->setFarValue(m_viewing_range_max * BS * 10);

	// Render distance feedback loop
	updateViewingRange(frametime);

	// If the player seems to be walking on solid ground,
	// view bobbing is enabled and free_move is off,
	// start (or continue) the view bobbing animation.
	v3f speed = player->getSpeed();
	if ((hypot(speed.X, speed.Z) > BS) &&
		(player->touching_ground) &&
		(g_settings.getBool("view_bobbing") == true) &&
		(g_settings.getBool("free_move") == false))
	{
		// Start animation
		m_view_bobbing_state = 1;
		m_view_bobbing_speed = MYMIN(speed.getLength(), 40);
	}
	else if (m_view_bobbing_state == 1)
	{
		// Stop animation
		m_view_bobbing_state = 2;
		m_view_bobbing_speed = 100;
	}
	else if (m_view_bobbing_state == 2 && bobframe == 0)
	{
		// Stop animation completed
		m_view_bobbing_state = 0;
	}
}

void Camera::updateViewingRange(f32 frametime_in)
{
	if (m_draw_control.range_all)
		return;

	m_added_frametime += frametime_in;
	m_added_frames += 1;

	// Actually this counter kind of sucks because frametime is busytime
	m_frametime_counter -= frametime_in;
	if (m_frametime_counter > 0)
		return;
	m_frametime_counter = 0.2;

	/*dstream<<__FUNCTION_NAME
			<<": Collected "<<m_added_frames<<" frames, total of "
			<<m_added_frametime<<"s."<<std::endl;

	dstream<<"m_draw_control.blocks_drawn="
			<<m_draw_control.blocks_drawn
			<<", m_draw_control.blocks_would_have_drawn="
			<<m_draw_control.blocks_would_have_drawn
			<<std::endl;*/

	m_draw_control.wanted_min_range = m_viewing_range_min;
	m_draw_control.wanted_max_blocks = (1.5*m_draw_control.blocks_would_have_drawn)+1;
	if (m_draw_control.wanted_max_blocks < 10)
		m_draw_control.wanted_max_blocks = 10;

	f32 block_draw_ratio = 1.0;
	if (m_draw_control.blocks_would_have_drawn != 0)
	{
		block_draw_ratio = (f32)m_draw_control.blocks_drawn
			/ (f32)m_draw_control.blocks_would_have_drawn;
	}

	// Calculate the average frametime in the case that all wanted
	// blocks had been drawn
	f32 frametime = m_added_frametime / m_added_frames / block_draw_ratio;

	m_added_frametime = 0.0;
	m_added_frames = 0;

	f32 wanted_frametime_change = m_wanted_frametime - frametime;
	//dstream<<"wanted_frametime_change="<<wanted_frametime_change<<std::endl;

	// If needed frametime change is small, just return
	if (fabs(wanted_frametime_change) < m_wanted_frametime*0.4)
	{
		//dstream<<"ignoring small wanted_frametime_change"<<std::endl;
		return;
	}

	f32 range = m_draw_control.wanted_range;
	f32 new_range = range;

	f32 d_range = range - m_range_old;
	f32 d_frametime = frametime - m_frametime_old;
	if (d_range != 0)
	{
		m_time_per_range = d_frametime / d_range;
	}

	// The minimum allowed calculated frametime-range derivative:
	// Practically this sets the maximum speed of changing the range.
	// The lower this value, the higher the maximum changing speed.
	// A low value here results in wobbly range (0.001)
	// A high value here results in slow changing range (0.0025)
	// SUGG: This could be dynamically adjusted so that when
	//       the camera is turning, this is lower
	//f32 min_time_per_range = 0.0015;
	f32 min_time_per_range = 0.0010;
	//f32 min_time_per_range = 0.05 / range;
	if(m_time_per_range < min_time_per_range)
	{
		m_time_per_range = min_time_per_range;
		//dstream<<"m_time_per_range="<<m_time_per_range<<" (min)"<<std::endl;
	}
	else
	{
		//dstream<<"m_time_per_range="<<m_time_per_range<<std::endl;
	}

	f32 wanted_range_change = wanted_frametime_change / m_time_per_range;
	// Dampen the change a bit to kill oscillations
	//wanted_range_change *= 0.9;
	//wanted_range_change *= 0.75;
	wanted_range_change *= 0.5;
	//dstream<<"wanted_range_change="<<wanted_range_change<<std::endl;

	// If needed range change is very small, just return
	if(fabs(wanted_range_change) < 0.001)
	{
		//dstream<<"ignoring small wanted_range_change"<<std::endl;
		return;
	}

	new_range += wanted_range_change;
	
	//f32 new_range_unclamped = new_range;
	new_range = MYMAX(new_range, m_viewing_range_min);
	new_range = MYMIN(new_range, m_viewing_range_max);
	/*dstream<<"new_range="<<new_range_unclamped
			<<", clamped to "<<new_range<<std::endl;*/

	m_draw_control.wanted_range = new_range;

	m_range_old = new_range;
	m_frametime_old = frametime;
}

void Camera::updateSettings()
{
	m_viewing_range_min = g_settings.getS16("viewing_range_nodes_min");
	m_viewing_range_min = MYMAX(5.0, m_viewing_range_min);

	m_viewing_range_max = g_settings.getS16("viewing_range_nodes_max");
	m_viewing_range_max = MYMAX(m_viewing_range_min, m_viewing_range_max);

	f32 fov_degrees = g_settings.getFloat("fov");
	fov_degrees = MYMAX(fov_degrees, 10.0);
	fov_degrees = MYMIN(fov_degrees, 170.0);
	m_fov_y = fov_degrees * PI / 180.0;

	f32 wanted_fps = g_settings.getFloat("wanted_fps");
	wanted_fps = MYMAX(wanted_fps, 1.0);
	m_wanted_frametime = 1.0 / wanted_fps;
}

void Camera::wield(InventoryItem* item)
{
	if (item != NULL)
	{
		dstream << "wield item: " << item->getName() << std::endl;
		m_wieldnode->setSprite(item->getImageRaw());
		m_wieldnode->setVisible(true);
	}
	else
	{
		dstream << "wield item: none" << std::endl;
		m_wieldnode->setVisible(false);
	}
}

void Camera::setDigging(bool digging)
{
	// TODO
}


ExtrudedSpriteSceneNode::ExtrudedSpriteSceneNode(
	scene::ISceneNode* parent,
	scene::ISceneManager* mgr,
	s32 id,
	const v3f& position,
	const v3f& rotation,
	const v3f& scale
):
	ISceneNode(parent, mgr, id, position, rotation, scale)
{
	m_meshnode = mgr->addMeshSceneNode(NULL, this, -1, v3f(0,0,0), v3f(0,0,0), v3f(1,1,1), true);
	m_thickness = 0.1;
	m_cubemesh = NULL;
	m_is_cube = false;
}

ExtrudedSpriteSceneNode::~ExtrudedSpriteSceneNode()
{
	removeChild(m_meshnode);
	if (m_cubemesh)
		m_cubemesh->drop();
}

void ExtrudedSpriteSceneNode::setSprite(video::ITexture* texture)
{
	if (texture == NULL)
	{
		m_meshnode->setVisible(false);
		return;
	}

	io::path name = getExtrudedName(texture);
	scene::IMeshCache* cache = SceneManager->getMeshCache();
	scene::IAnimatedMesh* mesh = cache->getMeshByName(name);
	if (mesh != NULL)
	{
		// Extruded texture has been found in cache.
		m_meshnode->setMesh(mesh);
	}
	else
	{
		// Texture was not yet extruded, do it now and save in cache
		mesh = extrude(texture);
		if (mesh == NULL)
		{
			dstream << "Warning: failed to extrude sprite" << std::endl;
			m_meshnode->setVisible(false);
			return;
		}
		cache->addMesh(name, mesh);
		m_meshnode->setMesh(mesh);
		mesh->drop();
	}

	m_meshnode->setScale(v3f(1, 1, m_thickness));
	m_meshnode->getMaterial(0).setTexture(0, texture);
	m_meshnode->getMaterial(0).setFlag(video::EMF_LIGHTING, false);
	m_meshnode->getMaterial(0).setFlag(video::EMF_BILINEAR_FILTER, false);
	m_meshnode->getMaterial(0).MaterialType = video::EMT_TRANSPARENT_ALPHA_CHANNEL_REF;
	m_meshnode->setVisible(true);
	m_is_cube = false;
}

void ExtrudedSpriteSceneNode::setCube(video::ITexture* texture)
{
	if (texture == NULL)
	{
		m_meshnode->setVisible(false);
		return;
	}

	if (m_cubemesh == NULL)
		m_cubemesh = SceneManager->getGeometryCreator()->createCubeMesh(v3f(1));

	m_meshnode->setMesh(m_cubemesh);
	m_meshnode->setScale(v3f(1));
	m_meshnode->getMaterial(0).setTexture(0, texture);
	m_meshnode->getMaterial(0).setFlag(video::EMF_LIGHTING, false);
	m_meshnode->getMaterial(0).setFlag(video::EMF_BILINEAR_FILTER, false);
	m_meshnode->getMaterial(0).MaterialType = video::EMT_TRANSPARENT_ALPHA_CHANNEL_REF;
	m_meshnode->setVisible(true);
	m_is_cube = true;
}

void ExtrudedSpriteSceneNode::removeSpriteFromCache(video::ITexture* texture)
{
	scene::IMeshCache* cache = SceneManager->getMeshCache();
	scene::IAnimatedMesh* mesh = cache->getMeshByName(getExtrudedName(texture));
	if (mesh != NULL)
		cache->removeMesh(mesh);
}

void ExtrudedSpriteSceneNode::setSpriteThickness(f32 thickness)
{
	m_thickness = thickness;
	if (!m_is_cube)
		m_meshnode->setScale(v3f(1, 1, thickness));
}

const core::aabbox3d<f32>& ExtrudedSpriteSceneNode::getBoundingBox() const
{
	return m_meshnode->getBoundingBox();
}

void ExtrudedSpriteSceneNode::OnRegisterSceneNode()
{
	if (IsVisible)
		SceneManager->registerNodeForRendering(this);
	ISceneNode::OnRegisterSceneNode();
}

void ExtrudedSpriteSceneNode::render()
{
	// do nothing
}

io::path ExtrudedSpriteSceneNode::getExtrudedName(video::ITexture* texture)
{
	io::path path = texture->getName();
	path.append("/[extruded]");
	return path;
}

scene::IAnimatedMesh* ExtrudedSpriteSceneNode::extrudeARGB(u32 width, u32 height, u8* data)
{
	const s32 argb_wstep = 4 * width;
	const s32 alpha_threshold = 1;

	scene::IMeshBuffer* buf = new scene::SMeshBuffer();
	video::SColor c(255,255,255,255);

	// Front and back
	{
		video::S3DVertex vertices[8] =
		{
			video::S3DVertex(-0.5,-0.5,-0.5, 0,0,-1, c, 0,1),
			video::S3DVertex(-0.5,0.5,-0.5, 0,0,-1, c, 0,0),
			video::S3DVertex(0.5,0.5,-0.5, 0,0,-1, c, 1,0),
			video::S3DVertex(0.5,-0.5,-0.5, 0,0,-1, c, 1,1),
			video::S3DVertex(0.5,-0.5,0.5, 0,0,1, c, 1,1),
			video::S3DVertex(0.5,0.5,0.5, 0,0,1, c, 1,0),
			video::S3DVertex(-0.5,0.5,0.5, 0,0,1, c, 0,0),
			video::S3DVertex(-0.5,-0.5,0.5, 0,0,1, c, 0,1),
		};
		u16 indices[12] = {0,1,2,2,3,0,4,5,6,6,7,4};
		buf->append(vertices, 8, indices, 12);
	}

	// "Interior"
	// (add faces where a solid pixel is next to a transparent one)
	u8* solidity = new u8[(width+2) * (height+2)];
	u32 wstep = width + 2;
	for (u32 y = 0; y < height + 2; ++y)
	{
		u8* scanline = solidity + y * wstep;
		if (y == 0 || y == height + 1)
		{
			for (u32 x = 0; x < width + 2; ++x)
				scanline[x] = 0;
		}
		else
		{
			scanline[0] = 0;
			u8* argb_scanline = data + (y - 1) * argb_wstep;
			for (u32 x = 0; x < width; ++x)
				scanline[x+1] = (argb_scanline[x*4+3] >= alpha_threshold);
			scanline[width + 1] = 0;
		}
	}

	// without this, there would be occasional "holes" in the mesh
	f32 eps = 0.01;

	for (u32 y = 0; y <= height; ++y)
	{
		u8* scanline = solidity + y * wstep + 1;
		for (u32 x = 0; x <= width; ++x)
		{
			if (scanline[x] && !scanline[x + wstep])
			{
				u32 xx = x + 1;
				while (scanline[xx] && !scanline[xx + wstep])
					++xx;
				f32 vx1 = (x - eps) / (f32) width - 0.5;
				f32 vx2 = (xx + eps) / (f32) width - 0.5;
				f32 vy = 0.5 - (y - eps) / (f32) height;
				f32 tx1 = x / (f32) width;
				f32 tx2 = xx / (f32) width;
				f32 ty = (y - 0.5) / (f32) height;
				video::S3DVertex vertices[8] =
				{
					video::S3DVertex(vx1,vy,-0.5, 0,-1,0, c, tx1,ty),
					video::S3DVertex(vx2,vy,-0.5, 0,-1,0, c, tx2,ty),
					video::S3DVertex(vx2,vy,0.5, 0,-1,0, c, tx2,ty),
					video::S3DVertex(vx1,vy,0.5, 0,-1,0, c, tx1,ty),
				};
				u16 indices[6] = {0,1,2,2,3,0};
				buf->append(vertices, 4, indices, 6);
				x = xx - 1;
			}
			if (!scanline[x] && scanline[x + wstep])
			{
				u32 xx = x + 1;
				while (!scanline[xx] && scanline[xx + wstep])
					++xx;
				f32 vx1 = (x - eps) / (f32) width - 0.5;
				f32 vx2 = (xx + eps) / (f32) width - 0.5;
				f32 vy = 0.5 - (y + eps) / (f32) height;
				f32 tx1 = x / (f32) width;
				f32 tx2 = xx / (f32) width;
				f32 ty = (y + 0.5) / (f32) height;
				video::S3DVertex vertices[8] =
				{
					video::S3DVertex(vx1,vy,-0.5, 0,1,0, c, tx1,ty),
					video::S3DVertex(vx1,vy,0.5, 0,1,0, c, tx1,ty),
					video::S3DVertex(vx2,vy,0.5, 0,1,0, c, tx2,ty),
					video::S3DVertex(vx2,vy,-0.5, 0,1,0, c, tx2,ty),
				};
				u16 indices[6] = {0,1,2,2,3,0};
				buf->append(vertices, 4, indices, 6);
				x = xx - 1;
			}
		}
	}

	for (u32 x = 0; x <= width; ++x)
	{
		u8* scancol = solidity + x + wstep;
		for (u32 y = 0; y <= height; ++y)
		{
			if (scancol[y * wstep] && !scancol[y * wstep + 1])
			{
				u32 yy = y + 1;
				while (scancol[yy * wstep] && !scancol[yy * wstep + 1])
					++yy;
				f32 vx = (x - eps) / (f32) width - 0.5;
				f32 vy1 = 0.5 - (y - eps) / (f32) height;
				f32 vy2 = 0.5 - (yy + eps) / (f32) height;
				f32 tx = (x - 0.5) / (f32) width;
				f32 ty1 = y / (f32) height;
				f32 ty2 = yy / (f32) height;
				video::S3DVertex vertices[8] =
				{
					video::S3DVertex(vx,vy1,-0.5, 1,0,0, c, tx,ty1),
					video::S3DVertex(vx,vy1,0.5, 1,0,0, c, tx,ty1),
					video::S3DVertex(vx,vy2,0.5, 1,0,0, c, tx,ty2),
					video::S3DVertex(vx,vy2,-0.5, 1,0,0, c, tx,ty2),
				};
				u16 indices[6] = {0,1,2,2,3,0};
				buf->append(vertices, 4, indices, 6);
				y = yy - 1;
			}
			if (!scancol[y * wstep] && scancol[y * wstep + 1])
			{
				u32 yy = y + 1;
				while (!scancol[yy * wstep] && scancol[yy * wstep + 1])
					++yy;
				f32 vx = (x + eps) / (f32) width - 0.5;
				f32 vy1 = 0.5 - (y - eps) / (f32) height;
				f32 vy2 = 0.5 - (yy + eps) / (f32) height;
				f32 tx = (x + 0.5) / (f32) width;
				f32 ty1 = y / (f32) height;
				f32 ty2 = yy / (f32) height;
				video::S3DVertex vertices[8] =
				{
					video::S3DVertex(vx,vy1,-0.5, -1,0,0, c, tx,ty1),
					video::S3DVertex(vx,vy2,-0.5, -1,0,0, c, tx,ty2),
					video::S3DVertex(vx,vy2,0.5, -1,0,0, c, tx,ty2),
					video::S3DVertex(vx,vy1,0.5, -1,0,0, c, tx,ty1),
				};
				u16 indices[6] = {0,1,2,2,3,0};
				buf->append(vertices, 4, indices, 6);
				y = yy - 1;
			}
		}
	}

	// Add to mesh
	scene::SMesh* mesh = new scene::SMesh();
	mesh->addMeshBuffer(buf);
	buf->drop();
	mesh->recalculateBoundingBox();
	scene::SAnimatedMesh* anim_mesh = new scene::SAnimatedMesh(mesh);
	mesh->drop();
	return anim_mesh;
}

scene::IAnimatedMesh* ExtrudedSpriteSceneNode::extrude(video::ITexture* texture)
{
	scene::IAnimatedMesh* mesh = NULL;
	core::dimension2d<u32> size = texture->getSize();
	video::ECOLOR_FORMAT format = texture->getColorFormat();
	if (format == video::ECF_A8R8G8B8)
	{
		// Texture is in the correct color format, we can pass it
		// to extrudeARGB right away.
		void* data = texture->lock(true);
		if (data == NULL)
			return NULL;
		mesh = extrudeARGB(size.Width, size.Height, (u8*) data);
		texture->unlock();
	}
	else
	{
		video::IVideoDriver* driver = SceneManager->getVideoDriver();

		video::IImage* img1 = driver->createImageFromData(format, size, texture->lock(true));
		if (img1 == NULL)
			return NULL;

		// img1 is in the texture's color format, convert to 8-bit ARGB
		video::IImage* img2 = driver->createImage(video::ECF_A8R8G8B8, size);
		if (img2 != NULL)
		{
			img1->copyTo(img2);
			img1->drop();

			mesh = extrudeARGB(size.Width, size.Height, (u8*) img2->lock());
			img2->unlock();
			img2->drop();
		}
		img1->drop();
	}
	return mesh;
}

