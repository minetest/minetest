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

#include "client/shadows/shadowsshadercallbacks.h"

void ShadowDepthShaderCB::OnSetConstants(
		video::IMaterialRendererServices *services, s32 userData)
{
	video::IVideoDriver *driver = services->getVideoDriver();

	core::matrix4 lightMVP = driver->getTransform(video::ETS_PROJECTION);
	lightMVP *= driver->getTransform(video::ETS_VIEW);

	f32 cam_pos[4];
	lightMVP.transformVect(cam_pos, CameraPos);

	lightMVP *= driver->getTransform(video::ETS_WORLD);

	m_light_mvp_setting.set(lightMVP.pointer(), services);
	m_map_resolution_setting.set(&MapRes, services);
	m_max_far_setting.set(&MaxFar, services);
	s32 TextureId = 0;
	m_color_map_sampler_setting.set(&TextureId, services);
	f32 bias0 = PerspectiveBiasXY;
	m_perspective_bias0.set(&bias0, services);
	f32 bias1 = 1.0f - bias0 + 1e-5f;
	m_perspective_bias1.set(&bias1, services);
	f32 zbias = PerspectiveBiasZ;
	m_perspective_zbias.set(&zbias, services);

	m_cam_pos_setting.set(cam_pos, services);
}
