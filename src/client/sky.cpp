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

#include "sky.h"
#include "IVideoDriver.h"
#include "ISceneManager.h"
#include "ICameraSceneNode.h"
#include "S3DVertex.h"
#include "client/tile.h"
#include "noise.h"  // easeCurve
#include "profiler.h"
#include "util/numeric.h"
#include <cmath>
#include "client/renderingengine.h"
#include "settings.h"
#include "camera.h"  // CameraModes


Sky::Sky(s32 id, ITextureSource *tsrc):
		scene::ISceneNode(RenderingEngine::get_scene_manager()->getRootSceneNode(),
			RenderingEngine::get_scene_manager(), id)
{
	setAutomaticCulling(scene::EAC_OFF);
	m_box.MaxEdge.set(0, 0, 0);
	m_box.MinEdge.set(0, 0, 0);

	// Create material

	video::SMaterial mat;
	mat.Lighting = false;
#ifdef __ANDROID__
	mat.ZBuffer = video::ECFN_DISABLED;
#else
	mat.ZBuffer = video::ECFN_NEVER;
#endif
	mat.ZWriteEnable = false;
	mat.AntiAliasing = 0;
	mat.TextureLayer[0].TextureWrapU = video::ETC_CLAMP_TO_EDGE;
	mat.TextureLayer[0].TextureWrapV = video::ETC_CLAMP_TO_EDGE;
	mat.BackfaceCulling = false;

	m_materials[0] = mat;

	m_materials[1] = mat;
	//m_materials[1].MaterialType = video::EMT_TRANSPARENT_VERTEX_ALPHA;
	m_materials[1].MaterialType = video::EMT_TRANSPARENT_ALPHA_CHANNEL;

	m_materials[2] = mat;
	m_materials[2].setTexture(0, tsrc->getTextureForMesh("sunrisebg.png"));
	m_materials[2].MaterialType = video::EMT_TRANSPARENT_ALPHA_CHANNEL;
	//m_materials[2].MaterialType = video::EMT_TRANSPARENT_ADD_COLOR;

	m_sun_texture = tsrc->isKnownSourceImage("sun.png") ?
		tsrc->getTextureForMesh("sun.png") : NULL;
	m_moon_texture = tsrc->isKnownSourceImage("moon.png") ?
		tsrc->getTextureForMesh("moon.png") : NULL;
	m_sun_tonemap = tsrc->isKnownSourceImage("sun_tonemap.png") ?
		tsrc->getTexture("sun_tonemap.png") : NULL;
	m_moon_tonemap = tsrc->isKnownSourceImage("moon_tonemap.png") ?
		tsrc->getTexture("moon_tonemap.png") : NULL;

	if (m_sun_texture) {
		m_materials[3] = mat;
		m_materials[3].setTexture(0, m_sun_texture);
		m_materials[3].MaterialType = video::EMT_TRANSPARENT_ALPHA_CHANNEL;
		if (m_sun_tonemap)
			m_materials[3].Lighting = true;
	}

	if (m_moon_texture) {
		m_materials[4] = mat;
		m_materials[4].setTexture(0, m_moon_texture);
		m_materials[4].MaterialType = video::EMT_TRANSPARENT_ALPHA_CHANNEL;
		if (m_moon_tonemap)
			m_materials[4].Lighting = true;
	}

	for (v3f &star : m_stars) {
		star = v3f(
			myrand_range(-10000, 10000),
			myrand_range(-10000, 10000),
			myrand_range(-10000, 10000)
		);
		star.normalize();
	}

	m_directional_colored_fog = g_settings->getBool("directional_colored_fog");
}


void Sky::OnRegisterSceneNode()
{
	if (IsVisible)
		SceneManager->registerNodeForRendering(this, scene::ESNRP_SKY_BOX);

	scene::ISceneNode::OnRegisterSceneNode();
}


