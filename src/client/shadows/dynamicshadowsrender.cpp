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

#include <cstring>
#include <cmath>
#include "client/shadows/dynamicshadowsrender.h"
#include "client/shadows/shadowsScreenQuad.h"
#include "client/shadows/shadowsshadercallbacks.h"
#include "settings.h"
#include "filesys.h"
#include "util/string.h"
#include "client/shader.h"
#include "client/client.h"
#include "client/clientmap.h"
#include "profiler.h"
#include "EShaderTypes.h"
#include "IGPUProgrammingServices.h"
#include "IMaterialRenderer.h"

ShadowRenderer::ShadowRenderer(IrrlichtDevice *device, Client *client) :
		m_smgr(device->getSceneManager()), m_driver(device->getVideoDriver()),
		m_client(client), m_perspective_bias_z(0.5)
{
	(void) m_client;

	m_shadows_supported = true; // assume shadows supported. We will check actual support in initialize
	m_shadows_enabled = true;

	m_shadow_strength_gamma = g_settings->getFloat("shadow_strength_gamma");
	if (std::isnan(m_shadow_strength_gamma))
		m_shadow_strength_gamma = 1.0f;
	m_shadow_strength_gamma = core::clamp(m_shadow_strength_gamma, 0.1f, 10.0f);

	m_shadow_map_max_distance = g_settings->getFloat("shadow_map_max_distance");

	m_shadow_map_texture_size = g_settings->getFloat("shadow_map_texture_size");

	m_shadow_map_texture_32bit = g_settings->getBool("shadow_map_texture_32bit");
	m_shadow_map_colored = g_settings->getBool("shadow_map_color");
	m_shadow_samples = g_settings->getS32("shadow_filters");

	// add at least one light
	addDirectionalLight();
}

ShadowRenderer::~ShadowRenderer()
{
	// call to disable releases dynamically allocated resources
	disable();

	if (m_shadow_depth_cb)
		delete m_shadow_depth_cb;
	if (m_shadow_depth_entity_cb)
		delete m_shadow_depth_entity_cb;
	if (m_shadow_depth_trans_cb)
		delete m_shadow_depth_trans_cb;
	if (m_shadow_mix_cb)
		delete m_shadow_mix_cb;
	m_shadow_node_array.clear();
	m_light_list.clear();
}

void ShadowRenderer::disable()
{
	m_shadows_enabled = false;
	if (shadowMapTextureFinal) {
		m_driver->setRenderTarget(shadowMapTextureFinal, true, true,
			video::SColor(255, 255, 255, 255));
		m_driver->setRenderTarget(0, false, false);
	}

	if (shadowMapTextureDynamicObjects) {
		m_driver->removeTexture(shadowMapTextureDynamicObjects);
		shadowMapTextureDynamicObjects = nullptr;
	}

	if (shadowMapTextureFinal) {
		m_driver->removeTexture(shadowMapTextureFinal);
		shadowMapTextureFinal = nullptr;
	}

	if (shadowMapTextureColors) {
		m_driver->removeTexture(shadowMapTextureColors);
		shadowMapTextureColors = nullptr;
	}

	if (shadowMapClientMap) {
		m_driver->removeTexture(shadowMapClientMap);
		shadowMapClientMap = nullptr;
	}

	if (!shadowMapClientMapFuture.empty()) {
		for (auto texture: shadowMapClientMapFuture)
			m_driver->removeTexture(texture);
		shadowMapClientMapFuture.clear();
	}

	for (auto node : m_shadow_node_array)
		node.node->forEachMaterial([] (auto &mat) {
			mat.setTexture(TEXTURE_LAYER_SHADOW, nullptr);
		});
}

void ShadowRenderer::initialize()
{
	auto *gpu = m_driver->getGPUProgrammingServices();

	// we need glsl
	if (m_shadows_supported && gpu && m_driver->queryFeature(video::EVDF_ARB_GLSL)) {
		createShaders();
	} else {
		m_shadows_supported = false;

		warningstream << "Shadows: GLSL Shader not supported on this system."
			<< std::endl;
		return;
	}

	m_texture_format = m_shadow_map_texture_32bit
					   ? video::ECOLOR_FORMAT::ECF_R32F
					   : video::ECOLOR_FORMAT::ECF_R16F;

	m_texture_format_color = m_shadow_map_texture_32bit
						 ? video::ECOLOR_FORMAT::ECF_G32R32F
						 : video::ECOLOR_FORMAT::ECF_G16R16F;

	m_shadows_enabled &= m_shadows_supported;
}


