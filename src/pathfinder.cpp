/*
Minetest
Copyright (C) 2013 sapier, sapier at gmx dot net
Copyright (C) 2016 est31, <MTest31@outlook.com>

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

/******************************************************************************/
/* Includes                                                                   */
/******************************************************************************/

#include "pathfinder.h"
#include "serverenvironment.h"
#include "server.h"
#include "nodedef.h"

//#define PATHFINDER_DEBUG
//#define PATHFINDER_CALC_TIME

#ifdef PATHFINDER_DEBUG
	#include <string>
#endif
#ifdef PATHFINDER_DEBUG
	#include <iomanip>
#endif
#ifdef PATHFINDER_CALC_TIME
	#include <sys/time.h>
#endif

/******************************************************************************/
/* Typedefs and macros                                                        */
/******************************************************************************/

#define LVL "(" << level << ")" <<

#ifdef PATHFINDER_DEBUG
#define DEBUG_OUT(a)     std::cout << a
#define INFO_TARGET      std::cout
#define VERBOSE_TARGET   std::cout
#define ERROR_TARGET     std::cout
#else
#define DEBUG_OUT(a)     while(0)
#define INFO_TARGET      infostream << "Pathfinder: "
#define VERBOSE_TARGET   verbosestream << "Pathfinder: "
#define ERROR_TARGET     warningstream << "Pathfinder: "
#endif

/******************************************************************************/
/* Class definitions                                                          */
/******************************************************************************/


/** representation of cost in specific direction */
class PathCost {
public:

	/** default constructor */
	PathCost() = default;

	/** copy constructor */
	PathCost(const PathCost &b);

	/** assignment operator */
	PathCost &operator= (const PathCost &b);

	bool valid = false;              /**< movement is possible         */
	int  value = 0;                  /**< cost of movement             */
	int  direction = 0;              /**< y-direction of movement      */
	bool updated = false;            /**< this cost has ben calculated */

};


/** representation of a mapnode to be used for pathfinding */
class PathGridnode {

public:
	/** default constructor */
	PathGridnode() = default;

	/** copy constructor */
	PathGridnode(const PathGridnode &b);

	/**
	 * assignment operator
	 * @param b node to copy
	 */
	PathGridnode &operator= (const PathGridnode &b);

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
	void      setCost(v3s16 dir, const PathCost &cost);

	bool      valid = false;               /**< node is on surface                    */
	bool      target = false;              /**< node is target position               */
	bool      source = false;              /**< node is stating position              */
	int       totalcost = -1;              /**< cost to move here from starting point */
	v3s16     sourcedir;                   /**< origin of movement for current cost   */
	v3s16     pos;                         /**< real position of node                 */
	PathCost directions[4];                /**< cost in different directions          */

	/* debug values */
	bool      is_element = false;          /**< node is element of path detected      */
	char      type = 'u';                  /**< type of node                          */
};

class Pathfinder;

/** Abstract class to manage the map data */
class GridNodeContainer {
public:
	virtual PathGridnode &access(v3s16 p)=0;
	virtual ~GridNodeContainer() = default;

protected:
	Pathfinder *m_pathf;

	void initNode(v3s16 ipos, PathGridnode *p_node);
};

class ArrayGridNodeContainer : public GridNodeContainer {
public:
	virtual ~ArrayGridNodeContainer() = default;

	ArrayGridNodeContainer(Pathfinder *pathf, v3s16 dimensions);
	virtual PathGridnode &access(v3s16 p);
private:
	v3s16 m_dimensions;

	int m_x_stride;
	int m_y_stride;
	std::vector<PathGridnode> m_nodes_array;
};

class MapGridNodeContainer : public GridNodeContainer {
public:
	virtual ~MapGridNodeContainer() = default;

	MapGridNodeContainer(Pathfinder *pathf);
	virtual PathGridnode &access(v3s16 p);
private:
	std::map<v3s16, PathGridnode> m_nodes;
};

/** class doing pathfinding */
class Pathfinder {

public:
	/**
	 * default constructor
	 */
	Pathfinder() = default;

	~Pathfinder();

	/**
	 * path evaluation function
	 * @param env environment to look for path
	 * @param source origin of path
	 * @param destination end position of path
	 * @param searchdistance maximum number of nodes to look in each direction
	 * @param max_jump maximum number of blocks a path may jump up
	 * @param max_drop maximum number of blocks a path may drop
	 * @param algo Algorithm to use for finding a path
	 */
	std::vector<v3s16> getPath(ServerEnvironment *env,
			v3s16 source,
			v3s16 destination,
			unsigned int searchdistance,
			unsigned int max_jump,
			unsigned int max_drop,
			PathAlgorithm algo);

private:
	/* helper functions */

	/**
	 * transform index pos to mappos
	 * @param ipos a index position
	 * @return map position
	 */
	v3s16          getRealPos(v3s16 ipos);

	/**
	 * transform mappos to index pos
	 * @param pos a real pos
	 * @return index position
	 */
	v3s16          getIndexPos(v3s16 pos);

