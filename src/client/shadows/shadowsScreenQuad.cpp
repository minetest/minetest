/*
Minetest
Copyright (C) 2021 Liso <anlismon@gmail.com>

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

#include "shadowsScreenQuad.h"

shadowScreenQuad::shadowScreenQuad()
{
	Material.Wireframe = false;
	Material.Lighting = false;

	video::SColor color(0x0);
	Vertices[0] = video::S3DVertex(
			-1.0f, -1.0f, 0.0f, 0, 0, 1, color, 0.0f, 1.0f);
	Vertices[1] = video::S3DVertex(
			-1.0f, 1.0f, 0.0f, 0, 0, 1, color, 0.0f, 0.0f);
	Vertices[2] = video::S3DVertex(
			1.0f, 1.0f, 0.0f, 0, 0, 1, color, 1.0f, 0.0f);
	Vertices[3] = video::S3DVertex(
			1.0f, -1.0f, 0.0f, 0, 0, 1, color, 1.0f, 1.0f);
	Vertices[4] = video::S3DVertex(
			-1.0f, -1.0f, 0.0f, 0, 0, 1, color, 0.0f, 1.0f);
	Vertices[5] = video::S3DVertex(
			1.0f, 1.0f, 0.0f, 0, 0, 1, color, 1.0f, 0.0f);
}

void shadowScreenQuad::render(video::IVideoDriver *driver)
{
	u16 indices[6] = {0, 1, 2, 3, 4, 5};
	driver->setMaterial(Material);
	driver->setTransform(video::ETS_WORLD, core::matrix4());
	driver->drawIndexedTriangleList(&Vertices[0], 6, &indices[0], 2);
}

void shadowScreenQuadCB::OnSetConstants(
		video::IMaterialRendererServices *services, s32 userData)
{
	s32 TextureId = 0;
	m_sm_client_map_setting.set(&TextureId, services);

	TextureId = 1;
	m_sm_client_map_trans_setting.set(&TextureId, services);

	TextureId = 2;
	m_sm_dynamic_sampler_setting.set(&TextureId, services);
}
