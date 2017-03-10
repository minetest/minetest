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

#include "settings.h"
#include "wieldmesh.h"
#include "inventory.h"
#include "client.h"
#include "itemdef.h"
#include "nodedef.h"
#include "mesh.h"
#include "mapblock_mesh.h"
#include "client/tile.h"
#include "log.h"
#include "util/numeric.h"
#include <map>
#include <IMeshManipulator.h>

#define WIELD_SCALE_FACTOR 30.0
#define WIELD_SCALE_FACTOR_EXTRUDED 40.0

#define MIN_EXTRUSION_MESH_RESOLUTION 16
#define MAX_EXTRUSION_MESH_RESOLUTION 512

static scene::IMesh *createExtrusionMesh(int resolution_x, int resolution_y)
{
	const f32 r = 0.5;

	scene::IMeshBuffer *buf = new scene::SMeshBuffer();
	video::SColor c(255,255,255,255);
	v3f scale(1.0, 1.0, 0.1);

	// Front and back
	{
		video::S3DVertex vertices[8] = {
			// z-
			video::S3DVertex(-r,+r,-r, 0,0,-1, c, 0,0),
			video::S3DVertex(+r,+r,-r, 0,0,-1, c, 1,0),
			video::S3DVertex(+r,-r,-r, 0,0,-1, c, 1,1),
			video::S3DVertex(-r,-r,-r, 0,0,-1, c, 0,1),
			// z+
			video::S3DVertex(-r,+r,+r, 0,0,+1, c, 0,0),
			video::S3DVertex(-r,-r,+r, 0,0,+1, c, 0,1),
			video::S3DVertex(+r,-r,+r, 0,0,+1, c, 1,1),
			video::S3DVertex(+r,+r,+r, 0,0,+1, c, 1,0),
		};
		u16 indices[12] = {0,1,2,2,3,0,4,5,6,6,7,4};
		buf->append(vertices, 8, indices, 12);
	}

	f32 pixelsize_x = 1 / (f32) resolution_x;
	f32 pixelsize_y = 1 / (f32) resolution_y;

	for (int i = 0; i < resolution_x; ++i) {
		f32 pixelpos_x = i * pixelsize_x - 0.5;
		f32 x0 = pixelpos_x;
		f32 x1 = pixelpos_x + pixelsize_x;
		f32 tex0 = (i + 0.1) * pixelsize_x;
		f32 tex1 = (i + 0.9) * pixelsize_x;
		video::S3DVertex vertices[8] = {
			// x-
			video::S3DVertex(x0,-r,-r, -1,0,0, c, tex0,1),
			video::S3DVertex(x0,-r,+r, -1,0,0, c, tex1,1),
			video::S3DVertex(x0,+r,+r, -1,0,0, c, tex1,0),
			video::S3DVertex(x0,+r,-r, -1,0,0, c, tex0,0),
			// x+
			video::S3DVertex(x1,-r,-r, +1,0,0, c, tex0,1),
			video::S3DVertex(x1,+r,-r, +1,0,0, c, tex0,0),
			video::S3DVertex(x1,+r,+r, +1,0,0, c, tex1,0),
			video::S3DVertex(x1,-r,+r, +1,0,0, c, tex1,1),
		};
		u16 indices[12] = {0,1,2,2,3,0,4,5,6,6,7,4};
		buf->append(vertices, 8, indices, 12);
	}
	for (int i = 0; i < resolution_y; ++i) {
		f32 pixelpos_y = i * pixelsize_y - 0.5;
		f32 y0 = -pixelpos_y - pixelsize_y;
		f32 y1 = -pixelpos_y;
		f32 tex0 = (i + 0.1) * pixelsize_y;
		f32 tex1 = (i + 0.9) * pixelsize_y;
		video::S3DVertex vertices[8] = {
			// y-
			video::S3DVertex(-r,y0,-r, 0,-1,0, c, 0,tex0),
			video::S3DVertex(+r,y0,-r, 0,-1,0, c, 1,tex0),
			video::S3DVertex(+r,y0,+r, 0,-1,0, c, 1,tex1),
			video::S3DVertex(-r,y0,+r, 0,-1,0, c, 0,tex1),
			// y+
			video::S3DVertex(-r,y1,-r, 0,+1,0, c, 0,tex0),
			video::S3DVertex(-r,y1,+r, 0,+1,0, c, 0,tex1),
			video::S3DVertex(+r,y1,+r, 0,+1,0, c, 1,tex1),
			video::S3DVertex(+r,y1,-r, 0,+1,0, c, 1,tex0),
		};
		u16 indices[12] = {0,1,2,2,3,0,4,5,6,6,7,4};
		buf->append(vertices, 8, indices, 12);
	}

	// Create mesh object
	scene::SMesh *mesh = new scene::SMesh();
	mesh->addMeshBuffer(buf);
	buf->drop();
	scaleMesh(mesh, scale);  // also recalculates bounding box
	return mesh;
}

