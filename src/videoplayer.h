/*
Minetest
Copyright (C) 2020 Jean-Patrick Guerrero <jeanpatrick.guerrero@gmail.com>

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

class VideoPlayer
{
public:
	VideoPlayer() = default;
	~VideoPlayer();

	s32 init(const char *name, IrrlichtDevice *device);
	s32 decodeFrame();
	void pause();
	void goToFrame(u32 frame_idx);

	u32 getFrameWidth() const { return m_width; }
	u32 getFrameHeight() const { return m_height; }
	u32 getFrameRate() const { return m_framerate; }
	u64 getDuration() const { return m_duration; }
	u32 getFrameIdx() const { return m_frame_idx; }

	video::ITexture *getFrameTexture();

private:
	u32 m_width, m_height;
	u32 m_frame_idx;
	f32 m_framerate;
	u64 m_duration;
	s32 finished;
	s32 stream;
	s32 now, then;
	s32 decodeFrameInternal();

	u8                   *buffer  = nullptr;
	video::ITexture      *texture = nullptr;
	video::IVideoDriver  *driver;
	IrrlichtDevice       *device;
	video::IImage        *image;
	const char           *m_filename;
};