	/**
	 * get gridnode at a specific index position
	 * @param ipos index position
	 * @return gridnode for index
	 */
	PathGridnode &getIndexElement(v3s16 ipos);

	/**
	 * Get gridnode at a specific index position
	 * @return gridnode for index
	 */
	PathGridnode &getIdxElem(s16 x, s16 y, s16 z);

	/**
	 * invert a 3d position
	 * @param pos 3d position
	 * @return pos *-1
	 */
	v3s16          invert(v3s16 pos);

	/**
	 * check if a index is within current search area
	 * @param index position to validate
	 * @return true/false
	 */
	bool           isValidIndex(v3s16 index);

	/**
	 * translate position to float position
	 * @param pos integer position
	 * @return float position
	 */
	v3f            tov3f(v3s16 pos);


	/* algorithm functions */

	/**
	 * calculate 2d manahttan distance to target on the xz plane
	 * @param pos position to calc distance
	 * @return integer distance
	 */
	int           getXZManhattanDist(v3s16 pos);

	/**
	 * get best direction based uppon heuristics
	 * @param directions list of unchecked directions
	 * @param g_pos mapnode to start from
	 * @return direction to check
	 */
	v3s16         getDirHeuristic(std::vector<v3s16> &directions, PathGridnode &g_pos);

	/**
	 * build internal data representation of search area
	 * @return true/false if costmap creation was successfull
	 */
	bool          buildCostmap();

	/**
	 * calculate cost of movement
	 * @param pos real world position to start movement
	 * @param dir direction to move to
	 * @return cost information
	 */
	PathCost     calcCost(v3s16 pos, v3s16 dir);

	/**
	 * recursive update whole search areas total cost information
	 * @param ipos position to check next
	 * @param srcdir positionc checked last time
	 * @param total_cost cost of moving to ipos
	 * @param level current recursion depth
	 * @return true/false path to destination has been found
	 */
	bool          updateAllCosts(v3s16 ipos, v3s16 srcdir, int current_cost, int level);

	/**
	 * recursive try to find a patrh to destionation
	 * @param ipos position to check next
	 * @param srcdir positionc checked last time
	 * @param total_cost cost of moving to ipos
	 * @param level current recursion depth
	 * @return true/false path to destination has been found
	 */
	bool          updateCostHeuristic(v3s16 ipos, v3s16 srcdir, int current_cost, int level);

	/**
	 * recursive build a vector containing all nodes from source to destination
	 * @param path vector to add nodes to
	 * @param pos pos to check next
	 * @param level recursion depth
	 */
	void          buildPath(std::vector<v3s16> &path, v3s16 pos, int level);

	/* variables */
	int m_max_index_x = 0;            /**< max index of search area in x direction  */
	int m_max_index_y = 0;            /**< max index of search area in y direction  */
	int m_max_index_z = 0;            /**< max index of search area in z direction  */


	int m_searchdistance = 0;         /**< max distance to search in each direction */
	int m_maxdrop = 0;                /**< maximum number of blocks a path may drop */
	int m_maxjump = 0;                /**< maximum number of blocks a path may jump */
	int m_min_target_distance = 0;    /**< current smalest path to target           */

	bool m_prefetch = true;              /**< prefetch cost data                       */

	v3s16 m_start;                /**< source position                          */
	v3s16 m_destination;          /**< destination position                     */

	core::aabbox3d<s16> m_limits; /**< position limits in real map coordinates  */

	/** contains all map data already collected and analyzed.
		Access it via the getIndexElement/getIdxElem methods. */
	friend class GridNodeContainer;
	GridNodeContainer *m_nodes_container = nullptr;

	ServerEnvironment *m_env = 0;     /**< minetest environment pointer             */

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
	void printType();

	/**
	 * print pathlenght for all nodes in search area
	 */
	void printPathLen();

	/**
	 * print a path
	 * @param path path to show
	 */
	void printPath(std::vector<v3s16> path);

	/**
	 * print y direction for all movements
	 */
	void printYdir();

	/**
	 * print y direction for moving in a specific direction
	 * @param dir direction to show data
	 */
	void printYdir(PathDirections dir);

	/**
	 * helper function to translate a direction to speaking text
	 * @param dir direction to translate
	 * @return textual name of direction
	 */
	std::string dirToName(PathDirections dir);
#endif
};

/******************************************************************************/
/* implementation                                                             */
/******************************************************************************/

std::vector<v3s16> get_path(ServerEnvironment* env,
							v3s16 source,
							v3s16 destination,
							unsigned int searchdistance,
							unsigned int max_jump,
							unsigned int max_drop,
							PathAlgorithm algo)
{
	Pathfinder searchclass;

	return searchclass.getPath(env,
				source, destination,
				searchdistance, max_jump, max_drop, algo);
}

/******************************************************************************/
PathCost::PathCost(const PathCost &b)
{
	valid     = b.valid;
	direction = b.direction;
	value     = b.value;
	updated   = b.updated;
}

