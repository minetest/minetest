/*
Minetest
Copyright (C) 2015 Nerzhul, Loic Blot <loic.blot@unix-experience.fr>

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

#include "irr_v3d.h"

#include <map>
#include <vector>
#include <unordered_map>
#include <mutex>

/*
 * This class permits caching getFacePosition call results.
 * This reduces CPU usage and vector calls.
 */
class FacePositionCache {
public:
	static const std::vector<v3s16> &getFacePositions(u16 d);

private:
	static const std::vector<v3s16> &generateFacePosition(u16 d);
	static std::unordered_map<u16, std::vector<v3s16>> cache;
	static std::mutex cache_mutex;
};
