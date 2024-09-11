/*
Minetest
Copyright (C) 2024 Andrey, Andrey2470T <andreyt2203@gmail.com>
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

#include "client/client.h"
#include "client/texture_atlas.h"
#include "settings.h"
#include "log.h"


TextureAtlas::TextureAtlas(Client *client, u32 atlas_area, u32 min_area_tile, std::vector<TileInfo> &tiles_infos)
	: m_driver(client->getSceneManager()->getVideoDriver()), m_tsrc(client->getTextureSource())
{
	m_tiles_infos = std::move(tiles_infos);
	m_mip_maps = g_settings->getBool("mip_map");

	u32 atlas_side = std::pow(2u, (u32)std::ceil(std::log2(std::sqrt((f32)atlas_area))));

	packTextures(atlas_side);

	// `atlas_side*2` doubles the texture area for crack tiles
	core::dimension2du atlas_size(atlas_side*2, atlas_side);
	video::ECOLOR_FORMAT atlas_format = video::ECF_A32B32G32R32F;

	m_texture = m_driver->addTexture(atlas_size, "Atlas", atlas_format);

	for (auto &info : m_tiles_infos)
		if (info.tex)
			m_texture->drawToSubImage(info.x, info.y, info.width, info.height, info.tex);

	if (m_mip_maps) {
		m_max_mip_level = (u32)std::ceil(std::log2(std::sqrt((f32)min_area_tile)));

		m_texture->regenerateMipMapLevels(nullptr, 0, m_max_mip_level);
	}
}

bool TextureAtlas::canFit(const TileInfo &area, const TileInfo &tex)
{
	return (tex.width <= area.width && tex.height <= area.height);
}

void TextureAtlas::packTextures(int side)
{
	std::vector<TileInfo *> sorted_infos;

	for (auto &info : m_tiles_infos)
		sorted_infos.push_back(&info);

	std::sort(sorted_infos.begin(), sorted_infos.end(),
		[] (TileInfo *info1, TileInfo *info2)
		{
			return (info1->width*info1->height > info2->width*info2->height);
		});
	std::vector<TileInfo> areas;

	areas.emplace_back(0, 0, side, side);

	for (auto &info : sorted_infos) {
		bool packed = false;

		for (u32 i = 0; i < areas.size(); i++) {
			TileInfo area = areas[i];
			if (canFit(area, *info)) {
				packed = true;

				info->x = area.x;
				info->y = area.y;

				int dw = area.width - info->width;
				int dh = area.height - info->height;

				areas.erase(areas.begin()+i);

				if (dw > 0)
					areas.emplace_back(area.x + info->width, area.y, dw, info->height);

				if (dh > 0)
					areas.emplace_back(area.x, area.y + info->height, area.width, dh);

				std::sort(areas.begin(), areas.end(),
					[] (TileInfo area1, TileInfo area2)
					{
						return (area1.width*area1.height < area2.width*area2.height);
					});
				break;
			}
		}

		if (!packed)
			return;
	}
}

void TextureAtlas::updateAnimations(f32 time)
{
	if (m_tiles_infos.empty())
		return;

	bool has_animated_tiles = false;

	for (auto &tile : m_tiles_infos) {
		if (tile.anim.frame_count == 0)
			continue;

		int new_frame = (int)(time * 1000 / tile.anim.frame_length_ms
				+ tile.anim.frame_offset) % tile.anim.frame_count;

		if (new_frame == tile.anim.cur_frame)
			continue;

		tile.anim.cur_frame = new_frame;

		if (tile.anim.frames.at(new_frame)) {
			has_animated_tiles = true;
			m_texture->drawToSubImage(tile.x, tile.y, tile.width, tile.height, tile.anim.frames[new_frame]);
		}
	}

	if (m_mip_maps && has_animated_tiles)
		m_texture->regenerateMipMapLevels(nullptr, 0, m_max_mip_level);
}

void TextureAtlas::updateCrackAnimations(int new_crack)
{
	if (new_crack == m_last_crack)
		// New crack is old yet
		return;

	MutexAutoLock crack_tiles_lock(m_crack_tiles_mutex);

	if (new_crack == -1) {
		// Animation has finished
		m_crack_tiles.clear();
		m_last_crack = -1;

		return;
	}

	m_last_crack = new_crack;

	core::dimension2du atlas_size = m_texture->getSize();

	bool has_crack_tiles = false;

	for (const auto &crack_tile : m_crack_tiles) {
		std::string s = crack_tile.second + itos(new_crack);
		u32 new_texture_id = 0;
		video::ITexture *new_texture =
			m_tsrc->getTextureForMesh(s, &new_texture_id);

		TileInfo &tile = m_tiles_infos.at(crack_tile.first);
		if (new_texture) {
			has_crack_tiles = true;
			m_texture->drawToSubImage(tile.x + atlas_size.Width/2, tile.y, tile.width, tile.height, new_texture);
		}
	}

	if (m_mip_maps && has_crack_tiles)
		m_texture->regenerateMipMapLevels(nullptr, 0, m_max_mip_level);
}

void AtlasBuilder::buildAtlas(Client *client, std::list<TileLayer*> &layers)
{
	u32 atlas_area = 0;

	// Necessary for defining the max count of the atlas mips to avoid the artifacts with it
	int min_area_tile = 0;

	video::IVideoDriver *vdrv = client->getSceneManager()->getVideoDriver();
	u32 max_texture_size = (u32)(vdrv->getDriverAttributes().getAttributeAsInt("MaxTextureSize"));
	u32 max_texture_area = std::pow(max_texture_size, 2);

	// Tile layers with unique ITextures
	std::vector<TileLayer *> unique_layers;
	std::vector<TileInfo> tiles_infos;

	for (; !layers.empty(); layers.pop_front()) {
		TileLayer *layer = layers.front();
		// If for the layer the TileInfo was already created,
		// just save the index at this TileInfo and omit the iteration
		bool is_layer_collected = false;
		for (u32 i = 0; i < unique_layers.size(); i++)
			if (unique_layers[i]->texture == layer->texture) {
				layer->atlas_index = m_atlases.size();
				layer->atlas_tile_info_index = i;

				is_layer_collected = true;
				break;
			}

		if (is_layer_collected)
			continue;

		TileInfo tile_info;

		if (layer->animation_frame_count > 1)
			tile_info.tex = (*layer->frames)[0].texture;
		else
			tile_info.tex = layer->texture;

		if (tile_info.tex) {
			core::dimension2du size = tile_info.tex->getSize();
			tile_info.width = size.Width;
			tile_info.height = size.Height;

			if ((atlas_area + tile_info.width * tile_info.height) * 2 > max_texture_area)
				break;
		}

		if (layer->animation_frame_count > 1) {
			tile_info.anim.frame_length_ms = layer->animation_frame_length_ms;
			tile_info.anim.frame_count = layer->animation_frame_count;

			for (auto &frame : *layer->frames)
				tile_info.anim.frames.push_back(frame.texture);
		}

		layer->atlas_index = m_atlases.size();
		layer->atlas_tile_info_index = tiles_infos.size();
		tiles_infos.push_back(tile_info);
		unique_layers.push_back(layer);

		if (min_area_tile == 0)
			min_area_tile = tile_info.width * tile_info.height;
		else
			min_area_tile = std::min(min_area_tile, tile_info.width * tile_info.height);

		atlas_area += (tile_info.width * tile_info.height);
	}

	m_atlases.push_back(std::make_unique<TextureAtlas>(client, atlas_area, (u32)min_area_tile, tiles_infos));

	// Recursively create new atlases if remain tiles didn't fit in the current one
	if (!layers.empty())
		buildAtlas(client, layers);
}

void AtlasBuilder::updateAnimations(f32 time)
{
	for (auto &atlas : m_atlases)
		atlas->updateAnimations(time);
}

void AtlasBuilder::updateCrackAnimations(int new_crack)
{
	for (auto &atlas : m_atlases)
		atlas->updateCrackAnimations(new_crack);
}
