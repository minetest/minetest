/*
Minetest
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

#include "sound_openal.h"

#if defined(_WIN32)
	#include <al.h>
	#include <alc.h>
	//#include <alext.h>
#elif defined(__APPLE__)
	#include <OpenAL/al.h>
	#include <OpenAL/alc.h>
	//#include <OpenAL/alext.h>
#else
	#include <AL/al.h>
	#include <AL/alc.h>
	#include <AL/alext.h>
#endif
#include <vorbis/vorbisfile.h>
#include "log.h"
#include "filesys.h"
#include "util/numeric.h" // myrand()
#include "debug.h" // assert()
#include "porting.h"
#include <map>
#include <vector>
#include <fstream>

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

static ALenum warn_if_error(ALenum err, const char *desc)
{
	if(err == AL_NO_ERROR)
		return err;
	errorstream<<"WARNING: "<<desc<<": "<<alErrorString(err)<<std::endl;
	return err;
}

void f3_set(ALfloat *f3, v3f v)
{
	f3[0] = v.X;
	f3[1] = v.Y;
	f3[2] = v.Z;
}

struct SoundBuffer
{
	ALenum format;
	ALsizei freq;
	ALuint buffer_id;
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
	
	// Do a dumb-ass static string copy for old versions of ov_fopen
	// because they expect a non-const char*
	char nonconst[10000];
	snprintf(nonconst, 10000, "%s", filepath.c_str());
	// Try opening the given file
	//if(ov_fopen(filepath.c_str(), &oggFile) != 0)
	if(ov_fopen(nonconst, &oggFile) != 0)
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

	alGenBuffers(1, &snd->buffer_id);
	alBufferData(snd->buffer_id, snd->format,
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
	ALuint source_id;
	bool loop;
};

class OpenALSoundManager: public ISoundManager
{
private:
	OnDemandSoundFetcher *m_fetcher;
	ALCdevice *m_device;
	ALCcontext *m_context;
	bool m_can_vorbis;
	int m_next_id;
	std::map<std::string, std::vector<SoundBuffer*> > m_buffers;
	std::map<int, PlayingSound*> m_sounds_playing;
	v3f m_listener_pos;
public:
	bool m_is_initialized;
	OpenALSoundManager(OnDemandSoundFetcher *fetcher):
		m_fetcher(fetcher),
		m_device(NULL),
		m_context(NULL),
		m_can_vorbis(false),
		m_next_id(1),
		m_is_initialized(false)
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

		m_is_initialized = true;
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

		for (std::map<std::string, std::vector<SoundBuffer*> >::iterator i = m_buffers.begin();
				i != m_buffers.end(); i++) {
			for (std::vector<SoundBuffer*>::iterator iter = (*i).second.begin();
					iter != (*i).second.end(); iter++) {
				delete *iter;
			}
			(*i).second.clear();
		}
		m_buffers.clear();
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
		m_buffers[name] = bufs;
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

	PlayingSound* createPlayingSound(SoundBuffer *buf, bool loop,
			float volume)
	{
		infostream<<"OpenALSoundManager: Creating playing sound"<<std::endl;
		assert(buf);
		PlayingSound *sound = new PlayingSound;
		assert(sound);
		warn_if_error(alGetError(), "before createPlayingSound");
		alGenSources(1, &sound->source_id);
		alSourcei(sound->source_id, AL_BUFFER, buf->buffer_id);
		alSourcei(sound->source_id, AL_SOURCE_RELATIVE, true);
		alSource3f(sound->source_id, AL_POSITION, 0, 0, 0);
		alSource3f(sound->source_id, AL_VELOCITY, 0, 0, 0);
		alSourcei(sound->source_id, AL_LOOPING, loop ? AL_TRUE : AL_FALSE);
		volume = MYMAX(0.0, volume);
		alSourcef(sound->source_id, AL_GAIN, volume);
		alSourcePlay(sound->source_id);
		warn_if_error(alGetError(), "createPlayingSound");
		return sound;
	}

	PlayingSound* createPlayingSoundAt(SoundBuffer *buf, bool loop,
			float volume, v3f pos)
	{
		infostream<<"OpenALSoundManager: Creating positional playing sound"
				<<std::endl;
		assert(buf);
		PlayingSound *sound = new PlayingSound;
		assert(sound);
		warn_if_error(alGetError(), "before createPlayingSoundAt");
		alGenSources(1, &sound->source_id);
		alSourcei(sound->source_id, AL_BUFFER, buf->buffer_id);
		alSourcei(sound->source_id, AL_SOURCE_RELATIVE, false);
		alSource3f(sound->source_id, AL_POSITION, pos.X, pos.Y, pos.Z);
		alSource3f(sound->source_id, AL_VELOCITY, 0, 0, 0);
		//alSourcef(sound->source_id, AL_ROLLOFF_FACTOR, 0.7);
		alSourcef(sound->source_id, AL_REFERENCE_DISTANCE, 30.0);
		alSourcei(sound->source_id, AL_LOOPING, loop ? AL_TRUE : AL_FALSE);
		volume = MYMAX(0.0, volume);
		alSourcef(sound->source_id, AL_GAIN, volume);
		alSourcePlay(sound->source_id);
		warn_if_error(alGetError(), "createPlayingSoundAt");
		return sound;
	}

	int playSoundRaw(SoundBuffer *buf, bool loop, float volume)
	{
		assert(buf);
		PlayingSound *sound = createPlayingSound(buf, loop, volume);
		if(!sound)
			return -1;
		int id = m_next_id++;
		m_sounds_playing[id] = sound;
		return id;
	}

	int playSoundRawAt(SoundBuffer *buf, bool loop, float volume, v3f pos)
	{
		assert(buf);
		PlayingSound *sound = createPlayingSoundAt(buf, loop, volume, pos);
		if(!sound)
			return -1;
		int id = m_next_id++;
		m_sounds_playing[id] = sound;
		return id;
	}
	
	void deleteSound(int id)
	{
		std::map<int, PlayingSound*>::iterator i =
				m_sounds_playing.find(id);
		if(i == m_sounds_playing.end())
			return;
		PlayingSound *sound = i->second;
		
		alDeleteSources(1, &sound->source_id);

		delete sound;
		m_sounds_playing.erase(id);
	}

	/* If buffer does not exist, consult the fetcher */
	SoundBuffer* getFetchBuffer(const std::string &name)
	{
		SoundBuffer *buf = getBuffer(name);
		if(buf)
			return buf;
		if(!m_fetcher)
			return NULL;
		std::set<std::string> paths;
		std::set<std::string> datas;
		m_fetcher->fetchSounds(name, paths, datas);
		for(std::set<std::string>::iterator i = paths.begin();
				i != paths.end(); i++){
			loadSoundFile(name, *i);
		}
		for(std::set<std::string>::iterator i = datas.begin();
				i != datas.end(); i++){
			loadSoundData(name, *i);
		}
		return getBuffer(name);
	}
	
	// Remove stopped sounds
	void maintain()
	{
		verbosestream<<"OpenALSoundManager::maintain(): "
				<<m_sounds_playing.size()<<" playing sounds, "
				<<m_buffers.size()<<" sound names loaded"<<std::endl;
		std::set<int> del_list;
		for(std::map<int, PlayingSound*>::iterator
				i = m_sounds_playing.begin();
				i != m_sounds_playing.end(); i++)
		{
			int id = i->first;
			PlayingSound *sound = i->second;
			// If not playing, remove it
			{
				ALint state;
				alGetSourcei(sound->source_id, AL_SOURCE_STATE, &state);
				if(state != AL_PLAYING){
					del_list.insert(id);
				}
			}
		}
		if(del_list.size() != 0)
			verbosestream<<"OpenALSoundManager::maintain(): deleting "
					<<del_list.size()<<" playing sounds"<<std::endl;
		for(std::set<int>::iterator i = del_list.begin();
				i != del_list.end(); i++)
		{
			deleteSound(*i);
		}
	}

	/* Interface */

	bool loadSoundFile(const std::string &name,
			const std::string &filepath)
	{
		SoundBuffer *buf = loadOggFile(filepath);
		if(buf)
			addBuffer(name, buf);
		return false;
	}
	bool loadSoundData(const std::string &name,
			const std::string &filedata)
	{
		// The vorbis API sucks; just write it to a file and use vorbisfile
		// TODO: Actually load it directly from memory
		std::string basepath = porting::path_user + DIR_DELIM + "cache" +
				DIR_DELIM + "tmp";
		std::string path = basepath + DIR_DELIM + "tmp.ogg";
		verbosestream<<"OpenALSoundManager::loadSoundData(): Writing "
				<<"temporary file to ["<<path<<"]"<<std::endl;
		fs::CreateAllDirs(basepath);
		std::ofstream of(path.c_str(), std::ios::binary);
		of.write(filedata.c_str(), filedata.size());
		of.close();
		return loadSoundFile(name, path);
	}

	void updateListener(v3f pos, v3f vel, v3f at, v3f up)
	{
		m_listener_pos = pos;
		alListener3f(AL_POSITION, pos.X, pos.Y, pos.Z);
		alListener3f(AL_VELOCITY, vel.X, vel.Y, vel.Z);
		ALfloat f[6];
		f3_set(f, at);
		f3_set(f+3, -up);
		alListenerfv(AL_ORIENTATION, f);
		warn_if_error(alGetError(), "updateListener");
	}
	
	void setListenerGain(float gain)
	{
		alListenerf(AL_GAIN, gain);
	}

	int playSound(const std::string &name, bool loop, float volume)
	{
		maintain();
		if(name == "")
			return 0;
		SoundBuffer *buf = getFetchBuffer(name);
		if(!buf){
			infostream<<"OpenALSoundManager: \""<<name<<"\" not found."
					<<std::endl;
			return -1;
		}
		return playSoundRaw(buf, loop, volume);
	}
	int playSoundAt(const std::string &name, bool loop, float volume, v3f pos)
	{
		maintain();
		if(name == "")
			return 0;
		SoundBuffer *buf = getFetchBuffer(name);
		if(!buf){
			infostream<<"OpenALSoundManager: \""<<name<<"\" not found."
					<<std::endl;
			return -1;
		}
		return playSoundRawAt(buf, loop, volume, pos);
	}
	void stopSound(int sound)
	{
		maintain();
		deleteSound(sound);
	}
	bool soundExists(int sound)
	{
		maintain();
		return (m_sounds_playing.count(sound) != 0);
	}
	void updateSoundPosition(int id, v3f pos)
	{
		std::map<int, PlayingSound*>::iterator i =
				m_sounds_playing.find(id);
		if(i == m_sounds_playing.end())
			return;
		PlayingSound *sound = i->second;

		alSourcei(sound->source_id, AL_SOURCE_RELATIVE, false);
		alSource3f(sound->source_id, AL_POSITION, pos.X, pos.Y, pos.Z);
		alSource3f(sound->source_id, AL_VELOCITY, 0, 0, 0);
		alSourcef(sound->source_id, AL_REFERENCE_DISTANCE, 30.0);
	}
};

ISoundManager *createOpenALSoundManager(OnDemandSoundFetcher *fetcher)
{
	OpenALSoundManager *m = new OpenALSoundManager(fetcher);
	if(m->m_is_initialized)
		return m;
	delete m;
	return NULL;
};

