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

#ifndef S_ENV_H_
#define S_ENV_H_

#include "cpp_api/s_base.h"
#include "irr_v3d.h"

class ServerEnvironment;
struct MapgenParams;

class ScriptApiEnv
		: virtual public ScriptApiBase
{
public:
	// On environment step
	void environment_Step(float dtime);
	// After generating a piece of map
	void environment_OnGenerated(v3s16 minp, v3s16 maxp,u32 blockseed);
	// After initializing mapgens
	void environment_OnMapgenInit(MapgenParams *mgparams);

	//called on player event
	void player_event(ServerActiveObject* player, std::string type);

	void initializeEnvironment(ServerEnvironment *env);
};

#endif /* S_ENV_H_ */
