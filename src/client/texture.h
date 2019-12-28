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

#include "client/tile.h"
#include <ITexture.h>
#include <string>
#include <vector>

//! Animated texture object managed by TexturePool
class Texture {
public:
	Texture() = default;

	Texture(const Texture &texture) :
		m_name(texture.m_name),
		m_frame_count(texture.m_frame_count),
		m_frame_idx(texture.m_frame_idx),
		m_frame_duration(texture.m_frame_duration),
		m_frame_time(texture.m_frame_time),
		m_frame_names(texture.m_frame_names)
	{
	}

	~Texture() = default;

	void set(const std::string &_name, const std::string &base_name,
		s32 _frame_count, u64 _frame_duration);

	void set(const std::string &_name);

	std::string getName() const;

	video::ITexture *getTexture(ISimpleTextureSource *tsrc);

	void step(u64 step_duration);

private:
	std::string m_name;
	s32 m_frame_count = 0;
	s32 m_frame_idx = 0;
	u64 m_frame_duration = 0;
	u64 m_frame_time = 0;
	std::vector<std::string> m_frame_names;
};
