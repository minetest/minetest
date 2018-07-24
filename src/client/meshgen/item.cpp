/*
Minetest
Copyright (C) 2018 numzero, Lobachevskiy Vitaliy <numzer0@yandex.ru>

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

#include "item.h"
#include "client.h"
#include "collector.h"
#include "content_mapblock.h"
#include "itemdef.h"
#include "mesh.h"
#include "nodedef.h"

class ItemMeshGenerator
{
public:
	ItemMeshGenerator(Client *_client, const ItemStack &stack);
	~ItemMeshGenerator();
	void createInventoryMesh();
	void createWieldMesh();
	ItemMesh takeMesh() noexcept;

private:
	Client *client = nullptr;
	ITextureSource *tsrc = nullptr;
	const ItemDefinition *def = nullptr;
	const ContentFeatures *f = nullptr;
	content_t id = CONTENT_IGNORE;
	bool is_flat = true;

	scene::SMesh *mesh = nullptr;
	std::vector<ItemPartColor> colors;

	bool tryImage(const std::string &image, const std::string &overlay, bool extrude);
	bool tryNode();
	void createSpecialNodeMesh();
	void postProcessMesh();
	void postProcessNodeMesh();

};

ItemMesh ItemMeshSource::createInventoryItemMesh(const ItemStack &stack)
{
	ItemMeshGenerator generator(client, stack);
	generator.createInventoryMesh();
	return generator.takeMesh();
}

ItemMesh ItemMeshSource::createWieldItemMesh(const ItemStack &stack)
{
	ItemMeshGenerator generator(client, stack);
	generator.createWieldMesh();
	return generator.takeMesh();
}

ItemMeshGenerator::ItemMeshGenerator(Client *_client, const ItemStack &stack)
{
	client = _client;
	tsrc = client->getTextureSource();
	def = &stack.getDefinition(client->getItemDefManager());
}

ItemMeshGenerator::~ItemMeshGenerator()
{
	if (mesh)
		mesh->drop();
}

void ItemMeshGenerator::createSpecialNodeMesh()
{
	MeshMakeData mesh_make_data(client, false, false);
	MeshCollector collector;
	mesh_make_data.setSmoothLighting(false);
	MapblockMeshGenerator gen(&mesh_make_data, &collector);
	gen.renderSingle(id);
	mesh = new scene::SMesh();
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
			buf->append(&p.vertices[0], p.vertices.size(), &p.indices[0],
					p.indices.size());
			buf->drop();
			colors.emplace_back(p.layer.has_color, p.layer.color);
		}
}

bool ItemMeshGenerator::tryImage(
		const std::string &image, const std::string &overlay, bool extrude)
{
	if (image.empty())
		return false;
	video::ITexture *texture = tsrc->getTextureForMesh(image);
	if (!texture)
		return true;
	video::ITexture *overlay_texture = nullptr;
	if (!overlay.empty())
		overlay_texture = tsrc->getTexture(overlay);
	ItemMeshSource *imsrc = client->getItemMeshSource();
	if (extrude)
		mesh = imsrc->createExtrusionMesh(texture, overlay_texture);
	else
		mesh = imsrc->createFlatMesh(texture, overlay_texture);
	scaleMesh(mesh, v3f(2.0f));
	colors.emplace_back();
	colors.emplace_back(true, video::SColor(0xFFFFFFFF)); // overlay is not colored
	return true;
}

bool ItemMeshGenerator::tryNode()
{
	if (def->type != ITEM_NODE)
		return false;
	is_flat = false;
	const NodeDefManager *ndef = client->getNodeDefManager();
	id = ndef->getId(def->name);
	f = &ndef->get(id);
	if (f->mesh_ptr[0]) {
		mesh = cloneMesh(f->mesh_ptr[0]);
		scaleMesh(mesh, v3f(1.2f / BS));
		postProcessNodeMesh();
		return true;
	}
	switch (f->drawtype) {
	case NDT_NORMAL:
	case NDT_ALLFACES:
	case NDT_LIQUID:
	case NDT_FLOWINGLIQUID:
		mesh = createCubeMesh(v3f(1.2f));
		postProcessNodeMesh();
		break;
	default:
		createSpecialNodeMesh();
		scaleMesh(mesh, v3f(1.2f / BS));
		break;
	}
	return true;
}

void ItemMeshGenerator::postProcessNodeMesh()
{
	int n = mesh->getMeshBufferCount();
	colors.resize(n);
	for (int i = 0; i < n; ++i) {
		const TileSpec &tile = f->tiles[i];
		scene::IMeshBuffer *buf = mesh->getMeshBuffer(i);
		for (int layernum = 0; layernum < MAX_TILE_LAYERS; layernum++) {
			const TileLayer &layer = tile.layers[layernum];
			if (layer.texture_id == 0)
				continue;
			if (layernum != 0) {
				scene::IMeshBuffer *copy = cloneMeshBuffer(buf);
				mesh->addMeshBuffer(copy);
				copy->drop();
				buf = copy;
				colors.emplace_back(layer.has_color, layer.color);
			} else {
				colors[i] = ItemPartColor(layer.has_color, layer.color);
			}
			video::SMaterial &material = buf->getMaterial();
			layer.applyMaterialOptions(material);
			if (layer.animation_frame_count > 1) {
				const FrameSpec &animation_frame = (*layer.frames)[0];
				material.setTexture(0, animation_frame.texture);
			} else {
				material.setTexture(0, layer.texture);
			}
		}
	}
}

void ItemMeshGenerator::createInventoryMesh()
{
	static thread_local bool extrude_inv =
			g_settings->getBool("inventory_items_animations");
	// clang-format off
	// TODO: add custom mesh support
	tryImage(def->inventory_image, def->inventory_overlay, extrude_inv)
		|| tryNode();
	// clang-format on
	if (!mesh)
		return;
	postProcessMesh();
	if (is_flat)
		return;
	rotateMeshXZby(mesh, -45);
	rotateMeshYZby(mesh, -30);
}

void ItemMeshGenerator::createWieldMesh()
{
	// clang-format off
	tryImage(def->wield_image, def->wield_overlay, true)
		|| tryNode()
		|| tryImage(def->inventory_image, def->inventory_overlay, true);
	// clang-format on
	if (!mesh)
		return;
	postProcessMesh();
	scaleMesh(mesh, 2.0f * BS * def->wield_scale);
}

void ItemMeshGenerator::postProcessMesh()
{
	static thread_local bool anisotropic_filter =
			g_settings->getBool("anisotropic_filter");
	static thread_local bool bilinear_filter = g_settings->getBool("bilinear_filter");
	static thread_local bool trilinear_filter =
			g_settings->getBool("trilinear_filter");
	int n = mesh->getMeshBufferCount();
	for (int i = 0; i < n; ++i) {
		scene::IMeshBuffer *buf = mesh->getMeshBuffer(i);
		video::SMaterial &material = buf->getMaterial();
		material.MaterialType = video::EMT_TRANSPARENT_ALPHA_CHANNEL_REF;
		material.setFlag(video::EMF_LIGHTING, false);
		material.setFlag(video::EMF_NORMALIZE_NORMALS, false);
		material.setFlag(video::EMF_ANISOTROPIC_FILTER, anisotropic_filter);
		material.setFlag(video::EMF_BILINEAR_FILTER, bilinear_filter);
		material.setFlag(video::EMF_TRILINEAR_FILTER, trilinear_filter);
	}
}

ItemMesh ItemMeshGenerator::takeMesh() noexcept
{
	ItemMesh result;
	result.mesh = mesh;
	result.buffer_colors = std::move(colors);
	result.needs_shading = !is_flat;
	mesh = nullptr;
	return result;
}