/*
	Caches extrusion meshes so that only one of them per resolution
	is needed. Also caches one cube (for convenience).

	E.g. there is a single extrusion mesh that is used for all
	16x16 px images, another for all 256x256 px images, and so on.

	WARNING: Not thread safe. This should not be a problem since
	rendering related classes (such as WieldMeshSceneNode) will be
	used from the rendering thread only.
*/
class ExtrusionMeshCache: public IReferenceCounted
{
public:
	// Constructor
	ExtrusionMeshCache()
	{
		for (int resolution = MIN_EXTRUSION_MESH_RESOLUTION;
				resolution <= MAX_EXTRUSION_MESH_RESOLUTION;
				resolution *= 2) {
			m_extrusion_meshes[resolution] =
				createExtrusionMesh(resolution, resolution);
		}
		m_cube = createCubeMesh(v3f(1.0, 1.0, 1.0));
	}
	// Destructor
	virtual ~ExtrusionMeshCache()
	{
		for (std::map<int, scene::IMesh*>::iterator
				it = m_extrusion_meshes.begin();
				it != m_extrusion_meshes.end(); ++it) {
			it->second->drop();
		}
		m_cube->drop();
	}
	// Get closest extrusion mesh for given image dimensions
	// Caller must drop the returned pointer
	scene::IMesh* create(core::dimension2d<u32> dim)
	{
		// handle non-power of two textures inefficiently without cache
		if (!is_power_of_two(dim.Width) || !is_power_of_two(dim.Height)) {
			return createExtrusionMesh(dim.Width, dim.Height);
		}

		int maxdim = MYMAX(dim.Width, dim.Height);

		std::map<int, scene::IMesh*>::iterator
			it = m_extrusion_meshes.lower_bound(maxdim);

		if (it == m_extrusion_meshes.end()) {
			// no viable resolution found; use largest one
			it = m_extrusion_meshes.find(MAX_EXTRUSION_MESH_RESOLUTION);
			sanity_check(it != m_extrusion_meshes.end());
		}

		scene::IMesh *mesh = it->second;
		mesh->grab();
		return mesh;
	}
	// Returns a 1x1x1 cube mesh with one meshbuffer (material) per face
	// Caller must drop the returned pointer
	scene::IMesh* createCube()
	{
		m_cube->grab();
		return m_cube;
	}

private:
	std::map<int, scene::IMesh*> m_extrusion_meshes;
	scene::IMesh *m_cube;
};

ExtrusionMeshCache *g_extrusion_mesh_cache = NULL;


WieldMeshSceneNode::WieldMeshSceneNode(
		scene::ISceneNode *parent,
		scene::ISceneManager *mgr,
		s32 id,
		bool lighting
):
	scene::ISceneNode(parent, mgr, id),
	m_meshnode(NULL),
	m_material_type(video::EMT_TRANSPARENT_ALPHA_CHANNEL_REF),
	m_lighting(lighting),
	m_bounding_box(0.0, 0.0, 0.0, 0.0, 0.0, 0.0)
{
	m_enable_shaders = g_settings->getBool("enable_shaders");
	m_anisotropic_filter = g_settings->getBool("anisotropic_filter");
	m_bilinear_filter = g_settings->getBool("bilinear_filter");
	m_trilinear_filter = g_settings->getBool("trilinear_filter");

	// If this is the first wield mesh scene node, create a cache
	// for extrusion meshes (and a cube mesh), otherwise reuse it
	if (g_extrusion_mesh_cache == NULL)
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
		g_extrusion_mesh_cache = NULL;
}

