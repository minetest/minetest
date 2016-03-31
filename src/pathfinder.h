/*
Minetest
Copyright (C) 2013 sapier, sapier at gmx dot net

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

#ifndef PATHFINDER_H_
#define PATHFINDER_H_

/******************************************************************************/
/* Includes                                                                   */
/******************************************************************************/
#include <vector>
#include "irr_v3d.h"

/******************************************************************************/
/* Forward declarations                                                       */
/******************************************************************************/

class ServerEnvironment;

/******************************************************************************/
/* Typedefs and macros                                                        */
/******************************************************************************/

typedef enum {
	DIR_XP,
	DIR_XM,
	DIR_ZP,
	DIR_ZM
} path_directions;

/** List of supported algorithms */
typedef enum {
	DIJKSTRA,           /**< Dijkstra shortest path algorithm             */
	A_PLAIN,            /**< A* algorithm using heuristics to find a path */
	A_PLAIN_NP          /**< A* algorithm without prefetching of map data */
} algorithm;

/******************************************************************************/
/* declarations                                                               */
/******************************************************************************/

/** c wrapper function to use from scriptapi */
std::vector<v3s16> get_Path(ServerEnvironment* env,
							v3s16 source,
							v3s16 destination,
							unsigned int searchdistance,
							unsigned int max_jump,
							unsigned int max_drop,
							algorithm algo);

#endif /* PATHFINDER_H_ */
