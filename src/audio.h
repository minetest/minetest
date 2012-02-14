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

/*
 * USAGE:
 * 
 * First you have to declare a new Audio object for the whole game
 * and attach the player's position with Audio::setListenerPos(v3f).
 * (Don't forget to update the position whe he moves.)
 * You can also change the volume with Audio::setVolume(float) with a
 * floating variable which has a value between 0.0 and 1.0.
 * 
 * If you want to set up a sound for something, just create a new
 * SoundSource as this: 
 * SoundSource * my_source = Audio::createSource(filename) 
 * and destroy this source with Audio::destroySource(my_source).
 * (Don't forget to update the position if it's assigned to something
 * moving.)
 * You can also change its volume and pitch.
 * 
 * If you only need to play a simple sound, just use
 * Audio::play(filename);
 * 
 * 
 * Read the commentaries for more informations.
 */

#ifndef AUDIO_HEADER
#define AUDIO_HEADER

// Audio is only relevant for client.
#ifndef SERVER

// Gotta love CONSISTENCY!
#if defined(_MSC_VER)
#include <al.h>
#include <alc.h>
#include <alext.h>
#include <alut.h>
#elif defined(__APPLE__)
#include <OpenAL/al.h>
#include <OpenAL/alc.h>
#include <OpenAL/alext.h>
#include <OpenAl/alut.h>
#else
#include <AL/al.h>
#include <AL/alc.h>
#include <AL/alext.h>
#include <AL/alut.h>
#endif

#include <map>
#include <vorbis/vorbisfile.h>
#include <jthread.h>
#include "common_irrlicht.h"
#include "debug.h"
#include "filesys.h"

#define BUFFER_SIZE 32768 // 32 KB buffers

// SoundBuffer
// Stock a sound buffer in the memory.
// Use this buffer with SoundSource.
class SoundBuffer
{
public:
	//SoundBuffer();
	SoundBuffer(const std::string filename);
	~SoundBuffer();

	std::vector<char> getBuffer() { return m_buffer; };
	ALsizei getFreq() { return m_freq; };
	ALenum getFormat() { return m_format; };

	void load(const std::string filename);
	//bool isLoaded() { return m_buffer !== NULL; };

private:
	std::string m_filename;
	std::vector<char> m_buffer;

	// OpenAL/OGG stuff...
	ALenum m_format;		// The sound data format
	ALsizei m_freq;		// The frequency of the sound data
};

// SoundSource
// Play a sound buffer (see SoundBuffer).
class SoundSource
{
public:
	enum Type
	{
		AMBIANT, 	// Sourcepos variable, 	loop
		// TODO: Check if when MUSIC used, Sourcepos has to be defined by player's position.
		MUSIC, 		// Sourcepos to zero, 	no loop
		SOUND, 		// Sourcepos variable, 	no loop
	};

	//SoundSource();
	SoundSource(SoundBuffer * soundbuffer, Type type);
	~SoundSource();

	void pause();
	void play();
	void stop();

	float getGain();
	void setGain(float gain);

	float getPitch();
	void setPitch(float multiplier);

	v3f getSourcePos(v3f position);
	void setSourcePos(v3f position);
	
	ALint getState();
	
	Type getType() { return m_type; };
	void setType(Type type);

private:
	SoundBuffer * m_soundbuffer;
	Type m_type;
	
	// OpenAL/OGG stuff...
	ALint m_state;		// The state of the sound source.
	ALuint m_buffer;	// The OpenAL sound buffer ID.
	ALuint m_source;	// The OpenAL sound source ID.
};

// AudioException
// We have to handle errors.
class AudioException
{

};

// AudioThread
// Usage:
//	AudioThread * at = new AudioThread(function);
//	at->Start(); // Start the thread.
//	at->Kill(); // Kill the thread.
//	at->IsRunning(); // Return the state of the thread.
/*class AudioThread : public JThread
{
	void (*m_fct)(void);
public:
	AudioThread(void (*fct)())
	{
		m_fct = fct; 
	}
	void * Thread()
	{
		ThreadStarted();
		m_fct();
		return NULL;
	}
};*/

// PlaybackThread
// To use with Audio::play so the while loop can be will not
// put the game in freeze state and the whole sound could be played.
class PlaybackThread : public JThread
{
	SoundSource * source;
public:
	PlaybackThread(SoundSource * s)
	{
		source = s;
	}
	void * Thread()
	{
		ThreadStarted();
		ALint state;
		source->play();
		//alSourcePlay(source);
		do {
			// Query the state of the souce
			state = source->getState();
		} while (state != AL_STOPPED);
		delete source;
		return NULL;
	}
};

// Audio
// Main class
// Work as a collection of sounds defined as SoundBuffer.
// Each sound is mapped and must be registered by a name.
class Audio
{
public:
	Audio() { init(); };
	~Audio() { shutdown(); };
	
	void init();
	void shutdown();

	void registerSound(std::string filename);
	SoundBuffer * getSound(std::string filename);

	SoundSource * createSource(std::string filename, SoundSource::Type type);
	void destroySource(SoundSource * ss);
	
	void play(std::string filename); // Simple playback function.

	void setListenerPos(v3f position); // The player is defined as the listener.
	void setVolume(float volume); // Define the volume on the game. 0 to mute.
	
	
private:
	std::map<std::string,SoundBuffer*> m_buffer;
	v3f m_position;
	ALCdevice * m_device;
	ALCcontext * m_context;
};

#endif // SERVER
#endif // AUDIO_HEADER
