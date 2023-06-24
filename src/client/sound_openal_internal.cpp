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
with this program; ifnot, write to the Free Software Foundation, Inc.,
51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
*/

#include "sound_openal_internal.h"

#include "util/numeric.h" // myrand()
#include "filesys.h"
#include "settings.h"
#include <algorithm>
#include <cmath>

/*
 * Helpers
 */

static const char *getAlErrorString(ALenum err) noexcept
{
	switch (err) {
	case AL_NO_ERROR:
		return "no error";
	case AL_INVALID_NAME:
		return "invalid name";
	case AL_INVALID_ENUM:
		return "invalid enum";
	case AL_INVALID_VALUE:
		return "invalid value";
	case AL_INVALID_OPERATION:
		return "invalid operation";
	case AL_OUT_OF_MEMORY:
		return "out of memory";
	default:
		return "<unknown OpenAL error>";
	}
}

static ALenum warn_if_al_error(const char *desc)
{
	ALenum err = alGetError();
	if (err == AL_NO_ERROR)
		return err;
	warningstream << "[OpenAL Error] " << desc << ": " << getAlErrorString(err)
			<< std::endl;
	return err;
}

/**
 * Transforms vectors from a left-handed coordinate system to a right-handed one
 * and vice-versa.
 * (Needed because Minetest uses a left-handed one and OpenAL a right-handed one.)
 */
static inline v3f swap_handedness(v3f v) noexcept
{
	return v3f(-v.X, v.Y, v.Z);
}

/*
 * RAIIALSoundBuffer struct
 */

RAIIALSoundBuffer &RAIIALSoundBuffer::operator=(RAIIALSoundBuffer &&other) noexcept
{
	if (&other != this)
		reset(other.release());
	return *this;
}

void RAIIALSoundBuffer::reset(ALuint buf) noexcept
{
	if (m_buffer != 0) {
		alDeleteBuffers(1, &m_buffer);
		warn_if_al_error("Failed to free sound buffer");
	}

	m_buffer = buf;
}

RAIIALSoundBuffer RAIIALSoundBuffer::generate() noexcept
{
	ALuint buf;
	alGenBuffers(1, &buf);
	return RAIIALSoundBuffer(buf);
}

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

/*
 * SoundManagerSingleton class
 */

bool SoundManagerSingleton::init()
{
	if (!(m_device = unique_ptr_alcdevice(alcOpenDevice(nullptr)))) {
		errorstream << "Audio: Global Initialization: Failed to open device" << std::endl;
		return false;
	}

	if (!(m_context = unique_ptr_alccontext(alcCreateContext(m_device.get(), nullptr)))) {
		errorstream << "Audio: Global Initialization: Failed to create context" << std::endl;
		return false;
	}

	if (!alcMakeContextCurrent(m_context.get())) {
		errorstream << "Audio: Global Initialization: Failed to make current context" << std::endl;
		return false;
	}

	alDistanceModel(AL_INVERSE_DISTANCE_CLAMPED);

	// Speed of sound in nodes per second
	// FIXME: This value assumes 1 node sidelength = 1 meter, and "normal" air.
	//        Ideally this should be mod-controlled.
	alSpeedOfSound(343.3f);

	// doppler effect turned off for now, for best backwards compatibility
	alDopplerFactor(0.0f);

	if (alGetError() != AL_NO_ERROR) {
		errorstream << "Audio: Global Initialization: OpenAL Error " << alGetError() << std::endl;
		return false;
	}

	infostream << "Audio: Global Initialized: OpenAL " << alGetString(AL_VERSION)
		<< ", using " << alcGetString(m_device.get(), ALC_DEVICE_SPECIFIER)
		<< std::endl;

	return true;
}

SoundManagerSingleton::~SoundManagerSingleton()
{
	infostream << "Audio: Global Deinitialized." << std::endl;
}

/*
 * ISoundDataOpen struct
 */

std::shared_ptr<ISoundDataOpen> ISoundDataOpen::fromOggFile(std::unique_ptr<RAIIOggFile> oggfile,
		const std::string &filename_for_logging)
{
	// Get some information about the OGG file
	std::optional<OggFileDecodeInfo> decode_info = oggfile->getDecodeInfo(filename_for_logging);
	if (!decode_info.has_value()) {
		warningstream << "Audio: Error decoding "
				<< filename_for_logging << std::endl;
		return nullptr;
	}

	// use duration (in seconds) to decide whether to load all at once or to stream
	if (decode_info->length_seconds <= SOUND_DURATION_MAX_SINGLE) {
		return std::make_shared<SoundDataOpenBuffer>(std::move(oggfile), *decode_info);
	} else {
		return std::make_shared<SoundDataOpenStream>(std::move(oggfile), *decode_info);
	}
}

/*
 * SoundDataUnopenBuffer struct
 */