/******************************************************************************/
PathCost &PathCost::operator= (const PathCost &b)
{
	valid     = b.valid;
	direction = b.direction;
	value     = b.value;
	updated   = b.updated;

	return *this;
}

/******************************************************************************/
PathGridnode::PathGridnode(const PathGridnode &b)
:	valid(b.valid),
	target(b.target),
	source(b.source),
	totalcost(b.totalcost),
	sourcedir(b.sourcedir),
	pos(b.pos),
	is_element(b.is_element),
	type(b.type)
{

	directions[DIR_XP] = b.directions[DIR_XP];
	directions[DIR_XM] = b.directions[DIR_XM];
	directions[DIR_ZP] = b.directions[DIR_ZP];
	directions[DIR_ZM] = b.directions[DIR_ZM];
}

/******************************************************************************/
PathGridnode &PathGridnode::operator= (const PathGridnode &b)
{
	valid      = b.valid;
	target     = b.target;
	source     = b.source;
	is_element = b.is_element;
	totalcost  = b.totalcost;
	sourcedir  = b.sourcedir;
	pos        = b.pos;
	type       = b.type;

	directions[DIR_XP] = b.directions[DIR_XP];
	directions[DIR_XM] = b.directions[DIR_XM];
	directions[DIR_ZP] = b.directions[DIR_ZP];
	directions[DIR_ZM] = b.directions[DIR_ZM];

	return *this;
}

/******************************************************************************/
PathCost PathGridnode::getCost(v3s16 dir)
{
	if (dir.X > 0) {
		return directions[DIR_XP];
	}
	if (dir.X < 0) {
		return directions[DIR_XM];
	}
	if (dir.Z > 0) {
		return directions[DIR_ZP];
	}
	if (dir.Z < 0) {
		return directions[DIR_ZM];
	}
	PathCost retval;
	return retval;
}

/******************************************************************************/
void PathGridnode::setCost(v3s16 dir, const PathCost &cost)
{
	if (dir.X > 0) {
		directions[DIR_XP] = cost;
	}
	if (dir.X < 0) {
		directions[DIR_XM] = cost;
	}
	if (dir.Z > 0) {
		directions[DIR_ZP] = cost;
	}
	if (dir.Z < 0) {
		directions[DIR_ZM] = cost;
	}
}

void GridNodeContainer::initNode(v3s16 ipos, PathGridnode *p_node)
{
	const NodeDefManager *ndef = m_pathf->m_env->getGameDef()->ndef();
	PathGridnode &elem = *p_node;

	v3s16 realpos = m_pathf->getRealPos(ipos);

	MapNode current = m_pathf->m_env->getMap().getNode(realpos);
	MapNode below   = m_pathf->m_env->getMap().getNode(realpos + v3s16(0, -1, 0));


	if ((current.param0 == CONTENT_IGNORE) ||
			(below.param0 == CONTENT_IGNORE)) {
		DEBUG_OUT("Pathfinder: " << PP(realpos) <<
			" current or below is invalid element" << std::endl);
		if (current.param0 == CONTENT_IGNORE) {
			elem.type = 'i';
			DEBUG_OUT(PP(ipos) << ": " << 'i' << std::endl);
		}
		return;
	}

	//don't add anything if it isn't an air node
	if (ndef->get(current).walkable || !ndef->get(below).walkable) {
			DEBUG_OUT("Pathfinder: " << PP(realpos)
				<< " not on surface" << std::endl);
			if (ndef->get(current).walkable) {
				elem.type = 's';
				DEBUG_OUT(PP(ipos) << ": " << 's' << std::endl);
			} else {
				elem.type = '-';
				DEBUG_OUT(PP(ipos) << ": " << '-' << std::endl);
			}
			return;
	}

	elem.valid = true;
	elem.pos   = realpos;
	elem.type  = 'g';
	DEBUG_OUT(PP(ipos) << ": " << 'a' << std::endl);

	if (m_pathf->m_prefetch) {
		elem.directions[DIR_XP] = m_pathf->calcCost(realpos, v3s16( 1, 0, 0));
		elem.directions[DIR_XM] = m_pathf->calcCost(realpos, v3s16(-1, 0, 0));
		elem.directions[DIR_ZP] = m_pathf->calcCost(realpos, v3s16( 0, 0, 1));
		elem.directions[DIR_ZM] = m_pathf->calcCost(realpos, v3s16( 0, 0,-1));
	}
}

ArrayGridNodeContainer::ArrayGridNodeContainer(Pathfinder *pathf, v3s16 dimensions) :
	m_x_stride(dimensions.Y * dimensions.Z),
	m_y_stride(dimensions.Z)
{
	m_pathf = pathf;

	m_nodes_array.resize(dimensions.X * dimensions.Y * dimensions.Z);
	INFO_TARGET << "Pathfinder ArrayGridNodeContainer constructor." << std::endl;
	for (int x = 0; x < dimensions.X; x++) {
		for (int y = 0; y < dimensions.Y; y++) {
			for (int z= 0; z < dimensions.Z; z++) {
				v3s16 ipos(x, y, z);
				initNode(ipos, &access(ipos));
			}
		}
	}
}

