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
#include "nodedef.h"
#include "log.h"


TileInfo::TileInfo(const TileLayer &layer)
{
	if (layer.animation_frame_count > 1)
		tex = (*layer.frames)[0].texture;
	else
		tex = layer.texture;

	if (tex) {
		core::dimension2du size = tex->getSize();
		width = size.Width;
		height = size.Height;
	}

	if (layer.animation_frame_count > 1) {
		anim.frame_length_ms = layer.animation_frame_length_ms;
		anim.frame_count = layer.animation_frame_count;

		for (auto &frame : *layer.frames)
			anim.frames.push_back(frame.texture);
	}
}

/*!
 * Constructor.
 */
TextureAtlas::TextureAtlas(Client *client, u32 atlas_area, u32 max_mip_level, u32 atlas_n, std::vector<u32> &tiles_infos_refs)
	: m_driver(client->getSceneManager()->getVideoDriver()), m_tsrc(client->getTextureSource()),
	  m_builder(client->getNodeDefManager()->getAtlasBuilder()), m_tiles_infos_refs(std::move(tiles_infos_refs)),
	  m_max_mip_level(max_mip_level)
{
	m_mip_maps = g_settings->getBool("mip_map");
	m_filtering = g_settings->getBool("bilinear_filter") ||
		g_settings->getBool("trilinear_filter") || g_settings->getBool("anisotropic_filter");

	if (m_filtering) {
		u32 tile_frame_thickness = m_max_mip_level + 4;

		for (auto &info_i : m_tiles_infos_refs) {
			TileInfo &info = m_builder->getTileInfo(info_i);

			if (info.tex)
				info.tex = recreateTextureForFiltering(info.tex, tile_frame_thickness);

			for (u16 frame_i = 0; frame_i < info.anim.frames.size(); frame_i++)
				info.anim.frames[frame_i] = recreateTextureForFiltering(info.anim.frames[frame_i], tile_frame_thickness);
		}
	}

	u32 atlas_side = std::pow(2u, (u32)std::ceil(std::log2(std::sqrt((f32)atlas_area))));

	packTextures(atlas_side);

	core::dimension2du atlas_size(atlas_side*2, atlas_side);
	video::ECOLOR_FORMAT atlas_format = video::ECF_A32B32G32R32F;

	std::string atlas_name = "Atlas" + atlas_n;
	m_texture = m_driver->addTexture(atlas_size, atlas_name, atlas_format);

	m_texture_cache_id = m_tsrc->cacheExistentTexture(atlas_name, m_texture);

	for (auto &info_i : m_tiles_infos_refs) {
		TileInfo &info = m_builder->getTileInfo(info_i);

		if (info.tex)
			m_texture->drawToSubImage(info.x, info.y, info.width, info.height, info.tex);
	}

	if (m_mip_maps)
		m_texture->regenerateMipMapLevels(nullptr, 0, m_max_mip_level);
}

/*!
 * Returns a precalculated thickness of the frame around each tile in pixels.
 */
u32 TextureAtlas::getFrameThickness() const
{
	if (!m_filtering)
		return 0;

	return m_max_mip_level + 4;
}

/*!
 * Generates a new more extended texture for some atlas tile.
 * The extension happens due to the adding the pixel frame.
 */
video::ITexture *TextureAtlas::recreateTextureForFiltering(video::ITexture *tex, u32 ext_thickness)
{
	// Download the pixel data of the tile from GPU into IImage
	core::dimension2du old_size = tex->getSize();
	video::ECOLOR_FORMAT color_format = tex->getColorFormat();
	video::IImage *img = m_driver->createImageFromData(color_format,
		old_size, tex->lock());

	tex->unlock();

	core::dimension2du new_size(
		old_size.Width + 2 * ext_thickness,
		old_size.Height + 2 * ext_thickness
	);

	// Create the increased image from the previous IImage with a pixel frame of the certain thickness
	video::IImage *ext_img = m_driver->createImage(color_format, new_size);

	v2s32 img_offset(ext_thickness, ext_thickness);
	img->copyTo(ext_img, img_offset);

	v2s32 offset_dirs[4] = {
		v2s32(-1, 0),
		v2s32(0, 1),
		v2s32(1, 0),
		v2s32(0, -1)
	};

	v2s32 start_offsets[4] = {
		img_offset,
		img_offset + v2s32(0, old_size.Height-1),
		img_offset + v2s32(old_size.Width-1, 0),
		img_offset
	};

	core::recti pixel_sides[4] = {
		core::recti(v2s32(0, 0), v2s32(1, old_size.Height)),	// Left
		core::recti(v2s32(0, old_size.Height-1), v2s32(old_size.Width, old_size.Height)),	// Top
		core::recti(v2s32(old_size.Width-1, 0), v2s32(old_size.Width, old_size.Height)),	// Right
		core::recti(v2s32(0, 0), v2s32(old_size.Width, 1))		// Bottom
	};
	// Draw the pixel stripes at each side with 'offset' thickness
	//img->copyTo(ext_img, v2s32(10, 6), core::recti(v2s32(0, 0), v2s32(0, old_size.Height)));
	for (u16 side = 0; side < 4; side++)
		for (int offset = 1; offset <= (int)ext_thickness; offset++)
			img->copyTo(ext_img, start_offsets[side] + offset_dirs[side] * offset,
				pixel_sides[side]);

	video::ITexture *new_tex = m_driver->addTexture("IncreasedTexture", ext_img);
	img->drop();
	ext_img->drop();

	return new_tex;
}

