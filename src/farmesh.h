/*
Part of Minetest-c55
Copyright (C) 2010-2011 celeron55, Perttu Ahola <celeron55@gmail.com>

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

#ifndef FARMESH_HEADER
#define FARMESH_HEADER

/*
	A quick messy implementation of terrain rendering for a long
	distance according to map seed
*/

#include "irrlichttypes_extrabloated.h"

#define FARMESH_MATERIAL_COUNT 2

class Client;

class FarMesh : public scene::ISceneNode
{
public:
	FarMesh(
			scene::ISceneNode* parent,
			scene::ISceneManager* mgr,
			s32 id,
			u64 seed,
			Client *client
	);

	~FarMesh();

	/*
		ISceneNode methods
	*/

	virtual void OnRegisterSceneNode();

	virtual void render();
	
	virtual const core::aabbox3d<f32>& getBoundingBox() const
	{
		return m_box;
	}

	virtual u32 getMaterialCount() const;

	virtual video::SMaterial& getMaterial(u32 i);
	
	/*
		Other stuff
	*/

	void step(float dtime);

	void update(v2f camera_p, float brightness, s16 render_range);

private:
	video::SMaterial m_materials[FARMESH_MATERIAL_COUNT];
	core::aabbox3d<f32> m_box;
	float m_cloud_y;
	float m_brightness;
	u64 m_seed;
	v2f m_camera_pos;
	float m_time;
	Client *m_client;
	s16 m_render_range;
};

#endif

