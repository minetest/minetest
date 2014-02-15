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

#include "clouds.h"
#include "noise.h"
#include "constants.h"
#include "debug.h"
#include "main.h" // For g_profiler and g_settings
#include "profiler.h"
#include "settings.h"

Clouds::Clouds(
		scene::ISceneNode* parent,
		scene::ISceneManager* mgr,
		s32 id,
		u32 seed,
		s16 cloudheight
):
	scene::ISceneNode(parent, mgr, id),
	m_seed(seed),
	m_camera_pos(0,0),
	m_time(0),
	m_camera_offset(0,0,0)
{
	m_material.setFlag(video::EMF_LIGHTING, false);
	//m_material.setFlag(video::EMF_BACK_FACE_CULLING, false);
	m_material.setFlag(video::EMF_BACK_FACE_CULLING, true);
	m_material.setFlag(video::EMF_BILINEAR_FILTER, false);
	m_material.setFlag(video::EMF_FOG_ENABLE, true);
	m_material.setFlag(video::EMF_ANTI_ALIASING, true);
	//m_material.MaterialType = video::EMT_TRANSPARENT_VERTEX_ALPHA;
	m_material.MaterialType = video::EMT_TRANSPARENT_ALPHA_CHANNEL;

	m_cloud_y = BS * (cloudheight ? cloudheight :
				g_settings->getS16("cloud_height"));

	m_box = core::aabbox3d<f32>(-BS*1000000,m_cloud_y-BS,-BS*1000000,
			BS*1000000,m_cloud_y+BS,BS*1000000);

}

Clouds::~Clouds()
{
}

void Clouds::OnRegisterSceneNode()
{
	if(IsVisible)
	{
		SceneManager->registerNodeForRendering(this, scene::ESNRP_TRANSPARENT);
		//SceneManager->registerNodeForRendering(this, scene::ESNRP_SOLID);
	}

	ISceneNode::OnRegisterSceneNode();
}

#define MYROUND(x) (x > 0.0 ? (int)x : (int)x - 1)

