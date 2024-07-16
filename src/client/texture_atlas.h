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
#include <map>
#include <vector>
#include "client/tile.h"
#include "client/texturesource.h"

/*struct UVPair {
	v2f uv1;
	v2f uv2;

	UVPair() = default;

	UVPair(v2f _uv1, v2f _uv2)
	: uv1(_uv1), uv2(_uv2)
	{}
};*/

struct AnimationInfo {
	int frame_length_ms = 0;
	int frame_count = 0;

	std::vector<video::ITexture*> frames;

	int cur_frame = 0;
	int frame_offset = 0;
};

struct TileInfo {
	int x, y;

	int width, height;

	video::ITexture *tex;

	AnimationInfo anim;

	//std::string crack_modifier;
	//video::ITexture *crack_tex;

	TileInfo() = default;

	TileInfo(int _x, int _y, int _width, int _height)
		: x(_x), y(_y), width(_width), height(_height)
	{}

	//bool operator==(const TileInfo &other_info);
};

/*struct TextureAnimation {
	int frame_length_ms = 0;
	int frame_count = 0;
	std::vector<video::ITexture*> frames;
	bool operator==(const TextureAnimation &other_anim)
	{
		if (frame_count != other_anim.frame_count ||
			frame_length_ms != other_anim.frame_length_ms)
			return false;
		bool same = true;
		for (u32 i = 0; i < frame_count; i++)
			if (frames[i] != other_anim.frames[i]) {
				same = false;
				break;
			}
		return same;
	}
};*/

class TextureAtlas {
	video::IVideoDriver *m_driver;
	ITextureSource *m_tsrc;

	video::ITexture *m_atlas_texture;

	std::vector<TileInfo> m_tiles_infos;

	// Mapping of the m_tiles_infos index and texture string
	// for the corresponding tile
	std::map<u32, std::string> m_crack_tiles;

	// Number of last crack
	int m_last_crack = -1;

	// Position of last mapblock that was cracked
	v3s16 m_crack_anim_last_block_pos;

public:
	TextureAtlas(video::IVideoDriver *vdrv, ITextureSource *src, std::vector<TileLayer*> &layers);

	~TextureAtlas()
	{
		m_driver->removeTexture(m_atlas_texture);
	}

	video::ITexture *getTexture() const
	{
		return m_atlas_texture;
	}

	core::dimension2du getTextureSize() const
	{
		return m_atlas_texture->getSize();
	}

	const TileInfo &getTileInfo(u32 i) const
	{
		return m_tiles_infos.at(i);
	}

	u32 getClosestPowerOfTwo(f32 num);

	bool canFit(const TileInfo &area, const TileInfo &tex);

	void insertCrackTile(u32 i, std::string s)
	{
		m_crack_tiles.insert({i, s});
	}

	void clearCrackTiles()
	{
		m_crack_tiles.clear();
	}

	void setCrackAnimLastBlockPos(v3s16 block_pos)
	{
		m_crack_anim_last_block_pos = block_pos;
	}

	v3s16 getCrackAnimLastBlockPos() const
	{
		return m_crack_anim_last_block_pos;
	}

	void packTextures(int side);

	void updateAnimations(f32 time);

	void updateCrackAnimations(int new_crack);
};
