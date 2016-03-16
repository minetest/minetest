/*
Minetest
Copyright (C) 2015 celeron55, Perttu Ahola <celeron55@gmail.com>

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

#ifndef WMS_PRIORITY_HEADER
#define WMS_PRIORITY_HEADER
#include "constants.h"
#include "networkprotocol.h"

struct WMSPriority
{
	v3s16 player_p;
	float far_weight;

	WMSPriority(v3s16 player_p, float far_weight):
		player_p(player_p),
		far_weight(far_weight)
	{}

	// Lower is more important
	float getPriority(const WantedMapSend &wms) {
		float priority;
		switch (wms.type) {
		case WMST_INVALID:
			assert("WMST_INVALID");
			priority = 1000000;
			break;
		case WMST_MAPBLOCK:
			{
				v3s16 center_p = wms.p * MAP_BLOCKSIZE +
						v3s16(1,1,1) * (MAP_BLOCKSIZE/2);
				priority = (player_p - center_p).getLength();
			}
			break;
		case WMST_FARBLOCK:
			{
				v3s16 center_p = wms.p * MAP_BLOCKSIZE * FMP_SCALE +
						v3s16(1,1,1) * (MAP_BLOCKSIZE*FMP_SCALE/2);
				priority = (player_p - center_p).getLength();

				// Lessen priority by a static amount so that close-by MapBlocks
				// are always transferred first
				priority += MAP_BLOCKSIZE * FMP_SCALE;

				if (far_weight <= 0.0f)
					priority /= 0.1f; // Just get some proportional large value
					                  // so that FarBlocks are in correct order
					                  // to each other
				else
					priority /= far_weight;
			}
			break;
		default:
			priority = 1000000;
			break;
		}
		return priority;
	}

	// Sorts WantedMapSends in descending order of priority
	bool operator() (const WantedMapSend& wms1, const WantedMapSend& wms2) {
		return (getPriority(wms1) < getPriority(wms2));
	}
};

#endif