size_t ShadowRenderer::addDirectionalLight()
{
	m_light_list.emplace_back(m_shadow_map_texture_size,
			v3f(0.f, 0.f, 0.f),
			video::SColor(255, 255, 255, 255), m_shadow_map_max_distance);
	return m_light_list.size() - 1;
}

DirectionalLight &ShadowRenderer::getDirectionalLight(u32 index)
{
	return m_light_list[index];
}

size_t ShadowRenderer::getDirectionalLightCount() const
{
	return m_light_list.size();
}

f32 ShadowRenderer::getMaxShadowFar() const
{
	float zMax = m_light_list[0].getFarValue();
	return zMax;
}

void ShadowRenderer::setShadowIntensity(float shadow_intensity)
{
	m_shadow_strength = pow(shadow_intensity, 1.0f / m_shadow_strength_gamma);
	if (m_shadow_strength > 1E-2)
		enable();
	else
		disable();
}

void ShadowRenderer::addNodeToShadowList(
		scene::ISceneNode *node, E_SHADOW_MODE shadowMode)
{
	m_shadow_node_array.emplace_back(node, shadowMode);
	// node should never be ClientMap
	assert(strcmp(node->getName(), "ClientMap") != 0);

	node->forEachMaterial([this] (auto &mat) {
		mat.setTexture(TEXTURE_LAYER_SHADOW, shadowMapTextureFinal);
	});
}

void ShadowRenderer::removeNodeFromShadowList(scene::ISceneNode *node)
{
	node->forEachMaterial([] (auto &mat) {
		mat.setTexture(TEXTURE_LAYER_SHADOW, nullptr);
	});
	for (auto it = m_shadow_node_array.begin(); it != m_shadow_node_array.end();) {
		if (it->node == node) {
			it = m_shadow_node_array.erase(it);
			break;
		} else {
			++it;
		}
	}
}

// SM rendering rules:
// 1. Render ClientMap cascade 0 directly to ClientMap shadow map
// 2. Update distant cascades incrementally (1, 2,...)
//    - If distant cascade is finished
//      a. Blit cascade from backbuffer to ClientMap shadow Map
//      b. Render cascade's colored texture
// 3. Render objects (cascades 0, 1)
// 4. Merge the final SM texture = (Solid + Color + Objects)

// Texture layout for shadow maps:
// - Final SM texture contains N cascades
// - Entity shadow map contains 2 cascades: 0,1.
// - ClientMap shadow map contains N cascades: 1, 2, ...
// - N-1 ClientMap backbuffer textures for cascades: 1, 2, ...
// - ClientMap colored texture contains N cascades.