void Clouds::render()
{
	video::IVideoDriver* driver = SceneManager->getVideoDriver();

	if(SceneManager->getSceneNodeRenderPass() != scene::ESNRP_TRANSPARENT)
	//if(SceneManager->getSceneNodeRenderPass() != scene::ESNRP_SOLID)
		return;

	ScopeProfiler sp(g_profiler, "Rendering of clouds, avg", SPT_AVG);
	
	bool enable_3d = g_settings->getBool("enable_3d_clouds");
	int num_faces_to_draw = enable_3d ? 6 : 1;
	
	m_material.setFlag(video::EMF_BACK_FACE_CULLING, enable_3d);

	driver->setTransform(video::ETS_WORLD, AbsoluteTransformation);
	driver->setMaterial(m_material);
	
	/*
		Clouds move from X+ towards X-
	*/

	const s16 cloud_radius_i = 12;
	const float cloud_size = BS*64;
	const v2f cloud_speed(0, -BS*2);
	
	const float cloud_full_radius = cloud_size * cloud_radius_i;
	
	// Position of cloud noise origin in world coordinates
	v2f world_cloud_origin_pos_f = m_time*cloud_speed;
	// Position of cloud noise origin from the camera
	v2f cloud_origin_from_camera_f = world_cloud_origin_pos_f - m_camera_pos;
	// The center point of drawing in the noise
	v2f center_of_drawing_in_noise_f = -cloud_origin_from_camera_f;
	// The integer center point of drawing in the noise
	v2s16 center_of_drawing_in_noise_i(
		MYROUND(center_of_drawing_in_noise_f.X / cloud_size),
		MYROUND(center_of_drawing_in_noise_f.Y / cloud_size)
	);
	// The world position of the integer center point of drawing in the noise
	v2f world_center_of_drawing_in_noise_f = v2f(
		center_of_drawing_in_noise_i.X * cloud_size,
		center_of_drawing_in_noise_i.Y * cloud_size
	) + world_cloud_origin_pos_f;

	/*video::SColor c_top(128,b*240,b*240,b*255);
	video::SColor c_side_1(128,b*230,b*230,b*255);
	video::SColor c_side_2(128,b*220,b*220,b*245);
	video::SColor c_bottom(128,b*205,b*205,b*230);*/
	video::SColorf c_top_f(m_color);
	video::SColorf c_side_1_f(m_color);
	video::SColorf c_side_2_f(m_color);
	video::SColorf c_bottom_f(m_color);
	c_side_1_f.r *= 0.95;
	c_side_1_f.g *= 0.95;
	c_side_1_f.b *= 0.95;
	c_side_2_f.r *= 0.90;
	c_side_2_f.g *= 0.90;
	c_side_2_f.b *= 0.90;
	c_bottom_f.r *= 0.80;
	c_bottom_f.g *= 0.80;
	c_bottom_f.b *= 0.80;
	c_top_f.a = 0.9;
	c_side_1_f.a = 0.9;
	c_side_2_f.a = 0.9;
	c_bottom_f.a = 0.9;
	video::SColor c_top = c_top_f.toSColor();
	video::SColor c_side_1 = c_side_1_f.toSColor();
	video::SColor c_side_2 = c_side_2_f.toSColor();
	video::SColor c_bottom = c_bottom_f.toSColor();

	// Get fog parameters for setting them back later
	video::SColor fog_color(0,0,0,0);
	video::E_FOG_TYPE fog_type = video::EFT_FOG_LINEAR;
	f32 fog_start = 0;
	f32 fog_end = 0;
	f32 fog_density = 0;
	bool fog_pixelfog = false;
	bool fog_rangefog = false;
	driver->getFog(fog_color, fog_type, fog_start, fog_end, fog_density,
			fog_pixelfog, fog_rangefog);
	
	// Set our own fog
	driver->setFog(fog_color, fog_type, cloud_full_radius * 0.5,
			cloud_full_radius*1.2, fog_density, fog_pixelfog, fog_rangefog);

	// Read noise

	bool *grid = new bool[cloud_radius_i*2*cloud_radius_i*2];

	for(s16 zi=-cloud_radius_i; zi<cloud_radius_i; zi++)
	for(s16 xi=-cloud_radius_i; xi<cloud_radius_i; xi++)
	{
		u32 i = (zi+cloud_radius_i)*cloud_radius_i*2 + xi+cloud_radius_i;

		v2s16 p_in_noise_i(
			xi+center_of_drawing_in_noise_i.X,
			zi+center_of_drawing_in_noise_i.Y
		);

#if 0
		double noise = noise2d_perlin_abs(
				(float)p_in_noise_i.X*cloud_size/BS/200,
				(float)p_in_noise_i.Y*cloud_size/BS/200,
				m_seed, 3, 0.4);
		grid[i] = (noise >= 0.80);
#endif
#if 1
		double noise = noise2d_perlin(
				(float)p_in_noise_i.X*cloud_size/BS/200,
				(float)p_in_noise_i.Y*cloud_size/BS/200,
				m_seed, 3, 0.5);
		grid[i] = (noise >= 0.4);
#endif
	}

#define GETINDEX(x, z, radius) (((z)+(radius))*(radius)*2 + (x)+(radius))
#define CONTAINS(x, z, radius) \
	((x) >= -(radius) && (x) < (radius) && (z) >= -(radius) && (z) < (radius))

	for(s16 zi0=-cloud_radius_i; zi0<cloud_radius_i; zi0++)
	for(s16 xi0=-cloud_radius_i; xi0<cloud_radius_i; xi0++)
	{
		s16 zi = zi0;
		s16 xi = xi0;
		// Draw from front to back (needed for transparency)
		/*if(zi <= 0)
			zi = -cloud_radius_i - zi;
		if(xi <= 0)
			xi = -cloud_radius_i - xi;*/
		// Draw from back to front
		if(zi >= 0)
			zi = cloud_radius_i - zi - 1;
		if(xi >= 0)
			xi = cloud_radius_i - xi - 1;

		u32 i = GETINDEX(xi, zi, cloud_radius_i);

		if(grid[i] == false)
			continue;

		v2f p0 = v2f(xi,zi)*cloud_size + world_center_of_drawing_in_noise_f;

		video::S3DVertex v[4] = {
			video::S3DVertex(0,0,0, 0,0,0, c_top, 0, 1),
			video::S3DVertex(0,0,0, 0,0,0, c_top, 1, 1),
			video::S3DVertex(0,0,0, 0,0,0, c_top, 1, 0),
			video::S3DVertex(0,0,0, 0,0,0, c_top, 0, 0)
		};

		/*if(zi <= 0 && xi <= 0){
			v[0].Color.setBlue(255);
			v[1].Color.setBlue(255);
			v[2].Color.setBlue(255);
			v[3].Color.setBlue(255);
		}*/

		f32 rx = cloud_size/2;
		f32 ry = 8*BS;
		f32 rz = cloud_size/2;

		for(int i=0; i<num_faces_to_draw; i++)
		{
			switch(i)
			{
			case 0:	// top
				for(int j=0;j<4;j++){
					v[j].Normal.set(0,1,0);
				}
				v[0].Pos.set(-rx, ry,-rz);
				v[1].Pos.set(-rx, ry, rz);
				v[2].Pos.set( rx, ry, rz);
				v[3].Pos.set( rx, ry,-rz);
				break;
			case 1: // back
				if(CONTAINS(xi, zi-1, cloud_radius_i)){
					u32 j = GETINDEX(xi, zi-1, cloud_radius_i);
					if(grid[j])
						continue;
				}
				for(int j=0;j<4;j++){
					v[j].Color = c_side_1;
					v[j].Normal.set(0,0,-1);
				}
				v[0].Pos.set(-rx, ry,-rz);
				v[1].Pos.set( rx, ry,-rz);
				v[2].Pos.set( rx,-ry,-rz);
				v[3].Pos.set(-rx,-ry,-rz);
				break;
			case 2: //right
				if(CONTAINS(xi+1, zi, cloud_radius_i)){
					u32 j = GETINDEX(xi+1, zi, cloud_radius_i);
					if(grid[j])
						continue;
				}
				for(int j=0;j<4;j++){
					v[j].Color = c_side_2;
					v[j].Normal.set(1,0,0);
				}
				v[0].Pos.set( rx, ry,-rz);
				v[1].Pos.set( rx, ry, rz);
				v[2].Pos.set( rx,-ry, rz);
				v[3].Pos.set( rx,-ry,-rz);
				break;
			case 3: // front
				if(CONTAINS(xi, zi+1, cloud_radius_i)){
					u32 j = GETINDEX(xi, zi+1, cloud_radius_i);
					if(grid[j])
						continue;
				}
				for(int j=0;j<4;j++){
					v[j].Color = c_side_1;
					v[j].Normal.set(0,0,-1);
				}
				v[0].Pos.set( rx, ry, rz);
				v[1].Pos.set(-rx, ry, rz);
				v[2].Pos.set(-rx,-ry, rz);
				v[3].Pos.set( rx,-ry, rz);
				break;
			case 4: // left
				if(CONTAINS(xi-1, zi, cloud_radius_i)){
					u32 j = GETINDEX(xi-1, zi, cloud_radius_i);
					if(grid[j])
						continue;
				}
				for(int j=0;j<4;j++){
					v[j].Color = c_side_2;
					v[j].Normal.set(-1,0,0);
				}
				v[0].Pos.set(-rx, ry, rz);
				v[1].Pos.set(-rx, ry,-rz);
				v[2].Pos.set(-rx,-ry,-rz);
				v[3].Pos.set(-rx,-ry, rz);
				break;
			case 5: // bottom
				for(int j=0;j<4;j++){
					v[j].Color = c_bottom;
					v[j].Normal.set(0,-1,0);
				}
				v[0].Pos.set( rx,-ry, rz);
				v[1].Pos.set(-rx,-ry, rz);
				v[2].Pos.set(-rx,-ry,-rz);
				v[3].Pos.set( rx,-ry,-rz);
				break;
			}

			v3f pos(p0.X, m_cloud_y, p0.Y);
			pos -= intToFloat(m_camera_offset, BS);

			for(u16 i=0; i<4; i++)
				v[i].Pos += pos;
			u16 indices[] = {0,1,2,2,3,0};
			driver->drawVertexPrimitiveList(v, 4, indices, 2,
					video::EVT_STANDARD, scene::EPT_TRIANGLES, video::EIT_16BIT);
		}
	}

	delete[] grid;
	
	// Restore fog settings
	driver->setFog(fog_color, fog_type, fog_start, fog_end, fog_density,
			fog_pixelfog, fog_rangefog);
}

void Clouds::step(float dtime)
{
	m_time += dtime;
}

void Clouds::update(v2f camera_p, video::SColorf color)
{
	m_camera_pos = camera_p;
	m_color = color;
	//m_brightness = brightness;
	//dstream<<"m_brightness="<<m_brightness<<std::endl;
}

