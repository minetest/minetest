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
with this program; if not, write to the Free Software Foundation, Inc.,
51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
*/

#include "sound_openal.h"

#include "sound_singleton.h"
#include "proxy_sound_manager.h"

std::shared_ptr<SoundManagerSingleton> g_sound_manager_singleton;

std::shared_ptr<SoundManagerSingleton> createSoundManagerSingleton()
{
	auto smg = std::make_shared<SoundManagerSingleton>();
	if (!smg->init()) {
		smg.reset();
	}
	return smg;
}

std::unique_ptr<ISoundManager> createOpenALSoundManager(SoundManagerSingleton *smg,
		std::unique_ptr<SoundFallbackPathProvider> fallback_path_provider)
{
	return std::make_unique<sound::ProxySoundManager>(smg, std::move(fallback_path_provider));
};