void ShadowRenderer::ensureSMTextures(u8 n_cascades)
{
	if (!m_shadows_enabled || m_smgr->getActiveCamera() == nullptr) {
		return;
	}

	if (!shadowMapTextureDynamicObjects) {

		shadowMapTextureDynamicObjects = getSMTexture(
				std::string("shadow_dynamic_") + itos(m_shadow_map_texture_size),
				m_texture_format, MYMIN(2, n_cascades), true);
		assert(shadowMapTextureDynamicObjects != nullptr);
	}

	if (!shadowMapClientMap) {

		shadowMapClientMap = getSMTexture(
				std::string("shadow_clientmap_") + itos(m_shadow_map_texture_size),
				m_texture_format,
				n_cascades, true);
		assert(shadowMapClientMap != nullptr);
	}

	if (shadowMapClientMapFuture.empty()) {
		for (u8 i = 1; i < n_cascades; i++) {
			video::ITexture *cascade_texture = getSMTexture(
					std::string("shadow_clientmap_bb_") + itos(i) + std::string("_") + itos(m_shadow_map_texture_size),
					m_texture_format,
					1, true);
			assert(cascade_texture != nullptr);
			shadowMapClientMapFuture.push_back(cascade_texture);
		}
	}

	if (m_shadow_map_colored && !shadowMapTextureColors) {
		shadowMapTextureColors = getSMTexture(
				std::string("shadow_colored_") + itos(m_shadow_map_texture_size),
				m_texture_format_color,
				n_cascades, true);
		assert(shadowMapTextureColors != nullptr);
	}

	// The merge all shadowmaps texture
	if (!shadowMapTextureFinal) {
		video::ECOLOR_FORMAT frt;
		if (m_shadow_map_texture_32bit) {
			if (m_shadow_map_colored)
				frt = video::ECOLOR_FORMAT::ECF_A32B32G32R32F;
			else
				frt = video::ECOLOR_FORMAT::ECF_R32F;
		} else {
			if (m_shadow_map_colored)
				frt = video::ECOLOR_FORMAT::ECF_A16B16G16R16F;
			else
				frt = video::ECOLOR_FORMAT::ECF_R16F;
		}
		shadowMapTextureFinal = getSMTexture(
			std::string("shadowmap_final_") + itos(m_shadow_map_texture_size),
			frt, n_cascades, true);
		assert(shadowMapTextureFinal != nullptr);

		for (auto &node : m_shadow_node_array)
			node.node->forEachMaterial([this] (auto &mat) {
				mat.setTexture(TEXTURE_LAYER_SHADOW, shadowMapTextureFinal);
			});
	}
}

static void drawQuad(video::SColor color, video::ITexture *texture, video::IVideoDriver *driver)
{
	shadowScreenQuad quad(color);
	auto &material = quad.getMaterial();
	material.MaterialType = video::EMT_SOLID;
	material.TextureLayers[0].Texture = texture;
	quad.render(driver);
}

void ShadowRenderer::renderMapShadows()
{
	if (!m_shadows_enabled || m_smgr->getActiveCamera() == nullptr) {
		return;
	}

	if (!m_shadow_node_array.empty()) {
		// Update SM incrementally:
		for (DirectionalLight &light : m_light_list) {
			for (u8 i = 0; i < light.getCascadesCount(); i++) {
				auto& cascade = light.getCascade(i);

				if (cascade.current_frame >= cascade.max_frames)
					continue;

				// Static shader values.
				for (auto cb : {m_shadow_depth_cb, m_shadow_depth_entity_cb, m_shadow_depth_trans_cb})
					if (cb) {
						cb->MapRes = (f32)m_shadow_map_texture_size;
						cb->MaxFar = (f32)m_shadow_map_max_distance * BS;
						cb->PerspectiveBiasZ = getPerspectiveBiasZ();
						cb->Cascade = i;
					}

				// set the Render Target
				// For cascade 0 render directly into shadowMapClientMap texture
				// For other cascades render into a relevant back-buffer texture
				if (i == 0) {
					if (cascade.current_frame == 0) {
						m_driver->setRenderTarget(shadowMapClientMap, false, true);
						// erase the area of cascade 0
						m_driver->setViewPort(core::rect<s32>(
								0, 0,
								m_shadow_map_texture_size, m_shadow_map_texture_size));
						drawQuad(video::SColor(0xFFFFFFFF), nullptr, m_driver);
					}
				}
				else
					// shadowMapClientFuture has one less cascade
					m_driver->setRenderTarget(shadowMapClientMapFuture.at(i-1), cascade.current_frame == 0, true, video::SColor(0xFFFFFFFF));

				renderShadowMap(shadowMapClientMap, i, cascade);

				if (cascade.current_frame == cascade.max_frames - 1) {
					if (i > 0) {
						// blit current texture to the main
						m_driver->setRenderTarget(shadowMapClientMap, false, true);
						m_driver->setViewPort(core::rect<s32>(
								i * m_shadow_map_texture_size, 0,
								(i + 1) * m_shadow_map_texture_size, m_shadow_map_texture_size));
						drawQuad(video::SColor(0xFFFFFFFF), shadowMapClientMapFuture.at(i-1), m_driver);
					}

					// Render transparent part in one pass.
					// This is also handled in ClientMap.
					if (m_shadow_map_colored) {
						m_driver->setRenderTarget(shadowMapTextureColors,
								false, true);
						m_driver->setViewPort(core::rect<s32>(
								i * m_shadow_map_texture_size, 0,
								(i + 1) * m_shadow_map_texture_size, m_shadow_map_texture_size));
						// erase the area
						drawQuad(video::SColor(0xFFFFFFFF), nullptr, m_driver);
						// render the shadow map
						renderShadowMap(shadowMapTextureColors, i, cascade,
								scene::ESNRP_TRANSPARENT);
					}
					cascade.commitFrustum();
				}
				cascade.current_frame++;
			}
		} // end for lights
	}
}