PathGridnode &ArrayGridNodeContainer::access(v3s16 p)
{
	return m_nodes_array[p.X * m_x_stride + p.Y * m_y_stride + p.Z];
}

MapGridNodeContainer::MapGridNodeContainer(Pathfinder *pathf)
{
	m_pathf = pathf;
}

PathGridnode &MapGridNodeContainer::access(v3s16 p)
{
	std::map<v3s16, PathGridnode>::iterator it = m_nodes.find(p);
	if (it != m_nodes.end()) {
		return it->second;
	}
	PathGridnode &n = m_nodes[p];
	initNode(p, &n);
	return n;
}



/******************************************************************************/
std::vector<v3s16> Pathfinder::getPath(ServerEnvironment *env,
							v3s16 source,
							v3s16 destination,
							unsigned int searchdistance,
							unsigned int max_jump,
							unsigned int max_drop,
							PathAlgorithm algo)
{
#ifdef PATHFINDER_CALC_TIME
	timespec ts;
	clock_gettime(CLOCK_REALTIME, &ts);
#endif
	std::vector<v3s16> retval;

	//check parameters
	if (env == 0) {
		ERROR_TARGET << "missing environment pointer" << std::endl;
		return retval;
	}

	m_searchdistance = searchdistance;
	m_env = env;
	m_maxjump = max_jump;
	m_maxdrop = max_drop;
	m_start       = source;
	m_destination = destination;
	m_min_target_distance = -1;
	m_prefetch = true;

	if (algo == PA_PLAIN_NP) {
		m_prefetch = false;
	}

	int min_x = MYMIN(source.X, destination.X);
	int max_x = MYMAX(source.X, destination.X);

	int min_y = MYMIN(source.Y, destination.Y);
	int max_y = MYMAX(source.Y, destination.Y);

	int min_z = MYMIN(source.Z, destination.Z);
	int max_z = MYMAX(source.Z, destination.Z);

	m_limits.MinEdge.X = min_x - searchdistance;
	m_limits.MinEdge.Y = min_y - searchdistance;
	m_limits.MinEdge.Z = min_z - searchdistance;

	m_limits.MaxEdge.X = max_x + searchdistance;
	m_limits.MaxEdge.Y = max_y + searchdistance;
	m_limits.MaxEdge.Z = max_z + searchdistance;

	v3s16 diff = m_limits.MaxEdge - m_limits.MinEdge;

	m_max_index_x = diff.X;
	m_max_index_y = diff.Y;
	m_max_index_z = diff.Z;

	delete m_nodes_container;
	if (diff.getLength() > 5) {
		m_nodes_container = new MapGridNodeContainer(this);
	} else {
		m_nodes_container = new ArrayGridNodeContainer(this, diff);
	}
#ifdef PATHFINDER_DEBUG
	printType();
	printCost();
	printYdir();
#endif

	//validate and mark start and end pos
	v3s16 StartIndex  = getIndexPos(source);
	v3s16 EndIndex    = getIndexPos(destination);

	PathGridnode &startpos = getIndexElement(StartIndex);
	PathGridnode &endpos   = getIndexElement(EndIndex);

	if (!startpos.valid) {
		VERBOSE_TARGET << "invalid startpos" <<
				"Index: " << PP(StartIndex) <<
				"Realpos: " << PP(getRealPos(StartIndex)) << std::endl;
		return retval;
	}
	if (!endpos.valid) {
		VERBOSE_TARGET << "invalid stoppos" <<
				"Index: " << PP(EndIndex) <<
				"Realpos: " << PP(getRealPos(EndIndex)) << std::endl;
		return retval;
	}

	endpos.target      = true;
	startpos.source    = true;
	startpos.totalcost = 0;

	bool update_cost_retval = false;

	switch (algo) {
		case PA_DIJKSTRA:
			update_cost_retval = updateAllCosts(StartIndex, v3s16(0, 0, 0), 0, 0);
			break;
		case PA_PLAIN_NP:
		case PA_PLAIN:
			update_cost_retval = updateCostHeuristic(StartIndex, v3s16(0, 0, 0), 0, 0);
			break;
		default:
			ERROR_TARGET << "missing PathAlgorithm"<< std::endl;
			break;
	}

	if (update_cost_retval) {

#ifdef PATHFINDER_DEBUG
		std::cout << "Path to target found!" << std::endl;
		printPathLen();
#endif

		//find path
		std::vector<v3s16> path;
		buildPath(path, EndIndex, 0);

#ifdef PATHFINDER_DEBUG
		std::cout << "Full index path:" << std::endl;
		printPath(path);
#endif

		//finalize path
		std::vector<v3s16> full_path;
		full_path.reserve(path.size());
		for (const v3s16 &i : path) {
			full_path.push_back(getIndexElement(i).pos);
		}

#ifdef PATHFINDER_DEBUG
		std::cout << "full path:" << std::endl;
		printPath(full_path);
#endif
#ifdef PATHFINDER_CALC_TIME
		timespec ts2;
		clock_gettime(CLOCK_REALTIME, &ts2);

		int ms = (ts2.tv_nsec - ts.tv_nsec)/(1000*1000);
		int us = ((ts2.tv_nsec - ts.tv_nsec) - (ms*1000*1000))/1000;
		int ns = ((ts2.tv_nsec - ts.tv_nsec) - ( (ms*1000*1000) + (us*1000)));


		std::cout << "Calculating path took: " << (ts2.tv_sec - ts.tv_sec) <<
				"s " << ms << "ms " << us << "us " << ns << "ns " << std::endl;
#endif
		return full_path;
	}
	else {
#ifdef PATHFINDER_DEBUG
		printPathLen();
#endif
		ERROR_TARGET << "failed to update cost map"<< std::endl;
	}


	//return
	return retval;
}

