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

#include "videoplayer.h"

bool VideoPlayer::load(const char *filename)
{
	stop();

	m_file = m_device->getFileSystem()->createAndOpenFile(filename);

	if (!m_file) {
		errorstream << "Failed to open file: " << filename << std::endl;
		return false;
	}

	if (!prepareOgg())
		return false;
	if (!prepareBuffers())
		return false;

	m_player_state = PLAYING;

	return true;
}

bool VideoPlayer::stop()
{
	if (m_player_state == IDLE)
		return true;

	if (m_pkt_count) {
		ogg_stream_clear(&m_ogg_stream_state);
		theora_clear(&m_theora_state);
		theora_comment_clear(&m_theora_comment);
		theora_info_clear(&m_theora_info);
	}

	ogg_sync_clear(&m_ogg_sync_state);

	if (m_file) {
		m_file->drop();
		m_file = nullptr;
	}

	if (m_texture) {
		m_texture->drop();
		m_texture = nullptr;
	}

	if (m_image) {
		m_image->drop();
		m_image = nullptr;
	}

	m_player_state = IDLE;

	return true;
}

bool VideoPlayer::prepareOgg()
{
	m_file->seek(0);

	int state_flag = 0;

	ogg_sync_init(&m_ogg_sync_state);
	theora_comment_init(&m_theora_comment);
	theora_info_init(&m_theora_info);

	m_pkt_count = 0;
	m_current_frame = 0;
	m_current_time = 0;

	while (!state_flag) {
		const int ret = buffer_data();
		if (ret == 0)
			break;

		while (ogg_sync_pageout(&m_ogg_sync_state, &m_ogg_page) > 0) {
			ogg_stream_state test;

			if (!ogg_page_bos(&m_ogg_page)) {
				queue_page(&m_ogg_page);
				state_flag = 1;
				break;
			}

			ogg_stream_init(&test, ogg_page_serialno(&m_ogg_page));
			ogg_stream_pagein(&test, &m_ogg_page);
			ogg_stream_packetout(&test, &m_ogg_pkt);

			if (!m_pkt_count && theora_decode_header(&m_theora_info, &m_theora_comment, &m_ogg_pkt) >= 0) {
				memcpy(&m_ogg_stream_state, &test, sizeof(test));
				m_pkt_count = 1;
			} else {
				ogg_stream_clear(&test);
			}
		}
	}

	while (m_pkt_count && m_pkt_count < 3) {
		int ret;

		while (m_pkt_count && (m_pkt_count < 3)) {
			ret = ogg_stream_packetout(&m_ogg_stream_state, &m_ogg_pkt);

			if (ret < 0 || theora_decode_header(&m_theora_info, &m_theora_comment, &m_ogg_pkt)) {
				errorstream << "VideoPlayer: Error parsing Theora stream headers. Corrupt stream?" << std::endl;
				return false;
			}

			++m_pkt_count;
		}

		if (ogg_sync_pageout(&m_ogg_sync_state, &m_ogg_page) > 0)
			queue_page(&m_ogg_page);
		else {
			int ret = buffer_data();
			if (ret == 0) {
				errorstream << "VideoPlayer: EOF while searching for codec headers" << std::endl;
				return false;
			}
		}
	}

	if (m_pkt_count) {
		theora_decode_init(&m_theora_state, &m_theora_info);
	} else {
		errorstream << "Decoder initialization failed" << std::endl;

		theora_info_clear(&m_theora_info);
		theora_comment_clear(&m_theora_comment);

		return false;
	}

	while (ogg_sync_pageout(&m_ogg_sync_state, &m_ogg_page) > 0)
		queue_page(&m_ogg_page);

	return true;
}

bool VideoPlayer::prepareBuffers()
{
	bool m_old_mipmap_flag = m_driver->getTextureCreationFlag(video::ETCF_CREATE_MIP_MAPS);
	m_device->getVideoDriver()->setTextureCreationFlag(video::ETCF_CREATE_MIP_MAPS, false);

	const core::dimension2du size(m_theora_info.frame_width, m_theora_info.frame_height);
	m_texture = m_driver->addTexture(size, "VideoPlayerTexture", video::ECF_R8G8B8);

	m_driver->setTextureCreationFlag(video::ETCF_CREATE_MIP_MAPS, m_old_mipmap_flag);

	if (!m_texture) {
		errorstream << "VideoPlayer: Failed to create texture" << std::endl;
		return false;
	}

	m_texture->grab();

	const core::dimension2du &imgsize = m_texture->getOriginalSize();
	const u32 bufsize = m_texture->getPitch() * imgsize.Height;

	u8 *texture_buf = new u8[bufsize];
	if (!texture_buf) {
		errorstream << "VideoPlayer: Failed to allocate buffer memory" << std::endl;
		return false;
	}

	m_image = m_driver->createImageFromData(m_texture->getColorFormat(), imgsize, texture_buf);
	delete[] texture_buf;

	if (!m_image) {
		errorstream << "VideoPlayer: Failed to create image" << std::endl;
		return false;
	}

	return true;
}

