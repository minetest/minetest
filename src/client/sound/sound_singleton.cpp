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

#include "sound_singleton.h"
#include "settings.h"
#include <vector>

namespace sound {

// Build openal context initialization attribute vector from settings
static std::vector<ALCint> init_attrs();

bool SoundManagerSingleton::init()
{
	if (!(m_device = unique_ptr_alcdevice(alcOpenDevice(nullptr)))) {
		errorstream << "Audio: Global Initialization: Failed to open device" << std::endl;
		return false;
	}

	auto sound_attrs = init_attrs();

	// If there are no attributes, then preserve the original behavior
	// where it passes nullptr to alcCreateContext for the attributes
	ALCint const *attr_ptr = sound_attrs.empty() ? nullptr : sound_attrs.data();

	if (!(m_context = unique_ptr_alccontext(alcCreateContext(m_device.get(), attr_ptr)))) {
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

static std::vector<ALCint> init_attrs()
{
	std::vector<ALCint> attrs;

	s32 sound_sources_mono = -1;
	if (!g_settings->getS32NoEx("sound_sources_mono", sound_sources_mono))
		sound_sources_mono = -1;

	s32 sound_sources_stereo = -1;
	if (!g_settings->getS32NoEx("sound_sources_stereo", sound_sources_stereo))
		sound_sources_stereo = -1;

	if (sound_sources_mono >= 0) {
		infostream << "Using " << sound_sources_mono << 
			" mono (3D) sound sources" << std::endl;
		attrs.push_back(ALC_MONO_SOURCES);
		attrs.push_back((ALCint)sound_sources_mono);
	}

	if (sound_sources_stereo >= 0) {
		infostream << "Using " << sound_sources_stereo << 
			" stereo sound sources" << std::endl;
		attrs.push_back(ALC_STEREO_SOURCES);
		attrs.push_back((ALCint)sound_sources_stereo);
	}

	// Append a null terminator if any attributes got added
	if (!attrs.empty())
		attrs.push_back(0);

	return attrs;
}

} // namespace sound