std::shared_ptr<ISoundDataOpen> SoundDataUnopenBuffer::open(const std::string &sound_name) &&
{
	// load from m_buffer

	auto oggfile = std::make_unique<RAIIOggFile>();

	auto buffer_source = std::make_unique<OggVorbisBufferSource>();
	buffer_source->buf = std::move(m_buffer);

	oggfile->m_needs_clear = true;
	if (ov_open_callbacks(buffer_source.release(), oggfile->get(), nullptr, 0,
			OggVorbisBufferSource::s_ov_callbacks) != 0) {
		warningstream << "Audio: Error opening " << sound_name << " for decoding"
				<< std::endl;
		return nullptr;
	}

	return ISoundDataOpen::fromOggFile(std::move(oggfile), sound_name);
}

/*
 * SoundDataUnopenFile struct
 */

std::shared_ptr<ISoundDataOpen> SoundDataUnopenFile::open(const std::string &sound_name) &&
{
	// load from file at m_path

	auto oggfile = std::make_unique<RAIIOggFile>();

	if (ov_fopen(m_path.c_str(), oggfile->get()) != 0) {
		warningstream << "Audio: Error opening " << m_path << " for decoding"
				<< std::endl;
		return nullptr;
	}
	oggfile->m_needs_clear = true;

	return ISoundDataOpen::fromOggFile(std::move(oggfile), sound_name);
}

/*
 * SoundDataOpenBuffer struct
 */

SoundDataOpenBuffer::SoundDataOpenBuffer(std::unique_ptr<RAIIOggFile> oggfile,
		const OggFileDecodeInfo &decode_info) : ISoundDataOpen(decode_info)
{
	m_buffer = oggfile->loadBuffer(m_decode_info, 0, m_decode_info.length_samples);
	if (m_buffer.get() == 0) {
		warningstream << "SoundDataOpenBuffer: Failed to load sound \""
				<< m_decode_info.name_for_logging << "\"" << std::endl;
		return;
	}
}

/*
 * SoundDataOpenStream struct
 */

SoundDataOpenStream::SoundDataOpenStream(std::unique_ptr<RAIIOggFile> oggfile,
		const OggFileDecodeInfo &decode_info) :
	ISoundDataOpen(decode_info), m_oggfile(std::move(oggfile))
{
	// do nothing here. buffers are loaded at getOrLoadBufferAt
}

std::tuple<ALuint, ALuint, ALuint> SoundDataOpenStream::getOrLoadBufferAt(ALuint offset)
{
	if (offset >= m_decode_info.length_samples)
		return {0, m_decode_info.length_samples, 0};

	// find the right-most ContiguousBuffers, such that `m_start <= offset`
	// equivalent: the first element from the right such that `!(m_start > offset)`
	// (from the right, `offset` is a lower bound to the `m_start`s)
	auto lower_rit = std::lower_bound(m_bufferss.rbegin(), m_bufferss.rend(), offset,
			[](const ContiguousBuffers &bufs, ALuint offset) {
				return bufs.m_start > offset;
			});

	if (lower_rit != m_bufferss.rend()) {
		std::vector<SoundBufferUntil> &bufs = lower_rit->m_buffers;
		// find the left-most SoundBufferUntil, such that `m_end > offset`
		// equivalent: the first element from the left such that `m_end > offset`
		// (returns first element where comp gives true)
		auto upper_it = std::upper_bound(bufs.begin(), bufs.end(), offset,
				[](ALuint offset, const SoundBufferUntil &buf) {
					return offset < buf.m_end;
				});

		if (upper_it != bufs.end()) {
			ALuint start = upper_it == bufs.begin() ? lower_rit->m_start
					: (upper_it - 1)->m_end;
			return {upper_it->m_buffer.get(), upper_it->m_end, offset - start};
		}
	}

	// no loaded buffer starts before or at `offset`
	// or no loaded buffer (that starts before or at `offset`) ends after `offset`

	// lower_rit, but not reverse and 1 farther
	auto after_it = m_bufferss.begin() + (m_bufferss.rend() - lower_rit);

	return loadBufferAt(offset, after_it);
}

