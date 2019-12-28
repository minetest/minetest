/*
Minetest
Copyright (C) 2019 kilbith, Jean-Patrick Guerrero <jeanpatrick.guerrero@gmail.com>

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

#include "texture.h"
#include <string>
#include <vector>

class TexturePool {
public:
	TexturePool() = default;

	TexturePool(const TexturePool &texture_pool) :
		m_textures(texture_pool.m_textures),
		m_animations(texture_pool.m_animations),
		m_global_time(texture_pool.m_global_time)
	{
	}

	~TexturePool() = default;

	s32 addTexture(const std::string &name,
		const std::string &base_name, s32 frame_count,
		u64 frame_duration);

	s32 addTexture(const std::string &name);

	s32 getTextureIndex(const std::string &name);

	video::ITexture *getTexture(const std::string &name,
			ISimpleTextureSource *tsrc, s32 &texture_idx);

	void step();

private:
	std::vector<Texture> m_textures;
	std::vector<s32> m_animations;
	u64 m_global_time = 0;
	u64 m_texture_idx = 0;
};
