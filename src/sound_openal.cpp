/*
Minetest-c55
Copyright (C) 2012 celeron55, Perttu Ahola <celeron55@gmail.com>
OpenAL support based on work by:
Copyright (C) 2011 Sebastian 'Bahamada' Rühl
Copyright (C) 2011 Cyriaque 'Cisoun' Skrapits <cysoun@gmail.com>
Copyright (C) 2011 Giuseppe Bilotta <giuseppe.bilotta@gmail.com>

This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License along
with this program; ifnot, write to the Free Software Foundation, Inc.,
51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
*/

#include "sound_openal.h"

#if defined(_MSC_VER)
	#include <al.h>
	#include <alc.h>
	#include <alext.h>
#elif defined(__APPLE__)
	#include <OpenAL/al.h>
	#include <OpenAL/alc.h>
	#include <OpenAL/alext.h>
#else
	#include <AL/al.h>
	#include <AL/alc.h>
	#include <AL/alext.h>
#endif
#include <vorbis/vorbisfile.h>
#include "log.h"
#include <map>
#include <vector>
#include "utility.h" // myrand()

#define BUFFER_SIZE 30000

static const char *alcErrorString(ALCenum err)
{
	switch (err) {
	case ALC_NO_ERROR:
		return "no error";
	case ALC_INVALID_DEVICE:
		return "invalid device";
	case ALC_INVALID_CONTEXT:
		return "invalid context";
	case ALC_INVALID_ENUM:
		return "invalid enum";
	case ALC_INVALID_VALUE:
		return "invalid value";
	case ALC_OUT_OF_MEMORY:
		return "out of memory";
	default:
		return "<unknown OpenAL error>";
	}
}

static const char *alErrorString(ALenum err)
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

void f3_set(ALfloat *f3, v3f v)
{
	f3[0] = v.X;
	f3[1] = v.Y;
	f3[2] = v.Z;
}

struct SoundBuffer
{
	ALenum	format;
	ALsizei	freq;
	ALuint	bufferID;
	std::vector<char> buffer;
};

SoundBuffer* loadOggFile(const std::string &filepath)
{
	int endian = 0; // 0 for Little-Endian, 1 for Big-Endian
	int bitStream;
	long bytes;
	char array[BUFFER_SIZE]; // Local fixed size array
	vorbis_info *pInfo;
	OggVorbis_File oggFile;

	// Try opening the given file
	if(ov_fopen(filepath.c_str(), &oggFile) != 0)
	{
		infostream<<"Audio: Error opening "<<filepath<<" for decoding"<<std::endl;
		return NULL;
	}

	SoundBuffer *snd = new SoundBuffer;

	// Get some information about the OGG file
	pInfo = ov_info(&oggFile, -1);

	// Check the number of channels... always use 16-bit samples
	if(pInfo->channels == 1)
		snd->format = AL_FORMAT_MONO16;
	else
		snd->format = AL_FORMAT_STEREO16;

	// The frequency of the sampling rate
	snd->freq = pInfo->rate;

	// Keep reading until all is read
	do
	{
		// Read up to a buffer's worth of decoded sound data
		bytes = ov_read(&oggFile, array, BUFFER_SIZE, endian, 2, 1, &bitStream);

		if(bytes < 0)
		{
			ov_clear(&oggFile);
			infostream<<"Audio: Error decoding "<<filepath<<std::endl;
			return NULL;
		}

		// Append to end of buffer
		snd->buffer.insert(snd->buffer.end(), array, array + bytes);
	} while (bytes > 0);

	alGenBuffers(1, &snd->bufferID);
	alBufferData(snd->bufferID, snd->format,
			&(snd->buffer[0]), snd->buffer.size(),
			snd->freq);

	ALenum error = alGetError();

	if(error != AL_NO_ERROR){
		infostream<<"Audio: OpenAL error: "<<alErrorString(error)
				<<"preparing sound buffer"<<std::endl;
	}

	infostream<<"Audio file "<<filepath<<" loaded"<<std::endl;

	// Clean up!
	ov_clear(&oggFile);

	return snd;
}

struct PlayingSound
{
};