void ShadowRenderer::renderEntityShadows()
{
	if (!m_shadow_node_array.empty()) {

		// render shadows for the n0n-map objects.
		m_driver->setRenderTarget(shadowMapTextureDynamicObjects, true,
				true, video::SColor(255, 255, 255, 255));
		for (DirectionalLight &light : m_light_list) {
			for (u8 i = 0; i < 2; i++) {
				if (light.getCascadesCount() <= i)
					break;
				const auto &cascade = light.getCascade(i);
				// Static shader values for entities are set in updateSMTextures
				m_shadow_depth_entity_cb->Cascade = i;

				m_driver->setViewPort(core::rect<s32>(
						i * m_shadow_map_texture_size, 0,
						(i + 1) * m_shadow_map_texture_size, m_shadow_map_texture_size));
				
				// only render entities in the first cascade, we figure out later
				renderShadowObjects(shadowMapTextureDynamicObjects, cascade);
			} // end for cascades
		} // end for lights
	}
}

void ShadowRenderer::mergeShadowMaps()
{

	// in order to avoid too many map shadow renders,
	// we should make a second pass to mix clientmap shadows and
	// entities shadows :(
	m_screen_quad->getMaterial().setTexture(0, shadowMapClientMap);
	// dynamic objs shadow texture.
	if (m_shadow_map_colored)
		m_screen_quad->getMaterial().setTexture(1, shadowMapTextureColors);
	m_screen_quad->getMaterial().setTexture(2, shadowMapTextureDynamicObjects);

	m_driver->setRenderTarget(shadowMapTextureFinal, false, false,
			video::SColor(255, 255, 255, 255));
	m_screen_quad->render(m_driver);
}

void ShadowRenderer::update(video::ITexture *outputTarget)
{
	if (!m_shadows_enabled || m_smgr->getActiveCamera() == nullptr) {
		return;
	}

	ensureSMTextures(getDirectionalLight().getCascadesCount());

	if (shadowMapTextureFinal == nullptr) {
		return;
	}

	renderMapShadows();

	renderEntityShadows();

	mergeShadowMaps();
}

void ShadowRenderer::drawDebug()
{
	/* this code just shows shadows textures in screen and in ONLY for debugging*/
	#if 0
	// this is debug, ignore for now.
	if (shadowMapTextureFinal)
		m_driver->draw2DImage(shadowMapTextureFinal,
				core::rect<s32>(0, 50, 128 * 3, 128 + 50),
				core::rect<s32>({0, 0}, shadowMapTextureFinal->getSize()));

	if (shadowMapClientMap)
		m_driver->draw2DImage(shadowMapClientMap,
				core::rect<s32>(0, 50 + 128, 128 * 3, 128 + 50 + 128),
				core::rect<s32>({0, 0}, shadowMapTextureFinal->getSize()));

	if (shadowMapTextureDynamicObjects)
		m_driver->draw2DImage(shadowMapTextureDynamicObjects,
				core::rect<s32>(0, 128 + 50 + 128, 128 * 2,
						128 + 50 + 128 + 128),
				core::rect<s32>({0, 0}, shadowMapTextureDynamicObjects->getSize()));

	if (m_shadow_map_colored && shadowMapTextureColors) {

		m_driver->draw2DImage(shadowMapTextureColors,
				core::rect<s32>(128,128 + 50 + 128 + 128,
						128 * 3 + 128, 128 + 50 + 128 + 128 + 128),
				core::rect<s32>({0, 0}, shadowMapTextureColors->getSize()));
	}
	#endif
}


