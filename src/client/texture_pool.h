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
#include <iostream>

class TexturePool {
	TexturePool() = default;

	TexturePool(const TexturePool &texture_pool) :
		textures(texture_pool.textures),
		animations(texture_pool.animations),
		global_time(texture_pool.global_time)
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

	std::vector<Texture> textures;
	std::vector<s32> animations;
	u64 global_time = 0;
	u64 m_texture_idx = 0;
};
