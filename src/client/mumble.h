/*
Minetest
Copyright (C) 2016 ShadowNinja <shadowninja@minetest.net>

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

#ifndef CLIENT_MUMBLE_HEADER
#define CLIENT_MUMBLE_HEADER

#include "config.h"
#ifdef USE_MUMBLE

#include "irrlichttypes_bloated.h"
#include <string>
#include <stdint.h>

#ifdef _WIN32
	#include <windef.h>
#endif

struct MumbleLinkedMemory {
	uint32_t version;
	uint32_t tick;
	float	avatar_position[3];
	float	avatar_front[3];
	float	avatar_top[3];
	wchar_t	name[256];
	float	camera_position[3];
	float	camera_front[3];
	float	camera_top[3];
	wchar_t	identity[256];
	uint32_t context_len;
	unsigned char context[256];
	wchar_t description[2048];
};


class MumbleManager {
public:
	MumbleManager() : lm(NULL) {}
	~MumbleManager();

	/// Create the mumble link and set some initial values.
	/// Context is server specific bytes, identity is player name in UTF-8.
	bool start(const std::string &context, const std::string &identity);

	/// Update Mumble's position and orientation values.
	void step(v3f player_pos, v3f player_dir, v3f cam_pos, v3f cam_dir);

private:
	MumbleLinkedMemory *lm;
#ifdef WIN32
	HANDLE obj;
#else
	int fd;
#endif
};

#endif  // USE_MUMBLE
#endif  // CLIENT_MUMBLE_HEADER