void WieldMeshSceneNode::setCube(const TileSpec tiles[6],
			v3f wield_scale, ITextureSource *tsrc)
{
	scene::IMesh *cubemesh = g_extrusion_mesh_cache->createCube();
	changeToMesh(cubemesh);
	cubemesh->drop();

	m_meshnode->setScale(wield_scale * WIELD_SCALE_FACTOR);

	// Customize materials
	for (u32 i = 0; i < m_meshnode->getMaterialCount(); ++i) {
		assert(i < 6);
		video::SMaterial &material = m_meshnode->getMaterial(i);
		if (tiles[i].animation_frame_count == 1) {
			material.setTexture(0, tiles[i].texture);
		} else {
			FrameSpec animation_frame = tiles[i].frames[0];
			material.setTexture(0, animation_frame.texture);
		}
		tiles[i].applyMaterialOptions(material);
	}
}

void WieldMeshSceneNode::setExtruded(const std::string &imagename,
		v3f wield_scale, ITextureSource *tsrc, u8 num_frames)
{
	video::ITexture *texture = tsrc->getTexture(imagename);
	if (!texture) {
		changeToMesh(NULL);
		return;
	}

	core::dimension2d<u32> dim = texture->getSize();
	// Detect animation texture and pull off top frame instead of using entire thing
	if (num_frames > 1) {
		u32 frame_height = dim.Height / num_frames;
		dim = core::dimension2d<u32>(dim.Width, frame_height);
	}
	scene::IMesh *mesh = g_extrusion_mesh_cache->create(dim);
	changeToMesh(mesh);
	mesh->drop();

	m_meshnode->setScale(wield_scale * WIELD_SCALE_FACTOR_EXTRUDED);

	// Customize material
	video::SMaterial &material = m_meshnode->getMaterial(0);
	material.setTexture(0, tsrc->getTextureForMesh(imagename));
	material.TextureLayer[0].TextureWrapU = video::ETC_CLAMP_TO_EDGE;
	material.TextureLayer[0].TextureWrapV = video::ETC_CLAMP_TO_EDGE;
	material.MaterialType = m_material_type;
	material.setFlag(video::EMF_BACK_FACE_CULLING, true);
	// Enable bi/trilinear filtering only for high resolution textures
	if (dim.Width > 32) {
		material.setFlag(video::EMF_BILINEAR_FILTER, m_bilinear_filter);
		material.setFlag(video::EMF_TRILINEAR_FILTER, m_trilinear_filter);
	} else {
		material.setFlag(video::EMF_BILINEAR_FILTER, false);
		material.setFlag(video::EMF_TRILINEAR_FILTER, false);
	}
	material.setFlag(video::EMF_ANISOTROPIC_FILTER, m_anisotropic_filter);
	// mipmaps cause "thin black line" artifacts
#if (IRRLICHT_VERSION_MAJOR >= 1 && IRRLICHT_VERSION_MINOR >= 8) || IRRLICHT_VERSION_MAJOR >= 2
	material.setFlag(video::EMF_USE_MIP_MAPS, false);
#endif
	if (m_enable_shaders) {
		material.setTexture(2, tsrc->getShaderFlagsTexture(false));
	}
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

	if (m_enable_shaders) {
		u32 shader_id = shdrsrc->getShader("wielded_shader", TILE_MATERIAL_BASIC, NDT_NORMAL);
		m_material_type = shdrsrc->getShaderInfo(shader_id).material;
	}

	// Color-related
	m_colors.clear();
	video::SColor basecolor = idef->getItemstackColor(item, client);

	// If wield_image is defined, it overrides everything else
	if (def.wield_image != "") {
		setExtruded(def.wield_image, def.wield_scale, tsrc, 1);
		m_colors.push_back(basecolor);
		return;
	}
	// Handle nodes
	// See also CItemDefManager::createClientCached()
	else if (def.type == ITEM_NODE) {
		if (f.mesh_ptr[0]) {
			// e.g. mesh nodes and nodeboxes
			changeToMesh(f.mesh_ptr[0]);
			// mesh_ptr[0] is pre-scaled by BS * f->visual_scale
			m_meshnode->setScale(
					def.wield_scale * WIELD_SCALE_FACTOR
					/ (BS * f.visual_scale));
		} else if (f.drawtype == NDT_AIRLIKE) {
			changeToMesh(NULL);
		} else if (f.drawtype == NDT_PLANTLIKE) {
			setExtruded(tsrc->getTextureName(f.tiles[0].texture_id), def.wield_scale, tsrc, f.tiles[0].animation_frame_count);
		} else if (f.drawtype == NDT_NORMAL || f.drawtype == NDT_ALLFACES) {
			setCube(f.tiles, def.wield_scale, tsrc);
		} else {
			MeshMakeData mesh_make_data(client, false);
			MapNode mesh_make_node(id, 255, 0);
			mesh_make_data.fillSingleNode(&mesh_make_node);
			MapBlockMesh mapblock_mesh(&mesh_make_data, v3s16(0, 0, 0));
			changeToMesh(mapblock_mesh.getMesh());
			translateMesh(m_meshnode->getMesh(), v3f(-BS, -BS, -BS));
			m_meshnode->setScale(
					def.wield_scale * WIELD_SCALE_FACTOR
					/ (BS * f.visual_scale));
		}
		u32 material_count = m_meshnode->getMaterialCount();
		if (material_count > 6) {
			errorstream << "WieldMeshSceneNode::setItem: Invalid material "
				"count " << material_count << ", truncating to 6" << std::endl;
			material_count = 6;
		}
		for (u32 i = 0; i < material_count; ++i) {
			const TileSpec *tile = &(f.tiles[i]);
			video::SMaterial &material = m_meshnode->getMaterial(i);
			material.setFlag(video::EMF_BACK_FACE_CULLING, true);
			material.setFlag(video::EMF_BILINEAR_FILTER, m_bilinear_filter);
			material.setFlag(video::EMF_TRILINEAR_FILTER, m_trilinear_filter);
			bool animated = (tile->animation_frame_count > 1);
			if (animated) {
				FrameSpec animation_frame = tile->frames[0];
				material.setTexture(0, animation_frame.texture);
			} else {
				material.setTexture(0, tile->texture);
			}
			m_colors.push_back(tile->has_color ? tile->color : basecolor);
			material.MaterialType = m_material_type;
			if (m_enable_shaders) {
				if (tile->normal_texture) {
					if (animated) {
						FrameSpec animation_frame = tile->frames[0];
						material.setTexture(1, animation_frame.normal_texture);
					} else {
						material.setTexture(1, tile->normal_texture);
					}
				}
				material.setTexture(2, tile->flags_texture);
			}
		}
		return;
	}
	else if (def.inventory_image != "") {
		setExtruded(def.inventory_image, def.wield_scale, tsrc, 1);
		m_colors.push_back(basecolor);
		return;
	}

	// no wield mesh found
	changeToMesh(NULL);
}