video::ITexture *ShadowRenderer::getSMTexture(const std::string &shadow_map_name,
		video::ECOLOR_FORMAT texture_format, u8 n_cascades, bool force_creation)
{
	if (force_creation) {
		return m_driver->addRenderTargetTexture(
				core::dimension2du(m_shadow_map_texture_size * n_cascades,
						m_shadow_map_texture_size),
				shadow_map_name.c_str(), texture_format);
	}

	return m_driver->getTexture(shadow_map_name.c_str());
}

void ShadowRenderer::renderShadowMap(video::ITexture *target, u8 cascade_index,
		const ShadowCascade &cascade, scene::E_SCENE_NODE_RENDER_PASS pass)
{
	m_driver->setTransform(video::ETS_VIEW, cascade.getFutureViewMatrix());
	m_driver->setTransform(video::ETS_PROJECTION, cascade.getFutureProjectionMatrix());

	ClientMap &map_node = static_cast<ClientMap &>(m_client->getEnv().getMap());

	video::SMaterial material;
	if (map_node.getMaterialCount() > 0) {
		// we only want the first material, which is the one with the albedo info
		material = map_node.getMaterial(0);
	}

	material.BackfaceCulling = false;
	material.FrontfaceCulling = false;

	if (m_shadow_map_colored && pass != scene::ESNRP_SOLID) {
		material.MaterialType = (video::E_MATERIAL_TYPE) depth_shader_trans;
	}
	else {
		material.MaterialType = (video::E_MATERIAL_TYPE) depth_shader;
		material.BlendOperation = video::EBO_MIN;
	}

	int frame = cascade.current_frame;
	int total_frames = cascade.max_frames;

	map_node.renderMapShadows(cascade_index, m_driver, material, pass, frame, total_frames);
}

void ShadowRenderer::renderShadowObjects(
		video::ITexture *target, const ShadowCascade &cascade)
{
	m_driver->setTransform(video::ETS_VIEW, cascade.getViewMatrix());
	m_driver->setTransform(video::ETS_PROJECTION, cascade.getProjectionMatrix());

	for (const auto &shadow_node : m_shadow_node_array) {
		// we only take care of the shadow casters and only visible nodes cast shadows
		if (shadow_node.shadowMode == ESM_RECEIVE || !shadow_node.node->isVisible())
			continue;

		// render other objects
		u32 n_node_materials = shadow_node.node->getMaterialCount();
		std::vector<s32> BufferMaterialList;
		std::vector<std::pair<bool, bool>> BufferMaterialCullingList;
		std::vector<video::E_BLEND_OPERATION> BufferBlendOperationList;
		BufferMaterialList.reserve(n_node_materials);
		BufferMaterialCullingList.reserve(n_node_materials);
		BufferBlendOperationList.reserve(n_node_materials);

		// backup materialtype for each material
		// (aka shader)
		// and replace it by our "depth" shader
		for (u32 m = 0; m < n_node_materials; m++) {
			auto &current_mat = shadow_node.node->getMaterial(m);

			BufferMaterialList.push_back(current_mat.MaterialType);
			current_mat.MaterialType =
					(video::E_MATERIAL_TYPE)depth_shader_entities;

			BufferMaterialCullingList.emplace_back(
				(bool)current_mat.BackfaceCulling, (bool)current_mat.FrontfaceCulling);
			BufferBlendOperationList.push_back(current_mat.BlendOperation);

			current_mat.BackfaceCulling = true;
			current_mat.FrontfaceCulling = false;
		}

		m_driver->setTransform(video::ETS_WORLD,
				shadow_node.node->getAbsoluteTransformation());
		shadow_node.node->render();

		// restore the material.

		for (u32 m = 0; m < n_node_materials; m++) {
			auto &current_mat = shadow_node.node->getMaterial(m);

			current_mat.MaterialType = (video::E_MATERIAL_TYPE) BufferMaterialList[m];

			current_mat.BackfaceCulling = BufferMaterialCullingList[m].first;
			current_mat.FrontfaceCulling = BufferMaterialCullingList[m].second;
			current_mat.BlendOperation = BufferBlendOperationList[m];
		}

	} // end for caster shadow nodes
}

