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

#include "playing_sound.h"

#include "debug.h"
#include <cassert>
#include <cmath>

namespace sound {

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

	if (getState() == AL_PAUSED)
		return true;

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

} // namespace sound