Pathfinder::~Pathfinder()
{
	delete m_nodes_container;
}
/******************************************************************************/
v3s16 Pathfinder::getRealPos(v3s16 ipos)
{
	return m_limits.MinEdge + ipos;
}

/******************************************************************************/
PathCost Pathfinder::calcCost(v3s16 pos, v3s16 dir)
{
	const NodeDefManager *ndef = m_env->getGameDef()->ndef();
	PathCost retval;

	retval.updated = true;

	v3s16 pos2 = pos + dir;

	//check limits
	if (!m_limits.isPointInside(pos2)) {
		DEBUG_OUT("Pathfinder: " << PP(pos2) <<
				" no cost -> out of limits" << std::endl);
		return retval;
	}

	MapNode node_at_pos2 = m_env->getMap().getNode(pos2);

	//did we get information about node?
	if (node_at_pos2.param0 == CONTENT_IGNORE ) {
			VERBOSE_TARGET << "Pathfinder: (1) area at pos: "
					<< PP(pos2) << " not loaded";
			return retval;
	}

	if (!ndef->get(node_at_pos2).walkable) {
		MapNode node_below_pos2 =
			m_env->getMap().getNode(pos2 + v3s16(0, -1, 0));

		//did we get information about node?
		if (node_below_pos2.param0 == CONTENT_IGNORE ) {
				VERBOSE_TARGET << "Pathfinder: (2) area at pos: "
					<< PP((pos2 + v3s16(0, -1, 0))) << " not loaded";
				return retval;
		}

		if (ndef->get(node_below_pos2).walkable) {
			retval.valid = true;
			retval.value = 1;
			retval.direction = 0;
			DEBUG_OUT("Pathfinder: "<< PP(pos)
					<< " cost same height found" << std::endl);
		}
		else {
			v3s16 testpos = pos2 - v3s16(0, -1, 0);
			MapNode node_at_pos = m_env->getMap().getNode(testpos);

			while ((node_at_pos.param0 != CONTENT_IGNORE) &&
					(!ndef->get(node_at_pos).walkable) &&
					(testpos.Y > m_limits.MinEdge.Y)) {
				testpos += v3s16(0, -1, 0);
				node_at_pos = m_env->getMap().getNode(testpos);
			}

			//did we find surface?
			if ((testpos.Y >= m_limits.MinEdge.Y) &&
					(node_at_pos.param0 != CONTENT_IGNORE) &&
					(ndef->get(node_at_pos).walkable)) {
				if ((pos2.Y - testpos.Y - 1) <= m_maxdrop) {
					retval.valid = true;
					retval.value = 2;
					//difference of y-pos +1 (target node is ABOVE solid node)
					retval.direction = ((testpos.Y - pos2.Y) +1);
					DEBUG_OUT("Pathfinder cost below height found" << std::endl);
				}
				else {
					INFO_TARGET << "Pathfinder:"
							" distance to surface below to big: "
							<< (testpos.Y - pos2.Y) << " max: " << m_maxdrop
							<< std::endl;
				}
			}
			else {
				DEBUG_OUT("Pathfinder: no surface below found" << std::endl);
			}
		}
	}
	else {
		v3s16 testpos = pos2;
		MapNode node_at_pos = m_env->getMap().getNode(testpos);

		while ((node_at_pos.param0 != CONTENT_IGNORE) &&
				(ndef->get(node_at_pos).walkable) &&
				(testpos.Y < m_limits.MaxEdge.Y)) {
			testpos += v3s16(0, 1, 0);
			node_at_pos = m_env->getMap().getNode(testpos);
		}

		//did we find surface?
		if ((testpos.Y <= m_limits.MaxEdge.Y) &&
				(!ndef->get(node_at_pos).walkable)) {

			if (testpos.Y - pos2.Y <= m_maxjump) {
				retval.valid = true;
				retval.value = 2;
				retval.direction = (testpos.Y - pos2.Y);
				DEBUG_OUT("Pathfinder cost above found" << std::endl);
			}
			else {
				DEBUG_OUT("Pathfinder: distance to surface above to big: "
						<< (testpos.Y - pos2.Y) << " max: " << m_maxjump
						<< std::endl);
			}
		}
		else {
			DEBUG_OUT("Pathfinder: no surface above found" << std::endl);
		}
	}
	return retval;
}

