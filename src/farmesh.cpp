/*
Part of Minetest-c55
Copyright (C) 2011 celeron55, Perttu Ahola <celeron55@gmail.com>

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

/*
	A quick messy implementation of terrain rendering for a long
	distance according to map seed
*/

#include "farmesh.h"
#include "constants.h"
#include "debug.h"
#include "noise.h"
#include "map.h"

FarMesh::FarMesh(
		scene::ISceneNode* parent,
		scene::ISceneManager* mgr,
		s32 id,
		u64 seed
):
	scene::ISceneNode(parent, mgr, id),
	m_seed(seed),
	m_camera_pos(0,0),
	m_time(0)
{
	dstream<<__FUNCTION_NAME<<std::endl;

	m_material.setFlag(video::EMF_LIGHTING, false);
	m_material.setFlag(video::EMF_BACK_FACE_CULLING, true);
	m_material.setFlag(video::EMF_BILINEAR_FILTER, false);
	m_material.setFlag(video::EMF_FOG_ENABLE, false);
	//m_material.setFlag(video::EMF_ANTI_ALIASING, true);
	//m_material.MaterialType = video::EMT_TRANSPARENT_VERTEX_ALPHA;

	m_box = core::aabbox3d<f32>(-BS*1000000,-BS*31000,-BS*1000000,
			BS*1000000,BS*31000,BS*1000000);

}

FarMesh::~FarMesh()
{
	dstream<<__FUNCTION_NAME<<std::endl;
}

void FarMesh::OnRegisterSceneNode()
{
	if(IsVisible)
	{
		//SceneManager->registerNodeForRendering(this, scene::ESNRP_TRANSPARENT);
		//SceneManager->registerNodeForRendering(this, scene::ESNRP_SOLID);
		SceneManager->registerNodeForRendering(this, scene::ESNRP_SKY_BOX);
	}

	ISceneNode::OnRegisterSceneNode();
}

#define MYROUND(x) (x > 0.0 ? (int)x : (int)x - 1)

// Temporary hack
core::map<v2s16, float> g_heights;

float ground_height(u64 seed, v2s16 p2d)
{
	core::map<v2s16, float>::Node *n = g_heights.find(p2d);
	if(n)
		return n->getValue();
	float avg_mud_amount = 4;
	float gh = BS*base_rock_level_2d(seed, p2d) + avg_mud_amount*BS;
	//gh *= 10;
	g_heights[p2d] = gh;
	return gh;
}

