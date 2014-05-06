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
#include <string>
#include <map>
#include <cstdlib>

#include "irr_v3d.h"


/******************************************************************************/
/* Forward declarations                                                       */
/******************************************************************************/

class ServerEnvironment;

/******************************************************************************/
/* Typedefs and macros                                                        */
/******************************************************************************/

/** List of supported algorithms */
enum Algorithm
{
	A_STAR
};

enum Adjacency
{
	ADJACENCY_4,
	ADJACENCY_8
};

/******************************************************************************/
/* declarations                                                               */
/******************************************************************************/

/** c wrapper function to use from scriptapi */
std::vector<v3s16> getPath(ServerEnvironment* env,
                           v3s16 source,
                           v3s16 destination,
                           unsigned int searchdistance,
                           unsigned int max_jump,
                           unsigned int max_drop,
                           Algorithm algo,
                           Adjacency adjacency);

struct OpenElement
{
	OpenElement();
	OpenElement(unsigned int _f_value, unsigned int _distance, v3s16 _pos, v3s16 _prev_pos);
	OpenElement& operator=(const OpenElement& e);
	bool operator<(const OpenElement& e) const;

	unsigned int f_value;
	unsigned int start_cost;
	v3s16 pos;
	v3s16 prev_pos;
};

/** class doing pathfinding */
class PathFinder
{
public:
	PathFinder();

	/**
	 * path evaluation function
	 * @param env environment to look for path
	 * @param source origin of path
	 * @param destination end position of path
	 * @param searchdistance maximum number of nodes to look in each direction
	 * @param max_jump maximum number of blocks a path may jump up
	 * @param max_drop maximum number of blocks a path may drop
	 * @param algo algorithm to use for finding a path
	 */
	std::vector<v3s16> getPath(ServerEnvironment* env,
	                           v3s16 source,
	                           v3s16 destination,
	                           unsigned int searchdistance,
	                           unsigned int max_jump,
	                           unsigned int max_drop,
	                           Algorithm algo,
	                           Adjacency adjacency);

private:
	struct limits {
		struct limit {
			int min;
			int max;
		};

		limit X;
		limit Y;
		limit Z;
	};

	unsigned int getDirectionCost(unsigned int id);

	/* algorithm functions */

	/**
	 * calculate 2d manahttan distance to target
	 * @param pos position to calc distance
	 * @return integer distance
	 */
	inline static unsigned int getManhattanDistance(v3s16 pos1, v3s16 pos2);

	/**
	 * This method finds closest path to the target
	 */

	bool findPathHeuristic(v3s16 pos, std::vector <v3s16>& adjacencies,
	                       unsigned int (*heuristicFunction)(v3s16, v3s16));

	/**
	 * Create a vector containing all nodes from source to destination
	 * @param path vector to add nodes to
	 * @param pos pos to check next
	 * @param level recursion depth
	 */
	void buildPath(std::vector<v3s16>& path, v3s16 start_pos, v3s16 end_pos);

	int m_searchdistance;       /**< max distance to search in each direction */
	int m_maxdrop;              /**< maximum number of blocks a path may drop */
	int m_maxjump;              /**< maximum number of blocks a path may jump */

	v3s16 m_start;              /**< source position                          */
	v3s16 m_destination;        /**< destination position                     */

	limits m_limits;            /**< position limits in real map coordinates  */

	ServerEnvironment* m_env;   /**< minetest environment pointer             */

	Adjacency m_adjacency;

	std::vector <v3s16> m_adjacency_4;
	std::vector <v3s16> m_adjacency_8;

	std::vector <unsigned int> m_adjacency_4_cost;
	std::vector <unsigned int> m_adjacency_8_cost;

	std::map <v3s16, std::pair <v3s16, unsigned int> > used;
};

inline unsigned int PathFinder::getManhattanDistance(v3s16 pos1, v3s16 pos2)
{
	return fabs(pos1.X - pos2.X) + fabs(pos1.Z - pos2.Z);
}

#endif /* PATHFINDER_H_ */