/******************************************************************************/
v3s16 Pathfinder::getIndexPos(v3s16 pos)
{
	return pos - m_limits.MinEdge;
}

/******************************************************************************/
PathGridnode &Pathfinder::getIndexElement(v3s16 ipos)
{
	return m_nodes_container->access(ipos);
}

/******************************************************************************/
inline PathGridnode &Pathfinder::getIdxElem(s16 x, s16 y, s16 z)
{
	return m_nodes_container->access(v3s16(x,y,z));
}

/******************************************************************************/
bool Pathfinder::isValidIndex(v3s16 index)
{
	if (	(index.X < m_max_index_x) &&
			(index.Y < m_max_index_y) &&
			(index.Z < m_max_index_z) &&
			(index.X >= 0) &&
			(index.Y >= 0) &&
			(index.Z >= 0))
		return true;

	return false;
}

/******************************************************************************/
v3s16 Pathfinder::invert(v3s16 pos)
{
	v3s16 retval = pos;

	retval.X *=-1;
	retval.Y *=-1;
	retval.Z *=-1;

	return retval;
}

/******************************************************************************/
bool Pathfinder::updateAllCosts(v3s16 ipos,
								v3s16 srcdir,
								int current_cost,
								int level)
{
	PathGridnode &g_pos = getIndexElement(ipos);
	g_pos.totalcost = current_cost;
	g_pos.sourcedir = srcdir;

	level ++;

	//check if target has been found
	if (g_pos.target) {
		m_min_target_distance = current_cost;
		DEBUG_OUT(LVL " Pathfinder: target found!" << std::endl);
		return true;
	}

	bool retval = false;

	std::vector<v3s16> directions;

	directions.emplace_back(1,0, 0);
	directions.emplace_back(-1,0, 0);
	directions.emplace_back(0,0, 1);
	directions.emplace_back(0,0,-1);

	for (v3s16 &direction : directions) {
		if (direction != srcdir) {
			PathCost cost = g_pos.getCost(direction);

			if (cost.valid) {
				direction.Y = cost.direction;

				v3s16 ipos2 = ipos + direction;

				if (!isValidIndex(ipos2)) {
					DEBUG_OUT(LVL " Pathfinder: " << PP(ipos2) <<
						" out of range, max=" << PP(m_limits.MaxEdge) << std::endl);
					continue;
				}

				PathGridnode &g_pos2 = getIndexElement(ipos2);

				if (!g_pos2.valid) {
					VERBOSE_TARGET << LVL "Pathfinder: no data for new position: "
												<< PP(ipos2) << std::endl;
					continue;
				}

				assert(cost.value > 0);

				int new_cost = current_cost + cost.value;

				// check if there already is a smaller path
				if ((m_min_target_distance > 0) &&
						(m_min_target_distance < new_cost)) {
					return false;
				}

				if ((g_pos2.totalcost < 0) ||
						(g_pos2.totalcost > new_cost)) {
					DEBUG_OUT(LVL "Pathfinder: updating path at: "<<
							PP(ipos2) << " from: " << g_pos2.totalcost << " to "<<
							new_cost << std::endl);
					if (updateAllCosts(ipos2, invert(direction),
											new_cost, level)) {
						retval = true;
						}
					}
				else {
					DEBUG_OUT(LVL "Pathfinder:"
							" already found shorter path to: "
							<< PP(ipos2) << std::endl);
				}
			}
			else {
				DEBUG_OUT(LVL "Pathfinder:"
						" not moving to invalid direction: "
						<< PP(directions[i]) << std::endl);
			}
		}
	}
	return retval;
}

/******************************************************************************/
int Pathfinder::getXZManhattanDist(v3s16 pos)
{
	int min_x = MYMIN(pos.X, m_destination.X);
	int max_x = MYMAX(pos.X, m_destination.X);
	int min_z = MYMIN(pos.Z, m_destination.Z);
	int max_z = MYMAX(pos.Z, m_destination.Z);

	return (max_x - min_x) + (max_z - min_z);
}

