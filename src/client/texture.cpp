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

#include "texture.h"
#include "porting.h"

void Texture::set(const std::string &_name, const std::string &base_name,
		s32 _frame_count, u64 _frame_duration)
{
	m_name = _name;
	m_frame_count = _frame_count;
	m_frame_idx = 0;
	m_frame_duration = _frame_duration;
	m_frame_time = 0;
	m_frame_names.resize(m_frame_count);

	for (s32 i = 0; i < m_frame_count; ++i)
		m_frame_names[i] = base_name + "_" + std::to_string(i + 1) + ".png";
}

//! Set animation data to display a static texture
void Texture::set(const std::string &_name)
{
	m_name = _name;
	m_frame_count = 0;
	m_frame_idx = 0;
	m_frame_duration = 0;
	m_frame_time = 0;
	m_frame_names.resize(0);
}

std::string Texture::getName() const
{
	return m_name;
}

//! Get the texture corresponding to the current frame
video::ITexture *Texture::getTexture(ISimpleTextureSource *tsrc)
{
	if (m_frame_count > 0)
		return tsrc->getTexture(m_frame_names[m_frame_idx]);
	else
		return tsrc->getTexture(m_name);
}

//! Advance the animation by the given duration
void Texture::step(u64 step_duration)
{
	m_frame_time += step_duration;

	// Advance by the number of elapsed frames, looping if necessary
	m_frame_idx += u32(m_frame_time / m_frame_duration);
	m_frame_idx %= m_frame_count;

	// If 1 or more frames have elapsed, reset the frame time counter with
	// the remainder
	m_frame_time %= m_frame_duration;
}