std::tuple<ALuint, ALuint, ALuint> SoundDataOpenStream::loadBufferAt(ALuint offset,
		std::vector<ContiguousBuffers>::iterator after_it)
{
	bool has_before = after_it != m_bufferss.begin();
	bool has_after = after_it != m_bufferss.end();

	ALuint end_before = has_before ? (after_it - 1)->m_buffers.back().m_end : 0;
	ALuint start_after = has_after ? after_it->m_start : m_decode_info.length_samples;

	const ALuint min_buf_len_samples = m_decode_info.freq * MIN_STREAM_BUFFER_LENGTH;

	//
	// 1) Find the actual start and end of the new buffer
	//

	ALuint new_buf_start = offset;
	ALuint new_buf_end = offset + min_buf_len_samples;

	// Don't load into next buffer, or past the end
	if (new_buf_end > start_after) {
		new_buf_end = start_after;
		// Also move start (for min buf size) (but not *into* previous buffer)
		if (new_buf_end - new_buf_start < min_buf_len_samples) {
			new_buf_start = std::max(
					end_before,
					new_buf_end < min_buf_len_samples ? 0
							: new_buf_end - min_buf_len_samples
				);
		}
	}

	// Widen if space to right or left is smaller than min buf size
	if (new_buf_start - end_before < min_buf_len_samples)
		new_buf_start = end_before;
	if (start_after - new_buf_end < min_buf_len_samples)
		new_buf_end = start_after;

	//
	// 2) Load [new_buf_start, new_buf_end)
	//

	// If it fails, we get a 0-buffer. we store it and won't try loading again
	RAIIALSoundBuffer new_buf = m_oggfile->loadBuffer(m_decode_info, new_buf_start,
			new_buf_end);

	//
	// 3) Insert before after_it
	//

	// Choose ContiguousBuffers to add the new SoundBufferUntil into:
	// * `after_it - 1` (=before) if existent and if there's no space between its
	//   last buffer and the new buffer
	// * A new ContiguousBuffers otherwise
	auto it = has_before && new_buf_start == end_before ? after_it - 1
			: m_bufferss.insert(after_it, ContiguousBuffers{new_buf_start, {}});

	// Add the new SoundBufferUntil
	size_t new_buf_i = it->m_buffers.size();
	it->m_buffers.push_back(SoundBufferUntil{new_buf_end, std::move(new_buf)});

	if (has_after && new_buf_end == start_after) {
		// Merge after into my ContiguousBuffers
		auto &bufs = it->m_buffers;
		auto &bufs_after = (it + 1)->m_buffers;
		bufs.insert(bufs.end(), std::make_move_iterator(bufs_after.begin()),
				std::make_move_iterator(bufs_after.end()));
		it = m_bufferss.erase(it + 1) - 1;
	}

	return {it->m_buffers[new_buf_i].m_buffer.get(), new_buf_end, offset - new_buf_start};
}

/*
 * PlayingSound class
 */

PlayingSound::PlayingSound(ALuint source_id, std::shared_ptr<ISoundDataOpen> data,
		bool loop, f32 volume, f32 pitch, f32 start_time,
		const std::optional<std::pair<v3f, v3f>> &pos_vel_opt)
	: m_source_id(source_id), m_data(std::move(data)), m_looping(loop),
	m_is_positional(pos_vel_opt.has_value())
{
	// Calculate actual start_time (see lua_api.txt for specs)
	f32 len_seconds = m_data->m_decode_info.length_seconds;
	f32 len_samples = m_data->m_decode_info.length_samples;
	if (!m_looping) {
		if (start_time < 0.0f) {
			start_time = std::fmax(start_time + len_seconds, 0.0f);
		} else if (start_time >= len_seconds) {
			// No sound
			m_next_sample_pos = len_samples;
			return;
		}
	} else {
		// Modulo offset to be within looping time
		start_time = start_time - std::floor(start_time / len_seconds) * len_seconds;
	}

	// Queue first buffers

	m_next_sample_pos = std::min((start_time / len_seconds) * len_samples, len_samples);

	if (m_looping && m_next_sample_pos == len_samples)
		m_next_sample_pos = 0;

	if (!m_data->isStreaming()) {
		// If m_next_sample_pos >= len_samples, buf will be 0, and setting it as
		// AL_BUFFER is a NOP (source stays AL_UNDETERMINED). => No sound will be
		// played.

		auto [buf, buf_end, offset_in_buf] = m_data->getOrLoadBufferAt(m_next_sample_pos);
		m_next_sample_pos = buf_end;

		alSourcei(m_source_id, AL_BUFFER, buf);
		alSourcei(m_source_id, AL_SAMPLE_OFFSET, offset_in_buf);

		alSourcei(m_source_id, AL_LOOPING, m_looping ? AL_TRUE : AL_FALSE);

		warn_if_al_error("when creating non-streaming sound");

	} else {
		// Start with 2 buffers
		ALuint buf_ids[2];

		// If m_next_sample_pos >= len_samples (happens only if not looped), one
		// or both of buf_ids will be 0. Queuing 0 is a NOP.

		auto [buf0, buf0_end, offset_in_buf0] = m_data->getOrLoadBufferAt(m_next_sample_pos);
		buf_ids[0] = buf0;
		m_next_sample_pos = buf0_end;

		if (m_looping && m_next_sample_pos == len_samples)
			m_next_sample_pos = 0;

		auto [buf1, buf1_end, offset_in_buf1] = m_data->getOrLoadBufferAt(m_next_sample_pos);
		buf_ids[1] = buf1;
		m_next_sample_pos = buf1_end;
		assert(offset_in_buf1 == 0);

		alSourceQueueBuffers(m_source_id, 2, buf_ids);
		alSourcei(m_source_id, AL_SAMPLE_OFFSET, offset_in_buf0);

		// We can't use AL_LOOPING because more buffers are queued later
		// looping is therefore done manually

		m_stopped_means_dead = false;

		warn_if_al_error("when creating streaming sound");
	}

	// Set initial pos, volume, pitch
	if (m_is_positional) {
		updatePosVel(pos_vel_opt->first, pos_vel_opt->second);
	} else {
		// Make position-less
		alSourcei(m_source_id, AL_SOURCE_RELATIVE, true);
		alSource3f(m_source_id, AL_POSITION, 0.0f, 0.0f, 0.0f);
		alSource3f(m_source_id, AL_VELOCITY, 0.0f, 0.0f, 0.0f);
		warn_if_al_error("PlayingSound::PlayingSound at making position-less");
	}
	setGain(volume);
	setPitch(pitch);
}

