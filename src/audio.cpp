/*
Minetest audio system
Copyright (C) 2011 Giuseppe Bilotta <giuseppe.bilotta@gmail.com>
Copyright (C) 2011 Sebastian 'Bahamada' Rühl
Copyright (C) 2012 Cyriaque 'Cisoun' Skrapits <cysoun@gmail.com>

Part of the minetest project
Copyright (C) 2010-2012 celeron55, Perttu Ahola <celeron55@gmail.com>

This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License along
with this program; if not, write to the Free Software Foundation, Inc.,
51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
*/

#include "audio.h"

/*
================================================================================
	Audio
================================================================================
*/

void Audio::init()
{
	dstream << "Audio: initialization" << std::endl;

	// Initialize the OpenAL library
  	//alutInit(0, NULL);
  	m_device = alcOpenDevice(NULL);
	if (m_device) {
		m_context = alcCreateContext(m_device, NULL);
		alcMakeContextCurrent(m_context);
	}
  	m_position = {0.0f, 0.0f, 0.0f};
  	alListener3f(AL_POSITION, m_position.X, m_position.Y, m_position.Z);
}

void Audio::shutdown()
{
	dstream << "Audio: shutdown" << std::endl;

	// Delete each sound buffer in the cache.
	for (std::map<std::string,SoundBuffer*>::iterator it = m_buffer.begin(); it != m_buffer.end(); ++it)
	{
		delete (*it).second;
		m_buffer.erase(it);
	}
	
	// Clean up the OpenAL library.
	//alutExit();
	m_context = alcGetCurrentContext();
	m_device = alcGetContextsDevice(m_context);
	alcMakeContextCurrent(NULL);
	alcDestroyContext(m_context);
	alcCloseDevice(m_device);
}

SoundSource * Audio::createSource(std::string filename, SoundSource::Type type)
{
	// Check if the buffer already exists.
	if (m_buffer.find(filename) == m_buffer.end())
	{
		// Nope!
		this->registerSound(filename);
	}

	// Let's create the source.
	SoundSource * m_source = new SoundSource(m_buffer[filename], type);

	return m_source;
}

void Audio::destroySource(SoundSource * ss)
{
	delete ss;
}

void Audio::play(std::string filename)
{
	dstream << "Audio: simple playback called for " << filename << std::endl;
	SoundSource * m_source = createSource(filename, SoundSource::SOUND);
	PlaybackThread * pt = new PlaybackThread(m_source);
	pt->Start();
}

void Audio::setVolume(float volume)
{
	// Check if the gain is correct.
	if (volume < 0.0f)
		volume = 0.0f;
	else if (volume > 1.0f)
		volume = 1.0f;

	// Change the gain.
	alListenerf(AL_GAIN, (ALfloat)volume);
}

void Audio::setListenerPos(v3f position)
{
	m_position = position;
	alListener3f(AL_POSITION, position.X, position.Y, position.Z);
}

void Audio::registerSound(std::string filename)
{
	SoundBuffer * sb = new SoundBuffer(filename);
	m_buffer[filename] = sb; // Let's store that sh.. in the cache!
}

SoundBuffer * Audio::getSound(std::string filename)
{
	if (m_buffer[filename] != NULL)
		return m_buffer[filename];
	else
		return NULL;
}



/*
================================================================================
	SoundBuffer
================================================================================
*/

SoundBuffer::SoundBuffer(const std::string filename)
{
	dstream << "Audio: creating new sound buffer for " << filename;
	m_filename = filename;

	// TODO: Exception
	if (fs::PathExists(filename))
	{
		dstream << " [FOUND]" << std::endl;
		load(filename);
	}
	else
		dstream << " [NOT FOUND]" << std::endl;
}

SoundBuffer::~SoundBuffer()
{
	dstream << "Audio: removing sound buffer " << m_filename << std::endl;
}

