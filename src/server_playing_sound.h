// Luanti
// SPDX-License-Identifier: LGPL-2.1-or-later

#pragma once

#include "irr_v3d.h"
#include "serverenvironment.h"
#include "server/clientiface.h"
#include "sound.h"
#include <string>
#include <unordered_set>

// Combines the pure sound (SoundSpec) with positional information
struct ServerPlayingSound
{
	SoundLocation type = SoundLocation::Local;

	float gain = 1.0f; // for amplification of the base sound
	float max_hear_distance = 32 * BS;
	v3f pos;
	u16 object = 0;
	std::string to_player;
	std::string exclude_player;

	v3f getPos(ServerEnvironment *env, bool *pos_exists) const;

	SoundSpec spec;

	std::unordered_set<session_t> clients; // peer ids
};