bool PlayingSound::stepStream()
{
	if (isDead())
		return false;

	// unqueue finished buffers
	ALint num_unqueued_bufs = 0;
	alGetSourcei(m_source_id, AL_BUFFERS_PROCESSED, &num_unqueued_bufs);
	if (num_unqueued_bufs == 0)
		return true;
	// We always have 2 buffers enqueued at most
	SANITY_CHECK(num_unqueued_bufs <= 2);
	ALuint unqueued_buffer_ids[2];
	alSourceUnqueueBuffers(m_source_id, num_unqueued_bufs, unqueued_buffer_ids);

	// Fill up again
	for (ALint i = 0; i < num_unqueued_bufs; ++i) {
		if (m_next_sample_pos == m_data->m_decode_info.length_samples) {
			// Reached end
			if (m_looping) {
				m_next_sample_pos = 0;
			} else {
				m_stopped_means_dead = true;
				return false;
			}
		}

		auto [buf, buf_end, offset_in_buf] = m_data->getOrLoadBufferAt(m_next_sample_pos);
		m_next_sample_pos = buf_end;
		assert(offset_in_buf == 0);

		alSourceQueueBuffers(m_source_id, 1, &buf);

		// Start again if queue was empty and resulted in stop
		if (getState() == AL_STOPPED) {
			play();
			warningstream << "PlayingSound::stepStream: Sound queue ran empty for \""
					<< m_data->m_decode_info.name_for_logging << "\"" << std::endl;
		}
	}

	return true;
}

bool PlayingSound::fade(f32 step, f32 target_gain) noexcept
{
	bool already_fading = m_fade_state.has_value();

	target_gain = MYMAX(target_gain, 0.0f); // 0.0f if nan
	step = target_gain - getGain() > 0.0f ? std::abs(step) : -std::abs(step);

	m_fade_state = FadeState{step, target_gain};

	return !already_fading;
}

bool PlayingSound::doFade(f32 dtime) noexcept
{
	if (!m_fade_state || isDead())
		return false;

	FadeState &fade = *m_fade_state;
	assert(fade.step != 0.0f);

	f32 current_gain = getGain();
	current_gain += fade.step * dtime;

	if (fade.step < 0.0f)
		current_gain = std::max(current_gain, fade.target_gain);
	else
		current_gain = std::min(current_gain, fade.target_gain);

	if (current_gain <= 0.0f) {
		// stop sound
		m_stopped_means_dead = true;
		alSourceStop(m_source_id);

		m_fade_state = std::nullopt;
		return false;
	}

	setGain(current_gain);

	if (current_gain == fade.target_gain) {
		m_fade_state = std::nullopt;
		return false;
	} else {
		return true;
	}
}

void PlayingSound::updatePosVel(const v3f &pos, const v3f &vel) noexcept
{
	alSourcei(m_source_id, AL_SOURCE_RELATIVE, false);
	alSource3f(m_source_id, AL_POSITION, pos.X, pos.Y, pos.Z);
	alSource3f(m_source_id, AL_VELOCITY, vel.X, vel.Y, vel.Z);
	// Using alDistanceModel(AL_INVERSE_DISTANCE_CLAMPED) and setting reference
	// distance to clamp gain at <1 node distance avoids excessive volume when
	// closer.
	alSourcef(m_source_id, AL_REFERENCE_DISTANCE, 1.0f);

	warn_if_al_error("PlayingSound::updatePosVel");
}

void PlayingSound::setGain(f32 gain) noexcept
{
	// AL_REFERENCE_DISTANCE was once reduced from 3 nodes to 1 node.
	// We compensate this by multiplying the volume by 3.
	if (m_is_positional)
		gain *= 3.0f;

	alSourcef(m_source_id, AL_GAIN, gain);
}

f32 PlayingSound::getGain() noexcept
{
	ALfloat gain;
	alGetSourcef(m_source_id, AL_GAIN, &gain);
	// Same as above, but inverse.
	if (m_is_positional)
		gain *= 1.0f/3.0f;
	return gain;
}

/*
 * OpenALSoundManager class
 */

void OpenALSoundManager::stepStreams(f32 dtime)
{
	// spread work across steps
	const size_t num_issued_sounds = std::min(
			m_sounds_streaming_current_bigstep.size(),
			(size_t)std::ceil(m_sounds_streaming_current_bigstep.size()
				* dtime / m_stream_timer)
		);

	for (size_t i = 0; i < num_issued_sounds; ++i) {
		auto wptr = std::move(m_sounds_streaming_current_bigstep.back());
		m_sounds_streaming_current_bigstep.pop_back();

		std::shared_ptr<PlayingSound> snd = wptr.lock();
		if (!snd)
			continue;

		if (!snd->stepStream())
			continue;

		// sound still lives and needs more stream-stepping => add to next bigstep
		m_sounds_streaming_next_bigstep.push_back(std::move(wptr));
	}

	m_stream_timer -= dtime;
	if (m_stream_timer <= 0.0f) {
		m_stream_timer = STREAM_BIGSTEP_TIME;
		using std::swap;
		swap(m_sounds_streaming_current_bigstep, m_sounds_streaming_next_bigstep);
	}
}