void ShadowRenderer::mixShadowsQuad()
{
}

/*
 * @Liso's disclaimer ;) This function loads the Shadow Mapping Shaders.
 * I used a custom loader because I couldn't figure out how to use the base
 * Shaders system with custom IShaderConstantSetCallBack without messing up the
 * code too much. If anyone knows how to integrate this with the standard MT
 * shaders, please feel free to change it.
 */

void ShadowRenderer::createShaders()
{
	video::IGPUProgrammingServices *gpu = m_driver->getGPUProgrammingServices();

	if (depth_shader == -1) {
		std::string depth_shader_vs = getShaderPath("shadow_shaders", "pass1_vertex.glsl");
		if (depth_shader_vs.empty()) {
			m_shadows_supported = false;
			errorstream << "Error shadow mapping vs shader not found." << std::endl;
			return;
		}
		std::string depth_shader_fs = getShaderPath("shadow_shaders", "pass1_fragment.glsl");
		if (depth_shader_fs.empty()) {
			m_shadows_supported = false;
			errorstream << "Error shadow mapping fs shader not found." << std::endl;
			return;
		}
		m_shadow_depth_cb = new ShadowDepthShaderCB();

		depth_shader = gpu->addHighLevelShaderMaterial(
				readShaderFile(depth_shader_vs).c_str(), "vertexMain",
				video::EVST_VS_1_1,
				readShaderFile(depth_shader_fs).c_str(), "pixelMain",
				video::EPST_PS_1_2, m_shadow_depth_cb, video::EMT_ONETEXTURE_BLEND);

		if (depth_shader == -1) {
			// upsi, something went wrong loading shader.
			delete m_shadow_depth_cb;
			m_shadow_depth_cb = nullptr;
			m_shadows_enabled = false;
			m_shadows_supported = false;
			errorstream << "Error compiling shadow mapping shader." << std::endl;
			return;
		}

		// HACK, TODO: investigate this better
		// Grab the material renderer once more so minetest doesn't crash
		// on exit
		m_driver->getMaterialRenderer(depth_shader)->grab();
	}

	// This creates a clone of depth_shader with base material set to EMT_SOLID,
	// because entities won't render shadows with base material EMP_ONETEXTURE_BLEND
	if (depth_shader_entities == -1) {
		std::string depth_shader_vs = getShaderPath("shadow_shaders", "pass1_vertex.glsl");
		if (depth_shader_vs.empty()) {
			m_shadows_supported = false;
			errorstream << "Error shadow mapping vs shader not found." << std::endl;
			return;
		}
		std::string depth_shader_fs = getShaderPath("shadow_shaders", "pass1_fragment.glsl");
		if (depth_shader_fs.empty()) {
			m_shadows_supported = false;
			errorstream << "Error shadow mapping fs shader not found." << std::endl;
			return;
		}
		m_shadow_depth_entity_cb = new ShadowDepthShaderCB();

		depth_shader_entities = gpu->addHighLevelShaderMaterial(
				readShaderFile(depth_shader_vs).c_str(), "vertexMain",
				video::EVST_VS_1_1,
				readShaderFile(depth_shader_fs).c_str(), "pixelMain",
				video::EPST_PS_1_2, m_shadow_depth_entity_cb);

		if (depth_shader_entities == -1) {
			// upsi, something went wrong loading shader.
			delete m_shadow_depth_entity_cb;
			m_shadow_depth_entity_cb = nullptr;
			m_shadows_enabled = false;
			m_shadows_supported = false;
			errorstream << "Error compiling shadow mapping shader (dynamic)." << std::endl;
			return;
		}

		// HACK, TODO: investigate this better
		// Grab the material renderer once more so minetest doesn't crash
		// on exit
		m_driver->getMaterialRenderer(depth_shader_entities)->grab();
	}

	if (mixcsm_shader == -1) {
		std::string depth_shader_vs = getShaderPath("shadow_shaders", "pass2_vertex.glsl");
		if (depth_shader_vs.empty()) {
			m_shadows_supported = false;
			errorstream << "Error cascade shadow mapping fs shader not found." << std::endl;
			return;
		}

		std::string depth_shader_fs = getShaderPath("shadow_shaders", "pass2_fragment.glsl");
		if (depth_shader_fs.empty()) {
			m_shadows_supported = false;
			errorstream << "Error cascade shadow mapping fs shader not found." << std::endl;
			return;
		}
		m_shadow_mix_cb = new shadowScreenQuadCB();
		m_screen_quad = new shadowScreenQuad();
		mixcsm_shader = gpu->addHighLevelShaderMaterial(
				readShaderFile(depth_shader_vs).c_str(), "vertexMain",
				video::EVST_VS_1_1,
				readShaderFile(depth_shader_fs).c_str(), "pixelMain",
				video::EPST_PS_1_2, m_shadow_mix_cb);

		m_screen_quad->getMaterial().MaterialType =
				(video::E_MATERIAL_TYPE)mixcsm_shader;

		if (mixcsm_shader == -1) {
			// upsi, something went wrong loading shader.
			delete m_shadow_mix_cb;
			delete m_screen_quad;
			m_shadows_supported = false;
			errorstream << "Error compiling cascade shadow mapping shader." << std::endl;
			return;
		}

		// HACK, TODO: investigate this better
		// Grab the material renderer once more so minetest doesn't crash
		// on exit
		m_driver->getMaterialRenderer(mixcsm_shader)->grab();
	}

	if (m_shadow_map_colored && depth_shader_trans == -1) {
		std::string depth_shader_vs = getShaderPath("shadow_shaders", "pass1_trans_vertex.glsl");
		if (depth_shader_vs.empty()) {
			m_shadows_supported = false;
			errorstream << "Error shadow mapping vs shader not found." << std::endl;
			return;
		}
		std::string depth_shader_fs = getShaderPath("shadow_shaders", "pass1_trans_fragment.glsl");
		if (depth_shader_fs.empty()) {
			m_shadows_supported = false;
			errorstream << "Error shadow mapping fs shader not found." << std::endl;
			return;
		}
		m_shadow_depth_trans_cb = new ShadowDepthShaderCB();

		depth_shader_trans = gpu->addHighLevelShaderMaterial(
				readShaderFile(depth_shader_vs).c_str(), "vertexMain",
				video::EVST_VS_1_1,
				readShaderFile(depth_shader_fs).c_str(), "pixelMain",
				video::EPST_PS_1_2, m_shadow_depth_trans_cb);

		if (depth_shader_trans == -1) {
			// upsi, something went wrong loading shader.
			delete m_shadow_depth_trans_cb;
			m_shadow_depth_trans_cb = nullptr;
			m_shadow_map_colored = false;
			m_shadows_supported = false;
			errorstream << "Error compiling colored shadow mapping shader." << std::endl;
			return;
		}

		// HACK, TODO: investigate this better
		// Grab the material renderer once more so minetest doesn't crash
		// on exit
		m_driver->getMaterialRenderer(depth_shader_trans)->grab();
	}
}

std::string ShadowRenderer::readShaderFile(const std::string &path)
{
	std::string prefix;
	if (m_shadow_map_colored)
		prefix.append("#define COLORED_SHADOWS 1\n");
	prefix.append("#line 0\n");

	std::string content;
	fs::ReadFile(path, content);

	return prefix + content;
}

ShadowRenderer *createShadowRenderer(IrrlichtDevice *device, Client *client)
{
	// disable if unsupported
	if (g_settings->getBool("enable_dynamic_shadows") && (
		device->getVideoDriver()->getDriverType() != video::EDT_OPENGL ||
		!g_settings->getBool("enable_shaders"))) {
		g_settings->setBool("enable_dynamic_shadows", false);
	}

	if (g_settings->getBool("enable_shaders") &&
			g_settings->getBool("enable_dynamic_shadows")) {
		ShadowRenderer *shadow_renderer = new ShadowRenderer(device, client);
		shadow_renderer->initialize();
		return shadow_renderer;
	}

	return nullptr;
}