void WieldMeshSceneNode::setColor(video::SColor c)
{
	assert(!m_lighting);
	scene::IMesh *mesh=m_meshnode->getMesh();
	if (mesh == NULL)
		return;

	u8 red = c.getRed();
	u8 green = c.getGreen();
	u8 blue = c.getBlue();
	u32 mc = mesh->getMeshBufferCount();
	for (u32 j = 0; j < mc; j++) {
		video::SColor bc(0xFFFFFFFF);
		if (m_colors.size() > j)
			bc = m_colors[j];
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
	if (mesh == NULL) {
		scene::IMesh *dummymesh = g_extrusion_mesh_cache->createCube();
		m_meshnode->setVisible(false);
		m_meshnode->setMesh(dummymesh);
		dummymesh->drop();  // m_meshnode grabbed it
	} else {
		if (m_lighting) {
			m_meshnode->setMesh(mesh);
		} else {
			/*
				Lighting is disabled, this means the caller can (and probably will)
				call setColor later. We therefore need to clone the mesh so that
				setColor will only modify this scene node's mesh, not others'.
			*/
			scene::IMeshManipulator *meshmanip = SceneManager->getMeshManipulator();
			scene::IMesh *new_mesh = meshmanip->createMeshCopy(mesh);
			m_meshnode->setMesh(new_mesh);
			new_mesh->drop();  // m_meshnode grabbed it
		}
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

	scene::IMesh *mesh;

	// If inventory_image is defined, it overrides everything else
	if (def.inventory_image != "") {
		mesh = getExtrudedMesh(tsrc, def.inventory_image);
		result->mesh = mesh;
		result->buffer_colors.push_back(
			std::pair<bool, video::SColor>(false, video::SColor(0xFFFFFFFF)));
	} else if (def.type == ITEM_NODE) {
		if (f.mesh_ptr[0]) {
			mesh = cloneMesh(f.mesh_ptr[0]);
			scaleMesh(mesh, v3f(0.12, 0.12, 0.12));
		} else if (f.drawtype == NDT_PLANTLIKE) {
			mesh = getExtrudedMesh(tsrc,
				tsrc->getTextureName(f.tiles[0].texture_id));
		} else if (f.drawtype == NDT_NORMAL || f.drawtype == NDT_ALLFACES
			|| f.drawtype == NDT_LIQUID || f.drawtype == NDT_FLOWINGLIQUID) {
			mesh = cloneMesh(g_extrusion_mesh_cache->createCube());
			scaleMesh(mesh, v3f(1.2, 1.2, 1.2));
		} else {
			MeshMakeData mesh_make_data(client, false);
			MapNode mesh_make_node(id, 255, 0);
			mesh_make_data.fillSingleNode(&mesh_make_node);
			MapBlockMesh mapblock_mesh(&mesh_make_data, v3s16(0, 0, 0));
			mesh = cloneMesh(mapblock_mesh.getMesh());
			translateMesh(mesh, v3f(-BS, -BS, -BS));
			scaleMesh(mesh, v3f(0.12, 0.12, 0.12));

			u32 mc = mesh->getMeshBufferCount();
			for (u32 i = 0; i < mc; ++i) {
				video::SMaterial &material1 =
					mesh->getMeshBuffer(i)->getMaterial();
				video::SMaterial &material2 =
					mapblock_mesh.getMesh()->getMeshBuffer(i)->getMaterial();
				material1.setTexture(0, material2.getTexture(0));
				material1.setTexture(1, material2.getTexture(1));
				material1.setTexture(2, material2.getTexture(2));
				material1.setTexture(3, material2.getTexture(3));
				material1.MaterialType = material2.MaterialType;
			}
		}

		u32 mc = mesh->getMeshBufferCount();
		for (u32 i = 0; i < mc; ++i) {
			const TileSpec *tile = &(f.tiles[i]);
			scene::IMeshBuffer *buf = mesh->getMeshBuffer(i);
			result->buffer_colors.push_back(
				std::pair<bool, video::SColor>(tile->has_color, tile->color));
			colorizeMeshBuffer(buf, &tile->color);
			video::SMaterial &material = buf->getMaterial();
			material.MaterialType = video::EMT_TRANSPARENT_ALPHA_CHANNEL;
			material.setFlag(video::EMF_BILINEAR_FILTER, false);
			material.setFlag(video::EMF_TRILINEAR_FILTER, false);
			material.setFlag(video::EMF_BACK_FACE_CULLING, true);
			material.setFlag(video::EMF_LIGHTING, false);
			if (tile->animation_frame_count > 1) {
				FrameSpec animation_frame = tile->frames[0];
				material.setTexture(0, animation_frame.texture);
			} else {
				material.setTexture(0, tile->texture);
			}
		}

		rotateMeshXZby(mesh, -45);
		rotateMeshYZby(mesh, -30);
		result->mesh = mesh;
	}
}

scene::IMesh * getExtrudedMesh(ITextureSource *tsrc,
		const std::string &imagename)
{
	video::ITexture *texture = tsrc->getTextureForMesh(imagename);
	if (!texture) {
		return NULL;
	}

	core::dimension2d<u32> dim = texture->getSize();
	scene::IMesh *mesh = cloneMesh(g_extrusion_mesh_cache->create(dim));

	// Customize material
	video::SMaterial &material = mesh->getMeshBuffer(0)->getMaterial();
	material.setTexture(0, tsrc->getTexture(imagename));
	material.TextureLayer[0].TextureWrapU = video::ETC_CLAMP_TO_EDGE;
	material.TextureLayer[0].TextureWrapV = video::ETC_CLAMP_TO_EDGE;
	material.setFlag(video::EMF_BILINEAR_FILTER, false);
	material.setFlag(video::EMF_TRILINEAR_FILTER, false);
	material.setFlag(video::EMF_BACK_FACE_CULLING, true);
	material.setFlag(video::EMF_LIGHTING, false);
	material.MaterialType = video::EMT_TRANSPARENT_ALPHA_CHANNEL;
	scaleMesh(mesh, v3f(2.0, 2.0, 2.0));

	return mesh;
}
