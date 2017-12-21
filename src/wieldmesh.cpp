/*
Minetest
Copyright (C) 2010-2014 celeron55, Perttu Ahola <celeron55@gmail.com>

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

#include "wieldmesh.h"
#include "settings.h"
#include "shader.h"
#include "inventory.h"
#include "client.h"
#include "itemdef.h"
#include "nodedef.h"
#include "mesh.h"
#include "content_mapblock.h"
#include "mapblock_mesh.h"
#include "client/tile.h"
#include "log.h"
#include "util/numeric.h"
#include <map>
#include <IMeshManipulator.h>

#define WIELD_SCALE_FACTOR 30.0
#define WIELD_SCALE_FACTOR_EXTRUDED 40.0


WieldMeshSceneNode::WieldMeshSceneNode(scene::ISceneManager *mgr, s32 id, bool lighting):
	scene::ISceneNode(mgr->getRootSceneNode(), mgr, id),
	m_material_type(video::EMT_TRANSPARENT_ALPHA_CHANNEL_REF),
	m_lighting(lighting)
{
	m_enable_shaders = g_settings->getBool("enable_shaders");
	m_anisotropic_filter = g_settings->getBool("anisotropic_filter");
	m_bilinear_filter = g_settings->getBool("bilinear_filter");
	m_trilinear_filter = g_settings->getBool("trilinear_filter");

	// If this is the first wield mesh scene node, create a cache
	// for extrusion meshes (and a cube mesh), otherwise reuse it
	if (!g_extrusion_mesh_cache)
		g_extrusion_mesh_cache = new ExtrusionMeshCache();
	else
		g_extrusion_mesh_cache->grab();

	// Disable bounding box culling for this scene node
	// since we won't calculate the bounding box.
	setAutomaticCulling(scene::EAC_OFF);

	// Create the child scene node
	scene::IMesh *dummymesh = g_extrusion_mesh_cache->createCube();
	m_meshnode = SceneManager->addMeshSceneNode(dummymesh, this, -1);
	m_meshnode->setReadOnlyMaterials(false);
	m_meshnode->setVisible(false);
	dummymesh->drop(); // m_meshnode grabbed it
}

WieldMeshSceneNode::~WieldMeshSceneNode()
{
	sanity_check(g_extrusion_mesh_cache);
	if (g_extrusion_mesh_cache->drop())
		g_extrusion_mesh_cache = nullptr;
}

void WieldMeshSceneNode::setCube(const ContentFeatures &f,
			v3f wield_scale)
{
	scene::IMesh *cubemesh = g_extrusion_mesh_cache->createCube();
	scene::SMesh *copy = cloneMesh(cubemesh);
	cubemesh->drop();
	postProcessNodeMesh(copy, f, false, true, &m_material_type, &m_colors, true);
	changeToMesh(copy);
	copy->drop();
	m_meshnode->setScale(wield_scale * WIELD_SCALE_FACTOR);
}

void WieldMeshSceneNode::setExtruded(const std::string &imagename,
	const std::string &overlay_name, v3f wield_scale, ITextureSource *tsrc,
	u8 num_frames)
{
	video::ITexture *texture = tsrc->getTexture(imagename);
	if (!texture) {
		changeToMesh(nullptr);
		return;
	}
	scene::IMesh *mesh = createExtrusionMesh(texture);
/*
	video::ITexture *overlay_texture =
		overlay_name.empty() ? NULL : tsrc->getTexture(overlay_name);

	core::dimension2d<u32> dim = texture->getSize();
	// Detect animation texture and pull off top frame instead of using entire thing
	if (num_frames > 1) {
		u32 frame_height = dim.Height / num_frames;
		dim = core::dimension2d<u32>(dim.Width, frame_height);
	}
	scene::IMesh *original = g_extrusion_mesh_cache->create(dim);
	scene::SMesh *mesh = cloneMesh(original);
	original->drop();*/
	//set texture