void OpenALSoundManager::doFades(f32 dtime)
{
	for (size_t i = 0; i < m_sounds_fading.size();) {
		std::shared_ptr<PlayingSound> snd = m_sounds_fading[i].lock();
		if (snd) {
			if (snd->doFade(dtime)) {
				// needs more fading later, keep in m_sounds_fading
				++i;
				continue;
			}
		}

		// sound no longer needs to be faded
		m_sounds_fading[i] = std::move(m_sounds_fading.back());
		m_sounds_fading.pop_back();
		// continue with same i
	}
}

std::shared_ptr<ISoundDataOpen> OpenALSoundManager::openSingleSound(const std::string &sound_name)
{
	// if already open, nothing to do
	auto it = m_sound_datas_open.find(sound_name);
	if (it != m_sound_datas_open.end())
		return it->second;

	// find unopened data
	auto it_unopen = m_sound_datas_unopen.find(sound_name);
	if (it_unopen == m_sound_datas_unopen.end())
		return nullptr;
	std::unique_ptr<ISoundDataUnopen> unopn_snd = std::move(it_unopen->second);
	m_sound_datas_unopen.erase(it_unopen);

	// open
	std::shared_ptr<ISoundDataOpen> opn_snd = std::move(*unopn_snd).open(sound_name);
	if (!opn_snd)
		return nullptr;
	m_sound_datas_open.emplace(sound_name, opn_snd);
	return opn_snd;
}

std::string OpenALSoundManager::getLoadedSoundNameFromGroup(const std::string &group_name)
{
	std::string chosen_sound_name = "";

	auto it_groups = m_sound_groups.find(group_name);
	if (it_groups == m_sound_groups.end())
		return "";

	std::vector<std::string> &group_sounds = it_groups->second;
	while (!group_sounds.empty()) {
		// choose one by random
		int j = myrand() % group_sounds.size();
		chosen_sound_name = group_sounds[j];

		// find chosen one
		std::shared_ptr<ISoundDataOpen> snd = openSingleSound(chosen_sound_name);
		if (snd)
			return chosen_sound_name;

		// it doesn't exist
		// remove it from the group and try again
		group_sounds[j] = std::move(group_sounds.back());
		group_sounds.pop_back();
	}

	return "";
}

std::string OpenALSoundManager::getOrLoadLoadedSoundNameFromGroup(const std::string &group_name)
{
	std::string sound_name = getLoadedSoundNameFromGroup(group_name);
	if (!sound_name.empty())
		return sound_name;

	// load
	std::vector<std::string> paths = m_fallback_path_provider
			->getLocalFallbackPathsForSoundname(group_name);
	for (const std::string &path : paths) {
		if (loadSoundFile(path, path))
			addSoundToGroup(path, group_name);
	}
	return getLoadedSoundNameFromGroup(group_name);
}

std::shared_ptr<PlayingSound> OpenALSoundManager::createPlayingSound(
		const std::string &sound_name, bool loop, f32 volume, f32 pitch,
		f32 start_time, const std::optional<std::pair<v3f, v3f>> &pos_vel_opt)
{
	infostream << "OpenALSoundManager: Creating playing sound \"" << sound_name
			<< "\"" << std::endl;
	warn_if_al_error("before createPlayingSound");

	std::shared_ptr<ISoundDataOpen> lsnd = openSingleSound(sound_name);
	if (!lsnd) {
		// does not happen because of the call to getLoadedSoundNameFromGroup
		errorstream << "OpenALSoundManager::createPlayingSound: Sound \""
				<< sound_name << "\" disappeared." << std::endl;
		return nullptr;
	}

	if (lsnd->m_decode_info.is_stereo && pos_vel_opt.has_value()) {
		warningstream << "OpenALSoundManager::createPlayingSound: "
				<< "Creating positional stereo sound \"" << sound_name << "\"."
				<< std::endl;
	}

	ALuint source_id;
	alGenSources(1, &source_id);
	if (warn_if_al_error("createPlayingSound (alGenSources)") != AL_NO_ERROR) {
		// happens ie. if there are too many sources (out of memory)
		return nullptr;
	}

	auto sound = std::make_shared<PlayingSound>(source_id, std::move(lsnd), loop,
			volume, pitch, start_time, pos_vel_opt);

	sound->play();
	if (m_is_paused)
		sound->pause();
	warn_if_al_error("createPlayingSound");
	return sound;
}

