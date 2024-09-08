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
#include <mutex>
#include <cassert>
#include "client/tile.h"
#include "client/texturesource.h"
#include "threading/mutex_auto_lock.h"

struct AnimationInfo
{
	u16 frame_length_ms = 0;
	u16 frame_count = 0;

	std::vector<video::ITexture*> frames;

	u16 cur_frame = 0;
	u16 frame_offset = 0;
};

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
};

class Client;

class TextureAtlas
{
	video::IVideoDriver *m_driver;
	ITextureSource *m_tsrc;

	video::ITexture *m_texture;

	std::vector<TileInfo> m_tiles_infos;

	// Mapping of the m_tiles_infos index and texture string
	// for the corresponding tile
	std::map<u32, std::string> m_crack_tiles;

	std::mutex m_crack_tiles_mutex;

	// Number of last crack
	int m_last_crack = -1;

	// Highest mip map level
	u32 m_max_mip_level;

	bool m_mip_maps;

public:
	TextureAtlas(Client *client, u32 atlas_area, u32 min_area_tile, std::vector<TileInfo> &tiles_infos);

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

	const TileInfo &getTileInfo(u32 i) const
	{
		return m_tiles_infos.at(i);
	}

	bool canFit(const TileInfo &area, const TileInfo &tex);

	void insertCrackTile(u32 i, std::string s)
	{
		MutexAutoLock crack_tiles_lock(m_crack_tiles_mutex);

		m_crack_tiles.insert({i, s});
	}

	void packTextures(int side);

	void updateAnimations(f32 time);

	void updateCrackAnimations(int new_crack);
};

class TextureSingleton
{
	video::IVideoDriver *m_driver;
	ITextureSource *m_tsrc;

	video::ITexture *m_texture;

	AnimationInfo m_anim_info;

	// Number of last crack
	int m_last_crack = -1;

	bool m_mip_maps;

public:
	video::ITexture *original_texture;
	std::string crack_tile;

	TextureSingleton(Client *client, video::ITexture *texture);

	~TextureSingleton()
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

	void updateAnimation(f32 time);

	void updateCrackAnimation(int new_crack);
};

class TextureBuilder
{
	std::vector<TextureAtlas *> m_atlases;
	std::vector<TextureSingleton *> m_singletons;

public:
	TextureBuilder() = default;

	~TextureBuilder();

	TextureAtlas *getAtlas(u32 index) const {
		assert(index <= m_atlases.size()-1);

		return m_atlases.at(index);
	}

	TextureSingleton *findSingleton(video::ITexture *tex) const {
		auto st_it = std::find_if(m_singletons.begin(), m_singletons.end(),
			[tex] (TextureSingleton *st)
			{
				return st->original_texture == tex;
			});

		if (st_it == m_singletons.end())
			return nullptr;

		return *st_it;
	}

	void buildAtlas(Client *client, std::list<TileLayer*> &layers);
	TextureSingleton *buildSingleton(Client *client, video::ITexture *texture)
	{
		TextureSingleton *st = new TextureSingleton(client, texture);
		m_singletons.emplace_back(st);

		return st;
	}

	void updateAnimations(f32 time);
	void updateCrackAnimations(int new_crack);
};