// 	mesh->getMeshBuffer(0)->getMaterial().setTexture(0, texture);
/*	if (overlay_texture) {
		scene::IMeshBuffer *copy = cloneMeshBuffer(mesh->getMeshBuffer(0));
		copy->getMaterial().setTexture(0, overlay_texture);
		mesh->addMeshBuffer(copy);
		copy->drop();
	}
*/	changeToMesh(mesh);
	mesh->drop();

	m_meshnode->setScale(wield_scale * WIELD_SCALE_FACTOR_EXTRUDED);

	// Customize materials
	for (u32 layer = 0; layer < m_meshnode->getMaterialCount(); layer++) {
		video::SMaterial &material = m_meshnode->getMaterial(layer);
		material.TextureLayer[0].TextureWrapU = video::ETC_CLAMP_TO_EDGE;
		material.TextureLayer[0].TextureWrapV = video::ETC_CLAMP_TO_EDGE;
		material.MaterialType = m_material_type;
		material.setFlag(video::EMF_BACK_FACE_CULLING, true);
		// Enable bi/trilinear filtering only for high resolution textures
//		if (dim.Width > 32) {
//			material.setFlag(video::EMF_BILINEAR_FILTER, m_bilinear_filter);
//			material.setFlag(video::EMF_TRILINEAR_FILTER, m_trilinear_filter);
//		} else {
			material.setFlag(video::EMF_BILINEAR_FILTER, false);
			material.setFlag(video::EMF_TRILINEAR_FILTER, false);
//		}
		material.setFlag(video::EMF_ANISOTROPIC_FILTER, m_anisotropic_filter);
		// mipmaps cause "thin black line" artifacts
#if (IRRLICHT_VERSION_MAJOR >= 1 && IRRLICHT_VERSION_MINOR >= 8) || IRRLICHT_VERSION_MAJOR >= 2
		material.setFlag(video::EMF_USE_MIP_MAPS, false);
#endif
		if (m_enable_shaders) {
			material.setTexture(2, tsrc->getShaderFlagsTexture(false));
		}
	}
}

scene::SMesh *createSpecialNodeMesh(Client *client, content_t id, std::vector<ItemPartColor> *colors)
{
	MeshMakeData mesh_make_data(client, false, false);
	MeshCollector collector(false);
	mesh_make_data.setSmoothLighting(false);
	MapblockMeshGenerator gen(&mesh_make_data, &collector);
	gen.renderSingle(id);
	colors->clear();
	scene::SMesh *mesh = new scene::SMesh();
	for (auto &prebuffers : collector.prebuffers)
		for (PreMeshBuffer &p : prebuffers) {
			if (p.layer.material_flags & MATERIAL_FLAG_ANIMATION) {
				const FrameSpec &frame = (*p.layer.frames)[0];
				p.layer.texture = frame.texture;
				p.layer.normal_texture = frame.normal_texture;
			}
			for (video::S3DVertex &v : p.vertices)
				v.Color.setAlpha(255);
			scene::SMeshBuffer *buf = new scene::SMeshBuffer();
			buf->Material.setTexture(0, p.layer.texture);
			p.layer.applyMaterialOptions(buf->Material);
			mesh->addMeshBuffer(buf);
			buf->append(&p.vertices[0], p.vertices.size(),
					&p.indices[0], p.indices.size());
			buf->drop();
			colors->push_back(
				ItemPartColor(p.layer.has_color, p.layer.color));
		}
	return mesh;
}