bool VideoPlayer::update(const u32 timeMs)
{
	if (m_player_state != PLAYING)
		return true;

	m_current_time += timeMs;
	const u32 needed_frame = (u32)(1.f * (m_current_time / 1000.f) *
			m_theora_info.fps_numerator / m_theora_info.fps_denominator);
	const u32 frames_todo = needed_frame - m_current_frame;
	u32 was_decoded = false;

	for (u32 i = 0; i < frames_todo; ++i) {
		was_decoded = processNextFrame();
		if (m_player_state != PLAYING)
			break;
	}

	if (was_decoded) {
		updateBuffer();
		updateTexture();
	}

	return true;
}

bool VideoPlayer::processNextFrame()
{
	bool videobuf_ready = false;

	while (!videobuf_ready) {
		if (ogg_stream_packetout(&m_ogg_stream_state, &m_ogg_pkt) > 0) {
			theora_decode_packetin(&m_theora_state, &m_ogg_pkt);
			videobuf_ready = true;
			++m_current_frame;
		}

		// End of video
		if (m_file->getPos() == m_file->getSize())			
			return true;

		if (!videobuf_ready) {
			buffer_data();

			while (ogg_sync_pageout(&m_ogg_sync_state, &m_ogg_page) > 0)
				queue_page(&m_ogg_page);
		}
	}

	return videobuf_ready;
}

void VideoPlayer::updateTexture()
{
	if (!m_image)
		return;

	u8 *texture_data = (u8 *)m_texture->lock();
	if (!texture_data)
		return;

	const auto &size = m_texture->getSize();

	m_image->copyToScaling(texture_data, size.Width, size.Height,
			m_texture->getColorFormat(), m_texture->getPitch());

	m_texture->unlock();
}

void VideoPlayer::updateBuffer()
{
	if (!m_image)
		return;

	yuv_buffer yuv;
	theora_decode_YUVout(&m_theora_state, &yuv);

	u8 *bufdata = (u8 *)m_image->getData();
	const u32 rowsize = m_image->getPitch();

	for (int y = 0; y < yuv.y_height; ++y) {
		const int xsize = yuv.y_width;
		const int uvy = (y/2) * yuv.uv_stride;
		const int yy = y * yuv.y_stride;
		const int by = y * rowsize;

		for (int x = 0; x < xsize; ++x) {
			const int Y = yuv.y[yy + x] - 16;
			const int U = yuv.u[uvy + (x/2)] - 128;
			const int V = yuv.v[uvy + (x/2)] - 128;

			int R = ((298*Y	+ 409*V + 128) >> 8);
			int G = ((298*Y - 100*U - 208*V + 128) >> 8);
			int B = ((298*Y + 516*U	+ 128) >> 8);

			R = rangelim(R, 0, 255);
			G = rangelim(G, 0, 255);
			B = rangelim(B, 0, 255);

			switch (m_image->getColorFormat()) {
			case video::ECF_A8R8G8B8: {
				bufdata[by + x*4 + 0] = B;
				bufdata[by + x*4 + 1] = G;
				bufdata[by + x*4 + 2] = R;
				bufdata[by + x*4 + 3] = 0xff;
			}
			break;

			case video::ECF_R8G8B8: {
				bufdata[by + x*3 + 0] = B;
				bufdata[by + x*3 + 1] = G;
				bufdata[by + x*3 + 2] = R;
			}
			break;

			default:
				break;
			}
		}
	}
}

int VideoPlayer::buffer_data()
{
	char *buf = ogg_sync_buffer(&m_ogg_sync_state, 4096);
	const int bytes_read = m_file->read(buf, 4096);

	ogg_sync_wrote(&m_ogg_sync_state, bytes_read);

	return bytes_read;
}