void OpenALSoundManager::playSoundGeneric(sound_handle_t id, const std::string &group_name,
		bool loop, f32 volume, f32 fade, f32 pitch, bool use_local_fallback,
		f32 start_time, const std::optional<std::pair<v3f, v3f>> &pos_vel_opt)
{
	assert(id != 0);

	if (group_name.empty()) {
		reportRemovedSound(id);
		return;
	}

	// choose random sound name from group name
	std::string sound_name = use_local_fallback ?
			getOrLoadLoadedSoundNameFromGroup(group_name) :
			getLoadedSoundNameFromGroup(group_name);
	if (sound_name.empty()) {
		infostream << "OpenALSoundManager: \"" << group_name << "\" not found."
				<< std::endl;
		reportRemovedSound(id);
		return;
	}

	volume = std::max(0.0f, volume);
	f32 target_fade_volume = volume;
	if (fade > 0.0f)
		volume = 0.0f;

	if (!(pitch > 0.0f)) {
		warningstream << "OpenALSoundManager::playSoundGeneric: Illegal pitch value: "
				<< start_time << std::endl;
		pitch = 1.0f;
	}

	if (!std::isfinite(start_time)) {
		warningstream << "OpenALSoundManager::playSoundGeneric: Illegal start_time value: "
				<< start_time << std::endl;
		start_time = 0.0f;
	}

	// play it
	std::shared_ptr<PlayingSound> sound = createPlayingSound(sound_name, loop,
			volume, pitch, start_time, pos_vel_opt);
	if (!sound) {
		reportRemovedSound(id);
		return;
	}

	// add to streaming sounds if streaming
	if (sound->isStreaming())
		m_sounds_streaming_next_bigstep.push_back(sound);

	m_sounds_playing.emplace(id, std::move(sound));

	if (fade > 0.0f)
		fadeSound(id, fade, target_fade_volume);
}

int OpenALSoundManager::removeDeadSounds()
{
	int num_deleted_sounds = 0;

	for (auto it = m_sounds_playing.begin(); it != m_sounds_playing.end();) {
		sound_handle_t id = it->first;
		PlayingSound &sound = *it->second;
		// If dead, remove it
		if (sound.isDead()) {
			it = m_sounds_playing.erase(it);
			reportRemovedSound(id);
			++num_deleted_sounds;
		} else {
			++it;
		}
	}

	return num_deleted_sounds;
}

OpenALSoundManager::OpenALSoundManager(SoundManagerSingleton *smg,
		std::unique_ptr<SoundFallbackPathProvider> fallback_path_provider) :
	Thread("OpenALSoundManager"),
	m_fallback_path_provider(std::move(fallback_path_provider)),
	m_device(smg->m_device.get()),
	m_context(smg->m_context.get())
{
	SANITY_CHECK(!!m_fallback_path_provider);

	infostream << "Audio: Initialized: OpenAL " << std::endl;
}

OpenALSoundManager::~OpenALSoundManager()
{
	infostream << "Audio: Deinitializing..." << std::endl;
}

/* Interface */

void OpenALSoundManager::step(f32 dtime)
{
	m_time_until_dead_removal -= dtime;
	if (m_time_until_dead_removal <= 0.0f) {
		if (!m_sounds_playing.empty()) {
			verbosestream << "OpenALSoundManager::step(): "
					<< m_sounds_playing.size() << " playing sounds, "
					<< m_sound_datas_unopen.size() << " unopen sounds, "
					<< m_sound_datas_open.size() << " open sounds and "
					<< m_sound_groups.size() << " sound groups loaded."
					<< std::endl;
		}

		int num_deleted_sounds = removeDeadSounds();

		if (num_deleted_sounds != 0)
			verbosestream << "OpenALSoundManager::step(): Deleted "
					<< num_deleted_sounds << " dead playing sounds." << std::endl;

		m_time_until_dead_removal = REMOVE_DEAD_SOUNDS_INTERVAL;
	}

	doFades(dtime);
	stepStreams(dtime);
}

void OpenALSoundManager::pauseAll()
{
	for (auto &snd_p : m_sounds_playing) {
		PlayingSound &snd = *snd_p.second;
		snd.pause();
	}
	m_is_paused = true;
}

void OpenALSoundManager::resumeAll()
{
	for (auto &snd_p : m_sounds_playing) {
		PlayingSound &snd = *snd_p.second;
		snd.resume();
	}
	m_is_paused = false;
}

void OpenALSoundManager::updateListener(const v3f &pos_, const v3f &vel_,
		const v3f &at_, const v3f &up_)
{
	v3f pos = swap_handedness(pos_);
	v3f vel = swap_handedness(vel_);
	v3f at = swap_handedness(at_);
	v3f up = swap_handedness(up_);
	ALfloat orientation[6] = {at.X, at.Y, at.Z, up.X, up.Y, up.Z};

	alListener3f(AL_POSITION, pos.X, pos.Y, pos.Z);
	alListener3f(AL_VELOCITY, vel.X, vel.Y, vel.Z);
	alListenerfv(AL_ORIENTATION, orientation);
	warn_if_al_error("updateListener");
}

void OpenALSoundManager::setListenerGain(f32 gain)
{
	alListenerf(AL_GAIN, gain);
}

bool OpenALSoundManager::loadSoundFile(const std::string &name, const std::string &filepath)
{
	// do not add twice
	if (m_sound_datas_open.count(name) != 0 || m_sound_datas_unopen.count(name) != 0)
		return false;

	// coarse check
	if (!fs::IsFile(filepath))
		return false;

	loadSoundFileNoCheck(name, filepath);
	return true;
}

bool OpenALSoundManager::loadSoundData(const std::string &name, std::string &&filedata)
{
	// do not add twice
	if (m_sound_datas_open.count(name) != 0 || m_sound_datas_unopen.count(name) != 0)
		return false;

	loadSoundDataNoCheck(name, std::move(filedata));
	return true;
}

