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

#include "irr_v3d.h"


/******************************************************************************/
/* Forward declarations                                                       */
/******************************************************************************/

class ServerEnvironment;

/******************************************************************************/
/* Typedefs and macros                                                        */
/******************************************************************************/

// #define PATHFINDER_DEBUG

enum PathDirections
{
	DIR_XP,
	DIR_XM,
	DIR_ZP,
	DIR_ZM
};

/** List of supported algorithms */
enum Algorithm
{
	DIJKSTRA,           /**< Dijkstra shortest path algorithm             */
	A_PLAIN,            /**< A* algorithm using heuristics to find a path */
	A_PLAIN_NP          /**< A* algorithm without prefetching of map data */
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

/** representation of cost in specific direction */
class PathCost
{
public:
	PathCost();

	PathCost(const PathCost& b);

	PathCost& operator= (const PathCost& b);

	bool valid;              /**< movement is possible         */
	int  value;              /**< cost of movement             */
	int  direction;          /**< y-direction of movement      */
	bool updated;            /**< this cost has ben calculated */

};


/** representation of a mapnode to be used for pathfinding */
class PathGridnode
{
public:
	PathGridnode();

	PathGridnode(const PathGridnode& b);

	/**
	 * assignment operator
	 * @param b node to copy
	 */
	PathGridnode& operator= (const PathGridnode& b);

	/**
	 * read cost in a specific direction
	 * @param dir direction of cost to fetch
	 */
	PathCost getCost(v3s16 dir);

	/**
	 * set cost value for movement
	 * @param dir direction to set cost for
	 * @cost cost to set
	 */
	void      setCost(v3s16 dir, PathCost cost);

	bool      valid;               /**< node is on surface                    */
	bool      target;              /**< node is target position               */
	bool      source;              /**< node is stating position              */
	int       totalcost;           /**< cost to move here from starting point */
	v3s16     sourcedir;           /**< origin of movement for current cost   */
	v3s16     pos;                 /**< real position of node                 */
	PathCost directions[4];       /**< cost in different directions          */

	/* debug values */
	bool      is_element;          /**< node is element of path detected      */
	char      type;                /**< type of node                          */
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

	/* helper functions */

	/**
	 * transform index pos to mappos
	 * @param ipos a index position
	 * @return map position
	 */
	v3s16 getRealPos(v3s16 ipos);

	/**
	 * transform mappos to index pos
	 * @param pos a real pos
	 * @return index position
	 */
	v3s16 getIndexPos(v3s16 pos);

	/**
	 * get gridnode at a specific index position
	 * @param ipos index position
	 * @return gridnode for index
	 */
	PathGridnode& getIndexElement(v3s16 ipos);

	/**
	 * invert a 3d position
	 * @param pos 3d position
	 * @return pos *-1
	 */
	v3s16 invert(v3s16 pos);

	/**
	 * check if a index is within current search area
	 * @param index position to validate
	 * @return true/false
	 */
	bool validIndex(v3s16 index);

	/**
	 * translate position to float position
	 * @param pos integer position
	 * @return float position
	 */
	v3f tov3f(v3s16 pos);


	/* algorithm functions */

	/**
	 * calculate 2d manahttan distance to target
	 * @param pos position to calc distance
	 * @return integer distance
	 */
	inline static int getManhattanDistance(v3s16 pos1, v3s16 pos2);

	/**
	 * get best direction based uppon heuristics
	 * @param directions list of unchecked directions
	 * @param g_pos mapnode to start from
	 * @return direction to check
	 */
	v3s16 getDirHeuristic(std::vector<v3s16>& directions, PathGridnode& g_pos);

	/**
	 * build internal data representation of search area
	 * @return true/false if costmap creation was successfull
	 */
	bool buildCostmap();

	/**
	 * calculate cost of movement
	 * @param pos real world position to start movement
	 * @param dir direction to move to
	 * @return cost information
	 */
	PathCost calcCost(v3s16 pos,v3s16 dir);

	/**
	 * This method is created to replace shit that used previously.
	 */

	bool updateCostHeuristic(v3s16 pos, std::vector <v3s16>& adjacencies);

	/**
	 * recursive update whole search areas total cost information
	 * @param ipos position to check next
	 * @param srcdir positionc checked last time
	 * @param total_cost cost of moving to ipos
	 * @param level current recursion depth
	 * @return true/false path to destination has been found
	 */
	bool updateAllCosts(v3s16 ipos, v3s16 srcdir, int total_cost, int level);

	/**
	 * recursive try to find a patrh to destionation
	 * @param ipos position to check next
	 * @param srcdir positionc checked last time
	 * @param total_cost cost of moving to ipos
	 * @param level current recursion depth
	 * @return true/false path to destination has been found
	 */
	bool updateCostHeuristic(v3s16 ipos, v3s16 srcdir, int current_cost, int level);

	/**
	 * recursive build a vector containing all nodes from source to destination
	 * @param path vector to add nodes to
	 * @param pos pos to check next
	 * @param level recursion depth
	 */
	void buildPath(std::vector<v3s16>& path, v3s16 pos, int level);

	/* variables */
	int m_max_index_x;          /**< max index of search area in x direction  */
	int m_max_index_y;          /**< max index of search area in y direction  */
	int m_max_index_z;          /**< max index of search area in z direction  */


	int m_searchdistance;       /**< max distance to search in each direction */
	int m_maxdrop;              /**< maximum number of blocks a path may drop */
	int m_maxjump;              /**< maximum number of blocks a path may jump */
	int m_min_target_distance;  /**< current smalest path to target           */

	bool m_prefetch;            /**< prefetch cost data                       */

	v3s16 m_start;              /**< source position                          */
	v3s16 m_destination;        /**< destination position                     */

	limits m_limits;            /**< position limits in real map coordinates  */

	/** 3d grid containing all map data already collected and analyzed */
	std::vector<std::vector<std::vector<PathGridnode> > > m_data;

	ServerEnvironment* m_env;   /**< minetest environment pointer             */

	std::vector <v3s16> m_adjacency_4;
	std::vector <v3s16> m_adjacency_8;

#ifdef PATHFINDER_DEBUG

	/**
	 * print collected cost information
	 */
	void printCost();

	/**
	 * print collected cost information in a specific direction
	 * @param dir direction to print
	 */
	void printCost(PathDirections dir);

	/**
	 * print type of node as evaluated
	 */
	void printType() const;

	/**
	 * print pathlenght for all nodes in search area
	 */
	void printPathlen() const;

	/**
	 * print a path
	 * @param path path to show
	 */
	void printPath(std::vector<v3s16> path) const;

	/**
	 * print y direction for all movements
	 */
	void printYDir() const;

	/**
	 * print y direction for moving in a specific direction
	 * @param dir direction to show data
	 */
	void printYDir(PathDirections dir) const;

	/**
	 * helper function to translate a direction to speaking text
	 * @param dir direction to translate
	 * @return textual name of direction
	 */
	std::string dirToName(PathDirections dir) const;
#endif
};

inline int PathFinder::getManhattanDistance(v3s16 pos1, v3s16 pos2)
{
	return fabs(pos1.X - pos2.X) + fabs(pos1.Z - pos2.Z);
}

#endif /* PATHFINDER_H_ */