/******************************************************************************/
v3s16 Pathfinder::getDirHeuristic(std::vector<v3s16> &directions, PathGridnode &g_pos)
{
	int   minscore = -1;
	v3s16 retdir   = v3s16(0, 0, 0);
	v3s16 srcpos = g_pos.pos;
	DEBUG_OUT("Pathfinder: remaining dirs at beginning:"
				<< directions.size() << std::endl);

	for (v3s16 &direction : directions) {

		v3s16 pos1 = v3s16(srcpos.X + direction.X, 0, srcpos.Z+ direction.Z);

		int cur_manhattan = getXZManhattanDist(pos1);
		PathCost cost    = g_pos.getCost(direction);

		if (!cost.updated) {
			cost = calcCost(g_pos.pos, direction);
			g_pos.setCost(direction, cost);
		}

		if (cost.valid) {
			int score = cost.value + cur_manhattan;

			if ((minscore < 0)|| (score < minscore)) {
				minscore = score;
				retdir = direction;
			}
		}
	}

	if (retdir != v3s16(0, 0, 0)) {
		for (std::vector<v3s16>::iterator iter = directions.begin();
					iter != directions.end();
					++iter) {
			if(*iter == retdir) {
				DEBUG_OUT("Pathfinder: removing return direction" << std::endl);
				directions.erase(iter);
				break;
			}
		}
	}
	else {
		DEBUG_OUT("Pathfinder: didn't find any valid direction clearing"
					<< std::endl);
		directions.clear();
	}
	DEBUG_OUT("Pathfinder: remaining dirs at end:" << directions.size()
				<< std::endl);
	return retdir;
}

/******************************************************************************/
bool Pathfinder::updateCostHeuristic(	v3s16 ipos,
										v3s16 srcdir,
										int current_cost,
										int level)
{

	PathGridnode &g_pos = getIndexElement(ipos);
	g_pos.totalcost = current_cost;
	g_pos.sourcedir = srcdir;

	level ++;

	//check if target has been found
	if (g_pos.target) {
		m_min_target_distance = current_cost;
		DEBUG_OUT(LVL " Pathfinder: target found!" << std::endl);
		return true;
	}

	bool retval = false;

	std::vector<v3s16> directions;

	directions.emplace_back(1, 0,  0);
	directions.emplace_back(-1, 0,  0);
	directions.emplace_back(0, 0,  1);
	directions.emplace_back(0, 0, -1);

	v3s16 direction = getDirHeuristic(directions, g_pos);

	while (direction != v3s16(0, 0, 0) && (!retval)) {

		if (direction != srcdir) {
			PathCost cost = g_pos.getCost(direction);

			if (cost.valid) {
				direction.Y = cost.direction;

				v3s16 ipos2 = ipos + direction;

				if (!isValidIndex(ipos2)) {
					DEBUG_OUT(LVL " Pathfinder: " << PP(ipos2) <<
						" out of range, max=" << PP(m_limits.MaxEdge) << std::endl);
					direction = getDirHeuristic(directions, g_pos);
					continue;
				}

				PathGridnode &g_pos2 = getIndexElement(ipos2);

				if (!g_pos2.valid) {
					VERBOSE_TARGET << LVL "Pathfinder: no data for new position: "
												<< PP(ipos2) << std::endl;
					direction = getDirHeuristic(directions, g_pos);
					continue;
				}

				assert(cost.value > 0);

				int new_cost = current_cost + cost.value;

				// check if there already is a smaller path
				if ((m_min_target_distance > 0) &&
						(m_min_target_distance < new_cost)) {
					DEBUG_OUT(LVL "Pathfinder:"
							" already longer than best already found path "
							<< PP(ipos2) << std::endl);
					return false;
				}

				if ((g_pos2.totalcost < 0) ||
						(g_pos2.totalcost > new_cost)) {
					DEBUG_OUT(LVL "Pathfinder: updating path at: "<<
							PP(ipos2) << " from: " << g_pos2.totalcost << " to "<<
							new_cost << " srcdir=" <<
							PP(invert(direction))<< std::endl);
					if (updateCostHeuristic(ipos2, invert(direction),
											new_cost, level)) {
						retval = true;
						}
					}
				else {
					DEBUG_OUT(LVL "Pathfinder:"
							" already found shorter path to: "
							<< PP(ipos2) << std::endl);
				}
			}
			else {
				DEBUG_OUT(LVL "Pathfinder:"
						" not moving to invalid direction: "
						<< PP(direction) << std::endl);
			}
		}
		else {
			DEBUG_OUT(LVL "Pathfinder:"
							" skipping srcdir: "
							<< PP(direction) << std::endl);
		}
		direction = getDirHeuristic(directions, g_pos);
	}
	return retval;
}

/******************************************************************************/
void Pathfinder::buildPath(std::vector<v3s16> &path, v3s16 pos, int level)
{
	level ++;
	if (level > 700) {
		ERROR_TARGET
			<< LVL "Pathfinder: path is too long aborting" << std::endl;
		return;
	}

	PathGridnode &g_pos = getIndexElement(pos);
	if (!g_pos.valid) {
		ERROR_TARGET
			<< LVL "Pathfinder: invalid next pos detected aborting" << std::endl;
		return;
	}

	g_pos.is_element = true;

	//check if source reached
	if (g_pos.source) {
		path.push_back(pos);
		return;
	}

	buildPath(path, pos + g_pos.sourcedir, level);
	path.push_back(pos);
}

/******************************************************************************/
v3f Pathfinder::tov3f(v3s16 pos)
{
	return v3f(BS * pos.X, BS * pos.Y, BS * pos.Z);
}

#ifdef PATHFINDER_DEBUG

/******************************************************************************/
void Pathfinder::printCost()
{
	printCost(DIR_XP);
	printCost(DIR_XM);
	printCost(DIR_ZP);
	printCost(DIR_ZM);
}