void WieldMeshSceneNode::setItem(const ItemStack &item, Client *client)
{
	ITextureSource *tsrc = client->getTextureSource();
	IItemDefManager *idef = client->getItemDefManager();
	IShaderSource *shdrsrc = client->getShaderSource();
	INodeDefManager *ndef = client->getNodeDefManager();
	const ItemDefinition &def = item.getDefinition(idef);
	const ContentFeatures &f = ndef->get(def.name);
	content_t id = ndef->getId(def.name);

	scene::SMesh *mesh = nullptr;

	if (m_enable_shaders) {
		u32 shader_id = shdrsrc->getShader("wielded_shader", TILE_MATERIAL_BASIC, NDT_NORMAL);
		m_material_type = shdrsrc->getShaderInfo(shader_id).material;
	}

	// Color-related
	m_colors.clear();
	m_base_color = idef->getItemstackColor(item, client);

	// If wield_image is defined, it overrides everything else
	if (!def.wield_image.empty()) {
		setExtruded(def.wield_image, def.wield_overlay, def.wield_scale, tsrc,
			1);
		m_colors.emplace_back();
		// overlay is white, if present
		m_colors.emplace_back(true, video::SColor(0xFFFFFFFF));
		return;
	}

	// Handle nodes
	// See also CItemDefManager::createClientCached()
	if (def.type == ITEM_NODE) {
		if (f.mesh_ptr[0]) {
			// e.g. mesh nodes and nodeboxes
			mesh = cloneMesh(f.mesh_ptr[0]);
			postProcessNodeMesh(mesh, f, m_enable_shaders, true,
				&m_material_type, &m_colors);
			changeToMesh(mesh);
			mesh->drop();
			// mesh is pre-scaled by BS * f->visual_scale
			m_meshnode->setScale(
					def.wield_scale * WIELD_SCALE_FACTOR
					/ (BS * f.visual_scale));
		} else {
			switch (f.drawtype) {
				case NDT_AIRLIKE: {
					changeToMesh(nullptr);
					break;
				}
				case NDT_PLANTLIKE: {
					setExtruded(tsrc->getTextureName(f.tiles[0].layers[0].texture_id),
						tsrc->getTextureName(f.tiles[0].layers[1].texture_id),
						def.wield_scale, tsrc,
						f.tiles[0].layers[0].animation_frame_count);
					// Add color
					const TileLayer &l0 = f.tiles[0].layers[0];
					m_colors.emplace_back(l0.has_color, l0.color);
					const TileLayer &l1 = f.tiles[0].layers[1];
					m_colors.emplace_back(l1.has_color, l1.color);
					break;
				}
				case NDT_PLANTLIKE_ROOTED: {
					setExtruded(tsrc->getTextureName(f.special_tiles[0].layers[0].texture_id),
						"", def.wield_scale, tsrc,
						f.special_tiles[0].layers[0].animation_frame_count);
					// Add color
					const TileLayer &l0 = f.special_tiles[0].layers[0];
					m_colors.emplace_back(l0.has_color, l0.color);
					break;
				}
				case NDT_NORMAL:
				case NDT_ALLFACES:
				case NDT_LIQUID:
				case NDT_FLOWINGLIQUID: {
					setCube(f, def.wield_scale);
					break;
				}
				default: {
					mesh = createSpecialNodeMesh(client, id, &m_colors);
					changeToMesh(mesh);
					mesh->drop();
					m_meshnode->setScale(
							def.wield_scale * WIELD_SCALE_FACTOR
							/ (BS * f.visual_scale));
				}
			}
		}
		u32 material_count = m_meshnode->getMaterialCount();
		for (u32 i = 0; i < material_count; ++i) {
			video::SMaterial &material = m_meshnode->getMaterial(i);
			material.MaterialType = m_material_type;
			material.setFlag(video::EMF_BACK_FACE_CULLING, true);
			material.setFlag(video::EMF_BILINEAR_FILTER, m_bilinear_filter);
			material.setFlag(video::EMF_TRILINEAR_FILTER, m_trilinear_filter);
		}
		return;
	}
	else if (!def.inventory_image.empty()) {
		setExtruded(def.inventory_image, def.inventory_overlay, def.wield_scale,
			tsrc, 1);
		m_colors.emplace_back();
		// overlay is white, if present
		m_colors.emplace_back(true, video::SColor(0xFFFFFFFF));
		return;
	}

	// no wield mesh found
	changeToMesh(nullptr);
}

void WieldMeshSceneNode::setColor(video::SColor c)
{
	assert(!m_lighting);
	scene::IMesh *mesh = m_meshnode->getMesh();
	if (!mesh)
		return;

	u8 red = c.getRed();
	u8 green = c.getGreen();
	u8 blue = c.getBlue();
	u32 mc = mesh->getMeshBufferCount();
	for (u32 j = 0; j < mc; j++) {
		video::SColor bc(m_base_color);
		if ((m_colors.size() > j) && (m_colors[j].override_base))
			bc = m_colors[j].color;
		video::SColor buffercolor(255,
			bc.getRed() * red / 255,
			bc.getGreen() * green / 255,
			bc.getBlue() * blue / 255);
		scene::IMeshBuffer *buf = mesh->getMeshBuffer(j);
		colorizeMeshBuffer(buf, &buffercolor);
	}
}

void WieldMeshSceneNode::render()
{
	// note: if this method is changed to actually do something,
	// you probably should implement OnRegisterSceneNode as well
}

