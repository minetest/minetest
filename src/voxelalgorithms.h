/*
Minetest
Copyright (C) 2010-2013 celeron55, Perttu Ahola <celeron55@gmail.com>

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

#ifndef VOXELALGORITHMS_HEADER
#define VOXELALGORITHMS_HEADER

#include "voxel.h"
#include "mapnode.h"
#include <set>
#include <map>

namespace voxalgo
{

// TODO: Move unspreadLight and spreadLight from VoxelManipulator to here

void setLight(VoxelManipulator &v, VoxelArea a, u8 light,
		INodeDefManager *ndef);

void clearLightAndCollectSources(VoxelManipulator &v, VoxelArea a,
		enum LightBank bank, INodeDefManager *ndef,
		std::set<v3s16> & light_sources,
		std::map<v3s16, u8> & unlight_from);

struct SunlightPropagateResult
{
	bool bottom_sunlight_valid;

	SunlightPropagateResult(bool bottom_sunlight_valid_):
		bottom_sunlight_valid(bottom_sunlight_valid_)
	{}
};

SunlightPropagateResult propagateSunlight(VoxelManipulator &v, VoxelArea a,
		bool inexistent_top_provides_sunlight,
		std::set<v3s16> & light_sources,
		INodeDefManager *ndef);

} // namespace voxalgo

#endif