/******************************************************************************/
void Pathfinder::printYdir()
{
	printYdir(DIR_XP);
	printYdir(DIR_XM);
	printYdir(DIR_ZP);
	printYdir(DIR_ZM);
}

/******************************************************************************/
void Pathfinder::printCost(PathDirections dir)
{
	std::cout << "Cost in direction: " << dirToName(dir) << std::endl;
	std::cout << std::setfill('-') << std::setw(80) << "-" << std::endl;
	std::cout << std::setfill(' ');
	for (int y = 0; y < m_max_index_y; y++) {

		std::cout << "Level: " << y << std::endl;

		std::cout << std::setw(4) << " " << "  ";
		for (int x = 0; x < m_max_index_x; x++) {
			std::cout << std::setw(4) << x;
		}
		std::cout << std::endl;

		for (int z = 0; z < m_max_index_z; z++) {
			std::cout << std::setw(4) << z <<": ";
			for (int x = 0; x < m_max_index_x; x++) {
				if (getIdxElem(x, y, z).directions[dir].valid)
					std::cout << std::setw(4)
						<< getIdxElem(x, y, z).directions[dir].value;
				else
					std::cout << std::setw(4) << "-";
				}
			std::cout << std::endl;
		}
		std::cout << std::endl;
	}
}

/******************************************************************************/
void Pathfinder::printYdir(PathDirections dir)
{
	std::cout << "Height difference in direction: " << dirToName(dir) << std::endl;
	std::cout << std::setfill('-') << std::setw(80) << "-" << std::endl;
	std::cout << std::setfill(' ');
	for (int y = 0; y < m_max_index_y; y++) {

		std::cout << "Level: " << y << std::endl;

		std::cout << std::setw(4) << " " << "  ";
		for (int x = 0; x < m_max_index_x; x++) {
			std::cout << std::setw(4) << x;
		}
		std::cout << std::endl;

		for (int z = 0; z < m_max_index_z; z++) {
			std::cout << std::setw(4) << z <<": ";
			for (int x = 0; x < m_max_index_x; x++) {
				if (getIdxElem(x, y, z).directions[dir].valid)
					std::cout << std::setw(4)
						<< getIdxElem(x, y, z).directions[dir].direction;
				else
					std::cout << std::setw(4) << "-";
				}
			std::cout << std::endl;
		}
		std::cout << std::endl;
	}
}

/******************************************************************************/
void Pathfinder::printType()
{
	std::cout << "Type of node:" << std::endl;
	std::cout << std::setfill('-') << std::setw(80) << "-" << std::endl;
	std::cout << std::setfill(' ');
	for (int y = 0; y < m_max_index_y; y++) {

		std::cout << "Level: " << y << std::endl;

		std::cout << std::setw(3) << " " << "  ";
		for (int x = 0; x < m_max_index_x; x++) {
			std::cout << std::setw(3) << x;
		}
		std::cout << std::endl;

		for (int z = 0; z < m_max_index_z; z++) {
			std::cout << std::setw(3) << z <<": ";
			for (int x = 0; x < m_max_index_x; x++) {
				char toshow = getIdxElem(x, y, z).type;
				std::cout << std::setw(3) << toshow;
			}
			std::cout << std::endl;
		}
		std::cout << std::endl;
	}
	std::cout << std::endl;
}

/******************************************************************************/
void Pathfinder::printPathLen()
{
	std::cout << "Pathlen:" << std::endl;
		std::cout << std::setfill('-') << std::setw(80) << "-" << std::endl;
		std::cout << std::setfill(' ');
		for (int y = 0; y < m_max_index_y; y++) {

			std::cout << "Level: " << y << std::endl;

			std::cout << std::setw(3) << " " << "  ";
			for (int x = 0; x < m_max_index_x; x++) {
				std::cout << std::setw(3) << x;
			}
			std::cout << std::endl;

			for (int z = 0; z < m_max_index_z; z++) {
				std::cout << std::setw(3) << z <<": ";
				for (int x = 0; x < m_max_index_x; x++) {
					std::cout << std::setw(3) << getIdxElem(x, y, z).totalcost;
				}
				std::cout << std::endl;
			}
			std::cout << std::endl;
		}
		std::cout << std::endl;
}

/******************************************************************************/
std::string Pathfinder::dirToName(PathDirections dir)
{
	switch (dir) {
	case DIR_XP:
		return "XP";
		break;
	case DIR_XM:
		return "XM";
		break;
	case DIR_ZP:
		return "ZP";
		break;
	case DIR_ZM:
		return "ZM";
		break;
	default:
		return "UKN";
	}
}

/******************************************************************************/
void Pathfinder::printPath(std::vector<v3s16> path)
{
	unsigned int current = 0;
	for (std::vector<v3s16>::iterator i = path.begin();
			i != path.end(); ++i) {
		std::cout << std::setw(3) << current << ":" << PP((*i)) << std::endl;
		current++;
	}
}

#endif