void WieldMeshSceneNode::changeToMesh(scene::IMesh *mesh)
{
	if (!mesh) {
		scene::IMesh *dummymesh = g_extrusion_mesh_cache->createCube();
		m_meshnode->setVisible(false);
		m_meshnode->setMesh(dummymesh);
		dummymesh->drop();  // m_meshnode grabbed it
	} else {
		m_meshnode->setMesh(mesh);
	}

	m_meshnode->setMaterialFlag(video::EMF_LIGHTING, m_lighting);
	// need to normalize normals when lighting is enabled (because of setScale())
	m_meshnode->setMaterialFlag(video::EMF_NORMALIZE_NORMALS, m_lighting);
	m_meshnode->setVisible(true);
}

void getItemMesh(Client *client, const ItemStack &item, ItemMesh *result)
{
	ITextureSource *tsrc = client->getTextureSource();
	IItemDefManager *idef = client->getItemDefManager();
	INodeDefManager *ndef = client->getNodeDefManager();
	const ItemDefinition &def = item.getDefinition(idef);
	const ContentFeatures &f = ndef->get(def.name);
	content_t id = ndef->getId(def.name);

	if (!g_extrusion_mesh_cache) {
		g_extrusion_mesh_cache = new ExtrusionMeshCache();
	} else {
		g_extrusion_mesh_cache->grab();
	}

	scene::SMesh *mesh = nullptr;

	// Shading is on by default
	result->needs_shading = true;

	// If inventory_image is defined, it overrides everything else
	if (!def.inventory_image.empty()) {
		mesh = getExtrudedMesh(tsrc, def.inventory_image,
			def.inventory_overlay);
		result->buffer_colors.emplace_back();
		// overlay is white, if present
		result->buffer_colors.emplace_back(true, video::SColor(0xFFFFFFFF));
		// Items with inventory images do not need shading
		result->needs_shading = false;
	} else if (def.type == ITEM_NODE) {
		if (f.mesh_ptr[0]) {
			mesh = cloneMesh(f.mesh_ptr[0]);
			scaleMesh(mesh, v3f(0.12, 0.12, 0.12));
			postProcessNodeMesh(mesh, f, false, false, nullptr,
				&result->buffer_colors);
		} else {
			switch (f.drawtype) {
				case NDT_PLANTLIKE: {
					mesh = getExtrudedMesh(tsrc,
						tsrc->getTextureName(f.tiles[0].layers[0].texture_id),
						tsrc->getTextureName(f.tiles[0].layers[1].texture_id));
					// Add color
					const TileLayer &l0 = f.tiles[0].layers[0];
					result->buffer_colors.emplace_back(l0.has_color, l0.color);
					const TileLayer &l1 = f.tiles[0].layers[1];
					result->buffer_colors.emplace_back(l1.has_color, l1.color);
					break;
				}
				case NDT_PLANTLIKE_ROOTED: {
					mesh = getExtrudedMesh(tsrc,
						tsrc->getTextureName(f.special_tiles[0].layers[0].texture_id), "");
					// Add color
					const TileLayer &l0 = f.special_tiles[0].layers[0];
					result->buffer_colors.emplace_back(l0.has_color, l0.color);
					break;
				}
				case NDT_NORMAL:
				case NDT_ALLFACES:
				case NDT_LIQUID:
				case NDT_FLOWINGLIQUID: {
					scene::IMesh *cube = g_extrusion_mesh_cache->createCube();
					mesh = cloneMesh(cube);
					cube->drop();
					scaleMesh(mesh, v3f(1.2, 1.2, 1.2));
					// add overlays
					postProcessNodeMesh(mesh, f, false, false, nullptr,
						&result->buffer_colors);
					break;
				}
				default: {
					mesh = createSpecialNodeMesh(client, id, &result->buffer_colors);
					scaleMesh(mesh, v3f(0.12, 0.12, 0.12));
				}
			}
		}

		u32 mc = mesh->getMeshBufferCount();
		for (u32 i = 0; i < mc; ++i) {
			scene::IMeshBuffer *buf = mesh->getMeshBuffer(i);
			video::SMaterial &material = buf->getMaterial();
			material.MaterialType = video::EMT_TRANSPARENT_ALPHA_CHANNEL;
			material.setFlag(video::EMF_BILINEAR_FILTER, false);
			material.setFlag(video::EMF_TRILINEAR_FILTER, false);
			material.setFlag(video::EMF_BACK_FACE_CULLING, true);
			material.setFlag(video::EMF_LIGHTING, false);
		}

		rotateMeshXZby(mesh, -45);
		rotateMeshYZby(mesh, -30);
	}
	result->mesh = mesh;
}



