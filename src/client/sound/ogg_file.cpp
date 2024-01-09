/*
Minetest
Copyright (C) 2022 DS
Copyright (C) 2013 celeron55, Perttu Ahola <celeron55@gmail.com>
OpenAL support based on work by:
Copyright (C) 2011 Sebastian 'Bahamada' Rühl
Copyright (C) 2011 Cyriaque 'Cisoun' Skrapits <cysoun@gmail.com>
Copyright (C) 2011 Giuseppe Bilotta <giuseppe.bilotta@gmail.com>

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

#include "ogg_file.h"

#include <cstring> // memcpy

namespace sound {

/*
 * OggVorbisBufferSource struct
 */

size_t OggVorbisBufferSource::read_func(void *ptr, size_t size, size_t nmemb,
		void *datasource) noexcept
{
	OggVorbisBufferSource *s = (OggVorbisBufferSource *)datasource;
	size_t copied_size = MYMIN(s->buf.size() - s->cur_offset, size);
	memcpy(ptr, s->buf.data() + s->cur_offset, copied_size);
	s->cur_offset += copied_size;
	return copied_size;
}

int OggVorbisBufferSource::seek_func(void *datasource, ogg_int64_t offset, int whence) noexcept
{
	OggVorbisBufferSource *s = (OggVorbisBufferSource *)datasource;
	if (whence == SEEK_SET) {
		if (offset < 0 || (size_t)offset > s->buf.size()) {
			// offset out of bounds
			return -1;
		}
		s->cur_offset = offset;
		return 0;
	} else if (whence == SEEK_CUR) {
		if ((size_t)MYMIN(-offset, 0) > s->cur_offset
				|| s->cur_offset + offset > s->buf.size()) {
			// offset out of bounds
			return -1;
		}
		s->cur_offset += offset;
		return 0;
	} else if (whence == SEEK_END) {
		if (offset > 0 || (size_t)-offset > s->buf.size()) {
			// offset out of bounds
			return -1;
		}
		s->cur_offset = s->buf.size() - offset;
		return 0;
	}
	return -1;
}

int OggVorbisBufferSource::close_func(void *datasource) noexcept
{
	auto s = reinterpret_cast<OggVorbisBufferSource *>(datasource);
	delete s;
	return 0;
}

long OggVorbisBufferSource::tell_func(void *datasource) noexcept
{
	OggVorbisBufferSource *s = (OggVorbisBufferSource *)datasource;
	return s->cur_offset;
}

const ov_callbacks OggVorbisBufferSource::s_ov_callbacks = {
	&OggVorbisBufferSource::read_func,
	&OggVorbisBufferSource::seek_func,
	&OggVorbisBufferSource::close_func,
	&OggVorbisBufferSource::tell_func
};

/*
 * RAIIOggFile struct
 */

std::optional<OggFileDecodeInfo> RAIIOggFile::getDecodeInfo(const std::string &filename_for_logging)
{
	OggFileDecodeInfo ret;

	vorbis_info *pInfo = ov_info(&m_file, -1);
	if (!pInfo)
		return std::nullopt;

	ret.name_for_logging = filename_for_logging;

	if (pInfo->channels == 1) {
		ret.is_stereo = false;
		ret.format = AL_FORMAT_MONO16;
		ret.bytes_per_sample = 2;
	} else if (pInfo->channels == 2) {
		ret.is_stereo = true;
		ret.format = AL_FORMAT_STEREO16;
		ret.bytes_per_sample = 4;
	} else {
		warningstream << "Audio: Can't decode. Sound is neither mono nor stereo: "
				<< ret.name_for_logging << std::endl;
		return std::nullopt;
	}

	ret.freq = pInfo->rate;

	ret.length_samples = static_cast<ALuint>(ov_pcm_total(&m_file, -1));
	ret.length_seconds = static_cast<f32>(ov_time_total(&m_file, -1));

	return ret;
}

RAIIALSoundBuffer RAIIOggFile::loadBuffer(const OggFileDecodeInfo &decode_info,
		ALuint pcm_start, ALuint pcm_end)
{
	constexpr int endian = 0; // 0 for Little-Endian, 1 for Big-Endian
	constexpr int word_size = 2; // we use s16 samples
	constexpr int word_signed = 1; // ^

	// seek
	if (ov_pcm_tell(&m_file) != pcm_start) {
		if (ov_pcm_seek(&m_file, pcm_start) != 0) {
			warningstream << "Audio: Error decoding (could not seek) "
					<< decode_info.name_for_logging << std::endl;
			return RAIIALSoundBuffer();
		}
	}

	const size_t size = static_cast<size_t>(pcm_end - pcm_start)
			* decode_info.bytes_per_sample;

	std::unique_ptr<char[]> snd_buffer(new char[size]);

	// read size bytes
	size_t read_count = 0;
	int bitStream;
	while (read_count < size) {
		// Read up to a buffer's worth of decoded sound data
		long num_bytes = ov_read(&m_file, &snd_buffer[read_count], size - read_count,
				endian, word_size, word_signed, &bitStream);

		if (num_bytes <= 0) {
			warningstream << "Audio: Error decoding "
					<< decode_info.name_for_logging << std::endl;
			return RAIIALSoundBuffer();
		}

		read_count += num_bytes;
	}

	// load buffer to openal
	RAIIALSoundBuffer snd_buffer_id = RAIIALSoundBuffer::generate();
	alBufferData(snd_buffer_id.get(), decode_info.format, &(snd_buffer[0]), size,
			decode_info.freq);

	ALenum error = alGetError();
	if (error != AL_NO_ERROR) {
		warningstream << "Audio: OpenAL error: " << getAlErrorString(error)
				<< "preparing sound buffer for sound \""
				<< decode_info.name_for_logging << "\"" << std::endl;
	}

	return snd_buffer_id;
}

} // namespace sound
