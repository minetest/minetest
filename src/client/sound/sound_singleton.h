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

#pragma once

#include "al_helpers.h"

namespace sound {

/**
 * Class for the openal device and context
 */
class SoundManagerSingleton
{
public:
	struct AlcDeviceDeleter {
		void operator()(ALCdevice *p)
		{
			alcCloseDevice(p);
		}
	};

	struct AlcContextDeleter {
		void operator()(ALCcontext *p)
		{
			alcMakeContextCurrent(nullptr);
			alcDestroyContext(p);
		}
	};

	using unique_ptr_alcdevice = std::unique_ptr<ALCdevice, AlcDeviceDeleter>;
	using unique_ptr_alccontext = std::unique_ptr<ALCcontext, AlcContextDeleter>;

	unique_ptr_alcdevice  m_device;
	unique_ptr_alccontext m_context;

public:
	bool init();

	~SoundManagerSingleton();
};

} // namespace sound