scene::SMesh *getExtrudedMesh(ITextureSource *tsrc,
	const std::string &imagename, const std::string &overlay_name)
{
	// check textures
	video::ITexture *texture = tsrc->getTextureForMesh(imagename);
	if (!texture) {
		return NULL;
	}
	video::ITexture *overlay_texture =
		(overlay_name.empty()) ? NULL : tsrc->getTexture(overlay_name);

	// get mesh
	core::dimension2d<u32> dim = texture->getSize();
	scene::IMesh *original = g_extrusion_mesh_cache->create(dim);
	scene::SMesh *mesh = cloneMesh(original);
	original->drop();

	//set texture
	mesh->getMeshBuffer(0)->getMaterial().setTexture(0,
		tsrc->getTexture(imagename));
	if (overlay_texture) {
		scene::IMeshBuffer *copy = cloneMeshBuffer(mesh->getMeshBuffer(0));
		copy->getMaterial().setTexture(0, overlay_texture);
		mesh->addMeshBuffer(copy);
		copy->drop();
	}
	// Customize materials
	for (u32 layer = 0; layer < mesh->getMeshBufferCount(); layer++) {
		video::SMaterial &material = mesh->getMeshBuffer(layer)->getMaterial();
		material.TextureLayer[0].TextureWrapU = video::ETC_CLAMP_TO_EDGE;
		material.TextureLayer[0].TextureWrapV = video::ETC_CLAMP_TO_EDGE;
		material.setFlag(video::EMF_BILINEAR_FILTER, false);
		material.setFlag(video::EMF_TRILINEAR_FILTER, false);
		material.setFlag(video::EMF_BACK_FACE_CULLING, true);
		material.setFlag(video::EMF_LIGHTING, false);
		material.MaterialType = video::EMT_TRANSPARENT_ALPHA_CHANNEL;
	}
	scaleMesh(mesh, v3f(2.0, 2.0, 2.0));

	return mesh;
}

void postProcessNodeMesh(scene::SMesh *mesh, const ContentFeatures &f,
	bool use_shaders, bool set_material, const video::E_MATERIAL_TYPE *mattype,
	std::vector<ItemPartColor> *colors, bool apply_scale)
{
	u32 mc = mesh->getMeshBufferCount();
	// Allocate colors for existing buffers
	colors->clear();
	for (u32 i = 0; i < mc; ++i)
		colors->push_back(ItemPartColor());

	for (u32 i = 0; i < mc; ++i) {
		const TileSpec *tile = &(f.tiles[i]);
		scene::IMeshBuffer *buf = mesh->getMeshBuffer(i);
		for (int layernum = 0; layernum < MAX_TILE_LAYERS; layernum++) {
			const TileLayer *layer = &tile->layers[layernum];
			if (layer->texture_id == 0)
				continue;
			if (layernum != 0) {
				scene::IMeshBuffer *copy = cloneMeshBuffer(buf);
				copy->getMaterial() = buf->getMaterial();
				mesh->addMeshBuffer(copy);
				copy->drop();
				buf = copy;
				colors->push_back(
					ItemPartColor(layer->has_color, layer->color));
			} else {
				(*colors)[i] = ItemPartColor(layer->has_color, layer->color);
			}
			video::SMaterial &material = buf->getMaterial();
			if (set_material)
				layer->applyMaterialOptions(material);
			if (mattype) {
				material.MaterialType = *mattype;
			}
			if (layer->animation_frame_count > 1) {
				const FrameSpec &animation_frame = (*layer->frames)[0];
				material.setTexture(0, animation_frame.texture);
			} else {
				material.setTexture(0, layer->texture);
			}
			if (use_shaders) {
				if (layer->normal_texture) {
					if (layer->animation_frame_count > 1) {
						const FrameSpec &animation_frame = (*layer->frames)[0];
						material.setTexture(1, animation_frame.normal_texture);
					} else
						material.setTexture(1, layer->normal_texture);
				}
				material.setTexture(2, layer->flags_texture);
			}
			if (apply_scale && tile->world_aligned) {
				u32 n = buf->getVertexCount();
				for (u32 k = 0; k != n; ++k)
					buf->getTCoords(k) /= layer->scale;
			}
		}
	}
}