void SoundBuffer::load(const std::string filename)
{
	// NOTE: This code is based on this exemple:
	// http://www.gamedev.net/page/resources/_/technical/game-programming/introduction-to-ogg-vorbis-r2031

	int endian = 0;             // 0 for Little-Endian, 1 for Big-Endian
	int bitStream;
	long bytes;
	char array[BUFFER_SIZE];    // Local fixed size array.
	FILE * f;

	// Open for binary reading.
	f = fopen(filename.c_str(), "rb");
	vorbis_info *pInfo;
	OggVorbis_File oggFile;
	ov_open(f, &oggFile, NULL, 0);
	
	// Get some information about the OGG file.
	pInfo = ov_info(&oggFile, -1);

	// Check the number of channels... always use 16-bit samples.
	if (pInfo->channels == 1)
		m_format = AL_FORMAT_MONO16;
	else
		m_format = AL_FORMAT_STEREO16;

	// The frequency of the sampling rate.
	m_freq = pInfo->rate;
	
	do {
		// Read up to a buffer's worth of decoded sound data.
		bytes = ov_read(&oggFile, array, BUFFER_SIZE, endian, 2, 1, &bitStream);
		// Append to end of buffer.
		m_buffer.insert(m_buffer.end(), array, array + bytes);
	} while (bytes > 0);
	
	// Clean up!
	ov_clear(&oggFile);
}



/*
================================================================================
	SoundSource
================================================================================
*/

SoundSource::SoundSource(SoundBuffer * soundbuffer, Type type) : 
	m_soundbuffer(soundbuffer),
	m_type(type)
{
	// Create sound buffer and source.
    alGenBuffers(1, &m_buffer);
    alGenSources(1, &m_source);
    
    // Set the source and listener to the same location
    alSource3f(m_source, AL_POSITION, 0.0f, 0.0f, 0.0f);
    
    // Upload sound data to buffer.
	std::vector<char> buffer = m_soundbuffer->getBuffer();
	alBufferData(m_buffer, m_soundbuffer->getFormat(), &buffer[0], static_cast<ALsizei>(buffer.size()), m_soundbuffer->getFreq());

	// Attach sound buffer to source.
	alSourcei(m_source, AL_BUFFER, m_buffer);
	if (type == AMBIANT) alSourcei(m_source, AL_LOOPING, AL_TRUE); // Has to loop?
}

SoundSource::~SoundSource()
{
	// Clean up sound buffer and source.
	alDeleteBuffers(1, &m_buffer);
	alDeleteSources(1, &m_source);
}

void SoundSource::pause()
{
	// Pause the sound.
	alSourcePause(m_source);
}

void SoundSource::play()
{
	// Let's play this!
	alSourcePlay(m_source);
}

void SoundSource::stop()
{
	// Stop! (Hammer time!)
	alSourceStop(m_source);
}

float SoundSource::getGain()
{
	ALfloat gain;
	alGetSourcef(m_source, AL_GAIN, &gain);
	return (float)gain;
}

void SoundSource::setGain(float gain)
{
	if (gain < 0.0f) gain = 0.0f;
	else if (gain > 1.0f) gain = 1.0f; // Useful?
	alSourcef(m_source, AL_GAIN, gain);
}

float SoundSource::getPitch()
{
	ALfloat pitch;
	alGetSourcef(m_source, AL_PITCH, &pitch);
	return (float)pitch;
}

void SoundSource::setPitch(float multiplier)
{
	if (multiplier < 0) multiplier = 0;
	alSourcef(m_source, AL_PITCH, (ALfloat)multiplier);
}

ALint SoundSource::getState()
{
	// Query the state of the source and put it in m_state.
	// Available states:
	// - AL_INITIAL (not used here)
	// - AL_PLAYING
	// - AL_PAUSED
	// - AL_STOPPED (not used here)
	alGetSourcei(m_source, AL_SOURCE_STATE, &m_state);
	return m_state;
}