void Sky::render()
{
	if (!m_visible)
		return;

	video::IVideoDriver* driver = SceneManager->getVideoDriver();
	scene::ICameraSceneNode* camera = SceneManager->getActiveCamera();

	if (!camera || !driver)
		return;

	ScopeProfiler sp(g_profiler, "Sky::render()", SPT_AVG);

	// Draw perspective skybox

	core::matrix4 translate(AbsoluteTransformation);
	translate.setTranslation(camera->getAbsolutePosition());

	// Draw the sky box between the near and far clip plane
	const f32 viewDistance = (camera->getNearValue() + camera->getFarValue()) * 0.5f;
	core::matrix4 scale;
	scale.setScale(core::vector3df(viewDistance, viewDistance, viewDistance));

	driver->setTransform(video::ETS_WORLD, translate * scale);

	if (m_sunlight_seen) {
		float sunsize = 0.07;
		video::SColorf suncolor_f(1, 1, 0, 1);
		//suncolor_f.r = 1;
		//suncolor_f.g = MYMAX(0.3, MYMIN(1.0, 0.7 + m_time_brightness * 0.5));
		//suncolor_f.b = MYMAX(0.0, m_brightness * 0.95);
		video::SColorf suncolor2_f(1, 1, 1, 1);
		// The values below were probably meant to be suncolor2_f instead of a
		// reassignment of suncolor_f. However, the resulting colour was chosen
		// and is our long-running classic colour. So preserve, but comment-out
		// the unnecessary first assignments above.
		suncolor_f.r = 1;
		suncolor_f.g = MYMAX(0.3, MYMIN(1.0, 0.85 + m_time_brightness * 0.5));
		suncolor_f.b = MYMAX(0.0, m_brightness);

		float moonsize = 0.04;
		video::SColorf mooncolor_f(0.50, 0.57, 0.65, 1);
		video::SColorf mooncolor2_f(0.85, 0.875, 0.9, 1);

		float nightlength = 0.415;
		float wn = nightlength / 2;
		float wicked_time_of_day = 0;
		if (m_time_of_day > wn && m_time_of_day < 1.0 - wn)
			wicked_time_of_day = (m_time_of_day - wn) / (1.0 - wn * 2) * 0.5 + 0.25;
		else if (m_time_of_day < 0.5)
			wicked_time_of_day = m_time_of_day / wn * 0.25;
		else
			wicked_time_of_day = 1.0 - ((1.0 - m_time_of_day) / wn * 0.25);
		/*std::cerr<<"time_of_day="<<m_time_of_day<<" -> "
				<<"wicked_time_of_day="<<wicked_time_of_day<<std::endl;*/

		video::SColor suncolor = suncolor_f.toSColor();
		video::SColor suncolor2 = suncolor2_f.toSColor();
		video::SColor mooncolor = mooncolor_f.toSColor();
		video::SColor mooncolor2 = mooncolor2_f.toSColor();

		// Calculate offset normalized to the X dimension of a 512x1 px tonemap
		float offset = (1.0 - fabs(sin((m_time_of_day - 0.5) * irr::core::PI))) * 511;

		if (m_sun_tonemap) {
			u8 * texels = (u8 *)m_sun_tonemap->lock();
			video::SColor* texel = (video::SColor *)(texels + (u32)offset * 4);
			video::SColor texel_color (255, texel->getRed(),
				texel->getGreen(), texel->getBlue());
			m_sun_tonemap->unlock();
			m_materials[3].EmissiveColor = texel_color;
		}

		if (m_moon_tonemap) {
			u8 * texels = (u8 *)m_moon_tonemap->lock();
			video::SColor* texel = (video::SColor *)(texels + (u32)offset * 4);
			video::SColor texel_color (255, texel->getRed(),
				texel->getGreen(), texel->getBlue());
			m_moon_tonemap->unlock();
			m_materials[4].EmissiveColor = texel_color;
		}

		const f32 t = 1.0f;
		const f32 o = 0.0f;
		static const u16 indices[4] = {0, 1, 2, 3};
		video::S3DVertex vertices[4];

		driver->setMaterial(m_materials[1]);

		video::SColor cloudyfogcolor = m_bgcolor;

		// Draw far cloudy fog thing blended with skycolor
		for (u32 j = 0; j < 4; j++) {
			video::SColor c = cloudyfogcolor.getInterpolated(m_skycolor, 0.45);
			vertices[0] = video::S3DVertex(-1, 0.08, -1, 0, 0, 1, c, t, t);
			vertices[1] = video::S3DVertex( 1, 0.08, -1, 0, 0, 1, c, o, t);
			vertices[2] = video::S3DVertex( 1, 0.12, -1, 0, 0, 1, c, o, o);
			vertices[3] = video::S3DVertex(-1, 0.12, -1, 0, 0, 1, c, t, o);
			for (video::S3DVertex &vertex : vertices) {
				if (j == 0)
					// Don't switch
					{}
				else if (j == 1)
					// Switch from -Z (south) to +X (east)
					vertex.Pos.rotateXZBy(90);
				else if (j == 2)
					// Switch from -Z (south) to -X (west)
					vertex.Pos.rotateXZBy(-90);
				else
					// Switch from -Z (south) to +Z (north)
					vertex.Pos.rotateXZBy(-180);
			}
			driver->drawIndexedTriangleFan(&vertices[0], 4, indices, 2);
		}

		// Draw far cloudy fog thing at and below all horizons
		for (u32 j = 0; j < 4; j++) {
			video::SColor c = cloudyfogcolor;
			vertices[0] = video::S3DVertex(-1, -1.0, -1, 0, 0, 1, c, t, t);
			vertices[1] = video::S3DVertex( 1, -1.0, -1, 0, 0, 1, c, o, t);
			vertices[2] = video::S3DVertex( 1, 0.08, -1, 0, 0, 1, c, o, o);
			vertices[3] = video::S3DVertex(-1, 0.08, -1, 0, 0, 1, c, t, o);
			for (video::S3DVertex &vertex : vertices) {
				if (j == 0)
					// Don't switch
					{}
				else if (j == 1)
					// Switch from -Z (south) to +X (east)
					vertex.Pos.rotateXZBy(90);
				else if (j == 2)
					// Switch from -Z (south) to -X (west)
					vertex.Pos.rotateXZBy(-90);
				else
					// Switch from -Z (south) to +Z (north)
					vertex.Pos.rotateXZBy(-180);
			}
			driver->drawIndexedTriangleFan(&vertices[0], 4, indices, 2);
		}

		// If sun, moon and stars are (temporarily) disabled, abort here
		if (!m_bodies_visible)
			return;

		// Draw stars before sun and moon to be behind them
		do {
			driver->setMaterial(m_materials[1]);
			// Tune values so that stars first appear just after the sun
			// disappears over the horizon, and disappear just before the sun 
			// appears over the horizon.
			// Also tune so that stars are at full brightness from time 20000 to 
			// time 4000.
			float starbrightness = MYMAX(0, MYMIN(1,
				(0.25 - fabs(wicked_time_of_day < 0.5 ?
				wicked_time_of_day : (1.0 - wicked_time_of_day))) * 20));
			float f = starbrightness;
			float d = 0.007 / 2;
			video::SColor starcolor(255, f * 90, f * 90, f * 90);
			// Stars are only drawn when brighter than skycolor
			if (starcolor.getBlue() < m_skycolor.getBlue())
				break;
#ifdef __ANDROID__
			u16 indices[SKY_STAR_COUNT * 3];
			video::S3DVertex vertices[SKY_STAR_COUNT * 3];
			for (u32 i = 0; i < SKY_STAR_COUNT; i++) {
				indices[i * 3 + 0] = i * 3 + 0;
				indices[i * 3 + 1] = i * 3 + 1;
				indices[i * 3 + 2] = i * 3 + 2;
				v3f r = m_stars[i];
				core::CMatrix4<f32> a;
				a.buildRotateFromTo(v3f(0, 1, 0), r);
				v3f p = v3f(-d, 1, -d);
				v3f p1 = v3f(d, 1, 0);
				v3f p2 = v3f(-d, 1, d);
				a.rotateVect(p);
				a.rotateVect(p1);
				a.rotateVect(p2);
				p.rotateXYBy(wicked_time_of_day * 360 - 90);
				p1.rotateXYBy(wicked_time_of_day * 360 - 90);
				p2.rotateXYBy(wicked_time_of_day * 360 - 90);
				vertices[i * 3 + 0].Pos = p;
				vertices[i * 3 + 0].Color = starcolor;
				vertices[i * 3 + 1].Pos = p1;
				vertices[i * 3 + 1].Color = starcolor;
				vertices[i * 3 + 2].Pos = p2;
				vertices[i * 3 + 2].Color = starcolor;
			}
			driver->drawIndexedTriangleList(vertices, SKY_STAR_COUNT * 3,
					indices, SKY_STAR_COUNT);
#else
			u16 indices[SKY_STAR_COUNT * 4];
			video::S3DVertex vertices[SKY_STAR_COUNT * 4];
			for (u32 i = 0; i < SKY_STAR_COUNT; i++) {
				indices[i * 4 + 0] = i * 4 + 0;
				indices[i * 4 + 1] = i * 4 + 1;
				indices[i * 4 + 2] = i * 4 + 2;
				indices[i * 4 + 3] = i * 4 + 3;
				v3f r = m_stars[i];
				core::CMatrix4<f32> a;
				a.buildRotateFromTo(v3f(0, 1, 0), r);
				v3f p = v3f(-d, 1, -d);
				v3f p1 = v3f( d, 1, -d);
				v3f p2 = v3f( d, 1, d);
				v3f p3 = v3f(-d, 1, d);
				a.rotateVect(p);
				a.rotateVect(p1);
				a.rotateVect(p2);
				a.rotateVect(p3);
				p.rotateXYBy(wicked_time_of_day * 360 - 90);
				p1.rotateXYBy(wicked_time_of_day * 360 - 90);
				p2.rotateXYBy(wicked_time_of_day * 360 - 90);
				p3.rotateXYBy(wicked_time_of_day * 360 - 90);
				vertices[i * 4 + 0].Pos = p;
				vertices[i * 4 + 0].Color = starcolor;
				vertices[i * 4 + 1].Pos = p1;
				vertices[i * 4 + 1].Color = starcolor;
				vertices[i * 4 + 2].Pos = p2;
				vertices[i * 4 + 2].Color = starcolor;
				vertices[i * 4 + 3].Pos = p3;
				vertices[i * 4 + 3].Color = starcolor;
			}
			driver->drawVertexPrimitiveList(vertices, SKY_STAR_COUNT * 4,
				indices, SKY_STAR_COUNT, video::EVT_STANDARD,
				scene::EPT_QUADS, video::EIT_16BIT);
#endif
		} while(false);

		// Draw sunrise/sunset horizon glow texture (textures/base/pack/sunrisebg.png)
		{
			driver->setMaterial(m_materials[2]);
			float mid1 = 0.25;
			float mid = wicked_time_of_day < 0.5 ? mid1 : (1.0 - mid1);
			float a_ = 1.0f - std::fabs(wicked_time_of_day - mid) * 35.0f;
			float a = easeCurve(MYMAX(0, MYMIN(1, a_)));
			//std::cerr<<"a_="<<a_<<" a="<<a<<std::endl;
			video::SColor c(255, 255, 255, 255);
			float y = -(1.0 - a) * 0.22;
			vertices[0] = video::S3DVertex(-1, -0.05 + y, -1, 0, 0, 1, c, t, t);
			vertices[1] = video::S3DVertex( 1, -0.05 + y, -1, 0, 0, 1, c, o, t);
			vertices[2] = video::S3DVertex( 1,   0.2 + y, -1, 0, 0, 1, c, o, o);
			vertices[3] = video::S3DVertex(-1,   0.2 + y, -1, 0, 0, 1, c, t, o);
			for (video::S3DVertex &vertex : vertices) {
				if (wicked_time_of_day < 0.5)
					// Switch from -Z (south) to +X (east)
					vertex.Pos.rotateXZBy(90);
				else
					// Switch from -Z (south) to -X (west)
					vertex.Pos.rotateXZBy(-90);
			}
			driver->drawIndexedTriangleFan(&vertices[0], 4, indices, 2);
		}

		// Draw sun
		if (wicked_time_of_day > 0.15 && wicked_time_of_day < 0.85) {
			if (!m_sun_texture) {
				driver->setMaterial(m_materials[1]);
				float d = sunsize * 1.7;
				video::SColor c = suncolor;
				c.setAlpha(0.05 * 255);
				vertices[0] = video::S3DVertex(-d, -d, -1, 0, 0, 1, c, t, t);
				vertices[1] = video::S3DVertex( d, -d, -1, 0, 0, 1, c, o, t);
				vertices[2] = video::S3DVertex( d,  d, -1, 0, 0, 1, c, o, o);
				vertices[3] = video::S3DVertex(-d,  d, -1, 0, 0, 1, c, t, o);
				for (video::S3DVertex &vertex : vertices) {
					// Switch from -Z (south) to +X (east)
					vertex.Pos.rotateXZBy(90);
					vertex.Pos.rotateXYBy(wicked_time_of_day * 360 - 90);
				}
				driver->drawIndexedTriangleFan(&vertices[0], 4, indices, 2);

				d = sunsize * 1.2;
				c = suncolor;
				c.setAlpha(0.15 * 255);
				vertices[0] = video::S3DVertex(-d, -d, -1, 0, 0, 1, c, t, t);
				vertices[1] = video::S3DVertex( d, -d, -1, 0, 0, 1, c, o, t);
				vertices[2] = video::S3DVertex( d,  d, -1, 0, 0, 1, c, o, o);
				vertices[3] = video::S3DVertex(-d,  d, -1, 0, 0, 1, c, t, o);
				for (video::S3DVertex &vertex : vertices) {
					// Switch from -Z (south) to +X (east)
					vertex.Pos.rotateXZBy(90);
					vertex.Pos.rotateXYBy(wicked_time_of_day * 360 - 90);
				}
				driver->drawIndexedTriangleFan(&vertices[0], 4, indices, 2);

				d = sunsize;
				vertices[0] = video::S3DVertex(-d, -d, -1, 0, 0, 1, suncolor, t, t);
				vertices[1] = video::S3DVertex( d, -d, -1, 0, 0, 1, suncolor, o, t);
				vertices[2] = video::S3DVertex( d,  d, -1, 0, 0, 1, suncolor, o, o);
				vertices[3] = video::S3DVertex(-d,  d, -1, 0, 0, 1, suncolor, t, o);
				for (video::S3DVertex &vertex : vertices) {
					// Switch from -Z (south) to +X (east)
					vertex.Pos.rotateXZBy(90);
					vertex.Pos.rotateXYBy(wicked_time_of_day * 360 - 90);
				}
				driver->drawIndexedTriangleFan(&vertices[0], 4, indices, 2);

				d = sunsize * 0.7;
				vertices[0] = video::S3DVertex(-d, -d, -1, 0, 0, 1, suncolor2, t, t);
				vertices[1] = video::S3DVertex( d, -d, -1, 0, 0, 1, suncolor2, o, t);
				vertices[2] = video::S3DVertex( d,  d, -1, 0, 0, 1, suncolor2, o, o);
				vertices[3] = video::S3DVertex(-d,  d, -1, 0, 0, 1, suncolor2, t, o);
				for (video::S3DVertex &vertex : vertices) {
					// Switch from -Z (south) to +X (east)
					vertex.Pos.rotateXZBy(90);
					vertex.Pos.rotateXYBy(wicked_time_of_day * 360 - 90);
				}
				driver->drawIndexedTriangleFan(&vertices[0], 4, indices, 2);
			} else {
				driver->setMaterial(m_materials[3]);
				float d = sunsize * 1.7;
				video::SColor c;
				if (m_sun_tonemap)
					c = video::SColor (0, 0, 0, 0);
				else
					c = video::SColor (255, 255, 255, 255);
				vertices[0] = video::S3DVertex(-d, -d, -1, 0, 0, 1, c, t, t);
				vertices[1] = video::S3DVertex( d, -d, -1, 0, 0, 1, c, o, t);
				vertices[2] = video::S3DVertex( d,  d, -1, 0, 0, 1, c, o, o);
				vertices[3] = video::S3DVertex(-d,  d, -1, 0, 0, 1, c, t, o);
				for (video::S3DVertex &vertex : vertices) {
					// Switch from -Z (south) to +X (east)
					vertex.Pos.rotateXZBy(90);
					vertex.Pos.rotateXYBy(wicked_time_of_day * 360 - 90);
				}
				driver->drawIndexedTriangleFan(&vertices[0], 4, indices, 2);
			}
		}

		// Draw moon
		if (wicked_time_of_day < 0.3 || wicked_time_of_day > 0.7) {
			if (!m_moon_texture) {
				driver->setMaterial(m_materials[1]);
				float d = moonsize * 1.9;
				video::SColor c = mooncolor;
				c.setAlpha(0.05 * 255);
				vertices[0] = video::S3DVertex(-d, -d, -1, 0, 0, 1, c, t, t);
				vertices[1] = video::S3DVertex( d, -d, -1, 0, 0, 1, c, o, t);
				vertices[2] = video::S3DVertex( d,  d, -1, 0, 0, 1, c, o, o);
				vertices[3] = video::S3DVertex(-d,  d, -1, 0, 0, 1, c, t, o);
				for (video::S3DVertex &vertex : vertices) {
					// Switch from -Z (south) to -X (west)
					vertex.Pos.rotateXZBy(-90);
					vertex.Pos.rotateXYBy(wicked_time_of_day * 360 - 90);
				}
				driver->drawIndexedTriangleFan(&vertices[0], 4, indices, 2);

				d = moonsize * 1.3;
				c = mooncolor;
				c.setAlpha(0.15 * 255);
				vertices[0] = video::S3DVertex(-d, -d, -1, 0, 0, 1, c, t, t);
				vertices[1] = video::S3DVertex( d, -d, -1, 0, 0, 1, c, o, t);
				vertices[2] = video::S3DVertex( d,  d, -1, 0, 0, 1, c, o, o);
				vertices[3] = video::S3DVertex(-d,  d, -1, 0, 0, 1, c, t, o);
				for (video::S3DVertex &vertex : vertices) {
					// Switch from -Z (south) to -X (west)
					vertex.Pos.rotateXZBy(-90);
					vertex.Pos.rotateXYBy(wicked_time_of_day * 360 - 90);
				}
				driver->drawIndexedTriangleFan(&vertices[0], 4, indices, 2);

				d = moonsize;
				vertices[0] = video::S3DVertex(-d, -d, -1, 0, 0, 1, mooncolor, t, t);
				vertices[1] = video::S3DVertex( d, -d, -1, 0, 0, 1, mooncolor, o, t);
				vertices[2] = video::S3DVertex( d,  d, -1, 0, 0, 1, mooncolor, o, o);
				vertices[3] = video::S3DVertex(-d,  d, -1, 0, 0, 1, mooncolor, t, o);
				for (video::S3DVertex &vertex : vertices) {
					// Switch from -Z (south) to -X (west)
					vertex.Pos.rotateXZBy(-90);
					vertex.Pos.rotateXYBy(wicked_time_of_day * 360 - 90);
				}
				driver->drawIndexedTriangleFan(&vertices[0], 4, indices, 2);

				float d2 = moonsize * 0.6;
				vertices[0] = video::S3DVertex(-d, -d,  -1, 0, 0, 1, mooncolor2, t, t);
				vertices[1] = video::S3DVertex( d2,-d,  -1, 0, 0, 1, mooncolor2, o, t);
				vertices[2] = video::S3DVertex( d2, d2, -1, 0, 0, 1, mooncolor2, o, o);
				vertices[3] = video::S3DVertex(-d,  d2, -1, 0, 0, 1, mooncolor2, t, o);
				for (video::S3DVertex &vertex : vertices) {
					// Switch from -Z (south) to -X (west)
					vertex.Pos.rotateXZBy(-90);
					vertex.Pos.rotateXYBy(wicked_time_of_day * 360 - 90);
				}
				driver->drawIndexedTriangleFan(&vertices[0], 4, indices, 2);
			} else {
				driver->setMaterial(m_materials[4]);
				float d = moonsize * 1.9;
				video::SColor c;
				if (m_moon_tonemap)
					c = video::SColor (0, 0, 0, 0);
				else
					c = video::SColor (255, 255, 255, 255);
				vertices[0] = video::S3DVertex(-d, -d, -1, 0, 0, 1, c, t, t);
				vertices[1] = video::S3DVertex( d, -d, -1, 0, 0, 1, c, o, t);
				vertices[2] = video::S3DVertex( d,  d, -1, 0, 0, 1, c, o, o);
				vertices[3] = video::S3DVertex(-d,  d, -1, 0, 0, 1, c, t, o);
				for (video::S3DVertex &vertex : vertices) {
					// Switch from -Z (south) to -X (west)
					vertex.Pos.rotateXZBy(-90);
					vertex.Pos.rotateXYBy(wicked_time_of_day * 360 - 90);
				}
				driver->drawIndexedTriangleFan(&vertices[0], 4, indices, 2);
			}
		}

		// Draw far cloudy fog thing below all horizons in front of sun, moon
		// and stars.
		driver->setMaterial(m_materials[1]);

		for (u32 j = 0; j < 4; j++) {
			video::SColor c = cloudyfogcolor;
			vertices[0] = video::S3DVertex(-1, -1.0,  -1, 0, 0, 1, c, t, t);
			vertices[1] = video::S3DVertex( 1, -1.0,  -1, 0, 0, 1, c, o, t);
			vertices[2] = video::S3DVertex( 1, -0.02, -1, 0, 0, 1, c, o, o);
			vertices[3] = video::S3DVertex(-1, -0.02, -1, 0, 0, 1, c, t, o);
			for (video::S3DVertex &vertex : vertices) {
				if (j == 0)
					// Don't switch
					{}
				else if (j == 1)
					// Switch from -Z (south) to +X (east)
					vertex.Pos.rotateXZBy(90);
				else if (j == 2)
					// Switch from -Z (south) to -X (west)
					vertex.Pos.rotateXZBy(-90);
				else
					// Switch from -Z (south) to +Z (north)
					vertex.Pos.rotateXZBy(-180);
			}
			driver->drawIndexedTriangleFan(&vertices[0], 4, indices, 2);
		}

		// Draw bottom far cloudy fog thing in front of sun, moon and stars
		video::SColor c = cloudyfogcolor;
		vertices[0] = video::S3DVertex(-1, -1.0, -1, 0, 1, 0, c, t, t);
		vertices[1] = video::S3DVertex( 1, -1.0, -1, 0, 1, 0, c, o, t);
		vertices[2] = video::S3DVertex( 1, -1.0, 1, 0, 1, 0, c, o, o);
		vertices[3] = video::S3DVertex(-1, -1.0, 1, 0, 1, 0, c, t, o);
		driver->drawIndexedTriangleFan(&vertices[0], 4, indices, 2);
	}
}