void OpenALSoundManager::loadSoundFileNoCheck(const std::string &name, const std::string &filepath)
{
	// remember for lazy loading
	m_sound_datas_unopen.emplace(name, std::make_unique<SoundDataUnopenFile>(filepath));
}

void OpenALSoundManager::loadSoundDataNoCheck(const std::string &name, std::string &&filedata)
{
	// remember for lazy loading
	m_sound_datas_unopen.emplace(name, std::make_unique<SoundDataUnopenBuffer>(std::move(filedata)));
}

void OpenALSoundManager::addSoundToGroup(const std::string &sound_name, const std::string &group_name)
{
	auto it_groups = m_sound_groups.find(group_name);
	if (it_groups != m_sound_groups.end())
		it_groups->second.push_back(sound_name);
	else
		m_sound_groups.emplace(group_name, std::vector<std::string>{sound_name});
}

void OpenALSoundManager::playSound(sound_handle_t id, const SoundSpec &spec)
{
	return playSoundGeneric(id, spec.name, spec.loop, spec.gain, spec.fade, spec.pitch,
			spec.use_local_fallback, spec.start_time, std::nullopt);
}

void OpenALSoundManager::playSoundAt(sound_handle_t id, const SoundSpec &spec,
		const v3f &pos_, const v3f &vel_)
{
	std::optional<std::pair<v3f, v3f>> pos_vel_opt({
			swap_handedness(pos_),
			swap_handedness(vel_)
		});

	return playSoundGeneric(id, spec.name, spec.loop, spec.gain, spec.fade, spec.pitch,
			spec.use_local_fallback, spec.start_time, pos_vel_opt);
}

void OpenALSoundManager::stopSound(sound_handle_t sound)
{
	m_sounds_playing.erase(sound);
	reportRemovedSound(sound);
}

void OpenALSoundManager::fadeSound(sound_handle_t soundid, f32 step, f32 target_gain)
{
	// Ignore the command if step isn't valid.
	if (step == 0.0f)
		return;
	auto sound_it = m_sounds_playing.find(soundid);
	if (sound_it == m_sounds_playing.end())
		return; // No sound to fade
	PlayingSound &sound = *sound_it->second;
	if (sound.fade(step, target_gain))
		m_sounds_fading.emplace_back(sound_it->second);
}

void OpenALSoundManager::updateSoundPosVel(sound_handle_t id, const v3f &pos_,
		const v3f &vel_)
{
	v3f pos = swap_handedness(pos_);
	v3f vel = swap_handedness(vel_);

	auto i = m_sounds_playing.find(id);
	if (i == m_sounds_playing.end())
		return;
	i->second->updatePosVel(pos, vel);
}

void *OpenALSoundManager::run()
{
	using namespace sound_manager_messages_to_mgr;

	struct MsgVisitor {
		enum class Result { Ok, Empty, StopRequested };

		OpenALSoundManager &mgr;

		Result operator()(std::monostate &&) {
			return Result::Empty; }

		Result operator()(PauseAll &&) {
			mgr.pauseAll(); return Result::Ok; }
		Result operator()(ResumeAll &&) {
			mgr.resumeAll(); return Result::Ok; }

		Result operator()(UpdateListener &&msg) {
			mgr.updateListener(msg.pos_, msg.vel_, msg.at_, msg.up_); return Result::Ok; }
		Result operator()(SetListenerGain &&msg) {
			mgr.setListenerGain(msg.gain); return Result::Ok; }

		Result operator()(LoadSoundFile &&msg) {
			mgr.loadSoundFileNoCheck(msg.name, msg.filepath); return Result::Ok; }
		Result operator()(LoadSoundData &&msg) {
			mgr.loadSoundDataNoCheck(msg.name, std::move(msg.filedata)); return Result::Ok; }
		Result operator()(AddSoundToGroup &&msg) {
			mgr.addSoundToGroup(msg.sound_name, msg.group_name); return Result::Ok; }

		Result operator()(PlaySound &&msg) {
			mgr.playSound(msg.id, msg.spec); return Result::Ok; }
		Result operator()(PlaySoundAt &&msg) {
			mgr.playSoundAt(msg.id, msg.spec, msg.pos_, msg.vel_); return Result::Ok; }
		Result operator()(StopSound &&msg) {
			mgr.stopSound(msg.sound); return Result::Ok; }
		Result operator()(FadeSound &&msg) {
			mgr.fadeSound(msg.soundid, msg.step, msg.target_gain); return Result::Ok; }
		Result operator()(UpdateSoundPosVel &&msg) {
			mgr.updateSoundPosVel(msg.sound, msg.pos_, msg.vel_); return Result::Ok; }

		Result operator()(PleaseStop &&msg) {
			return Result::StopRequested; }
	};

	u64 t_step_start = porting::getTimeMs();
	while (true) {
		auto get_time_since_last_step = [&] {
			return (f32)(porting::getTimeMs() - t_step_start);
		};
		auto get_remaining_timeout = [&] {
			return (s32)((1.0e3f * PROXYSOUNDMGR_DTIME) - get_time_since_last_step());
		};

		bool stop_requested = false;

		while (true) {
			SoundManagerMsgToMgr msg =
					m_queue_to_mgr.pop_frontNoEx(std::max(get_remaining_timeout(), 0));

			MsgVisitor::Result res = std::visit(MsgVisitor{*this}, std::move(msg));

			if (res == MsgVisitor::Result::Empty && get_remaining_timeout() <= 0) {
				break; // finished sleeping
			} else if (res == MsgVisitor::Result::StopRequested) {
				stop_requested = true;
				break;
			}
		}
		if (stop_requested)
			break;

		f32 dtime = get_time_since_last_step();
		t_step_start = porting::getTimeMs();
		step(dtime);
	}

	send(sound_manager_messages_to_proxy::Stopped{});

	return nullptr;
}

