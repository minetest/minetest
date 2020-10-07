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
	#define OPENAL_DEPRECATED
	#include <OpenAL/al.h>
	#include <OpenAL/alc.h>
	//#include <OpenAL/alext.h>
#else
	#include <AL/al.h>
	#include <AL/alc.h>
	#include <AL/alext.h>
#endif
#include <cmath>
#include <vorbis/vorbisfile.h>
#include <cassert>
#include "log.h"
#include "util/numeric.h" // myrand()
#include "porting.h"
#include <vector>
#include <fstream>
#include <unordered_map>
#include <unordered_set>

#define BUFFER_SIZE 30000

std::shared_ptr<SoundManagerSingleton> g_sound_manager_singleton;

typedef std::unique_ptr<ALCdevice, void (*)(ALCdevice *p)> unique_ptr_alcdevice;
typedef std::unique_ptr<ALCcontext, void(*)(ALCcontext *p)> unique_ptr_alccontext;

static void delete_alcdevice(ALCdevice *p)
{
	if (p)
		alcCloseDevice(p);
}

static void delete_alccontext(ALCcontext *p)
{
	if (p) {
		alcMakeContextCurrent(nullptr);
		alcDestroyContext(p);
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
	warningstream<<desc<<": "<<alErrorString(err)<<std::endl;
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

SoundBuffer *load_opened_ogg_file(OggVorbis_File *oggFile,
		const std::string &filename_for_logging)
{
	int endian = 0; // 0 for Little-Endian, 1 for Big-Endian
	int bitStream;
	long bytes;
	char array[BUFFER_SIZE]; // Local fixed size array
	vorbis_info *pInfo;

	SoundBuffer *snd = new SoundBuffer;

	// Get some information about the OGG file
	pInfo = ov_info(oggFile, -1);

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
		bytes = ov_read(oggFile, array, BUFFER_SIZE, endian, 2, 1, &bitStream);

		if(bytes < 0)
		{
			ov_clear(oggFile);
			infostream << "Audio: Error decoding "
				<< filename_for_logging << std::endl;
			delete snd;
			return nullptr;
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
		infostream << "Audio: OpenAL error: " << alErrorString(error)
				<< "preparing sound buffer" << std::endl;
	}

	//infostream << "Audio file "
	//	<< filename_for_logging << " loaded" << std::endl;

	// Clean up!
	ov_clear(oggFile);

	return snd;
}

SoundBuffer *load_ogg_from_file(const std::string &path)
{
	OggVorbis_File oggFile;

	// Try opening the given file.
	// This requires libvorbis >= 1.3.2, as
	// previous versions expect a non-const char *
	if (ov_fopen(path.c_str(), &oggFile) != 0) {
		infostream << "Audio: Error opening " << path
			<< " for decoding" << std::endl;
		return nullptr;
	}

	return load_opened_ogg_file(&oggFile, path);
}

struct BufferSource {
	const char *buf;
	size_t cur_offset;
	size_t len;
};

size_t buffer_sound_read_func(void *ptr, size_t size, size_t nmemb, void *datasource)
{
	BufferSource *s = (BufferSource *)datasource;
	size_t copied_size = MYMIN(s->len - s->cur_offset, size);
	memcpy(ptr, s->buf + s->cur_offset, copied_size);
	s->cur_offset += copied_size;
	return copied_size;
}

int buffer_sound_seek_func(void *datasource, ogg_int64_t offset, int whence)
{
	BufferSource *s = (BufferSource *)datasource;
	if (whence == SEEK_SET) {
		if (offset < 0 || (size_t)MYMAX(offset, 0) >= s->len) {
			// offset out of bounds
			return -1;
		}
		s->cur_offset = offset;
		return 0;
	} else if (whence == SEEK_CUR) {
		if ((size_t)MYMIN(-offset, 0) > s->cur_offset
				|| s->cur_offset + offset > s->len) {
			// offset out of bounds
			return -1;
		}
		s->cur_offset += offset;
		return 0;
	}
	// invalid whence param (SEEK_END doesn't have to be supported)
	return -1;
}

long BufferSourceell_func(void *datasource)
{
	BufferSource *s = (BufferSource *)datasource;
	return s->cur_offset;
}

static ov_callbacks g_buffer_ov_callbacks = {
	&buffer_sound_read_func,
	&buffer_sound_seek_func,
	nullptr,
	&BufferSourceell_func
};

SoundBuffer *load_ogg_from_buffer(const std::string &buf, const std::string &id_for_log)
{
	OggVorbis_File oggFile;

	BufferSource s;
	s.buf = buf.c_str();
	s.cur_offset = 0;
	s.len = buf.size();

	if (ov_open_callbacks(&s, &oggFile, nullptr, 0, g_buffer_ov_callbacks) != 0) {
		infostream << "Audio: Error opening " << id_for_log
			<< " for decoding" << std::endl;
		return nullptr;
	}

	return load_opened_ogg_file(&oggFile, id_for_log);
}

struct PlayingSound
{
	ALuint source_id;
	bool loop;
};

class SoundManagerSingleton
{
public:
	unique_ptr_alcdevice  m_device;
	unique_ptr_alccontext m_context;
public:
	SoundManagerSingleton() :
		m_device(nullptr, delete_alcdevice),
		m_context(nullptr, delete_alccontext)
	{
	}

	bool init()
	{
		if (!(m_device = unique_ptr_alcdevice(alcOpenDevice(nullptr), delete_alcdevice))) {
			errorstream << "Audio: Global Initialization: Failed to open device" << std::endl;
			return false;
		}

		if (!(m_context = unique_ptr_alccontext(
				alcCreateContext(m_device.get(), nullptr), delete_alccontext))) {
			errorstream << "Audio: Global Initialization: Failed to create context" << std::endl;
			return false;
		}

		if (!alcMakeContextCurrent(m_context.get())) {
			errorstream << "Audio: Global Initialization: Failed to make current context" << std::endl;
			return false;
		}

		alDistanceModel(AL_INVERSE_DISTANCE_CLAMPED);

		if (alGetError() != AL_NO_ERROR) {
			errorstream << "Audio: Global Initialization: OpenAL Error " << alGetError() << std::endl;
			return false;
		}

		infostream << "Audio: Global Initialized: OpenAL " << alGetString(AL_VERSION)
			<< ", using " << alcGetString(m_device.get(), ALC_DEVICE_SPECIFIER)
			<< std::endl;

		return true;
	}

	~SoundManagerSingleton()
	{
		infostream << "Audio: Global Deinitialized." << std::endl;
	}
};

class OpenALSoundManager: public ISoundManager
{
private:
	OnDemandSoundFetcher *m_fetcher;
	ALCdevice *m_device;
	ALCcontext *m_context;
	int m_next_id;
	std::unordered_map<std::string, std::vector<SoundBuffer*>> m_buffers;
	std::unordered_map<int, PlayingSound*> m_sounds_playing;
	struct FadeState {
		FadeState() = default;

		FadeState(float step, float current_gain, float target_gain):
			step(step),
			current_gain(current_gain),
			target_gain(target_gain) {}
		float step;
		float current_gain;
		float target_gain;
	};

	std::unordered_map<int, FadeState> m_sounds_fading;
public:
	OpenALSoundManager(SoundManagerSingleton *smg, OnDemandSoundFetcher *fetcher):
		m_fetcher(fetcher),
		m_device(smg->m_device.get()),
		m_context(smg->m_context.get()),
		m_next_id(1)
	{
		infostream << "Audio: Initialized: OpenAL " << std::endl;
	}

	~OpenALSoundManager()
	{
		infostream << "Audio: Deinitializing..." << std::endl;

		std::unordered_set<int> source_del_list;

		for (const auto &sp : m_sounds_playing)
			source_del_list.insert(sp.first);

		for (const auto &id : source_del_list)
			deleteSound(id);

		for (auto &buffer : m_buffers) {
			for (SoundBuffer *sb : buffer.second) {
				delete sb;
			}
			buffer.second.clear();
		}
		m_buffers.clear();

		infostream << "Audio: Deinitialized." << std::endl;
	}

	void step(float dtime)
	{
		doFades(dtime);
	}

	void addBuffer(const std::string &name, SoundBuffer *buf)
	{
		std::unordered_map<std::string, std::vector<SoundBuffer*>>::iterator i =
				m_buffers.find(name);
		if(i != m_buffers.end()){
			i->second.push_back(buf);
			return;
		}
		std::vector<SoundBuffer*> bufs;
		bufs.push_back(buf);
		m_buffers[name] = bufs;
	}

	SoundBuffer* getBuffer(const std::string &name)
	{
		std::unordered_map<std::string, std::vector<SoundBuffer*>>::iterator i =
				m_buffers.find(name);
		if(i == m_buffers.end())
			return nullptr;
		std::vector<SoundBuffer*> &bufs = i->second;
		int j = myrand() % bufs.size();
		return bufs[j];
	}

	PlayingSound* createPlayingSound(SoundBuffer *buf, bool loop,
			float volume, float pitch)
	{
		infostream << "OpenALSoundManager: Creating playing sound" << std::endl;
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
		volume = std::fmax(0.0f, volume);
		alSourcef(sound->source_id, AL_GAIN, volume);
		alSourcef(sound->source_id, AL_PITCH, pitch);
		alSourcePlay(sound->source_id);
		warn_if_error(alGetError(), "createPlayingSound");
		return sound;
	}

	PlayingSound* createPlayingSoundAt(SoundBuffer *buf, bool loop,
			float volume, v3f pos, float pitch)
	{
		infostream << "OpenALSoundManager: Creating positional playing sound"
				<< std::endl;
		assert(buf);
		PlayingSound *sound = new PlayingSound;
		assert(sound);
		warn_if_error(alGetError(), "before createPlayingSoundAt");
		alGenSources(1, &sound->source_id);
		alSourcei(sound->source_id, AL_BUFFER, buf->buffer_id);
		alSourcei(sound->source_id, AL_SOURCE_RELATIVE, false);
		alSource3f(sound->source_id, AL_POSITION, pos.X, pos.Y, pos.Z);
		alSource3f(sound->source_id, AL_VELOCITY, 0, 0, 0);
		// Use alDistanceModel(AL_INVERSE_DISTANCE_CLAMPED) and set reference
		// distance to clamp gain at <1 node distance, to avoid excessive
		// volume when closer
		alSourcef(sound->source_id, AL_REFERENCE_DISTANCE, 10.0f);
		alSourcei(sound->source_id, AL_LOOPING, loop ? AL_TRUE : AL_FALSE);
		// Multiply by 3 to compensate for reducing AL_REFERENCE_DISTANCE from
		// the previous value of 30 to the new value of 10
		volume = std::fmax(0.0f, volume * 3.0f);
		alSourcef(sound->source_id, AL_GAIN, volume);
		alSourcef(sound->source_id, AL_PITCH, pitch);
		alSourcePlay(sound->source_id);
		warn_if_error(alGetError(), "createPlayingSoundAt");
		return sound;
	}

	int playSoundRaw(SoundBuffer *buf, bool loop, float volume, float pitch)
	{
		assert(buf);
		PlayingSound *sound = createPlayingSound(buf, loop, volume, pitch);
		if(!sound)
			return -1;
		int id = m_next_id++;
		m_sounds_playing[id] = sound;
		return id;
	}

	int playSoundRawAt(SoundBuffer *buf, bool loop, float volume, const v3f &pos,
			float pitch)
	{
		assert(buf);
		PlayingSound *sound = createPlayingSoundAt(buf, loop, volume, pos, pitch);
		if(!sound)
			return -1;
		int id = m_next_id++;
		m_sounds_playing[id] = sound;
		return id;
	}

	void deleteSound(int id)
	{
		std::unordered_map<int, PlayingSound*>::iterator i = m_sounds_playing.find(id);
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
			return nullptr;
		std::set<std::string> paths;
		std::set<std::string> datas;
		m_fetcher->fetchSounds(name, paths, datas);
		for (const std::string &path : paths) {
			loadSoundFile(name, path);
		}
		for (const std::string &data : datas) {
			loadSoundData(name, data);
		}
		return getBuffer(name);
	}

	// Remove stopped sounds
	void maintain()
	{
		if (!m_sounds_playing.empty()) {
			verbosestream << "OpenALSoundManager::maintain(): "
					<< m_sounds_playing.size() <<" playing sounds, "
					<< m_buffers.size() <<" sound names loaded"<<std::endl;
		}
		std::unordered_set<int> del_list;
		for (const auto &sp : m_sounds_playing) {
			int id = sp.first;
			PlayingSound *sound = sp.second;
			// If not playing, remove it
			{
				ALint state;
				alGetSourcei(sound->source_id, AL_SOURCE_STATE, &state);
				if(state != AL_PLAYING){
					del_list.insert(id);
				}
			}
		}
		if(!del_list.empty())
			verbosestream<<"OpenALSoundManager::maintain(): deleting "
					<<del_list.size()<<" playing sounds"<<std::endl;
		for (int i : del_list) {
			deleteSound(i);
		}
	}

	/* Interface */

	bool loadSoundFile(const std::string &name,
			const std::string &filepath)
	{
		SoundBuffer *buf = load_ogg_from_file(filepath);
		if (buf)
			addBuffer(name, buf);
		return !!buf;
	}

	bool loadSoundData(const std::string &name,
			const std::string &filedata)
	{
		SoundBuffer *buf = load_ogg_from_buffer(filedata, name);
		if (buf)
			addBuffer(name, buf);
		return !!buf;
	}

	void updateListener(const v3f &pos, const v3f &vel, const v3f &at, const v3f &up)
	{
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

	int playSound(const std::string &name, bool loop, float volume, float fade, float pitch)
	{
		maintain();
		if (name.empty())
			return 0;
		SoundBuffer *buf = getFetchBuffer(name);
		if(!buf){
			infostream << "OpenALSoundManager: \"" << name << "\" not found."
					<< std::endl;
			return -1;
		}
		int handle = -1;
		if (fade > 0) {
			handle = playSoundRaw(buf, loop, 0.0f, pitch);
			fadeSound(handle, fade, volume);
		} else {
			handle = playSoundRaw(buf, loop, volume, pitch);
		}
		return handle;
	}

	int playSoundAt(const std::string &name, bool loop, float volume, v3f pos, float pitch)
	{
		maintain();
		if (name.empty())
			return 0;
		SoundBuffer *buf = getFetchBuffer(name);
		if(!buf){
			infostream << "OpenALSoundManager: \"" << name << "\" not found."
					<< std::endl;
			return -1;
		}
		return playSoundRawAt(buf, loop, volume, pos, pitch);
	}

	void stopSound(int sound)
	{
		maintain();
		deleteSound(sound);
	}

	void fadeSound(int soundid, float step, float gain)
	{
		// Ignore the command if step isn't valid.
		if (step == 0)
			return;
		float current_gain = getSoundGain(soundid);
		step = gain - current_gain > 0 ? abs(step) : -abs(step);
		if (m_sounds_fading.find(soundid) != m_sounds_fading.end()) {
			auto current_fade = m_sounds_fading[soundid];
			// Do not replace the fade if it's equivalent.
			if (current_fade.target_gain == gain && current_fade.step == step)
				return;
			m_sounds_fading.erase(soundid);
		}
		gain = rangelim(gain, 0, 1);
		m_sounds_fading[soundid] = FadeState(step, current_gain, gain);
	}

	void doFades(float dtime)
	{
		for (auto i = m_sounds_fading.begin(); i != m_sounds_fading.end();) {
			FadeState& fade = i->second;
			assert(fade.step != 0);
			fade.current_gain += (fade.step * dtime);

			if (fade.step < 0.f)
				fade.current_gain = std::max(fade.current_gain, fade.target_gain);
			else
				fade.current_gain = std::min(fade.current_gain, fade.target_gain);

			if (fade.current_gain <= 0.f)
				stopSound(i->first);
			else
				updateSoundGain(i->first, fade.current_gain);

			// The increment must happen during the erase call, or else it'll segfault.
			if (fade.current_gain == fade.target_gain)
				m_sounds_fading.erase(i++);
			else
				i++;
		}
	}

	bool soundExists(int sound)
	{
		maintain();
		return (m_sounds_playing.count(sound) != 0);
	}

	void updateSoundPosition(int id, v3f pos)
	{
		auto i = m_sounds_playing.find(id);
		if (i == m_sounds_playing.end())
			return;
		PlayingSound *sound = i->second;

		alSourcei(sound->source_id, AL_SOURCE_RELATIVE, false);
		alSource3f(sound->source_id, AL_POSITION, pos.X, pos.Y, pos.Z);
		alSource3f(sound->source_id, AL_VELOCITY, 0, 0, 0);
		alSourcef(sound->source_id, AL_REFERENCE_DISTANCE, 30.0);
	}

	bool updateSoundGain(int id, float gain)
	{
		auto i = m_sounds_playing.find(id);
		if (i == m_sounds_playing.end())
			return false;

		PlayingSound *sound = i->second;
		alSourcef(sound->source_id, AL_GAIN, gain);
		return true;
	}

	float getSoundGain(int id)
	{
		auto i = m_sounds_playing.find(id);
		if (i == m_sounds_playing.end())
			return 0;

		PlayingSound *sound = i->second;
		ALfloat gain;
		alGetSourcef(sound->source_id, AL_GAIN, &gain);
		return gain;
	}
};

std::shared_ptr<SoundManagerSingleton> createSoundManagerSingleton()
{
	auto smg = std::make_shared<SoundManagerSingleton>();
	if (!smg->init()) {
		smg.reset();
	}
	return smg;
}

ISoundManager *createOpenALSoundManager(SoundManagerSingleton *smg, OnDemandSoundFetcher *fetcher)
{
	return new OpenALSoundManager(smg, fetcher);
};
