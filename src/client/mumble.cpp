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

#include "client/mumble.h"
#include "util/string.h"
#include "util/numeric.h"
#include "porting.h"
#include "log.h"
#include <cassert>
#include <cfloat>
#include <cerrno>

#ifdef WIN32
	#include <windows.h>
#else
	#include <unistd.h>
	#include <sys/mman.h>
	#include <sys/stat.h>
	#include <fcntl.h>
#endif


MumbleManager::~MumbleManager()
{
#ifdef WIN32
	UnmapViewOfFile(lm);
	if (obj != NULL)
		CloseHandle(obj);
#else
	munmap(lm, sizeof(*lm));
	if (fd > 0)
		close(fd);
#endif
}


bool MumbleManager::start(const std::string &context,
		const std::string &player_name)
{
	assert(lm == NULL);  // Pre-condition
#ifdef WIN32
	obj = OpenFileMappingW(FILE_MAP_ALL_ACCESS, FALSE, L"MumbleLink");
	if (obj == NULL) {
		errorstream << "Failed to open shared memory for Mumble: "
			<< porting::strwinerr() << std::endl;
		return false;
	}

	lm = static_cast<MumbleLinkedMemory *>(MapViewOfFile(obj,
			FILE_MAP_ALL_ACCESS, 0, 0, sizeof(*lm)));
	if (lm == NULL) {
		errorstream << "Failed to map shared memory for Mumble: "
			<< porting::strwinerr() << std::endl;
		CloseHandle(obj);
		obj = NULL;
		return false;
	}
#else
	std::string name = "/MumbleLink." + to_string(getuid());

	fd = shm_open(name.c_str(), O_RDWR, S_IRUSR | S_IWUSR);

	if (fd < 0) {
		errorstream << "Failed to open shared memory for Mumble: "
			<< porting::strerrno(errno) << std::endl;
		return false;
	}

	lm = static_cast<MumbleLinkedMemory *>(mmap(NULL, sizeof(*lm),
			PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0));
	if (lm == MAP_FAILED) {
		errorstream << "Failed to map shared memory for Mumble: "
			<< porting::strerrno(errno) << std::endl;
		lm = NULL;
		return false;
	}
#endif

	// Context is equal for players on the same server/world.
	// TODO: allow mods to append something to allow for teams.
	size_t len = MYMIN(ARRLEN(lm->context), context.size());
	memcpy(lm->context, context.c_str(), len);
	lm->context_len = len;

	std::wstring wpn = utf8_to_wide(player_name);
	wcsncpy(lm->identity, wpn.c_str(), ARRLEN(lm->identity));

	return true;
}


inline static void set_mumble_v3f(float mumble_vec[3], const v3f &vec)
{
	mumble_vec[0] = vec.X;
	mumble_vec[1] = vec.Y;
	mumble_vec[2] = vec.Z;
}


void MumbleManager::step(v3f player_pos, v3f player_dir, v3f cam_pos, v3f cam_dir)
{
	if (lm == NULL)
		return;

	if (lm->version != 2) {
		wcsncpy(lm->name, PROJECT_NAME_CW, ARRLEN(lm->name));
		wcsncpy(lm->description, L"Link to the " PROJECT_NAME_CW " game.", ARRLEN(lm->description));
		lm->version = 2;
	}
	lm->tick++;

	cam_dir.normalize();
	player_dir.normalize();

	// Mumble interprets (0, 0, 0) as a special value meaning
	// "use non-positional audio," so make sure we don't use it.
	if (player_pos == v3f(0, 0, 0))
		player_pos = v3f(FLT_MIN, FLT_MIN, FLT_MIN);

	// Left handed coordinate system.
	// Units are in meters.

	// Position of the avatar.
	// This is sent to other players for them to pan our audio.
	set_mumble_v3f(lm->avatar_position, player_pos);

	// Unit vector pointing out of the avatar's eyes.
	set_mumble_v3f(lm->avatar_front, player_dir);

	// Unit vector pointing out of the top of the avatar's head.
	// This is just the front vector rotated PI/2 radians up.
	v3f player_top(player_dir);
	player_top.rotateXYBy(90);
	set_mumble_v3f(lm->avatar_top, player_top);

	// Position of the camera.
	// This is used for panning remote audio.
	set_mumble_v3f(lm->camera_position, cam_pos);

	set_mumble_v3f(lm->camera_front, cam_dir);

	v3f cam_top(cam_dir);
	cam_top.rotateXYBy(90);
	set_mumble_v3f(lm->camera_top, cam_top);
}