/*
 * ProxySoundManager class
 */

ProxySoundManager::MsgResult ProxySoundManager::handleMsg(SoundManagerMsgToProxy &&msg)
{
	using namespace sound_manager_messages_to_proxy;

	return std::visit([&](auto &&msg) {
			using T = std::decay_t<decltype(msg)>;

			if constexpr (std::is_same_v<T, std::monostate>)
				return MsgResult::Empty;
			else if constexpr (std::is_same_v<T, ReportRemovedSound>)
				reportRemovedSound(msg.id);
			else if constexpr (std::is_same_v<T, Stopped>)
				return MsgResult::Stopped;

			return MsgResult::Ok;
		},
		std::move(msg));
}

ProxySoundManager::~ProxySoundManager()
{
	if (m_sound_manager.isRunning()) {
		send(sound_manager_messages_to_mgr::PleaseStop{});

		// recv until it stopped
		auto recv = [&]	{
			return m_sound_manager.m_queue_to_proxy.pop_frontNoEx();
		};

		while (true) {
			if (handleMsg(recv()) == MsgResult::Stopped)
				break;
		}

		// join
		m_sound_manager.stop();
		SANITY_CHECK(m_sound_manager.wait());
	}
}

void ProxySoundManager::step(f32 dtime)
{
	auto recv = [&]	{
		return m_sound_manager.m_queue_to_proxy.pop_frontNoEx(0);
	};

	while (true) {
		MsgResult res = handleMsg(recv());
		if (res == MsgResult::Empty)
			break;
		else if (res == MsgResult::Stopped)
			throw std::runtime_error("OpenALSoundManager stopped unexpectedly");
	}
}

void ProxySoundManager::pauseAll()
{
	send(sound_manager_messages_to_mgr::PauseAll{});
}

void ProxySoundManager::resumeAll()
{
	send(sound_manager_messages_to_mgr::ResumeAll{});
}

void ProxySoundManager::updateListener(const v3f &pos_, const v3f &vel_,
		const v3f &at_, const v3f &up_)
{
	send(sound_manager_messages_to_mgr::UpdateListener{pos_, vel_, at_, up_});
}

void ProxySoundManager::setListenerGain(f32 gain)
{
	send(sound_manager_messages_to_mgr::SetListenerGain{gain});
}

bool ProxySoundManager::loadSoundFile(const std::string &name,
		const std::string &filepath)
{
	// do not add twice
	if (m_known_sound_names.count(name) != 0)
		return false;

	// coarse check
	if (!fs::IsFile(filepath))
		return false;

	send(sound_manager_messages_to_mgr::LoadSoundFile{name, filepath});

	m_known_sound_names.insert(name);
	return true;
}

bool ProxySoundManager::loadSoundData(const std::string &name, std::string &&filedata)
{
	// do not add twice
	if (m_known_sound_names.count(name) != 0)
		return false;

	send(sound_manager_messages_to_mgr::LoadSoundData{name, std::move(filedata)});

	m_known_sound_names.insert(name);
	return true;
}

void ProxySoundManager::addSoundToGroup(const std::string &sound_name,
		const std::string &group_name)
{
	send(sound_manager_messages_to_mgr::AddSoundToGroup{sound_name, group_name});
}

void ProxySoundManager::playSound(sound_handle_t id, const SoundSpec &spec)
{
	if (id == 0)
		id = allocateId(1);
	send(sound_manager_messages_to_mgr::PlaySound{id, spec});
}

void ProxySoundManager::playSoundAt(sound_handle_t id, const SoundSpec &spec, const v3f &pos_,
		const v3f &vel_)
{
	if (id == 0)
		id = allocateId(1);
	send(sound_manager_messages_to_mgr::PlaySoundAt{id, spec, pos_, vel_});
}

void ProxySoundManager::stopSound(sound_handle_t sound)
{
	send(sound_manager_messages_to_mgr::StopSound{sound});
}

void ProxySoundManager::fadeSound(sound_handle_t soundid, f32 step, f32 target_gain)
{
	send(sound_manager_messages_to_mgr::FadeSound{soundid, step, target_gain});
}

void ProxySoundManager::updateSoundPosVel(sound_handle_t sound, const v3f &pos_, const v3f &vel_)
{
	send(sound_manager_messages_to_mgr::UpdateSoundPosVel{sound, pos_, vel_});
}
