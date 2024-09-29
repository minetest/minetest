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

#pragma once

#include "irrlichttypes_extrabloated.h"
#include <IVideoDriver.h>
#include <memory>
#include <map>
#include <set>
#include <vector>
#include <mutex>
#include <cassert>
#include "client/tile.h"
#include "client/texturesource.h"
#include "threading/mutex_auto_lock.h"

/*!
 * Animation parameters for an atlas tile catching from TileLayer
 */
struct AnimationInfo
{
	u16 frame_length_ms = 0;
	u16 frame_count = 0;

	std::vector<video::ITexture*> frames;

	u16 cur_frame = 0;
	u16 frame_offset = 0;
};

/*!
 * Parameters for an atlas tile. Mostly catching from TileLayer.
 * The tile coordinates are anchored to the left top corner (in pixels).
 * Width and height are also in pixels.
 */
struct TileInfo
{
	int x, y;

	int width, height;

	video::ITexture *tex;

	AnimationInfo anim;

	TileInfo() = default;

	TileInfo(int _x, int _y, int _width, int _height)
		: x(_x), y(_y), width(_width), height(_height)
	{}

	TileInfo(const TileLayer &layer);

	bool operator==(const TileInfo &other_info)
	{
		return tex == other_info.tex;
	}
};

class Client;
class AtlasBuilder;

/*!
 * Texture atlas handler.
 */
class TextureAtlas
{
	video::IVideoDriver *m_driver;
	ITextureSource *m_tsrc;
	AtlasBuilder *m_builder;

	/*!
	 * Texture of the atlas.
	 * It is always power of two and square.
	 * It allocates a double space for crack tiles at the right side.
	 * When the filtering is enabled, it draws around each tile additional frames with some pixel thickness.
	 */
	video::ITexture *m_texture;

	/*!
	 * Saves all indices to tiles that will be drawn to the atlas.
	 */
	std::vector<u32> m_tiles_infos_refs;

	/*!
	 * Mappings of the m_tiles_infos index and texture string for the corresponding tile.
	 * This map is temporary, so when the crack animation has finished, it gets cleared.
	 */
	std::map<u32, std::string> m_crack_tiles;

	/*!
	 * Necessary as the main and mesh generation threads can access to it simultaneously.
	 */
	std::mutex m_crack_tiles_mutex;

	/*!
	 * Number of the last crack.
	 */
	int m_last_crack = -1;

	/*!
	 * Cached texture id inside TextureSource
	 */
	u32 m_texture_cache_id;
	/*!
	 * Highest mip map level before which the atlas mipmaps will be generated.
	 * Necessary for solving the problem of the "adjacent pixels pollution" when mips downscale.
	 */
	u32 m_max_mip_level = 0;

	bool m_mip_maps;
	bool m_filtering;

public:
	/*!
	 * Constructor.
	 */
	TextureAtlas(Client *client, u32 atlas_area, u32 max_mip_level, u32 atlas_n, std::vector<u32> &tiles_infos_refs);

	/*!
	 * Destructor.
	 */
	~TextureAtlas()
	{
		m_driver->removeTexture(m_texture);
	}

	video::ITexture *getTexture() const
	{
		return m_texture;
	}

	core::dimension2du getTextureSize() const
	{
		return m_texture->getSize();
	}

	u32 getTextureCacheId() const
	{
		return m_texture_cache_id;
	}

	bool contains(u32 index) const
	{
		auto find_i = std::find(m_tiles_infos_refs.begin(), m_tiles_infos_refs.end(), index);

		return find_i != m_tiles_infos_refs.end();
	}

	/*!
	 * Returns a precalculated thickness of the frame around each tile in pixels.
	 */
	u32 getFrameThickness() const;

	/*!
	 * Checks if 'tex' can be entirely put inside 'area'.
	 */
	bool canFit(const TileInfo &area, const TileInfo &tex)
	{
		return (tex.width <= area.width && tex.height <= area.height);
	}

	void insertCrackTile(u32 i, std::string s)
	{
		MutexAutoLock crack_tiles_lock(m_crack_tiles_mutex);

		m_crack_tiles.insert({i, s});
	}

	/*!
	 * Generates a new more extended texture for some atlas tile.
	 * The extension happens due to the adding the pixel frame.
	 */
	video::ITexture *recreateTextureForFiltering(video::ITexture *tex, u32 ext_thickness);

	/*!
	 * Packs all collected unique tiles within the atlas area.
	 * Algorithm for packing used is 'divide-and-conquer'.
	 */
	void packTextures(int side);

	/*!
	 * Draws the next frames on the tiles in the atlas having an animation.
	 */
	void updateAnimations(f32 time);

	/*
	 * Draws the tiles with overlayed crack textures of some level atop in the right half of the atlas.
	 */
	void updateCrackAnimations(int new_crack);
};

/*!
 * Abstraction handling all atlases.
 */
class AtlasBuilder
{
	std::vector<std::unique_ptr<TextureAtlas>> m_atlases;
	std::vector<TileInfo> m_tiles_infos;
	// Indices to m_tile_infos sorted in order of increasing the tiles sizes
	std::vector<u32> m_sorted_tiles_infos;

	// Index to m_sorted_tile_infos
	u32 m_fill_start_index = 0;

	bool m_mip_maps;
	bool m_filtering;

public:
	AtlasBuilder() = default;

	TextureAtlas *getAtlas(u32 tile_info_i) const {
		for (auto &atlas : m_atlases)
			if (atlas->contains(tile_info_i))
				return atlas.get();

		return nullptr;
	}

	TileInfo &getTileInfo(u32 i) const
	{
		return const_cast<TileInfo &>(m_tiles_infos.at(i));
	}

	void buildAtlases(Client *client, std::vector<TileInfo> &infos);
	void buildAtlas(Client *client);

	void updateAnimations(f32 time);
	void updateCrackAnimations(int new_crack);
};
