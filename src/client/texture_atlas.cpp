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

#include "client/texture_atlas.h"
#include "settings.h"
#include "noise.h"
#include "log.h"


/*bool TileInfo::operator==(const TileInfo &other_info)
{
	if (tex != other_info.tex)
		return false;

	if (anim.frame_count != other_info.anim.frame_count ||
		anim.frame_length_ms != other_info.anim.frame_length_ms)
		return false;

	for (int i = 0; i < anim.frame_count; i++)
		if (anim.frames[i] != other_info.anim.frames[i])
			return false;

	return true;
}*/


TextureAtlas::TextureAtlas(video::IVideoDriver *vdrv, ITextureSource *src, std::vector<TileLayer*> &layers)//, v3s16 blockpos)
{
	m_driver = vdrv;

	m_tsrc = src;

	u32 atlas_size = 0;

	// Tile layers are only unique
	std::vector<TileLayer *> unique_layers;

	// Create tile infos for unique tile layers
	for (auto &layer : layers) {
		// If the given layer was already transformed to the TileInfo,
		// save the index at this tile info
		bool is_layer_collected = false;
		for (u32 i = 0; i < unique_layers.size(); i++)
			if (unique_layers[i]->texture == layer->texture) {
				layer->atlas_tile_info_index = i;
				is_layer_collected = true;
				break;
			}

		if (is_layer_collected)
			continue;
		/*if (layer->material_flags & MATERIAL_FLAG_CRACK) {
			std::ostringstream os(std::ios::binary);
			os << m_tsrc->getTextureName(layer->texture_id) << "^[crack";
			if (layer->material_flags & MATERIAL_FLAG_CRACK_OVERLAY)
				os << "o";
			u8 tiles = layer->scale;
			if (tiles > 1)
				os << ":" << (u32)tiles;
			os << ":" << (u32)layer->animation_frame_count << ":";
			tile_info.crack_modifier = os.str();
			tile_info.crack_texture = m_tsrc->getTextureForMesh(
					os.str() + "0",
					&layer->texture_id);
		}*/

		TileInfo tile_info;

		if (layer->animation_frame_count > 1)
			tile_info.tex = (*layer->frames)[0].texture;
		else
			tile_info.tex = layer->texture;

		if (tile_info.tex) {
			core::dimension2du size = tile_info.tex->getSize();
			tile_info.width = size.Width;
			tile_info.height = size.Height;
		}

		if (layer->animation_frame_count > 1) {
			tile_info.anim.frame_length_ms = layer->animation_frame_length_ms;
			tile_info.anim.frame_count = layer->animation_frame_count;

			//if (g_settings->getBool("desynchronize_mapblock_texture_animation"))
			//	tile_info.anim.frame_offset =
			//			100000 * (2.0 + noise3d(
			//			blockpos.X, blockpos.Y,
			//			blockpos.Z, 0));

			for (auto &frame : *layer->frames)
				tile_info.anim.frames.push_back(frame.texture);
		}

		layer->atlas_tile_info_index = m_tiles_infos.size();
		m_tiles_infos.push_back(tile_info);
		unique_layers.push_back(layer);

		atlas_size += (tile_info.width * tile_info.height);
	}

	u32 atlas_side = std::min(getClosestPowerOfTwo(std::sqrt((f32)atlas_size)), 16384u);

	packTextures(atlas_side);

	// Extending the atlas width twice for allocating the additional texture space for cracks
	m_atlas_texture = m_driver->addTexture(core::dimension2du(atlas_side*2, atlas_side), "DiffuseAtlas", video::ECF_A32B32G32R32F);

	for (auto &info : m_tiles_infos)
		if (info.tex)
			m_atlas_texture->drawToSubImage(info.x, info.y, info.width, info.height, info.tex);

	if (m_driver->getTextureCreationFlag(video::ETCF_CREATE_MIP_MAPS)) {
		// Each tile in both sides of the atlas must have not less 2 pixels in a mip
		u32 min_pixels_count = m_tiles_infos.size()*4;
		m_atlas_texture->regenerateMipMapLevels(0, 0, min_pixels_count);
	}
}

u32 TextureAtlas::getClosestPowerOfTwo(f32 num)
{
	return std::pow(2u, (u32)std::ceil(std::log2(num)));
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

	areas.push_back(TileInfo(0, 0, side, side));

	u32 counter = 0;
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
					areas.push_back(TileInfo(area.x + info->width, area.y, dw, info->height));

				if (dh > 0)
					areas.push_back(TileInfo(area.x, area.y + info->height, area.width, dh));

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

		//infostream << "pack() info id = " << counter << ", x = " << info->x << ", y = " << info->y << ", width = " << info->width << ", height: " << info->height << std::endl;
		counter++;
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
			m_atlas_texture->drawToSubImage(tile.x, tile.y, tile.width, tile.height, tile.anim.frames[new_frame]);
		}
	}

	if (m_driver->getTextureCreationFlag(video::ETCF_CREATE_MIP_MAPS) && has_animated_tiles) {
		// Each tile in both sides of the atlas must have not less 2 pixels in a mip
		u32 min_pixels_count = m_tiles_infos.size()*4;
		m_atlas_texture->regenerateMipMapLevels(0, 0, min_pixels_count);
	}
}

void TextureAtlas::updateCrackAnimations(int new_crack)
{
	// No any cracked mapblocks
	if (m_crack_tiles.empty()) {
		m_last_crack = -1;
		return;
	}

	// New crack is old yet
	if (new_crack == m_last_crack)
		return;

	m_last_crack = new_crack;

	core::dimension2du atlas_size = m_atlas_texture->getSize();

	bool has_crack_tiles = false;

	for (const auto &p : m_crack_tiles) {
		std::string s = p.second + itos(new_crack);
		u32 new_texture_id = 0;
		video::ITexture *new_texture =
			m_tsrc->getTextureForMesh(s, &new_texture_id);

		TileInfo &tile = m_tiles_infos.at(p.first);
		if (new_texture) {
			has_crack_tiles = true;
			m_atlas_texture->drawToSubImage(tile.x + atlas_size.Width/2, tile.y, tile.width, tile.height, new_texture);
		}
	}

	if (m_driver->getTextureCreationFlag(video::ETCF_CREATE_MIP_MAPS) && has_crack_tiles) {
		// Each tile in both sides of the atlas must have not less 2 pixels in a mip
		u32 min_pixels_count = m_tiles_infos.size()*8;
		m_atlas_texture->regenerateMipMapLevels(0, 0, min_pixels_count);
	}
}