/*!
 * Packs all collected unique tiles within the atlas area.
 * Algorithm for packing used is 'divide-and-conquer'.
*/
void TextureAtlas::packTextures(int side)
{
	std::vector<TileInfo *> sorted_infos;

	for (auto &info_i : m_tiles_infos_refs)
		sorted_infos.push_back(&m_builder->getTileInfo(info_i));

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

/*!
 * Draws the next frames on the tiles in the atlas having an animation.
 */
void TextureAtlas::updateAnimations(f32 time)
{
	if (m_tiles_infos_refs.empty())
		return;

	bool has_animated_tiles = false;

	for (auto &tile_i : m_tiles_infos_refs) {
		TileInfo &tile = m_builder->getTileInfo(tile_i);

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

/*
 * Draws the tiles with overlayed crack textures of some level atop in the right half of the atlas.
 */
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

		TileInfo &tile = m_builder->getTileInfo(crack_tile.first);
		if (new_texture) {
			has_crack_tiles = true;
			m_texture->drawToSubImage(tile.x + atlas_size.Width/2, tile.y, tile.width, tile.height, new_texture);
		}
	}

	if (m_mip_maps && has_crack_tiles)
		m_texture->regenerateMipMapLevels(nullptr, 0, m_max_mip_level);
}

void AtlasBuilder::buildAtlases(Client *client, std::vector<TileInfo> &infos)
{
	m_tiles_infos = std::move(infos);
	m_mip_maps = g_settings->getBool("mip_map");
	m_filtering = g_settings->getBool("bilinear_filter") ||
		g_settings->getBool("trilinear_filter") || g_settings->getBool("anisotropic_filter");

	for (u32 tile_i = 0; tile_i < m_tiles_infos.size(); tile_i++)
		m_sorted_tiles_infos.push_back(tile_i);

	std::sort(m_sorted_tiles_infos.begin(), m_sorted_tiles_infos.end(),
		[&] (u32 i1, u32 i2)
		{
			TileInfo &info1 = m_tiles_infos.at(i1);
			TileInfo &info2 = m_tiles_infos.at(i2);
			return (info1.width * info1.height < info2.width * info2.height);
		});

	buildAtlas(client);
}

void AtlasBuilder::buildAtlas(Client *client)
{
	u32 atlas_area = 0;

	// Necessary for defining the max count of the atlas mips to avoid the artifacts with it
	auto min_tile = m_tiles_infos.at(m_sorted_tiles_infos.at(m_fill_start_index));
	int min_area_tile = min_tile.width * min_tile.height;

	u32 max_mip_level = 0;
	u32 tile_frame_thickness = 0;

	if (m_mip_maps)
		max_mip_level = (u32)std::ceil(std::log2(std::sqrt((f32)min_area_tile)));

	if (m_filtering)
		tile_frame_thickness = max_mip_level + 4;

	video::IVideoDriver *vdrv = client->getSceneManager()->getVideoDriver();
	u32 max_texture_size = (u32)(vdrv->getDriverAttributes().getAttributeAsInt("MaxTextureSize"));
	u32 max_texture_area = std::pow(max_texture_size, 2);

	std::vector<u32> tiles_infos_indices;

	u32 i = m_fill_start_index;
	for (; i < m_sorted_tiles_infos.size(); i++)
	{
		TileInfo &info = m_tiles_infos.at(m_sorted_tiles_infos.at(i));

		int width = info.width + 2 * tile_frame_thickness;
		int height = info.height + 2 * tile_frame_thickness;

		int tile_area = width * height;

		// doubling the area is for taking into account its crack area also
		if ((atlas_area + tile_area) * 2 > max_texture_area) {
			m_fill_start_index = i;
			break;
		}

		info.width = width;
		info.height = height;
		atlas_area += tile_area;

		tiles_infos_indices.push_back(m_sorted_tiles_infos.at(i));
	}

	u32 atlas_n = m_atlases.size();
	m_atlases.emplace_back(std::make_unique<TextureAtlas>(client, atlas_area, max_mip_level, atlas_n, tiles_infos_indices));

	if (i < m_sorted_tiles_infos.size())
		buildAtlas(client);
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