void FarMesh::render()
{
	video::IVideoDriver* driver = SceneManager->getVideoDriver();

	/*if(SceneManager->getSceneNodeRenderPass() != scene::ESNRP_TRANSPARENT)
		return;*/
	/*if(SceneManager->getSceneNodeRenderPass() != scene::ESNRP_SOLID)
		return;*/
	if(SceneManager->getSceneNodeRenderPass() != scene::ESNRP_SKY_BOX)
		return;

	driver->setTransform(video::ETS_WORLD, AbsoluteTransformation);
	driver->setMaterial(m_material);
	
	const s16 grid_radius_i = 12;
	const float grid_size = BS*50;
	const v2f grid_speed(-BS*0, 0);
	
	// Position of grid noise origin in world coordinates
	v2f world_grid_origin_pos_f(0,0);
	// Position of grid noise origin from the camera
	v2f grid_origin_from_camera_f = world_grid_origin_pos_f - m_camera_pos;
	// The center point of drawing in the noise
	v2f center_of_drawing_in_noise_f = -grid_origin_from_camera_f;
	// The integer center point of drawing in the noise
	v2s16 center_of_drawing_in_noise_i(
		MYROUND(center_of_drawing_in_noise_f.X / grid_size),
		MYROUND(center_of_drawing_in_noise_f.Y / grid_size)
	);
	// The world position of the integer center point of drawing in the noise
	v2f world_center_of_drawing_in_noise_f = v2f(
		center_of_drawing_in_noise_i.X * grid_size,
		center_of_drawing_in_noise_i.Y * grid_size
	) + world_grid_origin_pos_f;

	for(s16 zi=-grid_radius_i; zi<grid_radius_i; zi++)
	for(s16 xi=-grid_radius_i; xi<grid_radius_i; xi++)
	{
		v2s16 p_in_noise_i(
			xi+center_of_drawing_in_noise_i.X,
			zi+center_of_drawing_in_noise_i.Y
		);

		/*if((p_in_noise_i.X + p_in_noise_i.Y)%2==0)
			continue;*/
		/*if((p_in_noise_i.X/2 + p_in_noise_i.Y/2)%2==0)
			continue;*/

		v2f p0 = v2f(xi,zi)*grid_size + world_center_of_drawing_in_noise_f;
		
		/*double noise[4];
		double d = 100*BS;
		noise[0] = d*noise2d_perlin(
				(float)(p_in_noise_i.X+0)*grid_size/BS/100,
				(float)(p_in_noise_i.Y+0)*grid_size/BS/100,
				m_seed, 3, 0.5);
		
		noise[1] = d*noise2d_perlin(
				(float)(p_in_noise_i.X+0)*grid_size/BS/100,
				(float)(p_in_noise_i.Y+1)*grid_size/BS/100,
				m_seed, 3, 0.5);
		
		noise[2] = d*noise2d_perlin(
				(float)(p_in_noise_i.X+1)*grid_size/BS/100,
				(float)(p_in_noise_i.Y+1)*grid_size/BS/100,
				m_seed, 3, 0.5);
		
		noise[3] = d*noise2d_perlin(
				(float)(p_in_noise_i.X+1)*grid_size/BS/100,
				(float)(p_in_noise_i.Y+0)*grid_size/BS/100,
				m_seed, 3, 0.5);*/
		
		float noise[4];
		noise[0] = ground_height(m_seed, v2s16(
				(p_in_noise_i.X+0)*grid_size/BS,
				(p_in_noise_i.Y+0)*grid_size/BS));
		noise[1] = ground_height(m_seed, v2s16(
				(p_in_noise_i.X+0)*grid_size/BS,
				(p_in_noise_i.Y+1)*grid_size/BS));
		noise[2] = ground_height(m_seed, v2s16(
				(p_in_noise_i.X+1)*grid_size/BS,
				(p_in_noise_i.Y+1)*grid_size/BS));
		noise[3] = ground_height(m_seed, v2s16(
				(p_in_noise_i.X+1)*grid_size/BS,
				(p_in_noise_i.Y+0)*grid_size/BS));

		float h_min = BS*65535;
		float h_max = -BS*65536;
		for(u32 i=0; i<4; i++)
		{
			if(noise[i] < h_min)
				h_min = noise[i];
			if(noise[i] > h_max)
				h_max = noise[i];
		}
		float steepness = (h_max - h_min)/grid_size;
		
		float h_avg = (noise[0]+noise[1]+noise[2]+noise[3])/4.0;
		float light_f = noise[0]+noise[1]-noise[2]-noise[3];
		light_f /= 100;
		if(light_f < -1.0) light_f = -1.0;
		if(light_f > 1.0) light_f = 1.0;
		//light_f += 1.0;
		//light_f /= 2.0;
		
		v2f p1 = p0 + v2f(1,1)*grid_size;

		video::SColor c;
		// Detect water
		if(h_avg < WATER_LEVEL*BS && h_max < (WATER_LEVEL+5)*BS)
		{
			c = video::SColor(128,59,86,146);
			/*// Set to water level
			for(u32 i=0; i<4; i++)
			{
				if(noise[i] < BS*WATER_LEVEL)
					noise[i] = BS*WATER_LEVEL;
			}*/
			light_f = 0;
		}
		else if(steepness > 2.0)
			c = video::SColor(128,128,128,128);
		else
			c = video::SColor(128,107,134,51);
		
		// Set to water level
		for(u32 i=0; i<4; i++)
		{
			if(noise[i] < BS*WATER_LEVEL)
				noise[i] = BS*WATER_LEVEL;
		}

		float b = m_brightness + light_f*0.1*m_brightness;
		if(b < 0) b = 0;
		if(b > 2) b = 2;
		
		c = video::SColor(128, b*c.getRed(), b*c.getGreen(), b*c.getBlue());

		video::S3DVertex vertices[4] =
		{
			video::S3DVertex(p0.X,noise[0],p0.Y, 0,0,0, c, 0,1),
			video::S3DVertex(p0.X,noise[1],p1.Y, 0,0,0, c, 1,1),
			video::S3DVertex(p1.X,noise[2],p1.Y, 0,0,0, c, 1,0),
			video::S3DVertex(p1.X,noise[3],p0.Y, 0,0,0, c, 0,0),
		};
		u16 indices[] = {0,1,2,2,3,0};
		driver->drawVertexPrimitiveList(vertices, 4, indices, 2,
				video::EVT_STANDARD, scene::EPT_TRIANGLES, video::EIT_16BIT);
	}

	driver->clearZBuffer();
}

void FarMesh::step(float dtime)
{
	m_time += dtime;
}

void FarMesh::update(v2f camera_p, float brightness)
{
	m_camera_pos = camera_p;
	m_brightness = brightness;
}