class OpenALSoundManager: public ISoundManager
{
private:
	ALCdevice *m_device;
	ALCcontext *m_context;
	bool m_can_vorbis;
	int m_next_id;
	std::map<std::string, std::vector<SoundBuffer*> > m_buffers;
	std::map<int, PlayingSound*> m_sounds_playing;
public:
	OpenALSoundManager():
		m_device(NULL),
		m_context(NULL),
		m_can_vorbis(false),
		m_next_id(1)
	{
		ALCenum error = ALC_NO_ERROR;
		
		infostream<<"Audio: Initializing..."<<std::endl;

		m_device = alcOpenDevice(NULL);
		if(!m_device){
			infostream<<"Audio: No audio device available, audio system "
				<<"not initialized"<<std::endl;
			return;
		}

		if(alcIsExtensionPresent(m_device, "EXT_vorbis")){
			infostream<<"Audio: Vorbis extension present"<<std::endl;
			m_can_vorbis = true;
		} else{
			infostream<<"Audio: Vorbis extension NOT present"<<std::endl;
			m_can_vorbis = false;
		}

		m_context = alcCreateContext(m_device, NULL);
		if(!m_context){
			error = alcGetError(m_device);
			infostream<<"Audio: Unable to initialize audio context, "
					<<"aborting audio initialization ("<<alcErrorString(error)
					<<")"<<std::endl;
			alcCloseDevice(m_device);
			m_device = NULL;
			return;
		}

		if(!alcMakeContextCurrent(m_context) ||
				(error = alcGetError(m_device) != ALC_NO_ERROR))
		{
			infostream<<"Audio: Error setting audio context, aborting audio "
					<<"initialization ("<<alcErrorString(error)<<")"<<std::endl;
			alcDestroyContext(m_context);
			m_context = NULL;
			alcCloseDevice(m_device);
			m_device = NULL;
			return;
		}

		alDistanceModel(AL_EXPONENT_DISTANCE);

		infostream<<"Audio: Initialized: OpenAL "<<alGetString(AL_VERSION)
				<<", using "<<alcGetString(m_device, ALC_DEVICE_SPECIFIER)
				<<std::endl;
	}

	~OpenALSoundManager()
	{
		infostream<<"Audio: Deinitializing..."<<std::endl;
		// KABOOM!
		// TODO: Clear SoundBuffers
		alcMakeContextCurrent(NULL);
		alcDestroyContext(m_context);
		m_context = NULL;
		alcCloseDevice(m_device);
		m_device = NULL;
		infostream<<"Audio: Deinitialized."<<std::endl;
	}
	
	void addBuffer(const std::string &name, SoundBuffer *buf)
	{
		std::map<std::string, std::vector<SoundBuffer*> >::iterator i =
				m_buffers.find(name);
		if(i != m_buffers.end()){
			i->second.push_back(buf);
			return;
		}
		std::vector<SoundBuffer*> bufs;
		bufs.push_back(buf);
		return;
	}

	SoundBuffer* getBuffer(const std::string &name)
	{
		std::map<std::string, std::vector<SoundBuffer*> >::iterator i =
				m_buffers.find(name);
		if(i == m_buffers.end())
			return NULL;
		std::vector<SoundBuffer*> &bufs = i->second;
		int j = myrand() % bufs.size();
		return bufs[j];
	}

	void updateListener(v3f pos, v3f vel, v3f at, v3f up)
	{
		ALfloat f[6];
		f3_set(f, pos);
		alListenerfv(AL_POSITION, f);
		f3_set(f, vel);
		alListenerfv(AL_VELOCITY, f);
		f3_set(f, at);
		f3_set(f+3, up);
		alListenerfv(AL_ORIENTATION, f);
	}
	
	bool loadSound(const std::string &name,
			const std::string &filepath)
	{
		SoundBuffer *buf = loadOggFile(filepath);
		if(buf)
			addBuffer(name, buf);
		return false;
	}
	bool loadSound(const std::string &name,
			const std::vector<char> &filedata)
	{
		errorstream<<"OpenALSoundManager: Loading from filedata not"
				" implemented"<<std::endl;
		return false;
	}

	int playSound(const std::string &name, int loopcount,
			float volume)
	{
		return -1;
	}
	int playSoundAt(const std::string &name, int loopcount,
			v3f pos, float volume)
	{
		return -1;
	}
	void stopSound(int sound)
	{
	}
};

ISoundManager *createSoundManager()
{
	return new OpenALSoundManager();
};