void Sky::update(float time_of_day, float time_brightness,
		float direct_brightness, bool sunlight_seen,
		CameraMode cam_mode, float yaw, float pitch)
{
	// Stabilize initial brightness and color values by flooding updates
	if (m_first_update) {
		/*dstream<<"First update with time_of_day="<<time_of_day
				<<" time_brightness="<<time_brightness
				<<" direct_brightness="<<direct_brightness
				<<" sunlight_seen="<<sunlight_seen<<std::endl;*/
		m_first_update = false;
		for (u32 i = 0; i < 100; i++) {
			update(time_of_day, time_brightness, direct_brightness,
				sunlight_seen, cam_mode, yaw, pitch);
		}
		return;
	}

	m_time_of_day = time_of_day;
	m_time_brightness = time_brightness;
	m_sunlight_seen = sunlight_seen;
	m_bodies_visible = true;

	bool is_dawn = (time_brightness >= 0.20 && time_brightness < 0.35);

	/*
	Development colours

	video::SColorf bgcolor_bright_normal_f(170. / 255, 200. / 255, 230. / 255, 1.0);
	video::SColorf bgcolor_bright_dawn_f(0.666, 200. / 255 * 0.7, 230. / 255 * 0.5, 1.0);
	video::SColorf bgcolor_bright_dawn_f(0.666, 0.549, 0.220, 1.0);
	video::SColorf bgcolor_bright_dawn_f(0.666 * 1.2, 0.549 * 1.0, 0.220 * 1.0, 1.0);
	video::SColorf bgcolor_bright_dawn_f(0.666 * 1.2, 0.549 * 1.0, 0.220 * 1.2, 1.0);

	video::SColorf cloudcolor_bright_dawn_f(1.0, 0.591, 0.4);
	video::SColorf cloudcolor_bright_dawn_f(1.0, 0.65, 0.44);
	video::SColorf cloudcolor_bright_dawn_f(1.0, 0.7, 0.5);
	*/

	video::SColorf bgcolor_bright_normal_f = video::SColor(255, 155, 193, 240);
	video::SColorf bgcolor_bright_indoor_f = video::SColor(255, 100, 100, 100);
	video::SColorf bgcolor_bright_dawn_f = video::SColor(255, 186, 193, 240);
	video::SColorf bgcolor_bright_night_f = video::SColor(255, 64, 144, 255);

	video::SColorf skycolor_bright_normal_f = video::SColor(255, 140, 186, 250);
	video::SColorf skycolor_bright_dawn_f = video::SColor(255, 180, 186, 250);
	video::SColorf skycolor_bright_night_f = video::SColor(255, 0, 107, 255);

	// pure white: becomes "diffuse light component" for clouds
	video::SColorf cloudcolor_bright_normal_f = video::SColor(255, 255, 255, 255);
	// dawn-factoring version of pure white (note: R is above 1.0)
	video::SColorf cloudcolor_bright_dawn_f(255.0f/240.0f, 223.0f/240.0f, 191.0f/255.0f);

	float cloud_color_change_fraction = 0.95;
	if (sunlight_seen) {
		if (std::fabs(time_brightness - m_brightness) < 0.2f) {
			m_brightness = m_brightness * 0.95 + time_brightness * 0.05;
		} else {
			m_brightness = m_brightness * 0.80 + time_brightness * 0.20;
			cloud_color_change_fraction = 0.0;
		}
	} else {
		if (direct_brightness < m_brightness)
			m_brightness = m_brightness * 0.95 + direct_brightness * 0.05;
		else
			m_brightness = m_brightness * 0.98 + direct_brightness * 0.02;
	}

	m_clouds_visible = true;
	float color_change_fraction = 0.98f;
	if (sunlight_seen) {
		if (is_dawn) { // Dawn
			m_bgcolor_bright_f = m_bgcolor_bright_f.getInterpolated(
				bgcolor_bright_dawn_f, color_change_fraction);
			m_skycolor_bright_f = m_skycolor_bright_f.getInterpolated(
				skycolor_bright_dawn_f, color_change_fraction);
			m_cloudcolor_bright_f = m_cloudcolor_bright_f.getInterpolated(
				cloudcolor_bright_dawn_f, color_change_fraction);
		} else {
			if (time_brightness < 0.13f) { // Night
				m_bgcolor_bright_f = m_bgcolor_bright_f.getInterpolated(
					bgcolor_bright_night_f, color_change_fraction);
				m_skycolor_bright_f = m_skycolor_bright_f.getInterpolated(
					skycolor_bright_night_f, color_change_fraction);
			} else { // Day
				m_bgcolor_bright_f = m_bgcolor_bright_f.getInterpolated(
					bgcolor_bright_normal_f, color_change_fraction);
				m_skycolor_bright_f = m_skycolor_bright_f.getInterpolated(
					skycolor_bright_normal_f, color_change_fraction);
			}

			m_cloudcolor_bright_f = m_cloudcolor_bright_f.getInterpolated(
				cloudcolor_bright_normal_f, color_change_fraction);
		}
	} else {
		m_bgcolor_bright_f = m_bgcolor_bright_f.getInterpolated(
			bgcolor_bright_indoor_f, color_change_fraction);
		m_skycolor_bright_f = m_skycolor_bright_f.getInterpolated(
			bgcolor_bright_indoor_f, color_change_fraction);
		m_cloudcolor_bright_f = m_cloudcolor_bright_f.getInterpolated(
			cloudcolor_bright_normal_f, color_change_fraction);
		m_clouds_visible = false;
	}

	video::SColor bgcolor_bright = m_bgcolor_bright_f.toSColor();
	m_bgcolor = video::SColor(
		255,
		bgcolor_bright.getRed() * m_brightness,
		bgcolor_bright.getGreen() * m_brightness,
		bgcolor_bright.getBlue() * m_brightness
	);

	video::SColor skycolor_bright = m_skycolor_bright_f.toSColor();
	m_skycolor = video::SColor(
		255,
		skycolor_bright.getRed() * m_brightness,
		skycolor_bright.getGreen() * m_brightness,
		skycolor_bright.getBlue() * m_brightness
	);

	// Horizon coloring based on sun and moon direction during sunset and sunrise
	video::SColor pointcolor = video::SColor(m_bgcolor.getAlpha(), 255, 255, 255);
	if (m_directional_colored_fog) {
		if (m_horizon_blend() != 0) {
			// Calculate hemisphere value from yaw, (inverted in third person front view)
			s8 dir_factor = 1;
			if (cam_mode > CAMERA_MODE_THIRD)
				dir_factor = -1;
			f32 pointcolor_blend = wrapDegrees_0_360(yaw * dir_factor + 90);
			if (pointcolor_blend > 180)
				pointcolor_blend = 360 - pointcolor_blend;
			pointcolor_blend /= 180;
			// Bound view angle to determine where transition starts and ends
			pointcolor_blend = rangelim(1 - pointcolor_blend * 1.375, 0, 1 / 1.375) *
				1.375;
			// Combine the colors when looking up or down, otherwise turning looks weird
			pointcolor_blend += (0.5 - pointcolor_blend) *
				(1 - MYMIN((90 - std::fabs(pitch)) / 90 * 1.5, 1));
			// Invert direction to match where the sun and moon are rising
			if (m_time_of_day > 0.5)
				pointcolor_blend = 1 - pointcolor_blend;
			// Horizon colors of sun and moon
			f32 pointcolor_light = rangelim(m_time_brightness * 3, 0.2, 1);

			video::SColorf pointcolor_sun_f(1, 1, 1, 1);
			if (m_sun_tonemap) {
				pointcolor_sun_f.r = pointcolor_light *
					(float)m_materials[3].EmissiveColor.getRed() / 255;
				pointcolor_sun_f.b = pointcolor_light *
					(float)m_materials[3].EmissiveColor.getBlue() / 255;
				pointcolor_sun_f.g = pointcolor_light *
					(float)m_materials[3].EmissiveColor.getGreen() / 255;
			} else {
				pointcolor_sun_f.r = pointcolor_light * 1;
				pointcolor_sun_f.b = pointcolor_light *
					(0.25 + (rangelim(m_time_brightness, 0.25, 0.75) - 0.25) * 2 * 0.75);
				pointcolor_sun_f.g = pointcolor_light * (pointcolor_sun_f.b * 0.375 +
					(rangelim(m_time_brightness, 0.05, 0.15) - 0.05) * 10 * 0.625);
			}

			video::SColorf pointcolor_moon_f(0.5 * pointcolor_light,
				0.6 * pointcolor_light, 0.8 * pointcolor_light, 1);
			if (m_moon_tonemap) {
				pointcolor_moon_f.r = pointcolor_light *
					(float)m_materials[4].EmissiveColor.getRed() / 255;
				pointcolor_moon_f.b = pointcolor_light *
					(float)m_materials[4].EmissiveColor.getBlue() / 255;
				pointcolor_moon_f.g = pointcolor_light *
					(float)m_materials[4].EmissiveColor.getGreen() / 255;
			}

			video::SColor pointcolor_sun = pointcolor_sun_f.toSColor();
			video::SColor pointcolor_moon = pointcolor_moon_f.toSColor();
			// Calculate the blend color
			pointcolor = m_mix_scolor(pointcolor_moon, pointcolor_sun, pointcolor_blend);
		}
		m_bgcolor = m_mix_scolor(m_bgcolor, pointcolor, m_horizon_blend() * 0.5);
		m_skycolor = m_mix_scolor(m_skycolor, pointcolor, m_horizon_blend() * 0.25);
	}

	float cloud_direct_brightness = 0.0f;
	if (sunlight_seen) {
		if (!m_directional_colored_fog) {
			cloud_direct_brightness = time_brightness;
			// Boost cloud brightness relative to sky, at dawn, dusk and at night
			if (time_brightness < 0.7f)
				cloud_direct_brightness *= 1.3f;
		} else {
			cloud_direct_brightness = std::fmin(m_horizon_blend() * 0.15f +
				m_time_brightness, 1.0f);
			// Set the same minimum cloud brightness at night
			if (time_brightness < 0.5f)
				cloud_direct_brightness = std::fmax(cloud_direct_brightness,
					time_brightness * 1.3f);
		}
	} else {
		cloud_direct_brightness = direct_brightness;
	}

	m_cloud_brightness = m_cloud_brightness * cloud_color_change_fraction +
		cloud_direct_brightness * (1.0 - cloud_color_change_fraction);
	m_cloudcolor_f = video::SColorf(
		m_cloudcolor_bright_f.r * m_cloud_brightness,
		m_cloudcolor_bright_f.g * m_cloud_brightness,
		m_cloudcolor_bright_f.b * m_cloud_brightness,
		1.0
	);
	if (m_directional_colored_fog) {
		m_cloudcolor_f = m_mix_scolorf(m_cloudcolor_f,
			video::SColorf(pointcolor), m_horizon_blend() * 0.25);
	}
}
