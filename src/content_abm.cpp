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

#include "content_abm.h"

#include "environment.h"
#include "gamedef.h"
#include "nodedef.h"
#include "content_sao.h"
#include "settings.h"
#include "mapblock.h" // For getNodeBlockPos
#include "main.h" // for g_settings
#include "map.h"
#include "scripting_game.h"
#include "log.h"

#define PP(x) "("<<(x).X<<","<<(x).Y<<","<<(x).Z<<")"

void add_legacy_abms(ServerEnvironment *env, INodeDefManager *nodedef) {

}
