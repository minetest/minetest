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

#ifndef CLOUDS_HEADER
#define CLOUDS_HEADER

#include "irrlichttypes_extrabloated.h"
#include <iostream>
#include "constants.h"

// Menu clouds
class Clouds;
extern Clouds *g_menuclouds;

// Scene manager used for menu clouds
namespace irr{namespace scene{class ISceneManager;}}
extern irr::scene::ISceneManager *g_menucloudsmgr;

class Clouds : public scene::ISceneNode
{
public:
	Clouds(
			scene::ISceneNode* parent,
			scene::ISceneManager* mgr,
			s32 id,
			u32 seed,
			s16 cloudheight=0
	);

	~Clouds();

	/*
		ISceneNode methods
	*/

	virtual void OnRegisterSceneNode();

	virtual void render();
	
	virtual const aabb3f &getBoundingBox() const
	{
		return m_box;
	}

	virtual u32 getMaterialCount() const
	{
		return 1;
	}

	virtual video::SMaterial& getMaterial(u32 i)
	{
		return m_material;
	}
	
	/*
		Other stuff
	*/

	void step(float dtime);

	void update(v2f camera_p, video::SColorf color);
	
	void updateCameraOffset(v3s16 camera_offset)
	{
		m_camera_offset = camera_offset;
		m_box = aabb3f(-BS * 1000000, m_cloud_y - BS - BS * camera_offset.Y, -BS * 1000000,
			BS * 1000000, m_cloud_y + BS - BS * camera_offset.Y, BS * 1000000);
	}

	void readSettings();

private:
	video::SMaterial m_material;
	aabb3f m_box;
	s16 m_passed_cloud_y;
	float m_cloud_y;
	u16 m_cloud_radius_i;
	bool m_enable_3d;
	video::SColorf m_color;
	u32 m_seed;
	v2f m_camera_pos;
	float m_time;
	v3s16 m_camera_offset;
};



#endif

