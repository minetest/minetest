/*
Minetest
Copyright (C) 2013 celeron55, Perttu Ahola <celeron55@gmail.com>

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

#include <set>
#include <string>
#include "irr_v3d.h"
#include "../sound.h"

class OnDemandSoundFetcher
{
public:
	virtual void fetchSounds(const std::string &name,
			std::set<std::string> &dst_paths,
			std::set<std::string> &dst_datas) = 0;
};

class ISoundManager
{
public:
	virtual ~ISoundManager() = default;

	// Multiple sounds can be loaded per name; when played, the sound
	// should be chosen randomly from alternatives
	// Return value determines success/failure
	virtual bool loadSoundFile(
			const std::string &name, const std::string &filepath) = 0;
	virtual bool loadSoundData(
			const std::string &name, const std::string &filedata) = 0;

	virtual void updateListener(
			const v3f &pos, const v3f &vel, const v3f &at, const v3f &up) = 0;
	virtual void setListenerGain(float gain) = 0;

	// playSound functions return -1 on failure, otherwise a handle to the
	// sound. If name=="", call should be ignored without error.
	virtual int playSound(const std::string &name, bool loop, float volume,
			float fade = 0.0f, float pitch = 1.0f) = 0;
	virtual int playSoundAt(const std::string &name, bool loop, float volume, v3f pos,
			float pitch = 1.0f) = 0;
	virtual void stopSound(int sound) = 0;
	virtual bool soundExists(int sound) = 0;
	virtual void updateSoundPosition(int sound, v3f pos) = 0;
	virtual bool updateSoundGain(int id, float gain) = 0;
	virtual float getSoundGain(int id) = 0;
	virtual void step(float dtime) = 0;
	virtual void fadeSound(int sound, float step, float gain) = 0;

	int playSound(const SimpleSoundSpec &spec, bool loop)
	{
		return playSound(spec.name, loop, spec.gain, spec.fade, spec.pitch);
	}
	int playSoundAt(const SimpleSoundSpec &spec, bool loop, const v3f &pos)
	{
		return playSoundAt(spec.name, loop, spec.gain, pos, spec.pitch);
	}
};

class DummySoundManager : public ISoundManager
{
public:
	virtual bool loadSoundFile(const std::string &name, const std::string &filepath)
	{
		return true;
	}
	virtual bool loadSoundData(const std::string &name, const std::string &filedata)
	{
		return true;
	}
	void updateListener(const v3f &pos, const v3f &vel, const v3f &at, const v3f &up)
	{
	}
	void setListenerGain(float gain) {}
	int playSound(const std::string &name, bool loop, float volume, float fade,
			float pitch)
	{
		return 0;
	}
	int playSoundAt(const std::string &name, bool loop, float volume, v3f pos,
			float pitch)
	{
		return 0;
	}
	void stopSound(int sound) {}
	bool soundExists(int sound) { return false; }
	void updateSoundPosition(int sound, v3f pos) {}
	bool updateSoundGain(int id, float gain) { return false; }
	float getSoundGain(int id) { return 0; }
	void step(float dtime) {}
	void fadeSound(int sound, float step, float gain) {}
};

// Global DummySoundManager singleton
extern DummySoundManager dummySoundManager;
