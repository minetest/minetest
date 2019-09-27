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
	name = _name;
	frame_count = _frame_count;
	frame_idx = 0;
	frame_duration = _frame_duration;
	frame_time = 0;
	frame_names.resize(frame_count);

	for (s32 i = 0; i < frame_count; ++i)
		frame_names[i] = base_name + "_" + (char)(i + 1 + '0') + ".png";
}

void Texture::set(const std::string &_name)
{
	name = _name;
	frame_count = 0;
	frame_idx = 0;
	frame_duration = 0;
	frame_time = 0;
	frame_names.resize(0);
}

video::ITexture *Texture::getTexture(ISimpleTextureSource *tsrc)
{
	if (frame_count > 0)
		return tsrc->getTexture(frame_names[frame_idx]);
	else
		return tsrc->getTexture(name);
}

void Texture::step(u64 step_duration)
{
	frame_time += step_duration;
	frame_idx += u32(frame_time / frame_duration);
	frame_idx %= frame_count;
	frame_time %= frame_duration;
}
