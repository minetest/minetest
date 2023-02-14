/*
Minetest
Copyright (c) 2006-2023 LOVE Development Team (License: zlib)
Copyright (C) 2023 Jean-Patrick Guerrero <jeanpatrick.guerrero@gmail.com>

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

#include <theora/theora.h>
#include "client.h"
#include "client/renderingengine.h"

enum THEORA_PLAYER_STATE
{
	IDLE = 0,
	PLAYING,
};

class VideoPlayer
{
public:
	VideoPlayer() = default;
	~VideoPlayer() { stop(); }

	bool load(const char *filename);
	bool update(const u32 timeMs);
	bool stop();

	inline const THEORA_PLAYER_STATE &getState() const { return m_player_state; }
	inline video::ITexture *getTexture() const { return m_texture; }
	inline video::IImage *getImage() const { return m_image; }

private:
	bool processNextFrame();

	void updateBuffer();
	void updateTexture();

	bool prepareOgg();
	bool prepareBuffers();

	inline int buffer_data();
	inline void queue_page(ogg_page *page)
	{
		if (m_pkt_count)
			ogg_stream_pagein(&m_ogg_stream_state, &m_ogg_page);
	}

	IrrlichtDevice       *m_device = RenderingEngine::get_raw_device();
	video::IVideoDriver  *m_driver = m_device->getVideoDriver();
	video::ITexture      *m_texture;
	video::IImage        *m_image;
	io::IReadFile        *m_file;

	ogg_page              m_ogg_page;
	ogg_packet            m_ogg_pkt;
	ogg_sync_state        m_ogg_sync_state;
	ogg_stream_state      m_ogg_stream_state;

	theora_info           m_theora_info;
	theora_comment        m_theora_comment;
	theora_state          m_theora_state;

	int m_pkt_count;
	u32 m_current_frame;
	u32 m_current_time;

	THEORA_PLAYER_STATE m_player_state = IDLE;
};
